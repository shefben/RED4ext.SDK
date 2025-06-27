// Packet wire format for cp2077-coop.
// Example header JSON: {"type":1,"size":42}
#pragma once

#include "Snapshot.hpp"
#include <RED4ext/Scripting/Natives/Generated/Vector3.hpp>
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
    QuestStageP2P,
    QuestFullSync,
    QuestResyncRequest,
    SceneTrigger,
    HitRequest,
    HitConfirm,
    VehicleSpawn,
    SeatRequest,
    VehicleSummonRequest,
    SeatAssign,
    VehicleHit,
    Quickhack,
    HeatSync,
    WorldState,
    ScoreUpdate,
    MatchOver,
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
    RuleChange,
    AdminCmd,
    SpectateRequest,
    SpectateGranted,
    NatCandidate,
    CineStart,
    Viseme,
    DialogChoice,
    Voice,
    GlobalEvent,
    CrowdSeed,
    VendorStock,
    VendorStockUpdate,
    VendorRefresh,
    PurchaseRequest,
    PurchaseResult,
    SnapshotAck,
    WorldMarkers,
    NpcSpawnCruiser,
    NpcState,
    CrimeEventSpawn,
    CyberEquip,
    SlowMoStart,
    PerkUnlock,
    PerkRespecRequest,
    PerkRespecAck,
    SkillXP, // SX-1
    StatusApply,
    StatusTick,
    TrafficSeed,
    TrafficDespawn,
    PropBreak,
    PropIgnite,
    VOPlay,
    FixerCallStart,
    FixerCallEnd,
    GigSpawn,
    VehicleSummon,
    Appearance,
    PingOutline,
    LootRoll,
    DealerBuy,
    VehicleUnlock,
    WeaponInspectStart,
    FinisherStart,
    FinisherEnd,
    TextureBiasChange,
    CriticalVoteStart, // PX-6
    CriticalVoteCast,
    PhaseBundle,
    AptPurchase,
    AptPurchaseAck,
    AptEnterReq,
    AptEnterAck,
    AptPermChange,
    VehicleHitHighSpeed,
    VehicleTowRequest,
    VehicleTowAck,
    ReRollRequest, // WM-1
    ReRollResult,
    RipperInstallRequest,
    TileGameStart, // MG-1
    TileSelect,
    ShardProgress, // MG-2
    TradeInit,     // TRD-1
    TradeOffer,
    TradeAccept,
    TradeFinalize,
    EndingVoteStart, // EG-1
    EndingVoteCast,
    VehicleSnapshot, // VT-1
    TurretAim,       // VT-2
    AirVehSpawn,     // VT-3
    AirVehUpdate,
    VehiclePaintChange, // VT-4
    PanicEvent,         // AI-1
    AIHack,             // AI-2
    BossPhase,          // AI-3
    SectorLOD,          // PRF-1
    LowBWMode,          // PRF-2
    CrowdCfg,           // CD-1
    Emote,              // EM-1
    CrowdChatterStart,  // CA-1
    CrowdChatterEnd,
    HoloSeed, // HB-1
    HoloNextAd,
    DoorBreachStart, // DH-1
    DoorBreachTick,
    DoorBreachSuccess,
    DoorBreachAbort,
    HTableOpen, // HT-1
    HTableScrub,
    QuestGadgetFire, // QG-1
    ItemGrab,        // IP-1
    ItemDrop,
    ItemStore,
    MetroBoard,  // SB-1
    MetroArrive, // SB-2
    RadioChange, // RS-1
    CamHijack,   // SF-1
    CamFrameStart,
    CarryBegin, // PC-1
    CarrySnap,
    CarryEnd,
    GrenadePrime, // GR-1
    GrenadeSnap,
    SmartCamStart, // RC-1
    SmartCamEnd,
    SlowMoFinisher // RB-1
};

struct PacketHeader
{
    uint16_t type;
    uint16_t size;
};

struct HelloPacket
{
    uint8_t pub[crypto_kx_PUBLICKEYBYTES];
};

struct WelcomePacket
{
    uint8_t pub[crypto_kx_PUBLICKEYBYTES];
};

struct PingPacket
{
    uint32_t timeMs;
};

struct PongPacket
{
    uint32_t timeMs;
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
    uint32_t archetypeId;
    uint32_t paintId;
    uint32_t phaseId;
    TransformSnap transform;
};

struct SeatRequestPacket
{
    uint32_t vehicleId;
    uint8_t seatIdx; // 0-3
};

struct VehicleSummonRequestPacket
{
    uint32_t vehId;
    TransformSnap pos;
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
    uint8_t side; // 1 if side impact
    uint8_t pad;
};

struct VehicleHitHighSpeedPacket
{
    uint32_t vehA;
    uint32_t vehB;
    RED4ext::Vector3 deltaVel;
};

struct WorldStatePacket
{
    uint16_t sunAngleDeg; // 0-359
    uint8_t weatherId;
    uint16_t particleSeed;
};

struct ScoreUpdatePacket
{
    uint32_t peerId;
    uint16_t k;
    uint16_t d;
};

struct MatchOverPacket
{
    uint32_t winnerId;
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

struct QuestStagePacket
{
    uint32_t nameHash;
    uint16_t stage;
    uint16_t _pad;
};

struct QuestStageP2PPacket
{
    uint32_t phaseId; // PX-2
    uint32_t questHash;
    uint16_t stage;
    uint16_t _pad;
};

struct QuestResyncRequestPacket
{
    uint32_t _pad; // unused
};

struct SceneTriggerPacket
{
    uint32_t phaseId; // PX-1
    uint32_t nameHash;
    uint8_t start;
    uint8_t _pad[3];
};

struct QuestEntry
{
    uint32_t nameHash;
    uint16_t stage;
    uint16_t _pad;
};

struct QuestFullSyncPacket
{
    uint16_t count;
    uint16_t _pad;
    QuestEntry entries[32];
};

struct HeatPacket
{
    uint8_t level;
    uint8_t _pad[3];
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
    RED4ext::Vector3 velocity;
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

struct RuleChangePacket
{
    uint8_t friendlyFire;
    uint8_t _pad[3];
};

struct HolocallStartPacket
{
    uint32_t fixerId;
    uint32_t callId;
    uint8_t count;
    uint8_t _pad[3];
    uint32_t peerIds[4];
};

struct HolocallEndPacket
{
    uint32_t callId;
};

struct HTableOpenPacket
{
    uint32_t sceneId;
};

struct HTableScrubPacket
{
    uint32_t timestampMs;
};

struct QuestGadgetFirePacket
{
    uint32_t questId;
    uint8_t gadgetType;
    uint8_t charge;    // RailGun
    uint32_t targetId; // Nanowire
    uint8_t _pad;
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
    uint32_t phaseId; // PX-4
    uint8_t solo;
    uint8_t _pad[3];
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

struct GlobalEventPacket
{
    uint32_t eventId;
    uint32_t seed;
    uint8_t phase;
    uint8_t start; // 1=start, 0=stop
    uint8_t pad[2];
};

struct CrowdSeedPacket
{
    uint64_t sectorHash;
    uint32_t seed;
};

struct VendorStockItem
{
    uint32_t itemId;
    uint32_t price;
    uint16_t qty;
    uint16_t _pad;
};

struct VendorStockPacket
{
    uint32_t vendorId;
    uint32_t phaseId; // PX-8
    uint8_t count;
    uint8_t _pad[3];
    VendorStockItem items[8];
};

struct VendorStockUpdatePacket
{
    uint32_t vendorId;
    uint32_t phaseId; // PX-8
    uint32_t itemId;
    uint16_t qty;
    uint16_t _pad;
};

struct VendorRefreshPacket
{
    uint32_t vendorId;
    uint32_t phaseId; // PX-8
};

struct PurchaseRequestPacket
{
    uint32_t vendorId;
    uint32_t itemId;
    uint64_t nonce;
};

struct PurchaseResultPacket
{
    uint32_t vendorId;
    uint32_t itemId;
    uint64_t balance;
    uint8_t success;
    uint8_t _pad[3];
};

struct WorldMarkersPacket
{
    uint16_t blobBytes;
    uint8_t zstdBlob[1];
};

struct NpcSpawnCruiserPacket
{
    uint8_t waveIdx;
    uint8_t _pad[3];
    uint32_t npcSeeds[4];
};

struct NpcStatePacket
{
    uint32_t npcId;
    uint8_t aiState;
    uint8_t _pad[3];
};

struct CrimeEventSpawnPacket
{
    uint32_t eventId;
    uint32_t seed;
    uint8_t count;
    uint8_t _pad[3];
    uint32_t npcIds[4];
};

struct CyberEquipPacket
{
    uint32_t peerId;
    uint8_t slotId;
    uint8_t _pad[3];
    ItemSnap snap;
};

struct SlowMoStartPacket
{
    uint32_t peerId;
    float factor;
    uint16_t durationMs;
    uint16_t _pad;
};

struct PerkUnlockPacket
{
    uint32_t peerId;
    uint32_t perkId;
    uint8_t rank;
    uint8_t _pad[3];
};

struct PerkRespecRequestPacket
{
    uint32_t peerId;
};

struct PerkRespecAckPacket
{
    uint32_t peerId;
    uint16_t newPoints;
    uint8_t _pad[2];
};

struct SkillXPPacket
{
    uint32_t peerId;
    uint16_t skillId;
    int16_t deltaXP;
};

struct StatusApplyPacket
{
    uint32_t targetId;
    uint8_t effectId;
    uint16_t durMs;
    uint8_t amp;
};

struct StatusTickPacket
{
    uint32_t targetId;
    int16_t hpDelta;
};

struct TrafficSeedPacket
{
    uint64_t sectorHash;
    uint64_t seed64;
};

struct TrafficDespawnPacket
{
    uint32_t vehId;
};

struct PropBreakPacket
{
    uint32_t entityId;
    uint32_t seed;
};

struct PropIgnitePacket
{
    uint32_t entityId;
    uint16_t delayMs;
    uint16_t _pad;
};

struct VOPlayPacket
{
    uint32_t lineId;
};

struct FixerCallPacket
{
    uint32_t fixerId;
};

struct GigSpawnPacket
{
    uint32_t questId;
    uint32_t seed;
};

struct VehicleSummonPacket
{
    uint32_t vehId;
    uint32_t ownerId;
    TransformSnap pos;
};

struct AppearancePacket
{
    uint32_t peerId;
    uint32_t meshId;
    uint32_t tintId;
};

struct PingOutlinePacket
{
    uint32_t peerId;
    uint8_t count;
    uint8_t _pad;
    uint16_t durationMs;
    uint32_t entityIds[32];
};

struct LootRollPacket
{
    uint32_t containerId;
    uint32_t seed;
};

struct DealerBuyPacket
{
    uint32_t vehicleTpl;
    uint32_t price;
};

struct VehicleUnlockPacket
{
    uint32_t peerId;
    uint32_t vehicleTpl;
};

struct WeaponInspectPacket
{
    uint32_t peerId;
    uint16_t animId;
    uint16_t _pad;
};

struct FinisherStartPacket
{
    uint32_t actorId;
    uint32_t victimId;
    uint8_t finisherType;
    uint8_t _pad[3];
};

struct FinisherEndPacket
{
    uint32_t actorId;
};

struct SlowMoFinisherPacket
{
    uint32_t peerId;
    uint32_t targetId;
    uint16_t durationMs;
    uint16_t _pad;
};

struct TextureBiasPacket
{
    uint8_t bias;
    uint8_t _pad[3];
};

struct CriticalVoteStartPacket
{
    uint32_t questHash;
};

struct CriticalVoteCastPacket
{
    uint32_t peerId;
    uint8_t yes;
    uint8_t _pad[3];
};

struct PhaseBundlePacket
{
    uint32_t phaseId;
    uint16_t blobBytes;
    uint8_t zstdBlob[1];
};

struct AptPurchasePacket
{
    uint32_t aptId;
};

struct AptPurchaseAckPacket
{
    uint32_t aptId;
    uint64_t balance;
    uint8_t success;
    uint8_t _pad[3];
};

struct AptEnterReqPacket
{
    uint32_t aptId;
    uint32_t ownerPhaseId;
};

struct AptEnterAckPacket
{
    uint8_t allow;
    uint8_t _pad[3];
    uint32_t phaseId;
    uint32_t interiorSeed;
};

struct AptPermChangePacket
{
    uint32_t aptId;
    uint32_t targetPeerId;
    uint8_t allow;
    uint8_t _pad[3];
};

struct VehicleTowRequestPacket
{
    RED4ext::Vector3 pos;
};

struct VehicleTowAckPacket
{
    uint32_t ownerId;
    uint8_t ok;
    uint8_t _pad[3];
};

struct ReRollRequestPacket
{
    uint64_t itemId;
    uint32_t seed;
};

struct ReRollResultPacket
{
    ItemSnap snap;
};

struct RipperInstallRequestPacket
{
    uint8_t slotId;
    uint8_t _pad[3];
};

struct TileGameStartPacket
{
    uint32_t phaseId;
    uint32_t seed;
};

struct TileSelectPacket
{
    uint32_t peerId;
    uint32_t phaseId;
    uint8_t row;
    uint8_t col;
    uint8_t _pad[2];
};

struct ShardProgressPacket
{
    uint32_t phaseId;
    uint8_t percent;
    uint8_t _pad[3];
};

struct TradeInitPacket
{
    uint32_t fromId;
    uint32_t toId;
};

struct TradeOfferPacket
{
    uint32_t fromId;
    uint32_t toId;
    uint8_t count;
    uint8_t _pad[3];
    uint32_t eddies;
    ItemSnap items[8];
};

struct TradeAcceptPacket
{
    uint32_t peerId;
    uint8_t accept;
    uint8_t _pad[3];
};

struct TradeFinalizePacket
{
    uint8_t success;
    uint8_t _pad[3];
};

struct EndingVoteStartPacket
{
    uint32_t questHash;
};

struct EndingVoteCastPacket
{
    uint32_t peerId;
    uint8_t yes;
    uint8_t _pad[3];
};

struct VehicleSnapshotPacket
{
    VehicleSnap snap;
};

struct TurretAimPacket
{
    uint32_t vehId;
    float yaw;
    float pitch;
};

struct AirVehSpawnPacket
{
    uint32_t vehId;
    uint8_t count;
    uint8_t _pad[3];
    RED4ext::Vector3 points[8];
};

struct AirVehUpdatePacket
{
    uint32_t vehId;
    TransformSnap snap;
};

struct VehiclePaintChangePacket
{
    uint32_t vehId;
    uint32_t colorId;
    char plateId[8];
};

struct PanicEventPacket
{
    RED4ext::Vector3 pos;
    uint32_t seed;
};

struct AIHackPacket
{
    uint32_t targetId;
    uint8_t effectId;
    uint8_t _pad[3];
};

struct BossPhasePacket
{
    uint32_t npcId;
    uint8_t phaseIdx;
    uint8_t _pad[3];
};

struct SectorLODPacket
{
    uint64_t sectorHash;
    uint8_t lod;
    uint8_t _pad[3];
};

struct LowBWModePacket
{
    uint8_t enable;
    uint8_t _pad[3];
};

struct CrowdCfgPacket
{
    uint8_t density;
    uint8_t _pad[3];
};

struct AvatarSpawnPacket
{
    uint32_t peerId;
    TransformSnap snap;
    uint32_t phaseId; // PX-1
};

struct AvatarDespawnPacket
{
    uint32_t peerId;
    uint32_t phaseId; // PX-1
};

struct ChatPacket
{
    uint32_t peerId;
    char msg[64];
};

struct EmotePacket
{
    uint32_t peerId;
    uint8_t emoteId;
    uint8_t _pad[3];
};

struct CrowdChatterStartPacket
{
    uint32_t npcA;
    uint32_t npcB;
    uint32_t lineId;
    uint32_t seed;
};

struct CrowdChatterEndPacket
{
    uint32_t convId;
};

struct HoloSeedPacket
{
    uint64_t sectorHash;
    uint64_t seed64;
};

struct HoloNextAdPacket
{
    uint64_t sectorHash;
    uint32_t adId;
};

struct DoorBreachStartPacket
{
    uint32_t doorId;
    uint32_t phaseId;
    uint32_t seed;
};

struct DoorBreachTickPacket
{
    uint32_t doorId;
    uint8_t percent;
    uint8_t _pad[3];
};

struct DoorBreachSuccessPacket
{
    uint32_t doorId;
};

struct DoorBreachAbortPacket
{
    uint32_t doorId;
};

struct ItemGrabPacket
{
    uint32_t peerId;
    uint32_t itemId;
};

struct ItemDropPacket
{
    uint32_t peerId;
    uint32_t itemId;
    RED4ext::Vector3 pos;
};

struct ItemStorePacket
{
    uint32_t peerId;
    uint32_t itemId;
};

struct MetroBoardPacket
{
    uint32_t peerId;
    uint32_t lineId;
    uint8_t carIdx;
    uint8_t _pad[3];
};

struct MetroArrivePacket
{
    uint32_t peerId;
    uint32_t stationId;
};

struct RadioChangePacket
{
    uint32_t vehId;
    uint8_t stationId;
    uint8_t _pad;
    uint32_t offsetSec;
};

struct CamHijackPacket
{
    uint32_t camId;
    uint32_t peerId;
};

struct CamFrameStartPacket
{
    uint32_t camId;
};

struct CarryBeginPacket
{
    uint32_t carrierId;
    uint32_t entityId;
};

struct CarrySnapPacket
{
    uint32_t entityId;
    RED4ext::Vector3 pos;
    RED4ext::Vector3 vel;
};

struct CarryEndPacket
{
    uint32_t entityId;
    RED4ext::Vector3 pos;
    RED4ext::Vector3 vel;
};

struct GrenadePrimePacket
{
    uint32_t entityId;
    uint32_t startTick;
};

struct GrenadeSnapPacket
{
    uint32_t entityId;
    RED4ext::Vector3 pos;
    RED4ext::Vector3 vel;
};

struct SmartCamStartPacket
{
    uint32_t projId;
};

struct SmartCamEndPacket
{
    uint32_t projId;
};

} // namespace CoopNet
