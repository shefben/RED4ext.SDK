#include "AuthenticationManager.hpp"
#include "RED4ext/Api/Sdk.hpp"
#include <algorithm>
#include <random>
#include <sstream>
#include <regex>
#include <openssl/sha.h>
#include <openssl/rand.h>

namespace RED4ext
{
    // AuthenticationManager Implementation
    AuthenticationManager& AuthenticationManager::GetInstance()
    {
        static AuthenticationManager instance;
        return instance;
    }

    void AuthenticationManager::Initialize()
    {
        std::unique_lock<std::shared_mutex> sessionsLock(m_sessionsMutex);
        std::unique_lock<std::shared_mutex> permissionsLock(m_permissionsMutex);
        std::unique_lock<std::shared_mutex> bansLock(m_bansMutex);

        // Clear existing data
        m_playerSessions.clear();
        m_playerNameToId.clear();
        m_sessionTokenToPlayerId.clear();
        m_permissions.clear();
        m_defaultPermissions.clear();
        m_banRecords.clear();
        m_bannedIPs.clear();
        m_bannedHardwareIds.clear();
        m_loginAttempts.clear();

        // Initialize configuration
        m_nextPlayerId = 1;
        m_totalSessionsCreated = 0;
        m_lastCleanup = std::chrono::steady_clock::now();

        // Load persistent data
        LoadPlayerData();
        LoadBanData();

        // Initialize default permissions
        InitializeDefaultPermissions();
    }

    void AuthenticationManager::Shutdown()
    {
        // Save persistent data
        SavePlayerData();
        SaveBanData();

        // Clear all sessions
        std::unique_lock<std::shared_mutex> sessionsLock(m_sessionsMutex);
        std::unique_lock<std::shared_mutex> permissionsLock(m_permissionsMutex);
        std::unique_lock<std::shared_mutex> bansLock(m_bansMutex);

        m_playerSessions.clear();
        m_playerNameToId.clear();
        m_sessionTokenToPlayerId.clear();
        m_permissions.clear();
        m_defaultPermissions.clear();
        m_banRecords.clear();
        m_bannedIPs.clear();
        m_bannedHardwareIds.clear();
        m_loginAttempts.clear();

        // Clear callbacks
        m_playerAuthenticatedCallback = nullptr;
        m_playerLoggedOutCallback = nullptr;
        m_playerBannedCallback = nullptr;
        m_permissionChangedCallback = nullptr;
    }

    void AuthenticationManager::Update()
    {
        auto currentTime = std::chrono::steady_clock::now();

        // Periodic cleanup (every 5 minutes)
        auto timeSinceCleanup = std::chrono::duration_cast<std::chrono::minutes>(
            currentTime - m_lastCleanup).count();

        if (timeSinceCleanup >= 5) {
            CleanupExpiredSessions();
            CleanupExpiredBans();
            CleanupOldLoginAttempts();
            m_lastCleanup = currentTime;
        }
    }

    AuthenticationResult AuthenticationManager::AuthenticatePlayer(
        const std::string& playerName,
        const std::string& authToken,
        AuthenticationMethod method,
        const std::string& hardwareId,
        const std::string& ipAddress)
    {
        // Check maintenance mode
        if (m_maintenanceModeEnabled) {
            return AuthenticationResult::MaintenanceMode;
        }

        // Validate inputs
        if (!ValidatePlayerName(playerName) ||
            !ValidateAuthToken(authToken, method) ||
            !ValidateHardwareId(hardwareId)) {
            return AuthenticationResult::RequiredDataMissing;
        }

        // Check rate limiting
        if (IsIPRateLimited(ipAddress)) {
            RecordLoginAttempt(ipAddress, false, "Rate limited");
            return AuthenticationResult::TooManyAttempts;
        }

        // Check bans
        if (IsIPBanned(ipAddress) || IsHardwareBanned(hardwareId)) {
            RecordLoginAttempt(ipAddress, false, "Banned");
            return AuthenticationResult::AccountBanned;
        }

        // Check server capacity
        if (GetOnlinePlayerCount() >= m_maxOnlinePlayers) {
            RecordLoginAttempt(ipAddress, false, "Server full");
            return AuthenticationResult::ServerFull;
        }

        // Find or create player session
        uint32_t playerId = 0;
        {
            std::shared_lock<std::shared_mutex> lock(m_sessionsMutex);
            auto it = m_playerNameToId.find(playerName);
            if (it != m_playerNameToId.end()) {
                playerId = it->second;

                // Check if player is banned
                if (IsPlayerBanned(playerId)) {
                    RecordLoginAttempt(ipAddress, false, "Player banned");
                    return AuthenticationResult::AccountBanned;
                }
            }
        }

        // Create new player if not found
        if (playerId == 0) {
            playerId = GeneratePlayerId();
        }

        // Create or update session
        auto session = std::make_unique<PlayerSession>();
        session->playerId = playerId;
        session->playerName = playerName;
        session->displayName = playerName; // Can be customized later
        session->sessionToken = GenerateSessionToken();
        session->permissionLevel = PlayerPermissionLevel::Player;
        session->authMethod = method;
        session->hardwareId = hardwareId;
        session->ipAddress = ipAddress;
        session->loginTime = std::chrono::steady_clock::now();
        session->lastActivity = session->loginTime;
        session->sessionExpiry = session->loginTime + std::chrono::minutes(m_sessionTimeoutMinutes);
        session->isOnline = true;
        session->isAuthenticated = true;
        session->requiresReauth = false;

        // Apply default permissions
        ApplyDefaultPermissions(session.get());

        // Store session
        {
            std::unique_lock<std::shared_mutex> lock(m_sessionsMutex);
            m_playerSessions[playerId] = std::move(session);
            m_playerNameToId[playerName] = playerId;
            m_sessionTokenToPlayerId[m_playerSessions[playerId]->sessionToken] = playerId;
        }

        m_totalSessionsCreated++;

        // Record successful login
        RecordLoginAttempt(ipAddress, true);

        // Notify listeners
        NotifyPlayerAuthenticated(playerId);

        return AuthenticationResult::Success;
    }

    bool AuthenticationManager::LogoutPlayer(uint32_t playerId)
    {
        std::unique_lock<std::shared_mutex> lock(m_sessionsMutex);

        auto it = m_playerSessions.find(playerId);
        if (it == m_playerSessions.end()) {
            return false;
        }

        auto& session = it->second;

        // Clean up session mappings
        m_playerNameToId.erase(session->playerName);
        m_sessionTokenToPlayerId.erase(session->sessionToken);

        // Remove session
        m_playerSessions.erase(it);

        // Notify listeners
        NotifyPlayerLoggedOut(playerId);

        return true;
    }

    bool AuthenticationManager::IsPlayerAuthenticated(uint32_t playerId) const
    {
        std::shared_lock<std::shared_mutex> lock(m_sessionsMutex);

        auto it = m_playerSessions.find(playerId);
        if (it == m_playerSessions.end()) {
            return false;
        }

        auto& session = it->second;
        return session->isAuthenticated &&
               session->isOnline &&
               std::chrono::steady_clock::now() < session->sessionExpiry;
    }

    bool AuthenticationManager::RefreshPlayerSession(uint32_t playerId)
    {
        std::unique_lock<std::shared_mutex> lock(m_sessionsMutex);

        auto it = m_playerSessions.find(playerId);
        if (it == m_playerSessions.end()) {
            return false;
        }

        auto& session = it->second;
        session->sessionExpiry = std::chrono::steady_clock::now() +
                                std::chrono::minutes(m_sessionTimeoutMinutes);
        session->lastActivity = std::chrono::steady_clock::now();

        return true;
    }

    void AuthenticationManager::InvalidateAllSessions()
    {
        std::unique_lock<std::shared_mutex> lock(m_sessionsMutex);

        std::vector<uint32_t> playerIds;
        for (const auto& [playerId, session] : m_playerSessions) {
            playerIds.push_back(playerId);
        }

        // Clear all session data
        m_playerSessions.clear();
        m_playerNameToId.clear();
        m_sessionTokenToPlayerId.clear();

        lock.unlock();

        // Notify all logged out players
        for (uint32_t playerId : playerIds) {
            NotifyPlayerLoggedOut(playerId);
        }
    }

    PlayerSession* AuthenticationManager::GetPlayerSession(uint32_t playerId)
    {
        std::shared_lock<std::shared_mutex> lock(m_sessionsMutex);

        auto it = m_playerSessions.find(playerId);
        return (it != m_playerSessions.end()) ? it->second.get() : nullptr;
    }

    const PlayerSession* AuthenticationManager::GetPlayerSession(uint32_t playerId) const
    {
        std::shared_lock<std::shared_mutex> lock(m_sessionsMutex);

        auto it = m_playerSessions.find(playerId);
        return (it != m_playerSessions.end()) ? it->second.get() : nullptr;
    }

    PlayerSession* AuthenticationManager::FindSessionByToken(const std::string& token)
    {
        std::shared_lock<std::shared_mutex> lock(m_sessionsMutex);

        auto it = m_sessionTokenToPlayerId.find(token);
        if (it == m_sessionTokenToPlayerId.end()) {
            return nullptr;
        }

        return GetPlayerSession(it->second);
    }

    PlayerSession* AuthenticationManager::FindSessionByName(const std::string& playerName)
    {
        std::shared_lock<std::shared_mutex> lock(m_sessionsMutex);

        auto it = m_playerNameToId.find(playerName);
        if (it == m_playerNameToId.end()) {
            return nullptr;
        }

        return GetPlayerSession(it->second);
    }

    std::vector<uint32_t> AuthenticationManager::GetOnlinePlayers() const
    {
        std::shared_lock<std::shared_mutex> lock(m_sessionsMutex);

        std::vector<uint32_t> onlinePlayers;
        auto currentTime = std::chrono::steady_clock::now();

        for (const auto& [playerId, session] : m_playerSessions) {
            if (session->isOnline &&
                session->isAuthenticated &&
                currentTime < session->sessionExpiry) {
                onlinePlayers.push_back(playerId);
            }
        }

        return onlinePlayers;
    }

    uint32_t AuthenticationManager::GetOnlinePlayerCount() const
    {
        return static_cast<uint32_t>(GetOnlinePlayers().size());
    }

    void AuthenticationManager::UpdatePlayerActivity(uint32_t playerId)
    {
        auto* session = GetPlayerSession(playerId);
        if (session) {
            session->lastActivity = std::chrono::steady_clock::now();
        }
    }

    // Permission system implementation
    bool AuthenticationManager::HasPermission(uint32_t playerId, const std::string& permission) const
    {
        auto* session = GetPlayerSession(playerId);
        if (!session || !session->isAuthenticated) {
            return false;
        }

        // Check explicit permission
        if (session->grantedPermissions.count(permission)) {
            return true;
        }

        // Check permission level requirements
        std::shared_lock<std::shared_mutex> lock(m_permissionsMutex);
        auto it = m_permissions.find(permission);
        if (it != m_permissions.end()) {
            return static_cast<int>(session->permissionLevel) >= static_cast<int>(it->second.requiredLevel);
        }

        return false;
    }

    bool AuthenticationManager::HasPermissionLevel(uint32_t playerId, PlayerPermissionLevel level) const
    {
        auto* session = GetPlayerSession(playerId);
        if (!session || !session->isAuthenticated) {
            return false;
        }

        return static_cast<int>(session->permissionLevel) >= static_cast<int>(level);
    }

    bool AuthenticationManager::GrantPermission(uint32_t playerId, const std::string& permission)
    {
        auto* session = GetPlayerSession(playerId);
        if (!session) {
            return false;
        }

        if (!IsValidPermission(permission)) {
            return false;
        }

        session->grantedPermissions.insert(permission);
        return true;
    }

    bool AuthenticationManager::RevokePermission(uint32_t playerId, const std::string& permission)
    {
        auto* session = GetPlayerSession(playerId);
        if (!session) {
            return false;
        }

        session->grantedPermissions.erase(permission);
        return true;
    }

    bool AuthenticationManager::SetPlayerPermissionLevel(uint32_t adminId, uint32_t targetId, PlayerPermissionLevel level)
    {
        // Verify admin permissions
        if (!HasAdminPermission(adminId, targetId, "set_permission_level")) {
            return false;
        }

        auto* targetSession = GetPlayerSession(targetId);
        if (!targetSession) {
            return false;
        }

        PlayerPermissionLevel oldLevel = targetSession->permissionLevel;
        targetSession->permissionLevel = level;

        // Apply default permissions for new level
        ApplyDefaultPermissions(targetSession);

        // Notify listeners
        NotifyPermissionChanged(targetId, level);

        return true;
    }

    PlayerPermissionLevel AuthenticationManager::GetPlayerPermissionLevel(uint32_t playerId) const
    {
        auto* session = GetPlayerSession(playerId);
        return session ? session->permissionLevel : PlayerPermissionLevel::Guest;
    }

    std::vector<std::string> AuthenticationManager::GetPlayerPermissions(uint32_t playerId) const
    {
        auto* session = GetPlayerSession(playerId);
        if (!session) {
            return {};
        }

        std::vector<std::string> permissions;
        permissions.reserve(session->grantedPermissions.size());

        for (const auto& permission : session->grantedPermissions) {
            permissions.push_back(permission);
        }

        return permissions;
    }

    // Ban system implementation
    std::string AuthenticationManager::BanPlayer(uint32_t adminId, uint32_t targetId, const std::string& reason,
                                                BanType banType, uint32_t durationMinutes)
    {
        // Verify admin permissions
        if (!HasAdminPermission(adminId, targetId, "ban_player")) {
            return "";
        }

        auto* targetSession = GetPlayerSession(targetId);
        if (!targetSession) {
            return "";
        }

        auto ban = std::make_unique<BanRecord>();
        ban->banId = GenerateBanId();
        ban->playerId = targetId;
        ban->playerName = targetSession->playerName;
        ban->reason = reason;
        ban->banType = banType;
        ban->bannedById = adminId;

        auto* adminSession = GetPlayerSession(adminId);
        ban->bannedByName = adminSession ? adminSession->playerName : "System";

        ban->banTime = std::chrono::steady_clock::now();

        if (banType == BanType::Temporary && durationMinutes > 0) {
            ban->expiryTime = ban->banTime + std::chrono::minutes(durationMinutes);
        } else {
            ban->expiryTime = ban->banTime + std::chrono::hours(24 * 365 * 10); // 10 years for permanent
        }

        ban->isActive = true;
        ban->isAppealed = false;
        ban->ipAddress = targetSession->ipAddress;
        ban->hardwareId = targetSession->hardwareId;

        std::string banId = ban->banId;

        // Add to ban system
        {
            std::unique_lock<std::shared_mutex> lock(m_bansMutex);
            AddBanToIndices(*ban);
            m_banRecords.push_back(std::move(ban));
        }

        // Kick the player
        LogoutPlayer(targetId);

        // Notify listeners
        NotifyPlayerBanned(targetId, reason);

        return banId;
    }

    bool AuthenticationManager::UnbanPlayer(uint32_t adminId, const std::string& banId, const std::string& reason)
    {
        if (!HasPermissionLevel(adminId, PlayerPermissionLevel::Admin)) {
            return false;
        }

        std::unique_lock<std::shared_mutex> lock(m_bansMutex);

        auto* ban = FindBan(banId);
        if (!ban || !ban->isActive) {
            return false;
        }

        ban->isActive = false;
        ban->notes.push_back("Unbanned by admin: " + reason);

        RemoveBanFromIndices(*ban);

        return true;
    }

    bool AuthenticationManager::IsPlayerBanned(uint32_t playerId) const
    {
        std::shared_lock<std::shared_mutex> lock(m_bansMutex);

        auto currentTime = std::chrono::steady_clock::now();

        for (const auto& ban : m_banRecords) {
            if (ban->playerId == playerId &&
                ban->isActive &&
                currentTime < ban->expiryTime) {
                return true;
            }
        }

        return false;
    }

    bool AuthenticationManager::IsIPBanned(const std::string& ipAddress) const
    {
        std::shared_lock<std::shared_mutex> lock(m_bansMutex);
        return m_bannedIPs.count(ipAddress) > 0;
    }

    bool AuthenticationManager::IsHardwareBanned(const std::string& hardwareId) const
    {
        std::shared_lock<std::shared_mutex> lock(m_bansMutex);
        return m_bannedHardwareIds.count(hardwareId) > 0;
    }

    // Rate limiting implementation
    bool AuthenticationManager::IsIPRateLimited(const std::string& ipAddress) const
    {
        std::lock_guard<std::mutex> lock(m_loginAttemptsMutex);

        auto it = m_loginAttempts.find(ipAddress);
        if (it == m_loginAttempts.end()) {
            return false;
        }

        auto currentTime = std::chrono::steady_clock::now();
        auto windowStart = currentTime - std::chrono::minutes(m_loginAttemptWindowMinutes);

        // Count failed attempts in the window
        uint32_t failedAttempts = 0;
        for (const auto& attempt : it->second) {
            if (attempt.attemptTime >= windowStart && !attempt.wasSuccessful) {
                failedAttempts++;
            }
        }

        return failedAttempts >= m_maxLoginAttempts;
    }

    void AuthenticationManager::RecordLoginAttempt(const std::string& ipAddress, bool successful, const std::string& reason)
    {
        std::lock_guard<std::mutex> lock(m_loginAttemptsMutex);

        LoginAttempt attempt;
        attempt.ipAddress = ipAddress;
        attempt.attemptTime = std::chrono::steady_clock::now();
        attempt.wasSuccessful = successful;
        attempt.failureReason = reason;

        m_loginAttempts[ipAddress].push_back(attempt);
    }

    // Private implementation methods
    uint32_t AuthenticationManager::GeneratePlayerId()
    {
        return m_nextPlayerId++;
    }

    std::string AuthenticationManager::GenerateSessionToken()
    {
        return AuthUtils::GenerateSecureToken(32);
    }

    bool AuthenticationManager::ValidateAuthToken(const std::string& token, AuthenticationMethod method)
    {
        if (token.empty()) {
            return false;
        }

        switch (method) {
            case AuthenticationMethod::Anonymous:
                return true; // Anonymous auth doesn't require token validation
            case AuthenticationMethod::Steam:
            case AuthenticationMethod::GOG:
            case AuthenticationMethod::Epic:
                return token.length() >= 16; // Platform tokens should be substantial
            case AuthenticationMethod::Custom:
                return AuthUtils::ValidateSessionToken(token);
            default:
                return false;
        }
    }

    bool AuthenticationManager::ValidatePlayerName(const std::string& name)
    {
        return AuthUtils::ValidatePlayerName(name);
    }

    bool AuthenticationManager::ValidateHardwareId(const std::string& hardwareId)
    {
        return !hardwareId.empty() && hardwareId.length() <= 128;
    }

    void AuthenticationManager::InitializeDefaultPermissions()
    {
        // Register core permissions
        RegisterPermission(Permission("play", "Basic play permission", PlayerPermissionLevel::Player, true));
        RegisterPermission(Permission("chat", "Send chat messages", PlayerPermissionLevel::Player, true));
        RegisterPermission(Permission("voice", "Use voice chat", PlayerPermissionLevel::Player, true));
        RegisterPermission(Permission("create_room", "Create game rooms", PlayerPermissionLevel::Player, true));
        RegisterPermission(Permission("join_room", "Join game rooms", PlayerPermissionLevel::Player, true));

        RegisterPermission(Permission("kick_player", "Kick players from rooms", PlayerPermissionLevel::Moderator));
        RegisterPermission(Permission("mute_player", "Mute players in chat", PlayerPermissionLevel::Moderator));
        RegisterPermission(Permission("manage_rooms", "Advanced room management", PlayerPermissionLevel::Moderator));

        RegisterPermission(Permission("ban_player", "Ban players from server", PlayerPermissionLevel::Admin));
        RegisterPermission(Permission("unban_player", "Unban players", PlayerPermissionLevel::Admin));
        RegisterPermission(Permission("set_permission_level", "Change player permissions", PlayerPermissionLevel::Admin));
        RegisterPermission(Permission("view_admin_panel", "Access admin panel", PlayerPermissionLevel::Admin));

        RegisterPermission(Permission("server_config", "Modify server configuration", PlayerPermissionLevel::SuperAdmin));
        RegisterPermission(Permission("system_commands", "Execute system commands", PlayerPermissionLevel::Developer));
    }

    void AuthenticationManager::ApplyDefaultPermissions(PlayerSession* session)
    {
        std::shared_lock<std::shared_mutex> lock(m_permissionsMutex);

        // Grant all default permissions
        for (const auto& permissionName : m_defaultPermissions) {
            session->grantedPermissions.insert(permissionName);
        }

        // Grant level-based permissions
        for (const auto& [name, permission] : m_permissions) {
            if (static_cast<int>(session->permissionLevel) >= static_cast<int>(permission.requiredLevel)) {
                session->grantedPermissions.insert(name);
            }
        }
    }

    // Utility functions implementation
    namespace AuthUtils
    {
        std::string PermissionLevelToString(PlayerPermissionLevel level)
        {
            switch (level) {
                case PlayerPermissionLevel::Banned: return "Banned";
                case PlayerPermissionLevel::Guest: return "Guest";
                case PlayerPermissionLevel::Player: return "Player";
                case PlayerPermissionLevel::VIP: return "VIP";
                case PlayerPermissionLevel::Moderator: return "Moderator";
                case PlayerPermissionLevel::Admin: return "Admin";
                case PlayerPermissionLevel::SuperAdmin: return "SuperAdmin";
                case PlayerPermissionLevel::Developer: return "Developer";
                default: return "Unknown";
            }
        }

        PlayerPermissionLevel StringToPermissionLevel(const std::string& levelStr)
        {
            if (levelStr == "Banned") return PlayerPermissionLevel::Banned;
            if (levelStr == "Guest") return PlayerPermissionLevel::Guest;
            if (levelStr == "Player") return PlayerPermissionLevel::Player;
            if (levelStr == "VIP") return PlayerPermissionLevel::VIP;
            if (levelStr == "Moderator") return PlayerPermissionLevel::Moderator;
            if (levelStr == "Admin") return PlayerPermissionLevel::Admin;
            if (levelStr == "SuperAdmin") return PlayerPermissionLevel::SuperAdmin;
            if (levelStr == "Developer") return PlayerPermissionLevel::Developer;
            return PlayerPermissionLevel::Guest;
        }

        bool ValidatePlayerName(const std::string& name)
        {
            if (name.empty() || name.length() > 32) {
                return false;
            }

            // Check for valid characters (alphanumeric, spaces, underscores, hyphens)
            std::regex validPattern("^[a-zA-Z0-9 _-]+$");
            return std::regex_match(name, validPattern);
        }

        bool ValidateSessionToken(const std::string& token)
        {
            if (token.length() < 16 || token.length() > 128) {
                return false;
            }

            // Check for valid characters (alphanumeric and some symbols)
            std::regex validPattern("^[a-zA-Z0-9+/=_-]+$");
            return std::regex_match(token, validPattern);
        }

        std::string GenerateSecureToken(size_t length)
        {
            const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_int_distribution<> dis(0, chars.size() - 1);

            std::string token;
            token.reserve(length);

            for (size_t i = 0; i < length; ++i) {
                token += chars[dis(gen)];
            }

            return token;
        }

        std::string HashPassword(const std::string& password, const std::string& salt)
        {
            std::string combined = password + salt;

            unsigned char hash[SHA256_DIGEST_LENGTH];
            SHA256(reinterpret_cast<const unsigned char*>(combined.c_str()), combined.length(), hash);

            std::ostringstream oss;
            for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
                oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
            }

            return oss.str();
        }

        std::string GenerateSalt()
        {
            return GenerateSecureToken(16);
        }

        bool VerifyPassword(const std::string& password, const std::string& hash, const std::string& salt)
        {
            return HashPassword(password, salt) == hash;
        }

        uint32_t CalculatePermissionHash(const std::vector<std::string>& permissions)
        {
            std::string combined;
            for (const auto& permission : permissions) {
                combined += permission + ";";
            }

            return std::hash<std::string>{}(combined);
        }
    }

    // Missing private method implementations for AuthenticationManager
    std::string AuthenticationManager::GenerateBanId()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 15);

        std::stringstream ss;
        ss << "ban_";
        for (int i = 0; i < 8; ++i) {
            ss << std::hex << dis(gen);
        }

        return ss.str();
    }

    void AuthenticationManager::AddBanToIndices(const BanRecord& ban)
    {
        if (!ban.ipAddress.empty()) {
            m_bannedIPs.insert(ban.ipAddress);
        }

        if (!ban.hardwareId.empty()) {
            m_bannedHardwareIds.insert(ban.hardwareId);
        }
    }

    void AuthenticationManager::RemoveBanFromIndices(const BanRecord& ban)
    {
        if (!ban.ipAddress.empty()) {
            m_bannedIPs.erase(ban.ipAddress);
        }

        if (!ban.hardwareId.empty()) {
            m_bannedHardwareIds.erase(ban.hardwareId);
        }
    }

    bool AuthenticationManager::HasAdminPermission(uint32_t adminId, uint32_t targetId, const std::string& action) const
    {
        // Get admin permission level
        PlayerPermissionLevel adminLevel = GetPlayerPermissionLevel(adminId);

        // Check if admin has sufficient permission level
        if (adminLevel < PlayerPermissionLevel::Moderator) {
            return false;
        }

        // Super admins and developers can do anything
        if (adminLevel >= PlayerPermissionLevel::SuperAdmin) {
            return true;
        }

        // Get target permission level
        PlayerPermissionLevel targetLevel = GetPlayerPermissionLevel(targetId);

        // Admins cannot act on other admins of equal or higher level
        if (targetLevel >= adminLevel) {
            return false;
        }

        // Check specific action requirements
        PlayerPermissionLevel requiredLevel = GetMinimumLevelForAction(action);
        return adminLevel >= requiredLevel;
    }

    PlayerPermissionLevel AuthenticationManager::GetMinimumLevelForAction(const std::string& action) const
    {
        if (action == "ban" || action == "unban") {
            return PlayerPermissionLevel::Admin;
        } else if (action == "kick" || action == "mute") {
            return PlayerPermissionLevel::Moderator;
        } else if (action == "promote" || action == "demote") {
            return PlayerPermissionLevel::Admin;
        } else {
            return PlayerPermissionLevel::Moderator;
        }
    }

    void AuthenticationManager::NotifyPermissionChanged(uint32_t playerId, PlayerPermissionLevel level)
    {
        if (m_permissionChangedCallback) {
            m_permissionChangedCallback(playerId, level);
        }
    }

    void AuthenticationManager::SavePlayerData()
    {
        // Placeholder implementation - would save to database or file
        // This would serialize player session data for persistence
    }

    void AuthenticationManager::LoadPlayerData()
    {
        // Placeholder implementation - would load from database or file
        // This would restore player session data on startup
    }

    void AuthenticationManager::SaveBanData()
    {
        // Placeholder implementation - would save ban records to database or file
        // This would serialize ban data for persistence
    }

    void AuthenticationManager::LoadBanData()
    {
        // Placeholder implementation - would load ban records from database or file
        // This would restore ban data on startup
    }

    void AuthenticationManager::CleanupExpiredSessions()
    {
        std::unique_lock<std::shared_mutex> lock(m_sessionsMutex);

        auto currentTime = std::chrono::steady_clock::now();
        std::vector<uint32_t> sessionsToRemove;

        for (const auto& [playerId, session] : m_playerSessions) {
            if (currentTime >= session->sessionExpiry) {
                sessionsToRemove.push_back(playerId);
            }
        }

        for (uint32_t playerId : sessionsToRemove) {
            auto it = m_playerSessions.find(playerId);
            if (it != m_playerSessions.end()) {
                // Clean up mappings
                m_playerNameToId.erase(it->second->playerName);
                m_sessionTokenToPlayerId.erase(it->second->sessionToken);

                // Remove session
                m_playerSessions.erase(it);

                // Notify listeners
                lock.unlock();
                NotifyPlayerLoggedOut(playerId);
                lock.lock();
            }
        }
    }

    void AuthenticationManager::CleanupExpiredBans()
    {
        std::unique_lock<std::shared_mutex> lock(m_bansMutex);

        auto currentTime = std::chrono::steady_clock::now();

        for (auto it = m_banRecords.begin(); it != m_banRecords.end();) {
            auto& ban = *it;
            if (ban->banType == BanType::Temporary &&
                ban->isActive &&
                currentTime >= ban->expiryTime) {

                // Mark ban as inactive and remove from indices
                ban->isActive = false;
                RemoveBanFromIndices(*ban);

                ++it;
            } else {
                ++it;
            }
        }
    }

    void AuthenticationManager::CleanupOldLoginAttempts()
    {
        std::lock_guard<std::mutex> lock(m_loginAttemptsMutex);

        auto currentTime = std::chrono::steady_clock::now();
        auto cutoffTime = currentTime - std::chrono::minutes(m_loginAttemptWindowMinutes);

        for (auto& [ipAddress, attempts] : m_loginAttempts) {
            attempts.erase(
                std::remove_if(attempts.begin(), attempts.end(),
                    [cutoffTime](const LoginAttempt& attempt) {
                        return attempt.attemptTime < cutoffTime;
                    }),
                attempts.end()
            );
        }
    }

    void AuthenticationManager::NotifyPlayerAuthenticated(uint32_t playerId)
    {
        if (m_playerAuthenticatedCallback) {
            m_playerAuthenticatedCallback(playerId);
        }
    }

    void AuthenticationManager::NotifyPlayerLoggedOut(uint32_t playerId)
    {
        if (m_playerLoggedOutCallback) {
            m_playerLoggedOutCallback(playerId);
        }
    }

    void AuthenticationManager::NotifyPlayerBanned(uint32_t playerId, const std::string& reason)
    {
        if (m_playerBannedCallback) {
            m_playerBannedCallback(playerId, reason);
        }
    }

    bool AuthenticationManager::RegisterPermission(const Permission& permission)
    {
        std::unique_lock<std::shared_mutex> lock(m_permissionsMutex);

        if (m_permissions.count(permission.name)) {
            return false; // Permission already exists
        }

        m_permissions[permission.name] = permission;

        if (permission.isDefault) {
            m_defaultPermissions.insert(permission.name);
        }

        return true;
    }

    bool AuthenticationManager::IsValidPermission(const std::string& permissionName) const
    {
        std::shared_lock<std::shared_mutex> lock(m_permissionsMutex);
        return m_permissions.count(permissionName) > 0;
    }

    BanRecord* AuthenticationManager::FindBan(const std::string& banId)
    {
        std::shared_lock<std::shared_mutex> lock(m_bansMutex);

        for (auto& ban : m_banRecords) {
            if (ban->banId == banId && ban->isActive) {
                return ban.get();
            }
        }

        return nullptr;
    }

}