#pragma once
#include "../net/Snapshot.hpp"
#include "../net/Packets.hpp"
#include "MultiOccupancyManager.hpp"

namespace CoopNet
{
class Connection;

// Enhanced VehicleController with MultiOccupancyManager integration
class VehicleController
{
public:
    static VehicleController& GetInstance();

    // Lifecycle
    void Initialize();
    void Shutdown();
    void ServerTick(float dt);
    void PhysicsStep(float dt);

    // Vehicle management (enhanced with MultiOccupancyManager)
    void Spawn(uint32_t archetype, uint32_t paint, const TransformSnap& t);
    void SpawnPhaseVehicle(uint32_t archetype, uint32_t paint, const TransformSnap& t, uint32_t phaseId);
    void HandleSummon(CoopNet::Connection* c, uint32_t vehId, const TransformSnap& t);
    void RemovePeer(uint32_t peerId);

    // Multi-occupancy integration
    void HandleSeatRequest(CoopNet::Connection* c, uint32_t vehicleId, uint8_t seatIdx);
    void HandleVehicleEntry(CoopNet::Connection* c, uint32_t vehicleId, int32_t preferredSeat = -1);
    void HandleVehicleExit(CoopNet::Connection* c, uint32_t vehicleId);
    void HandleSeatReservation(CoopNet::Connection* c, uint32_t vehicleId, int32_t preferredSeat = -1);
    void HandleDriverTransfer(CoopNet::Connection* c, uint32_t vehicleId, uint32_t newDriverId);

    // Damage and physics
    void ApplyDamage(uint16_t dmg, bool side);
    void HandleHit(uint32_t vehicleId, uint16_t dmg, bool side);
    void ApplyHitValidated(uint32_t attackerPeerId, uint32_t vehicleId, uint16_t dmg, bool side);
    void HandleTowRequest(CoopNet::Connection* c, const RED4ext::Vector3& pos);

    // Advanced features
    void HandleCustomization(CoopNet::Connection* c, uint32_t vehicleId, const VehicleCustomizationPacket& customization);
    void HandlePassengerSync(CoopNet::Connection* c, uint32_t vehicleId, uint8_t seatIndex, uint32_t passengerId, bool isEntering);
    void UpdateInterpolationBuffer(uint32_t vehicleId, const TransformSnap& snap);
    TransformSnap InterpolatePosition(uint32_t vehicleId, float deltaTime);
    bool ValidateDamage(uint32_t vehicleId, uint16_t damage, uint32_t attackerId);

    // Query methods
    uint32_t GetPeerVehicleId(uint32_t peerId);
    bool IsVehicleRegistered(uint64_t vehicleId) const;
    std::vector<uint32_t> GetVehicleOccupants(uint64_t vehicleId) const;
    uint32_t GetVehicleDriver(uint64_t vehicleId) const;

    // Statistics
    uint32_t GetTotalVehicles() const;
    uint32_t GetActiveVehicles() const;

private:
    VehicleController() = default;
    ~VehicleController() = default;
    VehicleController(const VehicleController&) = delete;
    VehicleController& operator=(const VehicleController&) = delete;

    // Internal helpers
    void SyncOccupancyWithLegacyState(uint64_t vehicleId);
    void OnVehicleEntryResult(uint32_t playerId, uint64_t vehicleId, int32_t seatIndex, RED4ext::VehicleEntryResult result);
    void OnVehicleExitResult(uint32_t playerId, uint64_t vehicleId, int32_t seatIndex, RED4ext::VehicleExitResult result);
    void OnSeatReservationResult(uint32_t playerId, uint64_t vehicleId, int32_t seatIndex, RED4ext::SeatReservationResult result);
    void OnDriverChange(uint64_t vehicleId, uint32_t oldDriverId, uint32_t newDriverId);
    void BroadcastVehicleUpdate(uint32_t vehicleId);
    bool ValidateConnection(CoopNet::Connection* c) const;
    uint32_t PeerIdToPlayerId(uint32_t peerId) const;
    uint32_t PlayerIdToPeerId(uint32_t playerId) const;

public:
    // Integration with MultiOccupancyManager (needs to be public for legacy functions)
    void RegisterVehicleWithOccupancyManager(uint64_t vehicleId, uint32_t maxSeats);
    // Static data for global vehicle state (for compatibility with legacy functions)
    struct VehicleState {
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
        VehicleCustomizationPacket customization{};
        TransformSnap interpolationBuffer[3]{};
        uint8_t bufferIndex = 0;
        float lastUpdate = 0.f;
        bool needsValidation = false;
    };

    static std::unordered_map<uint32_t, VehicleState> g_vehicles;
    static std::mutex g_vehicleMutex;
    static std::atomic<uint32_t> g_nextVehId;
};

// Legacy function wrappers for compatibility
void VehicleController_ServerTick(float dt);
void VehicleController_ApplyDamage(uint16_t dmg, bool side);
void VehicleController_SetOccupant(uint32_t peerId);
void VehicleController_Spawn(uint32_t archetype, uint32_t paint, const TransformSnap& t);
void VehicleController_SpawnPhaseVehicle(uint32_t archetype, uint32_t paint, const TransformSnap& t, uint32_t phaseId);
void VehicleController_HandleSeatRequest(CoopNet::Connection* c, uint32_t vehicleId, uint8_t seatIdx);
void VehicleController_HandleHit(uint32_t vehicleId, uint16_t dmg, bool side);
void VehicleController_RemovePeer(uint32_t peerId);
void VehicleController_HandleSummon(CoopNet::Connection* c, uint32_t vehId, const TransformSnap& t);
void VehicleController_HandleTowRequest(CoopNet::Connection* c, const RED4ext::Vector3& pos);
void VehicleController_PhysicsStep(float dt);
uint32_t VehicleController_GetPeerVehicleId(uint32_t peerId);
void VehicleController_ApplyHitValidated(uint32_t attackerPeerId, uint32_t vehicleId, uint16_t dmg, bool side);

} // namespace CoopNet
