#include "Net.hpp"
#include "../core/Hash.hpp"
#include "../server/AdminController.hpp"
#include "../server/PoliceDispatch.hpp"
#include "../server/QuestWatchdog.hpp"
#include "Connection.hpp"
#include "NatClient.hpp"
#include "NetConfig.hpp"
#include "Packets.hpp"
#include <algorithm>
#include <cstring>
#include <enet/enet.h>
#include <iostream>
#include <vector>

using CoopNet::Connection;

namespace
{
struct PeerEntry
{
    ENetPeer* peer;
    Connection* conn;
};

ENetHost* g_Host = nullptr;
std::vector<PeerEntry> g_Peers;
static uint32_t g_nextPeerId = 1;
// Helper used by world streaming to match sector hashing in the game.
} // namespace

void Net_Init()
{
    if (enet_initialize() != 0)
    {
        std::cout << "enet_initialize failed" << std::endl;
        return;
    }

    g_Host = enet_host_create(nullptr, 8, 2, 0, 0);
    Nat_SetCandidateCallback(
        [](const char* cand)
        {
            std::cout << "Local candidate: " << cand << std::endl;
            Net_BroadcastNatCandidate(cand);
        });
    Nat_Start();
    std::cout << "Net_Init complete" << std::endl;
}

void Net_Shutdown()
{
    for (auto& e : g_Peers)
    {
        delete e.conn;
    }
    g_Peers.clear();

    if (g_Host)
    {
        enet_host_destroy(g_Host);
        g_Host = nullptr;
    }

    enet_deinitialize();
    std::cout << "Net_Shutdown complete" << std::endl;
}

void Net_Poll(uint32_t maxMs)
{
    if (!g_Host)
        return;

    ENetEvent evt;
    int wait = static_cast<int>(maxMs);
    while (enet_host_service(g_Host, &evt, wait) > 0)
    {
        wait = 0; // subsequent polls are non-blocking
        switch (evt.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
        {
            PeerEntry e;
            e.peer = evt.peer;
            e.conn = new Connection();
            e.conn->peerId = g_nextPeerId++;
            if (CoopNet::AdminController_IsBanned(e.conn->peerId))
            {
                enet_peer_disconnect(evt.peer, 0);
                delete e.conn;
            }
            else
            {
                g_Peers.push_back(e);
                std::cout << "peer connected id=" << e.conn->peerId << std::endl;
                Nat_PerformHandshake(e.conn);
                e.conn->SendSectorChange(CoopNet::Fnv1a64("start_sector"));
            }
            break;
        }
        case ENET_EVENT_TYPE_DISCONNECT:
        {
            auto it =
                std::find_if(g_Peers.begin(), g_Peers.end(), [&](const PeerEntry& p) { return p.peer == evt.peer; });
            if (it != g_Peers.end())
            {
                delete it->conn;
                g_Peers.erase(it);
            }
            std::cout << "peer disconnected" << std::endl;
            break;
        }
        case ENET_EVENT_TYPE_RECEIVE:
        {
            if (evt.packet && evt.packet->dataLength >= sizeof(PacketHeader))
            {
                auto it = std::find_if(g_Peers.begin(), g_Peers.end(),
                                       [&](const PeerEntry& p) { return p.peer == evt.peer; });
                if (it != g_Peers.end())
                {
                    Connection::RawPacket pkt;
                    pkt.hdr = *reinterpret_cast<PacketHeader*>(evt.packet->data);
                    pkt.data.resize(evt.packet->dataLength - sizeof(PacketHeader));
                    memcpy(pkt.data.data(), evt.packet->data + sizeof(PacketHeader), pkt.data.size());
                    it->conn->EnqueuePacket(pkt);
                }
            }
            break;
        }
        default:
            break;
        }
    }
}

bool Net_IsAuthoritative()
{
    return CoopNet::kDedicatedAuthority;
}

std::vector<Connection*> Net_GetConnections()
{
    std::vector<Connection*> out;
    out.reserve(g_Peers.size());
    for (auto& e : g_Peers)
        out.push_back(e.conn);
    return out;
}

void Net_Send(Connection* conn, EMsg type, const void* data, uint16_t size)
{
    if (!g_Host || !conn)
        return;

    auto it = std::find_if(g_Peers.begin(), g_Peers.end(), [&](const PeerEntry& p) { return p.conn == conn; });
    if (it == g_Peers.end())
        return;

    ENetPacket* pkt = enet_packet_create(nullptr, sizeof(PacketHeader) + size, ENET_PACKET_FLAG_RELIABLE);
    PacketHeader hdr{static_cast<uint16_t>(type), size};
    std::memcpy(pkt->data, &hdr, sizeof(hdr));
    if (size > 0 && data)
        std::memcpy(pkt->data + sizeof(hdr), data, size);
    enet_peer_send(it->peer, 0, pkt);
}

void Net_Broadcast(EMsg type, const void* data, uint16_t size)
{
    if (!g_Host)
        return;

    ENetPacket* pkt = enet_packet_create(nullptr, sizeof(PacketHeader) + size, ENET_PACKET_FLAG_RELIABLE);
    PacketHeader hdr{static_cast<uint16_t>(type), size};
    std::memcpy(pkt->data, &hdr, sizeof(hdr));
    if (size > 0 && data)
        std::memcpy(pkt->data + sizeof(hdr), data, size);
    enet_host_broadcast(g_Host, 0, pkt);
}

void Net_SendUnreliableToAll(EMsg type, const void* data, uint16_t size)
{
    if (!g_Host)
        return;

    ENetPacket* pkt = enet_packet_create(nullptr, sizeof(PacketHeader) + size, 0);
    PacketHeader hdr{static_cast<uint16_t>(type), size};
    std::memcpy(pkt->data, &hdr, sizeof(hdr));
    if (size > 0 && data)
        std::memcpy(pkt->data + sizeof(hdr), data, size);
    enet_host_broadcast(g_Host, 0, pkt);
}

void Net_SendSectorReady(uint64_t hash)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        conns[0]->SendSectorReady(hash);
    }
}

void Net_SendCraftRequest(uint32_t recipeId)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        CraftRequestPacket pkt{recipeId};
        Net_Send(conns[0], EMsg::CraftRequest, &pkt, sizeof(pkt));
    }
}

void Net_SendAttachRequest(uint64_t itemId, uint8_t slotIdx, uint64_t attachmentId)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        AttachModRequestPacket pkt{itemId, slotIdx, {0, 0, 0}, attachmentId};
        Net_Send(conns[0], EMsg::AttachModRequest, &pkt, sizeof(pkt));
    }
}

void Net_SendPurchaseRequest(uint32_t vendorId, uint32_t itemId, uint64_t nonce)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        PurchaseRequestPacket pkt{vendorId, itemId, nonce};
        Net_Send(conns[0], EMsg::PurchaseRequest, &pkt, sizeof(pkt));
    }
}

void Net_SendVehicleSummonRequest(uint32_t vehId, const TransformSnap& pos)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        VehicleSummonRequestPacket pkt{vehId, pos};
        Net_Send(conns[0], EMsg::VehicleSummonRequest, &pkt, sizeof(pkt));
    }
}

void Net_SendBreachInput(uint8_t index)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        BreachInputPacket pkt{0u, index, {0, 0, 0}};
        Net_Send(conns[0], EMsg::BreachInput, &pkt, sizeof(pkt));
    }
}

void Net_SendElevatorCall(uint32_t elevatorId, uint8_t floorIdx)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        ElevatorCallPacket pkt{0u, elevatorId, floorIdx, {0, 0, 0}};
        Net_Send(conns[0], EMsg::ElevatorCall, &pkt, sizeof(pkt));
    }
}

void Net_BroadcastVehicleExplode(uint32_t vehicleId, uint32_t vfxId, uint32_t seed)
{
    VehicleExplodePacket pkt{vehicleId, vfxId, seed};
    Net_Broadcast(EMsg::VehicleExplode, &pkt, sizeof(pkt));
}

void Net_BroadcastPartDetach(uint32_t vehicleId, uint8_t partId)
{
    VehiclePartDetachPacket pkt{vehicleId, partId, {0, 0, 0}};
    Net_Broadcast(EMsg::VehiclePartDetach, &pkt, sizeof(pkt));
}

void Net_BroadcastEject(uint32_t peerId, const RED4ext::Vector3& vel)
{
    EjectOccupantPacket pkt{peerId, vel};
    Net_Broadcast(EMsg::EjectOccupant, &pkt, sizeof(pkt));
}

void Net_BroadcastVehicleSpawn(uint32_t vehicleId, uint32_t archetypeId, uint32_t paintId, const TransformSnap& t)
{
    VehicleSpawnPacket pkt{vehicleId, archetypeId, paintId, t};
    Net_Broadcast(EMsg::VehicleSpawn, &pkt, sizeof(pkt));
}

void Net_SendSeatRequest(uint32_t vehicleId, uint8_t seatIdx)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        SeatRequestPacket pkt{vehicleId, seatIdx};
        Net_Send(conns[0], EMsg::SeatRequest, &pkt, sizeof(pkt));
    }
}

void Net_BroadcastSeatAssign(uint32_t peerId, uint32_t vehicleId, uint8_t seatIdx)
{
    SeatAssignPacket pkt{peerId, vehicleId, seatIdx};
    Net_Broadcast(EMsg::SeatAssign, &pkt, sizeof(pkt));
}

void Net_SendVehicleHit(uint32_t vehicleId, uint16_t dmg, bool side)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        VehicleHitPacket pkt{vehicleId, dmg};
        pkt.pad = side ? 1u : 0u;
        Net_Send(conns[0], EMsg::VehicleHit, &pkt, sizeof(pkt));
    }
}

void Net_BroadcastVehicleHit(uint32_t vehicleId, uint16_t dmg)
{
    VehicleHitPacket pkt{vehicleId, dmg};
    Net_Broadcast(EMsg::VehicleHit, &pkt, sizeof(pkt));
}

void Net_BroadcastBreachStart(uint32_t peerId, uint32_t seed, uint8_t w, uint8_t h)
{
    BreachStartPacket pkt{peerId, seed, w, h, {0, 0}};
    Net_Broadcast(EMsg::BreachStart, &pkt, sizeof(pkt));
}

void Net_BroadcastBreachInput(uint32_t peerId, uint8_t index)
{
    BreachInputPacket pkt{peerId, index, {0, 0, 0}};
    Net_Broadcast(EMsg::BreachInput, &pkt, sizeof(pkt));
}

void Net_BroadcastBreachResult(uint32_t peerId, uint8_t mask)
{
    BreachResultPacket pkt{peerId, mask, {0, 0, 0}};
    Net_Broadcast(EMsg::BreachResult, &pkt, sizeof(pkt));
}

void Net_BroadcastHeat(uint8_t level)
{
    HeatPacket pkt{level, {0, 0, 0}};
    Net_Broadcast(EMsg::HeatSync, &pkt, sizeof(pkt));
    PoliceDispatch_OnHeatChange(level);
}

void Net_BroadcastElevatorCall(uint32_t peerId, uint32_t elevatorId, uint8_t floorIdx)
{
    ElevatorCallPacket pkt{peerId, elevatorId, floorIdx, {0, 0, 0}};
    Net_Broadcast(EMsg::ElevatorCall, &pkt, sizeof(pkt));
}

void Net_BroadcastElevatorArrive(uint32_t elevatorId, uint64_t sectorHash, const RED4ext::Vector3& pos)
{
    ElevatorArrivePacket pkt{elevatorId, sectorHash, pos};
    Net_Broadcast(EMsg::ElevatorArrive, &pkt, sizeof(pkt));
}

void Net_SendTeleportAck(uint32_t elevatorId)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        TeleportAckPacket pkt{elevatorId};
        Net_Send(conns[0], EMsg::TeleportAck, &pkt, sizeof(pkt));
    }
}

void Net_BroadcastQuestStage(uint32_t nameHash, uint16_t stage)
{
    QuestStagePacket pkt{nameHash, stage, 0};
    for (auto& e : g_Peers)
        CoopNet::QuestWatchdog_Record(e.conn->peerId, nameHash, stage);
    Net_Broadcast(EMsg::QuestStage, &pkt, sizeof(pkt));
}

void Net_BroadcastQuestStageP2P(uint32_t phaseId, uint32_t questHash, uint16_t stage)
{
    QuestStageP2PPacket pkt{phaseId, questHash, stage, 0};
    for (auto& e : g_Peers)
        CoopNet::QuestWatchdog_Record(phaseId, questHash, stage);
    Net_Broadcast(EMsg::QuestStageP2P, &pkt, sizeof(pkt));
}

void Net_SendQuestResyncRequest()
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        QuestResyncRequestPacket pkt{0};
        Net_Send(conns[0], EMsg::QuestResyncRequest, &pkt, sizeof(pkt));
    }
}

void Net_SendQuestResyncRequestTo(Connection* conn)
{
    if (!conn)
        return;
    QuestResyncRequestPacket pkt{0};
    Net_Send(conn, EMsg::QuestResyncRequest, &pkt, sizeof(pkt));
}

Connection* Net_FindConnection(uint32_t peerId)
{
    auto it =
        std::find_if(g_Peers.begin(), g_Peers.end(), [&](const PeerEntry& p) { return p.conn->peerId == peerId; });
    if (it == g_Peers.end())
        return nullptr;
    return it->conn;
}

void Net_SendQuestFullSync(CoopNet::Connection* conn, const QuestFullSyncPacket& pkt)
{
    Net_Send(conn, EMsg::QuestFullSync, &pkt, sizeof(pkt));
}

void Net_BroadcastHoloCallStart(uint32_t peerId)
{
    HoloCallPacket pkt{peerId};
    Net_Broadcast(EMsg::HoloCallStart, &pkt, sizeof(pkt));
}

void Net_BroadcastHoloCallEnd(uint32_t peerId)
{
    HoloCallPacket pkt{peerId};
    Net_Broadcast(EMsg::HoloCallEnd, &pkt, sizeof(pkt));
}

void Net_BroadcastTickRateChange(uint16_t tickMs)
{
    TickRateChangePacket pkt{tickMs, 0};
    Net_Broadcast(EMsg::TickRateChange, &pkt, sizeof(pkt));
}

void Net_BroadcastRuleChange(bool friendly)
{
    RuleChangePacket pkt{static_cast<uint8_t>(friendly), {0, 0, 0}};
    Net_Broadcast(EMsg::RuleChange, &pkt, sizeof(pkt));
}

void Net_SendSpectateRequest(uint32_t peerId)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        SpectatePacket pkt{peerId};
        Net_Send(conns[0], EMsg::SpectateRequest, &pkt, sizeof(pkt));
    }
}

void Net_SendSpectateGranted(uint32_t peerId)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        SpectatePacket pkt{peerId};
        Net_Send(conns[0], EMsg::SpectateGranted, &pkt, sizeof(pkt));
    }
}

void Net_BroadcastScoreUpdate(uint32_t peerId, uint16_t k, uint16_t d)
{
    ScoreUpdatePacket pkt{peerId, k, d};
    Net_Broadcast(EMsg::ScoreUpdate, &pkt, sizeof(pkt));
}

void Net_BroadcastMatchOver(uint32_t winnerId)
{
    MatchOverPacket pkt{winnerId};
    Net_Broadcast(EMsg::MatchOver, &pkt, sizeof(pkt));
}

void Net_SendAdminCmd(Connection* conn, uint8_t cmdType, uint64_t param)
{
    if (!conn)
        return;
    AdminCmdPacket pkt{cmdType, {0, 0, 0}, param};
    Net_Send(conn, EMsg::AdminCmd, &pkt, sizeof(pkt));
}

void Net_Disconnect(Connection* conn)
{
    if (!g_Host || !conn)
        return;
    auto it = std::find_if(g_Peers.begin(), g_Peers.end(), [&](const PeerEntry& p) { return p.conn == conn; });
    if (it != g_Peers.end())
    {
        enet_peer_disconnect(it->peer, 0);
    }
}

void Net_BroadcastNatCandidate(const char* sdp)
{
    NatCandidatePacket pkt{};
    std::strncpy(pkt.sdp, sdp, sizeof(pkt.sdp) - 1);
    Net_Broadcast(EMsg::NatCandidate, &pkt, sizeof(pkt));
}

void Net_BroadcastCineStart(uint32_t sceneId, uint32_t startTimeMs, uint32_t phaseId, bool solo)
{
    CineStartPacket pkt{sceneId, startTimeMs, phaseId, static_cast<uint8_t>(solo), {0, 0, 0}};
    Net_Broadcast(EMsg::CineStart, &pkt, sizeof(pkt));
}

void Net_BroadcastViseme(uint32_t npcId, uint8_t visemeId, uint32_t timeMs)
{
    VisemePacket pkt{npcId, visemeId, {0, 0, 0}, timeMs};
    Net_Broadcast(EMsg::Viseme, &pkt, sizeof(pkt));
}

void Net_SendDialogChoice(uint8_t choiceIdx)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        DialogChoicePacket pkt{0u, choiceIdx, {0, 0, 0}};
        Net_Send(conns[0], EMsg::DialogChoice, &pkt, sizeof(pkt));
    }
}

void Net_BroadcastDialogChoice(uint32_t peerId, uint8_t choiceIdx)
{
    DialogChoicePacket pkt{peerId, choiceIdx, {0, 0, 0}};
    Net_Broadcast(EMsg::DialogChoice, &pkt, sizeof(pkt));
}

void Net_SendVoice(const uint8_t* data, uint16_t size, uint16_t seq)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        VoicePacket pkt{0u, seq, size, {0}};
        std::memcpy(pkt.data, data, std::min<size_t>(size, sizeof(pkt.data)));
        Net_Send(conns[0], EMsg::Voice, &pkt, static_cast<uint16_t>(sizeof(pkt)));
        conns[0]->voiceBytes += sizeof(pkt);
    }
}

void Net_BroadcastVoice(uint32_t peerId, const uint8_t* data, uint16_t size, uint16_t seq)
{
    VoicePacket pkt{peerId, seq, size, {0}};
    std::memcpy(pkt.data, data, std::min<size_t>(size, sizeof(pkt.data)));
    Net_Broadcast(EMsg::Voice, &pkt, static_cast<uint16_t>(sizeof(pkt)));
    for (auto* c : Net_GetConnections())
        c->voiceBytes += sizeof(pkt);
}

void Net_BroadcastWorldState(uint16_t sunAngleDeg, uint8_t weatherId)
{
    WorldStatePacket pkt{sunAngleDeg, weatherId};
    Net_SendUnreliableToAll(EMsg::WorldState, &pkt, sizeof(pkt));
}

void Net_BroadcastGlobalEvent(uint32_t eventId, uint8_t phase, bool start, uint32_t seed)
{
    GlobalEventPacket pkt{eventId, seed, phase, static_cast<uint8_t>(start), {0, 0}};
    Net_Broadcast(EMsg::GlobalEvent, &pkt, sizeof(pkt));
}

void Net_BroadcastCrowdSeed(uint64_t sectorHash, uint32_t seed)
{
    CrowdSeedPacket pkt{sectorHash, seed};
    Net_Broadcast(EMsg::CrowdSeed, &pkt, sizeof(pkt));
}

void Net_BroadcastVendorStock(const VendorStockPacket& pkt)
{
    Net_Broadcast(EMsg::VendorStock, &pkt, sizeof(VendorStockPacket));
}

void Net_BroadcastVendorStockUpdate(const VendorStockUpdatePacket& pkt)
{
    Net_Broadcast(EMsg::VendorStockUpdate, &pkt, sizeof(VendorStockUpdatePacket));
}

void Net_SendWorldMarkers(Connection* conn, const std::vector<uint8_t>& blob)
{
    if (!conn || blob.size() > 10240)
        return;
    std::vector<uint8_t> buf(sizeof(WorldMarkersPacket) + blob.size());
    auto* pkt = reinterpret_cast<WorldMarkersPacket*>(buf.data());
    pkt->blobBytes = static_cast<uint16_t>(blob.size());
    std::memcpy(pkt->zstdBlob, blob.data(), blob.size());
    Net_Send(conn, EMsg::WorldMarkers, pkt, static_cast<uint16_t>(buf.size()));
}

void Net_BroadcastNpcSpawnCruiser(uint8_t waveIdx, const uint32_t seeds[4])
{
    NpcSpawnCruiserPacket pkt{};
    pkt.waveIdx = waveIdx;
    std::memcpy(pkt.npcSeeds, seeds, sizeof(pkt.npcSeeds));
    Net_Broadcast(EMsg::NpcSpawnCruiser, &pkt, sizeof(pkt));
}

void Net_BroadcastNpcState(uint32_t npcId, uint8_t aiState)
{
    NpcStatePacket pkt{npcId, aiState, {0, 0, 0}};
    Net_Broadcast(EMsg::NpcState, &pkt, sizeof(pkt));
}

void Net_BroadcastCrimeEvent(const CrimeEventSpawnPacket& pkt)
{
    Net_Broadcast(EMsg::CrimeEventSpawn, &pkt, sizeof(CrimeEventSpawnPacket));
}

void Net_BroadcastCyberEquip(uint32_t peerId, uint8_t slotId, const ItemSnap& snap)
{
    CyberEquipPacket pkt{};
    pkt.peerId = peerId;
    pkt.slotId = slotId;
    pkt.snap = snap;
    Net_Broadcast(EMsg::CyberEquip, &pkt, sizeof(pkt));
}

void Net_BroadcastSlowMoStart(uint32_t peerId, float factor, uint16_t durationMs)
{
    SlowMoStartPacket pkt{peerId, factor, durationMs, 0};
    Net_Broadcast(EMsg::SlowMoStart, &pkt, sizeof(pkt));
}

void Net_SendPerkUnlock(uint32_t perkId, uint8_t rank)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        PerkUnlockPacket pkt{0u, perkId, rank, {0, 0, 0}};
        Net_Send(conns[0], EMsg::PerkUnlock, &pkt, sizeof(pkt));
    }
}

void Net_BroadcastPerkUnlock(uint32_t peerId, uint32_t perkId, uint8_t rank)
{
    PerkUnlockPacket pkt{peerId, perkId, rank, {0, 0, 0}};
    Net_Broadcast(EMsg::PerkUnlock, &pkt, sizeof(pkt));
}

void Net_SendPerkRespecRequest()
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        PerkRespecRequestPacket pkt{0u};
        Net_Send(conns[0], EMsg::PerkRespecRequest, &pkt, sizeof(pkt));
    }
}

void Net_SendPerkRespecAck(CoopNet::Connection* conn, uint16_t newPoints)
{
    PerkRespecAckPacket pkt{conn->peerId, newPoints, {0, 0}};
    Net_Send(conn, EMsg::PerkRespecAck, &pkt, sizeof(pkt));
}

void Net_BroadcastStatusApply(uint32_t targetId, uint8_t effectId, uint16_t durMs, uint8_t amp)
{
    StatusApplyPacket pkt{targetId, effectId, durMs, amp};
    Net_Broadcast(EMsg::StatusApply, &pkt, sizeof(pkt));
}

void Net_BroadcastStatusTick(uint32_t targetId, int16_t hpDelta)
{
    StatusTickPacket pkt{targetId, hpDelta};
    Net_Broadcast(EMsg::StatusTick, &pkt, sizeof(pkt));
}

void Net_BroadcastTrafficSeed(uint64_t sectorHash, uint64_t seed)
{
    TrafficSeedPacket pkt{sectorHash, seed};
    Net_Broadcast(EMsg::TrafficSeed, &pkt, sizeof(pkt));
}

void Net_BroadcastTrafficDespawn(uint32_t vehId)
{
    TrafficDespawnPacket pkt{vehId};
    Net_Broadcast(EMsg::TrafficDespawn, &pkt, sizeof(pkt));
}

void Net_BroadcastPropBreak(uint32_t entityId, uint32_t seed)
{
    PropBreakPacket pkt{entityId, seed};
    Net_Broadcast(EMsg::PropBreak, &pkt, sizeof(pkt));
}

void Net_BroadcastPropIgnite(uint32_t entityId, uint16_t delayMs)
{
    PropIgnitePacket pkt{entityId, delayMs, 0};
    Net_Broadcast(EMsg::PropIgnite, &pkt, sizeof(pkt));
}

void Net_BroadcastVOPlay(uint32_t lineId)
{
    VOPlayPacket pkt{lineId};
    Net_Broadcast(EMsg::VOPlay, &pkt, sizeof(pkt));
}

void Net_BroadcastFixerCallStart(uint32_t fixerId)
{
    FixerCallPacket pkt{fixerId};
    Net_Broadcast(EMsg::FixerCallStart, &pkt, sizeof(pkt));
}

void Net_BroadcastFixerCallEnd(uint32_t fixerId)
{
    FixerCallPacket pkt{fixerId};
    Net_Broadcast(EMsg::FixerCallEnd, &pkt, sizeof(pkt));
}

void Net_BroadcastGigSpawn(uint32_t questId, uint32_t seed)
{
    GigSpawnPacket pkt{questId, seed};
    Net_Broadcast(EMsg::GigSpawn, &pkt, sizeof(pkt));
}

void Net_BroadcastVehicleSummon(uint32_t vehId, uint32_t ownerId, const TransformSnap& pos)
{
    VehicleSummonPacket pkt{vehId, ownerId, pos};
    Net_Broadcast(EMsg::VehicleSummon, &pkt, sizeof(pkt));
}

void Net_BroadcastAppearance(uint32_t peerId, uint32_t meshId, uint32_t tintId)
{
    AppearancePacket pkt{peerId, meshId, tintId};
    Net_Broadcast(EMsg::Appearance, &pkt, sizeof(pkt));
}

void Net_BroadcastPingOutline(uint32_t peerId, uint16_t durationMs, const std::vector<uint32_t>& ids)
{
    if (ids.empty() || ids.size() > 32)
        return;
    PingOutlinePacket pkt{};
    pkt.peerId = peerId;
    pkt.count = static_cast<uint8_t>(ids.size());
    pkt._pad = 0;
    pkt.durationMs = durationMs;
    for (size_t i = 0; i < ids.size(); ++i)
        pkt.entityIds[i] = ids[i];
    Net_Broadcast(EMsg::PingOutline, &pkt, sizeof(uint32_t) * pkt.count + 8);
}

void Net_BroadcastLootRoll(uint32_t containerId, uint32_t seed)
{
    LootRollPacket pkt{containerId, seed};
    Net_Broadcast(EMsg::LootRoll, &pkt, sizeof(pkt));
}

void Net_SendDealerBuy(uint32_t vehicleTpl, uint32_t price)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        DealerBuyPacket pkt{vehicleTpl, price};
        Net_Send(conns[0], EMsg::DealerBuy, &pkt, sizeof(pkt));
    }
}

void Net_BroadcastVehicleUnlock(uint32_t peerId, uint32_t vehicleTpl)
{
    VehicleUnlockPacket pkt{peerId, vehicleTpl};
    Net_Broadcast(EMsg::VehicleUnlock, &pkt, sizeof(pkt));
}

void Net_BroadcastWeaponInspect(uint32_t peerId, uint16_t animId)
{
    WeaponInspectPacket pkt{peerId, animId, 0};
    Net_Broadcast(EMsg::WeaponInspectStart, &pkt, sizeof(pkt));
}

void Net_BroadcastFinisherStart(uint32_t actorId, uint32_t victimId, uint16_t animId)
{
    FinisherStartPacket pkt{actorId, victimId, animId, 0};
    Net_Broadcast(EMsg::FinisherStart, &pkt, sizeof(pkt));
}

void Net_BroadcastFinisherEnd(uint32_t actorId)
{
    FinisherEndPacket pkt{actorId};
    Net_Broadcast(EMsg::FinisherEnd, &pkt, sizeof(pkt));
}

void Net_BroadcastTextureBiasChange(uint8_t bias)
{
    TextureBiasPacket pkt{bias, {0, 0, 0}};
    Net_Broadcast(EMsg::TextureBiasChange, &pkt, sizeof(pkt));
}

void Net_BroadcastCriticalVoteStart(uint32_t questHash)
{
    CriticalVoteStartPacket pkt{questHash};
    Net_Broadcast(EMsg::CriticalVoteStart, &pkt, sizeof(pkt));
}

void Net_SendCriticalVoteCast(bool yes)
{
    CriticalVoteCastPacket pkt{0u, static_cast<uint8_t>(yes), {0, 0, 0}};
    auto conns = Net_GetConnections();
    if (!conns.empty())
        Net_Send(conns[0], EMsg::CriticalVoteCast, &pkt, sizeof(pkt));
}

void Net_SendPhaseBundle(Connection* conn, uint32_t phaseId, const std::vector<uint8_t>& blob)
{
    if (!conn || blob.empty() || blob.size() > 16384)
        return;
    std::vector<uint8_t> buf(sizeof(PhaseBundlePacket) + blob.size());
    auto* pkt = reinterpret_cast<PhaseBundlePacket*>(buf.data());
    pkt->phaseId = phaseId;
    pkt->blobBytes = static_cast<uint16_t>(blob.size());
    std::memcpy(pkt->zstdBlob, blob.data(), blob.size());
    Net_Send(conn, EMsg::PhaseBundle, pkt, static_cast<uint16_t>(buf.size()));
}
