#pragma once

// Networking layer for cp2077-coop.
// Provides thin wrappers around ENet.
#include "../core/QuestGadget.hpp"
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
bool Net_IsConnected();
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
void Net_BroadcastVehicleSpawn(uint32_t vehicleId, uint32_t archetypeId, uint32_t paintId, uint32_t phaseId,
                               const TransformSnap& t);
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
void Net_SendJoinRequest(uint32_t serverId);
void Net_BroadcastQuestStage(uint32_t nameHash, uint16_t stage);
void Net_BroadcastQuestStageP2P(uint32_t phaseId, uint32_t questHash, uint16_t stage); // PX-2
void Net_SendQuestResyncRequest();
void Net_SendQuestResyncRequestTo(CoopNet::Connection* conn);
CoopNet::Connection* Net_FindConnection(uint32_t peerId);
void Net_SendQuestFullSync(CoopNet::Connection* conn, const QuestFullSyncPacket& pkt);
void Net_BroadcastHoloCallStart(uint32_t fixerId, uint32_t callId, const uint32_t* peerIds, uint8_t count);
void Net_BroadcastHoloCallEnd(uint32_t callId);
void Net_BroadcastTickRateChange(uint16_t tickMs);
void Net_BroadcastRuleChange(bool friendly);
void Net_SendSpectateRequest(uint32_t peerId);
void Net_SendSpectateGranted(uint32_t peerId);
void Net_SendAdminCmd(CoopNet::Connection* conn, uint8_t cmdType, uint64_t param);
void Net_BroadcastScoreUpdate(uint32_t peerId, uint16_t k, uint16_t d);
void Net_BroadcastMatchOver(uint32_t winnerId);
void Net_BroadcastChat(const std::string& msg);
void Net_BroadcastKillfeed(const std::string& msg);
void Net_Disconnect(CoopNet::Connection* conn);
void Nat_Start();
void Nat_PerformHandshake(CoopNet::Connection* conn);
uint64_t Nat_GetRelayBytes();
void Net_BroadcastNatCandidate(const char* sdp);
void Net_BroadcastCineStart(uint32_t sceneId, uint32_t startTimeMs, uint32_t phaseId, bool solo); // PX-4
void Net_BroadcastViseme(uint32_t npcId, uint8_t visemeId, uint32_t timeMs);
void Net_SendDialogChoice(uint8_t choiceIdx);
void Net_BroadcastDialogChoice(uint32_t peerId, uint8_t choiceIdx);
void Net_BroadcastSceneTrigger(uint32_t phaseId, uint32_t nameHash, bool start);
void Net_SendVoiceCaps(CoopNet::Connection* conn, uint16_t maxBytes);
void Net_SendVoice(const uint8_t* data, uint16_t size, uint16_t seq);
void Net_BroadcastVoice(uint32_t peerId, const uint8_t* data, uint16_t size, uint16_t seq);
void Net_BroadcastWorldState(uint16_t sunAngleDeg, uint8_t weatherId, uint16_t particleSeed);
void Net_SendWorldState(CoopNet::Connection* conn, uint16_t sunAngleDeg, uint8_t weatherId, uint16_t particleSeed);
void Net_SendGlobalEvent(CoopNet::Connection* conn, uint32_t eventId, uint8_t phase, bool start, uint32_t seed);
void Net_SendNpcReputation(CoopNet::Connection* conn, uint32_t npcId, int16_t value);
void Net_BroadcastNpcReputation(uint32_t npcId, int16_t value);
void Net_BroadcastGlobalEvent(uint32_t eventId, uint8_t phase, bool start, uint32_t seed);
void Net_BroadcastDynamicEvent(uint8_t eventType, uint32_t seed); // DE-1
void Net_BroadcastCrowdSeed(uint64_t sectorHash, uint32_t seed);
void Net_BroadcastVendorStock(const VendorStockPacket& pkt);
void Net_BroadcastVendorStockUpdate(const VendorStockUpdatePacket& pkt);
void Net_BroadcastVendorRefresh(const VendorRefreshPacket& pkt);
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
void Net_SendSkillXP(uint16_t skillId, int16_t deltaXP);                       // SX-1
void Net_BroadcastSkillXP(uint32_t peerId, uint16_t skillId, int16_t deltaXP); // SX-1
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
void Net_BroadcastVehicleHitHighSpeed(uint32_t vehA, uint32_t vehB, const RED4ext::Vector3& delta);
void Net_BroadcastWeaponInspect(uint32_t peerId, uint16_t animId);
void Net_BroadcastFinisherStart(uint32_t actorId, uint32_t victimId, uint8_t finisherType);
void Net_BroadcastFinisherEnd(uint32_t actorId);
void Net_BroadcastSlowMoFinisher(uint32_t peerId, uint32_t targetId, uint16_t durMs); // RB-1
void Net_BroadcastTextureBiasChange(uint8_t bias);
void Net_BroadcastCriticalVoteStart(uint32_t questHash);                                                 // PX-6
void Net_SendCriticalVoteCast(bool yes);                                                                 // PX-6
void Net_SendPhaseBundle(CoopNet::Connection* conn, uint32_t phaseId, const std::vector<uint8_t>& blob); // PX-7
std::vector<uint32_t> QuestWatchdog_ListPhases();                                                        // PX-7 helper
std::vector<uint8_t> BuildPhaseBundle(uint32_t phaseId);                                                 // PX-7
void ApplyPhaseBundle(uint32_t phaseId, const uint8_t* buf, size_t len);                                 // PX-7
void Net_SendAptPurchase(uint32_t aptId);
void Net_SendAptEnterReq(uint32_t aptId, uint32_t ownerPhaseId);
void Net_SendAptPermChange(uint32_t aptId, uint32_t targetPeerId, bool allow);
void Net_SendAptShareChange(uint32_t aptId, uint32_t targetPeerId, bool allow);
void Net_SendAptInteriorStateReq(const char* json, uint32_t len);
void Net_BroadcastAptInteriorState(uint32_t phaseId, const char* json, uint16_t len);
void Net_BroadcastAptPermChange(uint32_t aptId, uint32_t targetPeerId, bool allow);
void Net_SendAptPurchaseAck(CoopNet::Connection* conn, uint32_t aptId, bool success, uint64_t balance);
void Net_SendAptEnterAck(CoopNet::Connection* conn, bool allow, uint32_t phaseId, uint32_t interiorSeed);
void Net_SendVehicleTowRequest(const RED4ext::Vector3& pos);
void Net_SendVehicleTowAck(CoopNet::Connection* conn, uint32_t ownerId, bool ok);
void Net_SendReRollRequest(uint64_t itemId, uint32_t seed);                 // WM-1
void Net_SendReRollResult(CoopNet::Connection* conn, const ItemSnap& snap); // WM-1
void Net_SendRipperInstallRequest(uint8_t slotId);
void Net_SendTileSelect(uint8_t row, uint8_t col);                // MG-1
void Net_BroadcastTileGameStart(uint32_t phaseId, uint32_t seed); // MG-1
void Net_BroadcastTileSelect(uint32_t peerId, uint32_t phaseId, uint8_t row,
                             uint8_t col);                          // MG-1
void Net_BroadcastShardProgress(uint32_t phaseId, uint8_t percent); // MG-2
void Net_SendTradeInit(uint32_t targetPeerId);                      // TRD-1
void Net_SendTradeOffer(const ItemSnap* items, uint8_t count,
                        uint32_t eddies);                                                  // TRD-1
void Net_SendTradeAccept(bool accept);                                                     // TRD-1
void Net_BroadcastTradeFinalize(bool success);                                             // TRD-1
void Net_BroadcastEndingVoteStart(uint32_t questHash);                                     // EG-1
void Net_SendEndingVoteCast(bool yes);                                                     // EG-1
void Net_BroadcastVehicleSnap(const VehicleSnap& snap);                                    // VT-1
void Net_BroadcastTurretAim(uint32_t vehId, float yaw, float pitch);                       // VT-2
void Net_BroadcastAirVehSpawn(uint32_t vehId, const RED4ext::Vector3* pts, uint8_t count); // VT-3
void Net_BroadcastAirVehUpdate(uint32_t vehId, const TransformSnap& t);                    // VT-3
void Net_BroadcastVehiclePaintChange(uint32_t vehId, uint32_t colorId, const char* plate); // VT-4
void Net_BroadcastPanicEvent(const RED4ext::Vector3& pos, uint32_t seed);                  // AI-1
void Net_BroadcastAIHack(uint32_t targetId, uint8_t effectId);                             // AI-2
void Net_BroadcastBossPhase(uint32_t npcId, uint8_t phaseIdx);                             // AI-3
void Net_BroadcastSectorLOD(uint64_t sectorHash, uint8_t lod);                             // PRF-1
void Net_SendLowBWMode(CoopNet::Connection* conn, bool enable);                            // PRF-2
void Net_SendCrowdCfg(CoopNet::Connection* conn, uint8_t density);
// CD-1
void Net_BroadcastEmote(uint32_t peerId, uint8_t emoteId); // EM-1
void Net_BroadcastCrowdChatterStart(uint32_t npcA, uint32_t npcB, uint32_t lineId,
                                    uint32_t seed);               // CA-1
void Net_BroadcastCrowdChatterEnd(uint32_t convId);               // CA-1
void Net_BroadcastHoloSeed(uint64_t sectorHash, uint64_t seed64); // HB-1
void Net_BroadcastHoloNextAd(uint64_t sectorHash, uint32_t adId); // HB-1
void Net_BroadcastDoorBreachStart(uint32_t doorId, uint32_t phaseId,
                                  uint32_t seed);                                                             // DH-1
void Net_BroadcastDoorBreachTick(uint32_t doorId, uint8_t percent);                                           // DH-1
void Net_BroadcastDoorBreachSuccess(uint32_t doorId);                                                         // DH-1
void Net_BroadcastDoorBreachAbort(uint32_t doorId);                                                           // DH-1
void Net_BroadcastHTableOpen(uint32_t sceneId);                                                               // HT-1
void Net_BroadcastHTableScrub(uint32_t timestampMs);                                                          // HT-1
void Net_BroadcastQuestGadgetFire(uint32_t questId, QuestGadgetType type, uint8_t charge, uint32_t targetId); // QG-1
void Net_BroadcastItemGrab(uint32_t peerId, uint32_t itemId);                                                 // IP-1
void Net_BroadcastItemDrop(uint32_t peerId, uint32_t itemId, const RED4ext::Vector3& pos);                    // IP-1
void Net_BroadcastItemStore(uint32_t peerId, uint32_t itemId);                                                // IP-1
void Net_BroadcastMetroBoard(uint32_t peerId, uint32_t lineId, uint8_t carIdx);                               // SB-1
void Net_BroadcastMetroArrive(uint32_t peerId, uint32_t stationId);                                           // SB-2
void Net_BroadcastRadioChange(uint32_t vehId, uint8_t stationId, uint32_t offsetSec);                         // RS-1
void Net_BroadcastCamHijack(uint32_t camId, uint32_t peerId);                                                 // SF-1
void Net_BroadcastCamFrameStart(uint32_t camId);                                                              // SF-1
void Net_BroadcastCarryBegin(uint32_t carrierId, uint32_t entityId);                                          // PC-1
void Net_BroadcastCarrySnap(uint32_t entityId, const RED4ext::Vector3& pos, const RED4ext::Vector3& vel);     // PC-1
void Net_BroadcastCarryEnd(uint32_t entityId, const RED4ext::Vector3& pos, const RED4ext::Vector3& vel);      // PC-1
void Net_BroadcastGrenadePrime(uint32_t entityId, uint32_t startTick);                                        // GR-1
void Net_BroadcastGrenadeSnap(uint32_t entityId, const RED4ext::Vector3& pos, const RED4ext::Vector3& vel);   // GR-1
void Net_BroadcastSmartCamStart(uint32_t projId);                                                             // RC-1
void Net_BroadcastSmartCamEnd(uint32_t projId);                                                               // RC-1
void Net_BroadcastArcadeStart(uint32_t cabId, uint32_t peerId, uint32_t seed);
void Net_SendArcadeInput(uint32_t frame, uint8_t buttonMask);
void Net_BroadcastArcadeScore(uint32_t peerId, uint32_t score);
void Net_BroadcastArcadeHighScore(uint32_t cabId, uint32_t peerId, uint32_t score);
void Net_SendPluginRPC(CoopNet::Connection* conn, uint16_t pluginId, uint32_t fnHash,
                       const char* json, uint16_t len);
void Net_BroadcastPluginRPC(uint16_t pluginId, uint32_t fnHash, const char* json,
                            uint16_t len);
void Net_BroadcastAssetBundle(uint16_t pluginId, const std::vector<uint8_t>& data);
