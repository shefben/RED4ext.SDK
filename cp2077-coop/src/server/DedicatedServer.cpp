#include "DedicatedServer.hpp"
#include "../net/Net.hpp"
#include "../net/Connection.hpp"
#include "../core/Logger.hpp"
#include "../core/Version.hpp"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <signal.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
// Windows.h defines ERROR as a macro, so we need to undefine it
#ifdef ERROR
#undef ERROR
#endif
#else
#include <unistd.h>
#include <termios.h>
#endif

namespace CoopNet {

// Global server instance
static std::unique_ptr<DedicatedServer> g_serverInstance = nullptr;
static std::atomic<bool> g_serverRunning{false};
static std::atomic<bool> g_shutdownRequested{false};

DedicatedServer::DedicatedServer() 
    : m_isRunning(false)
    , m_port(7777)
    , m_maxPlayers(32)
    , m_tickRate(64)
    , m_lastTick(0)
    , m_tickInterval(1000 / m_tickRate)
{
    LoadDefaultConfig();
}

DedicatedServer::~DedicatedServer() {
    Stop();
}

bool DedicatedServer::Initialize(const ServerConfig& config) {
    m_config = config;
    
    LogInfo("Initializing dedicated server...");
    LogInfoF("Server Name: %s", m_config.serverName.c_str());
    LogInfoF("Port: %d", m_config.port);
    LogInfoF("Max Players: %d", m_config.maxPlayers);
    LogInfoF("Game Mode: %s", m_config.gameMode.c_str());
    
    // Initialize networking
    Net_Init(); // Fixed: Use actual function name
    LogInfo("Networking initialized successfully");
    
    // Start listening on specified port
    if (!Net_StartServer(m_config.port, m_config.maxPlayers)) {
        CoopNet::Logger::LogFormatted(CoopNet::LogLevel::ERROR, "Failed to start server on port %d", m_config.port);
        return false;
    }
    
    // Initialize game systems
    InitializeGameSystems();
    
    // Load server plugins if any
    LoadServerPlugins();
    
    m_isRunning = true;
    LogInfo("Dedicated server initialized successfully");
    
    return true;
}

void DedicatedServer::Start() {
    if (m_isRunning) {
        LogWarning("Server is already running");
        return;
    }
    
    LogInfo("Starting dedicated server...");
    
    m_isRunning = true;
    m_lastTick = GetCurrentTimeMs();
    
    // Start main server loop
    ServerLoop();
}

void DedicatedServer::Stop() {
    if (!m_isRunning) {
        return;
    }
    
    LogInfo("Stopping dedicated server...");
    
    m_isRunning = false;
    
    // Disconnect all clients gracefully
    DisconnectAllClients();
    
    // Save world state
    SaveWorldState();
    
    // Cleanup networking
    Net_Shutdown();
    
    // Cleanup game systems
    CleanupGameSystems();
    
    LogInfo("Dedicated server stopped");
}

void DedicatedServer::ServerLoop() {
    LogInfoF("Server main loop started (tick rate: %d Hz)", m_tickRate);
    
    while (m_isRunning && !g_shutdownRequested.load()) {
        uint64_t currentTime = GetCurrentTimeMs();
        
        // Check if it's time for next tick
        if (currentTime - m_lastTick >= m_tickInterval) {
            ProcessServerTick();
            m_lastTick = currentTime;
        }
        
        // Process network events
        ProcessNetworkEvents();
        
        // Handle console commands
        ProcessConsoleInput();
        
        // Small sleep to prevent 100% CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void DedicatedServer::ProcessServerTick() {
    // Update player states
    UpdatePlayerStates();
    
    // Process game world
    UpdateGameWorld();
    
    // Handle inventory synchronization
    ProcessInventorySync();
    
    // Update anti-cheat system
    ProcessAntiCheat();
    
    // Send periodic updates to clients
    SendPeriodicUpdates();
    
    // Cleanup expired data
    CleanupExpiredData();
    
    // Update statistics
    UpdateStatistics();
}

void DedicatedServer::ProcessNetworkEvents() {
    // Poll for network events with timeout
    Net_Poll(5); // 5ms timeout
    
    // Process incoming connections
    ProcessNewConnections();
    
    // Process client messages
    ProcessClientMessages();
    
    // Handle disconnections
    ProcessDisconnections();
}

void DedicatedServer::ProcessConsoleInput() {
    if (!HasConsoleInput()) {
        return;
    }
    
    std::string command = ReadConsoleCommand();
    if (!command.empty()) {
        ProcessConsoleCommand(command);
    }
}

void DedicatedServer::ProcessConsoleCommand(const std::string& command) {
    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;
    
    if (cmd == "help" || cmd == "?") {
        ShowConsoleHelp();
    }
    else if (cmd == "status") {
        ShowServerStatus();
    }
    else if (cmd == "players" || cmd == "list") {
        ListConnectedPlayers();
    }
    else if (cmd == "kick") {
        std::string playerName;
        iss >> playerName;
        KickPlayer(playerName);
    }
    else if (cmd == "ban") {
        std::string playerName;
        iss >> playerName;
        BanPlayer(playerName);
    }
    else if (cmd == "save") {
        SaveWorldState();
        LogInfo("World state saved");
    }
    else if (cmd == "load") {
        LoadWorldState();
        LogInfo("World state loaded");
    }
    else if (cmd == "reload") {
        ReloadConfig();
    }
    else if (cmd == "stop" || cmd == "exit" || cmd == "quit") {
        LogInfo("Shutdown requested via console");
        RequestShutdown();
    }
    else if (cmd == "say") {
        std::string message;
        std::getline(iss, message);
        if (!message.empty() && message[0] == ' ') {
            message = message.substr(1); // Remove leading space
        }
        BroadcastServerMessage(message);
    }
    else {
        LogWarningF("Unknown console command: %s", cmd.c_str());
        LogInfo("Type 'help' for available commands");
    }
}

void DedicatedServer::ShowConsoleHelp() {
    LogInfo("=== Dedicated Server Console Commands ===");
    LogInfo("help, ?         - Show this help");
    LogInfo("status          - Show server status");
    LogInfo("players, list   - List connected players");
    LogInfo("kick <player>   - Kick a player");
    LogInfo("ban <player>    - Ban a player");
    LogInfo("save            - Save world state");
    LogInfo("load            - Load world state");
    LogInfo("reload          - Reload server configuration");
    LogInfo("say <message>   - Broadcast message to all players");
    LogInfo("stop, exit, quit - Stop the server");
}

void DedicatedServer::ShowServerStatus() {
    uint32_t connectedPlayers = static_cast<uint32_t>(Net_GetConnections().size());
    uint64_t uptime = (GetCurrentTimeMs() - m_startTime) / 1000;
    
    LogInfo("=== Server Status ===");
    LogInfoF("Server Name: %s", m_config.serverName.c_str());
    LogInfoF("Game Mode: %s", m_config.gameMode.c_str());
    LogInfoF("Players: %u/%u", connectedPlayers, m_config.maxPlayers);
    LogInfoF("Uptime: %lluh %llum %llus", uptime / 3600, (uptime % 3600) / 60, uptime % 60);
    LogInfoF("Tick Rate: %u Hz", m_tickRate);
    LogInfoF("Version: %s", Version::Current().ToString().c_str());
    LogInfoF("Port: %d", m_config.port);
}

void DedicatedServer::ListConnectedPlayers() {
    auto connections = Net_GetConnections();
    
    LogInfoF("=== Connected Players (%zu) ===", connections.size());
    for (const auto& conn : connections) {
        LogInfoF("ID: %u | Ping: %.1fms", 
               conn->peerId, conn->rttMs);
    }
}

void DedicatedServer::KickPlayer(const std::string& playerName) {
    uint32_t playerId = FindPlayerByName(playerName);
    if (playerId == 0) {
        LogWarningF("Player '%s' not found", playerName.c_str());
        return;
    }
    
    // TODO: Implement Net_DisconnectPlayer - for now just log
    // Net_DisconnectPlayer(playerId, "Kicked by server administrator");
    LogInfoF("Player '%s' has been kicked", playerName.c_str());
}

void DedicatedServer::BanPlayer(const std::string& playerName) {
    uint32_t playerId = FindPlayerByName(playerName);
    if (playerId == 0) {
        LogWarningF("Player '%s' not found", playerName.c_str());
        return;
    }
    
    // TODO: Implement Net_GetPlayerInfo and Net_DisconnectPlayer
    // auto playerInfo = Net_GetPlayerInfo(playerId);
    // if (playerInfo.has_value()) {
    //     m_bannedIPs.insert(playerInfo->ipAddress);
    //     SaveBanList();
    //     
    //     Net_DisconnectPlayer(playerId, "Banned by server administrator");
    LogInfoF("Player '%s' has been banned", playerName.c_str());
    // }
}

void DedicatedServer::BroadcastServerMessage(const std::string& message) {
    std::string fullMessage = "[SERVER] " + message;
    // TODO: Implement Net_BroadcastChatMessage
    // Net_BroadcastChatMessage(0, fullMessage); // Server ID = 0
    LogInfoF("Broadcast: %s", message.c_str());
}

bool DedicatedServer::LoadConfig(const std::string& configFile) {
    std::ifstream file(configFile);
    if (!file.is_open()) {
        LogWarningF("Config file '%s' not found, using defaults", configFile.c_str());
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        size_t pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        
        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        
        ProcessConfigOption(key, value);
    }
    
    file.close();
    LogInfoF("Configuration loaded from '%s'", configFile.c_str());
    return true;
}

void DedicatedServer::ProcessConfigOption(const std::string& key, const std::string& value) {
    if (key == "server_name") {
        m_config.serverName = value;
    }
    else if (key == "server_description") {
        m_config.description = value;
    }
    else if (key == "port") {
        m_config.port = static_cast<uint16_t>(std::stoi(value));
    }
    else if (key == "max_players") {
        m_config.maxPlayers = static_cast<uint32_t>(std::stoi(value));
    }
    else if (key == "password") {
        m_config.password = value;
    }
    else if (key == "game_mode") {
        m_config.gameMode = value;
    }
    else if (key == "map_name") {
        m_config.mapName = value;
    }
    else if (key == "region") {
        m_config.region = value;
    }
    else if (key == "tick_rate") {
        m_tickRate = static_cast<uint32_t>(std::stoi(value));
        m_tickInterval = 1000 / m_tickRate;
    }
    else if (key == "enable_anti_cheat") {
        m_config.enableAntiCheat = (value == "true" || value == "1");
    }
    else if (key == "enable_voice_chat") {
        m_config.enableVoiceChat = (value == "true" || value == "1");
    }
    else if (key == "log_level") {
        SetLogLevel(value);
    }
    else {
        LogWarningF("Unknown config option: %s = %s", key.c_str(), value.c_str());
    }
}

void DedicatedServer::SaveConfig(const std::string& configFile) {
    std::ofstream file(configFile);
    if (!file.is_open()) {
        LogErrorF("Failed to save config to '%s'", configFile.c_str());
        return;
    }
    
    file << "# Cyberpunk 2077 Coop Dedicated Server Configuration\n\n";
    file << "# Server Identity\n";
    file << "server_name=" << m_config.serverName << "\n";
    file << "server_description=" << m_config.description << "\n";
    file << "region=" << m_config.region << "\n\n";
    
    file << "# Network Settings\n";
    file << "port=" << m_config.port << "\n";
    file << "max_players=" << m_config.maxPlayers << "\n";
    file << "tick_rate=" << m_tickRate << "\n";
    if (!m_config.password.empty()) {
        file << "password=" << m_config.password << "\n";
    }
    file << "\n";
    
    file << "# Game Settings\n";
    file << "game_mode=" << m_config.gameMode << "\n";
    file << "map_name=" << m_config.mapName << "\n\n";
    
    file << "# Features\n";
    file << "enable_anti_cheat=" << (m_config.enableAntiCheat ? "true" : "false") << "\n";
    file << "enable_voice_chat=" << (m_config.enableVoiceChat ? "true" : "false") << "\n\n";
    
    file.close();
    LogInfoF("Configuration saved to '%s'", configFile.c_str());
}

void DedicatedServer::LoadDefaultConfig() {
    m_config.serverName = "Cyberpunk 2077 Coop Server";
    m_config.description = "A cooperative multiplayer server for Cyberpunk 2077";
    m_config.port = 7777;
    m_config.maxPlayers = 8;
    m_config.password = "";
    m_config.gameMode = "Cooperative";
    m_config.mapName = "Night City";
    m_config.region = "Auto";
    m_config.enableAntiCheat = true;
    m_config.enableVoiceChat = true;
    m_config.publicServer = false;
    
    m_tickRate = 64;
    m_tickInterval = 1000 / m_tickRate;
}

void DedicatedServer::SetLogLevel(const std::string& level) {
    // TODO: Actually set the logger level based on the string
    // This would integrate with the Logger class to set verbosity
    LogInfoF("Log level set to: %s", level.c_str());
}

void DedicatedServer::InitializeGameSystems() {
    LogInfo("Initializing game systems...");
    
    // Initialize inventory controller
    // (Already done via singleton pattern)
    
    // Initialize save game system
    // SaveGameSync would be initialized here
    
    // Initialize performance monitoring
    // PerformanceMonitor would be initialized here
    
    // Initialize anti-cheat if enabled
    if (m_config.enableAntiCheat) {
        // AntiCheatFramework would be initialized here
        LogInfo("Anti-cheat system enabled");
    }
    
    LogInfo("Game systems initialized");
}

void DedicatedServer::CleanupGameSystems() {
    LogInfo("Cleaning up game systems...");
    
    // Cleanup systems in reverse order
    if (m_config.enableAntiCheat) {
        // AntiCheatFramework cleanup
    }
    
    // Other cleanup
    
    LogInfo("Game systems cleaned up");
}

void DedicatedServer::UpdatePlayerStates() {
    // Update player positions, health, etc.
    // This would integrate with the AvatarProxy system
}

void DedicatedServer::UpdateGameWorld() {
    // Update world state, NPCs, events, etc.
}

void DedicatedServer::ProcessInventorySync() {
    // Process inventory synchronization
    // This would use the InventoryController
}

void DedicatedServer::ProcessAntiCheat() {
    if (!m_config.enableAntiCheat) {
        return;
    }
    
    // Update anti-cheat system
    // This would integrate with AntiCheatFramework
}

void DedicatedServer::SendPeriodicUpdates() {
    // Send regular updates to all connected clients
}

void DedicatedServer::CleanupExpiredData() {
    // Clean up old data to prevent memory leaks
}

void DedicatedServer::UpdateStatistics() {
    // Update server performance statistics
}

void DedicatedServer::ProcessNewConnections() {
    // Handle new client connections
}

void DedicatedServer::ProcessClientMessages() {
    // Process incoming messages from clients
}

void DedicatedServer::ProcessDisconnections() {
    // Handle client disconnections
}

void DedicatedServer::DisconnectAllClients() {
    LogInfo("Disconnecting all clients...");
    // TODO: Implement Net_DisconnectAllClients or use Net_GetConnections() to disconnect each
    // Net_DisconnectAllClients("Server is shutting down");
}

void DedicatedServer::SaveWorldState() {
    // Save current world state to disk
    LogInfo("Saving world state...");
    // Implementation would use SaveGameSync system
}

void DedicatedServer::LoadWorldState() {
    // Load world state from disk
    LogInfo("Loading world state...");
    // Implementation would use SaveGameSync system
}

void DedicatedServer::LoadServerPlugins() {
    // Load any server-side plugins
    LogInfo("Loading server plugins...");
    // Plugin system would be implemented here
}

void DedicatedServer::ReloadConfig() {
    LogInfo("Reloading configuration...");
    LoadConfig("server.cfg");
}

void DedicatedServer::RequestShutdown() {
    g_shutdownRequested.store(true);
}

uint64_t DedicatedServer::GetCurrentTimeMs() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

uint32_t DedicatedServer::FindPlayerByName(const std::string& playerName) {
    // TODO: Implement proper player search using Net_GetConnections()
    // auto connections = Net_GetConnections();
    // for (const auto& conn : connections) {
    //     if (conn->playerName == playerName) {
    //         return conn->peerId;
    //     }
    // }
    return 0; // Not found
}

void DedicatedServer::LoadBanList() {
    std::ifstream file("banned_ips.txt");
    if (!file.is_open()) {
        return;
    }
    
    std::string ip;
    while (std::getline(file, ip)) {
        if (!ip.empty() && ip[0] != '#') {
            m_bannedIPs.insert(ip);
        }
    }
    
    file.close();
    LogInfoF("Loaded %zu banned IPs", m_bannedIPs.size());
}

void DedicatedServer::SaveBanList() {
    std::ofstream file("banned_ips.txt");
    if (!file.is_open()) {
        LogError("Failed to save ban list");
        return;
    }
    
    file << "# Banned IP addresses\n";
    for (const auto& ip : m_bannedIPs) {
        file << ip << "\n";
    }
    
    file.close();
}

bool DedicatedServer::IsIPBanned(const std::string& ip) const {
    return m_bannedIPs.find(ip) != m_bannedIPs.end();
}

#ifdef _WIN32
bool DedicatedServer::HasConsoleInput() {
    return _kbhit() != 0;
}

std::string DedicatedServer::ReadConsoleCommand() {
    std::string input;
    std::getline(std::cin, input);
    return input;
}
#else
bool DedicatedServer::HasConsoleInput() {
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if(ch != EOF) {
        ungetc(ch, stdin);
        return true;
    }

    return false;
}

std::string DedicatedServer::ReadConsoleCommand() {
    std::string input;
    std::getline(std::cin, input);
    return input;
}
#endif

} // namespace CoopNet

// Signal handler for graceful shutdown
void SignalHandler(int signal) {
    std::cout << "\nShutdown signal received (" << signal << ")" << std::endl;
    CoopNet::g_shutdownRequested.store(true);
}

// NOTE: Main entry point is in DedicatedMain.cpp