#pragma once

#include "RED4ext/Api/Sdk.hpp"
#include "RED4ext/Common.hpp"
#include "RED4ext/NativeTypes.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <chrono>
#include <functional>
#include <string>
#include <memory>

namespace RED4ext
{
    // Authentication and permission enums
    enum class AuthenticationResult : uint8_t
    {
        Success = 0,
        InvalidCredentials = 1,
        AccountBanned = 2,
        AccountSuspended = 3,
        TooManyAttempts = 4,
        ServerFull = 5,
        VersionMismatch = 6,
        NetworkError = 7,
        MaintenanceMode = 8,
        RequiredDataMissing = 9
    };

    enum class PlayerPermissionLevel : uint8_t
    {
        Banned = 0,
        Guest = 1,
        Player = 2,
        VIP = 3,
        Moderator = 4,
        Admin = 5,
        SuperAdmin = 6,
        Developer = 7
    };

    enum class BanType : uint8_t
    {
        Temporary = 0,
        Permanent = 1,
        IP = 2,
        Hardware = 3
    };

    enum class AuthenticationMethod : uint8_t
    {
        Anonymous = 0,
        Steam = 1,
        GOG = 2,
        Epic = 3,
        Custom = 4
    };

    // Player session information
    struct PlayerSession
    {
        uint32_t playerId;
        std::string playerName;
        std::string displayName;
        std::string sessionToken;
        PlayerPermissionLevel permissionLevel;
        AuthenticationMethod authMethod;

        std::string steamId;
        std::string gogId;
        std::string epicId;
        std::string hardwareId;
        std::string ipAddress;

        std::chrono::steady_clock::time_point loginTime;
        std::chrono::steady_clock::time_point lastActivity;
        std::chrono::steady_clock::time_point sessionExpiry;

        bool isOnline;
        bool isAuthenticated;
        bool requiresReauth;

        std::unordered_set<std::string> grantedPermissions;
        std::unordered_map<std::string, std::string> sessionData;

        PlayerSession()
            : playerId(0), permissionLevel(PlayerPermissionLevel::Guest),
              authMethod(AuthenticationMethod::Anonymous), isOnline(false),
              isAuthenticated(false), requiresReauth(false) {}
    };

    // Ban record
    struct BanRecord
    {
        std::string banId;
        uint32_t playerId;
        std::string playerName;
        std::string reason;
        BanType banType;

        uint32_t bannedById; // Admin who issued the ban
        std::string bannedByName;

        std::chrono::steady_clock::time_point banTime;
        std::chrono::steady_clock::time_point expiryTime;

        bool isActive;
        bool isAppealed;

        std::string ipAddress;
        std::string hardwareId;
        std::vector<std::string> notes;

        BanRecord() : playerId(0), banType(BanType::Temporary), bannedById(0),
                     isActive(true), isAppealed(false) {}
    };

    // Login attempt tracking
    struct LoginAttempt
    {
        std::string ipAddress;
        std::chrono::steady_clock::time_point attemptTime;
        bool wasSuccessful;
        std::string failureReason;
    };

    // Permission definition
    struct Permission
    {
        std::string name;
        std::string description;
        PlayerPermissionLevel requiredLevel;
        bool isDefault; // Granted to all players by default

        Permission() : requiredLevel(PlayerPermissionLevel::Player), isDefault(false) {}
        Permission(const std::string& n, const std::string& desc, PlayerPermissionLevel level, bool def = false)
            : name(n), description(desc), requiredLevel(level), isDefault(def) {}
    };

    // Authentication Manager - handles player login, permissions, and security
    class AuthenticationManager
    {
    public:
        static AuthenticationManager& GetInstance();

        // Lifecycle
        void Initialize();
        void Shutdown();
        void Update();

        // Authentication
        AuthenticationResult AuthenticatePlayer(const std::string& playerName,
                                              const std::string& authToken,
                                              AuthenticationMethod method,
                                              const std::string& hardwareId,
                                              const std::string& ipAddress);

        bool LogoutPlayer(uint32_t playerId);
        bool IsPlayerAuthenticated(uint32_t playerId) const;
        bool RefreshPlayerSession(uint32_t playerId);
        void InvalidateAllSessions();

        // Session management
        PlayerSession* GetPlayerSession(uint32_t playerId);
        const PlayerSession* GetPlayerSession(uint32_t playerId) const;
        PlayerSession* FindSessionByToken(const std::string& token);
        PlayerSession* FindSessionByName(const std::string& playerName);

        std::vector<uint32_t> GetOnlinePlayers() const;
        uint32_t GetOnlinePlayerCount() const;
        void UpdatePlayerActivity(uint32_t playerId);

        // Permission system
        bool HasPermission(uint32_t playerId, const std::string& permission) const;
        bool HasPermissionLevel(uint32_t playerId, PlayerPermissionLevel level) const;
        bool GrantPermission(uint32_t playerId, const std::string& permission);
        bool RevokePermission(uint32_t playerId, const std::string& permission);
        bool SetPlayerPermissionLevel(uint32_t adminId, uint32_t targetId, PlayerPermissionLevel level);

        PlayerPermissionLevel GetPlayerPermissionLevel(uint32_t playerId) const;
        std::vector<std::string> GetPlayerPermissions(uint32_t playerId) const;

        // Permission definitions
        bool RegisterPermission(const Permission& permission);
        bool UnregisterPermission(const std::string& permissionName);
        std::vector<Permission> GetAllPermissions() const;
        bool IsValidPermission(const std::string& permissionName) const;

        // Ban system
        std::string BanPlayer(uint32_t adminId, uint32_t targetId, const std::string& reason,
                             BanType banType, uint32_t durationMinutes = 0);
        bool UnbanPlayer(uint32_t adminId, const std::string& banId, const std::string& reason);
        bool IsPlayerBanned(uint32_t playerId) const;
        bool IsIPBanned(const std::string& ipAddress) const;
        bool IsHardwareBanned(const std::string& hardwareId) const;

        std::vector<BanRecord> GetActiveBans() const;
        std::vector<BanRecord> GetPlayerBans(uint32_t playerId) const;
        BanRecord* FindBan(const std::string& banId);

        // Security and rate limiting
        bool IsIPRateLimited(const std::string& ipAddress) const;
        void RecordLoginAttempt(const std::string& ipAddress, bool successful, const std::string& reason = "");
        void ClearLoginAttempts(const std::string& ipAddress);

        // Administrative functions
        bool PromotePlayer(uint32_t adminId, uint32_t targetId, PlayerPermissionLevel newLevel);
        bool DemotePlayer(uint32_t adminId, uint32_t targetId, PlayerPermissionLevel newLevel);
        bool KickPlayer(uint32_t adminId, uint32_t targetId, const std::string& reason);

        // Server configuration
        void SetMaxOnlinePlayers(uint32_t maxPlayers);
        void SetSessionTimeout(uint32_t timeoutMinutes);
        void SetMaxLoginAttempts(uint32_t maxAttempts);
        void SetLoginAttemptWindow(uint32_t windowMinutes);
        void EnableMaintenanceMode(bool enabled);
        void SetRequiredClientVersion(const std::string& version);

        // Events and callbacks
        using PlayerAuthenticatedCallback = std::function<void(uint32_t playerId)>;
        using PlayerLoggedOutCallback = std::function<void(uint32_t playerId)>;
        using PlayerBannedCallback = std::function<void(uint32_t playerId, const std::string& reason)>;
        using PermissionChangedCallback = std::function<void(uint32_t playerId, PlayerPermissionLevel level)>;

        void SetPlayerAuthenticatedCallback(PlayerAuthenticatedCallback callback);
        void SetPlayerLoggedOutCallback(PlayerLoggedOutCallback callback);
        void SetPlayerBannedCallback(PlayerBannedCallback callback);
        void SetPermissionChangedCallback(PermissionChangedCallback callback);

        // Statistics and monitoring
        uint32_t GetTotalRegisteredPlayers() const;
        uint32_t GetBannedPlayerCount() const;
        std::chrono::milliseconds GetAverageSessionDuration() const;
        std::unordered_map<PlayerPermissionLevel, uint32_t> GetPermissionLevelDistribution() const;

        // Data persistence
        void SavePlayerData();
        void LoadPlayerData();
        void SaveBanData();
        void LoadBanData();

    private:
        AuthenticationManager() = default;
        ~AuthenticationManager() = default;
        AuthenticationManager(const AuthenticationManager&) = delete;
        AuthenticationManager& operator=(const AuthenticationManager&) = delete;

        // Player data
        std::unordered_map<uint32_t, std::unique_ptr<PlayerSession>> m_playerSessions;
        std::unordered_map<std::string, uint32_t> m_playerNameToId;
        std::unordered_map<std::string, uint32_t> m_sessionTokenToPlayerId;

        // Permission system
        std::unordered_map<std::string, Permission> m_permissions;
        std::unordered_set<std::string> m_defaultPermissions;

        // Ban system
        std::vector<std::unique_ptr<BanRecord>> m_banRecords;
        std::unordered_set<std::string> m_bannedIPs;
        std::unordered_set<std::string> m_bannedHardwareIds;

        // Rate limiting
        std::unordered_map<std::string, std::vector<LoginAttempt>> m_loginAttempts;

        // Thread safety
        mutable std::shared_mutex m_sessionsMutex;
        mutable std::shared_mutex m_permissionsMutex;
        mutable std::shared_mutex m_bansMutex;
        mutable std::mutex m_loginAttemptsMutex;
        mutable std::mutex m_configMutex;

        // Configuration
        uint32_t m_maxOnlinePlayers = 100;
        uint32_t m_sessionTimeoutMinutes = 60;
        uint32_t m_maxLoginAttempts = 5;
        uint32_t m_loginAttemptWindowMinutes = 15;
        bool m_maintenanceModeEnabled = false;
        std::string m_requiredClientVersion;

        // Statistics
        uint32_t m_nextPlayerId = 1;
        uint32_t m_totalSessionsCreated = 0;
        std::chrono::steady_clock::time_point m_lastCleanup;

        // Callbacks
        PlayerAuthenticatedCallback m_playerAuthenticatedCallback;
        PlayerLoggedOutCallback m_playerLoggedOutCallback;
        PlayerBannedCallback m_playerBannedCallback;
        PermissionChangedCallback m_permissionChangedCallback;

        // Internal methods
        uint32_t GeneratePlayerId();
        std::string GenerateSessionToken();
        bool ValidateAuthToken(const std::string& token, AuthenticationMethod method);
        bool ValidatePlayerName(const std::string& name);
        bool ValidateHardwareId(const std::string& hardwareId);

        void CleanupExpiredSessions();
        void CleanupExpiredBans();
        void CleanupOldLoginAttempts();

        void InitializeDefaultPermissions();
        void ApplyDefaultPermissions(PlayerSession* session);

        void NotifyPlayerAuthenticated(uint32_t playerId);
        void NotifyPlayerLoggedOut(uint32_t playerId);
        void NotifyPlayerBanned(uint32_t playerId, const std::string& reason);
        void NotifyPermissionChanged(uint32_t playerId, PlayerPermissionLevel level);

        std::string GenerateBanId();
        void AddBanToIndices(const BanRecord& ban);
        void RemoveBanFromIndices(const BanRecord& ban);

        bool HasAdminPermission(uint32_t adminId, uint32_t targetId, const std::string& action) const;
        PlayerPermissionLevel GetMinimumLevelForAction(const std::string& action) const;
    };

    // Permission Manager - manages detailed permission system
    class PermissionManager
    {
    public:
        static PermissionManager& GetInstance();

        // Permission groups
        struct PermissionGroup
        {
            std::string name;
            std::string description;
            std::vector<std::string> permissions;
            PlayerPermissionLevel defaultLevel;
        };

        // Group management
        bool CreatePermissionGroup(const PermissionGroup& group);
        bool DeletePermissionGroup(const std::string& groupName);
        bool AddPermissionToGroup(const std::string& groupName, const std::string& permission);
        bool RemovePermissionFromGroup(const std::string& groupName, const std::string& permission);

        // Player group assignment
        bool AssignPlayerToGroup(uint32_t playerId, const std::string& groupName);
        bool RemovePlayerFromGroup(uint32_t playerId, const std::string& groupName);
        std::vector<std::string> GetPlayerGroups(uint32_t playerId) const;

        // Permission inheritance
        std::vector<std::string> GetEffectivePermissions(uint32_t playerId) const;
        bool HasEffectivePermission(uint32_t playerId, const std::string& permission) const;

        // Permission templates
        void CreateAdminTemplate();
        void CreateModeratorTemplate();
        void CreateVIPTemplate();
        void ApplyPermissionTemplate(uint32_t playerId, const std::string& templateName);

    private:
        PermissionManager() = default;
        ~PermissionManager() = default;

        std::unordered_map<std::string, PermissionGroup> m_groups;
        std::unordered_map<uint32_t, std::vector<std::string>> m_playerGroups;
        mutable std::shared_mutex m_groupsMutex;
    };

    // Two-Factor Authentication Manager
    class TwoFactorAuthManager
    {
    public:
        static TwoFactorAuthManager& GetInstance();

        // 2FA Setup
        bool EnableTwoFactorAuth(uint32_t playerId, const std::string& secret);
        bool DisableTwoFactorAuth(uint32_t playerId, const std::string& verificationCode);
        bool IsTwoFactorEnabled(uint32_t playerId) const;

        // 2FA Verification
        bool VerifyTwoFactorCode(uint32_t playerId, const std::string& code);
        std::string GenerateBackupCodes(uint32_t playerId);
        bool VerifyBackupCode(uint32_t playerId, const std::string& backupCode);

        // Recovery
        void InitiateRecovery(uint32_t playerId);
        bool CompleteRecovery(uint32_t playerId, const std::string& recoveryToken);

    private:
        TwoFactorAuthManager() = default;
        ~TwoFactorAuthManager() = default;

        struct TwoFactorData
        {
            std::string secret;
            std::vector<std::string> backupCodes;
            std::vector<std::string> usedBackupCodes;
            bool isEnabled;
        };

        std::unordered_map<uint32_t, TwoFactorData> m_twoFactorData;
        mutable std::shared_mutex m_twoFactorMutex;

        std::string GenerateTOTPCode(const std::string& secret) const;
        bool ValidateTOTPCode(const std::string& secret, const std::string& code) const;
    };

    // Utility structures for network communication
    struct AuthenticationRequest
    {
        std::string playerName;
        std::string authToken;
        AuthenticationMethod method;
        std::string hardwareId;
        std::string clientVersion;
        std::string twoFactorCode;
        std::chrono::steady_clock::time_point timestamp;
    };

    struct AuthenticationResponse
    {
        AuthenticationResult result;
        uint32_t playerId;
        std::string sessionToken;
        PlayerPermissionLevel permissionLevel;
        std::vector<std::string> permissions;
        std::chrono::steady_clock::time_point sessionExpiry;
        std::string errorMessage;
    };

    struct PermissionUpdateData
    {
        uint32_t playerId;
        PlayerPermissionLevel newLevel;
        std::vector<std::string> grantedPermissions;
        std::vector<std::string> revokedPermissions;
        uint32_t updatedByAdminId;
        std::chrono::steady_clock::time_point timestamp;
    };

    struct BanNotificationData
    {
        uint32_t playerId;
        std::string playerName;
        std::string banId;
        BanType banType;
        std::string reason;
        uint32_t bannedById;
        std::string bannedByName;
        std::chrono::steady_clock::time_point expiryTime;
    };

    // Utility functions
    namespace AuthUtils
    {
        std::string PermissionLevelToString(PlayerPermissionLevel level);
        PlayerPermissionLevel StringToPermissionLevel(const std::string& levelStr);
        std::string AuthMethodToString(AuthenticationMethod method);
        AuthenticationMethod StringToAuthMethod(const std::string& methodStr);
        std::string BanTypeToString(BanType type);
        BanType StringToBanType(const std::string& typeStr);

        bool ValidatePlayerName(const std::string& name);
        bool ValidateSessionToken(const std::string& token);
        std::string HashPassword(const std::string& password, const std::string& salt);
        std::string GenerateSalt();
        bool VerifyPassword(const std::string& password, const std::string& hash, const std::string& salt);

        std::string GenerateSecureToken(size_t length = 32);
        uint32_t CalculatePermissionHash(const std::vector<std::string>& permissions);
    }
}