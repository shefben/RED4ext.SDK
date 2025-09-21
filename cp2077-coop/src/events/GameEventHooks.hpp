#pragma once

#include <RED4ext/RED4ext.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <memory>
#include <functional>
#include <chrono>
#include <queue>
#include <atomic>
#include <string>

namespace CoopNet {

// Campaign event categories automatically detected
enum class CampaignEventType : uint8_t {
    // Main storyline events
    MainQuest = 0,
    SideQuest = 1,
    GigQuest = 2,
    FixerQuest = 3,

    // Character progression
    LevelUp = 10,
    AttributeIncrease = 11,
    PerkUnlock = 12,
    SkillProgression = 13,
    CyberwareInstall = 14,

    // World interaction
    LocationDiscovered = 20,
    FastTravelUnlock = 21,
    VehicleAcquired = 22,
    WeaponFound = 23,
    ItemCrafted = 24,

    // Combat events
    EnemyKilled = 30,
    BossDefeated = 31,
    CombatStarted = 32,
    CombatEnded = 33,
    PlayerDeath = 34,

    // Story progression
    DialogueChoice = 40,
    RomanceProgression = 41,
    EndingPath = 42,
    LifepathSpecific = 43,

    // Corporate/Gang relations
    CorpRelationChange = 50,
    GangRelationChange = 51,
    StreetCredIncrease = 52,

    // Economy events
    EddieTransaction = 60,
    ShopPurchase = 61,
    VehiclePurchase = 62,
    ApartmentRent = 63,

    // Technical events
    BrainDanceComplete = 70,
    HackingSuccess = 71,
    NetrunnerProgression = 72,

    // Custom multiplayer
    Custom = 255
};

// Event synchronization modes
enum class EventSyncMode : uint8_t {
    Broadcast = 0,      // Send to all players
    Proximity = 1,      // Send to nearby players
    Quest = 2,          // Send to quest participants
    Individual = 3,     // Per-player event
    ServerOnly = 4,     // Server processing only
    Conditional = 5     // Based on conditions
};

// Event priority for network transmission
enum class EventPriority : uint8_t {
    Critical = 0,       // Story progression, player death
    High = 1,           // Combat, quest updates
    Medium = 2,         // Item acquisition, dialogue
    Low = 3,            // Environmental, ambient
    Background = 4      // Statistics, telemetry
};

// Forward declarations
struct GameEvent;
struct EventHandler;
struct CampaignEventData;
struct EventBatch;
struct EventStats;

// Core game event structure
struct GameEvent {
    uint64_t eventId;
    CampaignEventType type;
    EventSyncMode syncMode;
    EventPriority priority;
    uint32_t sourcePlayerId;
    std::vector<uint32_t> targetPlayerIds;

    // Event data
    std::string eventName;          // Original game event name
    std::string eventDescription;
    std::unordered_map<std::string, std::string> parameters;
    std::vector<uint8_t> binaryData;

    // Context information
    uint32_t questHash = 0;
    uint32_t locationHash = 0;
    uint32_t npcHash = 0;
    float worldX = 0.0f, worldY = 0.0f, worldZ = 0.0f;

    // Timing
    uint64_t timestamp;
    uint64_t gameTime;              // In-game time
    bool isReplayable = false;

    // Network optimization
    bool requiresAck = false;
    bool canBatch = true;
    uint32_t maxRetries = 3;

    // Validation
    uint32_t checksum;
    std::string signature;
};

// Campaign event data from original game
struct CampaignEventData {
    std::string originalEventName;
    CampaignEventType category;
    EventPriority defaultPriority;
    EventSyncMode defaultSyncMode;
    bool affectsStoryProgression;
    bool requiresValidation;
    std::vector<std::string> requiredParameters;
    std::vector<std::string> optionalParameters;
    std::string description;
};

// Event handler registration
struct EventHandler {
    uint64_t handlerId;
    std::string eventFilter;        // Pattern or specific event name
    CampaignEventType typeFilter;
    std::function<bool(const GameEvent&)> callback;
    EventPriority minPriority;
    bool isActive;
    uint64_t callCount;
    std::chrono::steady_clock::time_point lastCalled;
};

// Event batching for network efficiency
struct EventBatch {
    uint64_t batchId;
    std::vector<GameEvent> events;
    uint32_t targetPlayerId;
    EventPriority highestPriority;
    uint64_t createTime;
    uint32_t maxBatchSize = 50;
    float maxBatchDelay = 100.0f; // milliseconds
};

// Event system statistics
struct EventStats {
    // Event counts by type
    std::unordered_map<CampaignEventType, uint64_t> eventCounts;
    uint64_t totalEventsProcessed = 0;
    uint64_t totalEventsBroadcast = 0;
    uint64_t totalEventsFiltered = 0;

    // Performance metrics
    float averageProcessingTime = 0.0f;
    float averageNetworkDelay = 0.0f;
    uint32_t pendingEvents = 0;
    uint32_t activeHandlers = 0;

    // Error tracking
    uint64_t processingErrors = 0;
    uint64_t networkErrors = 0;
    uint64_t validationErrors = 0;

    std::chrono::steady_clock::time_point lastStatsUpdate;
};

// Main game event hooks system
class GameEventHooks {
public:
    static GameEventHooks& Instance();

    // System lifecycle
    bool Initialize();
    void Shutdown();
    void Tick(float deltaTime);

    // Automatic campaign event discovery
    bool DiscoverCampaignEvents();
    bool RegisterCampaignEvent(const std::string& eventName, CampaignEventType type,
                              EventPriority priority = EventPriority::Medium,
                              EventSyncMode syncMode = EventSyncMode::Broadcast);
    std::vector<CampaignEventData> GetDiscoveredEvents() const;

    // Event triggering and processing
    bool TriggerEvent(const GameEvent& event);
    bool TriggerCampaignEvent(const std::string& eventName, uint32_t playerId,
                             const std::unordered_map<std::string, std::string>& params = {});
    void ProcessPendingEvents();

    // Handler registration
    uint64_t RegisterEventHandler(const std::string& eventFilter,
                                 std::function<bool(const GameEvent&)> callback,
                                 CampaignEventType typeFilter = CampaignEventType::Custom,
                                 EventPriority minPriority = EventPriority::Low);
    bool UnregisterEventHandler(uint64_t handlerId);
    void EnableHandler(uint64_t handlerId, bool enabled = true);

    // Specialized handlers for campaign events
    uint64_t RegisterMainQuestHandler(std::function<bool(const GameEvent&)> callback);
    uint64_t RegisterCombatHandler(std::function<bool(const GameEvent&)> callback);
    uint64_t RegisterProgressionHandler(std::function<bool(const GameEvent&)> callback);
    uint64_t RegisterDialogueHandler(std::function<bool(const GameEvent&)> callback);

    // Event filtering and routing
    void SetEventFilter(CampaignEventType type, bool enabled);
    void SetSyncModeOverride(CampaignEventType type, EventSyncMode mode);
    void SetPriorityOverride(const std::string& eventName, EventPriority priority);

    // Campaign integration
    bool HookIntoGameSystems();
    bool AttachToQuestSystem();
    bool AttachToInventorySystem();
    bool AttachToCombatSystem();
    bool AttachToDialogueSystem();
    bool AttachToProgressionSystem();

    // Network integration
    bool BroadcastEvent(const GameEvent& event);
    bool SendEventToPlayer(const GameEvent& event, uint32_t playerId);
    bool SendEventBatch(const EventBatch& batch);
    void ProcessIncomingEvent(const GameEvent& event);

    // Event validation and security
    bool ValidateEvent(const GameEvent& event) const;
    bool CheckEventPermissions(const GameEvent& event, uint32_t playerId) const;
    uint32_t CalculateEventChecksum(const GameEvent& event) const;

    // Campaign state synchronization
    bool SynchronizeCampaignState(uint32_t targetPlayerId);
    bool GetPlayerCampaignProgress(uint32_t playerId, std::vector<GameEvent>& progressEvents);
    bool ApplyCampaignProgress(uint32_t playerId, const std::vector<GameEvent>& events);

    // Event history and replay
    void RecordEvent(const GameEvent& event);
    std::vector<GameEvent> GetEventHistory(uint32_t playerId,
                                          CampaignEventType type = CampaignEventType::Custom,
                                          uint64_t since = 0) const;
    bool ReplayEvents(uint32_t playerId, const std::vector<uint64_t>& eventIds);

    // Statistics and monitoring
    EventStats GetEventStats() const;
    void ResetStats();
    uint32_t GetActiveEventCount() const;
    std::vector<std::string> GetMostFrequentEvents(uint32_t count = 10) const;

    // Configuration
    struct Config {
        bool enableAutomaticDiscovery = true;
        bool enableEventBatching = true;
        bool enableEventHistory = true;
        bool validateAllEvents = true;
        uint32_t maxPendingEvents = 10000;
        uint32_t maxEventHistory = 50000;
        uint32_t maxHandlers = 1000;
        float eventProcessingInterval = 16.67f; // 60 FPS
        float batchFlushInterval = 100.0f; // milliseconds
        float networkTimeout = 5000.0f; // milliseconds
        std::string logLevel = "INFO";
    } m_config;

    void UpdateConfig(const Config& config);
    Config GetConfig() const;

private:
    GameEventHooks() = default;
    ~GameEventHooks() = default;
    GameEventHooks(const GameEventHooks&) = delete;
    GameEventHooks& operator=(const GameEventHooks&) = delete;

    // Automatic event discovery
    bool ScanForQuestEvents();
    bool ScanForProgressionEvents();
    bool ScanForCombatEvents();
    bool ScanForInteractionEvents();
    bool ScanForEconomyEvents();
    CampaignEventType ClassifyEvent(const std::string& eventName) const;

    // Hook installation
    bool InstallGameHooks();
    bool InstallQuestHooks();
    bool InstallInventoryHooks();
    bool InstallCombatHooks();
    bool InstallDialogueHooks();
    bool InstallProgressionHooks();
    bool InstallWorldHooks();

    // Event processing
    void ProcessEventQueue();
    void ProcessEventBatches();
    bool DispatchEvent(const GameEvent& event);
    void HandleEventError(const GameEvent& event, const std::string& error);

    // Network processing
    bool SerializeEvent(const GameEvent& event, std::vector<uint8_t>& outData) const;
    bool DeserializeEvent(const std::vector<uint8_t>& data, GameEvent& outEvent) const;
    void CreateEventBatch(uint32_t targetPlayerId);
    void FlushEventBatch(uint64_t batchId);

    // Handler management
    uint64_t GenerateHandlerId();
    void CleanupInactiveHandlers();
    std::vector<EventHandler*> GetMatchingHandlers(const GameEvent& event);

    // Campaign event mapping
    void InitializeCampaignEventMap();
    void LoadEventDefinitions();
    bool MapOriginalEvent(const std::string& originalName, CampaignEventType& type,
                         EventPriority& priority, EventSyncMode& syncMode);

    // Utility methods
    uint64_t GenerateEventId();
    uint64_t GetCurrentTimestamp() const;
    uint64_t GetGameTime() const;
    std::string GetEventTypeName(CampaignEventType type) const;
    void UpdateStatistics();
    void TriggerEventCallback(const std::string& eventType, const GameEvent& event);

    // Data storage
    std::unordered_map<std::string, CampaignEventData> m_campaignEvents;
    std::unordered_map<uint64_t, EventHandler> m_eventHandlers;
    std::queue<GameEvent> m_pendingEvents;
    std::unordered_map<uint64_t, EventBatch> m_eventBatches;
    std::unordered_map<uint32_t, std::vector<GameEvent>> m_eventHistory;

    // Event filtering
    std::unordered_set<CampaignEventType> m_disabledEventTypes;
    std::unordered_map<CampaignEventType, EventSyncMode> m_syncModeOverrides;
    std::unordered_map<std::string, EventPriority> m_priorityOverrides;

    // Synchronization
    mutable std::mutex m_eventsMutex;
    mutable std::mutex m_handlersMutex;
    mutable std::mutex m_batchesMutex;
    mutable std::mutex m_historyMutex;
    mutable std::mutex m_statsMutex;

    // System state
    bool m_initialized = false;
    bool m_hooksInstalled = false;
    EventStats m_stats;

    // Timing
    float m_processingTimer = 0.0f;
    float m_batchTimer = 0.0f;
    float m_statsTimer = 0.0f;

    // ID generation
    std::atomic<uint64_t> m_nextEventId{1};
    std::atomic<uint64_t> m_nextHandlerId{1};
    std::atomic<uint64_t> m_nextBatchId{1};

    // Game system hooks (function pointers)
    struct GameHooks {
        void* questCompletedHook = nullptr;
        void* levelUpHook = nullptr;
        void* combatStartHook = nullptr;
        void* dialogueChoiceHook = nullptr;
        void* itemAcquiredHook = nullptr;
        void* locationDiscoveredHook = nullptr;
        void* vehicleAcquiredHook = nullptr;
        void* relationshipChangeHook = nullptr;
        void* credIncreasedHook = nullptr;
        void* cyberwareInstalledHook = nullptr;
    } m_gameHooks;

    // Campaign event definitions (loaded from game data)
    std::unordered_map<std::string, std::vector<std::string>> m_eventCategories;

    // Performance monitoring
    std::chrono::steady_clock::time_point m_lastProcessTime;
    std::queue<float> m_processingTimes;
};

// Network packets for event system
struct GameEventPacket {
    uint64_t eventId;
    uint32_t sourcePlayerId;
    uint8_t eventType;
    uint8_t syncMode;
    uint8_t priority;
    uint32_t dataSize;
    uint64_t timestamp;
    uint32_t checksum;
    // Followed by serialized event data
};

struct EventBatchPacket {
    uint64_t batchId;
    uint32_t eventCount;
    uint8_t highestPriority;
    uint64_t timestamp;
    uint32_t totalDataSize;
    // Followed by serialized events
};

struct EventAckPacket {
    uint64_t eventId;
    uint32_t playerId;
    uint8_t status; // 0=success, 1=error, 2=ignored
    uint64_t timestamp;
};

// Utility functions for game events
namespace EventUtils {
    std::string GetEventTypeName(CampaignEventType type);
    std::string GetSyncModeName(EventSyncMode mode);
    std::string GetPriorityName(EventPriority priority);
    bool IsStoryEvent(CampaignEventType type);
    bool RequiresValidation(CampaignEventType type);
    uint32_t EstimateEventSize(const GameEvent& event);
    bool CanEventsGroup(const GameEvent& event1, const GameEvent& event2);
}

} // namespace CoopNet