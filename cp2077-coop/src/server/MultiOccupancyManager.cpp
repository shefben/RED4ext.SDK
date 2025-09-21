#include "MultiOccupancyManager.hpp"
#include "RED4ext/Api/Sdk.hpp"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace RED4ext
{
    // MultiOccupancyManager Implementation
    MultiOccupancyManager& MultiOccupancyManager::GetInstance()
    {
        static MultiOccupancyManager instance;
        return instance;
    }

    void MultiOccupancyManager::Initialize()
    {
        std::unique_lock<std::shared_mutex> lock(m_statesMutex);

        // Clear existing data
        m_playerStates.clear();
        m_occupiedVehicles.clear();
        m_seatReservations.clear();
        m_playerToVehicle.clear();
        m_vehicleToPlayers.clear();

        // Initialize timing
        m_lastUpdate = std::chrono::steady_clock::now();
        m_lastCleanup = m_lastUpdate;
        m_updateInterval = 0.1f; // 10 FPS occupancy updates

        // Initialize statistics
        m_totalVehicleEntries = 0;
        m_totalVehicleExits = 0;
        m_totalSeatReservations = 0;
        m_totalDriverChanges = 0;
    }

    void MultiOccupancyManager::Shutdown()
    {
        std::unique_lock<std::shared_mutex> lock(m_statesMutex);

        m_playerStates.clear();
        m_occupiedVehicles.clear();
        m_seatReservations.clear();
        m_playerToVehicle.clear();
        m_vehicleToPlayers.clear();

        // Clear callbacks
        std::lock_guard<std::mutex> callbacksLock(m_callbacksMutex);
        m_vehicleEntryCallback = nullptr;
        m_vehicleExitCallback = nullptr;
        m_seatReservationCallback = nullptr;
        m_driverChangeCallback = nullptr;
        m_vehicleOccupancyCallback = nullptr;
    }

    void MultiOccupancyManager::Update()
    {
        auto currentTime = std::chrono::steady_clock::now();
        auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - m_lastUpdate).count() / 1000.0f;

        m_lastUpdate = currentTime;

        // Update occupancy states
        UpdateOccupancyStates(deltaTime);

        // Update player occupancy states
        UpdatePlayerOccupancyStates(deltaTime);

        // Process seat reservation expirations
        ProcessSeatReservations();

        // Process occupancy expirations
        ProcessOccupancyExpirations();

        // Validate occupancy states
        ValidateOccupancyStates();

        // Periodic cleanup (every 2 minutes)
        auto timeSinceCleanup = std::chrono::duration_cast<std::chrono::seconds>(
            currentTime - m_lastCleanup).count();

        if (timeSinceCleanup >= 120) {
            CleanupEmptyVehicles();
            m_lastCleanup = currentTime;
        }
    }

    void MultiOccupancyManager::AddPlayer(uint32_t playerId, const std::string& playerName)
    {
        std::unique_lock<std::shared_mutex> lock(m_statesMutex);

        auto playerState = std::make_unique<PlayerOccupancyState>();
        playerState->playerId = playerId;
        playerState->playerName = playerName;
        playerState->isConnected = true;
        playerState->syncPriority = 1.0f;

        m_playerStates[playerId] = std::move(playerState);
    }

    void MultiOccupancyManager::RemovePlayer(uint32_t playerId)
    {
        std::unique_lock<std::shared_mutex> lock(m_statesMutex);

        // Force player to exit any vehicle they're in
        ForcePlayerExitVehicle(playerId);

        // Remove from all mappings
        RemovePlayerFromAllMappings(playerId);

        // Remove player state
        m_playerStates.erase(playerId);
    }

    VehicleEntryResult MultiOccupancyManager::RequestVehicleEntry(uint32_t playerId, uint64_t vehicleId, int32_t preferredSeat)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto playerIt = m_playerStates.find(playerId);
        if (playerIt == m_playerStates.end()) {
            return VehicleEntryResult::InvalidSeat;
        }

        // Check if player is already in a vehicle
        if (playerIt->second->isInVehicle) {
            return VehicleEntryResult::SeatOccupied;
        }

        lock.unlock();

        // Find or create vehicle state
        auto* vehicleState = FindOrCreateVehicleState(vehicleId);
        if (!vehicleState) {
            return VehicleEntryResult::VehicleFull;
        }

        // Determine seat to use
        int32_t seatIndex = preferredSeat;
        if (seatIndex == -1 || !vehicleState->IsSeatAvailable(seatIndex)) {
            seatIndex = vehicleState->FindAvailableSeat();
        }

        if (seatIndex == -1) {
            return VehicleEntryResult::VehicleFull;
        }

        // Validate the entry
        if (!ValidateVehicleEntry(playerId, vehicleId, seatIndex)) {
            return VehicleEntryResult::InvalidSeat;
        }

        // Process the vehicle entry
        if (!ProcessVehicleEntry(playerId, vehicleId, seatIndex)) {
            return VehicleEntryResult::SeatOccupied;
        }

        // Update statistics
        m_totalVehicleEntries++;

        // Notify listeners
        NotifyVehicleEntry(playerId, vehicleId, seatIndex, VehicleEntryResult::Success);

        // Broadcast entry
        VehicleEnterData enterData;
        enterData.playerId = playerId;
        enterData.vehicleId = vehicleId;
        enterData.seatIndex = seatIndex;
        enterData.timestamp = std::chrono::steady_clock::now();

        BroadcastVehicleEntry(enterData);

        return VehicleEntryResult::Success;
    }

    VehicleExitResult MultiOccupancyManager::RequestVehicleExit(uint32_t playerId, uint64_t vehicleId)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto playerIt = m_playerStates.find(playerId);
        if (playerIt == m_playerStates.end()) {
            return VehicleExitResult::PlayerNotInVehicle;
        }

        auto& playerState = playerIt->second;

        if (!playerState->isInVehicle || playerState->currentVehicleId != vehicleId) {
            return VehicleExitResult::PlayerNotInVehicle;
        }

        int32_t seatIndex = playerState->currentSeatIndex;

        lock.unlock();

        // Process the vehicle exit
        if (!ProcessVehicleExit(playerId, vehicleId)) {
            return VehicleExitResult::VehicleNotFound;
        }

        // Update statistics
        m_totalVehicleExits++;

        // Notify listeners
        NotifyVehicleExit(playerId, vehicleId, seatIndex, VehicleExitResult::Success);

        // Broadcast exit
        VehicleExitData exitData;
        exitData.playerId = playerId;
        exitData.vehicleId = vehicleId;
        exitData.seatIndex = seatIndex;
        exitData.timestamp = std::chrono::steady_clock::now();

        BroadcastVehicleExit(exitData);

        return VehicleExitResult::Success;
    }

    bool MultiOccupancyManager::ForcePlayerExitVehicle(uint32_t playerId)
    {
        auto* playerState = GetPlayerOccupancyState(playerId);
        if (!playerState || !playerState->isInVehicle) {
            return false;
        }

        uint64_t vehicleId = playerState->currentVehicleId;
        return ProcessVehicleExit(playerId, vehicleId);
    }

    bool MultiOccupancyManager::TransferVehicleControl(uint64_t vehicleId, uint32_t newDriverId)
    {
        auto* vehicleState = GetVehicleState(vehicleId);
        if (!vehicleState) {
            return false;
        }

        // Check if new driver is in the vehicle
        int32_t newDriverSeat = vehicleState->GetPlayerSeat(newDriverId);
        if (newDriverSeat == -1) {
            return false;
        }

        uint32_t oldDriverId = vehicleState->driverId;

        // Update driver assignment
        UpdateDriverAssignment(vehicleId, newDriverId, oldDriverId);

        // Notify listeners
        NotifyDriverChange(vehicleId, oldDriverId, newDriverId);

        m_totalDriverChanges++;

        return true;
    }

    SeatReservationResult MultiOccupancyManager::RequestSeatReservation(uint32_t playerId, uint64_t vehicleId, int32_t preferredSeat)
    {
        auto* vehicleState = GetVehicleState(vehicleId);
        if (!vehicleState) {
            return SeatReservationResult::InvalidSeat;
        }

        // Determine seat to reserve
        int32_t seatIndex = preferredSeat;
        if (seatIndex == -1 || !vehicleState->IsSeatAvailable(seatIndex)) {
            seatIndex = vehicleState->FindAvailableSeat();
        }

        if (seatIndex == -1) {
            return SeatReservationResult::VehicleFull;
        }

        // Check if seat is already reserved
        auto* existingReservation = FindSeatReservation(vehicleId, seatIndex);
        if (existingReservation && !existingReservation->IsExpired()) {
            return SeatReservationResult::AlreadyReserved;
        }

        // Validate reservation
        if (!ValidateSeatReservation(playerId, vehicleId, seatIndex)) {
            return SeatReservationResult::InvalidSeat;
        }

        // Create reservation
        std::lock_guard<std::mutex> lock(m_reservationsMutex);

        SeatReservation reservation;
        reservation.playerId = playerId;
        reservation.vehicleId = vehicleId;
        reservation.seatIndex = seatIndex;
        reservation.expirationTime = std::chrono::steady_clock::now() + std::chrono::seconds(5);

        m_seatReservations.push_back(reservation);
        m_totalSeatReservations++;

        // Notify listeners
        NotifySeatReservation(playerId, vehicleId, seatIndex, SeatReservationResult::Success);

        // Broadcast reservation
        SeatReservationData reservationData;
        reservationData.playerId = playerId;
        reservationData.vehicleId = vehicleId;
        reservationData.seatIndex = seatIndex;
        reservationData.reservationDuration = 5.0f;
        reservationData.timestamp = std::chrono::steady_clock::now();

        BroadcastSeatReservation(reservationData);

        return (seatIndex == preferredSeat) ? SeatReservationResult::Success : SeatReservationResult::AlternativeSeat;
    }

    bool MultiOccupancyManager::CancelSeatReservation(uint32_t playerId, uint64_t vehicleId)
    {
        return RemoveSeatReservation(playerId, vehicleId);
    }

    void MultiOccupancyManager::ProcessSeatReservations()
    {
        std::lock_guard<std::mutex> lock(m_reservationsMutex);

        auto currentTime = std::chrono::steady_clock::now();
        auto it = m_seatReservations.begin();

        while (it != m_seatReservations.end()) {
            if (it->IsExpired()) {
                ExpireSeatReservation(*it);
                it = m_seatReservations.erase(it);
            } else {
                ++it;
            }
        }
    }

    bool MultiOccupancyManager::RegisterVehicle(uint64_t vehicleId, uint32_t maxSeats)
    {
        auto* vehicleState = FindOrCreateVehicleState(vehicleId);
        if (!vehicleState) {
            return false;
        }

        vehicleState->maxSeats = maxSeats;
        vehicleState->InitializeSeatAssignments();

        return true;
    }

    bool MultiOccupancyManager::UnregisterVehicle(uint64_t vehicleId)
    {
        std::unique_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_occupiedVehicles.find(vehicleId);
        if (it == m_occupiedVehicles.end()) {
            return false;
        }

        // Force all players to exit
        auto occupants = it->second->GetOccupants();
        lock.unlock();

        for (uint32_t playerId : occupants) {
            ForcePlayerExitVehicle(playerId);
        }

        lock.lock();
        m_occupiedVehicles.erase(it);

        // Remove from mappings
        m_vehicleToPlayers.erase(vehicleId);

        return true;
    }

    void MultiOccupancyManager::CleanupEmptyVehicles()
    {
        std::unique_lock<std::shared_mutex> lock(m_statesMutex);

        auto currentTime = std::chrono::steady_clock::now();
        std::vector<uint64_t> emptyVehicles;

        for (auto& [vehicleId, vehicleState] : m_occupiedVehicles) {
            if (vehicleState->IsEmpty()) {
                auto timeSinceEmpty = std::chrono::duration_cast<std::chrono::minutes>(
                    currentTime - vehicleState->lastUpdate).count();

                if (timeSinceEmpty >= 5) { // 5 minutes empty
                    emptyVehicles.push_back(vehicleId);
                }
            }
        }

        // Remove empty vehicles
        for (uint64_t vehicleId : emptyVehicles) {
            m_occupiedVehicles.erase(vehicleId);
            m_vehicleToPlayers.erase(vehicleId);
        }
    }

    PlayerOccupancyState* MultiOccupancyManager::GetPlayerOccupancyState(uint32_t playerId)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        return (it != m_playerStates.end()) ? it->second.get() : nullptr;
    }

    const PlayerOccupancyState* MultiOccupancyManager::GetPlayerOccupancyState(uint32_t playerId) const
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        return (it != m_playerStates.end()) ? it->second.get() : nullptr;
    }

    OccupiedVehicleState* MultiOccupancyManager::GetVehicleState(uint64_t vehicleId)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_occupiedVehicles.find(vehicleId);
        return (it != m_occupiedVehicles.end()) ? it->second.get() : nullptr;
    }

    const OccupiedVehicleState* MultiOccupancyManager::GetVehicleState(uint64_t vehicleId) const
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_occupiedVehicles.find(vehicleId);
        return (it != m_occupiedVehicles.end()) ? it->second.get() : nullptr;
    }

    std::vector<uint32_t> MultiOccupancyManager::GetVehicleOccupants(uint64_t vehicleId) const
    {
        auto* vehicleState = GetVehicleState(vehicleId);
        if (vehicleState) {
            return vehicleState->GetOccupants();
        }
        return {};
    }

    std::vector<int32_t> MultiOccupancyManager::GetAvailableSeats(uint64_t vehicleId) const
    {
        auto* vehicleState = GetVehicleState(vehicleId);
        if (vehicleState) {
            return vehicleState->GetAvailableSeats();
        }
        return {};
    }

    uint32_t MultiOccupancyManager::GetVehicleDriver(uint64_t vehicleId) const
    {
        auto* vehicleState = GetVehicleState(vehicleId);
        if (vehicleState) {
            return vehicleState->driverId;
        }
        return 0;
    }

    uint64_t MultiOccupancyManager::GetPlayerCurrentVehicle(uint32_t playerId) const
    {
        auto* playerState = GetPlayerOccupancyState(playerId);
        if (playerState && playerState->isInVehicle) {
            return playerState->currentVehicleId;
        }
        return 0;
    }

    int32_t MultiOccupancyManager::GetPlayerCurrentSeat(uint32_t playerId) const
    {
        auto* playerState = GetPlayerOccupancyState(playerId);
        if (playerState && playerState->isInVehicle) {
            return playerState->currentSeatIndex;
        }
        return -1;
    }

    bool MultiOccupancyManager::IsVehicleOccupied(uint64_t vehicleId) const
    {
        auto* vehicleState = GetVehicleState(vehicleId);
        return vehicleState && !vehicleState->IsEmpty();
    }

    bool MultiOccupancyManager::IsPlayerInVehicle(uint32_t playerId) const
    {
        auto* playerState = GetPlayerOccupancyState(playerId);
        return playerState && playerState->isInVehicle;
    }

    bool MultiOccupancyManager::IsPlayerDriver(uint32_t playerId) const
    {
        auto* playerState = GetPlayerOccupancyState(playerId);
        if (!playerState || !playerState->isInVehicle) {
            return false;
        }

        auto* vehicleState = GetVehicleState(playerState->currentVehicleId);
        return vehicleState && vehicleState->driverId == playerId;
    }

    bool MultiOccupancyManager::ValidateVehicleEntry(uint32_t playerId, uint64_t vehicleId, int32_t seatIndex) const
    {
        auto* vehicleState = GetVehicleState(vehicleId);
        if (!vehicleState) {
            return false;
        }

        return IsValidSeatIndex(vehicleId, seatIndex) && vehicleState->IsSeatAvailable(seatIndex);
    }

    bool MultiOccupancyManager::ValidateSeatReservation(uint32_t playerId, uint64_t vehicleId, int32_t seatIndex) const
    {
        auto* vehicleState = GetVehicleState(vehicleId);
        if (!vehicleState) {
            return false;
        }

        return IsValidSeatIndex(vehicleId, seatIndex) && vehicleState->IsSeatAvailable(seatIndex);
    }

    bool MultiOccupancyManager::IsValidSeatIndex(uint64_t vehicleId, int32_t seatIndex) const
    {
        auto* vehicleState = GetVehicleState(vehicleId);
        if (!vehicleState) {
            return false;
        }

        return seatIndex >= 0 && seatIndex < static_cast<int32_t>(vehicleState->maxSeats);
    }

    bool MultiOccupancyManager::CanPlayerControlVehicle(uint32_t playerId, uint64_t vehicleId) const
    {
        auto* vehicleState = GetVehicleState(vehicleId);
        if (!vehicleState) {
            return false;
        }

        return vehicleState->driverId == playerId;
    }

    void MultiOccupancyManager::BroadcastVehicleEntry(const VehicleEnterData& enterData)
    {
        SendVehicleEntryToClients(enterData);
    }

    void MultiOccupancyManager::BroadcastVehicleExit(const VehicleExitData& exitData)
    {
        SendVehicleExitToClients(exitData);
    }

    void MultiOccupancyManager::BroadcastSeatReservation(const SeatReservationData& reservationData)
    {
        SendSeatReservationToClients(reservationData);
    }

    void MultiOccupancyManager::BroadcastOccupancyUpdate(const VehicleOccupancyUpdateData& occupancyData)
    {
        SendOccupancyUpdateToClients(occupancyData);
    }

    void MultiOccupancyManager::SynchronizePlayerOccupancy(uint32_t playerId)
    {
        auto* playerState = GetPlayerOccupancyState(playerId);
        if (!playerState) {
            return;
        }

        // Create occupancy state update
        OccupancyStateUpdate updateData;
        updateData.playerId = playerId;
        updateData.currentVehicleId = playerState->currentVehicleId;
        updateData.currentSeatIndex = playerState->currentSeatIndex;
        updateData.isInVehicle = playerState->isInVehicle;
        updateData.updateTime = std::chrono::steady_clock::now();
        updateData.syncVersion = 1;

        // Would broadcast this update to clients
    }

    void MultiOccupancyManager::ForceSyncPlayer(uint32_t playerId)
    {
        SynchronizePlayerOccupancy(playerId);
    }

    void MultiOccupancyManager::SetSyncPriority(uint32_t playerId, float priority)
    {
        auto* playerState = GetPlayerOccupancyState(playerId);
        if (playerState) {
            playerState->syncPriority = priority;
        }
    }

    uint32_t MultiOccupancyManager::GetActivePlayerCount() const
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        uint32_t count = 0;
        for (const auto& [playerId, playerState] : m_playerStates) {
            if (playerState->isConnected) {
                count++;
            }
        }

        return count;
    }

    uint32_t MultiOccupancyManager::GetOccupiedVehicleCount() const
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);
        return static_cast<uint32_t>(m_occupiedVehicles.size());
    }

    uint32_t MultiOccupancyManager::GetActiveReservationCount() const
    {
        std::lock_guard<std::mutex> lock(m_reservationsMutex);
        return static_cast<uint32_t>(m_seatReservations.size());
    }

    uint32_t MultiOccupancyManager::GetTotalVehicleEntries() const
    {
        return m_totalVehicleEntries;
    }

    uint32_t MultiOccupancyManager::GetTotalSeatChanges() const
    {
        return m_totalSeatReservations;
    }

    std::unordered_map<SeatType, uint32_t> MultiOccupancyManager::GetSeatTypeStats() const
    {
        std::unordered_map<SeatType, uint32_t> stats;
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        for (const auto& [vehicleId, vehicleState] : m_occupiedVehicles) {
            for (const auto& seat : vehicleState->seatAssignments) {
                if (seat.isOccupied) {
                    stats[seat.seatType]++;
                }
            }
        }

        return stats;
    }

    std::unordered_map<uint64_t, uint32_t> MultiOccupancyManager::GetVehicleOccupancyStats() const
    {
        std::unordered_map<uint64_t, uint32_t> stats;
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        for (const auto& [vehicleId, vehicleState] : m_occupiedVehicles) {
            uint32_t occupiedCount = 0;
            for (const auto& seat : vehicleState->seatAssignments) {
                if (seat.isOccupied) {
                    occupiedCount++;
                }
            }
            stats[vehicleId] = occupiedCount;
        }

        return stats;
    }

    // Private implementation methods
    void MultiOccupancyManager::UpdateOccupancyStates(float deltaTime)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto currentTime = std::chrono::steady_clock::now();

        for (auto& [vehicleId, vehicleState] : m_occupiedVehicles) {
            if (vehicleState->hasOccupancyChanged) {
                // Broadcast occupancy update
                VehicleOccupancyUpdateData updateData;
                updateData.vehicleId = vehicleId;
                updateData.seatAssignments = vehicleState->seatAssignments;
                updateData.driverId = vehicleState->driverId;
                updateData.timestamp = currentTime;

                lock.unlock();
                BroadcastOccupancyUpdate(updateData);
                lock.lock();

                vehicleState->hasOccupancyChanged = false;
                vehicleState->lastUpdate = currentTime;
            }
        }
    }

    void MultiOccupancyManager::UpdatePlayerOccupancyStates(float deltaTime)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto currentTime = std::chrono::steady_clock::now();

        for (auto& [playerId, playerState] : m_playerStates) {
            // Check for player timeout (5 minutes of inactivity)
            auto timeSinceActivity = std::chrono::duration_cast<std::chrono::minutes>(
                currentTime - playerState->lastActivity).count();

            if (timeSinceActivity >= 5) {
                playerState->isConnected = false;
            }

            // Update sync priority based on vehicle state
            if (playerState->isInVehicle) {
                playerState->syncPriority = 2.0f; // Higher priority for players in vehicles
            } else {
                playerState->syncPriority = 1.0f; // Normal priority
            }
        }
    }

    void MultiOccupancyManager::ProcessOccupancyExpirations()
    {
        // Clean up any expired occupancy data
        // This would handle timeout scenarios for players who disconnect while in vehicles
    }

    void MultiOccupancyManager::ValidateOccupancyStates()
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        for (auto& [playerId, playerState] : m_playerStates) {
            if (playerState->isInVehicle) {
                // Validate that the vehicle still exists and player is still in it
                auto* vehicleState = const_cast<MultiOccupancyManager*>(this)->GetVehicleState(playerState->currentVehicleId);
                if (!vehicleState) {
                    // Vehicle no longer exists, reset player state
                    playerState->isInVehicle = false;
                    playerState->currentVehicleId = 0;
                    playerState->currentSeatIndex = -1;
                } else {
                    // Verify player is still in the expected seat
                    int32_t currentSeat = vehicleState->GetPlayerSeat(playerId);
                    if (currentSeat != playerState->currentSeatIndex) {
                        playerState->currentSeatIndex = currentSeat;
                        if (currentSeat == -1) {
                            playerState->isInVehicle = false;
                            playerState->currentVehicleId = 0;
                        }
                    }
                }
            }
        }
    }

    OccupiedVehicleState* MultiOccupancyManager::FindOrCreateVehicleState(uint64_t vehicleId)
    {
        std::unique_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_occupiedVehicles.find(vehicleId);
        if (it != m_occupiedVehicles.end()) {
            return it->second.get();
        }

        // Create new vehicle state
        auto vehicleState = std::make_unique<OccupiedVehicleState>();
        vehicleState->vehicleId = vehicleId;
        vehicleState->maxSeats = DetermineVehicleMaxSeats(vehicleId);
        vehicleState->InitializeSeatAssignments();

        auto* statePtr = vehicleState.get();
        m_occupiedVehicles[vehicleId] = std::move(vehicleState);

        return statePtr;
    }

    SeatReservation* MultiOccupancyManager::FindSeatReservation(uint32_t playerId, uint64_t vehicleId)
    {
        std::lock_guard<std::mutex> lock(m_reservationsMutex);

        for (auto& reservation : m_seatReservations) {
            if (reservation.playerId == playerId && reservation.vehicleId == vehicleId) {
                return &reservation;
            }
        }

        return nullptr;
    }

    SeatReservation* MultiOccupancyManager::FindSeatReservation(uint64_t vehicleId, int32_t seatIndex)
    {
        std::lock_guard<std::mutex> lock(m_reservationsMutex);

        for (auto& reservation : m_seatReservations) {
            if (reservation.vehicleId == vehicleId && reservation.seatIndex == seatIndex) {
                return &reservation;
            }
        }

        return nullptr;
    }

    bool MultiOccupancyManager::RemoveSeatReservation(uint32_t playerId, uint64_t vehicleId)
    {
        std::lock_guard<std::mutex> lock(m_reservationsMutex);

        auto it = std::find_if(m_seatReservations.begin(), m_seatReservations.end(),
            [playerId, vehicleId](const SeatReservation& reservation) {
                return reservation.playerId == playerId && reservation.vehicleId == vehicleId;
            });

        if (it != m_seatReservations.end()) {
            m_seatReservations.erase(it);
            return true;
        }

        return false;
    }

    void MultiOccupancyManager::ExpireSeatReservation(const SeatReservation& reservation)
    {
        // Notify that reservation has expired
        NotifySeatReservation(reservation.playerId, reservation.vehicleId,
                            reservation.seatIndex, SeatReservationResult::InvalidSeat);
    }

    uint32_t MultiOccupancyManager::DetermineVehicleMaxSeats(uint64_t vehicleId) const
    {
        // This would query the game engine for the vehicle's actual seat count
        // For now, return a default value
        return 4;
    }

    bool MultiOccupancyManager::ProcessVehicleEntry(uint32_t playerId, uint64_t vehicleId, int32_t seatIndex)
    {
        auto* vehicleState = GetVehicleState(vehicleId);
        auto* playerState = GetPlayerOccupancyState(playerId);

        if (!vehicleState || !playerState) {
            return false;
        }

        // Assign seat in vehicle
        if (!vehicleState->AssignSeat(playerId, seatIndex)) {
            return false;
        }

        // Update player state
        playerState->isInVehicle = true;
        playerState->currentVehicleId = vehicleId;
        playerState->currentSeatIndex = seatIndex;
        playerState->lastVehicleUpdate = std::chrono::steady_clock::now();
        playerState->vehiclesEntered++;

        if (seatIndex == 0) { // Driver seat
            playerState->timesAsDriver++;
        }

        // Update mappings
        UpdatePlayerToVehicleMapping(playerId, vehicleId, true);
        UpdateVehicleToPlayersMapping(vehicleId, playerId, true);

        return true;
    }

    bool MultiOccupancyManager::ProcessVehicleExit(uint32_t playerId, uint64_t vehicleId)
    {
        auto* vehicleState = GetVehicleState(vehicleId);
        auto* playerState = GetPlayerOccupancyState(playerId);

        if (!vehicleState || !playerState) {
            return false;
        }

        // Remove player from vehicle
        vehicleState->RemovePlayer(playerId);

        // Update player state
        playerState->isInVehicle = false;
        playerState->currentVehicleId = 0;
        playerState->currentSeatIndex = -1;
        playerState->lastVehicleUpdate = std::chrono::steady_clock::now();

        // Update mappings
        UpdatePlayerToVehicleMapping(playerId, vehicleId, false);
        UpdateVehicleToPlayersMapping(vehicleId, playerId, false);

        return true;
    }

    void MultiOccupancyManager::UpdateDriverAssignment(uint64_t vehicleId, uint32_t newDriverId, uint32_t oldDriverId)
    {
        auto* vehicleState = GetVehicleState(vehicleId);
        if (!vehicleState) {
            return;
        }

        // Remove old driver from driver seat if different
        if (oldDriverId != 0 && oldDriverId != newDriverId) {
            for (auto& seat : vehicleState->seatAssignments) {
                if (seat.playerId == oldDriverId && seat.seatIndex == 0) {
                    // Move old driver to another seat
                    int32_t newSeat = vehicleState->FindAvailableSeat();
                    if (newSeat != -1 && newSeat != 0) {
                        seat.isOccupied = false;
                        vehicleState->seatAssignments[newSeat].playerId = oldDriverId;
                        vehicleState->seatAssignments[newSeat].isOccupied = true;

                        // Update player state
                        auto* oldDriverState = GetPlayerOccupancyState(oldDriverId);
                        if (oldDriverState) {
                            oldDriverState->currentSeatIndex = newSeat;
                        }
                    }
                    break;
                }
            }
        }

        // Move new driver to driver seat
        int32_t currentSeat = vehicleState->GetPlayerSeat(newDriverId);
        if (currentSeat != -1 && currentSeat != 0) {
            // Clear current seat
            vehicleState->seatAssignments[currentSeat].isOccupied = false;
            vehicleState->seatAssignments[currentSeat].playerId = 0;

            // Assign to driver seat
            vehicleState->seatAssignments[0].playerId = newDriverId;
            vehicleState->seatAssignments[0].isOccupied = true;

            // Update player state
            auto* newDriverState = GetPlayerOccupancyState(newDriverId);
            if (newDriverState) {
                newDriverState->currentSeatIndex = 0;
            }
        }

        vehicleState->driverId = newDriverId;
        vehicleState->hasOccupancyChanged = true;
    }

    void MultiOccupancyManager::UpdatePlayerToVehicleMapping(uint32_t playerId, uint64_t vehicleId, bool isActive)
    {
        if (isActive) {
            m_playerToVehicle[playerId] = vehicleId;
        } else {
            m_playerToVehicle.erase(playerId);
        }
    }

    void MultiOccupancyManager::UpdateVehicleToPlayersMapping(uint64_t vehicleId, uint32_t playerId, bool isActive)
    {
        auto& playerList = m_vehicleToPlayers[vehicleId];

        if (isActive) {
            if (std::find(playerList.begin(), playerList.end(), playerId) == playerList.end()) {
                playerList.push_back(playerId);
            }
        } else {
            playerList.erase(std::remove(playerList.begin(), playerList.end(), playerId), playerList.end());
        }
    }

    void MultiOccupancyManager::RemovePlayerFromAllMappings(uint32_t playerId)
    {
        // Remove from player to vehicle mapping
        auto playerVehicleIt = m_playerToVehicle.find(playerId);
        if (playerVehicleIt != m_playerToVehicle.end()) {
            uint64_t vehicleId = playerVehicleIt->second;

            // Remove from vehicle to players mapping
            auto& playerList = m_vehicleToPlayers[vehicleId];
            playerList.erase(std::remove(playerList.begin(), playerList.end(), playerId), playerList.end());

            m_playerToVehicle.erase(playerVehicleIt);
        }

        // Remove any seat reservations
        std::lock_guard<std::mutex> lock(m_reservationsMutex);
        m_seatReservations.erase(
            std::remove_if(m_seatReservations.begin(), m_seatReservations.end(),
                [playerId](const SeatReservation& reservation) {
                    return reservation.playerId == playerId;
                }),
            m_seatReservations.end());
    }

    // Notification methods
    void MultiOccupancyManager::NotifyVehicleEntry(uint32_t playerId, uint64_t vehicleId, int32_t seatIndex, VehicleEntryResult result)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_vehicleEntryCallback) {
            m_vehicleEntryCallback(playerId, vehicleId, seatIndex, result);
        }
    }

    void MultiOccupancyManager::NotifyVehicleExit(uint32_t playerId, uint64_t vehicleId, int32_t seatIndex, VehicleExitResult result)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_vehicleExitCallback) {
            m_vehicleExitCallback(playerId, vehicleId, seatIndex, result);
        }
    }

    void MultiOccupancyManager::NotifySeatReservation(uint32_t playerId, uint64_t vehicleId, int32_t seatIndex, SeatReservationResult result)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_seatReservationCallback) {
            m_seatReservationCallback(playerId, vehicleId, seatIndex, result);
        }
    }

    void MultiOccupancyManager::NotifyDriverChange(uint64_t vehicleId, uint32_t oldDriverId, uint32_t newDriverId)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_driverChangeCallback) {
            m_driverChangeCallback(vehicleId, oldDriverId, newDriverId);
        }
    }

    void MultiOccupancyManager::NotifyVehicleOccupancy(uint64_t vehicleId, const std::vector<uint32_t>& occupants)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_vehicleOccupancyCallback) {
            m_vehicleOccupancyCallback(vehicleId, occupants);
        }
    }

    void MultiOccupancyManager::SendVehicleEntryToClients(const VehicleEnterData& enterData)
    {
        // This would send network messages to all clients
        // Implementation would depend on the networking system
    }

    void MultiOccupancyManager::SendVehicleExitToClients(const VehicleExitData& exitData)
    {
        // This would send network messages to all clients
        // Implementation would depend on the networking system
    }

    void MultiOccupancyManager::SendSeatReservationToClients(const SeatReservationData& reservationData)
    {
        // This would send network messages to all clients
        // Implementation would depend on the networking system
    }

    void MultiOccupancyManager::SendOccupancyUpdateToClients(const VehicleOccupancyUpdateData& occupancyData)
    {
        // This would send network messages to all clients
        // Implementation would depend on the networking system
    }

    // Additional method implementations
    void MultiOccupancyManager::UpdatePlayerActivity(uint32_t playerId)
    {
        auto* playerState = GetPlayerOccupancyState(playerId);
        if (playerState) {
            playerState->lastActivity = std::chrono::steady_clock::now();
            playerState->isConnected = true;
        }
    }

    // Callback setters
    void MultiOccupancyManager::SetVehicleEntryCallback(VehicleEntryCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_vehicleEntryCallback = callback;
    }

    void MultiOccupancyManager::SetVehicleExitCallback(VehicleExitCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_vehicleExitCallback = callback;
    }

    void MultiOccupancyManager::SetSeatReservationCallback(SeatReservationCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_seatReservationCallback = callback;
    }

    void MultiOccupancyManager::SetDriverChangeCallback(DriverChangeCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_driverChangeCallback = callback;
    }

    void MultiOccupancyManager::SetVehicleOccupancyCallback(VehicleOccupancyCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_vehicleOccupancyCallback = callback;
    }

    // Utility functions implementation
    namespace MultiOccupancyUtils
    {
        std::string SeatTypeToString(SeatType seatType)
        {
            switch (seatType) {
                case SeatType::Driver: return "Driver";
                case SeatType::Passenger: return "Passenger";
                case SeatType::Rear: return "Rear";
                case SeatType::Special: return "Special";
                default: return "Unknown";
            }
        }

        SeatType StringToSeatType(const std::string& seatStr)
        {
            if (seatStr == "Driver") return SeatType::Driver;
            if (seatStr == "Passenger") return SeatType::Passenger;
            if (seatStr == "Rear") return SeatType::Rear;
            if (seatStr == "Special") return SeatType::Special;
            return SeatType::Rear; // Default
        }

        std::string SeatReservationResultToString(SeatReservationResult result)
        {
            switch (result) {
                case SeatReservationResult::Success: return "Success";
                case SeatReservationResult::AlternativeSeat: return "AlternativeSeat";
                case SeatReservationResult::VehicleFull: return "VehicleFull";
                case SeatReservationResult::InvalidSeat: return "InvalidSeat";
                case SeatReservationResult::AlreadyReserved: return "AlreadyReserved";
                default: return "Unknown";
            }
        }

        std::string VehicleEntryResultToString(VehicleEntryResult result)
        {
            switch (result) {
                case VehicleEntryResult::Success: return "Success";
                case VehicleEntryResult::SeatOccupied: return "SeatOccupied";
                case VehicleEntryResult::VehicleFull: return "VehicleFull";
                case VehicleEntryResult::InvalidSeat: return "InvalidSeat";
                case VehicleEntryResult::ReservationConflict: return "ReservationConflict";
                default: return "Unknown";
            }
        }

        std::string VehicleExitResultToString(VehicleExitResult result)
        {
            switch (result) {
                case VehicleExitResult::Success: return "Success";
                case VehicleExitResult::PlayerNotInVehicle: return "PlayerNotInVehicle";
                case VehicleExitResult::VehicleNotFound: return "VehicleNotFound";
                default: return "Unknown";
            }
        }

        bool IsDriverSeat(int32_t seatIndex)
        {
            return seatIndex == 0;
        }

        bool IsPassengerSeat(int32_t seatIndex)
        {
            return seatIndex == 1;
        }

        bool IsRearSeat(int32_t seatIndex)
        {
            return seatIndex >= 2;
        }

        uint32_t CalculateVehicleCapacity(const OccupiedVehicleState& vehicleState)
        {
            return vehicleState.maxSeats;
        }

        float CalculateOccupancyPercentage(const OccupiedVehicleState& vehicleState)
        {
            uint32_t occupiedCount = 0;
            for (const auto& seat : vehicleState.seatAssignments) {
                if (seat.isOccupied) {
                    occupiedCount++;
                }
            }

            return static_cast<float>(occupiedCount) / static_cast<float>(vehicleState.maxSeats);
        }

        uint32_t HashOccupancyState(const PlayerOccupancyState& state)
        {
            // Simple hash combining multiple state values
            uint32_t hash = 0;
            hash ^= state.playerId + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= state.currentVehicleId + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= static_cast<uint32_t>(state.currentSeatIndex) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            return hash;
        }

        bool ShouldSyncOccupancyState(const PlayerOccupancyState& oldState, const PlayerOccupancyState& newState)
        {
            // Sync if vehicle state changed
            if (oldState.isInVehicle != newState.isInVehicle) {
                return true;
            }

            // Sync if vehicle changed
            if (oldState.currentVehicleId != newState.currentVehicleId) {
                return true;
            }

            // Sync if seat changed
            if (oldState.currentSeatIndex != newState.currentSeatIndex) {
                return true;
            }

            return false;
        }
    }
}