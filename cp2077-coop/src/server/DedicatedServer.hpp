#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <atomic>
#include <memory>
#include <optional>
#include <sstream>

namespace CoopNet {

struct ServerConfig {
    std::string serverName;
    std::string description;
    uint16_t port;
    uint32_t maxPlayers;
    std::string password;
    std::string gameMode;
    std::string mapName;
    std::string region;
    bool enableAntiCheat;
    bool enableVoiceChat;
    bool publicServer;
    std::vector<std::string> tags;
    std::string welcomeMessage;
    std::string motd; // Message of the day
};

struct PlayerInfo {
    uint32_t id;
    std::string name;
    std::string ipAddress;
    uint32_t ping;
    uint64_t connectionTime;
    bool isAdmin;
};

class DedicatedServer {
public:
    DedicatedServer();
    ~DedicatedServer();
    
    // Core server lifecycle
    bool Initialize(const ServerConfig& config);
    void Start();
    void Stop();
    bool IsRunning() const { return m_isRunning; }
    
    // Configuration management
    bool LoadConfig(const std::string& configFile);
    void SaveConfig(const std::string& configFile);
    void LoadDefaultConfig();
    void ReloadConfig();
    const ServerConfig& GetConfig() const { return m_config; }
    
    // Server management
    void RequestShutdown();
    
    // Console commands
    void ProcessConsoleCommand(const std::string& command);
    void ShowConsoleHelp();
    void ShowServerStatus();
    void ListConnectedPlayers();
    void KickPlayer(const std::string& playerName);
    void BanPlayer(const std::string& playerName);
    void BroadcastServerMessage(const std::string& message);
    
    // Player management
    bool IsIPBanned(const std::string& ip) const;
    void LoadBanList();
    void SaveBanList();
    uint32_t FindPlayerByName(const std::string& playerName);
    
private:
    // Core server loop
    void ServerLoop();
    void ProcessServerTick();
    void ProcessNetworkEvents();
    void ProcessConsoleInput();
    
    // Game systems
    void InitializeGameSystems();
    void CleanupGameSystems();
    void UpdatePlayerStates();
    void UpdateGameWorld();
    void ProcessInventorySync();
    void ProcessAntiCheat();
    void SendPeriodicUpdates();
    void CleanupExpiredData();
    void UpdateStatistics();
    
    // Network event processing
    void ProcessNewConnections();
    void ProcessClientMessages();
    void ProcessDisconnections();
    void DisconnectAllClients();
    
    // World state management
    void SaveWorldState();
    void LoadWorldState();
    
    // Plugin system
    void LoadServerPlugins();
    
    // Configuration processing
    void ProcessConfigOption(const std::string& key, const std::string& value);
    
    // Utility functions
    uint64_t GetCurrentTimeMs() const;
    bool HasConsoleInput();
    std::string ReadConsoleCommand();
    void SetLogLevel(const std::string& level);
    
private:
    // Core state
    std::atomic<bool> m_isRunning;
    ServerConfig m_config;
    
    // Network settings
    uint16_t m_port;
    uint32_t m_maxPlayers;
    
    // Timing
    uint32_t m_tickRate;
    uint64_t m_lastTick;
    uint64_t m_tickInterval;
    uint64_t m_startTime;
    
    // Ban management
    std::unordered_set<std::string> m_bannedIPs;
    
    // Statistics
    struct ServerStats {
        uint64_t totalConnections = 0;
        uint64_t totalPacketsSent = 0;
        uint64_t totalPacketsReceived = 0;
        uint64_t totalBytesIn = 0;
        uint64_t totalBytesOut = 0;
        uint32_t peakPlayers = 0;
        double averageTickTime = 0.0;
    } m_stats;
};

// Global server management functions
DedicatedServer* GetDedicatedServerInstance();
bool StartDedicatedServer(const ServerConfig& config);
void StopDedicatedServer();

} // namespace CoopNet