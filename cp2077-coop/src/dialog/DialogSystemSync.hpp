#pragma once

#include <RED4ext/RED4ext.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <memory>
#include <chrono>
#include <queue>
#include <functional>
#include <atomic>

namespace CoopNet {

// Dialog synchronization modes
enum class DialogSyncMode : uint8_t {
    Speaker = 0,        // Only speaker controls dialog progression
    Majority = 1,       // Majority vote for dialog choices
    Consensus = 2,      // All players must agree
    Proximity = 3,      // Players within proximity can participate
    Quest = 4,          // Based on quest requirements
    Individual = 5      // Each player sees their own dialog
};

// Dialog participant roles
enum class DialogRole : uint8_t {
    Speaker = 0,        // Player actively speaking with NPC
    Listener = 1,       // Player observing the conversation
    Participant = 2,    // Player can vote on choices
    Excluded = 3        // Player cannot see/affect dialog
};

// Dialog choice types
enum class DialogChoiceType : uint8_t {
    Normal = 0,         // Standard dialog option
    Skill = 1,          // Requires specific skill check
    Romance = 2,        // Romance-related choice
    Aggressive = 3,     // Combat/aggressive option
    Passive = 4,        // Peaceful/diplomatic option
    Quest = 5,          // Quest-specific choice
    Ending = 6,         // Conversation ending choice
    Branch = 7          // Branching narrative choice
};

// Dialog system states
enum class DialogState : uint8_t {
    Inactive = 0,
    Starting = 1,
    Active = 2,
    AwaitingChoice = 3,
    Processing = 4,
    Ending = 5,
    Interrupted = 6
};

// Dialog vote status
enum class VoteStatus : uint8_t {
    Pending = 0,
    InProgress = 1,
    Passed = 2,
    Failed = 3,
    Timeout = 4
};

// Forward declarations
struct DialogSession;
struct DialogChoice;
struct DialogVote;
struct DialogParticipant;
struct NPCDialogState;

// Dialog choice information
struct DialogChoice {
    uint32_t choiceId;
    uint32_t dialogId;
    std::string choiceText;
    DialogChoiceType type;
    uint32_t requiredSkill = 0;        // Skill requirement (0 = none)
    uint32_t requiredLevel = 0;        // Level requirement
    uint32_t questRequirement = 0;     // Quest requirement hash
    bool requiresConsensus = false;
    bool isQuestCritical = false;
    std::vector<uint32_t> consequences; // Consequence IDs
    std::string metadata;              // Additional choice data
    uint64_t timestamp;
};

// Dialog participant data
struct DialogParticipant {
    uint32_t playerId;
    std::string playerName;
    DialogRole role;
    bool canVote;
    bool hasVoted;
    uint32_t votedChoice = 0;
    float distanceToSpeaker;
    bool meetsRequirements;
    uint64_t lastActivity;
};

// Dialog voting system
struct DialogVote {
    uint32_t voteId;
    uint32_t dialogId;
    uint32_t choiceId;
    DialogSyncMode syncMode;
    VoteStatus status;
    uint32_t initiatingPlayer;
    std::unordered_map<uint32_t, bool> playerVotes; // playerId -> approve
    uint64_t voteDeadline;
    uint64_t startTime;
    uint32_t requiredVotes;
    uint32_t currentVotes;
    std::string voteReason;
};

// NPC dialog state tracking
struct NPCDialogState {
    uint32_t npcId;
    uint32_t currentDialog;
    uint32_t currentChoice;
    DialogState state;
    uint32_t activeSpeaker;
    std::vector<uint32_t> availableChoices;
    std::unordered_map<uint32_t, bool> choiceHistory; // choiceId -> selected
    uint64_t dialogStartTime;
    uint64_t lastUpdateTime;
    float dialogTimeout = 60.0f; // 60 seconds default
    std::string contextData;
};

// Dialog session management
struct DialogSession {
    uint32_t sessionId;
    uint32_t npcId;
    uint32_t questHash;
    DialogSyncMode syncMode;
    DialogState state;
    uint32_t primarySpeaker;
    std::unordered_map<uint32_t, DialogParticipant> participants;
    std::vector<DialogChoice> availableChoices;
    std::unique_ptr<DialogVote> activeVote;
    NPCDialogState npcState;
    uint64_t sessionStartTime;
    uint64_t lastActivityTime;
    bool allowSpectators = true;
    bool recordChoices = true;
    float proximityRange = 10.0f; // meters
    std::string sessionData;
};

// Dialog system statistics
struct DialogSystemStats {
    uint64_t totalDialogs = 0;
    uint64_t completedDialogs = 0;
    uint64_t interruptedDialogs = 0;
    uint64_t totalVotes = 0;
    uint64_t passedVotes = 0;
    uint64_t failedVotes = 0;
    uint64_t timeoutVotes = 0;
    float averageDialogDuration = 0.0f;
    float averageVoteDuration = 0.0f;
    uint32_t activeDialogs = 0;
    uint32_t activeSpeakers = 0;
    std::chrono::steady_clock::time_point lastStatsUpdate;
};

// Dialog system configuration
struct DialogConfig {
    float defaultVoteTimeout = 30.0f;     // 30 seconds for votes
    float dialogTimeout = 120.0f;         // 2 minutes for dialogs
    float proximityRange = 15.0f;          // 15 meters for proximity mode
    uint32_t maxParticipants = 8;          // Max players in dialog
    bool enableVoting = true;
    bool enableProximityCheck = true;
    bool enableSkillChecks = true;
    bool enableQuestRequirements = true;
    bool allowSpectatorMode = true;
    bool recordDialogHistory = true;
    bool enableRomanceSync = false;        // Romance dialogs are individual by default
    std::string logLevel = "INFO";
};

// Main dialog synchronization system
class DialogSystemSync {
public:
    static DialogSystemSync& Instance();

    // System lifecycle
    bool Initialize();
    void Shutdown();
    void Tick(float deltaTime);

    // Dialog session management
    bool StartDialog(uint32_t npcId, uint32_t speakerId, uint32_t questHash = 0,
                    DialogSyncMode syncMode = DialogSyncMode::Speaker);
    bool EndDialog(uint32_t sessionId, bool force = false);
    bool InterruptDialog(uint32_t sessionId, uint32_t interruptingPlayer);
    std::shared_ptr<DialogSession> GetActiveDialog(uint32_t npcId) const;
    std::shared_ptr<DialogSession> GetDialogSession(uint32_t sessionId) const;

    // Participant management
    bool AddParticipant(uint32_t sessionId, uint32_t playerId, DialogRole role = DialogRole::Listener);
    bool RemoveParticipant(uint32_t sessionId, uint32_t playerId);
    bool UpdateParticipantRole(uint32_t sessionId, uint32_t playerId, DialogRole newRole);
    bool TransferSpeakerRole(uint32_t sessionId, uint32_t newSpeakerId);
    std::vector<DialogParticipant> GetParticipants(uint32_t sessionId) const;

    // Choice handling
    bool RegisterDialogChoice(uint32_t sessionId, const DialogChoice& choice);
    bool SelectDialogChoice(uint32_t sessionId, uint32_t playerId, uint32_t choiceId);
    bool ValidateChoice(uint32_t sessionId, uint32_t playerId, uint32_t choiceId) const;
    std::vector<DialogChoice> GetAvailableChoices(uint32_t sessionId, uint32_t playerId) const;

    // Voting system
    bool StartChoiceVote(uint32_t sessionId, uint32_t choiceId, uint32_t initiatingPlayer);
    bool CastVote(uint32_t sessionId, uint32_t playerId, bool approve);
    bool CancelVote(uint32_t sessionId);
    VoteStatus GetVoteStatus(uint32_t sessionId) const;

    // NPC integration
    bool UpdateNPCDialogState(uint32_t npcId, uint32_t dialogId, DialogState state);
    bool SetNPCResponse(uint32_t npcId, const std::string& response, float duration = 5.0f);
    NPCDialogState GetNPCState(uint32_t npcId) const;
    bool IsNPCInDialog(uint32_t npcId) const;

    // Proximity and requirements
    bool UpdatePlayerPosition(uint32_t playerId, float x, float y, float z);
    bool CheckProximityRequirement(uint32_t sessionId, uint32_t playerId) const;
    bool CheckSkillRequirement(uint32_t playerId, const DialogChoice& choice) const;
    bool CheckQuestRequirement(uint32_t playerId, const DialogChoice& choice) const;

    // Romance dialog handling
    bool StartRomanceDialog(uint32_t npcId, uint32_t playerId, uint32_t romanceFlags);
    bool IsRomanceDialog(uint32_t sessionId) const;
    bool CanParticipateInRomance(uint32_t sessionId, uint32_t playerId) const;

    // Configuration and settings
    void SetSyncMode(DialogSyncMode mode);
    void SetProximityRange(float range);
    void SetVoteTimeout(float timeout);
    void EnableFeature(const std::string& feature, bool enabled);
    DialogConfig GetConfig() const;

    // Statistics and monitoring
    DialogSystemStats GetStats() const;
    void ResetStats();
    std::vector<uint32_t> GetActiveDialogSessions() const;
    uint32_t GetActiveDialogCount() const;

    // Event callbacks
    using DialogEventCallback = std::function<void(uint32_t sessionId, const std::string& eventType, const std::string& data)>;
    void RegisterEventCallback(const std::string& eventType, DialogEventCallback callback);
    void UnregisterEventCallback(const std::string& eventType);

    // Quest integration
    bool SetQuestDialogRequirements(uint32_t questHash, const std::vector<uint32_t>& requiredChoices);
    bool IsQuestChoiceRequired(uint32_t sessionId, uint32_t choiceId) const;
    bool CompleteQuestDialog(uint32_t sessionId, uint32_t questHash);

    // Advanced features
    bool CreateDialogCheckpoint(uint32_t sessionId);
    bool RestoreDialogCheckpoint(uint32_t sessionId);
    bool RecordDialogHistory(uint32_t sessionId, const std::string& action, const std::string& data);
    std::vector<std::string> GetDialogHistory(uint32_t sessionId) const;

private:
    DialogSystemSync() = default;
    ~DialogSystemSync() = default;
    DialogSystemSync(const DialogSystemSync&) = delete;
    DialogSystemSync& operator=(const DialogSystemSync&) = delete;

    // Core processing
    void ProcessActiveSessions();
    void ProcessVoting();
    void ProcessTimeouts();
    void UpdateStatistics();

    // Session management
    uint32_t GenerateSessionId();
    bool CreateSession(uint32_t npcId, uint32_t speakerId, uint32_t questHash, DialogSyncMode syncMode);
    bool CleanupSession(uint32_t sessionId);
    void NotifyParticipants(uint32_t sessionId, const std::string& event, const std::string& data);

    // Vote processing
    bool ProcessVoteResult(DialogVote* vote);
    bool CheckVoteRequirements(const DialogVote* vote) const;
    void CompleteVote(DialogVote* vote, bool passed);

    // Requirement validation
    bool ValidateParticipantRequirements(uint32_t sessionId, uint32_t playerId) const;
    bool CanPlayerParticipate(uint32_t sessionId, uint32_t playerId) const;
    float CalculateDistance(uint32_t playerId1, uint32_t playerId2) const;

    // Choice processing
    bool ApplyChoiceConsequences(uint32_t sessionId, const DialogChoice& choice);
    bool UpdateDialogFlow(uint32_t sessionId, uint32_t selectedChoice);

    // Network integration
    bool BroadcastDialogUpdate(uint32_t sessionId, const std::string& updateType, const std::string& data);
    bool SendDialogPacket(uint32_t targetPlayer, const std::string& packetType, const std::string& data);

    // Utility methods
    void TriggerEvent(const std::string& eventType, uint32_t sessionId, const std::string& data);
    uint64_t GetCurrentTimestamp() const;
    std::string SerializeSession(const DialogSession& session) const;
    bool DeserializeSession(const std::string& data, DialogSession& session) const;

    // Data storage
    std::unordered_map<uint32_t, std::shared_ptr<DialogSession>> m_activeSessions;
    std::unordered_map<uint32_t, NPCDialogState> m_npcStates;
    std::unordered_map<uint32_t, std::vector<float>> m_playerPositions; // playerId -> [x,y,z]
    std::unordered_map<uint32_t, std::vector<uint32_t>> m_questChoiceRequirements;

    // Synchronization
    mutable std::mutex m_sessionsMutex;
    mutable std::mutex m_npcStatesMutex;
    mutable std::mutex m_positionsMutex;
    mutable std::mutex m_statsMutex;

    // System state
    bool m_initialized = false;
    DialogConfig m_config;
    DialogSystemStats m_stats;

    // Session management
    std::atomic<uint32_t> m_nextSessionId{1};
    std::atomic<uint32_t> m_nextVoteId{1};

    // Event system
    std::unordered_map<std::string, std::vector<DialogEventCallback>> m_eventCallbacks;
    std::mutex m_callbackMutex;

    // Dialog history
    std::unordered_map<uint32_t, std::vector<std::string>> m_dialogHistory;
    std::unordered_map<uint32_t, DialogSession> m_dialogCheckpoints;
    std::mutex m_historyMutex;
};

// Dialog network packets
struct DialogStartPacket {
    uint32_t sessionId;
    uint32_t npcId;
    uint32_t speakerId;
    uint32_t questHash;
    uint8_t syncMode;
    uint64_t timestamp;
};

struct DialogChoicePacket {
    uint32_t sessionId;
    uint32_t playerId;
    uint32_t choiceId;
    uint64_t timestamp;
};

struct DialogVotePacket {
    uint32_t sessionId;
    uint32_t voteId;
    uint32_t playerId;
    uint32_t choiceId;
    bool approve;
    uint64_t timestamp;
};

struct DialogEndPacket {
    uint32_t sessionId;
    uint32_t endingPlayer;
    uint8_t reason; // 0=completed, 1=interrupted, 2=timeout
    uint64_t timestamp;
};

struct DialogStateUpdatePacket {
    uint32_t sessionId;
    uint32_t npcId;
    uint8_t newState;
    uint32_t currentChoice;
    uint64_t timestamp;
};

// Utility functions for dialog system
namespace DialogUtils {
    std::string GetSyncModeName(DialogSyncMode mode);
    std::string GetRoleName(DialogRole role);
    std::string GetChoiceTypeName(DialogChoiceType type);
    std::string GetStateName(DialogState state);
    std::string GetVoteStatusName(VoteStatus status);
    bool IsChoiceTypeRestricted(DialogChoiceType type);
    uint32_t CalculateVoteRequirement(DialogSyncMode mode, uint32_t participantCount);
    bool ValidateDialogText(const std::string& text);
}

} // namespace CoopNet