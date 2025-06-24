// Packet wire format for cp2077-coop.
// Example header JSON: {"type":1,"size":42}
#pragma once

#include <cstdint>
#include "Snapshot.hpp"

namespace CoopNet
{

enum class EMsg : uint16_t
{
    Hello = 1,
    Welcome,
    Ping,
    Pong,
    Seed,
    Snapshot,
    Chat,
    JoinRequest,
    JoinAccept,
    JoinDeny,
    Disconnect,
    SeedAck,
    Version,
    AvatarSpawn,
    AvatarDespawn,
    QuestStage,
    SceneTrigger,
    HitRequest,
    HitConfirm,
    VehicleSpawn,
    SeatAssign,
    VehicleHit,
    Quickhack,
    HeatSync,
    WorldState,
    ScoreUpdate,
    NpcSnapshot,
    NpcSpawn,
    NpcDespawn
};

struct PacketHeader
{
    uint16_t type;
    uint16_t size;
};

constexpr size_t kHeaderSize = sizeof(PacketHeader);

inline constexpr size_t Packet_GetSize(const PacketHeader& hdr)
{
    return hdr.size;
}

inline void Packet_SetSize(PacketHeader& hdr, uint16_t payloadBytes)
{
    hdr.size = payloadBytes;
}

static_assert(sizeof(PacketHeader) == 4, "header must be packed");

// Seed synchronization packet used for deterministic RNG.
// Example exchange:
//   client -> server : SeedRequest
//   server -> all    : Seed(seed=123456u)
struct SeedPacket
{
    uint32_t seed;
};

struct HitRequestPacket
{
    uint32_t targetId;
    uint16_t damage;
};

struct HitConfirmPacket
{
    uint32_t targetId;
    uint16_t appliedDamage;
};

struct VersionPacket
{
    uint32_t crc;
};

struct VehicleSpawnPacket
{
    uint32_t vehicleId;
    TransformSnap transform;
};

struct SeatAssignPacket
{
    uint32_t peerId;
    uint32_t vehicleId;
    uint8_t seatIdx; // 0-3
};

struct VehicleHitPacket
{
    uint32_t vehicleId;
    uint16_t dmg;
};

struct WorldStatePacket
{
    uint32_t sunAngle; // degrees * 100
    uint8_t weatherId;
};

struct ScoreUpdatePacket
{
    uint32_t peerId;
    uint16_t k;
    uint16_t d;
};

struct NpcSnapshotPacket
{
    NpcSnap snap;
};

struct NpcSpawnPacket
{
    NpcSnap snap; // full snap on spawn
};

struct NpcDespawnPacket
{
    uint32_t npcId;
};

} // namespace CoopNet

