#pragma once

#include "RED4ext/Api/Sdk.hpp"
#include "RED4ext/Common.hpp"
#include "RED4ext/NativeTypes.hpp"
#include "RED4ext/Scripting/Natives/Generated/Vector3.hpp"
#include "RED4ext/Scripting/Natives/Generated/Vector4.hpp"
#include <unordered_map>
#include <vector>
#include <mutex>
#include <chrono>

namespace RED4ext
{
    // Vehicle spawn request structure
    struct VehicleSpawnRequest
    {
        uint32_t playerId;
        uint64_t vehicleId;
        TweakDBID vehicleRecord;
        Vector3 position;
        Vector4 rotation;
        float spawnRadius;
        uint64_t requestId;
        std::chrono::steady_clock::time_point timestamp;

        VehicleSpawnRequest() = default;
        VehicleSpawnRequest(uint32_t pId, uint64_t vId, TweakDBID record,
                          const Vector3& pos, const Vector4& rot, float radius)
            : playerId(pId), vehicleId(vId), vehicleRecord(record),
              position(pos), rotation(rot), spawnRadius(radius),
              requestId(GenerateRequestId()), timestamp(std::chrono::steady_clock::now()) {}

    private:
        static uint64_t GenerateRequestId();
    };

    // Spawned vehicle state
    struct SpawnedVehicle
    {
        uint64_t vehicleId;
        uint32_t ownerId;
        TweakDBID vehicleRecord;
        Vector3 currentPosition;
        Vector4 currentRotation;
        bool isOccupied;
        uint32_t driverId;
        float health;
        float fuel;
        bool isDestroyed;
        std::chrono::steady_clock::time_point lastUpdate;

        SpawnedVehicle() = default;
        SpawnedVehicle(uint64_t vId, uint32_t owner, TweakDBID record,
                      const Vector3& pos, const Vector4& rot)
            : vehicleId(vId), ownerId(owner), vehicleRecord(record),
              currentPosition(pos), currentRotation(rot), isOccupied(false),
              driverId(0), health(100.0f), fuel(100.0f), isDestroyed(false),
              lastUpdate(std::chrono::steady_clock::now()) {}
    };

    // Vehicle conflict resolution result
    enum class SpawnResult : uint8_t
    {
        Success = 0,
        ConflictResolved = 1,
        InvalidLocation = 2,
        VehicleLimitReached = 3,
        DuplicateRequest = 4,
        InvalidVehicleType = 5,
        InsufficientPermissions = 6,
        NetworkError = 7
    };

    class VehicleSpawner
    {
    public:
        static VehicleSpawner& GetInstance();

        // Core spawning functionality
        SpawnResult RequestVehicleSpawn(const VehicleSpawnRequest& request);
        bool DespawnVehicle(uint64_t vehicleId, uint32_t requestingPlayer);
        bool UpdateVehicleState(uint64_t vehicleId, const Vector3& position,
                               const Vector4& rotation, float health, float fuel);

        // Vehicle management
        SpawnedVehicle* GetVehicle(uint64_t vehicleId);
        std::vector<SpawnedVehicle> GetVehiclesInRadius(const Vector3& center, float radius);
        std::vector<SpawnedVehicle> GetPlayerVehicles(uint32_t playerId);

        // Occupancy management
        bool SetVehicleOccupancy(uint64_t vehicleId, uint32_t driverId, bool occupied);
        bool IsVehicleOccupied(uint64_t vehicleId);
        uint32_t GetVehicleDriver(uint64_t vehicleId);

        // Conflict resolution
        bool IsSpawnLocationValid(const Vector3& position, float radius);
        Vector3 FindAlternativeSpawnLocation(const Vector3& preferredPosition, float searchRadius);
        bool ResolveSpawnConflict(VehicleSpawnRequest& request);

        // Administrative functions
        void SetMaxVehiclesPerPlayer(uint32_t maxVehicles);
        void SetGlobalVehicleLimit(uint32_t maxGlobalVehicles);
        void CleanupAbandonedVehicles();
        void CleanupPlayerVehicles(uint32_t playerId);

        // Network synchronization
        void BroadcastVehicleSpawn(const SpawnedVehicle& vehicle);
        void BroadcastVehicleDespawn(uint64_t vehicleId);
        void BroadcastVehicleUpdate(const SpawnedVehicle& vehicle);

        // Statistics and monitoring
        uint32_t GetActiveVehicleCount() const;
        uint32_t GetPlayerVehicleCount(uint32_t playerId) const;
        std::vector<uint64_t> GetActiveVehicleIds() const;

        // Initialization and cleanup
        void Initialize();
        void Shutdown();
        void Update();

    private:
        VehicleSpawner() = default;
        ~VehicleSpawner() = default;
        VehicleSpawner(const VehicleSpawner&) = delete;
        VehicleSpawner& operator=(const VehicleSpawner&) = delete;

        // Internal state
        std::unordered_map<uint64_t, SpawnedVehicle> m_spawnedVehicles;
        std::unordered_map<uint32_t, std::vector<uint64_t>> m_playerVehicles;
        std::vector<VehicleSpawnRequest> m_pendingRequests;

        // Configuration
        uint32_t m_maxVehiclesPerPlayer = 5;
        uint32_t m_maxGlobalVehicles = 100;
        float m_minSpawnDistance = 5.0f;
        float m_conflictSearchRadius = 50.0f;
        std::chrono::seconds m_abandonedVehicleTimeout{300}; // 5 minutes

        // Thread safety
        mutable std::shared_mutex m_vehiclesMutex;
        mutable std::mutex m_requestsMutex;

        // Internal methods
        bool ValidateSpawnRequest(const VehicleSpawnRequest& request);
        bool CheckVehicleLimits(uint32_t playerId);
        bool CheckSpawnConflicts(const Vector3& position, float radius,
                                uint64_t excludeVehicleId = 0);
        void ProcessPendingRequests();
        void UpdateVehicleTracking();
        bool IsValidVehicleRecord(TweakDBID vehicleRecord);
        uint64_t GenerateVehicleId();
        SpawnResult ProcessVehicleSpawn(const VehicleSpawnRequest& request, SpawnResult result);
        bool IsPlayerAdmin(uint32_t playerId);

        // Cleanup helpers
        void RemoveVehicleFromTracking(uint64_t vehicleId);
        bool IsVehicleAbandoned(const SpawnedVehicle& vehicle) const;
        void NotifyVehicleDestroyed(uint64_t vehicleId);
    };

    // Utility functions for vehicle management
    namespace VehicleUtils
    {
        bool IsPositionAccessible(const Vector3& position);
        bool IsPositionOnRoad(const Vector3& position);
        float CalculateVehicleDistance(const SpawnedVehicle& vehicle1, const SpawnedVehicle& vehicle2);
        TweakDBID GetRandomVehicleRecord();
        std::vector<TweakDBID> GetAvailableVehicleRecords();
        bool CanPlayerSpawnVehicle(uint32_t playerId, TweakDBID vehicleRecord);
    }
}