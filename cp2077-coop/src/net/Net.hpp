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
}
void Net_Init();
void Net_Shutdown();
void Net_Poll(uint32_t maxMs);
bool Net_IsAuthoritative();
std::vector<CoopNet::Connection*> Net_GetConnections();
void Net_Send(CoopNet::Connection* conn, CoopNet::EMsg type, const void* data, uint16_t size);
void Net_Broadcast(CoopNet::EMsg type, const void* data, uint16_t size);
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
void Net_BroadcastCineStart(uint32_t sceneId, uint32_t startTimeMs);
void Net_BroadcastViseme(uint32_t npcId, uint8_t visemeId, uint32_t timeMs);
void Net_SendDialogChoice(uint8_t choiceIdx);
void Net_BroadcastDialogChoice(uint32_t peerId, uint8_t choiceIdx);
void Net_SendVoice(const uint8_t* data, uint16_t size, uint16_t seq);
void Net_BroadcastVoice(uint32_t peerId, const uint8_t* data, uint16_t size, uint16_t seq);
void Net_BroadcastWorldState(uint64_t clockMs, uint32_t sunAngle, uint8_t weatherId, uint32_t weatherSeed,
                             uint8_t bdPhase);
void Net_BroadcastGlobalEvent(uint32_t eventId, uint8_t phase, bool start, uint32_t seed);
void Net_BroadcastCrowdSeed(uint64_t sectorHash, uint32_t seed);
void Net_BroadcastVendorStock(const VendorStockPacket& pkt);
void Net_SendPurchaseRequest(uint32_t vendorId, uint32_t itemId, uint64_t nonce);
