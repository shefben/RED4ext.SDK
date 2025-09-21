#include "EnhancedQuestManager.hpp"
#include "../core/Logger.hpp"
#include "../core/CoopExports.hpp"
#include "../net/Net.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <random>

namespace CoopNet {

// Cyberpunk 2077 Story Quest Integration
namespace CP2077Quests {
    // Main story quest hashes (these need to be extracted from the actual game)
    const std::unordered_set<uint32_t> MAIN_STORY_QUESTS = {
        // Act 1
        0x12345678, // "The Rescue" (prologue)
        0x23456789, // "The Pickup"
        0x34567890, // "The Information"
        0x45678901, // "The Heist"
        0x56789012, // "Love Like Fire"
        0x67890123, // "Playing for Time"

        // Act 2
        0x78901234, // "Automatic Love"
        0x89012345, // "Ghost Town"
        0x90123456, // "Lightning Breaks"
        0x01234567, // "Life During Wartime"
        0x12345679, // "Search and Destroy"
        0x23456780, // "Never Fade Away"
        0x34567891, // "Tapeworm"
        0x45678902, // "Chippin' In"
        0x56789013, // "Blistering Love"

        // Act 3 & Endings
        0x67890124, // "Nocturne Op55N1"
        0x78901235, // "Totalimmortal"
        0x89012346, // "New Dawn Fades"
        0x90123457, // "Where is My Mind?"
        0x01234568, // "Path of Glory"
        0x12345680  // "The Sun"
    };

    const std::unordered_set<uint32_t> ROMANCE_QUESTS = {
        0x11111111, // Panam romance
        0x22222222, // Judy romance
        0x33333333, // River romance
        0x44444444, // Kerry romance
    };

    const std::unordered_set<uint32_t> CRITICAL_SYNC_QUESTS = {
        0x45678901, // "The Heist" - critical story point
        0x67890123, // "Playing for Time" - introduces Johnny
        0x67890124, // "Nocturne Op55N1" - point of no return
    };
}

EnhancedQuestManager& EnhancedQuestManager::Instance() {
    static EnhancedQuestManager instance;
    return instance;
}

bool EnhancedQuestManager::Initialize() {
    std::lock_guard<std::mutex> lock(m_questsMutex);

    if (m_initialized) {
        return true;
    }

    // Initialize the quest system
    m_quests.clear();
    m_players.clear();
    m_conflicts.clear();
    m_stats = {};
    m_lastStatsUpdate = std::chrono::steady_clock::now();

    // Register all Cyberpunk 2077 main story quests
    RegisterCP2077StoryQuests();

    // Load custom quest definitions from configuration
    LoadCustomQuestDefinitions();

    m_initialized = true;
    Logger::Log(LogLevel::INFO, "[EnhancedQuestManager] Quest system initialized successfully");
    return true;
}

void EnhancedQuestManager::RegisterCP2077StoryQuests() {
    Logger::Log(LogLevel::INFO, "[EnhancedQuestManager] Registering CP2077 story quests");

    // Register main story quests with strict synchronization
    for (uint32_t questHash : CP2077Quests::MAIN_STORY_QUESTS) {
        std::string questName = "cp2077_main_" + std::to_string(questHash);

        QuestSyncMode syncMode = QuestSyncMode::Strict;
        QuestPriority priority = QuestPriority::High;

        // Critical story moments require consensus
        if (CP2077Quests::CRITICAL_SYNC_QUESTS.count(questHash)) {
            syncMode = QuestSyncMode::Consensus;
            priority = QuestPriority::Critical;
        }

        RegisterQuest(questHash, questName, QuestType::Main, priority, syncMode);

        auto* quest = GetQuest(questHash);
        if (quest) {
            quest->conflictMode = ConflictResolution::Vote;
            // Add quest dependencies for story progression
            AddStoryQuestDependencies(questHash, quest);
        }
    }

    // Register romance quests with individual progression
    for (uint32_t questHash : CP2077Quests::ROMANCE_QUESTS) {
        std::string questName = "cp2077_romance_" + std::to_string(questHash);
        RegisterQuest(questHash, questName, QuestType::Romance, QuestPriority::Medium, QuestSyncMode::Individual);
    }

    Logger::Log(LogLevel::INFO, "[EnhancedQuestManager] Registered " +
               std::to_string(CP2077Quests::MAIN_STORY_QUESTS.size()) + " main story quests");
}

void EnhancedQuestManager::AddStoryQuestDependencies(uint32_t questHash, QuestSyncData* quest) {
    // Add story progression dependencies (simplified example)
    switch (questHash) {
        case 0x23456789: // "The Pickup"
            quest->prerequisiteQuests = {0x12345678}; // Requires "The Rescue"
            break;
        case 0x34567890: // "The Information"
            quest->prerequisiteQuests = {0x23456789}; // Requires "The Pickup"
            break;
        case 0x45678901: // "The Heist"
            quest->prerequisiteQuests = {0x34567890}; // Requires "The Information"
            break;
        case 0x67890123: // "Playing for Time"
            quest->prerequisiteQuests = {0x45678901}; // Requires "The Heist"
            break;
        // Add more dependencies as needed
    }
}

void EnhancedQuestManager::LoadCustomQuestDefinitions() {
    Logger::Log(LogLevel::INFO, "[EnhancedQuestManager] Loading custom quest definitions");

    // Load custom quests from JSON configuration file
    std::ifstream configFile("custom_quests.json");
    if (!configFile.is_open()) {
        Logger::Log(LogLevel::WARNING, "[EnhancedQuestManager] No custom quest configuration found, creating default");
        CreateDefaultCustomQuests();
        return;
    }

    // TODO: Parse JSON configuration for custom quests
    // For now, create some example custom quests
    CreateDefaultCustomQuests();
}

void EnhancedQuestManager::CreateDefaultCustomQuests() {
    // Example custom multiplayer quests

    // Cooperative heist quest
    uint32_t heistQuest = QuestUtils::HashQuestName("custom_coop_heist");
    RegisterQuest(heistQuest, "Cooperative Heist", QuestType::Custom, QuestPriority::High, QuestSyncMode::Strict);
    auto* heist = GetQuest(heistQuest);
    if (heist) {
        heist->objectives = {
            {1, heistQuest, "Infiltrate the building", QuestState::Active, false, true, {}, 0},
            {2, heistQuest, "Disable security systems", QuestState::Inactive, false, false, {}, 0},
            {3, heistQuest, "Steal the data", QuestState::Inactive, false, true, {}, 0},
            {4, heistQuest, "Escape safely", QuestState::Inactive, false, true, {}, 0}
        };
    }

    // Gang war quest
    uint32_t gangWarQuest = QuestUtils::HashQuestName("custom_gang_war");
    RegisterQuest(gangWarQuest, "Gang Territory War", QuestType::Custom, QuestPriority::Medium, QuestSyncMode::Majority);
    auto* gangWar = GetQuest(gangWarQuest);
    if (gangWar) {
        gangWar->objectives = {
            {1, gangWarQuest, "Take control of 3 territories", QuestState::Active, false, false, {}, 0},
            {2, gangWarQuest, "Eliminate rival gang leaders", QuestState::Inactive, true, false, {}, 0},
            {3, gangWarQuest, "Establish your dominance", QuestState::Inactive, false, true, {}, 0}
        };
    }

    // Racing circuit quest
    uint32_t racingQuest = QuestUtils::HashQuestName("custom_street_racing");
    RegisterQuest(racingQuest, "Underground Racing Circuit", QuestType::Custom, QuestPriority::Low, QuestSyncMode::Individual);
    auto* racing = GetQuest(racingQuest);
    if (racing) {
        racing->objectives = {
            {1, racingQuest, "Win 5 street races", QuestState::Active, false, false, {}, 0},
            {2, racingQuest, "Unlock elite racing tier", QuestState::Inactive, false, false, {}, 0},
            {3, racingQuest, "Become the street racing champion", QuestState::Inactive, false, false, {}, 0}
        };
    }

    Logger::Log(LogLevel::INFO, "[EnhancedQuestManager] Created default custom quests");
}

bool EnhancedQuestManager::RegisterQuest(uint32_t questHash, const std::string& questName,
                                       QuestType type, QuestPriority priority, QuestSyncMode syncMode) {
    std::lock_guard<std::mutex> lock(m_questsMutex);

    if (m_quests.find(questHash) != m_quests.end()) {
        Logger::Log(LogLevel::WARNING, "[EnhancedQuestManager] Quest already registered: " + questName);
        return false;
    }

    auto quest = std::make_unique<QuestSyncData>();
    quest->questHash = questHash;
    quest->questName = questName;
    quest->type = type;
    quest->priority = priority;
    quest->syncMode = syncMode;
    quest->conflictMode = (type == QuestType::Main) ? ConflictResolution::Vote : ConflictResolution::AutoResolve;
    quest->authorityStage = 0;
    quest->authorityState = QuestState::Inactive;
    quest->questLeader = 0;
    quest->lastSyncTime = GetCurrentTimestamp();
    quest->syncAttempts = 0;
    quest->hasPendingConflict = false;
    quest->hasActiveVote = false;

    // Set appropriate conflict resolution for CP2077 story quests
    if (type == QuestType::Main && CP2077Quests::CRITICAL_SYNC_QUESTS.count(questHash)) {
        quest->conflictMode = ConflictResolution::Vote;
        quest->syncMode = QuestSyncMode::Consensus;
    }

    m_quests[questHash] = std::move(quest);
    Logger::Log(LogLevel::DEBUG, "[EnhancedQuestManager] Registered quest: " + questName +
               " (Type: " + QuestUtils::GetQuestTypeName(type) + ")");
    return true;
}

bool EnhancedQuestManager::UpdateQuestStage(uint32_t playerId, uint32_t questHash, uint16_t newStage) {
    std::lock_guard<std::mutex> lock(m_questsMutex);

    auto questIt = m_quests.find(questHash);
    if (questIt == m_quests.end()) {
        Logger::Log(LogLevel::WARNING, "[EnhancedQuestManager] Attempted to update unknown quest: " + std::to_string(questHash));
        return false;
    }

    auto* quest = questIt->second.get();

    // Get or create player progress
    auto& playerProgress = quest->playerProgress[playerId];
    playerProgress.playerId = playerId;
    playerProgress.questHash = questHash;

    uint16_t oldStage = playerProgress.currentStage;
    playerProgress.currentStage = newStage;
    playerProgress.lastUpdate = GetCurrentTimestamp();

    Logger::Log(LogLevel::DEBUG, "[EnhancedQuestManager] Player " + std::to_string(playerId) +
               " quest " + quest->questName + " stage: " + std::to_string(oldStage) + " -> " + std::to_string(newStage));

    // Handle synchronization based on quest type and sync mode
    switch (quest->syncMode) {
        case QuestSyncMode::Strict:
            // For CP2077 main story - ensure all players are synchronized
            if (quest->type == QuestType::Main) {
                HandleStoryQuestSync(quest, playerId, newStage);
            } else {
                SynchronizeQuest(questHash);
            }
            break;

        case QuestSyncMode::Individual:
            // Player can progress independently (side quests, custom content)
            break;

        case QuestSyncMode::Majority:
        case QuestSyncMode::Consensus:
            // Check if consensus/majority is reached
            CheckForConsensus(quest);
            break;

        case QuestSyncMode::Leader:
            // Only quest leader can advance
            if (quest->questLeader == playerId || quest->questLeader == 0) {
                quest->authorityStage = newStage;
                SynchronizeQuest(questHash);
            }
            break;
    }

    return true;
}

void EnhancedQuestManager::HandleStoryQuestSync(QuestSyncData* quest, uint32_t playerId, uint16_t newStage) {
    // Special handling for CP2077 story quests
    bool isStoryQuest = (quest->type == QuestType::Main &&
                        CP2077Quests::MAIN_STORY_QUESTS.count(quest->questHash));

    if (!isStoryQuest) {
        SynchronizeQuest(quest->questHash);
        return;
    }

    // Check if this is a critical story moment
    bool isCriticalQuest = CP2077Quests::CRITICAL_SYNC_QUESTS.count(quest->questHash);

    if (isCriticalQuest) {
        // Critical story moments require consensus or voting
        Logger::Log(LogLevel::INFO, "[EnhancedQuestManager] Critical story quest progression detected: " + quest->questName);

        if (quest->syncMode == QuestSyncMode::Consensus) {
            // Start consensus check
            CheckForConsensus(quest);
        } else {
            // Start voting process
            StartConflictVote(quest->questHash, newStage, playerId);
        }
    } else {
        // Regular story quest - advance all players to keep story coherent
        quest->authorityStage = std::max(quest->authorityStage, newStage);

        // Advance all players to the highest stage to prevent story breaking
        for (auto& [otherPlayerId, progress] : quest->playerProgress) {
            if (progress.currentStage < quest->authorityStage) {
                progress.currentStage = quest->authorityStage;
                progress.lastUpdate = GetCurrentTimestamp();

                Logger::Log(LogLevel::INFO, "[EnhancedQuestManager] Advanced player " + std::to_string(otherPlayerId) +
                           " to stage " + std::to_string(quest->authorityStage) + " for story coherence");
            }
        }

        SynchronizeQuest(quest->questHash);
    }
}

void EnhancedQuestManager::CheckForConsensus(QuestSyncData* quest) {
    if (quest->playerProgress.empty()) {
        return;
    }

    // Count votes for each stage
    std::unordered_map<uint16_t, uint32_t> stageCounts;
    uint32_t totalPlayers = quest->playerProgress.size();

    for (const auto& [playerId, progress] : quest->playerProgress) {
        stageCounts[progress.currentStage]++;
    }

    // Find the stage with consensus/majority
    uint16_t consensusStage = 0;
    uint32_t maxVotes = 0;
    bool hasConsensus = false;

    for (const auto& [stage, count] : stageCounts) {
        if (count > maxVotes) {
            maxVotes = count;
            consensusStage = stage;
        }
    }

    // Check if consensus is achieved
    if (quest->syncMode == QuestSyncMode::Consensus) {
        hasConsensus = (maxVotes == totalPlayers);
    } else if (quest->syncMode == QuestSyncMode::Majority) {
        hasConsensus = (maxVotes > totalPlayers / 2);
    }

    if (hasConsensus) {
        quest->authorityStage = consensusStage;

        // Update all players to consensus stage
        for (auto& [playerId, progress] : quest->playerProgress) {
            if (progress.currentStage != consensusStage) {
                progress.currentStage = consensusStage;
                progress.lastUpdate = GetCurrentTimestamp();
            }
        }

        SynchronizeQuest(quest->questHash);
        Logger::Log(LogLevel::INFO, "[EnhancedQuestManager] Consensus reached for quest " + quest->questName +
                   " at stage " + std::to_string(consensusStage));
    }
}

bool EnhancedQuestManager::StartConflictVote(uint32_t questHash, uint16_t targetStage, uint32_t initiatingPlayer) {
    std::lock_guard<std::mutex> lock(m_questsMutex);

    auto questIt = m_quests.find(questHash);
    if (questIt == m_quests.end()) {
        return false;
    }

    auto* quest = questIt->second.get();

    if (quest->hasActiveVote) {
        Logger::Log(LogLevel::WARNING, "[EnhancedQuestManager] Vote already active for quest: " + quest->questName);
        return false;
    }

    quest->hasActiveVote = true;
    quest->voteTargetStage = targetStage;
    quest->voteDeadline = GetCurrentTimestamp() + (m_config.voteTimeoutSeconds * 1000);
    quest->playerVotes.clear();

    // Automatically approve for the initiating player
    quest->playerVotes[initiatingPlayer] = true;

    Logger::Log(LogLevel::INFO, "[EnhancedQuestManager] Started vote for quest " + quest->questName +
               " to advance to stage " + std::to_string(targetStage));

    // Broadcast vote notification to all players
    TriggerEvent("quest_vote_started", questHash, initiatingPlayer,
                "target_stage:" + std::to_string(targetStage));

    return true;
}

bool EnhancedQuestManager::CastConflictVote(uint32_t questHash, uint32_t playerId, bool approve) {
    std::lock_guard<std::mutex> lock(m_questsMutex);

    auto questIt = m_quests.find(questHash);
    if (questIt == m_quests.end()) {
        return false;
    }

    auto* quest = questIt->second.get();

    if (!quest->hasActiveVote) {
        Logger::Log(LogLevel::WARNING, "[EnhancedQuestManager] No active vote for quest: " + quest->questName);
        return false;
    }

    quest->playerVotes[playerId] = approve;

    Logger::Log(LogLevel::DEBUG, "[EnhancedQuestManager] Player " + std::to_string(playerId) +
               " voted " + (approve ? "YES" : "NO") + " for quest " + quest->questName);

    // Check if all players have voted
    uint32_t totalPlayers = quest->playerProgress.size();
    if (quest->playerVotes.size() >= totalPlayers) {
        ProcessVoteResult(quest);
    }

    return true;
}

void EnhancedQuestManager::ProcessVoteResult(QuestSyncData* quest) {
    uint32_t yesVotes = 0;
    uint32_t totalVotes = quest->playerVotes.size();

    for (const auto& [playerId, vote] : quest->playerVotes) {
        if (vote) yesVotes++;
    }

    bool votePass = (yesVotes > totalVotes / 2);

    if (votePass) {
        quest->authorityStage = quest->voteTargetStage;

        // Update all players to the voted stage
        for (auto& [playerId, progress] : quest->playerProgress) {
            progress.currentStage = quest->voteTargetStage;
            progress.lastUpdate = GetCurrentTimestamp();
        }

        Logger::Log(LogLevel::INFO, "[EnhancedQuestManager] Vote PASSED for quest " + quest->questName +
                   " (" + std::to_string(yesVotes) + "/" + std::to_string(totalVotes) + ")");

        SynchronizeQuest(quest->questHash);
        TriggerEvent("quest_vote_passed", quest->questHash, 0, "stage:" + std::to_string(quest->voteTargetStage));
    } else {
        Logger::Log(LogLevel::INFO, "[EnhancedQuestManager] Vote FAILED for quest " + quest->questName +
                   " (" + std::to_string(yesVotes) + "/" + std::to_string(totalVotes) + ")");

        TriggerEvent("quest_vote_failed", quest->questHash, 0, "stage:" + std::to_string(quest->voteTargetStage));
    }

    // Clear vote state
    quest->hasActiveVote = false;
    quest->voteTargetStage = 0;
    quest->voteDeadline = 0;
    quest->playerVotes.clear();
}

// Utility function implementations
namespace QuestUtils {
    uint32_t HashQuestName(const std::string& questName) {
        // Simple hash function - in real implementation would use game's hash function
        uint32_t hash = 0;
        for (char c : questName) {
            hash = hash * 31 + static_cast<uint32_t>(c);
        }
        return hash;
    }

    std::string GetQuestTypeName(QuestType type) {
        switch (type) {
            case QuestType::Main: return "Main Story";
            case QuestType::Side: return "Side Quest";
            case QuestType::Gig: return "Gig";
            case QuestType::NCPD: return "NCPD Scanner";
            case QuestType::Romance: return "Romance";
            case QuestType::Corporate: return "Corporate";
            case QuestType::Fixer: return "Fixer";
            case QuestType::Custom: return "Custom";
            default: return "Unknown";
        }
    }

    QuestPriority DetermineQuestPriority(QuestType type) {
        switch (type) {
            case QuestType::Main: return QuestPriority::Critical;
            case QuestType::Romance: return QuestPriority::High;
            case QuestType::Side: return QuestPriority::Medium;
            case QuestType::Custom: return QuestPriority::Medium;
            default: return QuestPriority::Low;
        }
    }

    QuestSyncMode GetRecommendedSyncMode(QuestType type) {
        switch (type) {
            case QuestType::Main: return QuestSyncMode::Strict;
            case QuestType::Custom: return QuestSyncMode::Majority;
            case QuestType::Romance: return QuestSyncMode::Individual;
            default: return QuestSyncMode::Individual;
        }
    }

    std::string GetQuestStateName(QuestState state) {
        switch (state) {
            case QuestState::Inactive: return "Inactive";
            case QuestState::Active: return "Active";
            case QuestState::Completed: return "Completed";
            case QuestState::Failed: return "Failed";
            case QuestState::Suspended: return "Suspended";
            case QuestState::Locked: return "Locked";
            default: return "Unknown";
        }
    }

    std::string GetSyncModeName(QuestSyncMode mode) {
        switch (mode) {
            case QuestSyncMode::Strict: return "Strict";
            case QuestSyncMode::Majority: return "Majority";
            case QuestSyncMode::Individual: return "Individual";
            case QuestSyncMode::Leader: return "Leader";
            case QuestSyncMode::Consensus: return "Consensus";
            default: return "Unknown";
        }
    }

    bool IsQuestNameValid(const std::string& questName) {
        if (questName.empty() || questName.length() > 128) {
            return false;
        }

        // Check for valid characters (alphanumeric, spaces, underscores, hyphens)
        for (char c : questName) {
            if (!std::isalnum(c) && c != ' ' && c != '_' && c != '-') {
                return false;
            }
        }

        return true;
    }
}

void EnhancedQuestManager::Tick(float deltaTime) {
    if (!m_initialized) {
        return;
    }

    // Update timers
    m_syncTimer += deltaTime;
    m_conflictTimer += deltaTime;
    m_validationTimer += deltaTime;

    // Process synchronization queue
    if (m_syncTimer >= m_config.syncInterval) {
        ProcessSynchronization();
        m_syncTimer = 0.0f;
    }

    // Check for conflicts
    if (m_conflictTimer >= m_config.conflictCheckInterval) {
        ProcessConflictResolution();
        m_conflictTimer = 0.0f;
    }

    // Validate quest integrity
    if (m_validationTimer >= m_config.validationInterval) {
        ValidateQuestIntegrity();
        m_validationTimer = 0.0f;
    }

    // Process voting timeouts
    ProcessVoting();

    // Cleanup expired data
    CleanupExpiredData();
}

void EnhancedQuestManager::Shutdown() {
    std::lock_guard<std::mutex> questsLock(m_questsMutex);
    std::lock_guard<std::mutex> playersLock(m_playersMutex);
    std::lock_guard<std::mutex> conflictsLock(m_conflictsMutex);

    if (!m_initialized) {
        return;
    }

    // Save quest snapshot before shutdown
    SaveQuestSnapshot("quest_shutdown_backup.json");

    // Clear all data structures
    m_quests.clear();
    m_players.clear();
    m_conflicts.clear();
    m_eventCallbacks.clear();
    m_questCheckpoints.clear();

    // Clear queues
    std::queue<uint32_t> empty1;
    std::queue<uint32_t> empty2;
    m_syncQueue.swap(empty1);
    m_validationQueue.swap(empty2);

    m_initialized = false;
    Logger::Log(LogLevel::INFO, "[EnhancedQuestManager] Quest system shutdown complete");
}

void EnhancedQuestManager::SynchronizeQuest(uint32_t questHash) {
    std::lock_guard<std::mutex> lock(m_questsMutex);

    auto questIt = m_quests.find(questHash);
    if (questIt == m_quests.end()) {
        return;
    }

    auto* quest = questIt->second.get();

    // Update sync metadata
    quest->lastSyncTime = GetCurrentTimestamp();
    quest->syncAttempts++;

    // Create sync packet for network transmission
    EnhancedQuestSyncPacket packet;
    packet.questHash = questHash;
    packet.targetStage = quest->authorityStage;
    packet.targetState = quest->authorityState;
    packet.playerId = 0; // Server authority
    packet.timestamp = quest->lastSyncTime;
    packet.syncType = 0; // Normal sync

    // Send to network manager for distribution
    if (Net_GetConnectedPlayerCount() > 1) {
        Net_Broadcast(CoopNet::EMsg::QuestFullSync, &packet, sizeof(EnhancedQuestSyncPacket));
        Logger::Log(LogLevel::DEBUG, "[EnhancedQuestManager] Broadcasting quest sync packet for " + quest->questName);
    }

    // Update statistics
    m_stats.syncOperationsPerSecond++;

    // Trigger sync event
    TriggerEvent("quest_synchronized", questHash, 0,
                "stage:" + std::to_string(quest->authorityStage) +
                ",state:" + std::to_string(static_cast<uint8_t>(quest->authorityState)));

    Logger::Log(LogLevel::DEBUG, "[EnhancedQuestManager] Synchronized quest " + quest->questName +
               " to stage " + std::to_string(quest->authorityStage));
}

void EnhancedQuestManager::ProcessSynchronization() {
    std::lock_guard<std::mutex> queueLock(m_queueMutex);
    std::lock_guard<std::mutex> questsLock(m_questsMutex);

    // Process pending sync requests
    while (!m_syncQueue.empty()) {
        uint32_t questHash = m_syncQueue.front();
        m_syncQueue.pop();

        auto questIt = m_quests.find(questHash);
        if (questIt != m_quests.end()) {
            auto* quest = questIt->second.get();

            // Check if quest needs synchronization based on priority
            uint64_t timeSinceLastSync = GetCurrentTimestamp() - quest->lastSyncTime;
            uint64_t syncThreshold = GetSyncThresholdForPriority(quest->priority);

            if (timeSinceLastSync >= syncThreshold) {
                SynchronizeQuest(questHash);
            }
        }
    }

    // Auto-sync high priority quests
    for (const auto& [questHash, quest] : m_quests) {
        if (quest->priority == QuestPriority::Critical || quest->priority == QuestPriority::High) {
            uint64_t timeSinceLastSync = GetCurrentTimestamp() - quest->lastSyncTime;
            uint64_t syncThreshold = GetSyncThresholdForPriority(quest->priority);

            if (timeSinceLastSync >= syncThreshold) {
                SynchronizeQuest(questHash);
            }
        }
    }
}

void EnhancedQuestManager::ProcessConflictResolution() {
    std::lock_guard<std::mutex> conflictsLock(m_conflictsMutex);
    std::lock_guard<std::mutex> questsLock(m_questsMutex);

    auto conflicts = DetectConflicts();

    for (const auto& conflict : conflicts) {
        if (conflict.isResolved) {
            continue;
        }

        auto questIt = m_quests.find(conflict.questHash);
        if (questIt == m_quests.end()) {
            continue;
        }

        auto* quest = questIt->second.get();

        // Apply conflict resolution strategy
        bool resolved = false;
        switch (conflict.resolutionMethod) {
            case ConflictResolution::RollbackAll:
                resolved = ResolveByRollback(conflict);
                break;
            case ConflictResolution::AdvanceAll:
                resolved = ResolveByAdvance(conflict);
                break;
            case ConflictResolution::Vote:
                resolved = ResolveByVote(conflict);
                break;
            case ConflictResolution::LeaderDecides:
                resolved = ResolveByLeader(conflict);
                break;
            case ConflictResolution::AutoResolve:
                resolved = ResolveAutomatically(conflict);
                break;
        }

        if (resolved) {
            quest->hasPendingConflict = false;
            SynchronizeQuest(conflict.questHash);

            Logger::Log(LogLevel::INFO, "[EnhancedQuestManager] Resolved conflict for quest " + quest->questName);
            TriggerEvent("quest_conflict_resolved", conflict.questHash, 0, "method:" + std::to_string(static_cast<uint8_t>(conflict.resolutionMethod)));
        }
    }
}

void EnhancedQuestManager::ProcessVoting() {
    std::lock_guard<std::mutex> lock(m_questsMutex);

    uint64_t currentTime = GetCurrentTimestamp();

    for (auto& [questHash, quest] : m_quests) {
        if (quest->hasActiveVote && currentTime >= quest->voteDeadline) {
            // Vote timed out
            Logger::Log(LogLevel::WARNING, "[EnhancedQuestManager] Vote timed out for quest " + quest->questName);

            ProcessVoteResult(quest.get());
            TriggerEvent("quest_vote_timeout", questHash, 0, "target_stage:" + std::to_string(quest->voteTargetStage));
        }
    }
}

void EnhancedQuestManager::ValidateQuestIntegrity() {
    std::lock_guard<std::mutex> lock(m_questsMutex);

    uint32_t errorCount = 0;

    for (const auto& [questHash, quest] : m_quests) {
        auto validation = ValidateQuestState(questHash);

        if (!validation.isValid) {
            errorCount += validation.errors.size();

            Logger::Log(LogLevel::WARNING, "[EnhancedQuestManager] Quest validation failed for " + quest->questName);
            for (const auto& error : validation.errors) {
                Logger::Log(LogLevel::WARNING, "[EnhancedQuestManager] - " + error);
            }

            // Attempt auto-repair if enabled
            if (m_config.enableAutoRepair) {
                if (RepairQuestState(questHash)) {
                    Logger::Log(LogLevel::INFO, "[EnhancedQuestManager] Auto-repaired quest " + quest->questName);
                }
            }
        }
    }

    m_stats.validationErrors = errorCount;
}

void EnhancedQuestManager::CleanupExpiredData() {
    std::lock_guard<std::mutex> conflictsLock(m_conflictsMutex);
    std::lock_guard<std::mutex> checkpointLock(m_checkpointMutex);

    uint64_t currentTime = GetCurrentTimestamp();
    const uint64_t CLEANUP_THRESHOLD = 24 * 60 * 60 * 1000; // 24 hours

    // Clean up old conflicts
    auto conflictIt = m_conflicts.begin();
    while (conflictIt != m_conflicts.end()) {
        if (conflictIt->second->isResolved &&
            (currentTime - conflictIt->second->detectedTime) > CLEANUP_THRESHOLD) {
            conflictIt = m_conflicts.erase(conflictIt);
        } else {
            ++conflictIt;
        }
    }

    // Clean up old checkpoints (keep only latest N checkpoints per quest)
    for (auto& [questHash, checkpoints] : m_questCheckpoints) {
        if (checkpoints.size() > m_config.maxQuestHistory) {
            checkpoints.erase(checkpoints.begin(),
                            checkpoints.begin() + (checkpoints.size() - m_config.maxQuestHistory));
        }
    }
}

uint64_t EnhancedQuestManager::GetSyncThresholdForPriority(QuestPriority priority) const {
    switch (priority) {
        case QuestPriority::Critical: return 0;      // Immediate
        case QuestPriority::High: return 1000;       // 1 second
        case QuestPriority::Medium: return 5000;     // 5 seconds
        case QuestPriority::Low: return 30000;       // 30 seconds
        case QuestPriority::Background: return 300000; // 5 minutes
        default: return 5000;
    }
}

bool EnhancedQuestManager::ResolveByRollback(const QuestConflict& conflict) {
    auto questIt = m_quests.find(conflict.questHash);
    if (questIt == m_quests.end()) {
        return false;
    }

    auto* quest = questIt->second.get();

    // Find the minimum stage among all conflicting players
    uint16_t rollbackStage = UINT16_MAX;
    for (uint32_t playerId : conflict.affectedPlayers) {
        auto progressIt = quest->playerProgress.find(playerId);
        if (progressIt != quest->playerProgress.end()) {
            rollbackStage = std::min(rollbackStage, progressIt->second.currentStage);
        }
    }

    if (rollbackStage == UINT16_MAX) {
        return false;
    }

    // Roll back all players to the minimum stage
    quest->authorityStage = rollbackStage;
    for (uint32_t playerId : conflict.affectedPlayers) {
        auto& progress = quest->playerProgress[playerId];
        progress.currentStage = rollbackStage;
        progress.lastUpdate = GetCurrentTimestamp();
    }

    return true;
}

bool EnhancedQuestManager::ResolveByAdvance(const QuestConflict& conflict) {
    auto questIt = m_quests.find(conflict.questHash);
    if (questIt == m_quests.end()) {
        return false;
    }

    auto* quest = questIt->second.get();

    // Find the maximum stage among all conflicting players
    uint16_t advanceStage = 0;
    for (uint32_t playerId : conflict.affectedPlayers) {
        auto progressIt = quest->playerProgress.find(playerId);
        if (progressIt != quest->playerProgress.end()) {
            advanceStage = std::max(advanceStage, progressIt->second.currentStage);
        }
    }

    // Advance all players to the maximum stage
    quest->authorityStage = advanceStage;
    for (uint32_t playerId : conflict.affectedPlayers) {
        auto& progress = quest->playerProgress[playerId];
        progress.currentStage = advanceStage;
        progress.lastUpdate = GetCurrentTimestamp();
    }

    return true;
}

bool EnhancedQuestManager::ResolveByVote(const QuestConflict& conflict) {
    auto questIt = m_quests.find(conflict.questHash);
    if (questIt == m_quests.end()) {
        return false;
    }

    auto* quest = questIt->second.get();

    if (!quest->hasActiveVote) {
        // Start a vote to resolve the conflict
        uint16_t targetStage = *std::max_element(conflict.conflictingStages.begin(), conflict.conflictingStages.end());
        return StartConflictVote(conflict.questHash, targetStage, conflict.affectedPlayers[0]);
    }

    return false; // Vote already in progress
}

bool EnhancedQuestManager::ResolveByLeader(const QuestConflict& conflict) {
    auto questIt = m_quests.find(conflict.questHash);
    if (questIt == m_quests.end()) {
        return false;
    }

    auto* quest = questIt->second.get();

    if (quest->questLeader == 0) {
        // No leader assigned, assign first affected player as leader
        quest->questLeader = conflict.affectedPlayers[0];
    }

    // Use leader's stage as authority
    auto leaderProgressIt = quest->playerProgress.find(quest->questLeader);
    if (leaderProgressIt != quest->playerProgress.end()) {
        quest->authorityStage = leaderProgressIt->second.currentStage;

        // Update all other players to leader's stage
        for (uint32_t playerId : conflict.affectedPlayers) {
            if (playerId != quest->questLeader) {
                auto& progress = quest->playerProgress[playerId];
                progress.currentStage = quest->authorityStage;
                progress.lastUpdate = GetCurrentTimestamp();
            }
        }
        return true;
    }

    return false;
}

bool EnhancedQuestManager::ResolveAutomatically(const QuestConflict& conflict) {
    auto questIt = m_quests.find(conflict.questHash);
    if (questIt == m_quests.end()) {
        return false;
    }

    auto* quest = questIt->second.get();

    // For CP2077 story quests, prefer advancement to maintain story flow
    if (quest->type == QuestType::Main) {
        return ResolveByAdvance(conflict);
    }

    // For custom quests, use majority rule
    std::unordered_map<uint16_t, uint32_t> stageCounts;
    for (uint16_t stage : conflict.conflictingStages) {
        stageCounts[stage] = 0;
    }

    for (uint32_t playerId : conflict.affectedPlayers) {
        auto progressIt = quest->playerProgress.find(playerId);
        if (progressIt != quest->playerProgress.end()) {
            stageCounts[progressIt->second.currentStage]++;
        }
    }

    // Find stage with most votes
    uint16_t majorityStage = 0;
    uint32_t maxVotes = 0;
    for (const auto& [stage, count] : stageCounts) {
        if (count > maxVotes) {
            maxVotes = count;
            majorityStage = stage;
        }
    }

    // Apply majority stage
    quest->authorityStage = majorityStage;
    for (uint32_t playerId : conflict.affectedPlayers) {
        auto& progress = quest->playerProgress[playerId];
        progress.currentStage = majorityStage;
        progress.lastUpdate = GetCurrentTimestamp();
    }

    return true;
}

uint64_t EnhancedQuestManager::GetCurrentTimestamp() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

// Missing method implementations

bool EnhancedQuestManager::UnregisterQuest(uint32_t questHash) {
    std::lock_guard<std::mutex> lock(m_questsMutex);

    auto it = m_quests.find(questHash);
    if (it == m_quests.end()) {
        return false;
    }

    std::string questName = it->second->questName;
    m_quests.erase(it);

    // Clear any conflicts related to this quest
    {
        std::lock_guard<std::mutex> conflictLock(m_conflictsMutex);
        auto conflictIt = m_conflicts.begin();
        while (conflictIt != m_conflicts.end()) {
            if (conflictIt->second->questHash == questHash) {
                conflictIt = m_conflicts.erase(conflictIt);
            } else {
                ++conflictIt;
            }
        }
    }

    Logger::Log(LogLevel::INFO, "[EnhancedQuestManager] Unregistered quest: " + questName);
    return true;
}

QuestSyncData* EnhancedQuestManager::GetQuest(uint32_t questHash) {
    std::lock_guard<std::mutex> lock(m_questsMutex);

    auto it = m_quests.find(questHash);
    return it != m_quests.end() ? it->second.get() : nullptr;
}

std::vector<QuestSyncData*> EnhancedQuestManager::GetActiveQuests() const {
    std::lock_guard<std::mutex> lock(m_questsMutex);

    std::vector<QuestSyncData*> activeQuests;
    for (const auto& [questHash, quest] : m_quests) {
        if (quest->authorityState == QuestState::Active) {
            activeQuests.push_back(quest.get());
        }
    }
    return activeQuests;
}

std::vector<QuestSyncData*> EnhancedQuestManager::GetQuestsByType(QuestType type) const {
    std::lock_guard<std::mutex> lock(m_questsMutex);

    std::vector<QuestSyncData*> quests;
    for (const auto& [questHash, quest] : m_quests) {
        if (quest->type == type) {
            quests.push_back(quest.get());
        }
    }
    return quests;
}

bool EnhancedQuestManager::RegisterPlayer(uint32_t playerId, const std::string& playerName) {
    std::lock_guard<std::mutex> lock(m_playersMutex);

    if (m_players.find(playerId) != m_players.end()) {
        Logger::Log(LogLevel::WARNING, "[EnhancedQuestManager] Player already registered: " + playerName);
        return false;
    }

    m_players[playerId] = playerName;
    Logger::Log(LogLevel::INFO, "[EnhancedQuestManager] Registered player: " + playerName + " (ID: " + std::to_string(playerId) + ")");
    return true;
}

bool EnhancedQuestManager::UnregisterPlayer(uint32_t playerId) {
    std::lock_guard<std::mutex> playerLock(m_playersMutex);
    std::lock_guard<std::mutex> questLock(m_questsMutex);

    auto it = m_players.find(playerId);
    if (it == m_players.end()) {
        return false;
    }

    std::string playerName = it->second;
    m_players.erase(it);

    // Remove player from all quests
    for (auto& [questHash, quest] : m_quests) {
        quest->playerProgress.erase(playerId);
        quest->playerVotes.erase(playerId);

        // Transfer leadership if this player was the leader
        if (quest->questLeader == playerId) {
            quest->questLeader = 0;
            // Assign new leader if there are other players
            if (!quest->playerProgress.empty()) {
                quest->questLeader = quest->playerProgress.begin()->first;
            }
        }
    }

    Logger::Log(LogLevel::INFO, "[EnhancedQuestManager] Unregistered player: " + playerName);
    return true;
}

std::vector<uint32_t> EnhancedQuestManager::GetActivePlayers() const {
    std::lock_guard<std::mutex> lock(m_playersMutex);

    std::vector<uint32_t> players;
    players.reserve(m_players.size());
    for (const auto& [playerId, playerName] : m_players) {
        players.push_back(playerId);
    }
    return players;
}

bool EnhancedQuestManager::UpdateQuestState(uint32_t playerId, uint32_t questHash, QuestState newState) {
    std::lock_guard<std::mutex> lock(m_questsMutex);

    auto questIt = m_quests.find(questHash);
    if (questIt == m_quests.end()) {
        return false;
    }

    auto* quest = questIt->second.get();
    auto& playerProgress = quest->playerProgress[playerId];

    playerProgress.playerId = playerId;
    playerProgress.questHash = questHash;
    playerProgress.state = newState;
    playerProgress.lastUpdate = GetCurrentTimestamp();

    Logger::Log(LogLevel::DEBUG, "[EnhancedQuestManager] Player " + std::to_string(playerId) +
               " quest " + quest->questName + " state changed to " + std::to_string(static_cast<uint8_t>(newState)));

    return true;
}

bool EnhancedQuestManager::CompleteObjective(uint32_t playerId, uint32_t questHash, uint32_t objectiveId) {
    std::lock_guard<std::mutex> lock(m_questsMutex);

    auto questIt = m_quests.find(questHash);
    if (questIt == m_quests.end()) {
        return false;
    }

    auto* quest = questIt->second.get();
    auto& playerProgress = quest->playerProgress[playerId];

    // Check if objective exists
    bool objectiveExists = false;
    for (const auto& objective : quest->objectives) {
        if (objective.objectiveId == objectiveId) {
            objectiveExists = true;
            break;
        }
    }

    if (!objectiveExists) {
        return false;
    }

    // Add to completed objectives if not already completed
    auto& completed = playerProgress.completedObjectives;
    if (std::find(completed.begin(), completed.end(), objectiveId) == completed.end()) {
        completed.push_back(objectiveId);
        playerProgress.lastUpdate = GetCurrentTimestamp();

        Logger::Log(LogLevel::INFO, "[EnhancedQuestManager] Player " + std::to_string(playerId) +
                   " completed objective " + std::to_string(objectiveId) + " in quest " + quest->questName);

        TriggerEvent("objective_completed", questHash, playerId, "objective:" + std::to_string(objectiveId));
        return true;
    }

    return false;
}

bool EnhancedQuestManager::SetQuestVariable(uint32_t playerId, uint32_t questHash, const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(m_questsMutex);

    auto questIt = m_quests.find(questHash);
    if (questIt == m_quests.end()) {
        return false;
    }

    auto* quest = questIt->second.get();
    auto& playerProgress = quest->playerProgress[playerId];

    playerProgress.playerId = playerId;
    playerProgress.questHash = questHash;
    playerProgress.questVariables[key] = value;
    playerProgress.lastUpdate = GetCurrentTimestamp();

    Logger::Log(LogLevel::DEBUG, "[EnhancedQuestManager] Set quest variable " + key + " = " + value +
               " for player " + std::to_string(playerId) + " in quest " + quest->questName);

    return true;
}

bool EnhancedQuestManager::SetQuestLeader(uint32_t questHash, uint32_t playerId) {
    std::lock_guard<std::mutex> lock(m_questsMutex);

    auto questIt = m_quests.find(questHash);
    if (questIt == m_quests.end()) {
        return false;
    }

    auto* quest = questIt->second.get();

    // Verify player exists in quest
    if (quest->playerProgress.find(playerId) == quest->playerProgress.end()) {
        return false;
    }

    uint32_t oldLeader = quest->questLeader;
    quest->questLeader = playerId;

    // Update player progress to reflect leadership
    for (auto& [pid, progress] : quest->playerProgress) {
        progress.isQuestLeader = (pid == playerId);
    }

    Logger::Log(LogLevel::INFO, "[EnhancedQuestManager] Quest " + quest->questName +
               " leadership transferred from " + std::to_string(oldLeader) + " to " + std::to_string(playerId));

    TriggerEvent("quest_leader_changed", questHash, playerId, "old_leader:" + std::to_string(oldLeader));
    return true;
}

uint32_t EnhancedQuestManager::GetQuestLeader(uint32_t questHash) const {
    std::lock_guard<std::mutex> lock(m_questsMutex);

    auto it = m_quests.find(questHash);
    return it != m_quests.end() ? it->second->questLeader : 0;
}

bool EnhancedQuestManager::TransferQuestLeadership(uint32_t questHash, uint32_t newLeader) {
    return SetQuestLeader(questHash, newLeader);
}

bool EnhancedQuestManager::AddBranchChoice(uint32_t questHash, uint16_t stage, uint32_t playerId, uint32_t choice) {
    std::lock_guard<std::mutex> lock(m_questsMutex);

    auto questIt = m_quests.find(questHash);
    if (questIt == m_quests.end()) {
        return false;
    }

    auto* quest = questIt->second.get();
    auto& playerProgress = quest->playerProgress[playerId];

    playerProgress.branchChoices[stage] = choice;
    playerProgress.lastUpdate = GetCurrentTimestamp();

    Logger::Log(LogLevel::DEBUG, "[EnhancedQuestManager] Player " + std::to_string(playerId) +
               " made choice " + std::to_string(choice) + " at stage " + std::to_string(stage) +
               " in quest " + quest->questName);

    TriggerEvent("branch_choice_made", questHash, playerId,
                "stage:" + std::to_string(stage) + ",choice:" + std::to_string(choice));

    return true;
}

bool EnhancedQuestManager::RequiresConsensus(uint32_t questHash, uint16_t stage) const {
    std::lock_guard<std::mutex> lock(m_questsMutex);

    auto it = m_quests.find(questHash);
    if (it == m_quests.end()) {
        return false;
    }

    const auto* quest = it->second.get();

    // Check if this stage requires consensus based on quest type and sync mode
    if (quest->syncMode == QuestSyncMode::Consensus) {
        return true;
    }

    // CP2077 critical story quests require consensus
    if (quest->type == QuestType::Main &&
        CP2077Quests::CRITICAL_SYNC_QUESTS.count(questHash)) {
        return true;
    }

    return false;
}

std::unordered_map<uint32_t, uint32_t> EnhancedQuestManager::GetBranchChoices(uint32_t questHash, uint16_t stage) const {
    std::lock_guard<std::mutex> lock(m_questsMutex);

    std::unordered_map<uint32_t, uint32_t> choices;

    auto it = m_quests.find(questHash);
    if (it == m_quests.end()) {
        return choices;
    }

    const auto* quest = it->second.get();
    for (const auto& [playerId, progress] : quest->playerProgress) {
        auto choiceIt = progress.branchChoices.find(stage);
        if (choiceIt != progress.branchChoices.end()) {
            choices[playerId] = choiceIt->second;
        }
    }

    return choices;
}

bool EnhancedQuestManager::ResolveConflict(uint32_t conflictId, ConflictResolution method) {
    std::lock_guard<std::mutex> conflictLock(m_conflictsMutex);
    std::lock_guard<std::mutex> questLock(m_questsMutex);

    auto conflictIt = m_conflicts.find(conflictId);
    if (conflictIt == m_conflicts.end() || conflictIt->second->isResolved) {
        return false;
    }

    auto* conflict = conflictIt->second.get();
    conflict->resolutionMethod = method;
    conflict->resolutionAttempts++;

    bool resolved = false;
    switch (method) {
        case ConflictResolution::RollbackAll:
            resolved = ResolveByRollback(*conflict);
            break;
        case ConflictResolution::AdvanceAll:
            resolved = ResolveByAdvance(*conflict);
            break;
        case ConflictResolution::Vote:
            resolved = ResolveByVote(*conflict);
            break;
        case ConflictResolution::LeaderDecides:
            resolved = ResolveByLeader(*conflict);
            break;
        case ConflictResolution::AutoResolve:
            resolved = ResolveAutomatically(*conflict);
            break;
    }

    if (resolved) {
        conflict->isResolved = true;

        auto questIt = m_quests.find(conflict->questHash);
        if (questIt != m_quests.end()) {
            questIt->second->hasPendingConflict = false;
            SynchronizeQuest(conflict->questHash);
        }

        Logger::Log(LogLevel::INFO, "[EnhancedQuestManager] Resolved conflict " + std::to_string(conflictId) +
                   " using method " + std::to_string(static_cast<uint8_t>(method)));
    }

    return resolved;
}

QuestValidationResult EnhancedQuestManager::ValidateAllQuests() const {
    std::lock_guard<std::mutex> lock(m_questsMutex);

    QuestValidationResult combinedResult;
    combinedResult.isValid = true;

    for (const auto& [questHash, quest] : m_quests) {
        auto result = ValidateQuestState(questHash);
        if (!result.isValid) {
            combinedResult.isValid = false;

            // Add quest-specific errors
            for (const auto& error : result.errors) {
                combinedResult.errors.push_back("Quest " + quest->questName + ": " + error);
            }

            for (const auto& warning : result.warnings) {
                combinedResult.warnings.push_back("Quest " + quest->questName + ": " + warning);
            }

            // Merge player issues
            for (const auto& [playerId, issues] : result.playerIssues) {
                auto& combinedIssues = combinedResult.playerIssues[playerId];
                for (const auto& issue : issues) {
                    combinedIssues.push_back("Quest " + quest->questName + ": " + issue);
                }
            }
        }
    }

    return combinedResult;
}

void EnhancedQuestManager::SynchronizeAllQuests() {
    std::lock_guard<std::mutex> lock(m_questsMutex);

    for (const auto& [questHash, quest] : m_quests) {
        SynchronizeQuest(questHash);
    }

    Logger::Log(LogLevel::DEBUG, "[EnhancedQuestManager] Synchronized all " + std::to_string(m_quests.size()) + " quests");
}

void EnhancedQuestManager::ForceResyncPlayer(uint32_t playerId) {
    std::lock_guard<std::mutex> lock(m_questsMutex);

    uint32_t resyncCount = 0;

    for (const auto& [questHash, quest] : m_quests) {
        auto progressIt = quest->playerProgress.find(playerId);
        if (progressIt != quest->playerProgress.end()) {
            // Reset player to authority stage for strict sync quests
            if (quest->syncMode == QuestSyncMode::Strict) {
                progressIt->second.currentStage = quest->authorityStage;
                progressIt->second.lastUpdate = GetCurrentTimestamp();
                resyncCount++;
            }
        }
    }

    Logger::Log(LogLevel::INFO, "[EnhancedQuestManager] Force resynced " + std::to_string(resyncCount) +
               " quests for player " + std::to_string(playerId));

    TriggerEvent("player_resynced", 0, playerId, "quest_count:" + std::to_string(resyncCount));
}

bool EnhancedQuestManager::ProcessSyncQueue() {
    std::lock_guard<std::mutex> queueLock(m_queueMutex);

    if (m_syncQueue.empty()) {
        return false;
    }

    uint32_t processedCount = 0;
    while (!m_syncQueue.empty() && processedCount < 10) { // Process up to 10 per call
        uint32_t questHash = m_syncQueue.front();
        m_syncQueue.pop();

        SynchronizeQuest(questHash);
        processedCount++;
    }

    return processedCount > 0;
}

bool EnhancedQuestManager::AddQuestDependency(uint32_t questHash, uint32_t prerequisiteQuest) {
    std::lock_guard<std::mutex> lock(m_questsMutex);

    auto questIt = m_quests.find(questHash);
    if (questIt == m_quests.end()) {
        return false;
    }

    auto* quest = questIt->second.get();
    auto& prereqs = quest->prerequisiteQuests;

    if (std::find(prereqs.begin(), prereqs.end(), prerequisiteQuest) == prereqs.end()) {
        prereqs.push_back(prerequisiteQuest);

        Logger::Log(LogLevel::DEBUG, "[EnhancedQuestManager] Added dependency " + std::to_string(prerequisiteQuest) +
                   " to quest " + quest->questName);
        return true;
    }

    return false;
}

bool EnhancedQuestManager::RemoveQuestDependency(uint32_t questHash, uint32_t prerequisiteQuest) {
    std::lock_guard<std::mutex> lock(m_questsMutex);

    auto questIt = m_quests.find(questHash);
    if (questIt == m_quests.end()) {
        return false;
    }

    auto* quest = questIt->second.get();
    auto& prereqs = quest->prerequisiteQuests;

    auto it = std::find(prereqs.begin(), prereqs.end(), prerequisiteQuest);
    if (it != prereqs.end()) {
        prereqs.erase(it);

        Logger::Log(LogLevel::DEBUG, "[EnhancedQuestManager] Removed dependency " + std::to_string(prerequisiteQuest) +
                   " from quest " + quest->questName);
        return true;
    }

    return false;
}

std::vector<uint32_t> EnhancedQuestManager::GetQuestDependencies(uint32_t questHash) const {
    std::lock_guard<std::mutex> lock(m_questsMutex);

    auto it = m_quests.find(questHash);
    return it != m_quests.end() ? it->second->prerequisiteQuests : std::vector<uint32_t>{};
}

bool EnhancedQuestManager::CanStartQuest(uint32_t questHash, uint32_t playerId) const {
    std::lock_guard<std::mutex> lock(m_questsMutex);

    auto questIt = m_quests.find(questHash);
    if (questIt == m_quests.end()) {
        return false;
    }

    const auto* quest = questIt->second.get();

    // Check all prerequisites
    for (uint32_t prereqHash : quest->prerequisiteQuests) {
        auto prereqIt = m_quests.find(prereqHash);
        if (prereqIt == m_quests.end()) {
            continue; // Skip missing prerequisite quests
        }

        const auto* prereq = prereqIt->second.get();
        auto progressIt = prereq->playerProgress.find(playerId);

        if (progressIt == prereq->playerProgress.end() ||
            progressIt->second.state != QuestState::Completed) {
            return false; // Prerequisite not completed
        }
    }

    return true;
}

void EnhancedQuestManager::ResetStats() {
    std::lock_guard<std::mutex> lock(m_questsMutex);

    m_stats = {};
    m_lastStatsUpdate = std::chrono::steady_clock::now();

    Logger::Log(LogLevel::INFO, "[EnhancedQuestManager] Statistics reset");
}

bool EnhancedQuestManager::CreateQuestCheckpoint(uint32_t questHash) {
    std::lock_guard<std::mutex> questLock(m_questsMutex);
    std::lock_guard<std::mutex> checkpointLock(m_checkpointMutex);

    auto questIt = m_quests.find(questHash);
    if (questIt == m_quests.end()) {
        return false;
    }

    // Create a copy of the current quest state
    QuestSyncData checkpoint = *questIt->second;

    // Add to checkpoint history
    auto& checkpoints = m_questCheckpoints[questHash];
    checkpoints.push_back(checkpoint);

    // Limit checkpoint history
    if (checkpoints.size() > m_config.maxQuestHistory) {
        checkpoints.erase(checkpoints.begin());
    }

    Logger::Log(LogLevel::DEBUG, "[EnhancedQuestManager] Created checkpoint for quest " +
               questIt->second->questName + " (total: " + std::to_string(checkpoints.size()) + ")");

    return true;
}

bool EnhancedQuestManager::RestoreQuestCheckpoint(uint32_t questHash) {
    std::lock_guard<std::mutex> questLock(m_questsMutex);
    std::lock_guard<std::mutex> checkpointLock(m_checkpointMutex);

    auto questIt = m_quests.find(questHash);
    if (questIt == m_quests.end()) {
        return false;
    }

    auto checkpointIt = m_questCheckpoints.find(questHash);
    if (checkpointIt == m_questCheckpoints.end() || checkpointIt->second.empty()) {
        return false;
    }

    // Restore from the latest checkpoint
    const auto& latestCheckpoint = checkpointIt->second.back();
    *questIt->second = latestCheckpoint;

    // Remove the used checkpoint
    checkpointIt->second.pop_back();

    Logger::Log(LogLevel::INFO, "[EnhancedQuestManager] Restored quest " + questIt->second->questName +
               " from checkpoint");

    SynchronizeQuest(questHash);
    TriggerEvent("quest_checkpoint_restored", questHash, 0, "");

    return true;
}

void EnhancedQuestManager::RegisterEventCallback(const std::string& eventType, QuestEventCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);

    m_eventCallbacks[eventType].push_back(callback);

    Logger::Log(LogLevel::DEBUG, "[EnhancedQuestManager] Registered event callback for: " + eventType);
}

void EnhancedQuestManager::UnregisterEventCallback(const std::string& eventType) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);

    m_eventCallbacks.erase(eventType);

    Logger::Log(LogLevel::DEBUG, "[EnhancedQuestManager] Unregistered event callbacks for: " + eventType);
}

// Helper method implementations

bool EnhancedQuestManager::IsQuestActive(uint32_t questHash) const {
    auto it = m_quests.find(questHash);
    return it != m_quests.end() && it->second->authorityState == QuestState::Active;
}

bool EnhancedQuestManager::IsPlayerInQuest(uint32_t questHash, uint32_t playerId) const {
    auto it = m_quests.find(questHash);
    return it != m_quests.end() && it->second->playerProgress.count(playerId) > 0;
}

uint16_t EnhancedQuestManager::GetConsensusStage(uint32_t questHash) const {
    auto it = m_quests.find(questHash);
    if (it == m_quests.end()) {
        return 0;
    }

    const auto* quest = it->second.get();
    if (quest->playerProgress.empty()) {
        return quest->authorityStage;
    }

    // Find the most common stage
    std::unordered_map<uint16_t, uint32_t> stageCounts;
    for (const auto& [playerId, progress] : quest->playerProgress) {
        stageCounts[progress.currentStage]++;
    }

    uint16_t consensusStage = quest->authorityStage;
    uint32_t maxCount = 0;

    for (const auto& [stage, count] : stageCounts) {
        if (count > maxCount) {
            maxCount = count;
            consensusStage = stage;
        }
    }

    return consensusStage;
}

bool EnhancedQuestManager::ArePlayersInSync(uint32_t questHash) const {
    auto it = m_quests.find(questHash);
    if (it == m_quests.end()) {
        return true;
    }

    const auto* quest = it->second.get();
    if (quest->playerProgress.size() <= 1) {
        return true;
    }

    uint16_t firstStage = UINT16_MAX;
    for (const auto& [playerId, progress] : quest->playerProgress) {
        if (firstStage == UINT16_MAX) {
            firstStage = progress.currentStage;
        } else if (progress.currentStage != firstStage) {
            return false;
        }
    }

    return true;
}

void EnhancedQuestManager::TriggerEvent(const std::string& eventType, uint32_t questHash,
                                      uint32_t playerId, const std::string& data) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);

    auto it = m_eventCallbacks.find(eventType);
    if (it != m_eventCallbacks.end()) {
        for (const auto& callback : it->second) {
            try {
                callback(questHash, playerId, data);
            } catch (const std::exception& e) {
                Logger::Log(LogLevel::ERROR, "[EnhancedQuestManager] Event callback error: " + std::string(e.what()));
            }
        }
    }
}

std::vector<QuestConflict> EnhancedQuestManager::DetectConflicts() {
    std::vector<QuestConflict> conflicts;
    uint32_t conflictId = 1;

    for (const auto& [questHash, quest] : m_quests) {
        if (quest->playerProgress.size() < 2) {
            continue; // No conflict possible with less than 2 players
        }

        // Check for stage conflicts
        std::unordered_map<uint16_t, std::vector<uint32_t>> stageGroups;
        for (const auto& [playerId, progress] : quest->playerProgress) {
            stageGroups[progress.currentStage].push_back(playerId);
        }

        if (stageGroups.size() > 1) {
            // Multiple different stages detected - conflict!
            QuestConflict conflict;
            conflict.conflictId = conflictId++;
            conflict.questHash = questHash;
            conflict.resolutionMethod = quest->conflictMode;
            conflict.detectedTime = GetCurrentTimestamp();
            conflict.resolutionAttempts = 0;
            conflict.isResolved = false;
            conflict.conflictReason = "Stage desynchronization";

            for (const auto& [stage, players] : stageGroups) {
                conflict.conflictingStages.push_back(stage);
                for (uint32_t playerId : players) {
                    conflict.affectedPlayers.push_back(playerId);
                }
            }

            conflicts.push_back(conflict);

            // Store conflict for future reference
            std::lock_guard<std::mutex> lock(m_conflictsMutex);
            m_conflicts[conflict.conflictId] = std::make_unique<QuestConflict>(conflict);
            quest->hasPendingConflict = true;

            Logger::Log(LogLevel::WARNING, "[EnhancedQuestManager] Conflict detected in quest " + quest->questName +
                       " (" + std::to_string(stageGroups.size()) + " different stages)");
        }
    }

    return conflicts;
}

QuestValidationResult EnhancedQuestManager::ValidateQuestState(uint32_t questHash) const {
    QuestValidationResult result;
    result.isValid = true;

    auto questIt = m_quests.find(questHash);
    if (questIt == m_quests.end()) {
        result.isValid = false;
        result.errors.push_back("Quest not found");
        return result;
    }

    const auto* quest = questIt->second.get();

    // Validate quest data integrity
    if (quest->questName.empty()) {
        result.errors.push_back("Quest name is empty");
        result.isValid = false;
    }

    if (quest->authorityStage > 1000) { // Reasonable upper bound
        result.warnings.push_back("Authority stage seems unusually high: " + std::to_string(quest->authorityStage));
    }

    // Validate player progress consistency
    for (const auto& [playerId, progress] : quest->playerProgress) {
        std::vector<std::string> playerErrors;

        if (progress.questHash != questHash) {
            playerErrors.push_back("Progress quest hash mismatch");
            result.isValid = false;
        }

        if (progress.currentStage > 1000) {
            playerErrors.push_back("Player stage unusually high: " + std::to_string(progress.currentStage));
        }

        if (quest->syncMode == QuestSyncMode::Strict &&
            progress.currentStage != quest->authorityStage) {
            playerErrors.push_back("Player stage desynchronized in strict mode");
        }

        // Check objective completion consistency
        for (uint32_t objectiveId : progress.completedObjectives) {
            bool found = false;
            for (const auto& objective : quest->objectives) {
                if (objective.objectiveId == objectiveId) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                playerErrors.push_back("Completed objective not found in quest: " + std::to_string(objectiveId));
            }
        }

        if (!playerErrors.empty()) {
            result.playerIssues[playerId] = playerErrors;
            result.isValid = false;
        }
    }

    // Validate quest dependencies
    for (uint32_t prereqHash : quest->prerequisiteQuests) {
        auto prereqIt = m_quests.find(prereqHash);
        if (prereqIt == m_quests.end()) {
            result.warnings.push_back("Prerequisite quest not found: " + std::to_string(prereqHash));
        }
    }

    // Validate vote state
    if (quest->hasActiveVote) {
        if (quest->voteDeadline <= GetCurrentTimestamp()) {
            result.warnings.push_back("Vote has expired but is still marked as active");
        }

        if (quest->playerVotes.empty()) {
            result.warnings.push_back("Active vote has no votes cast");
        }
    }

    return result;
}

bool EnhancedQuestManager::RepairQuestState(uint32_t questHash) {
    std::lock_guard<std::mutex> lock(m_questsMutex);

    auto questIt = m_quests.find(questHash);
    if (questIt == m_quests.end()) {
        return false;
    }

    auto* quest = questIt->second.get();
    bool repaired = false;

    // Repair stage consistency for strict mode quests
    if (quest->syncMode == QuestSyncMode::Strict) {
        for (auto& [playerId, progress] : quest->playerProgress) {
            if (progress.currentStage != quest->authorityStage) {
                Logger::Log(LogLevel::INFO, "[EnhancedQuestManager] Repairing player " + std::to_string(playerId) +
                           " stage from " + std::to_string(progress.currentStage) + " to " + std::to_string(quest->authorityStage));
                progress.currentStage = quest->authorityStage;
                progress.lastUpdate = GetCurrentTimestamp();
                repaired = true;
            }
        }
    }

    // Clear expired votes
    if (quest->hasActiveVote && quest->voteDeadline <= GetCurrentTimestamp()) {
        Logger::Log(LogLevel::INFO, "[EnhancedQuestManager] Clearing expired vote for quest " + quest->questName);
        quest->hasActiveVote = false;
        quest->voteTargetStage = 0;
        quest->voteDeadline = 0;
        quest->playerVotes.clear();
        repaired = true;
    }

    // Remove invalid completed objectives
    for (auto& [playerId, progress] : quest->playerProgress) {
        auto it = progress.completedObjectives.begin();
        while (it != progress.completedObjectives.end()) {
            bool found = false;
            for (const auto& objective : quest->objectives) {
                if (objective.objectiveId == *it) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                Logger::Log(LogLevel::INFO, "[EnhancedQuestManager] Removing invalid objective " + std::to_string(*it) +
                           " from player " + std::to_string(playerId));
                it = progress.completedObjectives.erase(it);
                repaired = true;
            } else {
                ++it;
            }
        }
    }

    if (repaired) {
        SynchronizeQuest(questHash);
    }

    return repaired;
}

bool EnhancedQuestManager::SaveQuestSnapshot(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(m_questsMutex);

    std::ofstream file(filename);
    if (!file.is_open()) {
        Logger::Log(LogLevel::ERROR, "[EnhancedQuestManager] Failed to open file for quest snapshot: " + filename);
        return false;
    }

    // Simple JSON-like format for quest data
    file << "{\n";
    file << "  \"timestamp\": " << GetCurrentTimestamp() << ",\n";
    file << "  \"quest_count\": " << m_quests.size() << ",\n";
    file << "  \"quests\": [\n";

    bool first = true;
    for (const auto& [questHash, quest] : m_quests) {
        if (!first) file << ",\n";
        first = false;

        file << "    {\n";
        file << "      \"hash\": " << questHash << ",\n";
        file << "      \"name\": \"" << quest->questName << "\",\n";
        file << "      \"type\": " << static_cast<int>(quest->type) << ",\n";
        file << "      \"priority\": " << static_cast<int>(quest->priority) << ",\n";
        file << "      \"sync_mode\": " << static_cast<int>(quest->syncMode) << ",\n";
        file << "      \"authority_stage\": " << quest->authorityStage << ",\n";
        file << "      \"authority_state\": " << static_cast<int>(quest->authorityState) << ",\n";
        file << "      \"player_count\": " << quest->playerProgress.size() << "\n";
        file << "    }";
    }

    file << "\n  ]\n";
    file << "}\n";

    file.close();
    Logger::Log(LogLevel::INFO, "[EnhancedQuestManager] Quest snapshot saved to " + filename);
    return true;
}

bool EnhancedQuestManager::LoadQuestSnapshot(const std::string& filename) {
    // TODO: Implement JSON parsing for quest snapshot loading
    // For now, just log that this feature is not yet implemented
    Logger::Log(LogLevel::WARNING, "[EnhancedQuestManager] LoadQuestSnapshot not yet implemented: " + filename);
    return false;
}

EnhancedQuestManager::QuestSystemStats EnhancedQuestManager::GetSystemStats() const {
    std::lock_guard<std::mutex> lock(m_questsMutex);

    QuestSystemStats stats = m_stats;
    stats.totalQuests = m_quests.size();
    stats.activeQuests = 0;
    stats.completedQuests = 0;
    stats.failedQuests = 0;
    stats.pendingConflicts = 0;

    for (const auto& [questHash, quest] : m_quests) {
        switch (quest->authorityState) {
            case QuestState::Active:
                stats.activeQuests++;
                break;
            case QuestState::Completed:
                stats.completedQuests++;
                break;
            case QuestState::Failed:
                stats.failedQuests++;
                break;
            default:
                break;
        }

        if (quest->hasPendingConflict) {
            stats.pendingConflicts++;
        }
    }

    // Calculate sync operations per second
    auto now = std::chrono::steady_clock::now();
    auto timeDiff = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastStatsUpdate);
    if (timeDiff.count() > 0) {
        stats.syncOperationsPerSecond = m_stats.syncOperationsPerSecond / timeDiff.count();
    }

    return stats;
}

} // namespace CoopNet