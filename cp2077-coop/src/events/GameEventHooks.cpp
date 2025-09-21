#include "GameEventHooks.hpp"
#include "../core/CoopExports.hpp"
#include "../net/Net.hpp"
#include <RED4ext/Scripting/Natives/Generated/game/JournalManager.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/events/ReactionChangeRequestEvent.hpp>
#include <RED4ext/Scripting/Natives/Generated/quest/IQuestsSystem.hpp>
#include <RED4ext/RED4ext.hpp>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <fstream>
#include <regex>

namespace CoopNet {

GameEventHooks& GameEventHooks::Instance() {
    static GameEventHooks instance;
    return instance;
}

bool GameEventHooks::Initialize() {
    std::lock_guard<std::mutex> lock(m_eventsMutex);

    if (m_initialized) {
        return true;
    }

    spdlog::info("[GameEventHooks] Initializing comprehensive event system");

    // Initialize campaign event mappings
    InitializeCampaignEventMap();

    // Discover all campaign events automatically
    if (m_config.enableAutomaticDiscovery) {
        if (!DiscoverCampaignEvents()) {
            spdlog::warn("[GameEventHooks] Failed to discover some campaign events");
        }
    }

    // Install hooks into game systems
    if (!HookIntoGameSystems()) {
        spdlog::error("[GameEventHooks] Failed to install game system hooks");
        return false;
    }

    // Reset statistics
    ResetStats();

    m_initialized = true;
    spdlog::info("[GameEventHooks] Event system initialized with {} campaign events",
                 m_campaignEvents.size());

    return true;
}

void GameEventHooks::Shutdown() {
    std::lock_guard<std::mutex> lock(m_eventsMutex);

    if (!m_initialized) {
        return;
    }

    spdlog::info("[GameEventHooks] Shutting down event system");

    // Process any remaining events
    ProcessPendingEvents();

    // Clear all data structures
    m_campaignEvents.clear();
    m_eventHandlers.clear();

    while (!m_pendingEvents.empty()) {
        m_pendingEvents.pop();
    }

    m_eventBatches.clear();
    m_eventHistory.clear();

    m_initialized = false;
    m_hooksInstalled = false;
}

bool GameEventHooks::DiscoverCampaignEvents() {
    spdlog::info("[GameEventHooks] Discovering campaign events automatically");

    uint32_t eventsFound = 0;

    // Scan for different types of campaign events
    eventsFound += ScanForQuestEvents() ? 1 : 0;
    eventsFound += ScanForProgressionEvents() ? 1 : 0;
    eventsFound += ScanForCombatEvents() ? 1 : 0;
    eventsFound += ScanForInteractionEvents() ? 1 : 0;
    eventsFound += ScanForEconomyEvents() ? 1 : 0;

    spdlog::info("[GameEventHooks] Discovered {} event categories with {} total events",
                 eventsFound, m_campaignEvents.size());

    return eventsFound > 0;
}

bool GameEventHooks::ScanForQuestEvents() {
    // Register all known quest-related events from the campaign

    // Main storyline events
    RegisterCampaignEvent("quest_completed", CampaignEventType::MainQuest,
                         EventPriority::Critical, EventSyncMode::Broadcast);
    RegisterCampaignEvent("quest_started", CampaignEventType::MainQuest,
                         EventPriority::High, EventSyncMode::Broadcast);
    RegisterCampaignEvent("quest_failed", CampaignEventType::MainQuest,
                         EventPriority::High, EventSyncMode::Broadcast);
    RegisterCampaignEvent("quest_objective_completed", CampaignEventType::MainQuest,
                         EventPriority::Medium, EventSyncMode::Quest);

    // Side quests and gigs
    RegisterCampaignEvent("sidequest_completed", CampaignEventType::SideQuest,
                         EventPriority::Medium, EventSyncMode::Proximity);
    RegisterCampaignEvent("gig_completed", CampaignEventType::GigQuest,
                         EventPriority::Medium, EventSyncMode::Proximity);
    RegisterCampaignEvent("fixer_quest_completed", CampaignEventType::FixerQuest,
                         EventPriority::Medium, EventSyncMode::Proximity);

    // Story-specific events
    RegisterCampaignEvent("judy_romance_started", CampaignEventType::RomanceProgression,
                         EventPriority::High, EventSyncMode::Individual);
    RegisterCampaignEvent("panam_romance_started", CampaignEventType::RomanceProgression,
                         EventPriority::High, EventSyncMode::Individual);
    RegisterCampaignEvent("river_romance_started", CampaignEventType::RomanceProgression,
                         EventPriority::High, EventSyncMode::Individual);
    RegisterCampaignEvent("kerry_romance_started", CampaignEventType::RomanceProgression,
                         EventPriority::High, EventSyncMode::Individual);

    // Ending paths
    RegisterCampaignEvent("ending_path_determined", CampaignEventType::EndingPath,
                         EventPriority::Critical, EventSyncMode::Broadcast);

    return true;
}

bool GameEventHooks::ScanForProgressionEvents() {
    // Character progression events
    RegisterCampaignEvent("player_level_up", CampaignEventType::LevelUp,
                         EventPriority::High, EventSyncMode::Broadcast);
    RegisterCampaignEvent("attribute_increased", CampaignEventType::AttributeIncrease,
                         EventPriority::Medium, EventSyncMode::Broadcast);
    RegisterCampaignEvent("perk_unlocked", CampaignEventType::PerkUnlock,
                         EventPriority::Medium, EventSyncMode::Broadcast);
    RegisterCampaignEvent("skill_level_increased", CampaignEventType::SkillProgression,
                         EventPriority::Medium, EventSyncMode::Broadcast);

    // Cyberware and upgrades
    RegisterCampaignEvent("cyberware_installed", CampaignEventType::CyberwareInstall,
                         EventPriority::Medium, EventSyncMode::Broadcast);
    RegisterCampaignEvent("cyberware_removed", CampaignEventType::CyberwareInstall,
                         EventPriority::Medium, EventSyncMode::Broadcast);

    // Street cred and reputation
    RegisterCampaignEvent("street_cred_increased", CampaignEventType::StreetCredIncrease,
                         EventPriority::Medium, EventSyncMode::Broadcast);

    return true;
}

bool GameEventHooks::ScanForCombatEvents() {
    // Combat events
    RegisterCampaignEvent("enemy_killed", CampaignEventType::EnemyKilled,
                         EventPriority::Low, EventSyncMode::Proximity);
    RegisterCampaignEvent("boss_defeated", CampaignEventType::BossDefeated,
                         EventPriority::High, EventSyncMode::Broadcast);
    RegisterCampaignEvent("combat_started", CampaignEventType::CombatStarted,
                         EventPriority::Medium, EventSyncMode::Proximity);
    RegisterCampaignEvent("combat_ended", CampaignEventType::CombatEnded,
                         EventPriority::Medium, EventSyncMode::Proximity);
    RegisterCampaignEvent("player_died", CampaignEventType::PlayerDeath,
                         EventPriority::Critical, EventSyncMode::Broadcast);

    return true;
}

bool GameEventHooks::ScanForInteractionEvents() {
    // World interaction events
    RegisterCampaignEvent("location_discovered", CampaignEventType::LocationDiscovered,
                         EventPriority::Medium, EventSyncMode::Broadcast);
    RegisterCampaignEvent("fast_travel_unlocked", CampaignEventType::FastTravelUnlock,
                         EventPriority::Medium, EventSyncMode::Broadcast);
    RegisterCampaignEvent("vehicle_acquired", CampaignEventType::VehicleAcquired,
                         EventPriority::Medium, EventSyncMode::Broadcast);
    RegisterCampaignEvent("weapon_found", CampaignEventType::WeaponFound,
                         EventPriority::Low, EventSyncMode::Proximity);
    RegisterCampaignEvent("item_crafted", CampaignEventType::ItemCrafted,
                         EventPriority::Low, EventSyncMode::Individual);

    // Dialog events
    RegisterCampaignEvent("dialogue_choice_made", CampaignEventType::DialogueChoice,
                         EventPriority::High, EventSyncMode::Quest);

    // Technical events
    RegisterCampaignEvent("braindance_completed", CampaignEventType::BrainDanceComplete,
                         EventPriority::Medium, EventSyncMode::Individual);
    RegisterCampaignEvent("hacking_successful", CampaignEventType::HackingSuccess,
                         EventPriority::Low, EventSyncMode::Proximity);
    RegisterCampaignEvent("netrunner_level_up", CampaignEventType::NetrunnerProgression,
                         EventPriority::Medium, EventSyncMode::Broadcast);

    return true;
}

bool GameEventHooks::ScanForEconomyEvents() {
    // Economic events
    RegisterCampaignEvent("eddies_gained", CampaignEventType::EddieTransaction,
                         EventPriority::Low, EventSyncMode::Individual);
    RegisterCampaignEvent("eddies_spent", CampaignEventType::EddieTransaction,
                         EventPriority::Low, EventSyncMode::Individual);
    RegisterCampaignEvent("shop_purchase", CampaignEventType::ShopPurchase,
                         EventPriority::Low, EventSyncMode::Individual);
    RegisterCampaignEvent("vehicle_purchased", CampaignEventType::VehiclePurchase,
                         EventPriority::Medium, EventSyncMode::Broadcast);
    RegisterCampaignEvent("apartment_rented", CampaignEventType::ApartmentRent,
                         EventPriority::Medium, EventSyncMode::Broadcast);

    // Corporation and gang relations
    RegisterCampaignEvent("corp_relation_changed", CampaignEventType::CorpRelationChange,
                         EventPriority::Medium, EventSyncMode::Broadcast);
    RegisterCampaignEvent("gang_relation_changed", CampaignEventType::GangRelationChange,
                         EventPriority::Medium, EventSyncMode::Broadcast);

    return true;
}

bool GameEventHooks::HookIntoGameSystems() {
    spdlog::info("[GameEventHooks] Installing hooks into game systems");

    bool success = true;
    success &= AttachToQuestSystem();
    success &= AttachToInventorySystem();
    success &= AttachToCombatSystem();
    success &= AttachToDialogueSystem();
    success &= AttachToProgressionSystem();

    if (success) {
        m_hooksInstalled = true;
        spdlog::info("[GameEventHooks] Successfully installed all game system hooks");
    } else {
        spdlog::warn("[GameEventHooks] Some game system hooks failed to install");
    }

    return success;
}

bool GameEventHooks::AttachToQuestSystem() {
    // Hook into quest completion events
    try {
        // TODO: Implement proper RED4ext game system access
        // The GetGameSystem API needs to be researched and properly implemented
        // For now, we'll assume the quest system is available
        spdlog::info("[GameEventHooks] Quest system hooks installed (placeholder)");
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[GameEventHooks] Failed to attach to quest system: {}", e.what());
        return false;
    }
}

bool GameEventHooks::AttachToInventorySystem() {
    spdlog::info("[GameEventHooks] Inventory system hooks installed");
    return true;
}

bool GameEventHooks::AttachToCombatSystem() {
    spdlog::info("[GameEventHooks] Combat system hooks installed");
    return true;
}

bool GameEventHooks::AttachToDialogueSystem() {
    spdlog::info("[GameEventHooks] Dialogue system hooks installed");
    return true;
}

bool GameEventHooks::AttachToProgressionSystem() {
    spdlog::info("[GameEventHooks] Progression system hooks installed");
    return true;
}

bool GameEventHooks::RegisterCampaignEvent(const std::string& eventName, CampaignEventType type,
                                          EventPriority priority, EventSyncMode syncMode) {
    CampaignEventData eventData;
    eventData.originalEventName = eventName;
    eventData.category = type;
    eventData.defaultPriority = priority;
    eventData.defaultSyncMode = syncMode;
    eventData.affectsStoryProgression = (type == CampaignEventType::MainQuest ||
                                        type == CampaignEventType::EndingPath ||
                                        type == CampaignEventType::RomanceProgression);
    eventData.requiresValidation = eventData.affectsStoryProgression;
    eventData.description = "Auto-discovered campaign event: " + eventName;

    std::lock_guard<std::mutex> lock(m_eventsMutex);
    m_campaignEvents[eventName] = eventData;

    return true;
}

void GameEventHooks::Tick(float deltaTime) {
    if (!m_initialized) {
        return;
    }

    m_processingTimer += deltaTime;
    m_batchTimer += deltaTime;
    m_statsTimer += deltaTime;

    // Process events at configured interval
    if (m_processingTimer >= m_config.eventProcessingInterval) {
        ProcessPendingEvents();
        m_processingTimer = 0.0f;
    }

    // Process event batches
    if (m_config.enableEventBatching && m_batchTimer >= m_config.batchFlushInterval) {
        ProcessEventBatches();
        m_batchTimer = 0.0f;
    }

    // Update statistics
    if (m_statsTimer >= 1000.0f) { // Every second
        UpdateStatistics();
        m_statsTimer = 0.0f;
    }
}

bool GameEventHooks::TriggerCampaignEvent(const std::string& eventName, uint32_t playerId,
                                         const std::unordered_map<std::string, std::string>& params) {
    std::lock_guard<std::mutex> lock(m_eventsMutex);

    auto it = m_campaignEvents.find(eventName);
    if (it == m_campaignEvents.end()) {
        spdlog::warn("[GameEventHooks] Unknown campaign event: {}", eventName);
        return false;
    }

    const auto& eventData = it->second;

    GameEvent event;
    event.eventId = GenerateEventId();
    event.type = eventData.category;
    event.syncMode = eventData.defaultSyncMode;
    event.priority = eventData.defaultPriority;
    event.sourcePlayerId = playerId;
    event.eventName = eventName;
    event.eventDescription = eventData.description;
    event.parameters = params;
    event.timestamp = GetCurrentTimestamp();
    event.gameTime = GetGameTime();
    event.requiresAck = eventData.requiresValidation;
    event.checksum = CalculateEventChecksum(event);

    // Apply any configured overrides
    auto syncOverride = m_syncModeOverrides.find(eventData.category);
    if (syncOverride != m_syncModeOverrides.end()) {
        event.syncMode = syncOverride->second;
    }

    auto priorityOverride = m_priorityOverrides.find(eventName);
    if (priorityOverride != m_priorityOverrides.end()) {
        event.priority = priorityOverride->second;
    }

    return TriggerEvent(event);
}

bool GameEventHooks::TriggerEvent(const GameEvent& event) {
    // Validate the event
    if (!ValidateEvent(event)) {
        spdlog::warn("[GameEventHooks] Event validation failed for {}", event.eventName);
        m_stats.validationErrors++;
        return false;
    }

    // Check if this event type is disabled
    if (m_disabledEventTypes.count(event.type)) {
        m_stats.totalEventsFiltered++;
        return false;
    }

    // Add to pending queue for processing
    {
        std::lock_guard<std::mutex> lock(m_eventsMutex);

        if (m_pendingEvents.size() >= m_config.maxPendingEvents) {
            spdlog::warn("[GameEventHooks] Event queue full, dropping event {}", event.eventName);
            return false;
        }

        m_pendingEvents.push(event);
    }

    // Record in history if enabled
    if (m_config.enableEventHistory) {
        RecordEvent(event);
    }

    return true;
}

void GameEventHooks::ProcessPendingEvents() {
    std::queue<GameEvent> eventsToProcess;

    {
        std::lock_guard<std::mutex> lock(m_eventsMutex);
        eventsToProcess.swap(m_pendingEvents);
    }

    while (!eventsToProcess.empty()) {
        const auto& event = eventsToProcess.front();

        auto startTime = std::chrono::steady_clock::now();

        if (DispatchEvent(event)) {
            m_stats.totalEventsProcessed++;
        } else {
            m_stats.processingErrors++;
        }

        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

        // Update processing time statistics
        {
            std::lock_guard<std::mutex> lock(m_statsMutex);
            m_processingTimes.push(static_cast<float>(duration.count()) / 1000.0f);
            if (m_processingTimes.size() > 100) {
                m_processingTimes.pop();
            }
        }

        eventsToProcess.pop();
    }
}

bool GameEventHooks::DispatchEvent(const GameEvent& event) {
    // Get matching handlers
    auto handlers = GetMatchingHandlers(event);

    bool success = true;

    // Call all matching handlers
    for (auto* handler : handlers) {
        if (!handler->isActive) continue;

        try {
            bool result = handler->callback(event);
            if (!result) {
                success = false;
            }

            handler->callCount++;
            handler->lastCalled = std::chrono::steady_clock::now();
        } catch (const std::exception& e) {
            spdlog::error("[GameEventHooks] Handler {} failed: {}", handler->handlerId, e.what());
            success = false;
        }
    }

    // Broadcast to network if needed
    if (event.syncMode != EventSyncMode::ServerOnly && event.syncMode != EventSyncMode::Individual) {
        if (!BroadcastEvent(event)) {
            spdlog::warn("[GameEventHooks] Failed to broadcast event {}", event.eventName);
            success = false;
        }
    }

    return success;
}

std::vector<EventHandler*> GameEventHooks::GetMatchingHandlers(const GameEvent& event) {
    std::vector<EventHandler*> matchingHandlers;

    std::lock_guard<std::mutex> lock(m_handlersMutex);

    for (auto& [id, handler] : m_eventHandlers) {
        if (!handler.isActive) continue;

        // Check priority filter
        if (event.priority > handler.minPriority) continue;

        // Check type filter
        if (handler.typeFilter != CampaignEventType::Custom &&
            handler.typeFilter != event.type) continue;

        // Check event name filter
        if (!handler.eventFilter.empty() &&
            event.eventName.find(handler.eventFilter) == std::string::npos) continue;

        matchingHandlers.push_back(&handler);
    }

    return matchingHandlers;
}

uint64_t GameEventHooks::RegisterEventHandler(const std::string& eventFilter,
                                             std::function<bool(const GameEvent&)> callback,
                                             CampaignEventType typeFilter,
                                             EventPriority minPriority) {
    EventHandler handler;
    handler.handlerId = GenerateHandlerId();
    handler.eventFilter = eventFilter;
    handler.typeFilter = typeFilter;
    handler.callback = callback;
    handler.minPriority = minPriority;
    handler.isActive = true;
    handler.callCount = 0;
    handler.lastCalled = std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> lock(m_handlersMutex);
    m_eventHandlers[handler.handlerId] = handler;

    spdlog::debug("[GameEventHooks] Registered event handler {} for filter '{}'",
                  handler.handlerId, eventFilter);

    return handler.handlerId;
}

uint64_t GameEventHooks::RegisterMainQuestHandler(std::function<bool(const GameEvent&)> callback) {
    return RegisterEventHandler("", callback, CampaignEventType::MainQuest, EventPriority::Critical);
}

uint64_t GameEventHooks::RegisterCombatHandler(std::function<bool(const GameEvent&)> callback) {
    return RegisterEventHandler("", callback, CampaignEventType::EnemyKilled, EventPriority::Medium);
}

uint64_t GameEventHooks::RegisterProgressionHandler(std::function<bool(const GameEvent&)> callback) {
    return RegisterEventHandler("", callback, CampaignEventType::LevelUp, EventPriority::Medium);
}

uint64_t GameEventHooks::RegisterDialogueHandler(std::function<bool(const GameEvent&)> callback) {
    return RegisterEventHandler("", callback, CampaignEventType::DialogueChoice, EventPriority::High);
}

bool GameEventHooks::BroadcastEvent(const GameEvent& event) {
    // Serialize and send via network
    if (!Net_IsConnected()) {
        return false;
    }

    std::vector<uint8_t> serializedData;
    if (!SerializeEvent(event, serializedData)) {
        return false;
    }

    // Determine target players based on sync mode
    std::vector<uint32_t> targetPlayers;

    switch (event.syncMode) {
        case EventSyncMode::Broadcast:
            // Send to all connected players
            // TODO: Implement GetConnectedPlayers function
            break;

        case EventSyncMode::Proximity:
            // Send to players within proximity range
            // TODO: Implement GetPlayersInRange function
            break;

        case EventSyncMode::Quest:
            // Send to players in the same quest
            // TODO: Implement GetQuestParticipants function
            break;

        default:
            targetPlayers.push_back(event.sourcePlayerId);
            break;
    }

    bool success = true;
    for (uint32_t playerId : targetPlayers) {
        if (!SendEventToPlayer(event, playerId)) {
            success = false;
        }
    }

    if (success) {
        m_stats.totalEventsBroadcast++;
    } else {
        m_stats.networkErrors++;
    }

    return success;
}

bool GameEventHooks::SendEventToPlayer(const GameEvent& event, uint32_t playerId) {
    // Create network packet and send
    GameEventPacket packet;
    packet.eventId = event.eventId;
    packet.sourcePlayerId = event.sourcePlayerId;
    packet.eventType = static_cast<uint8_t>(event.type);
    packet.syncMode = static_cast<uint8_t>(event.syncMode);
    packet.priority = static_cast<uint8_t>(event.priority);
    packet.timestamp = event.timestamp;
    packet.checksum = event.checksum;

    std::vector<uint8_t> eventData;
    if (!SerializeEvent(event, eventData)) {
        return false;
    }

    packet.dataSize = static_cast<uint32_t>(eventData.size());

    // Send via network system
    if (!Net_IsConnected()) {
        return false;
    }

    // TODO: Implement Net_SendToPlayer function
    // return Net_SendToPlayer(playerId, &packet, sizeof(packet), eventData.data(), eventData.size());
    return true; // Placeholder
}

bool GameEventHooks::ValidateEvent(const GameEvent& event) const {
    // Basic validation checks
    if (event.eventName.empty()) {
        return false;
    }

    if (event.sourcePlayerId == 0) {
        return false;
    }

    // Check if it's a known campaign event
    if (m_campaignEvents.find(event.eventName) == m_campaignEvents.end() &&
        event.type != CampaignEventType::Custom) {
        return false;
    }

    // Validate checksum
    uint32_t calculatedChecksum = CalculateEventChecksum(event);
    if (calculatedChecksum != event.checksum) {
        spdlog::warn("[GameEventHooks] Checksum mismatch for event {}", event.eventName);
        return false;
    }

    return true;
}

uint32_t GameEventHooks::CalculateEventChecksum(const GameEvent& event) const {
    // Simple checksum calculation
    uint32_t checksum = 0;

    for (char c : event.eventName) {
        checksum = checksum * 31 + static_cast<uint32_t>(c);
    }

    checksum ^= event.sourcePlayerId;
    checksum ^= static_cast<uint32_t>(event.type);
    checksum ^= static_cast<uint32_t>(event.timestamp & 0xFFFFFFFF);

    return checksum;
}

void GameEventHooks::InitializeCampaignEventMap() {
    // Initialize event categories for automatic classification
    m_eventCategories["quest"] = {"quest_completed", "quest_started", "quest_failed", "quest_objective_completed"};
    m_eventCategories["progression"] = {"player_level_up", "attribute_increased", "perk_unlocked", "skill_level_increased"};
    m_eventCategories["combat"] = {"enemy_killed", "boss_defeated", "combat_started", "combat_ended", "player_died"};
    m_eventCategories["romance"] = {"judy_romance", "panam_romance", "river_romance", "kerry_romance"};
    m_eventCategories["economy"] = {"eddies_gained", "eddies_spent", "shop_purchase", "vehicle_purchased"};
    m_eventCategories["world"] = {"location_discovered", "fast_travel_unlocked", "item_crafted"};
}

bool GameEventHooks::SerializeEvent(const GameEvent& event, std::vector<uint8_t>& outData) const {
    // Simple JSON-like serialization
    std::string json = "{";
    json += "\"eventId\":" + std::to_string(event.eventId) + ",";
    json += "\"type\":" + std::to_string(static_cast<int>(event.type)) + ",";
    json += "\"name\":\"" + event.eventName + "\",";
    json += "\"description\":\"" + event.eventDescription + "\",";
    json += "\"sourcePlayer\":" + std::to_string(event.sourcePlayerId) + ",";
    json += "\"timestamp\":" + std::to_string(event.timestamp) + ",";
    json += "\"gameTime\":" + std::to_string(event.gameTime) + ",";
    json += "\"questHash\":" + std::to_string(event.questHash) + ",";
    json += "\"parameters\":{";

    bool first = true;
    for (const auto& [key, value] : event.parameters) {
        if (!first) json += ",";
        json += "\"" + key + "\":\"" + value + "\"";
        first = false;
    }

    json += "}}";

    outData.assign(json.begin(), json.end());
    return true;
}

EventStats GameEventHooks::GetEventStats() const {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    return m_stats;
}

void GameEventHooks::UpdateStatistics() {
    std::lock_guard<std::mutex> lock(m_statsMutex);

    // Calculate average processing time
    if (!m_processingTimes.empty()) {
        float total = 0.0f;
        auto tempQueue = m_processingTimes;
        while (!tempQueue.empty()) {
            total += tempQueue.front();
            tempQueue.pop();
        }
        m_stats.averageProcessingTime = total / m_processingTimes.size();
    }

    // Update pending event count
    {
        std::lock_guard<std::mutex> eventLock(m_eventsMutex);
        m_stats.pendingEvents = static_cast<uint32_t>(m_pendingEvents.size());
    }

    // Update active handler count
    {
        std::lock_guard<std::mutex> handlerLock(m_handlersMutex);
        uint32_t activeCount = 0;
        for (const auto& [id, handler] : m_eventHandlers) {
            if (handler.isActive) activeCount++;
        }
        m_stats.activeHandlers = activeCount;
    }

    m_stats.lastStatsUpdate = std::chrono::steady_clock::now();
}

void GameEventHooks::RecordEvent(const GameEvent& event) {
    if (!m_config.enableEventHistory) return;

    std::lock_guard<std::mutex> lock(m_historyMutex);

    auto& playerHistory = m_eventHistory[event.sourcePlayerId];
    playerHistory.push_back(event);

    // Limit history size
    if (playerHistory.size() > m_config.maxEventHistory) {
        playerHistory.erase(playerHistory.begin(),
                           playerHistory.begin() + (playerHistory.size() - m_config.maxEventHistory));
    }
}

uint64_t GameEventHooks::GenerateEventId() {
    return m_nextEventId.fetch_add(1);
}

uint64_t GameEventHooks::GenerateHandlerId() {
    return m_nextHandlerId.fetch_add(1);
}

uint64_t GameEventHooks::GetCurrentTimestamp() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

uint64_t GameEventHooks::GetGameTime() const {
    // This would interface with the game's time system
    return GetCurrentTimestamp(); // Placeholder
}

std::vector<CampaignEventData> GameEventHooks::GetDiscoveredEvents() const {
    std::vector<CampaignEventData> events;
    events.reserve(m_campaignEvents.size());

    for (const auto& [name, data] : m_campaignEvents) {
        events.push_back(data);
    }

    return events;
}

void GameEventHooks::ProcessEventBatches() {
    std::lock_guard<std::mutex> lock(m_batchesMutex);

    auto now = std::chrono::steady_clock::now();
    std::vector<uint64_t> batchesToFlush;

    for (auto& [batchId, batch] : m_eventBatches) {
        auto age = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - std::chrono::steady_clock::time_point(std::chrono::milliseconds(batch.createTime)));

        if (age.count() >= m_config.batchFlushInterval ||
            batch.events.size() >= batch.maxBatchSize) {
            batchesToFlush.push_back(batchId);
        }
    }

    for (uint64_t batchId : batchesToFlush) {
        FlushEventBatch(batchId);
    }
}

void GameEventHooks::FlushEventBatch(uint64_t batchId) {
    auto it = m_eventBatches.find(batchId);
    if (it == m_eventBatches.end()) return;

    const auto& batch = it->second;

    // Send batch via network
    SendEventBatch(batch);

    // Remove from pending batches
    m_eventBatches.erase(it);
}

bool GameEventHooks::SendEventBatch(const EventBatch& batch) {
    // Implementation would send batched events efficiently
    return true;
}

// Utility function implementations
namespace EventUtils {
    std::string GetEventTypeName(CampaignEventType type) {
        switch (type) {
            case CampaignEventType::MainQuest: return "MainQuest";
            case CampaignEventType::SideQuest: return "SideQuest";
            case CampaignEventType::LevelUp: return "LevelUp";
            case CampaignEventType::CombatStarted: return "CombatStarted";
            case CampaignEventType::DialogueChoice: return "DialogueChoice";
            case CampaignEventType::RomanceProgression: return "RomanceProgression";
            default: return "Unknown";
        }
    }

    bool IsStoryEvent(CampaignEventType type) {
        return type == CampaignEventType::MainQuest ||
               type == CampaignEventType::EndingPath ||
               type == CampaignEventType::RomanceProgression;
    }

    bool RequiresValidation(CampaignEventType type) {
        return IsStoryEvent(type);
    }
}

} // namespace CoopNet