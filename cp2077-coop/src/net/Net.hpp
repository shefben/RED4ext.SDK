#pragma once

// Networking layer for cp2077-coop.
// Provides thin wrappers around ENet.
#include "Packets.hpp"
#include <RED4ext/Scripting/Natives/Generated/Vector3.hpp>
#include <cstdint>
#include <vector>

namespace CoopNet
{
class Connection;
struct NetStats
{
    uint32_t ping;
    float loss;
    uint16_t vKbps;
    uint16_t sKbps;
    uint16_t dropPkts;
};
} // namespace CoopNet
void Net_Init();
void Net_Shutdown();
void Net_Poll(uint32_t maxMs);
bool Net_IsAuthoritative();
std::vector<CoopNet::Connection*> Net_GetConnections();
void Net_Send(CoopNet::Connection* conn, CoopNet::EMsg type, const void* data, uint16_t size);
void Net_Broadcast(CoopNet::EMsg type, const void* data, uint16_t size);
void Net_SendUnreliableToAll(CoopNet::EMsg type, const void* data, uint16_t size);
void Net_SendSectorReady(uint64_t hash);
void Net_SendCraftRequest(uint32_t recipeId);
void Net_SendAttachRequest(uint64_t itemId, uint8_t slotIdx, uint64_t attachmentId);
void Net_SendBreachInput(uint8_t index);
void Net_BroadcastVehicleExplode(uint32_t vehicleId, uint32_t vfxId, uint32_t seed);
void Net_BroadcastPartDetach(uint32_t vehicleId, uint8_t partId);
void Net_BroadcastEject(uint32_t peerId, const RED4ext::Vector3& vel);
void Net_BroadcastVehicleSpawn(uint32_t vehicleId, uint32_t archetypeId, uint32_t paintId, const TransformSnap& t);
void Net_SendSeatRequest(uint32_t vehicleId, uint8_t seatIdx);
void Net_BroadcastSeatAssign(uint32_t peerId, uint32_t vehicleId, uint8_t seatIdx);
void Net_SendVehicleHit(uint32_t vehicleId, uint16_t dmg, bool side);
void Net_BroadcastVehicleHit(uint32_t vehicleId, uint16_t dmg);
void Net_BroadcastBreachStart(uint32_t peerId, uint32_t seed, uint8_t w, uint8_t h);
void Net_BroadcastBreachInput(uint32_t peerId, uint8_t index);
void Net_BroadcastBreachResult(uint32_t peerId, uint8_t mask);
void Net_BroadcastHeat(uint8_t level);
void Net_BroadcastElevatorCall(uint32_t peerId, uint32_t elevatorId, uint8_t floorIdx);
void Net_SendElevatorCall(uint32_t elevatorId, uint8_t floorIdx);
void Net_BroadcastElevatorArrive(uint32_t elevatorId, uint64_t sectorHash, const RED4ext::Vector3& pos);
void Net_SendTeleportAck(uint32_t elevatorId);
void Net_BroadcastQuestStage(uint32_t nameHash, uint16_t stage);
void Net_BroadcastQuestStageP2P(uint32_t phaseId, uint32_t questHash, uint16_t stage); // PX-2
void Net_SendQuestResyncRequest();
void Net_SendQuestResyncRequestTo(CoopNet::Connection* conn);
CoopNet::Connection* Net_FindConnection(uint32_t peerId);
void Net_SendQuestFullSync(CoopNet::Connection* conn, const QuestFullSyncPacket& pkt);
void Net_BroadcastHoloCallStart(uint32_t peerId);
void Net_BroadcastHoloCallEnd(uint32_t peerId);
void Net_BroadcastTickRateChange(uint16_t tickMs);
void Net_BroadcastRuleChange(bool friendly);
void Net_SendSpectateRequest(uint32_t peerId);
void Net_SendSpectateGranted(uint32_t peerId);
void Net_SendAdminCmd(CoopNet::Connection* conn, uint8_t cmdType, uint64_t param);
void Net_BroadcastScoreUpdate(uint32_t peerId, uint16_t k, uint16_t d);
void Net_BroadcastMatchOver(uint32_t winnerId);
void Net_Disconnect(CoopNet::Connection* conn);
void Nat_Start();
void Nat_PerformHandshake(CoopNet::Connection* conn);
uint64_t Nat_GetRelayBytes();
void Net_BroadcastNatCandidate(const char* sdp);
void Net_BroadcastCineStart(uint32_t sceneId, uint32_t startTimeMs, uint32_t phaseId, bool solo); // PX-4
void Net_BroadcastViseme(uint32_t npcId, uint8_t visemeId, uint32_t timeMs);
void Net_SendDialogChoice(uint8_t choiceIdx);
void Net_BroadcastDialogChoice(uint32_t peerId, uint8_t choiceIdx);
void Net_SendVoice(const uint8_t* data, uint16_t size, uint16_t seq);
void Net_BroadcastVoice(uint32_t peerId, const uint8_t* data, uint16_t size, uint16_t seq);
void Net_BroadcastWorldState(uint16_t sunAngleDeg, uint8_t weatherId);
void Net_BroadcastGlobalEvent(uint32_t eventId, uint8_t phase, bool start, uint32_t seed);
void Net_BroadcastCrowdSeed(uint64_t sectorHash, uint32_t seed);
void Net_BroadcastVendorStock(const VendorStockPacket& pkt);
void Net_BroadcastVendorStockUpdate(const VendorStockUpdatePacket& pkt);
void Net_SendPurchaseRequest(uint32_t vendorId, uint32_t itemId, uint64_t nonce);
void Net_SendWorldMarkers(CoopNet::Connection* conn, const std::vector<uint8_t>& blob);
std::vector<uint8_t> BuildMarkerBlob();
void ApplyMarkerBlob(const uint8_t* buf, size_t len);
void Net_BroadcastNpcSpawnCruiser(uint8_t waveIdx, const uint32_t seeds[4]);
void Net_BroadcastNpcState(uint32_t npcId, uint8_t aiState);
void Net_BroadcastCrimeEvent(const CrimeEventSpawnPacket& pkt);
void Net_BroadcastCyberEquip(uint32_t peerId, uint8_t slotId, const ItemSnap& snap);
void Net_BroadcastSlowMoStart(uint32_t peerId, float factor, uint16_t durationMs);
void Net_SendPerkUnlock(uint32_t perkId, uint8_t rank);
void Net_BroadcastPerkUnlock(uint32_t peerId, uint32_t perkId, uint8_t rank);
void Net_SendPerkRespecRequest();
void Net_SendPerkRespecAck(CoopNet::Connection* conn, uint16_t newPoints);
void Net_BroadcastStatusApply(uint32_t targetId, uint8_t effectId, uint16_t durMs, uint8_t amp);
void Net_BroadcastStatusTick(uint32_t targetId, int16_t hpDelta);
void Net_BroadcastTrafficSeed(uint64_t sectorHash, uint64_t seed);
void Net_BroadcastTrafficDespawn(uint32_t vehId);
void Net_BroadcastPropBreak(uint32_t entityId, uint32_t seed);
void Net_BroadcastPropIgnite(uint32_t entityId, uint16_t delayMs);
void Net_BroadcastVOPlay(uint32_t lineId);
void Net_SendVehicleSummonRequest(uint32_t vehId, const TransformSnap& pos);
void Net_BroadcastFixerCallStart(uint32_t fixerId);
void Net_BroadcastFixerCallEnd(uint32_t fixerId);
void Net_BroadcastGigSpawn(uint32_t questId, uint32_t seed);
void Net_BroadcastVehicleSummon(uint32_t vehId, uint32_t ownerId, const TransformSnap& pos);
void Net_BroadcastAppearance(uint32_t peerId, uint32_t meshId, uint32_t tintId);
void Net_BroadcastPingOutline(uint32_t peerId, uint16_t durationMs, const std::vector<uint32_t>& ids);
void Net_BroadcastLootRoll(uint32_t containerId, uint32_t seed);
void Net_SendDealerBuy(uint32_t vehicleTpl, uint32_t price);
void Net_BroadcastVehicleUnlock(uint32_t peerId, uint32_t vehicleTpl);
void Net_BroadcastWeaponInspect(uint32_t peerId, uint16_t animId);
void Net_BroadcastFinisherStart(uint32_t actorId, uint32_t victimId, uint16_t animId);
void Net_BroadcastFinisherEnd(uint32_t actorId);
void Net_BroadcastTextureBiasChange(uint8_t bias);
void Net_BroadcastCriticalVoteStart(uint32_t questHash);                                                 // PX-6
void Net_SendCriticalVoteCast(bool yes);                                                                 // PX-6
void Net_SendPhaseBundle(CoopNet::Connection* conn, uint32_t phaseId, const std::vector<uint8_t>& blob); // PX-7
std::vector<uint32_t> QuestWatchdog_ListPhases();                                                        // PX-7 helper
std::vector<uint8_t> BuildPhaseBundle(uint32_t phaseId);                                                 // PX-7
void ApplyPhaseBundle(uint32_t phaseId, const uint8_t* buf, size_t len);                                 // PX-7
