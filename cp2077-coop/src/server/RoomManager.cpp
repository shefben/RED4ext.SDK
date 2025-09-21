#include "RoomManager.hpp"
#include "RED4ext/Api/Sdk.hpp"
#include <algorithm>
#include <random>
#include <sstream>
#include <regex>

namespace RED4ext
{
    // Room Implementation
    Room::Room(const std::string& id, uint32_t ownerId, const RoomSettings& settings)
    {
        Initialize(id, ownerId, settings);
    }

    void Room::Initialize(const std::string& id, uint32_t ownerId, const RoomSettings& settings)
    {
        m_roomId = id;
        m_ownerId = ownerId;
        m_settings = settings;
        m_state = RoomState::Waiting;

        ValidateSettings();

        m_creationTime = std::chrono::steady_clock::now();
        m_startTime = {};
        m_lastUpdate = m_creationTime;

        m_players.clear();
        m_bannedPlayers.clear();
        m_invites.clear();
    }

    void Room::Update()
    {
        m_lastUpdate = std::chrono::steady_clock::now();

        // Clean up expired invites
        CleanupExpiredInvites();

        // Update player activity timeouts
        std::unique_lock<std::shared_mutex> lock(m_playersMutex);
        auto currentTime = std::chrono::steady_clock::now();

        for (auto& player : m_players) {
            auto inactiveTime = std::chrono::duration_cast<std::chrono::minutes>(
                currentTime - player.lastActivity).count();

            // Kick players inactive for more than 30 minutes
            if (inactiveTime >= 30) {
                // Mark for removal (would be handled in next update cycle)
                // This prevents iterator invalidation
            }
        }
    }

    void Room::Shutdown()
    {
        std::unique_lock<std::shared_mutex> playersLock(m_playersMutex);
        std::lock_guard<std::mutex> invitesLock(m_invitesMutex);

        // Notify all players of room shutdown
        for (const auto& player : m_players) {
            NotifyPlayerLeft(player.playerId);
        }

        m_players.clear();
        m_bannedPlayers.clear();
        m_invites.clear();

        m_state = RoomState::Cancelled;
    }

    RoomJoinResult Room::AddPlayer(uint32_t playerId, const std::string& playerName,
                                  const std::string& password, const std::string& inviteId)
    {
        // Check if player is banned
        if (IsPlayerBanned(playerId)) {
            return RoomJoinResult::Banned;
        }

        // Check room capacity
        if (GetPlayerCount() >= m_settings.maxPlayers) {
            return RoomJoinResult::RoomFull;
        }

        // Check if already in room
        if (IsPlayerInRoom(playerId)) {
            return RoomJoinResult::AlreadyInRoom;
        }

        // Validate password if required
        if (m_settings.isPasswordProtected) {
            if (password.empty()) {
                return RoomJoinResult::PasswordRequired;
            }
            if (!ValidatePassword(password)) {
                return RoomJoinResult::IncorrectPassword;
            }
        }

        // Handle invite-only rooms
        if (m_settings.isPrivate && !inviteId.empty()) {
            bool validInvite = false;
            {
                std::lock_guard<std::mutex> lock(m_invitesMutex);
                for (auto& invite : m_invites) {
                    if (invite.inviteId == inviteId &&
                        invite.inviteeId == playerId &&
                        !invite.isUsed &&
                        std::chrono::steady_clock::now() < invite.expirationTime) {

                        invite.isUsed = true;
                        validInvite = true;
                        break;
                    }
                }
            }

            if (!validInvite) {
                return RoomJoinResult::InviteRequired;
            }
        }

        // Add player to room
        {
            std::unique_lock<std::shared_mutex> lock(m_playersMutex);

            RoomPlayer newPlayer(playerId, playerName);

            // Set owner permission if this is the owner
            if (playerId == m_ownerId) {
                newPlayer.permissionLevel = RoomPermissionLevel::Owner;
            }

            m_players.push_back(newPlayer);
        }

        UpdatePlayerActivity(playerId);
        NotifyPlayerJoined(playerId);

        return RoomJoinResult::Success;
    }

    bool Room::RemovePlayer(uint32_t playerId, bool kicked)
    {
        std::unique_lock<std::shared_mutex> lock(m_playersMutex);

        auto it = std::find_if(m_players.begin(), m_players.end(),
            [playerId](const RoomPlayer& player) {
                return player.playerId == playerId;
            });

        if (it == m_players.end()) {
            return false;
        }

        m_players.erase(it);
        NotifyPlayerLeft(playerId);

        // If owner left and room has other players, transfer ownership
        if (playerId == m_ownerId && !m_players.empty()) {
            // Find player with highest permission level
            auto newOwner = std::max_element(m_players.begin(), m_players.end(),
                [](const RoomPlayer& a, const RoomPlayer& b) {
                    return static_cast<int>(a.permissionLevel) < static_cast<int>(b.permissionLevel);
                });

            if (newOwner != m_players.end()) {
                m_ownerId = newOwner->playerId;
                newOwner->permissionLevel = RoomPermissionLevel::Owner;
            }
        }

        return true;
    }

    bool Room::SetPlayerReady(uint32_t playerId, bool ready)
    {
        std::unique_lock<std::shared_mutex> lock(m_playersMutex);

        auto* player = FindPlayer(playerId);
        if (!player) {
            return false;
        }

        player->isReady = ready;
        UpdatePlayerActivity(playerId);
        NotifyPlayersRoomUpdate();

        return true;
    }

    bool Room::SetPlayerSpectator(uint32_t playerId, bool spectator)
    {
        if (!m_settings.allowSpectators && spectator) {
            return false;
        }

        std::unique_lock<std::shared_mutex> lock(m_playersMutex);

        auto* player = FindPlayer(playerId);
        if (!player) {
            return false;
        }

        player->isSpectator = spectator;
        UpdatePlayerActivity(playerId);
        NotifyPlayersRoomUpdate();

        return true;
    }

    bool Room::SetPlayerPermission(uint32_t playerId, RoomPermissionLevel level)
    {
        std::unique_lock<std::shared_mutex> lock(m_playersMutex);

        auto* player = FindPlayer(playerId);
        if (!player || player->playerId == m_ownerId) {
            return false; // Can't change owner's permission
        }

        player->permissionLevel = level;
        NotifyPlayersRoomUpdate();

        return true;
    }

    bool Room::StartRoom()
    {
        if (m_state != RoomState::Waiting) {
            return false;
        }

        // Check if all players are ready
        {
            std::shared_lock<std::shared_mutex> lock(m_playersMutex);
            for (const auto& player : m_players) {
                if (!player.isSpectator && !player.isReady) {
                    return false; // Not all players ready
                }
            }
        }

        m_state = RoomState::Starting;
        m_startTime = std::chrono::steady_clock::now();

        NotifyRoomStateChanged(m_state);

        // Transition to InProgress after brief starting period
        std::this_thread::sleep_for(std::chrono::seconds(3));
        m_state = RoomState::InProgress;
        NotifyRoomStateChanged(m_state);

        return true;
    }

    bool Room::PauseRoom()
    {
        if (m_state != RoomState::InProgress) {
            return false;
        }

        m_state = RoomState::Paused;
        NotifyRoomStateChanged(m_state);

        return true;
    }

    bool Room::ResumeRoom()
    {
        if (m_state != RoomState::Paused) {
            return false;
        }

        m_state = RoomState::InProgress;
        NotifyRoomStateChanged(m_state);

        return true;
    }

    bool Room::EndRoom()
    {
        if (m_state != RoomState::InProgress && m_state != RoomState::Paused) {
            return false;
        }

        m_state = RoomState::Completed;
        NotifyRoomStateChanged(m_state);

        return true;
    }

    void Room::UpdateSettings(const RoomSettings& newSettings)
    {
        std::lock_guard<std::mutex> lock(m_settingsMutex);

        RoomSettings oldSettings = m_settings;
        m_settings = newSettings;
        ValidateSettings();

        // Handle player count reduction
        if (newSettings.maxPlayers < oldSettings.maxPlayers) {
            std::unique_lock<std::shared_mutex> playersLock(m_playersMutex);

            while (m_players.size() > newSettings.maxPlayers) {
                // Remove last joined player
                auto lastPlayer = std::min_element(m_players.begin(), m_players.end(),
                    [](const RoomPlayer& a, const RoomPlayer& b) {
                        return a.joinTime > b.joinTime; // Most recently joined
                    });

                if (lastPlayer != m_players.end() && lastPlayer->playerId != m_ownerId) {
                    uint32_t playerId = lastPlayer->playerId;
                    m_players.erase(lastPlayer);
                    NotifyPlayerLeft(playerId);
                }
            }
        }

        NotifyPlayersRoomUpdate();
    }

    std::string Room::CreateInvite(uint32_t inviterId, uint32_t inviteeId)
    {
        // Verify inviter has permission
        if (!HasPermission(inviterId, RoomPermissionLevel::Player)) {
            return "";
        }

        std::lock_guard<std::mutex> lock(m_invitesMutex);

        RoomInvite invite(m_roomId, inviterId, inviteeId);
        std::string inviteId = invite.inviteId;

        m_invites.push_back(invite);

        return inviteId;
    }

    bool Room::AcceptInvite(const std::string& inviteId, uint32_t playerId)
    {
        std::lock_guard<std::mutex> lock(m_invitesMutex);

        for (auto& invite : m_invites) {
            if (invite.inviteId == inviteId &&
                invite.inviteeId == playerId &&
                !invite.isUsed &&
                std::chrono::steady_clock::now() < invite.expirationTime) {

                invite.isUsed = true;
                return true;
            }
        }

        return false;
    }

    bool Room::DeclineInvite(const std::string& inviteId)
    {
        std::lock_guard<std::mutex> lock(m_invitesMutex);

        auto it = std::find_if(m_invites.begin(), m_invites.end(),
            [&inviteId](const RoomInvite& invite) {
                return invite.inviteId == inviteId;
            });

        if (it != m_invites.end()) {
            m_invites.erase(it);
            return true;
        }

        return false;
    }

    void Room::CleanupExpiredInvites()
    {
        std::lock_guard<std::mutex> lock(m_invitesMutex);

        auto currentTime = std::chrono::steady_clock::now();

        m_invites.erase(
            std::remove_if(m_invites.begin(), m_invites.end(),
                [currentTime](const RoomInvite& invite) {
                    return invite.isUsed || currentTime >= invite.expirationTime;
                }),
            m_invites.end());
    }

    bool Room::SendChatMessage(uint32_t senderId, const std::string& message, uint32_t targetId)
    {
        if (!HasPermission(senderId, RoomPermissionLevel::Player)) {
            return false;
        }

        if (!m_settings.enableTextChat) {
            return false;
        }

        // Validate message
        if (message.empty() || message.length() > 500) {
            return false;
        }

        // TODO: Implement chat message broadcasting
        return true;
    }

    bool Room::KickPlayer(uint32_t adminId, uint32_t targetId, const std::string& reason)
    {
        if (!HasPermission(adminId, RoomPermissionLevel::Moderator)) {
            return false;
        }

        if (targetId == m_ownerId) {
            return false; // Can't kick owner
        }

        return RemovePlayer(targetId, true);
    }

    bool Room::BanPlayer(uint32_t adminId, uint32_t targetId, const std::string& reason, uint32_t durationMinutes)
    {
        if (!HasPermission(adminId, RoomPermissionLevel::Admin)) {
            return false;
        }

        if (targetId == m_ownerId) {
            return false; // Can't ban owner
        }

        // Add to banned list
        m_bannedPlayers.push_back(targetId);

        // Remove from room
        RemovePlayer(targetId, true);

        return true;
    }

    uint32_t Room::GetSpectatorCount() const
    {
        std::shared_lock<std::shared_mutex> lock(m_playersMutex);

        return static_cast<uint32_t>(
            std::count_if(m_players.begin(), m_players.end(),
                [](const RoomPlayer& player) {
                    return player.isSpectator;
                }));
    }

    uint32_t Room::GetReadyPlayerCount() const
    {
        std::shared_lock<std::shared_mutex> lock(m_playersMutex);

        return static_cast<uint32_t>(
            std::count_if(m_players.begin(), m_players.end(),
                [](const RoomPlayer& player) {
                    return player.isReady && !player.isSpectator;
                }));
    }

    bool Room::IsPlayerInRoom(uint32_t playerId) const
    {
        return FindPlayer(playerId) != nullptr;
    }

    RoomPlayer* Room::FindPlayer(uint32_t playerId)
    {
        auto it = std::find_if(m_players.begin(), m_players.end(),
            [playerId](const RoomPlayer& player) {
                return player.playerId == playerId;
            });

        return (it != m_players.end()) ? &(*it) : nullptr;
    }

    const RoomPlayer* Room::FindPlayer(uint32_t playerId) const
    {
        auto it = std::find_if(m_players.begin(), m_players.end(),
            [playerId](const RoomPlayer& player) {
                return player.playerId == playerId;
            });

        return (it != m_players.end()) ? &(*it) : nullptr;
    }

    std::vector<RoomPlayer> Room::GetPlayers() const
    {
        std::shared_lock<std::shared_mutex> lock(m_playersMutex);
        return m_players;
    }

    std::vector<RoomPlayer> Room::GetSpectators() const
    {
        std::shared_lock<std::shared_mutex> lock(m_playersMutex);

        std::vector<RoomPlayer> spectators;
        std::copy_if(m_players.begin(), m_players.end(),
                    std::back_inserter(spectators),
                    [](const RoomPlayer& player) {
                        return player.isSpectator;
                    });

        return spectators;
    }

    bool Room::CanPlayerJoin(uint32_t playerId, const std::string& password) const
    {
        if (IsPlayerBanned(playerId)) {
            return false;
        }

        if (GetPlayerCount() >= m_settings.maxPlayers) {
            return false;
        }

        if (IsPlayerInRoom(playerId)) {
            return false;
        }

        if (m_settings.isPasswordProtected && !ValidatePassword(password)) {
            return false;
        }

        return true;
    }

    bool Room::HasPermission(uint32_t playerId, RoomPermissionLevel requiredLevel) const
    {
        auto* player = FindPlayer(playerId);
        if (!player) {
            return false;
        }

        return static_cast<int>(player->permissionLevel) >= static_cast<int>(requiredLevel);
    }

    std::chrono::milliseconds Room::GetUptime() const
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - m_creationTime);
    }

    std::chrono::milliseconds Room::GetSessionDuration() const
    {
        if (m_startTime.time_since_epoch().count() == 0) {
            return std::chrono::milliseconds(0);
        }

        auto endTime = (m_state == RoomState::Completed || m_state == RoomState::Cancelled)
            ? m_lastUpdate : std::chrono::steady_clock::now();

        return std::chrono::duration_cast<std::chrono::milliseconds>(endTime - m_startTime);
    }

    // Private methods implementation
    void Room::ValidateSettings()
    {
        if (m_settings.maxPlayers == 0) {
            m_settings.maxPlayers = 1;
        }
        if (m_settings.maxPlayers > 50) {
            m_settings.maxPlayers = 50;
        }

        if (m_settings.gameplayDifficulty < 0.1f) {
            m_settings.gameplayDifficulty = 0.1f;
        }
        if (m_settings.gameplayDifficulty > 3.0f) {
            m_settings.gameplayDifficulty = 3.0f;
        }
    }

    bool Room::IsPlayerBanned(uint32_t playerId) const
    {
        return std::find(m_bannedPlayers.begin(), m_bannedPlayers.end(), playerId)
               != m_bannedPlayers.end();
    }

    void Room::UpdatePlayerActivity(uint32_t playerId)
    {
        auto* player = FindPlayer(playerId);
        if (player) {
            player->lastActivity = std::chrono::steady_clock::now();
        }
    }

    void Room::NotifyPlayersRoomUpdate()
    {
        // TODO: Send room update to all players
    }

    void Room::NotifyPlayerJoined(uint32_t playerId)
    {
        // TODO: Send player joined notification
    }

    void Room::NotifyPlayerLeft(uint32_t playerId)
    {
        // TODO: Send player left notification
    }

    void Room::NotifyRoomStateChanged(RoomState newState)
    {
        // TODO: Send room state change notification
    }

    bool Room::ValidatePassword(const std::string& password) const
    {
        return password == m_settings.password;
    }

    // RoomManager Implementation
    RoomManager& RoomManager::GetInstance()
    {
        static RoomManager instance;
        return instance;
    }

    void RoomManager::Initialize()
    {
        std::unique_lock<std::shared_mutex> roomsLock(m_roomsMutex);
        std::lock_guard<std::mutex> playerRoomLock(m_playerRoomMutex);

        m_rooms.clear();
        m_playerToRoom.clear();

        m_totalRoomsCreated = 0;
        m_lastCleanup = std::chrono::steady_clock::now();

        // Load room states if persistence is enabled
        if (m_roomPersistenceEnabled) {
            LoadRoomStates();
        }
    }

    void RoomManager::Shutdown()
    {
        // Save room states if persistence is enabled
        if (m_roomPersistenceEnabled) {
            SaveRoomStates();
        }

        std::unique_lock<std::shared_mutex> roomsLock(m_roomsMutex);
        std::lock_guard<std::mutex> playerRoomLock(m_playerRoomMutex);

        // Shutdown all rooms
        for (auto& [roomId, room] : m_rooms) {
            room->Shutdown();
        }

        m_rooms.clear();
        m_playerToRoom.clear();

        // Clear callbacks
        m_roomCreatedCallback = nullptr;
        m_roomDestroyedCallback = nullptr;
        m_playerJoinedRoomCallback = nullptr;
        m_playerLeftRoomCallback = nullptr;
    }

    void RoomManager::Update()
    {
        auto currentTime = std::chrono::steady_clock::now();

        // Update all rooms
        {
            std::shared_lock<std::shared_mutex> lock(m_roomsMutex);
            for (auto& [roomId, room] : m_rooms) {
                room->Update();
            }
        }

        // Periodic cleanup
        auto timeSinceCleanup = std::chrono::duration_cast<std::chrono::seconds>(
            currentTime - m_lastCleanup).count();

        if (timeSinceCleanup >= m_roomCleanupInterval) {
            CleanupEmptyRooms();
            CleanupExpiredInvites();
            m_lastCleanup = currentTime;
        }
    }

    std::string RoomManager::CreateRoom(uint32_t ownerId, const RoomSettings& settings)
    {
        if (!ValidateRoomSettings(settings)) {
            return "";
        }

        // Check room limits
        if (GetPlayerRoomCount(ownerId) >= m_maxRoomsPerPlayer) {
            return ""; // Player has too many rooms
        }

        {
            std::shared_lock<std::shared_mutex> lock(m_roomsMutex);
            if (m_rooms.size() >= m_maxTotalRooms) {
                return ""; // Server room limit reached
            }
        }

        std::string roomId = GenerateRoomId();

        // Create room
        auto room = std::make_unique<Room>(roomId, ownerId, settings);

        {
            std::unique_lock<std::shared_mutex> roomsLock(m_roomsMutex);
            m_rooms[roomId] = std::move(room);
        }

        {
            std::lock_guard<std::mutex> playerRoomLock(m_playerRoomMutex);
            m_playerToRoom[ownerId] = roomId;
        }

        m_totalRoomsCreated++;
        NotifyRoomCreated(roomId);

        return roomId;
    }

    bool RoomManager::DestroyRoom(const std::string& roomId, uint32_t requesterId)
    {
        Room* room = FindRoom(roomId);
        if (!room) {
            return false;
        }

        // Check if requester has permission
        if (!room->HasPermission(requesterId, RoomPermissionLevel::Owner)) {
            return false;
        }

        // Get all players before destroying room
        auto players = room->GetPlayers();

        // Remove room
        {
            std::unique_lock<std::shared_mutex> roomsLock(m_roomsMutex);
            m_rooms.erase(roomId);
        }

        // Remove player mappings
        {
            std::lock_guard<std::mutex> playerRoomLock(m_playerRoomMutex);
            for (const auto& player : players) {
                auto it = m_playerToRoom.find(player.playerId);
                if (it != m_playerToRoom.end() && it->second == roomId) {
                    m_playerToRoom.erase(it);
                }
            }
        }

        NotifyRoomDestroyed(roomId);
        return true;
    }

    Room* RoomManager::FindRoom(const std::string& roomId)
    {
        std::shared_lock<std::shared_mutex> lock(m_roomsMutex);

        auto it = m_rooms.find(roomId);
        return (it != m_rooms.end()) ? it->second.get() : nullptr;
    }

    const Room* RoomManager::FindRoom(const std::string& roomId) const
    {
        std::shared_lock<std::shared_mutex> lock(m_roomsMutex);

        auto it = m_rooms.find(roomId);
        return (it != m_rooms.end()) ? it->second.get() : nullptr;
    }

    std::vector<std::string> RoomManager::GetPublicRooms() const
    {
        std::shared_lock<std::shared_mutex> lock(m_roomsMutex);

        std::vector<std::string> publicRooms;

        for (const auto& [roomId, room] : m_rooms) {
            if (!room->GetSettings().isPrivate) {
                publicRooms.push_back(roomId);
            }
        }

        return publicRooms;
    }

    std::vector<std::string> RoomManager::GetRoomsByType(RoomType type) const
    {
        std::shared_lock<std::shared_mutex> lock(m_roomsMutex);

        std::vector<std::string> roomsByType;

        for (const auto& [roomId, room] : m_rooms) {
            if (room->GetSettings().roomType == type) {
                roomsByType.push_back(roomId);
            }
        }

        return roomsByType;
    }

    std::vector<std::string> RoomManager::FindRoomsByName(const std::string& nameFilter) const
    {
        std::shared_lock<std::shared_mutex> lock(m_roomsMutex);

        std::vector<std::string> matchingRooms;
        std::string filterLower = nameFilter;
        std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);

        for (const auto& [roomId, room] : m_rooms) {
            std::string roomNameLower = room->GetSettings().name;
            std::transform(roomNameLower.begin(), roomNameLower.end(), roomNameLower.begin(), ::tolower);

            if (roomNameLower.find(filterLower) != std::string::npos) {
                matchingRooms.push_back(roomId);
            }
        }

        return matchingRooms;
    }

    std::vector<std::string> RoomManager::GetPlayerRooms(uint32_t playerId) const
    {
        std::shared_lock<std::shared_mutex> roomsLock(m_roomsMutex);

        std::vector<std::string> playerRooms;

        for (const auto& [roomId, room] : m_rooms) {
            if (room->IsPlayerInRoom(playerId)) {
                playerRooms.push_back(roomId);
            }
        }

        return playerRooms;
    }

    std::string RoomManager::GetPlayerCurrentRoom(uint32_t playerId) const
    {
        std::lock_guard<std::mutex> lock(m_playerRoomMutex);

        auto it = m_playerToRoom.find(playerId);
        return (it != m_playerToRoom.end()) ? it->second : "";
    }

    RoomJoinResult RoomManager::JoinRoom(uint32_t playerId, const std::string& roomId,
                                        const std::string& playerName, const std::string& password,
                                        const std::string& inviteId)
    {
        Room* room = FindRoom(roomId);
        if (!room) {
            return RoomJoinResult::RoomNotFound;
        }

        RoomJoinResult result = room->AddPlayer(playerId, playerName, password, inviteId);

        if (result == RoomJoinResult::Success) {
            {
                std::lock_guard<std::mutex> lock(m_playerRoomMutex);
                m_playerToRoom[playerId] = roomId;
            }

            NotifyPlayerJoinedRoom(roomId, playerId);
        }

        return result;
    }

    bool RoomManager::LeaveRoom(uint32_t playerId)
    {
        std::string roomId = GetPlayerCurrentRoom(playerId);
        if (roomId.empty()) {
            return false;
        }

        return LeaveRoom(playerId, roomId);
    }

    bool RoomManager::LeaveRoom(uint32_t playerId, const std::string& roomId)
    {
        Room* room = FindRoom(roomId);
        if (!room) {
            return false;
        }

        bool success = room->RemovePlayer(playerId);

        if (success) {
            {
                std::lock_guard<std::mutex> lock(m_playerRoomMutex);
                auto it = m_playerToRoom.find(playerId);
                if (it != m_playerToRoom.end() && it->second == roomId) {
                    m_playerToRoom.erase(it);
                }
            }

            NotifyPlayerLeftRoom(roomId, playerId);

            // Destroy room if empty and not persistent
            if (room->GetPlayerCount() == 0) {
                std::unique_lock<std::shared_mutex> lock(m_roomsMutex);
                m_rooms.erase(roomId);
                NotifyRoomDestroyed(roomId);
            }
        }

        return success;
    }

    std::string RoomManager::QuickJoinRoom(uint32_t playerId, const std::string& playerName, RoomType preferredType)
    {
        // Find suitable public room
        auto publicRooms = GetPublicRooms();

        for (const auto& roomId : publicRooms) {
            Room* room = FindRoom(roomId);
            if (room &&
                room->GetSettings().roomType == preferredType &&
                room->CanPlayerJoin(playerId) &&
                room->GetState() == RoomState::Waiting) {

                if (JoinRoom(playerId, roomId, playerName) == RoomJoinResult::Success) {
                    return roomId;
                }
            }
        }

        // No suitable room found, create new one
        RoomSettings settings;
        settings.name = playerName + "'s Room";
        settings.roomType = preferredType;
        settings.maxPlayers = 8;

        std::string newRoomId = CreateRoom(playerId, settings);
        if (!newRoomId.empty()) {
            JoinRoom(playerId, newRoomId, playerName);
        }

        return newRoomId;
    }

    std::string RoomManager::CreateAndJoinRoom(uint32_t playerId, const std::string& playerName, const RoomSettings& settings)
    {
        std::string roomId = CreateRoom(playerId, settings);
        if (!roomId.empty()) {
            JoinRoom(playerId, roomId, playerName);
        }
        return roomId;
    }

    uint32_t RoomManager::GetTotalRoomCount() const
    {
        std::shared_lock<std::shared_mutex> lock(m_roomsMutex);
        return static_cast<uint32_t>(m_rooms.size());
    }

    uint32_t RoomManager::GetActiveRoomCount() const
    {
        std::shared_lock<std::shared_mutex> lock(m_roomsMutex);

        uint32_t activeCount = 0;
        for (const auto& [roomId, room] : m_rooms) {
            if (room->GetState() == RoomState::InProgress) {
                activeCount++;
            }
        }

        return activeCount;
    }

    uint32_t RoomManager::GetTotalPlayerCount() const
    {
        std::shared_lock<std::shared_mutex> lock(m_roomsMutex);

        uint32_t totalPlayers = 0;
        for (const auto& [roomId, room] : m_rooms) {
            totalPlayers += room->GetPlayerCount();
        }

        return totalPlayers;
    }

    std::unordered_map<RoomType, uint32_t> RoomManager::GetRoomCountByType() const
    {
        std::shared_lock<std::shared_mutex> lock(m_roomsMutex);

        std::unordered_map<RoomType, uint32_t> counts;

        for (const auto& [roomId, room] : m_rooms) {
            RoomType type = room->GetSettings().roomType;
            counts[type]++;
        }

        return counts;
    }

    void RoomManager::CleanupEmptyRooms()
    {
        std::vector<std::string> emptyRooms;

        {
            std::shared_lock<std::shared_mutex> lock(m_roomsMutex);

            for (const auto& [roomId, room] : m_rooms) {
                if (room->GetPlayerCount() == 0 &&
                    room->GetUptime() > std::chrono::minutes(5)) {
                    emptyRooms.push_back(roomId);
                }
            }
        }

        // Remove empty rooms
        for (const auto& roomId : emptyRooms) {
            std::unique_lock<std::shared_mutex> lock(m_roomsMutex);
            auto it = m_rooms.find(roomId);
            if (it != m_rooms.end()) {
                m_rooms.erase(it);
                NotifyRoomDestroyed(roomId);
            }
        }
    }

    void RoomManager::CleanupExpiredInvites()
    {
        std::shared_lock<std::shared_mutex> lock(m_roomsMutex);

        for (auto& [roomId, room] : m_rooms) {
            room->CleanupExpiredInvites();
        }
    }

    // Private methods
    std::string RoomManager::GenerateRoomId()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(100000, 999999);

        return "room_" + std::to_string(dis(gen));
    }

    void RoomManager::NotifyRoomCreated(const std::string& roomId)
    {
        if (m_roomCreatedCallback) {
            m_roomCreatedCallback(roomId);
        }
    }

    void RoomManager::NotifyRoomDestroyed(const std::string& roomId)
    {
        if (m_roomDestroyedCallback) {
            m_roomDestroyedCallback(roomId);
        }
    }

    void RoomManager::NotifyPlayerJoinedRoom(const std::string& roomId, uint32_t playerId)
    {
        if (m_playerJoinedRoomCallback) {
            m_playerJoinedRoomCallback(roomId, playerId);
        }
    }

    void RoomManager::NotifyPlayerLeftRoom(const std::string& roomId, uint32_t playerId)
    {
        if (m_playerLeftRoomCallback) {
            m_playerLeftRoomCallback(roomId, playerId);
        }
    }

    bool RoomManager::ValidateRoomSettings(const RoomSettings& settings)
    {
        if (settings.name.empty() || settings.name.length() > 50) {
            return false;
        }

        if (settings.description.length() > 200) {
            return false;
        }

        if (settings.maxPlayers == 0 || settings.maxPlayers > 50) {
            return false;
        }

        return true;
    }

    uint32_t RoomManager::GetPlayerRoomCount(uint32_t playerId) const
    {
        std::shared_lock<std::shared_mutex> lock(m_roomsMutex);

        uint32_t count = 0;
        for (const auto& [roomId, room] : m_rooms) {
            if (room->GetOwnerId() == playerId) {
                count++;
            }
        }

        return count;
    }

    void RoomManager::SaveRoomStates()
    {
        // TODO: Implement room state persistence
    }

    void RoomManager::LoadRoomStates()
    {
        // TODO: Implement room state loading
    }

    // Utility functions
    namespace RoomUtils
    {
        std::string RoomTypeToString(RoomType type)
        {
            switch (type) {
                case RoomType::FreeRoam: return "FreeRoam";
                case RoomType::CooperativeMission: return "CooperativeMission";
                case RoomType::CompetitiveMatch: return "CompetitiveMatch";
                case RoomType::PrivateLobby: return "PrivateLobby";
                case RoomType::CustomGameMode: return "CustomGameMode";
                default: return "Unknown";
            }
        }

        RoomType StringToRoomType(const std::string& typeStr)
        {
            if (typeStr == "FreeRoam") return RoomType::FreeRoam;
            if (typeStr == "CooperativeMission") return RoomType::CooperativeMission;
            if (typeStr == "CompetitiveMatch") return RoomType::CompetitiveMatch;
            if (typeStr == "PrivateLobby") return RoomType::PrivateLobby;
            if (typeStr == "CustomGameMode") return RoomType::CustomGameMode;
            return RoomType::FreeRoam;
        }

        std::string RoomStateToString(RoomState state)
        {
            switch (state) {
                case RoomState::Waiting: return "Waiting";
                case RoomState::Starting: return "Starting";
                case RoomState::InProgress: return "InProgress";
                case RoomState::Paused: return "Paused";
                case RoomState::Completed: return "Completed";
                case RoomState::Cancelled: return "Cancelled";
                default: return "Unknown";
            }
        }

        std::string PermissionLevelToString(RoomPermissionLevel level)
        {
            switch (level) {
                case RoomPermissionLevel::Banned: return "Banned";
                case RoomPermissionLevel::Viewer: return "Viewer";
                case RoomPermissionLevel::Player: return "Player";
                case RoomPermissionLevel::Moderator: return "Moderator";
                case RoomPermissionLevel::Admin: return "Admin";
                case RoomPermissionLevel::Owner: return "Owner";
                default: return "Unknown";
            }
        }

        bool ValidateRoomName(const std::string& name)
        {
            if (name.empty() || name.length() > 50) {
                return false;
            }

            // Check for invalid characters
            std::regex validPattern("^[a-zA-Z0-9 _-]+$");
            return std::regex_match(name, validPattern);
        }

        bool ValidateRoomPassword(const std::string& password)
        {
            return password.length() <= 20;
        }

        std::string GenerateRoomCode(uint32_t length)
        {
            static const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_int_distribution<> dis(0, chars.size() - 1);

            std::string code;
            code.reserve(length);

            for (uint32_t i = 0; i < length; ++i) {
                code += chars[dis(gen)];
            }

            return code;
        }

        uint32_t EstimateRoomPing(const std::string& roomId)
        {
            // Simple ping estimation - in real implementation would measure actual network latency
            return 50 + (std::hash<std::string>{}(roomId) % 100);
        }
    }
}