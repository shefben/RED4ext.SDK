#include "VehicleController.hpp"
#include "../core/GameClock.hpp"
#include "../core/Hash.hpp"
#include "../core/SaveFork.hpp"
#include "../core/SessionState.hpp"
#include "../net/Connection.hpp"
#include "../net/Net.hpp"
#include "../net/Packets.hpp"
#include "../physics/CarPhysics.hpp"
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

struct VehicleState
{
    uint32_t id = 1;
    uint32_t archetype = 0;
    uint32_t paint = 0;
    TransformSnap snap{};
    uint16_t damage = 0;
    RED4ext::Vector3 prevVel{};
    bool destroyed = false;
    float despawn = 0.f;
    float idle = 0.f;
    uint32_t owner = 0;
    uint32_t phaseId = 0;
    uint32_t seat[4] = {0, 0, 0, 0};
    float lastHit = 0.f;
    float towTimer = 0.f;
    
    // Advanced features
    VehicleCustomization customization{};
    TransformSnap interpolationBuffer[3]{}; // For lag compensation
    uint8_t bufferIndex = 0;
    float lastUpdate = 0.f;
    bool needsValidation = false;
};
static std::unordered_map<uint32_t, VehicleState> g_vehicles;
static std::mutex g_vehicleMutex;
static std::atomic<uint32_t> g_nextVehId{1};

void VehicleController_SpawnPhaseVehicle(uint32_t archetype, uint32_t paint, const TransformSnap& t, uint32_t phaseId)
{
    std::lock_guard lock(g_vehicleMutex);
    uint32_t id = g_nextVehId++;
    VehicleState v{};
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
    g_vehicles[id] = v;
    VehicleSpawnPacket pkt{id, archetype, paint, phaseId, t};
    Net_Broadcast(EMsg::VehicleSpawn, &pkt, sizeof(pkt));
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
    std::lock_guard lock(g_vehicleMutex);
    for (auto& kv : g_vehicles)
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
    std::lock_guard lock(g_vehicleMutex);
    auto it = g_vehicles.find(vehicleId);
    if (it == g_vehicles.end()) return;
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
    std::lock_guard lock(g_vehicleMutex);
    auto it = g_vehicles.find(vehicleId);
    if (it == g_vehicles.end()) return;
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
    std::lock_guard lock(g_vehicleMutex);
    auto it = g_vehicles.find(vehId);
    if (it != g_vehicles.end() && !it->second.destroyed)
    {
        if (it->second.damage >= 500u) return;
        it->second.snap = t;
    }
    else
    {
        VehicleState v{};
        v.id = vehId;
        v.snap = t;
        v.damage = 0;
        v.destroyed = false;
        g_vehicles[vehId] = v;
    }
    auto& v = g_vehicles[vehId];
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
    std::lock_guard lock(g_vehicleMutex);
    uint32_t peer = c->peerId;
    uint32_t ownedId = 0;
    for (auto& kv : g_vehicles)
        if (kv.second.owner == peer) { ownedId = kv.first; break; }
    if (ownedId == 0) return;
    auto& v = g_vehicles[ownedId];
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
    std::lock_guard lock(g_vehicleMutex);
    for (auto& kv : g_vehicles)
    {
        for (int i = 0; i < 4; ++i)
            if (kv.second.seat[i] == peerId)
                kv.second.seat[i] = 0;
    }
}

uint32_t VehicleController_GetPeerVehicleId(uint32_t peerId)
{
    std::lock_guard lock(g_vehicleMutex);
    // Prefer ownership
    for (auto& kv : g_vehicles)
        if (kv.second.owner == peerId)
            return kv.first;
    // Fallback to any seat occupancy
    for (auto& kv : g_vehicles)
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
    std::lock_guard lock(g_vehicleMutex);
    for (auto& kv : g_vehicles)
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
    std::lock_guard lock(g_vehicleMutex);
    for (auto& kv : g_vehicles)
        if (!kv.second.destroyed)
            ServerSimulate(kv.second.snap, dt);
}

// === Advanced Vehicle Features ===

void VehicleController_HandleCustomization(CoopNet::Connection* c, uint32_t vehicleId, const CoopNet::VehicleCustomizationPacket& customization)
{
    if (!c) return;
    
    std::lock_guard lock(g_vehicleMutex);
    auto it = g_vehicles.find(vehicleId);
    if (it == g_vehicles.end()) return;
    
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
    vehicle.customization.modCount = 8; // Copy all modifications
    for (int i = 0; i < 8; ++i) {
        vehicle.customization.modIds[i] = customization.modifications[i];
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
    
    std::lock_guard lock(g_vehicleMutex);
    auto it = g_vehicles.find(vehicleId);
    if (it == g_vehicles.end()) return;
    
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
    std::lock_guard lock(g_vehicleMutex);
    auto it = g_vehicles.find(vehicleId);
    if (it == g_vehicles.end()) return;
    
    auto& vehicle = it->second;
    vehicle.interpolationBuffer[vehicle.bufferIndex] = snap;
    vehicle.bufferIndex = (vehicle.bufferIndex + 1) % 3;
    vehicle.lastUpdate = GetCurrentTimeMs() / 1000.0f;
}

TransformSnap VehicleController_InterpolatePosition(uint32_t vehicleId, float deltaTime)
{
    std::lock_guard lock(g_vehicleMutex);
    auto it = g_vehicles.find(vehicleId);
    if (it == g_vehicles.end()) return TransformSnap{};
    
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
    std::lock_guard lock(g_vehicleMutex);
    auto it = g_vehicles.find(vehicleId);
    if (it == g_vehicles.end()) return false;
    
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
