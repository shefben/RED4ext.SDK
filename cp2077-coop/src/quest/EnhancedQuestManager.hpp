#pragma once

#include <RED4ext/RED4ext.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <memory>
#include <chrono>
#include <queue>

namespace CoopNet {

// Enhanced quest state tracking
enum class QuestState : uint8_t {
    Inactive = 0,
    Active = 1,
    Completed = 2,
    Failed = 3,
    Suspended = 4,
    Locked = 5
};

enum class QuestType : uint8_t {
    Main = 0,
    Side = 1,
    Gig = 2,
    NCPD = 3,
    Romance = 4,
    Corporate = 5,
    Fixer = 6,
    Custom = 7
};

enum class QuestPriority : uint8_t {
    Critical = 0,    // Must be synchronized immediately
    High = 1,        // Synchronized within 1 second
    Medium = 2,      // Synchronized within 5 seconds
    Low = 3,         // Synchronized when convenient
    Background = 4   // Synchronized on demand
};

enum class QuestSyncMode : uint8_t {
    Strict = 0,      // All players must be at same stage
    Majority = 1,    // Majority vote for progression
    Individual = 2,  // Players can progress independently
    Leader = 3,      // Quest leader controls progression
    Consensus = 4    // Unanimous agreement required
};

enum class ConflictResolution : uint8_t {
    RollbackAll = 0,     // Roll back all players to common stage
    AdvanceAll = 1,      // Advance all players to highest stage
    Vote = 2,            // Vote on which stage to use
    LeaderDecides = 3,   // Quest leader decides
    AutoResolve = 4      // System automatically resolves based on rules
};

// Quest objective tracking
struct QuestObjective {
    uint32_t objectiveId;
    uint32_t questHash;
    std::string description;
    QuestState state;
    bool isOptional;
    bool requiresAllPlayers;
    std::vector<uint32_t> completedByPlayers;
    uint64_t lastModified;

    // Objective-specific data
    std::unordered_map<std::string, std::string> customData;
};

// Player-specific quest progress
struct PlayerQuestProgress {
    uint32_t playerId;
    uint32_t questHash;
    uint16_t currentStage;
    QuestState state;
    std::vector<uint32_t> completedObjectives;
    std::unordered_map<std::string, std::string> questVariables;
    uint64_t lastUpdate;
    bool isQuestLeader;

    // Player choices for branching quests
    std::unordered_map<uint32_t, uint32_t> branchChoices; // stage -> choice
};

// Quest synchronization data
struct QuestSyncData {
    uint32_t questHash;
    std::string questName;
    QuestType type;
    QuestPriority priority;
    QuestSyncMode syncMode;
    ConflictResolution conflictMode;

    // Current state
    uint16_t authorityStage;    // The "correct" stage according to authority
    QuestState authorityState;
    std::vector<QuestObjective> objectives;
    std::unordered_map<uint32_t, PlayerQuestProgress> playerProgress;

    // Synchronization metadata
    uint32_t questLeader;       // Player ID of quest leader (if applicable)
    uint64_t lastSyncTime;
    uint32_t syncAttempts;
    bool hasPendingConflict;

    // Voting system
    bool hasActiveVote;
    uint16_t voteTargetStage;
    std::unordered_map<uint32_t, bool> playerVotes; // playerId -> vote
    uint64_t voteDeadline;

    // Quest dependencies
    std::vector<uint32_t> prerequisiteQuests;
    std::vector<uint32_t> blockingQuests;

    // Custom quest data
    std::unordered_map<std::string, std::string> questData;
};

// Quest conflict information
struct QuestConflict {
    uint32_t conflictId;
    uint32_t questHash;
    std::vector<uint32_t> affectedPlayers;
    std::vector<uint16_t> conflictingStages;
    ConflictResolution resolutionMethod;
    uint64_t detectedTime;
    uint32_t resolutionAttempts;
    bool isResolved;

    // Additional context
    std::string conflictReason;
    std::unordered_map<std::string, std::string> debugInfo;
};

// Quest validation result
struct QuestValidationResult {
    bool isValid;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::unordered_map<uint32_t, std::vector<std::string>> playerIssues;
};

class EnhancedQuestManager {
public:
    static EnhancedQuestManager& Instance();

    // System lifecycle
    bool Initialize();
    void Shutdown();
    void Tick(float deltaTime);

    // Quest management
    bool RegisterQuest(uint32_t questHash, const std::string& questName, QuestType type,
                      QuestPriority priority = QuestPriority::Medium,
                      QuestSyncMode syncMode = QuestSyncMode::Strict);
    bool UnregisterQuest(uint32_t questHash);
    QuestSyncData* GetQuest(uint32_t questHash);
    std::vector<QuestSyncData*> GetActiveQuests() const;
    std::vector<QuestSyncData*> GetQuestsByType(QuestType type) const;

    // Player management
    bool RegisterPlayer(uint32_t playerId, const std::string& playerName);
    bool UnregisterPlayer(uint32_t playerId);
    std::vector<uint32_t> GetActivePlayers() const;

    // Quest progression
    bool UpdateQuestStage(uint32_t playerId, uint32_t questHash, uint16_t newStage);
    bool UpdateQuestState(uint32_t playerId, uint32_t questHash, QuestState newState);
    bool CompleteObjective(uint32_t playerId, uint32_t questHash, uint32_t objectiveId);
    bool SetQuestVariable(uint32_t playerId, uint32_t questHash, const std::string& key, const std::string& value);

    // Quest leadership
    bool SetQuestLeader(uint32_t questHash, uint32_t playerId);
    uint32_t GetQuestLeader(uint32_t questHash) const;
    bool TransferQuestLeadership(uint32_t questHash, uint32_t newLeader);

    // Branching quest support
    bool AddBranchChoice(uint32_t questHash, uint16_t stage, uint32_t playerId, uint32_t choice);
    bool RequiresConsensus(uint32_t questHash, uint16_t stage) const;
    std::unordered_map<uint32_t, uint32_t> GetBranchChoices(uint32_t questHash, uint16_t stage) const;

    // Conflict detection and resolution
    std::vector<QuestConflict> DetectConflicts();
    bool ResolveConflict(uint32_t conflictId, ConflictResolution method);
    bool StartConflictVote(uint32_t questHash, uint16_t targetStage, uint32_t initiatingPlayer);
    bool CastConflictVote(uint32_t questHash, uint32_t playerId, bool approve);

    // Quest validation
    QuestValidationResult ValidateQuestState(uint32_t questHash) const;
    QuestValidationResult ValidateAllQuests() const;
    bool RepairQuestState(uint32_t questHash);

    // Synchronization
    void SynchronizeQuest(uint32_t questHash);
    void SynchronizeAllQuests();
    void ForceResyncPlayer(uint32_t playerId);
    bool ProcessSyncQueue();

    // Quest dependencies
    bool AddQuestDependency(uint32_t questHash, uint32_t prerequisiteQuest);
    bool RemoveQuestDependency(uint32_t questHash, uint32_t prerequisiteQuest);
    std::vector<uint32_t> GetQuestDependencies(uint32_t questHash) const;
    bool CanStartQuest(uint32_t questHash, uint32_t playerId) const;

    // Performance and monitoring
    struct QuestSystemStats {
        uint32_t totalQuests;
        uint32_t activeQuests;
        uint32_t completedQuests;
        uint32_t failedQuests;
        uint32_t pendingConflicts;
        uint32_t syncOperationsPerSecond;
        float averageSyncLatency;
        uint32_t validationErrors;
    };

    QuestSystemStats GetSystemStats() const;
    void ResetStats();

    // Advanced features
    bool SaveQuestSnapshot(const std::string& filename) const;
    bool LoadQuestSnapshot(const std::string& filename);
    bool CreateQuestCheckpoint(uint32_t questHash);
    bool RestoreQuestCheckpoint(uint32_t questHash);

    // Event callbacks
    using QuestEventCallback = std::function<void(uint32_t questHash, uint32_t playerId, const std::string& eventData)>;
    void RegisterEventCallback(const std::string& eventType, QuestEventCallback callback);
    void UnregisterEventCallback(const std::string& eventType);

private:
    EnhancedQuestManager() = default;
    ~EnhancedQuestManager() = default;
    EnhancedQuestManager(const EnhancedQuestManager&) = delete;
    EnhancedQuestManager& operator=(const EnhancedQuestManager&) = delete;

    // Internal quest operations
    void ProcessConflictResolution();
    void ProcessVoting();
    void ProcessSynchronization();
    void ValidateQuestIntegrity();
    void CleanupExpiredData();

    // Conflict resolution strategies
    bool ResolveByRollback(const QuestConflict& conflict);
    bool ResolveByAdvance(const QuestConflict& conflict);
    bool ResolveByVote(const QuestConflict& conflict);
    bool ResolveByLeader(const QuestConflict& conflict);
    bool ResolveAutomatically(const QuestConflict& conflict);

    // Helper methods
    bool IsQuestActive(uint32_t questHash) const;
    bool IsPlayerInQuest(uint32_t questHash, uint32_t playerId) const;
    uint16_t GetConsensusStage(uint32_t questHash) const;
    bool ArePlayersInSync(uint32_t questHash) const;
    void TriggerEvent(const std::string& eventType, uint32_t questHash, uint32_t playerId, const std::string& data);
    uint64_t GetCurrentTimestamp() const;

    // Data storage
    std::unordered_map<uint32_t, std::unique_ptr<QuestSyncData>> m_quests;
    std::unordered_map<uint32_t, std::string> m_players; // playerId -> playerName
    std::unordered_map<uint32_t, std::unique_ptr<QuestConflict>> m_conflicts;
    mutable std::mutex m_questsMutex;
    mutable std::mutex m_playersMutex;
    mutable std::mutex m_conflictsMutex;

    // System state
    bool m_initialized = false;
    QuestSystemStats m_stats;
    std::chrono::steady_clock::time_point m_lastStatsUpdate;

    // Processing queues
    std::queue<uint32_t> m_syncQueue;
    std::queue<uint32_t> m_validationQueue;
    std::mutex m_queueMutex;

    // Event system
    std::unordered_map<std::string, std::vector<QuestEventCallback>> m_eventCallbacks;
    std::mutex m_callbackMutex;

    // Configuration
    struct Config {
        float syncInterval = 1.0f;              // Sync every second
        float conflictCheckInterval = 5.0f;     // Check conflicts every 5 seconds
        float validationInterval = 30.0f;       // Validate every 30 seconds
        uint32_t maxConflictRetries = 3;
        uint32_t voteTimeoutSeconds = 30;
        uint32_t maxQuestHistory = 100;
        bool enableAutoRepair = true;
        bool enableDetailedLogging = false;
    } m_config;

    // Timing
    float m_syncTimer = 0.0f;
    float m_conflictTimer = 0.0f;
    float m_validationTimer = 0.0f;

    // Quest checkpoints for rollback functionality
    std::unordered_map<uint32_t, std::vector<QuestSyncData>> m_questCheckpoints;
    std::mutex m_checkpointMutex;
};

// Quest synchronization network packets
struct EnhancedQuestSyncPacket {
    uint32_t questHash;
    uint16_t targetStage;
    QuestState targetState;
    uint32_t playerId;
    uint64_t timestamp;
    uint8_t syncType; // 0=normal, 1=conflict_resolution, 2=forced
};

struct QuestConflictNotificationPacket {
    uint32_t conflictId;
    uint32_t questHash;
    uint16_t conflictingStages[8]; // Max 8 different stages
    uint8_t stageCount;
    uint8_t resolutionMethod;
    uint32_t voteTimeoutSeconds;
};

struct QuestVotePacket {
    uint32_t questHash;
    uint32_t playerId;
    uint16_t targetStage;
    bool approve;
    uint64_t timestamp;
};

struct QuestObjectiveUpdatePacket {
    uint32_t questHash;
    uint32_t objectiveId;
    uint32_t playerId;
    uint8_t newState;
    uint64_t timestamp;
};

// Utility functions for quest management
namespace QuestUtils {
    uint32_t HashQuestName(const std::string& questName);
    std::string GetQuestTypeName(QuestType type);
    std::string GetQuestStateName(QuestState state);
    std::string GetSyncModeName(QuestSyncMode mode);
    bool IsQuestNameValid(const std::string& questName);
    QuestPriority DetermineQuestPriority(QuestType type);
    QuestSyncMode GetRecommendedSyncMode(QuestType type);
}

} // namespace CoopNet