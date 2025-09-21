#include "VehicleController.hpp"
#include "../core/GameClock.hpp"
#include "../core/Hash.hpp"
#include "../core/SaveFork.hpp"
#include "../core/SessionState.hpp"
#include "../net/Connection.hpp"
#include "../net/Net.hpp"
#include "../net/Packets.hpp"
#include "../physics/CarPhysics.hpp"
#include "../physics/EnhancedVehiclePhysics.hpp"
#include <cmath>
#include <iostream>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <chrono>
#include <cstring>

bool Nav_FindClosestRoad(const RED4ext::Vector3& pos, RED4ext::Vector3& out)
{
    // Simple placeholder implementation - just return the input position
    // In a full implementation this would find the nearest road
    out = pos;
    return true;
}

// Utility function to get current time in milliseconds
static uint64_t GetCurrentTimeMs()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

// Validate vehicle customization data
static bool ValidateVehicleCustomization(const CoopNet::VehicleCustomizationPacket& customization)
{
    // Check color ID is within reasonable bounds
    if (customization.colorId > 1000) {
        return false;
    }
    
    // Check plate text is null-terminated and reasonable length
    size_t plateLen = strnlen(customization.plateText, sizeof(customization.plateText));
    if (plateLen == 0 || plateLen >= sizeof(customization.plateText)) {
        return false;
    }
    
    // Check for valid characters in plate text (alphanumeric and spaces)
    for (size_t i = 0; i < plateLen; ++i) {
        char c = customization.plateText[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || 
              (c >= '0' && c <= '9') || c == ' ' || c == '-')) {
            return false;
        }
    }
    
    // Check modification IDs are within reasonable bounds
    for (int i = 0; i < 8; ++i) {
        if (customization.modifications[i] > 10000) {
            return false;
        }
    }
    
    return true;
}

namespace CoopNet
{
struct VehicleCustomization
{
    uint32_t colorId = 0;
    char plateText[16] = {0};
    uint32_t modIds[8] = {0}; // Up to 8 customization mods
    uint8_t modCount = 0;
};

// Enhanced VehicleController Implementation
VehicleController& VehicleController::GetInstance()
{
    static VehicleController instance;
    return instance;
}

void VehicleController::Initialize()
{
    // Initialize MultiOccupancyManager integration
    auto& occupancyManager = RED4ext::MultiOccupancyManager::GetInstance();

    // Set up callbacks for vehicle occupancy events
    occupancyManager.SetVehicleEntryCallback(
        [this](uint32_t playerId, uint64_t vehicleId, int32_t seatIndex, RED4ext::VehicleEntryResult result) {
            this->OnVehicleEntryResult(playerId, vehicleId, seatIndex, result);
        });

    occupancyManager.SetVehicleExitCallback(
        [this](uint32_t playerId, uint64_t vehicleId, int32_t seatIndex, RED4ext::VehicleExitResult result) {
            this->OnVehicleExitResult(playerId, vehicleId, seatIndex, result);
        });

    occupancyManager.SetSeatReservationCallback(
        [this](uint32_t playerId, uint64_t vehicleId, int32_t seatIndex, RED4ext::SeatReservationResult result) {
            this->OnSeatReservationResult(playerId, vehicleId, seatIndex, result);
        });

    occupancyManager.SetDriverChangeCallback(
        [this](uint64_t vehicleId, uint32_t oldDriverId, uint32_t newDriverId) {
            this->OnDriverChange(vehicleId, oldDriverId, newDriverId);
        });

    std::cout << "[VehicleController] Enhanced vehicle controller initialized with MultiOccupancyManager integration" << std::endl;
}

void VehicleController::Shutdown()
{
    // Clean up any resources
    std::cout << "[VehicleController] Enhanced vehicle controller shutdown" << std::endl;
}

void VehicleController::ServerTick(float dt)
{
    // Update MultiOccupancyManager
    auto& occupancyManager = RED4ext::MultiOccupancyManager::GetInstance();
    occupancyManager.Update();

    // Sync any legacy vehicle states that need updating
    std::lock_guard lock(VehicleController::g_vehicleMutex);
    for (const auto& [vehicleId, vehicle] : VehicleController::g_vehicles) {
        if (!vehicle.destroyed) {
            SyncOccupancyWithLegacyState(vehicleId);
        }
    }
}

void VehicleController::PhysicsStep(float dt)
{
    // Call legacy physics step
    VehicleController_PhysicsStep(dt);
}

void VehicleController::Spawn(uint32_t archetype, uint32_t paint, const TransformSnap& t)
{
    SpawnPhaseVehicle(archetype, paint, t, 0u);
}

void VehicleController::SpawnPhaseVehicle(uint32_t archetype, uint32_t paint, const TransformSnap& t, uint32_t phaseId)
{
    // Call legacy implementation
    VehicleController_SpawnPhaseVehicle(archetype, paint, t, phaseId);
}

void VehicleController::HandleSummon(CoopNet::Connection* c, uint32_t vehId, const TransformSnap& t)
{
    // Call legacy implementation
    VehicleController_HandleSummon(c, vehId, t);
}

void VehicleController::RemovePeer(uint32_t peerId)
{
    // Update MultiOccupancyManager
    uint32_t playerId = PeerIdToPlayerId(peerId);
    auto& occupancyManager = RED4ext::MultiOccupancyManager::GetInstance();
    occupancyManager.ForcePlayerExitVehicle(playerId);

    // Call legacy implementation
    VehicleController_RemovePeer(peerId);
}

void VehicleController::HandleSeatRequest(CoopNet::Connection* c, uint32_t vehicleId, uint8_t seatIdx)
{
    // Use enhanced vehicle entry instead
    HandleVehicleEntry(c, vehicleId, static_cast<int32_t>(seatIdx));
}

void VehicleController::ApplyDamage(uint16_t dmg, bool side)
{
    // Call legacy implementation
    VehicleController_ApplyDamage(dmg, side);
}

void VehicleController::HandleHit(uint32_t vehicleId, uint16_t dmg, bool side)
{
    // Call legacy implementation
    VehicleController_HandleHit(vehicleId, dmg, side);
}

void VehicleController::ApplyHitValidated(uint32_t attackerPeerId, uint32_t vehicleId, uint16_t dmg, bool side)
{
    // Call legacy implementation
    VehicleController_ApplyHitValidated(attackerPeerId, vehicleId, dmg, side);
}

void VehicleController::HandleTowRequest(CoopNet::Connection* c, const RED4ext::Vector3& pos)
{
    // Call legacy implementation
    VehicleController_HandleTowRequest(c, pos);
}

uint32_t VehicleController::GetPeerVehicleId(uint32_t peerId)
{
    // Try MultiOccupancyManager first
    uint32_t playerId = PeerIdToPlayerId(peerId);
    auto& occupancyManager = RED4ext::MultiOccupancyManager::GetInstance();
    uint64_t vehicleId = occupancyManager.GetPlayerCurrentVehicle(playerId);

    if (vehicleId != 0) {
        return static_cast<uint32_t>(vehicleId);
    }

    // Fallback to legacy implementation
    return VehicleController_GetPeerVehicleId(peerId);
}

void VehicleController::HandleVehicleEntry(CoopNet::Connection* c, uint32_t vehicleId, int32_t preferredSeat)
{
    if (!ValidateConnection(c)) {
        return;
    }

    uint32_t playerId = PeerIdToPlayerId(c->peerId);
    auto& occupancyManager = RED4ext::MultiOccupancyManager::GetInstance();

    // Use MultiOccupancyManager for enhanced vehicle entry
    auto result = occupancyManager.RequestVehicleEntry(playerId, vehicleId, preferredSeat);

    std::cout << "[VehicleController] Vehicle entry request: Player " << playerId
              << " -> Vehicle " << vehicleId << " (Seat " << preferredSeat << "): "
              << static_cast<int>(result) << std::endl;
}

void VehicleController::HandleVehicleExit(CoopNet::Connection* c, uint32_t vehicleId)
{
    if (!ValidateConnection(c)) {
        return;
    }

    uint32_t playerId = PeerIdToPlayerId(c->peerId);
    auto& occupancyManager = RED4ext::MultiOccupancyManager::GetInstance();

    // Use MultiOccupancyManager for enhanced vehicle exit
    auto result = occupancyManager.RequestVehicleExit(playerId, vehicleId);

    std::cout << "[VehicleController] Vehicle exit request: Player " << playerId
              << " -> Vehicle " << vehicleId << ": " << static_cast<int>(result) << std::endl;
}

void VehicleController::HandleSeatReservation(CoopNet::Connection* c, uint32_t vehicleId, int32_t preferredSeat)
{
    if (!ValidateConnection(c)) {
        return;
    }

    uint32_t playerId = PeerIdToPlayerId(c->peerId);
    auto& occupancyManager = RED4ext::MultiOccupancyManager::GetInstance();

    // Use MultiOccupancyManager for seat reservation
    auto result = occupancyManager.RequestSeatReservation(playerId, vehicleId, preferredSeat);

    std::cout << "[VehicleController] Seat reservation request: Player " << playerId
              << " -> Vehicle " << vehicleId << " (Seat " << preferredSeat << "): "
              << static_cast<int>(result) << std::endl;
}

void VehicleController::HandleDriverTransfer(CoopNet::Connection* c, uint32_t vehicleId, uint32_t newDriverId)
{
    if (!ValidateConnection(c)) {
        return;
    }

    auto& occupancyManager = RED4ext::MultiOccupancyManager::GetInstance();

    // Validate that the new driver is in the vehicle
    if (!occupancyManager.IsPlayerInVehicle(newDriverId)) {
        std::cout << "[VehicleController] Driver transfer failed: Player " << newDriverId
                  << " is not in vehicle " << vehicleId << std::endl;
        return;
    }

    bool success = occupancyManager.TransferVehicleControl(vehicleId, newDriverId);

    std::cout << "[VehicleController] Driver transfer request: Vehicle " << vehicleId
              << " -> New Driver " << newDriverId << ": " << (success ? "Success" : "Failed") << std::endl;
}

void VehicleController::RegisterVehicleWithOccupancyManager(uint64_t vehicleId, uint32_t maxSeats)
{
    auto& occupancyManager = RED4ext::MultiOccupancyManager::GetInstance();

    if (occupancyManager.RegisterVehicle(vehicleId, maxSeats)) {
        std::cout << "[VehicleController] Vehicle " << vehicleId
                  << " registered with MultiOccupancyManager (Max seats: " << maxSeats << ")" << std::endl;
    } else {
        std::cout << "[VehicleController] Failed to register vehicle " << vehicleId
                  << " with MultiOccupancyManager" << std::endl;
    }
}

void VehicleController::SyncOccupancyWithLegacyState(uint64_t vehicleId)
{
    std::lock_guard lock(VehicleController::g_vehicleMutex);
    auto it = VehicleController::g_vehicles.find(vehicleId);
    if (it == VehicleController::g_vehicles.end()) {
        return;
    }

    auto& legacyVehicle = it->second;
    auto& occupancyManager = RED4ext::MultiOccupancyManager::GetInstance();

    // Sync legacy seat assignments with MultiOccupancyManager
    for (int i = 0; i < 4; ++i) {
        if (legacyVehicle.seat[i] != 0) {
            uint32_t playerId = PeerIdToPlayerId(legacyVehicle.seat[i]);
            if (playerId != 0) {
                // Force sync the player's vehicle state
                auto result = occupancyManager.RequestVehicleEntry(playerId, vehicleId, i);
                if (result != RED4ext::VehicleEntryResult::Success) {
                    std::cout << "[VehicleController] Failed to sync legacy seat " << i
                              << " for player " << playerId << " in vehicle " << vehicleId << std::endl;
                }
            }
        }
    }
}

bool VehicleController::IsVehicleRegistered(uint64_t vehicleId) const
{
    auto& occupancyManager = RED4ext::MultiOccupancyManager::GetInstance();
    return occupancyManager.GetVehicleState(vehicleId) != nullptr;
}

std::vector<uint32_t> VehicleController::GetVehicleOccupants(uint64_t vehicleId) const
{
    auto& occupancyManager = RED4ext::MultiOccupancyManager::GetInstance();
    return occupancyManager.GetVehicleOccupants(vehicleId);
}

uint32_t VehicleController::GetVehicleDriver(uint64_t vehicleId) const
{
    auto& occupancyManager = RED4ext::MultiOccupancyManager::GetInstance();
    return occupancyManager.GetVehicleDriver(vehicleId);
}

uint32_t VehicleController::GetTotalVehicles() const
{
    std::lock_guard lock(VehicleController::g_vehicleMutex);
    return static_cast<uint32_t>(VehicleController::g_vehicles.size());
}

uint32_t VehicleController::GetActiveVehicles() const
{
    std::lock_guard lock(VehicleController::g_vehicleMutex);
    uint32_t count = 0;
    for (const auto& [vehicleId, vehicle] : VehicleController::g_vehicles) {
        if (!vehicle.destroyed) {
            count++;
        }
    }
    return count;
}

// Event handlers for MultiOccupancyManager callbacks
void VehicleController::OnVehicleEntryResult(uint32_t playerId, uint64_t vehicleId, int32_t seatIndex, RED4ext::VehicleEntryResult result)
{
    uint32_t peerId = PlayerIdToPeerId(playerId);

    if (result == RED4ext::VehicleEntryResult::Success) {
        // Update legacy state for compatibility
        std::lock_guard lock(VehicleController::g_vehicleMutex);
        auto it = VehicleController::g_vehicles.find(vehicleId);
        if (it != VehicleController::g_vehicles.end() && seatIndex >= 0 && seatIndex < 4) {
            it->second.seat[seatIndex] = peerId;
            if (seatIndex == 0) {
                it->second.owner = peerId; // Driver becomes owner
            }
        }

        // Broadcast seat assignment
        SeatAssignPacket pkt{peerId, static_cast<uint32_t>(vehicleId), static_cast<uint8_t>(seatIndex)};
        Net_Broadcast(EMsg::SeatAssign, &pkt, sizeof(pkt));

        std::cout << "[VehicleController] Player " << playerId << " successfully entered vehicle "
                  << vehicleId << " at seat " << seatIndex << std::endl;
    } else {
        std::cout << "[VehicleController] Player " << playerId << " failed to enter vehicle "
                  << vehicleId << ": " << static_cast<int>(result) << std::endl;
    }
}

void VehicleController::OnVehicleExitResult(uint32_t playerId, uint64_t vehicleId, int32_t seatIndex, RED4ext::VehicleExitResult result)
{
    uint32_t peerId = PlayerIdToPeerId(playerId);

    if (result == RED4ext::VehicleExitResult::Success) {
        // Update legacy state for compatibility
        std::lock_guard lock(VehicleController::g_vehicleMutex);
        auto it = VehicleController::g_vehicles.find(vehicleId);
        if (it != VehicleController::g_vehicles.end() && seatIndex >= 0 && seatIndex < 4) {
            it->second.seat[seatIndex] = 0;
            if (seatIndex == 0) {
                it->second.owner = 0; // Clear ownership if driver exits
            }
        }

        std::cout << "[VehicleController] Player " << playerId << " successfully exited vehicle "
                  << vehicleId << " from seat " << seatIndex << std::endl;
    } else {
        std::cout << "[VehicleController] Player " << playerId << " failed to exit vehicle "
                  << vehicleId << ": " << static_cast<int>(result) << std::endl;
    }
}

void VehicleController::OnSeatReservationResult(uint32_t playerId, uint64_t vehicleId, int32_t seatIndex, RED4ext::SeatReservationResult result)
{
    std::cout << "[VehicleController] Seat reservation for player " << playerId
              << " in vehicle " << vehicleId << " (Seat " << seatIndex << "): "
              << static_cast<int>(result) << std::endl;
}

void VehicleController::OnDriverChange(uint64_t vehicleId, uint32_t oldDriverId, uint32_t newDriverId)
{
    // Update legacy state for compatibility
    std::lock_guard lock(VehicleController::g_vehicleMutex);
    auto it = VehicleController::g_vehicles.find(vehicleId);
    if (it != VehicleController::g_vehicles.end()) {
        uint32_t oldPeerId = PlayerIdToPeerId(oldDriverId);
        uint32_t newPeerId = PlayerIdToPeerId(newDriverId);

        // Move old driver to passenger seat if available
        if (it->second.seat[1] == 0) {
            it->second.seat[1] = oldPeerId;
        }

        // Set new driver
        it->second.seat[0] = newPeerId;
        it->second.owner = newPeerId;
    }

    std::cout << "[VehicleController] Driver change in vehicle " << vehicleId
              << ": " << oldDriverId << " -> " << newDriverId << std::endl;
}

// Helper methods
void VehicleController::BroadcastVehicleUpdate(uint32_t vehicleId)
{
    // Broadcast vehicle state update to all clients
    std::lock_guard lock(VehicleController::g_vehicleMutex);
    auto it = VehicleController::g_vehicles.find(vehicleId);
    if (it != VehicleController::g_vehicles.end()) {
        // Would broadcast comprehensive vehicle update including occupancy
        std::cout << "[VehicleController] Broadcasting update for vehicle " << vehicleId << std::endl;
    }
}

bool VehicleController::ValidateConnection(CoopNet::Connection* c) const
{
    return c != nullptr && c->peerId != 0;
}

uint32_t VehicleController::PeerIdToPlayerId(uint32_t peerId) const
{
    // Simple mapping - in a real implementation, this would use a player registry
    return peerId;
}

uint32_t VehicleController::PlayerIdToPeerId(uint32_t playerId) const
{
    // Simple mapping - in a real implementation, this would use a player registry
    return playerId;
}

// Define static class members
std::unordered_map<uint32_t, VehicleController::VehicleState> VehicleController::VehicleController::g_vehicles;
std::mutex VehicleController::VehicleController::g_vehicleMutex;
std::atomic<uint32_t> VehicleController::VehicleController::g_nextVehId{1};

void VehicleController_SpawnPhaseVehicle(uint32_t archetype, uint32_t paint, const TransformSnap& t, uint32_t phaseId)
{
    std::lock_guard lock(VehicleController::VehicleController::g_vehicleMutex);
    uint32_t id = VehicleController::VehicleController::g_nextVehId++;
    VehicleController::VehicleController::VehicleState v{};
    v.id = id;
    v.phaseId = phaseId;
    v.archetype = archetype;
    v.paint = paint;
    v.snap = t;
    v.damage = 0;
    v.destroyed = false;
    v.despawn = 0.f;
    v.idle = 0.f;
    v.owner = 0;
    v.towTimer = 0.f;
    for (int i = 0; i < 4; ++i) v.seat[i] = 0;
    VehicleController::VehicleController::g_vehicles[id] = v;

    // Create vehicle in enhanced physics system
    auto& enhancedPhysics = CoopNet::EnhancedVehiclePhysics::Instance();
    CoopNet::VehicleProperties properties;

    // Set vehicle type based on archetype (simplified mapping)
    if (archetype >= 1000 && archetype < 2000) {
        properties.type = CoopNet::VehicleProperties::VehicleType::Motorcycle;
    } else if (archetype >= 2000 && archetype < 3000) {
        properties.type = CoopNet::VehicleProperties::VehicleType::Truck;
    } else {
        properties.type = CoopNet::VehicleProperties::VehicleType::Car;
    }

    if (enhancedPhysics.CreateVehicle(id, v.owner, properties)) {
        auto* enhancedVehicle = enhancedPhysics.GetVehicle(id);
        if (enhancedVehicle) {
            enhancedVehicle->FromTransformSnap(t);
        }
    }

    VehicleSpawnPacket pkt{id, archetype, paint, phaseId, t};
    Net_Broadcast(EMsg::VehicleSpawn, &pkt, sizeof(pkt));

    // Register with MultiOccupancyManager
    auto& vehicleController = VehicleController::GetInstance();
    vehicleController.RegisterVehicleWithOccupancyManager(id, 4); // Default 4 seats
}

void VehicleController_Spawn(uint32_t archetype, uint32_t paint, const TransformSnap& t)
{
    VehicleController_SpawnPhaseVehicle(archetype, paint, t, 0u);
}

static float VecLen(const RED4ext::Vector3& v)
{
    return std::sqrt(v.X * v.X + v.Y * v.Y + v.Z * v.Z);
}

void VehicleController_ApplyDamage(uint16_t dmg, bool side)
{
    // Deprecated single-vehicle path; kept for compatibility if needed.
}

void VehicleController_SetOccupant(uint32_t peerId)
{
    std::lock_guard lock(VehicleController::g_vehicleMutex);
    for (auto& kv : VehicleController::g_vehicles)
    {
        if (kv.second.owner == peerId)
        {
            kv.second.seat[0] = peerId;
            break;
        }
    }
}

void VehicleController_HandleSeatRequest(CoopNet::Connection* c, uint32_t vehicleId, uint8_t seatIdx)
{
    if (!c || seatIdx >= 4)
        return;

    // Use enhanced VehicleController for seat management
    auto& vehicleController = VehicleController::GetInstance();
    vehicleController.HandleVehicleEntry(c, vehicleId, static_cast<int32_t>(seatIdx));

    // Legacy fallback for compatibility
    std::lock_guard lock(VehicleController::g_vehicleMutex);
    auto it = VehicleController::g_vehicles.find(vehicleId);
    if (it == VehicleController::g_vehicles.end()) return;
    auto& v = it->second;
    if (v.seat[seatIdx] == 0)
    {
        v.seat[seatIdx] = c->peerId;
        SeatAssignPacket pkt{c->peerId, vehicleId, seatIdx};
        Net_Broadcast(EMsg::SeatAssign, &pkt, sizeof(pkt));
    }
}

void VehicleController_HandleHit(uint32_t vehicleId, uint16_t dmg, bool side)
{
    std::lock_guard lock(VehicleController::g_vehicleMutex);
    auto it = VehicleController::g_vehicles.find(vehicleId);
    if (it == VehicleController::g_vehicles.end()) return;
    auto& v = it->second;
    if (v.destroyed) return;
    float now = GameClock::GetCurrentTick() * GameClock::GetTickMs();
    if (now - v.lastHit < 200.f) return;
    v.lastHit = now;
    uint16_t apply = std::min<uint16_t>(dmg, 500u);
    v.damage = static_cast<uint16_t>(std::min<int>(1000, v.damage + apply));
    if (side && apply > 300u)
    {
        VehiclePartDetachPacket dpkt{v.id, 0, {0, 0, 0}};
        Net_Broadcast(EMsg::VehiclePartDetach, &dpkt, sizeof(dpkt));
    }
    if (v.damage >= 1000u)
    {
        uint32_t vfx = Fnv1a32("veh_explosion_big.ent");
        uint32_t seed = v.damage * 1664525u + 1013904223u;
        VehicleExplodePacket epkt{v.id, vfx, seed};
        Net_Broadcast(EMsg::VehicleExplode, &epkt, sizeof(epkt));
        v.destroyed = true;
        v.despawn = 10.f;
        v.towTimer = 300.f;
        CarParking cp{};
        cp.vehTpl = v.archetype;
        cp.pos = v.snap.pos;
        cp.rot = v.snap.rot;
        cp.health = 0;
        SaveCarParking(SessionState_GetId(), v.owner, cp);
    }
    VehicleHitPacket pkt{vehicleId, apply, side ? 1 : 0, 0};
    Net_Broadcast(EMsg::VehicleHit, &pkt, sizeof(pkt));
}

void VehicleController_HandleSummon(CoopNet::Connection* c, uint32_t vehId, const TransformSnap& t)
{
    if (!c) return;
    std::lock_guard lock(VehicleController::g_vehicleMutex);
    auto it = VehicleController::g_vehicles.find(vehId);
    if (it != VehicleController::g_vehicles.end() && !it->second.destroyed)
    {
        if (it->second.damage >= 500u) return;
        it->second.snap = t;
    }
    else
    {
        VehicleController::VehicleState v{};
        v.id = vehId;
        v.snap = t;
        v.damage = 0;
        v.destroyed = false;
        VehicleController::g_vehicles[vehId] = v;
    }
    auto& v = VehicleController::g_vehicles[vehId];
    v.owner = c->peerId;
    v.idle = 0.f;
    VehicleSummonPacket pkt{vehId, c->peerId, t};
    Net_Broadcast(EMsg::VehicleSummon, &pkt, sizeof(pkt));
}

static RED4ext::Vector3 FindSafePos(const RED4ext::Vector3& pos)
{
    RED4ext::Vector3 out;
    if (!Nav_FindClosestRoad(pos, out))
        out = pos;
    return out;
}

void VehicleController_HandleTowRequest(CoopNet::Connection* c, const RED4ext::Vector3& pos)
{
    if (!c) return;
    std::lock_guard lock(VehicleController::g_vehicleMutex);
    uint32_t peer = c->peerId;
    uint32_t ownedId = 0;
    for (auto& kv : VehicleController::g_vehicles)
        if (kv.second.owner == peer) { ownedId = kv.first; break; }
    if (ownedId == 0) return;
    auto& v = VehicleController::g_vehicles[ownedId];
    RED4ext::Vector3 safe = FindSafePos(pos);
    if (v.destroyed)
    {
        TransformSnap t = v.snap;
        t.pos = safe;
        v.snap = t;
        v.damage = 0;
        v.destroyed = false;
        v.despawn = 0.f;
        v.towTimer = 0.f;
        VehicleSpawnPacket pkt{v.id, v.archetype, v.paint, v.phaseId, t};
        Net_Broadcast(EMsg::VehicleSpawn, &pkt, sizeof(pkt));
        std::cout << "[Tow] Car respawn" << std::endl;
    }
    else
    {
        v.snap.pos = safe;
    }
    Net_SendVehicleTowAck(c, peer, true);
}

void VehicleController_RemovePeer(uint32_t peerId)
{
    std::lock_guard lock(VehicleController::g_vehicleMutex);
    for (auto& kv : VehicleController::g_vehicles)
    {
        for (int i = 0; i < 4; ++i)
            if (kv.second.seat[i] == peerId)
                kv.second.seat[i] = 0;
    }
}

uint32_t VehicleController_GetPeerVehicleId(uint32_t peerId)
{
    std::lock_guard lock(VehicleController::g_vehicleMutex);
    // Prefer ownership
    for (auto& kv : VehicleController::g_vehicles)
        if (kv.second.owner == peerId)
            return kv.first;
    // Fallback to any seat occupancy
    for (auto& kv : VehicleController::g_vehicles)
        for (int i = 0; i < 4; ++i)
            if (kv.second.seat[i] == peerId)
                return kv.first;
    return 0;
}

void VehicleController_ApplyHitValidated(uint32_t attackerPeerId, uint32_t vehicleId, uint16_t dmg, bool side)
{
    // Basic provenance and rate checks; extend as needed
    if (attackerPeerId == 0 || dmg == 0)
        return;
    if (Connection* a = Net_FindConnection(attackerPeerId); !a)
        return;
    // Clamp damage and reuse HandleHit logic
    if (dmg > 500u) dmg = 500u;
    VehicleController_HandleHit(vehicleId, dmg, side);
}

void VehicleController_ServerTick(float dt)
{
    // Update enhanced VehicleController
    auto& vehicleController = VehicleController::GetInstance();
    vehicleController.ServerTick(dt);

    std::lock_guard lock(VehicleController::g_vehicleMutex);
    for (auto& kv : VehicleController::g_vehicles)
    {
        auto& v = kv.second;
        if (v.destroyed)
        {
            v.despawn -= dt / 1000.f;
            if (v.towTimer > 0.f)
            {
                v.towTimer -= dt / 1000.f;
                if (v.towTimer <= 0.f && v.owner != 0)
                {
                    if (Connection* c = Net_FindConnection(v.owner))
                        Net_SendVehicleTowAck(c, v.owner, true);
                    std::cout << "[Tow] Car returned" << std::endl;
                    v.owner = 0;
                }
            }
            continue;
        }
        float vPrev = VecLen(v.prevVel);
        float vCur = VecLen(v.snap.vel);
        float decel = (vPrev - vCur) / (dt / 1000.f);
        if (decel > 12.f && v.seat[0] != 0)
        {
            RED4ext::Vector3 launch = v.prevVel;
            EjectOccupantPacket pkt{v.seat[0], launch};
            Net_Broadcast(EMsg::EjectOccupant, &pkt, sizeof(pkt));
            v.seat[0] = 0;
        }
        v.prevVel = v.snap.vel;
        if (v.seat[0] == 0 && vCur < 0.1f)
        {
            v.idle += dt / 1000.f;
            if (v.idle >= 10.f)
            {
                CarParking cp{};
                cp.vehTpl = v.archetype;
                cp.pos = v.snap.pos;
                cp.rot = v.snap.rot;
                cp.health = static_cast<uint16_t>(1000u - v.damage);
                SaveCarParking(SessionState_GetId(), v.owner, cp);
                Net_BroadcastTrafficDespawn(v.id);
                v.idle = 0.f;
            }
        }
        else
        {
            v.idle = 0.f;
        }
    }
}

void VehicleController_PhysicsStep(float dt)
{
    // Use enhanced physics system for better simulation
    auto& enhancedPhysics = CoopNet::EnhancedVehiclePhysics::Instance();
    enhancedPhysics.StepSimulation(dt);

    // Sync enhanced physics state back to legacy vehicle state for compatibility
    std::lock_guard lock(VehicleController::g_vehicleMutex);
    for (auto& kv : VehicleController::g_vehicles) {
        if (!kv.second.destroyed) {
            auto* enhancedVehicle = enhancedPhysics.GetVehicle(kv.first);
            if (enhancedVehicle) {
                // Update legacy state from enhanced physics
                kv.second.snap = enhancedVehicle->ToTransformSnap();
            } else {
                // Fallback to legacy physics if vehicle not in enhanced system
                ServerSimulate(kv.second.snap, dt);
            }
        }
    }
}

// === Advanced Vehicle Features ===

void VehicleController_HandleCustomization(CoopNet::Connection* c, uint32_t vehicleId, const CoopNet::VehicleCustomizationPacket& customization)
{
    if (!c) return;
    
    std::lock_guard lock(VehicleController::g_vehicleMutex);
    auto it = VehicleController::g_vehicles.find(vehicleId);
    if (it == VehicleController::g_vehicles.end()) return;
    
    auto& vehicle = it->second;
    if (vehicle.owner != c->peerId) {
        std::cout << "[VehicleController] Unauthorized customization attempt by peer " << c->peerId << std::endl;
        return;
    }
    
    // Validate customization data
    if (!ValidateVehicleCustomization(customization)) {
        std::cout << "[VehicleController] Invalid customization data from peer " << c->peerId << std::endl;
        return;
    }
    
    // Copy packet data to vehicle customization
    vehicle.customization.colorId = customization.colorId;
    strncpy(vehicle.customization.plateText, customization.plateText, sizeof(vehicle.customization.plateText));
    // Copy all modifications (no modCount needed in VehicleCustomizationPacket)
    for (int i = 0; i < 8; ++i) {
        vehicle.customization.modifications[i] = customization.modifications[i];
    }
    
    // Broadcast customization update
    VehicleCustomizationPacket pkt{};
    pkt.vehicleId = vehicleId;
    pkt.colorId = customization.colorId;
    strncpy(pkt.plateText, customization.plateText, sizeof(pkt.plateText));
    for (int i = 0; i < 8; ++i) {
        pkt.modifications[i] = customization.modifications[i];
    }
    Net_Broadcast(EMsg::VehicleCustomization, &pkt, sizeof(pkt));
    
    std::cout << "[VehicleController] Vehicle " << vehicleId << " customized by peer " << c->peerId << std::endl;
}

void VehicleController_HandlePassengerSync(CoopNet::Connection* c, uint32_t vehicleId, uint8_t seatIndex, uint32_t passengerId, bool isEntering)
{
    if (!c) return;
    
    std::lock_guard lock(VehicleController::g_vehicleMutex);
    auto it = VehicleController::g_vehicles.find(vehicleId);
    if (it == VehicleController::g_vehicles.end()) return;
    
    auto& vehicle = it->second;
    
    if (seatIndex >= 4) {
        std::cout << "[VehicleController] Invalid seat index " << static_cast<int>(seatIndex) << std::endl;
        return;
    }
    
    if (isEntering) {
        if (vehicle.seat[seatIndex] != 0 && vehicle.seat[seatIndex] != passengerId) {
            std::cout << "[VehicleController] Seat " << static_cast<int>(seatIndex) << " already occupied" << std::endl;
            return;
        }
        vehicle.seat[seatIndex] = passengerId;
    } else {
        if (vehicle.seat[seatIndex] != passengerId) {
            std::cout << "[VehicleController] Passenger " << passengerId << " not in seat " << static_cast<int>(seatIndex) << std::endl;
            return;
        }
        vehicle.seat[seatIndex] = 0;
    }
    
    // Broadcast passenger update
    PassengerSyncPacket pkt{vehicleId, seatIndex, passengerId, isEntering};
    Net_Broadcast(EMsg::PassengerSync, &pkt, sizeof(pkt));
    
    std::cout << "[VehicleController] Passenger " << passengerId << (isEntering ? " entered" : " exited") 
              << " seat " << static_cast<int>(seatIndex) << " of vehicle " << vehicleId << std::endl;
}

void VehicleController_UpdateInterpolationBuffer(uint32_t vehicleId, const TransformSnap& snap)
{
    std::lock_guard lock(VehicleController::g_vehicleMutex);
    auto it = VehicleController::g_vehicles.find(vehicleId);
    if (it == VehicleController::g_vehicles.end()) return;
    
    auto& vehicle = it->second;
    vehicle.interpolationBuffer[vehicle.bufferIndex] = snap;
    vehicle.bufferIndex = (vehicle.bufferIndex + 1) % 3;
    vehicle.lastUpdate = GetCurrentTimeMs() / 1000.0f;
}

TransformSnap VehicleController_InterpolatePosition(uint32_t vehicleId, float deltaTime)
{
    std::lock_guard lock(VehicleController::g_vehicleMutex);
    auto it = VehicleController::g_vehicles.find(vehicleId);
    if (it == VehicleController::g_vehicles.end()) return TransformSnap{};
    
    auto& vehicle = it->second;
    float timeSinceUpdate = (GetCurrentTimeMs() / 1000.0f) - vehicle.lastUpdate;
    
    // Use interpolation for high-latency connections
    if (timeSinceUpdate > 0.1f) { // 100ms threshold
        // Linear interpolation between last two positions
        uint8_t current = (vehicle.bufferIndex + 2) % 3;
        uint8_t previous = (vehicle.bufferIndex + 1) % 3;
        
        const auto& currentSnap = vehicle.interpolationBuffer[current];
        const auto& previousSnap = vehicle.interpolationBuffer[previous];
        
        float alpha = std::min(timeSinceUpdate / 0.1f, 1.0f);
        TransformSnap interpolated = currentSnap;
        
        // Interpolate position
        interpolated.pos.X = previousSnap.pos.X + alpha * (currentSnap.pos.X - previousSnap.pos.X);
        interpolated.pos.Y = previousSnap.pos.Y + alpha * (currentSnap.pos.Y - previousSnap.pos.Y);
        interpolated.pos.Z = previousSnap.pos.Z + alpha * (currentSnap.pos.Z - previousSnap.pos.Z);
        
        return interpolated;
    }
    
    return vehicle.snap;
}

bool VehicleController_ValidateDamage(uint32_t vehicleId, uint16_t damage, uint32_t attackerId)
{
    std::lock_guard lock(VehicleController::g_vehicleMutex);
    auto it = VehicleController::g_vehicles.find(vehicleId);
    if (it == VehicleController::g_vehicles.end()) return false;
    
    auto& vehicle = it->second;
    
    // Prevent damage exploitation
    if (damage > 1000) {
        std::cout << "[VehicleController] Suspicious damage amount " << damage << " from attacker " << attackerId << std::endl;
        return false;
    }
    
    // Check time since last hit to prevent spam
    float currentTime = GetCurrentTimeMs() / 1000.0f;
    if (currentTime - vehicle.lastHit < 0.1f) {
        std::cout << "[VehicleController] Damage rate limit exceeded for vehicle " << vehicleId << std::endl;
        return false;
    }
    
    vehicle.lastHit = currentTime;
    vehicle.needsValidation = true;
    return true;
}

// === Utility Functions ===


} // namespace CoopNet

// Fallback stub to resolve undefined reference on platforms without nav module.
// Returns false and leaves output as input position.
