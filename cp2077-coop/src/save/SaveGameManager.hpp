#pragma once

#include <unordered_map>
#include <vector>
#include <mutex>
#include <chrono>
#include <string>
#include <cstdint>

namespace CoopNet {

// Save data structures
struct PlayerSaveData {
    uint32_t peerId;
    uint32_t level;
    uint64_t experience;
    uint32_t streetCred;
    uint64_t money;
    float position[3];
    float rotation[4]; // Quaternion
    uint64_t timestamp;

    // TODO: Add more comprehensive save data:
    // - Inventory data
    // - Quest progress
    // - Attribute data
    // - Cyberware data
};

struct WorldSaveData {
    uint64_t gameTime;
    uint32_t weatherState;
    uint32_t ncpdWanted;
    uint64_t timestamp;

    // TODO: Add more comprehensive world data:
    // - Completed gigs
    // - Discovered locations
    // - Vehicle states
    // - World events
};

struct CompleteSaveData {
    uint64_t sessionId;
    uint32_t saveSlot;
    uint64_t timestamp;
    uint32_t version;
    uint32_t checksum;

    std::vector<PlayerSaveData> playerStates;
    WorldSaveData worldState;
};

struct SaveRequest {
    uint32_t requestId;
    uint32_t saveSlot;
    uint32_t initiatorPeerId;
    uint64_t timestamp;
    uint32_t playersReady;
    uint32_t expectedPlayers;
};

class SaveGameManager {
public:
    static SaveGameManager& Instance();

    // Initialization
    bool Initialize();
    void Cleanup();

    // Coordinated save operations
    bool InitiateCoordinatedSave(uint32_t saveSlot, uint32_t initiatorPeerId);
    bool OnSaveRequest(uint32_t requestId, uint32_t saveSlot, uint32_t initiatorPeerId);
    bool OnPlayerSaveStateReceived(uint32_t requestId, const PlayerSaveData& playerState);
    void OnSaveTimeout();

    // Load operations
    bool LoadCoordinatedSave(uint32_t saveSlot);

    // Status queries
    bool IsSaveInProgress() const { return m_saveInProgress; }
    uint32_t GetCurrentSaveRequestId() const { return m_currentSaveRequest.requestId; }

private:
    SaveGameManager();
    ~SaveGameManager();

    // Delete copy constructor and assignment operator
    SaveGameManager(const SaveGameManager&) = delete;
    SaveGameManager& operator=(const SaveGameManager&) = delete;

    // Save execution
    void ExecuteCoordinatedSave();
    void OnSaveCompleted(bool success, const std::string& message);

    // Save data building
    PlayerSaveData BuildPlayerSaveState();
    CompleteSaveData BuildCompleteSaveData();
    WorldSaveData BuildWorldState();

    // Validation
    bool ValidatePlayerSaveState(const PlayerSaveData& state);
    bool ValidateCompleteSaveData(const CompleteSaveData& saveData);
    uint32_t CalculateSaveChecksum(const CompleteSaveData& saveData);

    // File operations
    bool PerformSave(const CompleteSaveData& saveData);
    bool LoadSaveData(uint32_t saveSlot, CompleteSaveData& saveData);
    bool ApplySaveData(const CompleteSaveData& saveData);
    bool ApplyWorldState(const WorldSaveData& worldState);
    bool ApplyPlayerState(const PlayerSaveData& playerState);

    // Utility functions
    uint32_t GenerateRequestId();
    uint64_t GetCurrentTimestamp();
    uint32_t GetConnectedPlayerCount();
    bool CanPlayerSave();
    std::string GetSavePath(uint32_t saveSlot);
    void CreateSaveDirectories();
    uint64_t GetCurrentSessionId();

    // Network communication
    void SendSaveRequestToAll();
    void SendSaveResponse(uint32_t requestId, bool success, const std::string& reason);
    void SendPlayerSaveState(uint32_t requestId, const PlayerSaveData& playerState);
    void SendSaveCompletion(uint32_t requestId, bool success, const std::string& message);
    void StartSaveTimeout();

    // Constants
    static constexpr uint32_t MAX_SAVE_SLOTS = 20;
    static constexpr uint32_t SAVE_TIMEOUT_MS = 60000; // 1 minute
    static constexpr uint32_t SAVE_VERSION = 1;

    // Member variables
    bool m_initialized;
    bool m_saveInProgress;

    SaveRequest m_currentSaveRequest;
    std::unordered_map<uint32_t, PlayerSaveData> m_playerSaveStates;
    std::vector<uint32_t> m_pendingRequests;

    mutable std::mutex m_saveMutex;
};

} // namespace CoopNet