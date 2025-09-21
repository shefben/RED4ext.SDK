#include "DialogSystemSync.hpp"
#include "../core/Logger.hpp"
#include "../net/Net.hpp"
#include "../quest/EnhancedQuestManager.hpp"
#include <algorithm>
#include <sstream>
#include <cmath>

namespace CoopNet {

DialogSystemSync& DialogSystemSync::Instance() {
    static DialogSystemSync instance;
    return instance;
}

bool DialogSystemSync::Initialize() {
    if (m_initialized) {
        return true;
    }

    Logger::Log(LogLevel::INFO, "[DialogSystemSync] Initializing dialog synchronization system");

    // Initialize data structures
    m_activeSessions.clear();
    m_npcStates.clear();
    m_playerPositions.clear();
    m_questChoiceRequirements.clear();
    m_dialogHistory.clear();
    m_dialogCheckpoints.clear();

    // Reset statistics
    m_stats = DialogSystemStats{};
    m_stats.lastStatsUpdate = std::chrono::steady_clock::now();

    // Initialize configuration with defaults
    m_config = DialogConfig{};

    m_initialized = true;
    Logger::Log(LogLevel::INFO, "[DialogSystemSync] Dialog system initialized successfully");

    // Register for quest system events
    auto& questManager = EnhancedQuestManager::Instance();
    questManager.RegisterEventCallback("quest_started",
        [this](uint32_t questHash, uint32_t playerId, const std::string& data) {
            // Quest started - prepare any required dialog choices
            Logger::Log(LogLevel::DEBUG, "[DialogSystemSync] Quest " + std::to_string(questHash) + " started by player " + std::to_string(playerId));
        });

    return true;
}

void DialogSystemSync::Shutdown() {
    if (!m_initialized) {
        return;
    }

    Logger::Log(LogLevel::INFO, "[DialogSystemSync] Shutting down dialog system");

    // End all active sessions
    {
        std::lock_guard<std::mutex> lock(m_sessionsMutex);
        for (const auto& [sessionId, session] : m_activeSessions) {
            EndDialog(sessionId, true); // Force end
        }
        m_activeSessions.clear();
    }

    // Clear all data structures
    {
        std::lock_guard<std::mutex> npcLock(m_npcStatesMutex);
        std::lock_guard<std::mutex> posLock(m_positionsMutex);
        std::lock_guard<std::mutex> histLock(m_historyMutex);

        m_npcStates.clear();
        m_playerPositions.clear();
        m_questChoiceRequirements.clear();
        m_dialogHistory.clear();
        m_dialogCheckpoints.clear();
    }

    m_initialized = false;
    Logger::Log(LogLevel::INFO, "[DialogSystemSync] Dialog system shutdown complete");
}

void DialogSystemSync::Tick(float deltaTime) {
    if (!m_initialized) {
        return;
    }

    ProcessActiveSessions();
    ProcessVoting();
    ProcessTimeouts();
    UpdateStatistics();
}

bool DialogSystemSync::StartDialog(uint32_t npcId, uint32_t speakerId, uint32_t questHash, DialogSyncMode syncMode) {
    // Check if NPC is already in dialog
    if (IsNPCInDialog(npcId)) {
        Logger::Log(LogLevel::WARNING, "[DialogSystemSync] NPC " + std::to_string(npcId) + " is already in dialog");
        return false;
    }

    // Create new dialog session
    uint32_t sessionId = GenerateSessionId();
    if (!CreateSession(npcId, speakerId, questHash, syncMode)) {
        return false;
    }

    auto session = GetDialogSession(sessionId);
    if (!session) {
        return false;
    }

    // Add primary speaker as participant
    DialogParticipant speaker;
    speaker.playerId = speakerId;
    speaker.role = DialogRole::Speaker;
    speaker.canVote = true;
    speaker.hasVoted = false;
    speaker.distanceToSpeaker = 0.0f;
    speaker.meetsRequirements = true;
    speaker.lastActivity = GetCurrentTimestamp();

    session->participants[speakerId] = speaker;
    session->primarySpeaker = speakerId;
    session->state = DialogState::Starting;

    // Update NPC state
    {
        std::lock_guard<std::mutex> npcLock(m_npcStatesMutex);
        NPCDialogState& npcState = m_npcStates[npcId];
        npcState.npcId = npcId;
        npcState.currentDialog = sessionId;
        npcState.state = DialogState::Starting;
        npcState.activeSpeaker = speakerId;
        npcState.dialogStartTime = GetCurrentTimestamp();
        npcState.lastUpdateTime = npcState.dialogStartTime;
        session->npcState = npcState;
    }

    // Broadcast dialog start to nearby players
    BroadcastDialogUpdate(sessionId, "dialog_started",
                         "npc:" + std::to_string(npcId) + ",speaker:" + std::to_string(speakerId));

    // Record in history
    if (m_config.recordDialogHistory) {
        RecordDialogHistory(sessionId, "dialog_started",
                           "npc:" + std::to_string(npcId) + ",speaker:" + std::to_string(speakerId) +
                           ",quest:" + std::to_string(questHash));
    }

    m_stats.totalDialogs++;
    m_stats.activeDialogs++;

    Logger::Log(LogLevel::INFO, "[DialogSystemSync] Started dialog session " + std::to_string(sessionId) +
               " with NPC " + std::to_string(npcId) + " (speaker: " + std::to_string(speakerId) + ")");

    TriggerEvent("dialog_started", sessionId,
                "npc:" + std::to_string(npcId) + ",speaker:" + std::to_string(speakerId));

    return true;
}

bool DialogSystemSync::EndDialog(uint32_t sessionId, bool force) {
    auto session = GetDialogSession(sessionId);
    if (!session) {
        return false;
    }

    // Check if dialog can be ended (unless forced)
    if (!force && session->activeVote && session->activeVote->status == VoteStatus::InProgress) {
        Logger::Log(LogLevel::WARNING, "[DialogSystemSync] Cannot end dialog " + std::to_string(sessionId) +
                   " - vote in progress");
        return false;
    }

    session->state = DialogState::Ending;

    // Update NPC state
    {
        std::lock_guard<std::mutex> npcLock(m_npcStatesMutex);
        auto npcIt = m_npcStates.find(session->npcId);
        if (npcIt != m_npcStates.end()) {
            npcIt->second.state = DialogState::Ending;
            npcIt->second.lastUpdateTime = GetCurrentTimestamp();
        }
    }

    // Notify all participants
    NotifyParticipants(sessionId, "dialog_ending", force ? "forced" : "normal");

    // Record completion
    if (m_config.recordDialogHistory) {
        RecordDialogHistory(sessionId, "dialog_ended", force ? "forced" : "completed");
    }

    // Update statistics
    uint64_t dialogDuration = GetCurrentTimestamp() - session->sessionStartTime;
    float durationSeconds = dialogDuration / 1000.0f;

    if (force || session->state == DialogState::Interrupted) {
        m_stats.interruptedDialogs++;
    } else {
        m_stats.completedDialogs++;
        m_stats.averageDialogDuration = (m_stats.averageDialogDuration + durationSeconds) / 2.0f;
    }

    // Cleanup session
    CleanupSession(sessionId);

    m_stats.activeDialogs--;

    Logger::Log(LogLevel::INFO, "[DialogSystemSync] Ended dialog session " + std::to_string(sessionId) +
               " (duration: " + std::to_string(durationSeconds) + "s, force: " + std::to_string(force) + ")");

    TriggerEvent("dialog_ended", sessionId,
                "duration:" + std::to_string(durationSeconds) + ",force:" + std::to_string(force));

    return true;
}

bool DialogSystemSync::AddParticipant(uint32_t sessionId, uint32_t playerId, DialogRole role) {
    auto session = GetDialogSession(sessionId);
    if (!session) {
        return false;
    }

    // Check if already participating
    if (session->participants.find(playerId) != session->participants.end()) {
        Logger::Log(LogLevel::WARNING, "[DialogSystemSync] Player " + std::to_string(playerId) +
                   " already participating in dialog " + std::to_string(sessionId));
        return false;
    }

    // Check participant limits
    if (session->participants.size() >= m_config.maxParticipants) {
        Logger::Log(LogLevel::WARNING, "[DialogSystemSync] Dialog " + std::to_string(sessionId) +
                   " has reached maximum participants");
        return false;
    }

    // Check if player meets requirements
    if (!CanPlayerParticipate(sessionId, playerId)) {
        Logger::Log(LogLevel::DEBUG, "[DialogSystemSync] Player " + std::to_string(playerId) +
                   " does not meet requirements for dialog " + std::to_string(sessionId));
        return false;
    }

    // Create participant
    DialogParticipant participant;
    participant.playerId = playerId;
    participant.role = role;
    participant.canVote = (role == DialogRole::Speaker || role == DialogRole::Participant);
    participant.hasVoted = false;
    participant.distanceToSpeaker = CalculateDistance(playerId, session->primarySpeaker);
    participant.meetsRequirements = ValidateParticipantRequirements(sessionId, playerId);
    participant.lastActivity = GetCurrentTimestamp();

    session->participants[playerId] = participant;

    // Notify other participants
    NotifyParticipants(sessionId, "participant_added",
                      "player:" + std::to_string(playerId) + ",role:" + DialogUtils::GetRoleName(role));

    Logger::Log(LogLevel::DEBUG, "[DialogSystemSync] Added player " + std::to_string(playerId) +
               " to dialog " + std::to_string(sessionId) + " as " + DialogUtils::GetRoleName(role));

    TriggerEvent("participant_added", sessionId,
                "player:" + std::to_string(playerId) + ",role:" + DialogUtils::GetRoleName(role));

    return true;
}

bool DialogSystemSync::SelectDialogChoice(uint32_t sessionId, uint32_t playerId, uint32_t choiceId) {
    auto session = GetDialogSession(sessionId);
    if (!session) {
        return false;
    }

    // Check if player can make choices
    auto participantIt = session->participants.find(playerId);
    if (participantIt == session->participants.end()) {
        Logger::Log(LogLevel::WARNING, "[DialogSystemSync] Player " + std::to_string(playerId) +
                   " not participating in dialog " + std::to_string(sessionId));
        return false;
    }

    DialogParticipant& participant = participantIt->second;
    if (participant.role != DialogRole::Speaker && participant.role != DialogRole::Participant) {
        Logger::Log(LogLevel::WARNING, "[DialogSystemSync] Player " + std::to_string(playerId) +
                   " cannot make choices with role " + DialogUtils::GetRoleName(participant.role));
        return false;
    }

    // Validate choice
    if (!ValidateChoice(sessionId, playerId, choiceId)) {
        Logger::Log(LogLevel::WARNING, "[DialogSystemSync] Invalid choice " + std::to_string(choiceId) +
                   " for player " + std::to_string(playerId) + " in dialog " + std::to_string(sessionId));
        return false;
    }

    // Find the choice
    DialogChoice* selectedChoice = nullptr;
    for (auto& choice : session->availableChoices) {
        if (choice.choiceId == choiceId) {
            selectedChoice = &choice;
            break;
        }
    }

    if (!selectedChoice) {
        Logger::Log(LogLevel::ERROR, "[DialogSystemSync] Choice " + std::to_string(choiceId) + " not found");
        return false;
    }

    // Check if choice requires consensus/voting
    bool needsVote = false;
    switch (session->syncMode) {
        case DialogSyncMode::Speaker:
            needsVote = (participant.role != DialogRole::Speaker && selectedChoice->requiresConsensus);
            break;
        case DialogSyncMode::Majority:
        case DialogSyncMode::Consensus:
        case DialogSyncMode::Proximity:
            needsVote = true;
            break;
        case DialogSyncMode::Quest:
            needsVote = selectedChoice->isQuestCritical;
            break;
        case DialogSyncMode::Individual:
            needsVote = false;
            break;
    }

    if (needsVote) {
        // Start voting process
        if (session->activeVote && session->activeVote->status == VoteStatus::InProgress) {
            Logger::Log(LogLevel::WARNING, "[DialogSystemSync] Vote already in progress for dialog " +
                       std::to_string(sessionId));
            return false;
        }

        return StartChoiceVote(sessionId, choiceId, playerId);
    } else {
        // Apply choice immediately
        session->state = DialogState::Processing;
        participant.lastActivity = GetCurrentTimestamp();

        // Apply choice consequences
        ApplyChoiceConsequences(sessionId, *selectedChoice);

        // Update dialog flow
        UpdateDialogFlow(sessionId, choiceId);

        // Record choice
        if (m_config.recordDialogHistory) {
            RecordDialogHistory(sessionId, "choice_selected",
                               "player:" + std::to_string(playerId) + ",choice:" + std::to_string(choiceId) +
                               ",text:" + selectedChoice->choiceText);
        }

        // Notify participants
        NotifyParticipants(sessionId, "choice_selected",
                          "player:" + std::to_string(playerId) + ",choice:" + std::to_string(choiceId));

        Logger::Log(LogLevel::INFO, "[DialogSystemSync] Player " + std::to_string(playerId) +
                   " selected choice " + std::to_string(choiceId) + " in dialog " + std::to_string(sessionId));

        TriggerEvent("choice_selected", sessionId,
                    "player:" + std::to_string(playerId) + ",choice:" + std::to_string(choiceId));

        return true;
    }
}

bool DialogSystemSync::StartChoiceVote(uint32_t sessionId, uint32_t choiceId, uint32_t initiatingPlayer) {
    auto session = GetDialogSession(sessionId);
    if (!session) {
        return false;
    }

    // Check if vote is already in progress
    if (session->activeVote && session->activeVote->status == VoteStatus::InProgress) {
        return false;
    }

    // Count eligible voters
    uint32_t eligibleVoters = 0;
    for (const auto& [playerId, participant] : session->participants) {
        if (participant.canVote && participant.meetsRequirements) {
            eligibleVoters++;
        }
    }

    if (eligibleVoters == 0) {
        Logger::Log(LogLevel::ERROR, "[DialogSystemSync] No eligible voters for dialog " + std::to_string(sessionId));
        return false;
    }

    // Create vote
    auto vote = std::make_unique<DialogVote>();
    vote->voteId = m_nextVoteId.fetch_add(1);
    vote->dialogId = sessionId;
    vote->choiceId = choiceId;
    vote->syncMode = session->syncMode;
    vote->status = VoteStatus::InProgress;
    vote->initiatingPlayer = initiatingPlayer;
    vote->startTime = GetCurrentTimestamp();
    vote->voteDeadline = vote->startTime + static_cast<uint64_t>(m_config.defaultVoteTimeout * 1000);
    vote->requiredVotes = DialogUtils::CalculateVoteRequirement(session->syncMode, eligibleVoters);
    vote->currentVotes = 0;
    vote->voteReason = "Choice selection";

    // Auto-vote for initiating player
    vote->playerVotes[initiatingPlayer] = true;
    vote->currentVotes = 1;

    session->activeVote = std::move(vote);
    session->state = DialogState::AwaitingChoice;

    // Notify participants about vote
    NotifyParticipants(sessionId, "vote_started",
                      "choice:" + std::to_string(choiceId) + ",initiator:" + std::to_string(initiatingPlayer) +
                      ",timeout:" + std::to_string(m_config.defaultVoteTimeout));

    m_stats.totalVotes++;

    Logger::Log(LogLevel::INFO, "[DialogSystemSync] Started vote for choice " + std::to_string(choiceId) +
               " in dialog " + std::to_string(sessionId) + " (required: " +
               std::to_string(session->activeVote->requiredVotes) + "/" + std::to_string(eligibleVoters) + ")");

    TriggerEvent("vote_started", sessionId,
                "choice:" + std::to_string(choiceId) + ",required:" + std::to_string(session->activeVote->requiredVotes));

    return true;
}

bool DialogSystemSync::CastVote(uint32_t sessionId, uint32_t playerId, bool approve) {
    auto session = GetDialogSession(sessionId);
    if (!session || !session->activeVote) {
        return false;
    }

    DialogVote* vote = session->activeVote.get();
    if (vote->status != VoteStatus::InProgress) {
        return false;
    }

    // Check if player can vote
    auto participantIt = session->participants.find(playerId);
    if (participantIt == session->participants.end() || !participantIt->second.canVote) {
        return false;
    }

    // Check if already voted
    if (vote->playerVotes.find(playerId) != vote->playerVotes.end()) {
        Logger::Log(LogLevel::WARNING, "[DialogSystemSync] Player " + std::to_string(playerId) +
                   " already voted in dialog " + std::to_string(sessionId));
        return false;
    }

    // Record vote
    vote->playerVotes[playerId] = approve;
    if (approve) {
        vote->currentVotes++;
    }

    participantIt->second.hasVoted = true;
    participantIt->second.votedChoice = approve ? vote->choiceId : 0;
    participantIt->second.lastActivity = GetCurrentTimestamp();

    Logger::Log(LogLevel::DEBUG, "[DialogSystemSync] Player " + std::to_string(playerId) +
               " voted " + (approve ? "YES" : "NO") + " for choice " + std::to_string(vote->choiceId) +
               " (" + std::to_string(vote->currentVotes) + "/" + std::to_string(vote->requiredVotes) + ")");

    // Check if vote passes
    if (CheckVoteRequirements(vote)) {
        ProcessVoteResult(vote);
    }

    // Notify participants of vote
    NotifyParticipants(sessionId, "vote_cast",
                      "player:" + std::to_string(playerId) + ",approve:" + std::to_string(approve) +
                      ",current:" + std::to_string(vote->currentVotes) +
                      ",required:" + std::to_string(vote->requiredVotes));

    return true;
}

bool DialogSystemSync::ProcessVoteResult(DialogVote* vote) {
    if (!vote) {
        return false;
    }

    auto session = GetDialogSession(vote->dialogId);
    if (!session) {
        return false;
    }

    bool votePassses = (vote->currentVotes >= vote->requiredVotes);
    CompleteVote(vote, votePassses);

    if (votePassses) {
        // Apply the chosen option
        DialogChoice* selectedChoice = nullptr;
        for (auto& choice : session->availableChoices) {
            if (choice.choiceId == vote->choiceId) {
                selectedChoice = &choice;
                break;
            }
        }

        if (selectedChoice) {
            ApplyChoiceConsequences(vote->dialogId, *selectedChoice);
            UpdateDialogFlow(vote->dialogId, vote->choiceId);

            if (m_config.recordDialogHistory) {
                RecordDialogHistory(vote->dialogId, "choice_voted",
                                   "choice:" + std::to_string(vote->choiceId) +
                                   ",votes:" + std::to_string(vote->currentVotes) +
                                   ",result:passed");
            }
        }

        m_stats.passedVotes++;
        Logger::Log(LogLevel::INFO, "[DialogSystemSync] Vote PASSED for choice " + std::to_string(vote->choiceId) +
                   " in dialog " + std::to_string(vote->dialogId));
    } else {
        m_stats.failedVotes++;
        Logger::Log(LogLevel::INFO, "[DialogSystemSync] Vote FAILED for choice " + std::to_string(vote->choiceId) +
                   " in dialog " + std::to_string(vote->dialogId));
    }

    // Calculate vote duration
    uint64_t voteDuration = GetCurrentTimestamp() - vote->startTime;
    m_stats.averageVoteDuration = (m_stats.averageVoteDuration + (voteDuration / 1000.0f)) / 2.0f;

    // Clear the vote
    session->activeVote.reset();
    session->state = DialogState::Active;

    NotifyParticipants(vote->dialogId, "vote_completed",
                      "choice:" + std::to_string(vote->choiceId) + ",result:" + (votePassses ? "passed" : "failed"));

    TriggerEvent("vote_completed", vote->dialogId,
                "choice:" + std::to_string(vote->choiceId) + ",result:" + (votePassses ? "passed" : "failed"));

    return true;
}

void DialogSystemSync::ProcessTimeouts() {
    std::vector<uint32_t> sessionsToTimeout;
    std::vector<uint32_t> votesToTimeout;
    uint64_t currentTime = GetCurrentTimestamp();

    // Check for session timeouts
    {
        std::lock_guard<std::mutex> lock(m_sessionsMutex);
        for (const auto& [sessionId, session] : m_activeSessions) {
            if (session->state == DialogState::Active || session->state == DialogState::AwaitingChoice) {
                uint64_t sessionAge = currentTime - session->lastActivityTime;
                if (sessionAge > static_cast<uint64_t>(m_config.dialogTimeout * 1000)) {
                    sessionsToTimeout.push_back(sessionId);
                }
            }

            // Check vote timeouts
            if (session->activeVote && session->activeVote->status == VoteStatus::InProgress) {
                if (currentTime >= session->activeVote->voteDeadline) {
                    votesToTimeout.push_back(sessionId);
                }
            }
        }
    }

    // Process session timeouts
    for (uint32_t sessionId : sessionsToTimeout) {
        Logger::Log(LogLevel::WARNING, "[DialogSystemSync] Dialog " + std::to_string(sessionId) + " timed out");
        EndDialog(sessionId, true);
        TriggerEvent("dialog_timeout", sessionId, "reason:inactivity");
    }

    // Process vote timeouts
    for (uint32_t sessionId : votesToTimeout) {
        auto session = GetDialogSession(sessionId);
        if (session && session->activeVote) {
            Logger::Log(LogLevel::WARNING, "[DialogSystemSync] Vote timed out for dialog " + std::to_string(sessionId));

            session->activeVote->status = VoteStatus::Timeout;
            CompleteVote(session->activeVote.get(), false);
            m_stats.timeoutVotes++;

            NotifyParticipants(sessionId, "vote_timeout", "choice:" + std::to_string(session->activeVote->choiceId));
            TriggerEvent("vote_timeout", sessionId, "choice:" + std::to_string(session->activeVote->choiceId));

            session->activeVote.reset();
            session->state = DialogState::Active;
        }
    }
}

bool DialogSystemSync::ValidateChoice(uint32_t sessionId, uint32_t playerId, uint32_t choiceId) const {
    auto session = GetDialogSession(sessionId);
    if (!session) {
        return false;
    }

    // Find the choice
    const DialogChoice* choice = nullptr;
    for (const auto& availableChoice : session->availableChoices) {
        if (availableChoice.choiceId == choiceId) {
            choice = &availableChoice;
            break;
        }
    }

    if (!choice) {
        return false;
    }

    // Check skill requirements
    if (m_config.enableSkillChecks && !CheckSkillRequirement(playerId, *choice)) {
        return false;
    }

    // Check quest requirements
    if (m_config.enableQuestRequirements && !CheckQuestRequirement(playerId, *choice)) {
        return false;
    }

    return true;
}

bool DialogSystemSync::CheckSkillRequirement(uint32_t playerId, const DialogChoice& choice) const {
    if (choice.requiredSkill == 0 && choice.requiredLevel == 0) {
        return true; // No requirements
    }

    // In a real implementation, this would check the player's actual skills
    // For now, we'll just return true to allow all choices
    // TODO: Integrate with actual player skill system
    return true;
}

bool DialogSystemSync::CheckQuestRequirement(uint32_t playerId, const DialogChoice& choice) const {
    if (choice.questRequirement == 0) {
        return true; // No quest requirement
    }

    // Check with quest system
    auto& questManager = EnhancedQuestManager::Instance();
    return questManager.CanStartQuest(choice.questRequirement, playerId);
}

bool DialogSystemSync::UpdatePlayerPosition(uint32_t playerId, float x, float y, float z) {
    std::lock_guard<std::mutex> lock(m_positionsMutex);
    m_playerPositions[playerId] = {x, y, z};
    return true;
}

float DialogSystemSync::CalculateDistance(uint32_t playerId1, uint32_t playerId2) const {
    std::lock_guard<std::mutex> lock(m_positionsMutex);

    auto pos1It = m_playerPositions.find(playerId1);
    auto pos2It = m_playerPositions.find(playerId2);

    if (pos1It == m_playerPositions.end() || pos2It == m_playerPositions.end()) {
        return 1000.0f; // Very far if position unknown
    }

    const auto& pos1 = pos1It->second;
    const auto& pos2 = pos2It->second;

    float dx = pos1[0] - pos2[0];
    float dy = pos1[1] - pos2[1];
    float dz = pos1[2] - pos2[2];

    return std::sqrt(dx*dx + dy*dy + dz*dz);
}

bool DialogSystemSync::CanPlayerParticipate(uint32_t sessionId, uint32_t playerId) const {
    auto session = GetDialogSession(sessionId);
    if (!session) {
        return false;
    }

    // Check proximity requirement
    if (m_config.enableProximityCheck && session->syncMode == DialogSyncMode::Proximity) {
        float distance = CalculateDistance(playerId, session->primarySpeaker);
        if (distance > session->proximityRange) {
            return false;
        }
    }

    // Check if spectators are allowed
    if (!session->allowSpectators && session->participants.size() >= 2) {
        return false;
    }

    return true;
}

// Utility method implementations
uint32_t DialogSystemSync::GenerateSessionId() {
    return m_nextSessionId.fetch_add(1);
}

bool DialogSystemSync::CreateSession(uint32_t npcId, uint32_t speakerId, uint32_t questHash, DialogSyncMode syncMode) {
    uint32_t sessionId = GenerateSessionId();

    auto session = std::make_shared<DialogSession>();
    session->sessionId = sessionId;
    session->npcId = npcId;
    session->questHash = questHash;
    session->syncMode = syncMode;
    session->state = DialogState::Starting;
    session->primarySpeaker = speakerId;
    session->sessionStartTime = GetCurrentTimestamp();
    session->lastActivityTime = session->sessionStartTime;
    session->allowSpectators = m_config.allowSpectatorMode;
    session->recordChoices = m_config.recordDialogHistory;
    session->proximityRange = m_config.proximityRange;

    {
        std::lock_guard<std::mutex> lock(m_sessionsMutex);
        m_activeSessions[sessionId] = session;
    }

    return true;
}

void DialogSystemSync::TriggerEvent(const std::string& eventType, uint32_t sessionId, const std::string& data) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);

    auto it = m_eventCallbacks.find(eventType);
    if (it != m_eventCallbacks.end()) {
        for (const auto& callback : it->second) {
            try {
                callback(sessionId, eventType, data);
            } catch (const std::exception& e) {
                Logger::Log(LogLevel::ERROR, "[DialogSystemSync] Event callback error: " + std::string(e.what()));
            }
        }
    }
}

uint64_t DialogSystemSync::GetCurrentTimestamp() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

std::shared_ptr<DialogSession> DialogSystemSync::GetDialogSession(uint32_t sessionId) const {
    std::lock_guard<std::mutex> lock(m_sessionsMutex);
    auto it = m_activeSessions.find(sessionId);
    return (it != m_activeSessions.end()) ? it->second : nullptr;
}

DialogSystemStats DialogSystemSync::GetStats() const {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    return m_stats;
}

// DialogUtils namespace implementation
namespace DialogUtils {
    std::string GetSyncModeName(DialogSyncMode mode) {
        switch (mode) {
            case DialogSyncMode::Speaker: return "Speaker";
            case DialogSyncMode::Majority: return "Majority";
            case DialogSyncMode::Consensus: return "Consensus";
            case DialogSyncMode::Proximity: return "Proximity";
            case DialogSyncMode::Quest: return "Quest";
            case DialogSyncMode::Individual: return "Individual";
            default: return "Unknown";
        }
    }

    std::string GetRoleName(DialogRole role) {
        switch (role) {
            case DialogRole::Speaker: return "Speaker";
            case DialogRole::Listener: return "Listener";
            case DialogRole::Participant: return "Participant";
            case DialogRole::Excluded: return "Excluded";
            default: return "Unknown";
        }
    }

    std::string GetChoiceTypeName(DialogChoiceType type) {
        switch (type) {
            case DialogChoiceType::Normal: return "Normal";
            case DialogChoiceType::Skill: return "Skill";
            case DialogChoiceType::Romance: return "Romance";
            case DialogChoiceType::Aggressive: return "Aggressive";
            case DialogChoiceType::Passive: return "Passive";
            case DialogChoiceType::Quest: return "Quest";
            case DialogChoiceType::Ending: return "Ending";
            case DialogChoiceType::Branch: return "Branch";
            default: return "Unknown";
        }
    }

    std::string GetStateName(DialogState state) {
        switch (state) {
            case DialogState::Inactive: return "Inactive";
            case DialogState::Starting: return "Starting";
            case DialogState::Active: return "Active";
            case DialogState::AwaitingChoice: return "AwaitingChoice";
            case DialogState::Processing: return "Processing";
            case DialogState::Ending: return "Ending";
            case DialogState::Interrupted: return "Interrupted";
            default: return "Unknown";
        }
    }

    std::string GetVoteStatusName(VoteStatus status) {
        switch (status) {
            case VoteStatus::Pending: return "Pending";
            case VoteStatus::InProgress: return "InProgress";
            case VoteStatus::Passed: return "Passed";
            case VoteStatus::Failed: return "Failed";
            case VoteStatus::Timeout: return "Timeout";
            default: return "Unknown";
        }
    }

    uint32_t CalculateVoteRequirement(DialogSyncMode mode, uint32_t participantCount) {
        switch (mode) {
            case DialogSyncMode::Consensus:
                return participantCount; // All must agree
            case DialogSyncMode::Majority:
            case DialogSyncMode::Proximity:
                return (participantCount / 2) + 1; // Simple majority
            case DialogSyncMode::Quest:
                return std::max(1U, participantCount / 3); // 1/3 for quest decisions
            default:
                return 1; // Speaker only
        }
    }

    bool ValidateDialogText(const std::string& text) {
        if (text.empty() || text.length() > 1000) {
            return false;
        }
        // Additional validation could be added here
        return true;
    }
}

} // namespace CoopNet