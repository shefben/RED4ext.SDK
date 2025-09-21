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
    // Multi-occupancy enums matching REDscript
    enum class SeatType : uint8_t
    {
        Driver = 0,
        Passenger = 1,
        Rear = 2,
        Special = 3
    };

    enum class SeatReservationResult : uint8_t
    {
        Success = 0,
        AlternativeSeat = 1,
        VehicleFull = 2,
        InvalidSeat = 3,
        AlreadyReserved = 4
    };

    enum class VehicleEntryResult : uint8_t
    {
        Success = 0,
        SeatOccupied = 1,
        VehicleFull = 2,
        InvalidSeat = 3,
        ReservationConflict = 4
    };

    enum class VehicleExitResult : uint8_t
    {
        Success = 0,
        PlayerNotInVehicle = 1,
        VehicleNotFound = 2
    };

    // Data structures
    struct SeatAssignment
    {
        int32_t seatIndex;
        uint32_t playerId;
        bool isOccupied;
        SeatType seatType;
        std::chrono::steady_clock::time_point assignTime;

        SeatAssignment()
            : seatIndex(-1), playerId(0), isOccupied(false),
              seatType(SeatType::Rear), assignTime(std::chrono::steady_clock::now()) {}
    };

    struct VehicleEnterData
    {
        uint32_t playerId;
        uint64_t vehicleId;
        int32_t seatIndex;
        std::chrono::steady_clock::time_point timestamp;

        VehicleEnterData()
            : playerId(0), vehicleId(0), seatIndex(-1),
              timestamp(std::chrono::steady_clock::now()) {}
    };

    struct VehicleExitData
    {
        uint32_t playerId;
        uint64_t vehicleId;
        int32_t seatIndex;
        std::chrono::steady_clock::time_point timestamp;

        VehicleExitData()
            : playerId(0), vehicleId(0), seatIndex(-1),
              timestamp(std::chrono::steady_clock::now()) {}
    };

    struct SeatReservationData
    {
        uint32_t playerId;
        uint64_t vehicleId;
        int32_t seatIndex;
        float reservationDuration;
        std::chrono::steady_clock::time_point timestamp;

        SeatReservationData()
            : playerId(0), vehicleId(0), seatIndex(-1), reservationDuration(5.0f),
              timestamp(std::chrono::steady_clock::now()) {}
    };

    struct VehicleOccupancyUpdateData
    {
        uint64_t vehicleId;
        std::vector<SeatAssignment> seatAssignments;
        uint32_t driverId;
        std::chrono::steady_clock::time_point timestamp;

        VehicleOccupancyUpdateData()
            : vehicleId(0), driverId(0), timestamp(std::chrono::steady_clock::now()) {}
    };

    // Seat reservation class
    struct SeatReservation
    {
        uint32_t playerId;
        uint64_t vehicleId;
        int32_t seatIndex;
        std::chrono::steady_clock::time_point expirationTime;

        SeatReservation()
            : playerId(0), vehicleId(0), seatIndex(-1),
              expirationTime(std::chrono::steady_clock::now()) {}

        bool IsExpired() const
        {
            return std::chrono::steady_clock::now() >= expirationTime;
        }
    };

    // Occupied vehicle state tracker
    struct OccupiedVehicleState
    {
        uint64_t vehicleId;
        std::vector<SeatAssignment> seatAssignments;
        uint32_t maxSeats;
        uint32_t driverId;
        bool hasOccupancyChanged;
        std::chrono::steady_clock::time_point lastUpdate;
        std::chrono::steady_clock::time_point creationTime;

        OccupiedVehicleState()
            : vehicleId(0), maxSeats(4), driverId(0), hasOccupancyChanged(false),
              lastUpdate(std::chrono::steady_clock::now()),
              creationTime(std::chrono::steady_clock::now()) {}

        void InitializeSeatAssignments()
        {
            seatAssignments.clear();
            for (uint32_t i = 0; i < maxSeats; ++i) {
                SeatAssignment assignment;
                assignment.seatIndex = static_cast<int32_t>(i);
                assignment.playerId = 0;
                assignment.isOccupied = false;
                assignment.seatType = GetSeatType(static_cast<int32_t>(i));
                seatAssignments.push_back(assignment);
            }
        }

        SeatType GetSeatType(int32_t seatIndex) const
        {
            switch (seatIndex) {
                case 0: return SeatType::Driver;
                case 1: return SeatType::Passenger;
                default: return SeatType::Rear;
            }
        }

        bool AssignSeat(uint32_t playerId, int32_t seatIndex)
        {
            if (seatIndex < 0 || seatIndex >= static_cast<int32_t>(maxSeats)) {
                return false;
            }

            if (seatAssignments[seatIndex].isOccupied) {
                return false;
            }

            seatAssignments[seatIndex].playerId = playerId;
            seatAssignments[seatIndex].isOccupied = true;
            seatAssignments[seatIndex].assignTime = std::chrono::steady_clock::now();

            if (seatIndex == 0) {
                driverId = playerId;
            }

            hasOccupancyChanged = true;
            lastUpdate = std::chrono::steady_clock::now();
            return true;
        }

        bool RemovePlayer(uint32_t playerId)
        {
            bool removed = false;

            for (auto& assignment : seatAssignments) {
                if (assignment.playerId == playerId) {
                    assignment.playerId = 0;
                    assignment.isOccupied = false;

                    if (assignment.seatIndex == 0) {
                        driverId = 0;
                    }

                    removed = true;
                    hasOccupancyChanged = true;
                    lastUpdate = std::chrono::steady_clock::now();
                    break;
                }
            }

            return removed;
        }

        bool IsSeatAvailable(int32_t seatIndex) const
        {
            if (seatIndex < 0 || seatIndex >= static_cast<int32_t>(maxSeats)) {
                return false;
            }

            return !seatAssignments[seatIndex].isOccupied;
        }

        int32_t FindAvailableSeat() const
        {
            for (const auto& assignment : seatAssignments) {
                if (!assignment.isOccupied) {
                    return assignment.seatIndex;
                }
            }
            return -1; // No available seats
        }

        int32_t GetPlayerSeat(uint32_t playerId) const
        {
            for (const auto& assignment : seatAssignments) {
                if (assignment.playerId == playerId) {
                    return assignment.seatIndex;
                }
            }
            return -1; // Player not in vehicle
        }

        bool IsEmpty() const
        {
            for (const auto& assignment : seatAssignments) {
                if (assignment.isOccupied) {
                    return false;
                }
            }
            return true;
        }

        std::vector<uint32_t> GetOccupants() const
        {
            std::vector<uint32_t> occupants;
            for (const auto& assignment : seatAssignments) {
                if (assignment.isOccupied) {
                    occupants.push_back(assignment.playerId);
                }
            }
            return occupants;
        }

        std::vector<int32_t> GetAvailableSeats() const
        {
            std::vector<int32_t> availableSeats;
            for (const auto& assignment : seatAssignments) {
                if (!assignment.isOccupied) {
                    availableSeats.push_back(assignment.seatIndex);
                }
            }
            return availableSeats;
        }
    };

    struct PlayerOccupancyState
    {
        uint32_t playerId;
        std::string playerName;
        uint64_t currentVehicleId;
        int32_t currentSeatIndex;
        bool isInVehicle;
        std::chrono::steady_clock::time_point lastVehicleUpdate;
        std::chrono::steady_clock::time_point lastActivity;
        bool isConnected;
        float syncPriority;

        // Statistics
        uint32_t vehiclesEntered;
        uint32_t timesAsDriver;
        uint32_t totalTimeInVehicles; // seconds
        uint32_t seatChanges;

        PlayerOccupancyState()
            : playerId(0), currentVehicleId(0), currentSeatIndex(-1),
              isInVehicle(false), lastVehicleUpdate(std::chrono::steady_clock::now()),
              lastActivity(std::chrono::steady_clock::now()),
              isConnected(false), syncPriority(1.0f),
              vehiclesEntered(0), timesAsDriver(0),
              totalTimeInVehicles(0), seatChanges(0) {}
    };

    // Main multi-occupancy management class
    class MultiOccupancyManager
    {
    public:
        static MultiOccupancyManager& GetInstance();

        // Lifecycle
        void Initialize();
        void Shutdown();
        void Update();

        // Player management
        void AddPlayer(uint32_t playerId, const std::string& playerName);
        void RemovePlayer(uint32_t playerId);
        void UpdatePlayerActivity(uint32_t playerId);

        // Vehicle occupancy management
        VehicleEntryResult RequestVehicleEntry(uint32_t playerId, uint64_t vehicleId, int32_t preferredSeat = -1);
        VehicleExitResult RequestVehicleExit(uint32_t playerId, uint64_t vehicleId);
        bool ForcePlayerExitVehicle(uint32_t playerId);
        bool TransferVehicleControl(uint64_t vehicleId, uint32_t newDriverId);

        // Seat reservation system
        SeatReservationResult RequestSeatReservation(uint32_t playerId, uint64_t vehicleId, int32_t preferredSeat = -1);
        bool CancelSeatReservation(uint32_t playerId, uint64_t vehicleId);
        void ProcessSeatReservations();

        // Vehicle state management
        bool RegisterVehicle(uint64_t vehicleId, uint32_t maxSeats = 4);
        bool UnregisterVehicle(uint64_t vehicleId);
        void CleanupEmptyVehicles();

        // Query methods
        PlayerOccupancyState* GetPlayerOccupancyState(uint32_t playerId);
        const PlayerOccupancyState* GetPlayerOccupancyState(uint32_t playerId) const;
        OccupiedVehicleState* GetVehicleState(uint64_t vehicleId);
        const OccupiedVehicleState* GetVehicleState(uint64_t vehicleId) const;
        std::vector<uint32_t> GetVehicleOccupants(uint64_t vehicleId) const;
        std::vector<int32_t> GetAvailableSeats(uint64_t vehicleId) const;
        uint32_t GetVehicleDriver(uint64_t vehicleId) const;
        uint64_t GetPlayerCurrentVehicle(uint32_t playerId) const;
        int32_t GetPlayerCurrentSeat(uint32_t playerId) const;
        bool IsVehicleOccupied(uint64_t vehicleId) const;
        bool IsPlayerInVehicle(uint32_t playerId) const;
        bool IsPlayerDriver(uint32_t playerId) const;

        // Validation methods
        bool ValidateVehicleEntry(uint32_t playerId, uint64_t vehicleId, int32_t seatIndex) const;
        bool ValidateSeatReservation(uint32_t playerId, uint64_t vehicleId, int32_t seatIndex) const;
        bool IsValidSeatIndex(uint64_t vehicleId, int32_t seatIndex) const;
        bool CanPlayerControlVehicle(uint32_t playerId, uint64_t vehicleId) const;

        // Network synchronization
        void BroadcastVehicleEntry(const VehicleEnterData& enterData);
        void BroadcastVehicleExit(const VehicleExitData& exitData);
        void BroadcastSeatReservation(const SeatReservationData& reservationData);
        void BroadcastOccupancyUpdate(const VehicleOccupancyUpdateData& occupancyData);
        void SynchronizePlayerOccupancy(uint32_t playerId);
        void ForceSyncPlayer(uint32_t playerId);
        void SetSyncPriority(uint32_t playerId, float priority);

        // Statistics and monitoring
        uint32_t GetActivePlayerCount() const;
        uint32_t GetOccupiedVehicleCount() const;
        uint32_t GetActiveReservationCount() const;
        uint32_t GetTotalVehicleEntries() const;
        uint32_t GetTotalSeatChanges() const;
        std::unordered_map<SeatType, uint32_t> GetSeatTypeStats() const;
        std::unordered_map<uint64_t, uint32_t> GetVehicleOccupancyStats() const;

        // Event callbacks
        using VehicleEntryCallback = std::function<void(uint32_t playerId, uint64_t vehicleId, int32_t seatIndex, VehicleEntryResult result)>;
        using VehicleExitCallback = std::function<void(uint32_t playerId, uint64_t vehicleId, int32_t seatIndex, VehicleExitResult result)>;
        using SeatReservationCallback = std::function<void(uint32_t playerId, uint64_t vehicleId, int32_t seatIndex, SeatReservationResult result)>;
        using DriverChangeCallback = std::function<void(uint64_t vehicleId, uint32_t oldDriverId, uint32_t newDriverId)>;
        using VehicleOccupancyCallback = std::function<void(uint64_t vehicleId, const std::vector<uint32_t>& occupants)>;

        void SetVehicleEntryCallback(VehicleEntryCallback callback);
        void SetVehicleExitCallback(VehicleExitCallback callback);
        void SetSeatReservationCallback(SeatReservationCallback callback);
        void SetDriverChangeCallback(DriverChangeCallback callback);
        void SetVehicleOccupancyCallback(VehicleOccupancyCallback callback);

    private:
        MultiOccupancyManager() = default;
        ~MultiOccupancyManager() = default;
        MultiOccupancyManager(const MultiOccupancyManager&) = delete;
        MultiOccupancyManager& operator=(const MultiOccupancyManager&) = delete;

        // Internal data
        std::unordered_map<uint32_t, std::unique_ptr<PlayerOccupancyState>> m_playerStates;
        std::unordered_map<uint64_t, std::unique_ptr<OccupiedVehicleState>> m_occupiedVehicles;
        std::vector<SeatReservation> m_seatReservations;
        std::unordered_map<uint32_t, uint64_t> m_playerToVehicle;
        std::unordered_map<uint64_t, std::vector<uint32_t>> m_vehicleToPlayers;

        // Thread safety
        mutable std::shared_mutex m_statesMutex;
        mutable std::mutex m_callbacksMutex;
        mutable std::mutex m_reservationsMutex;

        // Update timing
        std::chrono::steady_clock::time_point m_lastUpdate;
        std::chrono::steady_clock::time_point m_lastCleanup;
        float m_updateInterval;

        // Statistics
        uint32_t m_totalVehicleEntries;
        uint32_t m_totalVehicleExits;
        uint32_t m_totalSeatReservations;
        uint32_t m_totalDriverChanges;

        // Event callbacks
        VehicleEntryCallback m_vehicleEntryCallback;
        VehicleExitCallback m_vehicleExitCallback;
        SeatReservationCallback m_seatReservationCallback;
        DriverChangeCallback m_driverChangeCallback;
        VehicleOccupancyCallback m_vehicleOccupancyCallback;

        // Internal methods
        void UpdateOccupancyStates(float deltaTime);
        void UpdatePlayerOccupancyStates(float deltaTime);
        void ProcessOccupancyExpirations();
        void ValidateOccupancyStates();

        OccupiedVehicleState* FindOrCreateVehicleState(uint64_t vehicleId);
        SeatReservation* FindSeatReservation(uint32_t playerId, uint64_t vehicleId);
        SeatReservation* FindSeatReservation(uint64_t vehicleId, int32_t seatIndex);
        bool RemoveSeatReservation(uint32_t playerId, uint64_t vehicleId);
        void ExpireSeatReservation(const SeatReservation& reservation);

        uint32_t DetermineVehicleMaxSeats(uint64_t vehicleId) const;
        bool ProcessVehicleEntry(uint32_t playerId, uint64_t vehicleId, int32_t seatIndex);
        bool ProcessVehicleExit(uint32_t playerId, uint64_t vehicleId);
        void UpdateDriverAssignment(uint64_t vehicleId, uint32_t newDriverId, uint32_t oldDriverId = 0);

        void UpdatePlayerToVehicleMapping(uint32_t playerId, uint64_t vehicleId, bool isActive);
        void UpdateVehicleToPlayersMapping(uint64_t vehicleId, uint32_t playerId, bool isActive);
        void RemovePlayerFromAllMappings(uint32_t playerId);

        void NotifyVehicleEntry(uint32_t playerId, uint64_t vehicleId, int32_t seatIndex, VehicleEntryResult result);
        void NotifyVehicleExit(uint32_t playerId, uint64_t vehicleId, int32_t seatIndex, VehicleExitResult result);
        void NotifySeatReservation(uint32_t playerId, uint64_t vehicleId, int32_t seatIndex, SeatReservationResult result);
        void NotifyDriverChange(uint64_t vehicleId, uint32_t oldDriverId, uint32_t newDriverId);
        void NotifyVehicleOccupancy(uint64_t vehicleId, const std::vector<uint32_t>& occupants);

        void SendVehicleEntryToClients(const VehicleEnterData& enterData);
        void SendVehicleExitToClients(const VehicleExitData& exitData);
        void SendSeatReservationToClients(const SeatReservationData& reservationData);
        void SendOccupancyUpdateToClients(const VehicleOccupancyUpdateData& occupancyData);
    };

    // Utility functions for multi-occupancy management
    namespace MultiOccupancyUtils
    {
        std::string SeatTypeToString(SeatType seatType);
        SeatType StringToSeatType(const std::string& seatStr);

        std::string SeatReservationResultToString(SeatReservationResult result);
        std::string VehicleEntryResultToString(VehicleEntryResult result);
        std::string VehicleExitResultToString(VehicleExitResult result);

        bool IsDriverSeat(int32_t seatIndex);
        bool IsPassengerSeat(int32_t seatIndex);
        bool IsRearSeat(int32_t seatIndex);

        uint32_t CalculateVehicleCapacity(const OccupiedVehicleState& vehicleState);
        float CalculateOccupancyPercentage(const OccupiedVehicleState& vehicleState);
        uint32_t HashOccupancyState(const PlayerOccupancyState& state);
        bool ShouldSyncOccupancyState(const PlayerOccupancyState& oldState, const PlayerOccupancyState& newState);
    }

    // Network message structures for client-server communication
    struct OccupancyStateUpdate
    {
        uint32_t playerId;
        uint64_t currentVehicleId;
        int32_t currentSeatIndex;
        bool isInVehicle;
        std::chrono::steady_clock::time_point updateTime;
        uint32_t syncVersion;
    };

    struct VehicleCapacityUpdate
    {
        uint64_t vehicleId;
        uint32_t maxSeats;
        uint32_t occupiedSeats;
        std::vector<SeatAssignment> seatAssignments;
        std::chrono::steady_clock::time_point updateTime;
    };

    struct SeatReservationUpdate
    {
        SeatReservationData reservationData;
        bool isExpiring;
        std::chrono::steady_clock::time_point updateTime;
    };

    struct DriverTransferUpdate
    {
        uint64_t vehicleId;
        uint32_t oldDriverId;
        uint32_t newDriverId;
        bool wasForced;
        std::chrono::steady_clock::time_point updateTime;
    };
}