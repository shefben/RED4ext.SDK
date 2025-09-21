#pragma once

#include "RED4ext/Api/Sdk.hpp"
#include "RED4ext/Common.hpp"
#include "RED4ext/NativeTypes.hpp"
#include <unordered_map>
#include <vector>
#include <mutex>
#include <chrono>
#include <functional>
#include <string>

namespace RED4ext
{
    // Room types and configurations
    enum class RoomType : uint8_t
    {
        FreeRoam = 0,
        CooperativeMission = 1,
        CompetitiveMatch = 2,
        PrivateLobby = 3,
        CustomGameMode = 4
    };

    enum class RoomState : uint8_t
    {
        Waiting = 0,
        Starting = 1,
        InProgress = 2,
        Paused = 3,
        Completed = 4,
        Cancelled = 5
    };

    enum class RoomJoinResult : uint8_t
    {
        Success = 0,
        RoomFull = 1,
        PasswordRequired = 2,
        IncorrectPassword = 3,
        Banned = 4,
        InviteRequired = 5,
        VersionMismatch = 6,
        RoomNotFound = 7,
        AlreadyInRoom = 8,
        NetworkError = 9
    };

    enum class RoomPermissionLevel : uint8_t
    {
        Banned = 0,
        Viewer = 1,
        Player = 2,
        Moderator = 3,
        Admin = 4,
        Owner = 5
    };

    // Room settings structure
    struct RoomSettings
    {
        std::string name;
        std::string description;
        std::string password;
        RoomType roomType;
        uint32_t maxPlayers;
        bool isPasswordProtected;
        bool isPrivate;
        bool allowSpectators;
        bool enableVoiceChat;
        bool enableTextChat;
        float gameplayDifficulty;
        std::string gameMode;
        std::string mapName;
        std::vector<std::string> allowedMods;
        std::unordered_map<std::string, std::string> customSettings;

        RoomSettings()
            : maxPlayers(8), isPasswordProtected(false), isPrivate(false),
              allowSpectators(true), enableVoiceChat(true), enableTextChat(true),
              gameplayDifficulty(1.0f), roomType(RoomType::FreeRoam) {}
    };

    // Player info in room
    struct RoomPlayer
    {
        uint32_t playerId;
        std::string playerName;
        RoomPermissionLevel permissionLevel;
        bool isReady;
        bool isSpectator;
        std::chrono::steady_clock::time_point joinTime;
        std::chrono::steady_clock::time_point lastActivity;
        std::string userAgent; // Game version, mods, etc.

        RoomPlayer() = default;
        RoomPlayer(uint32_t id, const std::string& name)
            : playerId(id), playerName(name), permissionLevel(RoomPermissionLevel::Player),
              isReady(false), isSpectator(false),
              joinTime(std::chrono::steady_clock::now()),
              lastActivity(std::chrono::steady_clock::now()) {}
    };

    // Room invite system
    struct RoomInvite
    {
        std::string inviteId;
        std::string roomId;
        uint32_t inviterId;
        uint32_t inviteeId;
        std::chrono::steady_clock::time_point expirationTime;
        bool isUsed;

        RoomInvite() = default;
        RoomInvite(const std::string& room, uint32_t inviter, uint32_t invitee)
            : roomId(room), inviterId(inviter), inviteeId(invitee), isUsed(false)
        {
            // Generate invite ID
            inviteId = "invite_" + std::to_string(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count());

            // Set expiration (24 hours from now)
            expirationTime = std::chrono::steady_clock::now() + std::chrono::hours(24);
        }
    };

    // Main Room class
    class Room
    {
    public:
        Room() = default;
        Room(const std::string& id, uint32_t ownerId, const RoomSettings& settings);
        ~Room() = default;

        // Room management
        void Initialize(const std::string& id, uint32_t ownerId, const RoomSettings& settings);
        void Update();
        void Shutdown();

        // Player management
        RoomJoinResult AddPlayer(uint32_t playerId, const std::string& playerName,
                                const std::string& password = "", const std::string& inviteId = "");
        bool RemovePlayer(uint32_t playerId, bool kicked = false);
        bool SetPlayerReady(uint32_t playerId, bool ready);
        bool SetPlayerSpectator(uint32_t playerId, bool spectator);
        bool SetPlayerPermission(uint32_t playerId, RoomPermissionLevel level);

        // Room state management
        bool StartRoom();
        bool PauseRoom();
        bool ResumeRoom();
        bool EndRoom();
        void UpdateSettings(const RoomSettings& newSettings);

        // Invite system
        std::string CreateInvite(uint32_t inviterId, uint32_t inviteeId);
        bool AcceptInvite(const std::string& inviteId, uint32_t playerId);
        bool DeclineInvite(const std::string& inviteId);
        void CleanupExpiredInvites();

        // Chat and communication
        bool SendChatMessage(uint32_t senderId, const std::string& message, uint32_t targetId = 0);
        bool KickPlayer(uint32_t adminId, uint32_t targetId, const std::string& reason = "");
        bool BanPlayer(uint32_t adminId, uint32_t targetId, const std::string& reason = "", uint32_t durationMinutes = 0);

        // Getters
        const std::string& GetId() const { return m_roomId; }
        const RoomSettings& GetSettings() const { return m_settings; }
        RoomState GetState() const { return m_state; }
        uint32_t GetOwnerId() const { return m_ownerId; }
        uint32_t GetPlayerCount() const { return static_cast<uint32_t>(m_players.size()); }
        uint32_t GetSpectatorCount() const;
        uint32_t GetReadyPlayerCount() const;
        bool IsPlayerInRoom(uint32_t playerId) const;
        RoomPlayer* FindPlayer(uint32_t playerId);
        const RoomPlayer* FindPlayer(uint32_t playerId) const;
        std::vector<RoomPlayer> GetPlayers() const;
        std::vector<RoomPlayer> GetSpectators() const;
        bool CanPlayerJoin(uint32_t playerId, const std::string& password = "") const;
        bool HasPermission(uint32_t playerId, RoomPermissionLevel requiredLevel) const;

        // Room statistics
        std::chrono::steady_clock::time_point GetCreationTime() const { return m_creationTime; }
        std::chrono::steady_clock::time_point GetStartTime() const { return m_startTime; }
        std::chrono::milliseconds GetUptime() const;
        std::chrono::milliseconds GetSessionDuration() const;

        // Serialization
        std::string SerializeState() const;
        bool DeserializeState(const std::string& data);

    private:
        std::string m_roomId;
        uint32_t m_ownerId;
        RoomSettings m_settings;
        RoomState m_state;

        std::vector<RoomPlayer> m_players;
        std::vector<uint32_t> m_bannedPlayers;
        std::vector<RoomInvite> m_invites;

        std::chrono::steady_clock::time_point m_creationTime;
        std::chrono::steady_clock::time_point m_startTime;
        std::chrono::steady_clock::time_point m_lastUpdate;

        mutable std::shared_mutex m_playersMutex;
        mutable std::mutex m_invitesMutex;
        mutable std::mutex m_settingsMutex;

        // Internal methods
        void ValidateSettings();
        bool IsPlayerBanned(uint32_t playerId) const;
        void UpdatePlayerActivity(uint32_t playerId);
        void NotifyPlayersRoomUpdate();
        void NotifyPlayerJoined(uint32_t playerId);
        void NotifyPlayerLeft(uint32_t playerId);
        void NotifyRoomStateChanged(RoomState newState);
        std::string GenerateRoomCode() const;
        bool ValidatePassword(const std::string& password) const;
    };

    // Room Manager - manages all rooms
    class RoomManager
    {
    public:
        static RoomManager& GetInstance();

        // Lifecycle
        void Initialize();
        void Shutdown();
        void Update();

        // Room creation and management
        std::string CreateRoom(uint32_t ownerId, const RoomSettings& settings);
        bool DestroyRoom(const std::string& roomId, uint32_t requesterId);
        Room* FindRoom(const std::string& roomId);
        const Room* FindRoom(const std::string& roomId) const;

        // Room discovery
        std::vector<std::string> GetPublicRooms() const;
        std::vector<std::string> GetRoomsByType(RoomType type) const;
        std::vector<std::string> FindRoomsByName(const std::string& nameFilter) const;
        std::vector<std::string> GetPlayerRooms(uint32_t playerId) const;

        // Player room management
        std::string GetPlayerCurrentRoom(uint32_t playerId) const;
        RoomJoinResult JoinRoom(uint32_t playerId, const std::string& roomId,
                               const std::string& playerName, const std::string& password = "",
                               const std::string& inviteId = "");
        bool LeaveRoom(uint32_t playerId);
        bool LeaveRoom(uint32_t playerId, const std::string& roomId);

        // Quick join functionality
        std::string QuickJoinRoom(uint32_t playerId, const std::string& playerName, RoomType preferredType = RoomType::FreeRoam);
        std::string CreateAndJoinRoom(uint32_t playerId, const std::string& playerName, const RoomSettings& settings);

        // Room statistics
        uint32_t GetTotalRoomCount() const;
        uint32_t GetActiveRoomCount() const;
        uint32_t GetTotalPlayerCount() const;
        std::unordered_map<RoomType, uint32_t> GetRoomCountByType() const;

        // Events and callbacks
        using RoomCreatedCallback = std::function<void(const std::string& roomId)>;
        using RoomDestroyedCallback = std::function<void(const std::string& roomId)>;
        using PlayerJoinedRoomCallback = std::function<void(const std::string& roomId, uint32_t playerId)>;
        using PlayerLeftRoomCallback = std::function<void(const std::string& roomId, uint32_t playerId)>;

        void SetRoomCreatedCallback(RoomCreatedCallback callback);
        void SetRoomDestroyedCallback(RoomDestroyedCallback callback);
        void SetPlayerJoinedRoomCallback(PlayerJoinedRoomCallback callback);
        void SetPlayerLeftRoomCallback(PlayerLeftRoomCallback callback);

        // Configuration
        void SetMaxRoomsPerPlayer(uint32_t maxRooms);
        void SetMaxTotalRooms(uint32_t maxRooms);
        void SetRoomCleanupInterval(uint32_t seconds);
        void EnableRoomPersistence(bool enabled);

        // Maintenance
        void CleanupEmptyRooms();
        void CleanupExpiredInvites();
        void SaveRoomStates();
        void LoadRoomStates();

        // Admin functions
        std::vector<std::string> GetAllRooms() const;
        bool ForceDestroyRoom(const std::string& roomId);
        bool TransferRoomOwnership(const std::string& roomId, uint32_t newOwnerId);

    private:
        RoomManager() = default;
        ~RoomManager() = default;
        RoomManager(const RoomManager&) = delete;
        RoomManager& operator=(const RoomManager&) = delete;

        std::unordered_map<std::string, std::unique_ptr<Room>> m_rooms;
        std::unordered_map<uint32_t, std::string> m_playerToRoom; // Player ID -> Room ID

        mutable std::shared_mutex m_roomsMutex;
        mutable std::mutex m_playerRoomMutex;
        mutable std::mutex m_configMutex;

        // Configuration
        uint32_t m_maxRoomsPerPlayer = 3;
        uint32_t m_maxTotalRooms = 1000;
        uint32_t m_roomCleanupInterval = 300; // 5 minutes
        bool m_roomPersistenceEnabled = false;

        // Statistics
        uint32_t m_totalRoomsCreated = 0;
        std::chrono::steady_clock::time_point m_lastCleanup;

        // Callbacks
        RoomCreatedCallback m_roomCreatedCallback;
        RoomDestroyedCallback m_roomDestroyedCallback;
        PlayerJoinedRoomCallback m_playerJoinedRoomCallback;
        PlayerLeftRoomCallback m_playerLeftRoomCallback;

        // Internal methods
        std::string GenerateRoomId();
        void NotifyRoomCreated(const std::string& roomId);
        void NotifyRoomDestroyed(const std::string& roomId);
        void NotifyPlayerJoinedRoom(const std::string& roomId, uint32_t playerId);
        void NotifyPlayerLeftRoom(const std::string& roomId, uint32_t playerId);
        bool ValidateRoomSettings(const RoomSettings& settings);
        uint32_t GetPlayerRoomCount(uint32_t playerId) const;
        void RemovePlayerFromAllRooms(uint32_t playerId);
    };

    // Room browser for discovering rooms
    class RoomBrowser
    {
    public:
        struct RoomListEntry
        {
            std::string roomId;
            std::string name;
            std::string description;
            RoomType type;
            uint32_t currentPlayers;
            uint32_t maxPlayers;
            bool isPasswordProtected;
            bool isPrivate;
            std::string gameMode;
            std::string mapName;
            uint32_t ping;
            std::vector<std::string> tags;
        };

        struct RoomFilters
        {
            std::string nameFilter;
            RoomType roomType = static_cast<RoomType>(255); // All types
            std::string gameModeFilter;
            uint32_t maxPing = 999;
            bool showPasswordProtected = true;
            bool showFull = false;
            bool showEmpty = true;
            uint32_t minPlayers = 0;
            uint32_t maxPlayers = 999;
            std::vector<std::string> requiredTags;
            std::vector<std::string> excludedTags;
        };

        static RoomBrowser& GetInstance();

        void Initialize();
        void Shutdown();
        void Update();

        // Room discovery
        void RefreshRoomList();
        std::vector<RoomListEntry> GetRoomList() const;
        std::vector<RoomListEntry> GetFilteredRoomList(const RoomFilters& filters) const;

        // Filtering and sorting
        void SetFilters(const RoomFilters& filters);
        void ClearFilters();
        void SortRooms(const std::string& criteria, bool ascending = true);

        // Room selection
        void SelectRoom(const std::string& roomId);
        RoomListEntry GetSelectedRoom() const;
        bool JoinSelectedRoom(uint32_t playerId, const std::string& playerName, const std::string& password = "");

        // Statistics
        uint32_t GetTotalRoomsFound() const;
        uint32_t GetFilteredRoomsCount() const;
        bool IsRefreshing() const;

    private:
        RoomBrowser() = default;
        ~RoomBrowser() = default;

        std::vector<RoomListEntry> m_roomList;
        std::vector<RoomListEntry> m_filteredRoomList;
        RoomFilters m_currentFilters;
        std::string m_selectedRoomId;
        bool m_isRefreshing = false;

        mutable std::shared_mutex m_roomListMutex;

        void ApplyFilters();
        bool MatchesFilters(const RoomListEntry& room, const RoomFilters& filters) const;
        void UpdateRoomPings();
    };

    // Utility structures for network communication
    struct RoomCreateData
    {
        uint32_t ownerId;
        RoomSettings settings;
        std::chrono::steady_clock::time_point timestamp;
    };

    struct RoomJoinData
    {
        uint32_t playerId;
        std::string playerName;
        std::string roomId;
        std::string password;
        std::string inviteId;
        std::chrono::steady_clock::time_point timestamp;
    };

    struct RoomLeaveData
    {
        uint32_t playerId;
        std::string roomId;
        bool wasKicked;
        std::string reason;
        std::chrono::steady_clock::time_point timestamp;
    };

    struct RoomStateUpdateData
    {
        std::string roomId;
        RoomState newState;
        std::vector<RoomPlayer> players;
        RoomSettings settings;
        std::chrono::steady_clock::time_point timestamp;
    };

    struct RoomChatData
    {
        std::string roomId;
        uint32_t senderId;
        std::string senderName;
        std::string message;
        uint32_t targetId; // 0 for broadcast
        std::chrono::steady_clock::time_point timestamp;
    };

    // Utility functions
    namespace RoomUtils
    {
        std::string RoomTypeToString(RoomType type);
        RoomType StringToRoomType(const std::string& typeStr);
        std::string RoomStateToString(RoomState state);
        std::string PermissionLevelToString(RoomPermissionLevel level);
        bool ValidateRoomName(const std::string& name);
        bool ValidateRoomPassword(const std::string& password);
        std::string GenerateRoomCode(uint32_t length = 6);
        uint32_t EstimateRoomPing(const std::string& roomId);
    }
}