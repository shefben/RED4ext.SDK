// Packet wire format for cp2077-coop.
// Example header JSON: {"type":1,"size":42}
#pragma once

#include "Snapshot.hpp"
#include <cstdint>

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
    NpcDespawn,
    SectorChange,
    SectorReady,
    ItemSnap,
    CraftRequest,
    CraftResult,
    AttachModRequest,
    AttachModResult,
    VehicleExplode,
    VehiclePartDetach,
    EjectOccupant,
    InterestAdd,
    InterestRemove,
    TickRateChange,
    BreachStart,
    BreachInput,
    BreachResult,
    ElevatorCall,
    ElevatorArrive,
    TeleportAck,
    HoloCallStart,
    HoloCallEnd,
    AdminCmd,
    SpectateRequest,
    SpectateGranted,
    NatCandidate,
    CineStart,
    Viseme,
    DialogChoice,
    Voice
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

struct SectorChangePacket
{
    uint32_t peerId;
    uint64_t sectorHash;
};

struct SectorReadyPacket
{
    uint64_t sectorHash;
};

struct ItemSnapPacket
{
    ItemSnap snap;
};

struct CraftRequestPacket
{
    uint32_t recipeId;
};

struct CraftResultPacket
{
    ItemSnap item;
};

struct AttachModRequestPacket
{
    uint64_t itemId;
    uint8_t slotIdx;
    uint8_t _pad[3];
    uint64_t attachmentId;
};

struct AttachModResultPacket
{
    ItemSnap item;
    uint8_t success;
    uint8_t _pad2[3];
};

struct VehicleExplodePacket
{
    uint32_t vehicleId;
    uint32_t vfxId;
    uint32_t seed;
};

struct VehiclePartDetachPacket
{
    uint32_t vehicleId;
    uint8_t partId; // 0=door_L,1=door_R,2=hood,3=trunk
    uint8_t _pad[3];
};

struct EjectOccupantPacket
{
    uint32_t peerId;
};

struct InterestPacket
{
    uint32_t id;
};

struct TickRateChangePacket
{
    uint16_t tickMs;
    uint16_t _pad;
};

struct BreachStartPacket
{
    uint32_t peerId;
    uint32_t seed;
    uint8_t gridW;
    uint8_t gridH;
    uint8_t _pad[2];
};

struct BreachInputPacket
{
    uint32_t peerId;
    uint8_t index;
    uint8_t _pad[3];
};

struct BreachResultPacket
{
    uint32_t peerId;
    uint8_t daemonsMask;
    uint8_t _pad[3];
};

struct ElevatorCallPacket
{
    uint32_t peerId;
    uint32_t elevatorId;
    uint8_t floorIdx;
    uint8_t _pad[3];
};

struct ElevatorArrivePacket
{
    uint32_t elevatorId;
    uint64_t sectorHash;
    RED4ext::Vector3 pos;
};

// Acknowledges elevator arrival per-connection; peer is inferred from ENet peer.
struct TeleportAckPacket
{
    uint32_t elevatorId;
};

struct HoloCallPacket
{
    uint32_t peerId;
};

struct AdminCmdPacket
{
    uint8_t cmdType; // 0=Kick,1=Ban,2=Mute
    uint8_t _pad[3];
    uint64_t param;
};

struct SpectatePacket
{
    uint32_t peerId;
};

struct NatCandidatePacket
{
    char sdp[256];
};

struct CineStartPacket
{
    uint32_t sceneId;
    uint32_t startTimeMs;
};

struct VisemePacket
{
    uint32_t npcId;
    uint8_t visemeId; // AA, TH, FV, etc.
    uint8_t _pad[3];
    uint32_t timeMs;
};

struct DialogChoicePacket
{
    uint32_t peerId;
    uint8_t choiceIdx;
    uint8_t _pad[3];
};

struct VoicePacket
{
    uint32_t peerId;
    uint16_t seq;
    uint16_t size;
    uint8_t data[256];
};

struct AvatarSpawnPacket
{
    uint32_t peerId;
    TransformSnap snap;
};

struct AvatarDespawnPacket
{
    uint32_t peerId;
};

struct ChatPacket
{
    uint32_t peerId;
    char msg[64];
};

} // namespace CoopNet
