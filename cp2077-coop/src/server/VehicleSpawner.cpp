#include "VehicleSpawner.hpp"
#include "RED4ext/Api/Runtime.hpp"
#include <random>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <cmath>

// Helper function for Vector3 distance calculation
static float CalculateDistance(const RED4ext::Vector3& a, const RED4ext::Vector3& b) {
    float dx = a.X - b.X;
    float dy = a.Y - b.Y;
    float dz = a.Z - b.Z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

namespace RED4ext
{
    // Static instance management
    VehicleSpawner& VehicleSpawner::GetInstance()
    {
        static VehicleSpawner instance;
        return instance;
    }

    uint64_t VehicleSpawnRequest::GenerateRequestId()
    {
        static std::random_device rd;
        static std::mt19937_64 gen(rd());
        return gen();
    }

    void VehicleSpawner::Initialize()
    {
        std::unique_lock lock(m_vehiclesMutex);

        m_spawnedVehicles.clear();
        m_playerVehicles.clear();
        m_pendingRequests.clear();

        // Set up default configuration
        m_maxVehiclesPerPlayer = 5;
        m_maxGlobalVehicles = 100;
        m_minSpawnDistance = 5.0f;
        m_conflictSearchRadius = 50.0f;
        m_abandonedVehicleTimeout = std::chrono::seconds(300);

        std::cout << "[VehicleSpawner] VehicleSpawner initialized" << std::endl;
    }

    void VehicleSpawner::Shutdown()
    {
        std::unique_lock lock(m_vehiclesMutex);

        // Clean up all vehicles
        for (const auto& [vehicleId, vehicle] : m_spawnedVehicles)
        {
            BroadcastVehicleDespawn(vehicleId);
        }

        m_spawnedVehicles.clear();
        m_playerVehicles.clear();
        m_pendingRequests.clear();

        std::cout << "[VehicleSpawner] VehicleSpawner shutdown complete" << std::endl;
    }

    void VehicleSpawner::Update()
    {
        ProcessPendingRequests();
        UpdateVehicleTracking();
        CleanupAbandonedVehicles();
    }

    SpawnResult VehicleSpawner::RequestVehicleSpawn(const VehicleSpawnRequest& request)
    {
        // Validate the spawn request
        if (!ValidateSpawnRequest(request))
        {
            return SpawnResult::InvalidVehicleType;
        }

        // Check vehicle limits
        if (!CheckVehicleLimits(request.playerId))
        {
            return SpawnResult::VehicleLimitReached;
        }

        // Check for spawn location conflicts
        if (!IsSpawnLocationValid(request.position, request.spawnRadius))
        {
            // Try to resolve conflicts
            VehicleSpawnRequest modifiedRequest = request;
            if (ResolveSpawnConflict(modifiedRequest))
            {
                return ProcessVehicleSpawn(modifiedRequest, SpawnResult::ConflictResolved);
            }
            else
            {
                return SpawnResult::InvalidLocation;
            }
        }

        return ProcessVehicleSpawn(request, SpawnResult::Success);
    }

    SpawnResult VehicleSpawner::ProcessVehicleSpawn(const VehicleSpawnRequest& request, SpawnResult result)
    {
        std::unique_lock lock(m_vehiclesMutex);

        // Create the spawned vehicle
        SpawnedVehicle vehicle(request.vehicleId, request.playerId, request.vehicleRecord,
                              request.position, request.rotation);

        // Add to tracking systems
        m_spawnedVehicles[request.vehicleId] = vehicle;
        m_playerVehicles[request.playerId].push_back(request.vehicleId);

        // Broadcast spawn to all clients
        BroadcastVehicleSpawn(vehicle);

        std::cout << "[VehicleSpawner] Vehicle " << vehicle.vehicleId << " spawned for player " << vehicle.ownerId
                  << " at position (" << request.position.X << ", " << request.position.Y << ", " << request.position.Z << ")" << std::endl;

        return result;
    }

    bool VehicleSpawner::DespawnVehicle(uint64_t vehicleId, uint32_t requestingPlayer)
    {
        std::unique_lock lock(m_vehiclesMutex);

        auto vehicleIt = m_spawnedVehicles.find(vehicleId);
        if (vehicleIt == m_spawnedVehicles.end())
        {
            return false;
        }

        const SpawnedVehicle& vehicle = vehicleIt->second;

        // Check permissions - only owner or admin can despawn
        if (vehicle.ownerId != requestingPlayer && !IsPlayerAdmin(requestingPlayer))
        {
            return false;
        }

        // Remove from tracking
        RemoveVehicleFromTracking(vehicleId);

        // Broadcast despawn
        BroadcastVehicleDespawn(vehicleId);

        std::cout << "[VehicleSpawner] Vehicle " << vehicleId << " despawned by player " << requestingPlayer << std::endl;
        return true;
    }

    bool VehicleSpawner::UpdateVehicleState(uint64_t vehicleId, const Vector3& position,
                                           const Vector4& rotation, float health, float fuel)
    {
        std::shared_lock lock(m_vehiclesMutex);

        auto vehicleIt = m_spawnedVehicles.find(vehicleId);
        if (vehicleIt == m_spawnedVehicles.end())
        {
            return false;
        }

        SpawnedVehicle& vehicle = vehicleIt->second;
        vehicle.currentPosition = position;
        vehicle.currentRotation = rotation;
        vehicle.health = health;
        vehicle.fuel = fuel;
        vehicle.lastUpdate = std::chrono::steady_clock::now();

        // Check for destruction
        if (health <= 0.0f && !vehicle.isDestroyed)
        {
            vehicle.isDestroyed = true;
            NotifyVehicleDestroyed(vehicleId);
        }

        return true;
    }

    SpawnedVehicle* VehicleSpawner::GetVehicle(uint64_t vehicleId)
    {
        std::shared_lock lock(m_vehiclesMutex);

        auto it = m_spawnedVehicles.find(vehicleId);
        return (it != m_spawnedVehicles.end()) ? &it->second : nullptr;
    }

    std::vector<SpawnedVehicle> VehicleSpawner::GetVehiclesInRadius(const Vector3& center, float radius)
    {
        std::shared_lock lock(m_vehiclesMutex);
        std::vector<SpawnedVehicle> result;

        for (const auto& [vehicleId, vehicle] : m_spawnedVehicles)
        {
            float distance = CalculateDistance(center, vehicle.currentPosition);
            if (distance <= radius)
            {
                result.push_back(vehicle);
            }
        }

        return result;
    }

    std::vector<SpawnedVehicle> VehicleSpawner::GetPlayerVehicles(uint32_t playerId)
    {
        std::shared_lock lock(m_vehiclesMutex);
        std::vector<SpawnedVehicle> result;

        auto playerIt = m_playerVehicles.find(playerId);
        if (playerIt != m_playerVehicles.end())
        {
            for (uint64_t vehicleId : playerIt->second)
            {
                auto vehicleIt = m_spawnedVehicles.find(vehicleId);
                if (vehicleIt != m_spawnedVehicles.end())
                {
                    result.push_back(vehicleIt->second);
                }
            }
        }

        return result;
    }

    bool VehicleSpawner::SetVehicleOccupancy(uint64_t vehicleId, uint32_t driverId, bool occupied)
    {
        std::unique_lock lock(m_vehiclesMutex);

        auto vehicleIt = m_spawnedVehicles.find(vehicleId);
        if (vehicleIt == m_spawnedVehicles.end())
        {
            return false;
        }

        SpawnedVehicle& vehicle = vehicleIt->second;
        vehicle.isOccupied = occupied;
        vehicle.driverId = occupied ? driverId : 0;
        vehicle.lastUpdate = std::chrono::steady_clock::now();

        // Broadcast occupancy change
        BroadcastVehicleUpdate(vehicle);
        return true;
    }

    bool VehicleSpawner::IsVehicleOccupied(uint64_t vehicleId)
    {
        std::shared_lock lock(m_vehiclesMutex);

        auto vehicleIt = m_spawnedVehicles.find(vehicleId);
        return (vehicleIt != m_spawnedVehicles.end()) ? vehicleIt->second.isOccupied : false;
    }

    uint32_t VehicleSpawner::GetVehicleDriver(uint64_t vehicleId)
    {
        std::shared_lock lock(m_vehiclesMutex);

        auto vehicleIt = m_spawnedVehicles.find(vehicleId);
        return (vehicleIt != m_spawnedVehicles.end()) ? vehicleIt->second.driverId : 0;
    }

    bool VehicleSpawner::IsSpawnLocationValid(const Vector3& position, float radius)
    {
        // Check if position is accessible
        if (!VehicleUtils::IsPositionAccessible(position))
        {
            return false;
        }

        // Check for conflicts with existing vehicles
        return !CheckSpawnConflicts(position, radius);
    }

    Vector3 VehicleSpawner::FindAlternativeSpawnLocation(const Vector3& preferredPosition, float searchRadius)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159f);
        std::uniform_real_distribution<float> radiusDist(m_minSpawnDistance, searchRadius);

        for (int attempts = 0; attempts < 20; ++attempts)
        {
            float angle = angleDist(gen);
            float distance = radiusDist(gen);

            Vector3 testPosition;
            testPosition.X = preferredPosition.X + distance * std::cos(angle);
            testPosition.Y = preferredPosition.Y + distance * std::sin(angle);
            testPosition.Z = preferredPosition.Z;

            if (IsSpawnLocationValid(testPosition, m_minSpawnDistance))
            {
                return testPosition;
            }
        }

        // Fallback: return original position if no alternative found
        return preferredPosition;
    }

    bool VehicleSpawner::ResolveSpawnConflict(VehicleSpawnRequest& request)
    {
        Vector3 alternativeLocation = FindAlternativeSpawnLocation(request.position, m_conflictSearchRadius);

        if (IsSpawnLocationValid(alternativeLocation, request.spawnRadius))
        {
            request.position = alternativeLocation;
            return true;
        }

        return false;
    }

    void VehicleSpawner::CleanupAbandonedVehicles()
    {
        std::unique_lock lock(m_vehiclesMutex);
        std::vector<uint64_t> abandonedVehicles;

        for (const auto& [vehicleId, vehicle] : m_spawnedVehicles)
        {
            if (IsVehicleAbandoned(vehicle))
            {
                abandonedVehicles.push_back(vehicleId);
            }
        }

        for (uint64_t vehicleId : abandonedVehicles)
        {
            RemoveVehicleFromTracking(vehicleId);
            BroadcastVehicleDespawn(vehicleId);
            std::cout << "[VehicleSpawner] Cleaned up abandoned vehicle " << vehicleId << std::endl;
        }
    }

    void VehicleSpawner::CleanupPlayerVehicles(uint32_t playerId)
    {
        std::unique_lock lock(m_vehiclesMutex);

        auto playerIt = m_playerVehicles.find(playerId);
        if (playerIt == m_playerVehicles.end())
        {
            return;
        }

        for (uint64_t vehicleId : playerIt->second)
        {
            auto vehicleIt = m_spawnedVehicles.find(vehicleId);
            if (vehicleIt != m_spawnedVehicles.end())
            {
                BroadcastVehicleDespawn(vehicleId);
                m_spawnedVehicles.erase(vehicleIt);
            }
        }

        m_playerVehicles.erase(playerIt);
        std::cout << "[VehicleSpawner] Cleaned up all vehicles for player " << playerId << std::endl;
    }

    // Private methods implementation
    bool VehicleSpawner::ValidateSpawnRequest(const VehicleSpawnRequest& request)
    {
        // Check if vehicle record is valid
        if (!IsValidVehicleRecord(request.vehicleRecord))
        {
            return false;
        }

        // Check if vehicle ID is unique
        std::shared_lock lock(m_vehiclesMutex);
        return m_spawnedVehicles.find(request.vehicleId) == m_spawnedVehicles.end();
    }

    bool VehicleSpawner::CheckVehicleLimits(uint32_t playerId)
    {
        std::shared_lock lock(m_vehiclesMutex);

        // Check global limit
        if (m_spawnedVehicles.size() >= m_maxGlobalVehicles)
        {
            return false;
        }

        // Check per-player limit
        auto playerIt = m_playerVehicles.find(playerId);
        if (playerIt != m_playerVehicles.end())
        {
            return playerIt->second.size() < m_maxVehiclesPerPlayer;
        }

        return true;
    }

    bool VehicleSpawner::CheckSpawnConflicts(const Vector3& position, float radius, uint64_t excludeVehicleId)
    {
        std::shared_lock lock(m_vehiclesMutex);

        for (const auto& [vehicleId, vehicle] : m_spawnedVehicles)
        {
            if (vehicleId == excludeVehicleId) continue;

            float distance = CalculateDistance(position, vehicle.currentPosition);
            if (distance < radius + m_minSpawnDistance)
            {
                return true; // Conflict found
            }
        }

        return false; // No conflicts
    }

    void VehicleSpawner::ProcessPendingRequests()
    {
        std::lock_guard<std::mutex> lock(m_requestsMutex);

        for (auto it = m_pendingRequests.begin(); it != m_pendingRequests.end();)
        {
            SpawnResult result = RequestVehicleSpawn(*it);
            if (result != SpawnResult::NetworkError)
            {
                it = m_pendingRequests.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void VehicleSpawner::UpdateVehicleTracking()
    {
        // This would integrate with the game's vehicle system
        // to track position updates and state changes
    }

    bool VehicleSpawner::IsValidVehicleRecord(TweakDBID vehicleRecord)
    {
        // Validate against known vehicle records
        return vehicleRecord.value != 0;
    }

    uint64_t VehicleSpawner::GenerateVehicleId()
    {
        static std::random_device rd;
        static std::mt19937_64 gen(rd());

        uint64_t id;
        do {
            id = gen();
        } while (m_spawnedVehicles.find(id) != m_spawnedVehicles.end());

        return id;
    }

    void VehicleSpawner::RemoveVehicleFromTracking(uint64_t vehicleId)
    {
        auto vehicleIt = m_spawnedVehicles.find(vehicleId);
        if (vehicleIt == m_spawnedVehicles.end()) return;

        uint32_t ownerId = vehicleIt->second.ownerId;
        m_spawnedVehicles.erase(vehicleIt);

        // Remove from player vehicles
        auto playerIt = m_playerVehicles.find(ownerId);
        if (playerIt != m_playerVehicles.end())
        {
            auto& vehicles = playerIt->second;
            vehicles.erase(std::remove(vehicles.begin(), vehicles.end(), vehicleId), vehicles.end());

            if (vehicles.empty())
            {
                m_playerVehicles.erase(playerIt);
            }
        }
    }

    bool VehicleSpawner::IsVehicleAbandoned(const SpawnedVehicle& vehicle) const
    {
        auto timeSinceUpdate = std::chrono::steady_clock::now() - vehicle.lastUpdate;
        return timeSinceUpdate > m_abandonedVehicleTimeout && !vehicle.isOccupied;
    }

    void VehicleSpawner::NotifyVehicleDestroyed(uint64_t vehicleId)
    {
        // Broadcast vehicle destruction event
        std::cout << "[VehicleSpawner] Vehicle " << vehicleId << " destroyed" << std::endl;
    }

    bool VehicleSpawner::IsPlayerAdmin(uint32_t playerId)
    {
        // This would check against admin permissions system
        return false; // Placeholder
    }

    // Network integration placeholders - these would integrate with the existing network system
    void VehicleSpawner::BroadcastVehicleSpawn(const SpawnedVehicle& vehicle)
    {
        // Broadcast vehicle spawn to all connected clients
        std::cout << "[VehicleSpawner] Broadcasting vehicle spawn: " << vehicle.vehicleId << std::endl;
    }

    void VehicleSpawner::BroadcastVehicleDespawn(uint64_t vehicleId)
    {
        // Broadcast vehicle despawn to all connected clients
        std::cout << "[VehicleSpawner] Broadcasting vehicle despawn: " << vehicleId << std::endl;
    }

    void VehicleSpawner::BroadcastVehicleUpdate(const SpawnedVehicle& vehicle)
    {
        // Broadcast vehicle state update to all connected clients
        std::cout << "[VehicleSpawner] Broadcasting vehicle update: " << vehicle.vehicleId << std::endl;
    }

    uint32_t VehicleSpawner::GetActiveVehicleCount() const
    {
        std::shared_lock lock(m_vehiclesMutex);
        return static_cast<uint32_t>(m_spawnedVehicles.size());
    }

    uint32_t VehicleSpawner::GetPlayerVehicleCount(uint32_t playerId) const
    {
        std::shared_lock lock(m_vehiclesMutex);
        auto playerIt = m_playerVehicles.find(playerId);
        return playerIt != m_playerVehicles.end() ?
               static_cast<uint32_t>(playerIt->second.size()) : 0;
    }

    std::vector<uint64_t> VehicleSpawner::GetActiveVehicleIds() const
    {
        std::shared_lock lock(m_vehiclesMutex);
        std::vector<uint64_t> ids;

        for (const auto& [vehicleId, vehicle] : m_spawnedVehicles)
        {
            ids.push_back(vehicleId);
        }

        return ids;
    }

    void VehicleSpawner::SetMaxVehiclesPerPlayer(uint32_t maxVehicles)
    {
        m_maxVehiclesPerPlayer = maxVehicles;
    }

    void VehicleSpawner::SetGlobalVehicleLimit(uint32_t maxGlobalVehicles)
    {
        m_maxGlobalVehicles = maxGlobalVehicles;
    }

    // Utility functions implementation
    namespace VehicleUtils
    {
        bool IsPositionAccessible(const Vector3& position)
        {
            // This would check if the position is accessible for vehicle spawning
            // (not inside buildings, not too steep, etc.)
            return true; // Placeholder
        }

        bool IsPositionOnRoad(const Vector3& position)
        {
            // This would check if the position is on a valid road surface
            return true; // Placeholder
        }

        float CalculateVehicleDistance(const SpawnedVehicle& vehicle1, const SpawnedVehicle& vehicle2)
        {
            return CalculateDistance(vehicle1.currentPosition, vehicle2.currentPosition);
        }

        TweakDBID GetRandomVehicleRecord()
        {
            // This would return a random valid vehicle record
            return TweakDBID(0ULL); // Placeholder
        }

        std::vector<TweakDBID> GetAvailableVehicleRecords()
        {
            // This would return all available vehicle records
            return {}; // Placeholder
        }

        bool CanPlayerSpawnVehicle(uint32_t playerId, TweakDBID vehicleRecord)
        {
            // This would check if player has permissions to spawn this vehicle type
            return true; // Placeholder
        }
    }
}