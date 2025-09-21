#include "CooperativeMissionManager.hpp"
#include "RED4ext/Api/Sdk.hpp"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>

namespace RED4ext
{
    // CooperativeMissionManager Implementation
    CooperativeMissionManager& CooperativeMissionManager::GetInstance()
    {
        static CooperativeMissionManager instance;
        return instance;
    }

    void CooperativeMissionManager::Initialize()
    {
        std::unique_lock<std::shared_mutex> missionsLock(m_missionsMutex);
        std::unique_lock<std::shared_mutex> participantsLock(m_participantsMutex);

        // Clear existing data
        m_missions.clear();
        m_participants.clear();
        m_playerToMission.clear();

        // Initialize statistics
        m_totalMissionsCreated = 0;
        m_totalMissionsCompleted = 0;
        m_lastCleanup = std::chrono::steady_clock::now();
    }

    void CooperativeMissionManager::Shutdown()
    {
        // End all active missions
        std::vector<std::string> activeMissions = GetActiveMissions();
        for (const auto& missionId : activeMissions) {
            EndMission(missionId);
        }

        std::unique_lock<std::shared_mutex> missionsLock(m_missionsMutex);
        std::unique_lock<std::shared_mutex> participantsLock(m_participantsMutex);

        m_missions.clear();
        m_participants.clear();
        m_playerToMission.clear();

        // Clear callbacks
        std::lock_guard<std::mutex> callbacksLock(m_callbacksMutex);
        m_missionStartedCallback = nullptr;
        m_missionEndedCallback = nullptr;
        m_participantJoinedCallback = nullptr;
        m_participantLeftCallback = nullptr;
        m_objectiveUpdatedCallback = nullptr;
        m_dialogueChoiceCallback = nullptr;
    }

    void CooperativeMissionManager::Update()
    {
        auto currentTime = std::chrono::steady_clock::now();

        // Periodic cleanup (every 2 minutes)
        auto timeSinceCleanup = std::chrono::duration_cast<std::chrono::minutes>(
            currentTime - m_lastCleanup).count();

        if (timeSinceCleanup >= 2) {
            CleanupInactiveMissions();
            CleanupDisconnectedParticipants();
            m_lastCleanup = currentTime;
        }

        // Process dialogue timeouts
        ProcessDialogueTimeouts();

        // Update mission progress and validate proximity
        std::shared_lock<std::shared_mutex> lock(m_missionsMutex);
        for (auto& [missionId, mission] : m_missions) {
            if (mission->state == MissionState::InProgress) {
                UpdateMissionProgress(mission.get());
                ValidateParticipantProximity(mission.get());
            }
        }
    }

    std::string CooperativeMissionManager::CreateMission(uint32_t hostPlayerId, const std::string& questId,
                                                        const std::vector<uint32_t>& participants)
    {
        // Validate quest ID
        if (!IsValidQuestId(questId)) {
            return "";
        }

        // Check if host is eligible
        if (!IsPlayerEligibleForMission(hostPlayerId, questId)) {
            return "";
        }

        // Validate all participants
        for (uint32_t playerId : participants) {
            if (!IsPlayerEligibleForMission(playerId, questId)) {
                return "";
            }
        }

        std::string missionId = GenerateMissionId();

        auto mission = std::make_unique<CooperativeMission>();
        mission->missionId = missionId;
        mission->questId = questId;
        mission->hostPlayerId = hostPlayerId;
        mission->participants = participants;
        mission->state = MissionState::Starting;
        mission->startTime = std::chrono::steady_clock::now();
        mission->lastUpdate = mission->startTime;

        // Create participants
        {
            std::unique_lock<std::shared_mutex> participantsLock(m_participantsMutex);

            for (uint32_t playerId : participants) {
                auto participant = std::make_unique<MissionParticipant>();
                participant->playerId = playerId;
                participant->isReady = false;
                participant->isConnected = true;
                participant->lastActivity = std::chrono::steady_clock::now();

                m_participants[playerId] = std::move(participant);
                m_playerToMission[playerId] = missionId;
            }
        }

        // Store mission
        {
            std::unique_lock<std::shared_mutex> lock(m_missionsMutex);
            m_missions[missionId] = std::move(mission);
        }

        m_totalMissionsCreated++;

        // Notify listeners
        NotifyMissionStarted(missionId);

        return missionId;
    }

    bool CooperativeMissionManager::StartMission(const std::string& missionId)
    {
        std::unique_lock<std::shared_mutex> lock(m_missionsMutex);

        auto it = m_missions.find(missionId);
        if (it == m_missions.end()) {
            return false;
        }

        auto& mission = it->second;
        if (mission->state != MissionState::Starting) {
            return false;
        }

        // Check if all participants are ready
        {
            std::shared_lock<std::shared_mutex> participantsLock(m_participantsMutex);
            for (uint32_t playerId : mission->participants) {
                auto participantIt = m_participants.find(playerId);
                if (participantIt == m_participants.end() || !participantIt->second->isReady) {
                    return false; // Not all participants ready
                }
            }
        }

        mission->state = MissionState::InProgress;
        mission->lastUpdate = std::chrono::steady_clock::now();

        // Broadcast mission state to all participants
        lock.unlock();
        BroadcastMissionState(missionId);

        return true;
    }

    bool CooperativeMissionManager::EndMission(const std::string& missionId)
    {
        std::unique_lock<std::shared_mutex> missionsLock(m_missionsMutex);

        auto it = m_missions.find(missionId);
        if (it == m_missions.end()) {
            return false;
        }

        auto& mission = it->second;
        bool wasSuccessful = (mission->state == MissionState::Completed);

        // Get participants list before cleanup
        std::vector<uint32_t> participants = mission->participants;

        // Update mission state
        mission->state = MissionState::Completed;
        mission->lastUpdate = std::chrono::steady_clock::now();

        missionsLock.unlock();

        // Cleanup participants
        {
            std::unique_lock<std::shared_mutex> participantsLock(m_participantsMutex);
            for (uint32_t playerId : participants) {
                m_participants.erase(playerId);
                m_playerToMission.erase(playerId);
            }
        }

        // Remove mission
        {
            std::unique_lock<std::shared_mutex> lock(m_missionsMutex);
            m_missions.erase(missionId);
        }

        m_totalMissionsCompleted++;

        // Notify listeners
        NotifyMissionEnded(missionId, wasSuccessful);

        return true;
    }

    bool CooperativeMissionManager::CancelMission(const std::string& missionId)
    {
        std::unique_lock<std::shared_mutex> lock(m_missionsMutex);

        auto it = m_missions.find(missionId);
        if (it == m_missions.end()) {
            return false;
        }

        auto& mission = it->second;
        mission->state = MissionState::Cancelled;
        mission->lastUpdate = std::chrono::steady_clock::now();

        lock.unlock();

        // End the mission
        return EndMission(missionId);
    }

    bool CooperativeMissionManager::AddParticipant(const std::string& missionId, uint32_t playerId)
    {
        std::unique_lock<std::shared_mutex> missionsLock(m_missionsMutex);

        auto it = m_missions.find(missionId);
        if (it == m_missions.end()) {
            return false;
        }

        auto& mission = it->second;

        // Check if player can join
        if (!CanPlayerJoinMission(playerId, missionId)) {
            return false;
        }

        // Add to participants list
        mission->participants.push_back(playerId);
        mission->syncVersion++;

        missionsLock.unlock();

        // Create participant entry
        {
            std::unique_lock<std::shared_mutex> participantsLock(m_participantsMutex);

            auto participant = std::make_unique<MissionParticipant>();
            participant->playerId = playerId;
            participant->isReady = false;
            participant->isConnected = true;
            participant->lastActivity = std::chrono::steady_clock::now();

            m_participants[playerId] = std::move(participant);
            m_playerToMission[playerId] = missionId;
        }

        // Sync mission state to new participant
        SyncMissionToPlayer(missionId, playerId);

        // Notify listeners
        NotifyParticipantJoined(missionId, playerId);

        return true;
    }

    bool CooperativeMissionManager::RemoveParticipant(const std::string& missionId, uint32_t playerId)
    {
        std::unique_lock<std::shared_mutex> missionsLock(m_missionsMutex);

        auto it = m_missions.find(missionId);
        if (it == m_missions.end()) {
            return false;
        }

        auto& mission = it->second;

        // Remove from participants list
        auto participantIt = std::find(mission->participants.begin(), mission->participants.end(), playerId);
        if (participantIt == mission->participants.end()) {
            return false;
        }

        mission->participants.erase(participantIt);
        mission->syncVersion++;

        // If removing host, transfer to another participant
        if (mission->hostPlayerId == playerId && !mission->participants.empty()) {
            mission->hostPlayerId = mission->participants[0];
        }

        missionsLock.unlock();

        // Remove participant entry
        {
            std::unique_lock<std::shared_mutex> participantsLock(m_participantsMutex);
            m_participants.erase(playerId);
            m_playerToMission.erase(playerId);
        }

        // If no participants left, end mission
        if (mission->participants.empty()) {
            EndMission(missionId);
        } else {
            // Broadcast updated state
            BroadcastMissionState(missionId);
        }

        // Notify listeners
        NotifyParticipantLeft(missionId, playerId);

        return true;
    }

    void CooperativeMissionManager::SyncQuestStart(const std::string& missionId, const std::string& questId)
    {
        std::shared_lock<std::shared_mutex> lock(m_missionsMutex);

        auto it = m_missions.find(missionId);
        if (it == m_missions.end()) {
            return;
        }

        auto& mission = it->second;
        mission->questId = questId;
        mission->syncVersion++;
        mission->lastUpdate = std::chrono::steady_clock::now();

        lock.unlock();

        BroadcastMissionState(missionId);
    }

    void CooperativeMissionManager::SyncQuestPhase(const std::string& missionId, const std::string& questPhase)
    {
        std::shared_lock<std::shared_mutex> lock(m_missionsMutex);

        auto it = m_missions.find(missionId);
        if (it == m_missions.end()) {
            return;
        }

        auto& mission = it->second;
        mission->currentPhase = questPhase;
        mission->syncVersion++;
        mission->lastUpdate = std::chrono::steady_clock::now();

        lock.unlock();

        BroadcastMissionState(missionId);
    }

    void CooperativeMissionManager::SyncObjectiveState(const std::string& missionId, const std::string& objectiveId,
                                                      ObjectiveState state, float progress)
    {
        std::shared_lock<std::shared_mutex> lock(m_missionsMutex);

        auto it = m_missions.find(missionId);
        if (it == m_missions.end()) {
            return;
        }

        auto& mission = it->second;

        // Update or create objective
        auto& objective = mission->objectives[objectiveId];
        objective.objectiveId = objectiveId;
        objective.state = state;
        objective.progressPercentage = progress;
        objective.lastUpdate = std::chrono::steady_clock::now();

        mission->syncVersion++;
        mission->lastUpdate = std::chrono::steady_clock::now();

        lock.unlock();

        // Notify objective update
        NotifyObjectiveUpdate(missionId, objectiveId);

        // Broadcast mission state
        BroadcastMissionState(missionId);
    }

    bool CooperativeMissionManager::StartDialogue(const std::string& missionId, const std::string& speakerId,
                                                 const std::string& dialogueId)
    {
        std::shared_lock<std::shared_mutex> lock(m_missionsMutex);

        auto it = m_missions.find(missionId);
        if (it == m_missions.end()) {
            return false;
        }

        auto& mission = it->second;
        mission->isInDialogue = true;
        mission->currentSpeaker = speakerId;
        mission->currentDialogueId = dialogueId;
        mission->pendingChoices.clear();
        mission->dialogueTimeout = std::chrono::steady_clock::now() + std::chrono::seconds(60); // 1 minute timeout
        mission->syncVersion++;

        lock.unlock();

        BroadcastMissionState(missionId);
        return true;
    }

    bool CooperativeMissionManager::SubmitDialogueChoice(const std::string& missionId, uint32_t playerId, int32_t choiceIndex)
    {
        std::shared_lock<std::shared_mutex> lock(m_missionsMutex);

        auto it = m_missions.find(missionId);
        if (it == m_missions.end()) {
            return false;
        }

        auto& mission = it->second;

        if (!mission->isInDialogue) {
            return false;
        }

        // Check if choice already exists
        for (auto& choice : mission->pendingChoices) {
            if (choice.choiceIndex == choiceIndex) {
                // Add approval if not already present
                if (std::find(choice.approvals.begin(), choice.approvals.end(), playerId) == choice.approvals.end()) {
                    choice.approvals.push_back(playerId);
                }
                lock.unlock();
                BroadcastMissionState(missionId);
                return true;
            }
        }

        // Create new choice
        DialogueChoice newChoice;
        newChoice.choiceIndex = choiceIndex;
        newChoice.suggestedById = playerId;
        newChoice.approvals.push_back(playerId);
        newChoice.submitTime = std::chrono::steady_clock::now();

        mission->pendingChoices.push_back(newChoice);
        mission->syncVersion++;

        lock.unlock();

        // If host or majority agrees, execute choice immediately
        if (playerId == mission->hostPlayerId || ShouldExecuteChoice(missionId, choiceIndex)) {
            NotifyDialogueChoice(missionId, choiceIndex);
        }

        BroadcastMissionState(missionId);
        return true;
    }

    std::string CooperativeMissionManager::CreateCheckpoint(const std::string& missionId, uint32_t creatorId,
                                                           const std::string& checkpointName)
    {
        std::shared_lock<std::shared_mutex> lock(m_missionsMutex);

        auto it = m_missions.find(missionId);
        if (it == m_missions.end()) {
            return "";
        }

        auto& mission = it->second;

        MissionCheckpoint checkpoint;
        checkpoint.checkpointId = GenerateCheckpointId();
        checkpoint.questId = mission->questId;
        checkpoint.questPhase = mission->currentPhase;
        checkpoint.createdByPlayerId = creatorId;
        checkpoint.creationTime = std::chrono::steady_clock::now();

        // Copy current objectives
        for (const auto& [objId, objective] : mission->objectives) {
            checkpoint.objectives.push_back(objective);
        }

        mission->checkpoints.push_back(checkpoint);
        mission->syncVersion++;

        return checkpoint.checkpointId;
    }

    CooperativeMission* CooperativeMissionManager::GetMission(const std::string& missionId)
    {
        std::shared_lock<std::shared_mutex> lock(m_missionsMutex);

        auto it = m_missions.find(missionId);
        return (it != m_missions.end()) ? it->second.get() : nullptr;
    }

    const CooperativeMission* CooperativeMissionManager::GetMission(const std::string& missionId) const
    {
        std::shared_lock<std::shared_mutex> lock(m_missionsMutex);

        auto it = m_missions.find(missionId);
        return (it != m_missions.end()) ? it->second.get() : nullptr;
    }

    CooperativeMission* CooperativeMissionManager::FindMissionByPlayer(uint32_t playerId)
    {
        std::shared_lock<std::shared_mutex> participantsLock(m_participantsMutex);

        auto it = m_playerToMission.find(playerId);
        if (it == m_playerToMission.end()) {
            return nullptr;
        }

        participantsLock.unlock();
        return GetMission(it->second);
    }

    std::vector<std::string> CooperativeMissionManager::GetActiveMissions() const
    {
        std::shared_lock<std::shared_mutex> lock(m_missionsMutex);

        std::vector<std::string> activeMissions;
        for (const auto& [missionId, mission] : m_missions) {
            if (mission->state == MissionState::Starting || mission->state == MissionState::InProgress) {
                activeMissions.push_back(missionId);
            }
        }

        return activeMissions;
    }

    void CooperativeMissionManager::UpdateParticipantPosition(uint32_t playerId, float x, float y, float z)
    {
        std::shared_lock<std::shared_mutex> lock(m_participantsMutex);

        auto it = m_participants.find(playerId);
        if (it != m_participants.end()) {
            auto& participant = it->second;
            participant->posX = x;
            participant->posY = y;
            participant->posZ = z;
            participant->lastActivity = std::chrono::steady_clock::now();
        }
    }

    void CooperativeMissionManager::BroadcastMissionState(const std::string& missionId)
    {
        auto* mission = GetMission(missionId);
        if (!mission) {
            return;
        }

        SendMissionStateToParticipants(mission);
    }

    // Private implementation methods
    std::string CooperativeMissionManager::GenerateMissionId()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 15);

        std::stringstream ss;
        ss << "mission_";
        for (int i = 0; i < 8; ++i) {
            ss << std::hex << dis(gen);
        }

        return ss.str();
    }

    std::string CooperativeMissionManager::GenerateCheckpointId()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 15);

        std::stringstream ss;
        ss << "checkpoint_";
        for (int i = 0; i < 8; ++i) {
            ss << std::hex << dis(gen);
        }

        return ss.str();
    }

    bool CooperativeMissionManager::IsPlayerEligibleForMission(uint32_t playerId, const std::string& questId) const
    {
        // Check if player is already in another mission
        std::shared_lock<std::shared_mutex> lock(m_participantsMutex);
        auto it = m_playerToMission.find(playerId);
        if (it != m_playerToMission.end()) {
            return false; // Player already in another mission
        }

        // Additional eligibility checks would go here
        return true;
    }

    bool CooperativeMissionManager::IsValidQuestId(const std::string& questId) const
    {
        // Validate quest ID format and existence
        return !questId.empty() && questId.length() <= 64;
    }

    bool CooperativeMissionManager::CanPlayerJoinMission(uint32_t playerId, const std::string& missionId) const
    {
        auto* mission = GetMission(missionId);
        if (!mission) {
            return false;
        }

        // Check mission state
        if (mission->state != MissionState::Starting && mission->state != MissionState::InProgress) {
            return false;
        }

        // Check if player is already a participant
        auto it = std::find(mission->participants.begin(), mission->participants.end(), playerId);
        if (it != mission->participants.end()) {
            return false;
        }

        // Check player eligibility
        return IsPlayerEligibleForMission(playerId, mission->questId);
    }

    void CooperativeMissionManager::CleanupInactiveMissions()
    {
        std::unique_lock<std::shared_mutex> lock(m_missionsMutex);

        auto currentTime = std::chrono::steady_clock::now();
        std::vector<std::string> missionsToRemove;

        for (const auto& [missionId, mission] : m_missions) {
            // Remove missions that have been inactive for more than 1 hour
            auto inactiveDuration = std::chrono::duration_cast<std::chrono::hours>(
                currentTime - mission->lastUpdate).count();

            if (inactiveDuration >= 1 && (mission->state == MissionState::Completed ||
                                         mission->state == MissionState::Failed ||
                                         mission->state == MissionState::Cancelled)) {
                missionsToRemove.push_back(missionId);
            }
        }

        for (const auto& missionId : missionsToRemove) {
            m_missions.erase(missionId);
        }
    }

    void CooperativeMissionManager::ProcessDialogueTimeouts()
    {
        std::shared_lock<std::shared_mutex> lock(m_missionsMutex);

        auto currentTime = std::chrono::steady_clock::now();

        for (auto& [missionId, mission] : m_missions) {
            if (mission->isInDialogue && currentTime >= mission->dialogueTimeout) {
                // Timeout reached, execute most popular choice or default choice
                if (!mission->pendingChoices.empty()) {
                    // Find choice with most approvals
                    int32_t bestChoice = -1;
                    size_t maxApprovals = 0;

                    for (const auto& choice : mission->pendingChoices) {
                        if (choice.approvals.size() > maxApprovals) {
                            maxApprovals = choice.approvals.size();
                            bestChoice = choice.choiceIndex;
                        }
                    }

                    if (bestChoice >= 0) {
                        mission->isInDialogue = false;
                        mission->pendingChoices.clear();
                        mission->syncVersion++;

                        // Execute the choice
                        lock.unlock();
                        NotifyDialogueChoice(missionId, bestChoice);
                        BroadcastMissionState(missionId);
                        lock.lock();
                    }
                }
            }
        }
    }

    bool CooperativeMissionManager::ShouldExecuteChoice(const std::string& missionId, int32_t choiceIndex) const
    {
        auto* mission = GetMission(missionId);
        if (!mission) {
            return false;
        }

        // Find the choice
        for (const auto& choice : mission->pendingChoices) {
            if (choice.choiceIndex == choiceIndex) {
                // Execute if majority of participants approve
                size_t totalParticipants = mission->participants.size();
                size_t requiredApprovals = (totalParticipants / 2) + 1;

                return choice.approvals.size() >= requiredApprovals;
            }
        }

        return false;
    }

    bool CooperativeMissionManager::UpdateMissionSettings(const std::string& missionId, bool syncChoices,
                                      bool syncObjectives, bool syncDialogue,
                                      bool allowIndependentExploration, float maxDistance)
    {
        std::unique_lock<std::shared_mutex> lock(m_missionsMutex);

        auto it = m_missions.find(missionId);
        if (it == m_missions.end()) {
            return false;
        }

        auto& mission = it->second;
        mission->syncChoices = syncChoices;
        mission->syncObjectives = syncObjectives;
        mission->syncDialogue = syncDialogue;
        mission->allowIndependentExploration = allowIndependentExploration;
        mission->maxDistanceFromMission = maxDistance;
        mission->syncVersion++;
        mission->lastUpdate = std::chrono::steady_clock::now();

        lock.unlock();
        BroadcastMissionState(missionId);

        return true;
    }

    uint32_t CooperativeMissionManager::GetActiveMissionCount() const
    {
        std::shared_lock<std::shared_mutex> lock(m_missionsMutex);

        uint32_t count = 0;
        for (const auto& [missionId, mission] : m_missions) {
            if (mission->state == MissionState::Starting ||
                mission->state == MissionState::InProgress ||
                mission->state == MissionState::Paused) {
                count++;
            }
        }
        return count;
    }

    uint32_t CooperativeMissionManager::GetTotalParticipants() const
    {
        std::shared_lock<std::shared_mutex> lock(m_participantsMutex);
        return static_cast<uint32_t>(m_participants.size());
    }

    std::chrono::milliseconds CooperativeMissionManager::GetAverageMissionDuration() const
    {
        std::shared_lock<std::shared_mutex> lock(m_missionsMutex);

        if (m_missions.empty()) {
            return std::chrono::milliseconds(0);
        }

        auto totalDuration = std::chrono::milliseconds(0);
        uint32_t activeMissions = 0;
        auto currentTime = std::chrono::steady_clock::now();

        for (const auto& [missionId, mission] : m_missions) {
            if (mission->state == MissionState::Starting ||
                mission->state == MissionState::InProgress ||
                mission->state == MissionState::Paused) {
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    currentTime - mission->startTime);
                totalDuration += duration;
                activeMissions++;
            }
        }

        return activeMissions > 0 ? totalDuration / activeMissions : std::chrono::milliseconds(0);
    }

    void CooperativeMissionManager::SendMissionStateToParticipants(const CooperativeMission* mission)
    {
        // This would send network messages to all participants
        // Implementation would depend on the networking system
    }

    void CooperativeMissionManager::NotifyObjectiveUpdate(const std::string& missionId, const std::string& objectiveId)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_objectiveUpdatedCallback) {
            auto* mission = GetMission(missionId);
            if (mission) {
                auto it = mission->objectives.find(objectiveId);
                if (it != mission->objectives.end()) {
                    m_objectiveUpdatedCallback(missionId, objectiveId, it->second.state);
                }
            }
        }
    }

    void CooperativeMissionManager::NotifyDialogueChoice(const std::string& missionId, int32_t choiceIndex)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_dialogueChoiceCallback) {
            m_dialogueChoiceCallback(missionId, 0, choiceIndex); // 0 = system choice
        }
    }

    // Additional missing public method implementations
    bool CooperativeMissionManager::PauseMission(const std::string& missionId)
    {
        std::unique_lock<std::shared_mutex> lock(m_missionsMutex);

        auto it = m_missions.find(missionId);
        if (it == m_missions.end()) {
            return false;
        }

        auto& mission = it->second;
        if (mission->state != MissionState::InProgress) {
            return false;
        }

        mission->state = MissionState::Paused;
        mission->lastUpdate = std::chrono::steady_clock::now();
        mission->syncVersion++;

        lock.unlock();
        BroadcastMissionState(missionId);

        return true;
    }

    bool CooperativeMissionManager::ResumeMission(const std::string& missionId)
    {
        std::unique_lock<std::shared_mutex> lock(m_missionsMutex);

        auto it = m_missions.find(missionId);
        if (it == m_missions.end()) {
            return false;
        }

        auto& mission = it->second;
        if (mission->state != MissionState::Paused) {
            return false;
        }

        mission->state = MissionState::InProgress;
        mission->lastUpdate = std::chrono::steady_clock::now();
        mission->syncVersion++;

        lock.unlock();
        BroadcastMissionState(missionId);

        return true;
    }

    bool CooperativeMissionManager::TransferHost(const std::string& missionId, uint32_t newHostId)
    {
        std::unique_lock<std::shared_mutex> lock(m_missionsMutex);

        auto it = m_missions.find(missionId);
        if (it == m_missions.end()) {
            return false;
        }

        auto& mission = it->second;

        // Check if new host is a participant
        auto participantIt = std::find(mission->participants.begin(), mission->participants.end(), newHostId);
        if (participantIt == mission->participants.end()) {
            return false;
        }

        mission->hostPlayerId = newHostId;
        mission->lastUpdate = std::chrono::steady_clock::now();
        mission->syncVersion++;

        lock.unlock();
        BroadcastMissionState(missionId);

        return true;
    }

    void CooperativeMissionManager::UpdateParticipantActivity(uint32_t playerId)
    {
        std::shared_lock<std::shared_mutex> lock(m_participantsMutex);

        auto it = m_participants.find(playerId);
        if (it != m_participants.end()) {
            it->second->lastActivity = std::chrono::steady_clock::now();
        }
    }

    void CooperativeMissionManager::SyncQuestCompletion(const std::string& missionId, bool successful)
    {
        std::unique_lock<std::shared_mutex> lock(m_missionsMutex);

        auto it = m_missions.find(missionId);
        if (it == m_missions.end()) {
            return;
        }

        auto& mission = it->second;
        mission->state = successful ? MissionState::Completed : MissionState::Failed;
        mission->lastUpdate = std::chrono::steady_clock::now();
        mission->syncVersion++;

        lock.unlock();

        if (successful) {
            m_totalMissionsCompleted++;
        }

        NotifyMissionEnded(missionId, successful);
        BroadcastMissionState(missionId);
    }

    bool CooperativeMissionManager::ApproveDialogueChoice(const std::string& missionId, uint32_t playerId, int32_t choiceIndex)
    {
        std::shared_lock<std::shared_mutex> lock(m_missionsMutex);

        auto it = m_missions.find(missionId);
        if (it == m_missions.end()) {
            return false;
        }

        auto& mission = it->second;

        if (!mission->isInDialogue) {
            return false;
        }

        // Find the choice and add approval
        for (auto& choice : mission->pendingChoices) {
            if (choice.choiceIndex == choiceIndex) {
                // Check if already approved
                auto approvalIt = std::find(choice.approvals.begin(), choice.approvals.end(), playerId);
                if (approvalIt == choice.approvals.end()) {
                    choice.approvals.push_back(playerId);
                }
                // Remove from rejections if present
                auto rejectionIt = std::find(choice.rejections.begin(), choice.rejections.end(), playerId);
                if (rejectionIt != choice.rejections.end()) {
                    choice.rejections.erase(rejectionIt);
                }
                mission->syncVersion++;
                lock.unlock();
                BroadcastMissionState(missionId);
                return true;
            }
        }

        return false;
    }

    bool CooperativeMissionManager::RejectDialogueChoice(const std::string& missionId, uint32_t playerId, int32_t choiceIndex)
    {
        std::shared_lock<std::shared_mutex> lock(m_missionsMutex);

        auto it = m_missions.find(missionId);
        if (it == m_missions.end()) {
            return false;
        }

        auto& mission = it->second;

        if (!mission->isInDialogue) {
            return false;
        }

        // Find the choice and add rejection
        for (auto& choice : mission->pendingChoices) {
            if (choice.choiceIndex == choiceIndex) {
                // Check if already rejected
                auto rejectionIt = std::find(choice.rejections.begin(), choice.rejections.end(), playerId);
                if (rejectionIt == choice.rejections.end()) {
                    choice.rejections.push_back(playerId);
                }
                // Remove from approvals if present
                auto approvalIt = std::find(choice.approvals.begin(), choice.approvals.end(), playerId);
                if (approvalIt != choice.approvals.end()) {
                    choice.approvals.erase(approvalIt);
                }
                mission->syncVersion++;
                lock.unlock();
                BroadcastMissionState(missionId);
                return true;
            }
        }

        return false;
    }

    void CooperativeMissionManager::ProcessDialogueTimeout(const std::string& missionId)
    {
        std::unique_lock<std::shared_mutex> lock(m_missionsMutex);

        auto it = m_missions.find(missionId);
        if (it == m_missions.end()) {
            return;
        }

        auto& mission = it->second;

        if (!mission->isInDialogue) {
            return;
        }

        auto currentTime = std::chrono::steady_clock::now();
        if (currentTime >= mission->dialogueTimeout) {
            // Timeout reached, execute most popular choice or default choice
            if (!mission->pendingChoices.empty()) {
                // Find choice with most approvals
                int32_t bestChoice = -1;
                size_t maxApprovals = 0;

                for (const auto& choice : mission->pendingChoices) {
                    if (choice.approvals.size() > maxApprovals) {
                        maxApprovals = choice.approvals.size();
                        bestChoice = choice.choiceIndex;
                    }
                }

                if (bestChoice >= 0) {
                    mission->isInDialogue = false;
                    mission->pendingChoices.clear();
                    mission->syncVersion++;

                    // Execute the choice
                    lock.unlock();
                    NotifyDialogueChoice(missionId, bestChoice);
                    BroadcastMissionState(missionId);
                }
            }
        }
    }

    bool CooperativeMissionManager::RestoreCheckpoint(const std::string& missionId, const std::string& checkpointId)
    {
        std::unique_lock<std::shared_mutex> lock(m_missionsMutex);

        auto it = m_missions.find(missionId);
        if (it == m_missions.end()) {
            return false;
        }

        auto& mission = it->second;

        // Find the checkpoint
        for (const auto& checkpoint : mission->checkpoints) {
            if (checkpoint.checkpointId == checkpointId) {
                // Restore quest state
                mission->questId = checkpoint.questId;
                mission->currentPhase = checkpoint.questPhase;

                // Restore objectives
                mission->objectives.clear();
                for (const auto& objective : checkpoint.objectives) {
                    mission->objectives[objective.objectiveId] = objective;
                }

                mission->syncVersion++;
                mission->lastUpdate = std::chrono::steady_clock::now();

                lock.unlock();
                BroadcastMissionState(missionId);
                return true;
            }
        }

        return false;
    }

    std::vector<MissionCheckpoint> CooperativeMissionManager::GetCheckpoints(const std::string& missionId) const
    {
        std::shared_lock<std::shared_mutex> lock(m_missionsMutex);

        auto it = m_missions.find(missionId);
        if (it != m_missions.end()) {
            return it->second->checkpoints;
        }

        return std::vector<MissionCheckpoint>();
    }

    std::vector<uint32_t> CooperativeMissionManager::GetMissionParticipants(const std::string& missionId) const
    {
        std::shared_lock<std::shared_mutex> lock(m_missionsMutex);

        auto it = m_missions.find(missionId);
        if (it != m_missions.end()) {
            return it->second->participants;
        }

        return std::vector<uint32_t>();
    }

    // Missing private method implementations
    void CooperativeMissionManager::UpdateMissionProgress(CooperativeMission* mission)
    {
        if (!mission) return;

        float oldProgress = 0.0f;
        float newProgress = MissionUtils::CalculateMissionProgress(mission->objectives);

        // Check if mission should be completed
        bool allObjectivesCompleted = true;
        for (const auto& [objectiveId, objective] : mission->objectives) {
            if (!objective.isOptional && objective.state != ObjectiveState::Completed) {
                allObjectivesCompleted = false;
                break;
            }
        }

        if (allObjectivesCompleted && mission->state == MissionState::InProgress) {
            mission->state = MissionState::Completed;
            m_totalMissionsCompleted++;
            NotifyMissionEnded(mission->missionId, true);
        }

        // Update progress for all participants
        for (uint32_t participantId : mission->participants) {
            auto participantIt = m_participants.find(participantId);
            if (participantIt != m_participants.end()) {
                uint32_t completed = 0;
                for (const auto& [objId, obj] : mission->objectives) {
                    if (obj.state == ObjectiveState::Completed) {
                        completed++;
                    }
                }
                participantIt->second->completedObjectives = completed;
                participantIt->second->totalObjectives = static_cast<uint32_t>(mission->objectives.size());
                participantIt->second->progressPercentage = newProgress;
            }
        }

        mission->lastUpdate = std::chrono::steady_clock::now();
        mission->syncVersion++;
    }


    void CooperativeMissionManager::ValidateParticipantProximity(CooperativeMission* mission)
    {
        if (!mission) return;

        auto currentTime = std::chrono::steady_clock::now();
        std::vector<uint32_t> playersToRemove;

        for (uint32_t participantId : mission->participants) {
            auto participantIt = m_participants.find(participantId);
            if (participantIt != m_participants.end()) {
                // Check if player has been away too long
                auto timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::minutes>(
                    currentTime - participantIt->second->lastActivity).count();

                if (timeSinceLastUpdate >= 10) { // 10 minutes timeout
                    playersToRemove.push_back(participantId);
                }

                // Check if player is too far from mission (using maxDistanceFromMission)
                if (!mission->allowIndependentExploration) {
                    // Calculate distance from mission center (use average participant position)
                    float avgX = 0.0f, avgY = 0.0f, avgZ = 0.0f;
                    uint32_t validParticipants = 0;

                    for (uint32_t otherParticipant : mission->participants) {
                        auto otherIt = m_participants.find(otherParticipant);
                        if (otherIt != m_participants.end() && otherIt->second->isConnected) {
                            avgX += otherIt->second->posX;
                            avgY += otherIt->second->posY;
                            avgZ += otherIt->second->posZ;
                            validParticipants++;
                        }
                    }

                    if (validParticipants > 0) {
                        avgX /= validParticipants;
                        avgY /= validParticipants;
                        avgZ /= validParticipants;

                        float distance = MissionUtils::CalculateDistance(
                            participantIt->second->posX, participantIt->second->posY, participantIt->second->posZ,
                            avgX, avgY, avgZ
                        );

                        participantIt->second->distanceFromMission = distance;

                        if (distance > mission->maxDistanceFromMission) {
                            playersToRemove.push_back(participantId);
                        }
                    }
                }
            } else {
                playersToRemove.push_back(participantId);
            }
        }

        // Remove disconnected or distant players
        for (uint32_t playerId : playersToRemove) {
            RemoveParticipant(mission->missionId, playerId);
        }
    }

    void CooperativeMissionManager::NotifyMissionStarted(const std::string& missionId)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_missionStartedCallback) {
            auto* mission = GetMission(missionId);
            if (mission) {
                m_missionStartedCallback(missionId, mission->participants);
            }
        }
    }

    void CooperativeMissionManager::NotifyMissionEnded(const std::string& missionId, bool success)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_missionEndedCallback) {
            auto* mission = GetMission(missionId);
            if (mission) {
                m_missionEndedCallback(missionId, success);
            }
        }
    }

    void CooperativeMissionManager::NotifyParticipantJoined(const std::string& missionId, uint32_t playerId)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_participantJoinedCallback) {
            m_participantJoinedCallback(missionId, playerId);
        }
    }

    void CooperativeMissionManager::NotifyParticipantLeft(const std::string& missionId, uint32_t playerId)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_participantLeftCallback) {
            m_participantLeftCallback(missionId, playerId);
        }
    }

    // Utility functions implementation
    namespace MissionUtils
    {
        std::string MissionStateToString(MissionState state)
        {
            switch (state) {
                case MissionState::Inactive: return "Inactive";
                case MissionState::Starting: return "Starting";
                case MissionState::InProgress: return "InProgress";
                case MissionState::Paused: return "Paused";
                case MissionState::Completed: return "Completed";
                case MissionState::Failed: return "Failed";
                case MissionState::Cancelled: return "Cancelled";
                default: return "Unknown";
            }
        }

        std::string ObjectiveStateToString(ObjectiveState state)
        {
            switch (state) {
                case ObjectiveState::Inactive: return "Inactive";
                case ObjectiveState::Active: return "Active";
                case ObjectiveState::Completed: return "Completed";
                case ObjectiveState::Failed: return "Failed";
                case ObjectiveState::Optional: return "Optional";
                default: return "Unknown";
            }
        }

        float CalculateDistance(float x1, float y1, float z1, float x2, float y2, float z2)
        {
            float dx = x2 - x1;
            float dy = y2 - y1;
            float dz = z2 - z1;
            return std::sqrt(dx*dx + dy*dy + dz*dz);
        }

        float CalculateMissionProgress(const std::unordered_map<std::string, QuestObjective>& objectives)
        {
            if (objectives.empty()) {
                return 0.0f;
            }

            size_t completed = 0;
            for (const auto& [id, objective] : objectives) {
                if (objective.state == ObjectiveState::Completed) {
                    completed++;
                }
            }

            return static_cast<float>(completed) / static_cast<float>(objectives.size());
        }

        bool ValidateQuestId(const std::string& questId)
        {
            return !questId.empty() && questId.length() <= 64 &&
                   questId.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") == std::string::npos;
        }

        bool ValidateObjectiveId(const std::string& objectiveId)
        {
            return !objectiveId.empty() && objectiveId.length() <= 64;
        }

        MissionState StringToMissionState(const std::string& stateStr)
        {
            if (stateStr == "Inactive") return MissionState::Inactive;
            if (stateStr == "Starting") return MissionState::Starting;
            if (stateStr == "InProgress") return MissionState::InProgress;
            if (stateStr == "Paused") return MissionState::Paused;
            if (stateStr == "Completed") return MissionState::Completed;
            if (stateStr == "Failed") return MissionState::Failed;
            if (stateStr == "Cancelled") return MissionState::Cancelled;
            return MissionState::Inactive;
        }

        ObjectiveState StringToObjectiveState(const std::string& stateStr)
        {
            if (stateStr == "Inactive") return ObjectiveState::Inactive;
            if (stateStr == "Active") return ObjectiveState::Active;
            if (stateStr == "Completed") return ObjectiveState::Completed;
            if (stateStr == "Failed") return ObjectiveState::Failed;
            if (stateStr == "Optional") return ObjectiveState::Optional;
            return ObjectiveState::Inactive;
        }

        uint32_t HashMissionState(const CooperativeMission& mission)
        {
            std::hash<std::string> hasher;
            return static_cast<uint32_t>(hasher(mission.missionId) ^ mission.syncVersion);
        }

        std::vector<uint32_t> GetEligiblePlayers(const std::string& questId)
        {
            // This would query the player system for eligible players
            // For now, return empty vector
            return std::vector<uint32_t>();
        }
    }

    // Additional missing CooperativeMissionManager method implementations
    void CooperativeMissionManager::SetMissionStartedCallback(MissionStartedCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_missionStartedCallback = callback;
    }

    void CooperativeMissionManager::SetMissionEndedCallback(MissionEndedCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_missionEndedCallback = callback;
    }

    void CooperativeMissionManager::SetParticipantJoinedCallback(ParticipantJoinedCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_participantJoinedCallback = callback;
    }

    void CooperativeMissionManager::SetParticipantLeftCallback(ParticipantLeftCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_participantLeftCallback = callback;
    }

    void CooperativeMissionManager::SetObjectiveUpdatedCallback(ObjectiveUpdatedCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_objectiveUpdatedCallback = callback;
    }

    void CooperativeMissionManager::SetDialogueChoiceCallback(DialogueChoiceCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_dialogueChoiceCallback = callback;
    }

    void CooperativeMissionManager::SyncMissionToPlayer(const std::string& missionId, uint32_t playerId)
    {
        auto* mission = GetMission(missionId);
        if (!mission) {
            return;
        }

        SendMissionStateToParticipants(mission);
    }

    void CooperativeMissionManager::CleanupDisconnectedParticipants()
    {
        std::unique_lock<std::shared_mutex> participantsLock(m_participantsMutex);
        std::vector<uint32_t> playersToRemove;

        auto currentTime = std::chrono::steady_clock::now();
        for (const auto& [playerId, participant] : m_participants) {
            if (!participant->isConnected) {
                auto timeSinceLastActivity = std::chrono::duration_cast<std::chrono::minutes>(
                    currentTime - participant->lastActivity).count();

                if (timeSinceLastActivity >= 5) { // 5 minutes timeout for disconnected players
                    playersToRemove.push_back(playerId);
                }
            }
        }

        participantsLock.unlock();

        // Remove disconnected participants from their missions
        for (uint32_t playerId : playersToRemove) {
            auto missionIt = m_playerToMission.find(playerId);
            if (missionIt != m_playerToMission.end()) {
                RemoveParticipant(missionIt->second, playerId);
            }
        }
    }

    MissionParticipant* CooperativeMissionManager::FindParticipant(uint32_t playerId)
    {
        std::shared_lock<std::shared_mutex> lock(m_participantsMutex);
        auto it = m_participants.find(playerId);
        return (it != m_participants.end()) ? it->second.get() : nullptr;
    }

    const MissionParticipant* CooperativeMissionManager::FindParticipant(uint32_t playerId) const
    {
        std::shared_lock<std::shared_mutex> lock(m_participantsMutex);
        auto it = m_participants.find(playerId);
        return (it != m_participants.end()) ? it->second.get() : nullptr;
    }

    void CooperativeMissionManager::SendObjectiveUpdateToParticipants(const CooperativeMission* mission, const std::string& objectiveId)
    {
        // This would send objective update messages to all participants
        // Implementation would depend on the networking system
    }

    void CooperativeMissionManager::SendDialogueUpdateToParticipants(const CooperativeMission* mission)
    {
        // This would send dialogue update messages to all participants
        // Implementation would depend on the networking system
    }

    void CooperativeMissionManager::NotifyObjectiveUpdated(const std::string& missionId, const std::string& objectiveId, ObjectiveState state)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_objectiveUpdatedCallback) {
            m_objectiveUpdatedCallback(missionId, objectiveId, state);
        }
    }

    void CooperativeMissionManager::NotifyDialogueChoiceMade(const std::string& missionId, uint32_t playerId, int32_t choiceIndex)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_dialogueChoiceCallback) {
            m_dialogueChoiceCallback(missionId, playerId, choiceIndex);
        }
    }
}