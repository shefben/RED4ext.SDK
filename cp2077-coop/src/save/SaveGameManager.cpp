#include "SaveGameManager.hpp"
#include "../core/Logger.hpp"
#include "../core/CoopExports.hpp"
#include "../net/Net.hpp"
#include <thread>
#include <fstream>
#include <sstream>

namespace CoopNet {

SaveGameManager& SaveGameManager::Instance() {
    static SaveGameManager instance;
    return instance;
}

SaveGameManager::SaveGameManager() : m_initialized(false), m_saveInProgress(false) {
}

SaveGameManager::~SaveGameManager() {
    Cleanup();
}

bool SaveGameManager::Initialize() {
    if (m_initialized) return true;

    Logger::Log(LogLevel::INFO, "Initializing Save Game Manager");

    // Initialize save directory structure
    CreateSaveDirectories();

    m_initialized = true;
    return true;
}

void SaveGameManager::Cleanup() {
    if (!m_initialized) return;

    Logger::Log(LogLevel::INFO, "Cleaning up Save Game Manager");

    // Cancel any ongoing save operations
    {
        std::lock_guard<std::mutex> lock(m_saveMutex);
        m_saveInProgress = false;
        m_pendingRequests.clear();
        m_playerSaveStates.clear();
    }

    m_initialized = false;
}

bool SaveGameManager::InitiateCoordinatedSave(uint32_t saveSlot, uint32_t initiatorPeerId) {
    std::lock_guard<std::mutex> lock(m_saveMutex);

    if (m_saveInProgress) {
        Logger::Log(LogLevel::WARNING, "Save already in progress, rejecting new save request");
        return false;
    }

    if (saveSlot >= MAX_SAVE_SLOTS) {
        Logger::Log(LogLevel::ERROR, "Invalid save slot: " + std::to_string(saveSlot));
        return false;
    }

    // Generate new save request ID
    m_currentSaveRequest.requestId = GenerateRequestId();
    m_currentSaveRequest.saveSlot = saveSlot;
    m_currentSaveRequest.initiatorPeerId = initiatorPeerId;
    m_currentSaveRequest.timestamp = GetCurrentTimestamp();
    m_currentSaveRequest.playersReady = 0;
    m_currentSaveRequest.expectedPlayers = GetConnectedPlayerCount();

    m_saveInProgress = true;
    m_playerSaveStates.clear();

    Logger::Log(LogLevel::INFO, "Initiating coordinated save to slot " + std::to_string(saveSlot) +
                " by peer " + std::to_string(initiatorPeerId));

    // Send save request to all connected players
    SendSaveRequestToAll();

    // Start timeout timer
    StartSaveTimeout();

    return true;
}

bool SaveGameManager::OnSaveRequest(uint32_t requestId, uint32_t saveSlot, uint32_t initiatorPeerId) {
    std::lock_guard<std::mutex> lock(m_saveMutex);

    if (m_saveInProgress && m_currentSaveRequest.requestId != requestId) {
        Logger::Log(LogLevel::WARNING, "Conflicting save request received");
        SendSaveResponse(requestId, false, "Save already in progress");
        return false;
    }

    if (!CanPlayerSave()) {
        Logger::Log(LogLevel::WARNING, "Player cannot save at this time");
        SendSaveResponse(requestId, false, "Player not in saveable state");
        return false;
    }

    // Accept save request
    m_currentSaveRequest.requestId = requestId;
    m_currentSaveRequest.saveSlot = saveSlot;
    m_currentSaveRequest.initiatorPeerId = initiatorPeerId;
    m_currentSaveRequest.timestamp = GetCurrentTimestamp();
    m_saveInProgress = true;

    // Build player save state
    PlayerSaveData playerState = BuildPlayerSaveState();
    if (!ValidatePlayerSaveState(playerState)) {
        Logger::Log(LogLevel::ERROR, "Failed to build valid player save state");
        SendSaveResponse(requestId, false, "Failed to prepare save data");
        OnSaveCompleted(false, "Save data preparation failed");
        return false;
    }

    // Send confirmation and save data
    SendSaveResponse(requestId, true, "");
    SendPlayerSaveState(requestId, playerState);

    Logger::Log(LogLevel::INFO, "Prepared save state for coordinated save " + std::to_string(requestId));
    return true;
}

bool SaveGameManager::OnPlayerSaveStateReceived(uint32_t requestId, const PlayerSaveData& playerState) {
    std::lock_guard<std::mutex> lock(m_saveMutex);

    if (!m_saveInProgress || m_currentSaveRequest.requestId != requestId) {
        Logger::Log(LogLevel::WARNING, "Received save state for inactive save request");
        return false;
    }

    // Validate player save state
    if (!ValidatePlayerSaveState(playerState)) {
        Logger::Log(LogLevel::ERROR, "Invalid player save state received from peer " +
                   std::to_string(playerState.peerId));
        OnSaveCompleted(false, "Invalid player save data");
        return false;
    }

    // Store player state
    m_playerSaveStates[playerState.peerId] = playerState;
    m_currentSaveRequest.playersReady++;

    Logger::Log(LogLevel::INFO, "Received save state from peer " + std::to_string(playerState.peerId) +
               " (" + std::to_string(m_currentSaveRequest.playersReady) + "/" +
               std::to_string(m_currentSaveRequest.expectedPlayers) + " players ready)");

    // Check if all players are ready
    if (m_currentSaveRequest.playersReady >= m_currentSaveRequest.expectedPlayers) {
        ExecuteCoordinatedSave();
    }

    return true;
}

void SaveGameManager::ExecuteCoordinatedSave() {
    Logger::Log(LogLevel::INFO, "Executing coordinated save with " +
               std::to_string(m_currentSaveRequest.playersReady) + " players");

    // Build complete save data
    CompleteSaveData saveData = BuildCompleteSaveData();

    // Validate save data integrity
    if (!ValidateCompleteSaveData(saveData)) {
        Logger::Log(LogLevel::ERROR, "Save data validation failed");
        OnSaveCompleted(false, "Save data validation failed");
        return;
    }

    // Execute the actual save operation
    bool success = PerformSave(saveData);
    if (success) {
        Logger::Log(LogLevel::INFO, "Coordinated save completed successfully");
        OnSaveCompleted(true, "Save completed");
    } else {
        Logger::Log(LogLevel::ERROR, "Save operation failed");
        OnSaveCompleted(false, "Save operation failed");
    }
}

void SaveGameManager::OnSaveTimeout() {
    std::lock_guard<std::mutex> lock(m_saveMutex);

    if (m_saveInProgress) {
        Logger::Log(LogLevel::WARNING, "Save operation timed out");
        OnSaveCompleted(false, "Save operation timed out");
    }
}

void SaveGameManager::OnSaveCompleted(bool success, const std::string& message) {
    m_saveInProgress = false;
    m_playerSaveStates.clear();

    // Notify all players of save completion
    SendSaveCompletion(m_currentSaveRequest.requestId, success, message);

    if (success) {
        Logger::Log(LogLevel::INFO, "Save completed: " + message);
    } else {
        Logger::Log(LogLevel::ERROR, "Save failed: " + message);
    }

    // Reset save request
    m_currentSaveRequest = {};
}

bool SaveGameManager::LoadCoordinatedSave(uint32_t saveSlot) {
    std::lock_guard<std::mutex> lock(m_saveMutex);

    if (m_saveInProgress) {
        Logger::Log(LogLevel::WARNING, "Cannot load while save is in progress");
        return false;
    }

    if (saveSlot >= MAX_SAVE_SLOTS) {
        Logger::Log(LogLevel::ERROR, "Invalid save slot for load: " + std::to_string(saveSlot));
        return false;
    }

    // Load save data
    CompleteSaveData saveData;
    if (!LoadSaveData(saveSlot, saveData)) {
        Logger::Log(LogLevel::ERROR, "Failed to load save data from slot " + std::to_string(saveSlot));
        return false;
    }

    // Validate save data
    if (!ValidateCompleteSaveData(saveData)) {
        Logger::Log(LogLevel::ERROR, "Save data validation failed for slot " + std::to_string(saveSlot));
        return false;
    }

    // Apply save data
    return ApplySaveData(saveData);
}

// === Private Implementation ===

uint32_t SaveGameManager::GenerateRequestId() {
    return static_cast<uint32_t>(GetCurrentTimestamp() & 0xFFFFFFFF);
}

uint64_t SaveGameManager::GetCurrentTimestamp() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

uint32_t SaveGameManager::GetConnectedPlayerCount() {
    // Get from networking system
    return Net_GetConnectedPlayerCount();
}

bool SaveGameManager::CanPlayerSave() {
    // Check various game state conditions
    // TODO: Integrate with actual game state checking
    // For now, basic implementation
    return true;
}

PlayerSaveData SaveGameManager::BuildPlayerSaveState() {
    PlayerSaveData state;

    // Get local peer ID
    state.peerId = Net_GetLocalPeerId();
    state.timestamp = GetCurrentTimestamp();

    // TODO: Integrate with actual game systems to get:
    // - Player level, experience, street cred
    // - Player position and rotation
    // - Player money and inventory
    // - Player attributes and cyberware
    // - Quest progress

    // Placeholder implementation
    state.level = 1;
    state.experience = 0;
    state.streetCred = 1;
    state.money = 0;
    state.position[0] = state.position[1] = state.position[2] = 0.0f;
    state.rotation[0] = state.rotation[1] = state.rotation[2] = state.rotation[3] = 0.0f;

    Logger::Log(LogLevel::DEBUG, "Built player save state for peer " + std::to_string(state.peerId));
    return state;
}

CompleteSaveData SaveGameManager::BuildCompleteSaveData() {
    CompleteSaveData saveData;

    saveData.sessionId = GetCurrentSessionId();
    saveData.saveSlot = m_currentSaveRequest.saveSlot;
    saveData.timestamp = GetCurrentTimestamp();
    saveData.version = SAVE_VERSION;

    // Copy all player states
    for (const auto& pair : m_playerSaveStates) {
        saveData.playerStates.push_back(pair.second);
    }

    // Build world state
    saveData.worldState = BuildWorldState();

    // Calculate checksum
    saveData.checksum = CalculateSaveChecksum(saveData);

    return saveData;
}

WorldSaveData SaveGameManager::BuildWorldState() {
    WorldSaveData worldState;

    // TODO: Integrate with actual game systems to get:
    // - Game time and weather state
    // - Completed gigs and discovered locations
    // - Vehicle states and world events
    // - NCPD wanted level

    // Placeholder implementation
    worldState.gameTime = GetCurrentTimestamp();
    worldState.weatherState = 0;
    worldState.ncpdWanted = 0;

    Logger::Log(LogLevel::DEBUG, "Built world save state");
    return worldState;
}

bool SaveGameManager::ValidatePlayerSaveState(const PlayerSaveData& state) {
    if (state.peerId == 0) {
        Logger::Log(LogLevel::ERROR, "Invalid peer ID in save state");
        return false;
    }

    if (state.level > 50) { // Max level in CP2077
        Logger::Log(LogLevel::ERROR, "Invalid player level: " + std::to_string(state.level));
        return false;
    }

    if (state.money > 999999999ULL) { // Reasonable money limit
        Logger::Log(LogLevel::ERROR, "Invalid money amount: " + std::to_string(state.money));
        return false;
    }

    // Validate position is reasonable
    float posLength = sqrt(state.position[0] * state.position[0] +
                          state.position[1] * state.position[1] +
                          state.position[2] * state.position[2]);
    if (posLength > 10000.0f) {
        Logger::Log(LogLevel::ERROR, "Invalid player position in save state");
        return false;
    }

    return true;
}

bool SaveGameManager::ValidateCompleteSaveData(const CompleteSaveData& saveData) {
    if (saveData.playerStates.empty()) {
        Logger::Log(LogLevel::ERROR, "Save data contains no player states");
        return false;
    }

    // Validate checksum
    uint32_t calculatedChecksum = CalculateSaveChecksum(saveData);
    if (calculatedChecksum != saveData.checksum) {
        Logger::Log(LogLevel::ERROR, "Save data checksum mismatch");
        return false;
    }

    // Validate each player state
    for (const auto& playerState : saveData.playerStates) {
        if (!ValidatePlayerSaveState(playerState)) {
            return false;
        }
    }

    return true;
}

uint32_t SaveGameManager::CalculateSaveChecksum(const CompleteSaveData& saveData) {
    // Simple CRC32-like checksum implementation
    uint32_t crc = 0xFFFFFFFF;

    // Hash session ID
    crc ^= static_cast<uint32_t>(saveData.sessionId);
    crc ^= static_cast<uint32_t>(saveData.sessionId >> 32);

    // Hash save slot and timestamp
    crc ^= saveData.saveSlot;
    crc ^= static_cast<uint32_t>(saveData.timestamp);

    // Hash player states
    for (const auto& player : saveData.playerStates) {
        crc ^= player.peerId;
        crc ^= player.level;
        crc ^= static_cast<uint32_t>(player.money);
    }

    // Hash world state
    crc ^= static_cast<uint32_t>(saveData.worldState.gameTime);
    crc ^= saveData.worldState.weatherState;

    return crc;
}

bool SaveGameManager::PerformSave(const CompleteSaveData& saveData) {
    try {
        std::string savePath = GetSavePath(saveData.saveSlot);
        std::ofstream file(savePath, std::ios::binary);

        if (!file.is_open()) {
            Logger::Log(LogLevel::ERROR, "Failed to open save file: " + savePath);
            return false;
        }

        // Write save data to file
        file.write(reinterpret_cast<const char*>(&saveData), sizeof(CompleteSaveData));

        // Write player states
        uint32_t playerCount = static_cast<uint32_t>(saveData.playerStates.size());
        file.write(reinterpret_cast<const char*>(&playerCount), sizeof(playerCount));

        for (const auto& player : saveData.playerStates) {
            file.write(reinterpret_cast<const char*>(&player), sizeof(PlayerSaveData));
        }

        file.close();

        Logger::Log(LogLevel::INFO, "Save data written to " + savePath);
        return true;

    } catch (const std::exception& e) {
        Logger::Log(LogLevel::ERROR, "Exception during save: " + std::string(e.what()));
        return false;
    }
}

bool SaveGameManager::LoadSaveData(uint32_t saveSlot, CompleteSaveData& saveData) {
    try {
        std::string savePath = GetSavePath(saveSlot);
        std::ifstream file(savePath, std::ios::binary);

        if (!file.is_open()) {
            Logger::Log(LogLevel::ERROR, "Failed to open save file: " + savePath);
            return false;
        }

        // Read save data from file
        file.read(reinterpret_cast<char*>(&saveData), sizeof(CompleteSaveData));

        // Read player states
        uint32_t playerCount;
        file.read(reinterpret_cast<char*>(&playerCount), sizeof(playerCount));

        saveData.playerStates.reserve(playerCount);
        for (uint32_t i = 0; i < playerCount; i++) {
            PlayerSaveData player;
            file.read(reinterpret_cast<char*>(&player), sizeof(PlayerSaveData));
            saveData.playerStates.push_back(player);
        }

        file.close();

        Logger::Log(LogLevel::INFO, "Save data loaded from " + savePath);
        return true;

    } catch (const std::exception& e) {
        Logger::Log(LogLevel::ERROR, "Exception during load: " + std::string(e.what()));
        return false;
    }
}

bool SaveGameManager::ApplySaveData(const CompleteSaveData& saveData) {
    Logger::Log(LogLevel::INFO, "Applying save data with " +
               std::to_string(saveData.playerStates.size()) + " players");

    // Apply world state first
    if (!ApplyWorldState(saveData.worldState)) {
        Logger::Log(LogLevel::ERROR, "Failed to apply world state");
        return false;
    }

    // Apply player states
    for (const auto& playerState : saveData.playerStates) {
        if (!ApplyPlayerState(playerState)) {
            Logger::Log(LogLevel::ERROR, "Failed to apply state for peer " +
                       std::to_string(playerState.peerId));
            return false;
        }
    }

    Logger::Log(LogLevel::INFO, "Save data applied successfully");
    return true;
}

bool SaveGameManager::ApplyWorldState(const WorldSaveData& worldState) {
    // TODO: Integrate with actual game systems to apply:
    // - Game time and weather state
    // - World events and NCPD wanted level

    Logger::Log(LogLevel::DEBUG, "Applied world state");
    return true;
}

bool SaveGameManager::ApplyPlayerState(const PlayerSaveData& playerState) {
    // TODO: Integrate with actual game systems to apply:
    // - Player level, experience, attributes
    // - Player position and rotation
    // - Player money and inventory
    // - Quest progress

    Logger::Log(LogLevel::DEBUG, "Applied player state for peer " + std::to_string(playerState.peerId));
    return true;
}

std::string SaveGameManager::GetSavePath(uint32_t saveSlot) {
    return "saves/coop_save_" + std::to_string(saveSlot) + ".dat";
}

void SaveGameManager::CreateSaveDirectories() {
    // Create saves directory if it doesn't exist
    // TODO: Use proper filesystem operations
    Logger::Log(LogLevel::DEBUG, "Save directories initialized");
}

uint64_t SaveGameManager::GetCurrentSessionId() {
    // TODO: Get from session manager
    return 1; // Placeholder
}

void SaveGameManager::SendSaveRequestToAll() {
    // TODO: Send save request message to all connected players
    Logger::Log(LogLevel::DEBUG, "Sent save request to all players");
}

void SaveGameManager::SendSaveResponse(uint32_t requestId, bool success, const std::string& reason) {
    // TODO: Send save response message
    Logger::Log(LogLevel::DEBUG, "Sent save response: " + std::to_string(success));
}

void SaveGameManager::SendPlayerSaveState(uint32_t requestId, const PlayerSaveData& playerState) {
    // TODO: Send player save state message
    Logger::Log(LogLevel::DEBUG, "Sent player save state for peer " + std::to_string(playerState.peerId));
}

void SaveGameManager::SendSaveCompletion(uint32_t requestId, bool success, const std::string& message) {
    // TODO: Send save completion message to all players
    Logger::Log(LogLevel::DEBUG, "Sent save completion: " + message);
}

void SaveGameManager::StartSaveTimeout() {
    // TODO: Start timeout timer
    Logger::Log(LogLevel::DEBUG, "Started save timeout timer");
}

} // namespace CoopNet