#include "Net.hpp"
#include "../core/GameClock.hpp"
#include "../core/Hash.hpp"
#include "../runtime/QuestSync.reds"
#include "../server/AdminController.hpp"
#include "../server/Journal.hpp"
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
#include <sodium.h>
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
                Net_SendCrowdCfg(e.conn, 2u);
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
                    const uint8_t* payload = evt.packet->data + sizeof(PacketHeader);
                    uint16_t psize = evt.packet->dataLength - sizeof(PacketHeader);
                    if (it->conn->hasKey && pkt.hdr.type != static_cast<uint16_t>(EMsg::Hello) &&
                        pkt.hdr.type != static_cast<uint16_t>(EMsg::Welcome))
                    {
                        if (psize < 4 + crypto_secretbox_MACBYTES)
                            break;
                        uint32_t nonce;
                        memcpy(&nonce, payload, 4);
                        if (it->conn->nonceSet.count(nonce))
                            break;
                        it->conn->nonceWindow.push_back(nonce);
                        it->conn->nonceSet.insert(nonce);
                        if (it->conn->nonceWindow.size() > 1024)
                        {
                            uint32_t old = it->conn->nonceWindow.front();
                            it->conn->nonceWindow.pop_front();
                            it->conn->nonceSet.erase(old);
                        }
                        unsigned char nbuf[crypto_secretbox_NONCEBYTES] = {0};
                        memcpy(nbuf, &nonce, 4);
                        std::vector<uint8_t> plain(psize - 4 - crypto_secretbox_MACBYTES);
                        if (crypto_secretbox_open_easy(plain.data(), payload + 4, psize - 4, nbuf,
                                                       it->conn->key.data()) != 0)
                            break;
                        pkt.data = std::move(plain);
                    }
                    else
                    {
                        pkt.data.resize(psize);
                        memcpy(pkt.data.data(), payload, psize);
                    }
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

bool Net_IsConnected()
{
    return !g_Peers.empty();
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

    std::vector<uint8_t> outBuf;
    uint16_t finalSize = size;
    if (conn->hasKey && type != EMsg::Hello && type != EMsg::Welcome)
    {
        uint32_t nonce = static_cast<uint32_t>(conn->lastNonce++);
        unsigned char nbuf[crypto_secretbox_NONCEBYTES] = {0};
        memcpy(nbuf, &nonce, 4);
        outBuf.resize(4 + size + crypto_secretbox_MACBYTES);
        memcpy(outBuf.data(), &nonce, 4);
        crypto_secretbox_easy(outBuf.data() + 4, static_cast<const unsigned char*>(data), size, nbuf, conn->key.data());
        finalSize = static_cast<uint16_t>(outBuf.size());
        data = outBuf.data();
    }
    ENetPacket* pkt = enet_packet_create(nullptr, sizeof(PacketHeader) + finalSize, ENET_PACKET_FLAG_RELIABLE);
    PacketHeader hdr{static_cast<uint16_t>(type), finalSize};
    std::memcpy(pkt->data, &hdr, sizeof(hdr));
    if (finalSize > 0 && data)
        std::memcpy(pkt->data + sizeof(hdr), data, finalSize);
    enet_peer_send(it->peer, 0, pkt);
}

void Net_Broadcast(EMsg type, const void* data, uint16_t size)
{
    if (!g_Host)
        return;
    for (auto& e : g_Peers)
    {
        Net_Send(e.conn, type, data, size);
    }
}

void Net_SendUnreliableToAll(EMsg type, const void* data, uint16_t size)
{
    if (!g_Host)
        return;

    for (auto& e : g_Peers)
    {
        ENetPacket* pkt = enet_packet_create(nullptr, sizeof(PacketHeader) + size, 0);
        PacketHeader hdr{static_cast<uint16_t>(type), size};
        std::memcpy(pkt->data, &hdr, sizeof(hdr));
        if (size > 0 && data)
            std::memcpy(pkt->data + sizeof(hdr), data, size);
        enet_peer_send(e.peer, 0, pkt);
    }
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

void Net_BroadcastVehicleSpawn(uint32_t vehicleId, uint32_t archetypeId, uint32_t paintId, uint32_t phaseId,
                               const TransformSnap& t)
{
    VehicleSpawnPacket pkt{vehicleId, archetypeId, paintId, phaseId, t};
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

void Net_SendJoinRequest(uint32_t serverId)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        uint32_t id = serverId;
        Net_Send(conns[0], EMsg::JoinRequest, &id, sizeof(id));
    }
}

void Net_BroadcastQuestStage(uint32_t nameHash, uint16_t stage)
{
    QuestStagePacket pkt{nameHash, stage, 0};
    for (auto& e : g_Peers)
        CoopNet::QuestWatchdog_Record(e.conn->peerId, nameHash, stage);
    Journal_Log(GameClock::GetCurrentTick(), 0, "questStage", nameHash, stage);
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

void Net_BroadcastHoloCallStart(uint32_t fixerId, uint32_t callId, const uint32_t* peerIds, uint8_t count)
{
    HolocallStartPacket pkt{};
    pkt.fixerId = fixerId;
    pkt.callId = callId;
    pkt.count = count;
    for (uint8_t i = 0; i < count && i < 4; ++i)
        pkt.peerIds[i] = peerIds[i];
    Net_Broadcast(EMsg::HoloCallStart, &pkt, sizeof(pkt));
}

void Net_BroadcastHoloCallEnd(uint32_t callId)
{
    HolocallEndPacket pkt{callId};
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

void Net_BroadcastChat(const std::string& msg)
{
    ChatPacket pkt{0, {0}};
    std::strncpy(pkt.msg, msg.c_str(), sizeof(pkt.msg) - 1);
    Net_Broadcast(EMsg::Chat, &pkt, sizeof(pkt));
}

void Net_BroadcastKillfeed(const std::string& msg)
{
    KillfeedPacket pkt{{0}};
    std::strncpy(pkt.msg, msg.c_str(), sizeof(pkt.msg) - 1);
    Net_Broadcast(EMsg::Killfeed, &pkt, sizeof(pkt));
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
        if (conns[0]->voiceMuted)
            return;
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

void Net_BroadcastWorldState(uint16_t sunAngleDeg, uint8_t weatherId, uint16_t particleSeed)
{
    WorldStatePacket pkt{sunAngleDeg, weatherId, particleSeed};
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

void Net_BroadcastVendorRefresh(const VendorRefreshPacket& pkt)
{
    Net_Broadcast(EMsg::VendorRefresh, &pkt, sizeof(VendorRefreshPacket));
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

void Net_SendSkillXP(uint16_t skillId, int16_t deltaXP)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        SkillXPPacket pkt{0u, skillId, deltaXP};
        Net_Send(conns[0], EMsg::SkillXP, &pkt, sizeof(pkt));
    }
}

void Net_BroadcastSkillXP(uint32_t peerId, uint16_t skillId, int16_t deltaXP)
{
    SkillXPPacket pkt{peerId, skillId, deltaXP};
    Net_Broadcast(EMsg::SkillXP, &pkt, sizeof(pkt));
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

void Net_BroadcastAptPermChange(uint32_t aptId, uint32_t targetPeerId, bool allow)
{
    AptPermChangePacket pkt{aptId, targetPeerId, static_cast<uint8_t>(allow), {0, 0, 0}};
    Net_Broadcast(EMsg::AptPermChange, &pkt, sizeof(pkt));
}

void Net_SendVehicleTowRequest(const RED4ext::Vector3& pos)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        VehicleTowRequestPacket pkt{pos};
        Net_Send(conns[0], EMsg::VehicleTowRequest, &pkt, sizeof(pkt));
    }
}

void Net_SendVehicleTowAck(Connection* conn, uint32_t ownerId, bool ok)
{
    if (!conn)
        return;
    VehicleTowAckPacket pkt{ownerId, static_cast<uint8_t>(ok), {0, 0, 0}};
    Net_Send(conn, EMsg::VehicleTowAck, &pkt, sizeof(pkt));
}

void Net_SendReRollRequest(uint64_t itemId, uint32_t seed)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        ReRollRequestPacket pkt{itemId, seed};
        Net_Send(conns[0], EMsg::ReRollRequest, &pkt, sizeof(pkt));
    }
}

void Net_SendReRollResult(Connection* conn, const ItemSnap& snap)
{
    if (!conn)
        return;
    ReRollResultPacket pkt{snap};
    Net_Send(conn, EMsg::ReRollResult, &pkt, sizeof(pkt));
}

void Net_SendRipperInstallRequest(uint8_t slotId)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        RipperInstallRequestPacket pkt{slotId, {0, 0, 0}};
        Net_Send(conns[0], EMsg::RipperInstallRequest, &pkt, sizeof(pkt));
    }
}

void Net_SendTileSelect(uint8_t row, uint8_t col)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        TileSelectPacket pkt{Net_GetPeerId(), QuestSync::localPhase, row, col, {0, 0}};
        Net_Send(conns[0], EMsg::TileSelect, &pkt, sizeof(pkt));
    }
}

void Net_BroadcastTileGameStart(uint32_t phaseId, uint32_t seed)
{
    TileGameStartPacket pkt{phaseId, seed};
    Net_Broadcast(EMsg::TileGameStart, &pkt, sizeof(pkt));
}

void Net_BroadcastTileSelect(uint32_t peerId, uint32_t phaseId, uint8_t row, uint8_t col)
{
    TileSelectPacket pkt{peerId, phaseId, row, col, {0, 0}};
    Net_Broadcast(EMsg::TileSelect, &pkt, sizeof(pkt));
}

void Net_BroadcastShardProgress(uint32_t phaseId, uint8_t percent)
{
    ShardProgressPacket pkt{phaseId, percent, {0, 0, 0}};
    Net_Broadcast(EMsg::ShardProgress, &pkt, sizeof(pkt));
}

void Net_SendTradeInit(uint32_t targetPeerId)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        TradeInitPacket pkt{Net_GetPeerId(), targetPeerId};
        Net_Send(conns[0], EMsg::TradeInit, &pkt, sizeof(pkt));
    }
}

void Net_SendTradeOffer(const ItemSnap* items, uint8_t count, uint32_t eddies)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        TradeOfferPacket pkt{};
        pkt.fromId = Net_GetPeerId();
        pkt.toId = 0u;
        pkt.count = count;
        pkt.eddies = eddies;
        if (count > 0 && items)
            std::memcpy(pkt.items, items, sizeof(ItemSnap) * count);
        Net_Send(conns[0], EMsg::TradeOffer, &pkt, sizeof(pkt));
    }
}

void Net_SendTradeAccept(bool accept)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        TradeAcceptPacket pkt{Net_GetPeerId(), static_cast<uint8_t>(accept), {0, 0, 0}};
        Net_Send(conns[0], EMsg::TradeAccept, &pkt, sizeof(pkt));
    }
}

void Net_BroadcastTradeFinalize(bool success)
{
    TradeFinalizePacket pkt{static_cast<uint8_t>(success), {0, 0, 0}};
    Net_Broadcast(EMsg::TradeFinalize, &pkt, sizeof(pkt));
}

void Net_BroadcastEndingVoteStart(uint32_t questHash)
{
    EndingVoteStartPacket pkt{questHash};
    Net_Broadcast(EMsg::EndingVoteStart, &pkt, sizeof(pkt));
}

void Net_SendEndingVoteCast(bool yes)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        EndingVoteCastPacket pkt{Net_GetPeerId(), static_cast<uint8_t>(yes), {0, 0, 0}};
        Net_Send(conns[0], EMsg::EndingVoteCast, &pkt, sizeof(pkt));
    }
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

void Net_BroadcastVehicleHitHighSpeed(uint32_t vehA, uint32_t vehB, const RED4ext::Vector3& delta)
{
    VehicleHitHighSpeedPacket pkt{vehA, vehB, delta};
    Net_Broadcast(EMsg::VehicleHitHighSpeed, &pkt, sizeof(pkt));
}

void Net_BroadcastVehicleSnap(const VehicleSnap& snap)
{
    VehicleSnapshotPacket pkt{snap};
    Net_Broadcast(EMsg::VehicleSnapshot, &pkt, sizeof(pkt));
}

void Net_BroadcastTurretAim(uint32_t vehId, float yaw, float pitch)
{
    TurretAimPacket pkt{vehId, yaw, pitch};
    Net_Broadcast(EMsg::TurretAim, &pkt, sizeof(pkt));
}

void Net_BroadcastAirVehSpawn(uint32_t vehId, const RED4ext::Vector3* pts, uint8_t count)
{
    AirVehSpawnPacket pkt{};
    pkt.vehId = vehId;
    pkt.count = count > 8 ? 8 : count;
    for (uint8_t i = 0; i < pkt.count; ++i)
        pkt.points[i] = pts[i];
    Net_Broadcast(EMsg::AirVehSpawn, &pkt, sizeof(pkt));
}

void Net_BroadcastAirVehUpdate(uint32_t vehId, const TransformSnap& t)
{
    AirVehUpdatePacket pkt{vehId, t};
    Net_Broadcast(EMsg::AirVehUpdate, &pkt, sizeof(pkt));
}

void Net_BroadcastVehiclePaintChange(uint32_t vehId, uint32_t colorId, const char* plate)
{
    VehiclePaintChangePacket pkt{};
    pkt.vehId = vehId;
    pkt.colorId = colorId;
    std::memset(pkt.plateId, 0, sizeof(pkt.plateId));
    if (plate)
        std::memcpy(pkt.plateId, plate, std::min<size_t>(7, std::strlen(plate)));
    Net_Broadcast(EMsg::VehiclePaintChange, &pkt, sizeof(pkt));
}

void Net_BroadcastPanicEvent(const RED4ext::Vector3& pos, uint32_t seed)
{
    PanicEventPacket pkt{pos, seed};
    Net_Broadcast(EMsg::PanicEvent, &pkt, sizeof(pkt));
}

void Net_BroadcastAIHack(uint32_t targetId, uint8_t effectId)
{
    AIHackPacket pkt{targetId, effectId, {0, 0, 0}};
    Net_Broadcast(EMsg::AIHack, &pkt, sizeof(pkt));
}

void Net_BroadcastBossPhase(uint32_t npcId, uint8_t phaseIdx)
{
    BossPhasePacket pkt{npcId, phaseIdx, {0, 0, 0}};
    Net_Broadcast(EMsg::BossPhase, &pkt, sizeof(pkt));
}

void Net_BroadcastWeaponInspect(uint32_t peerId, uint16_t animId)
{
    WeaponInspectPacket pkt{peerId, animId, 0};
    Net_Broadcast(EMsg::WeaponInspectStart, &pkt, sizeof(pkt));
}

void Net_BroadcastFinisherStart(uint32_t actorId, uint32_t victimId, uint8_t finisherType)
{
    FinisherStartPacket pkt{actorId, victimId, finisherType, {0, 0, 0}};
    Net_Broadcast(EMsg::FinisherStart, &pkt, sizeof(pkt));
}

void Net_BroadcastFinisherEnd(uint32_t actorId)
{
    FinisherEndPacket pkt{actorId};
    Net_Broadcast(EMsg::FinisherEnd, &pkt, sizeof(pkt));
}

void Net_BroadcastSlowMoFinisher(uint32_t peerId, uint32_t targetId, uint16_t durMs)
{
    SlowMoFinisherPacket pkt{peerId, targetId, durMs, 0};
    Net_Broadcast(EMsg::SlowMoFinisher, &pkt, sizeof(pkt));
}

void Net_BroadcastTextureBiasChange(uint8_t bias)
{
    TextureBiasPacket pkt{bias, {0, 0, 0}};
    Net_Broadcast(EMsg::TextureBiasChange, &pkt, sizeof(pkt));
}

void Net_BroadcastSectorLOD(uint64_t sectorHash, uint8_t lod)
{
    SectorLODPacket pkt{sectorHash, lod, {0, 0, 0}};
    Net_Broadcast(EMsg::SectorLOD, &pkt, sizeof(pkt));
}

void Net_SendLowBWMode(CoopNet::Connection* conn, bool enable)
{
    if (!conn)
        return;
    LowBWModePacket pkt{static_cast<uint8_t>(enable), {0, 0, 0}};
    Net_Send(conn, EMsg::LowBWMode, &pkt, sizeof(pkt));
}

void Net_SendCrowdCfg(CoopNet::Connection* conn, uint8_t density)
{
    if (!conn)
        return;
    CrowdCfgPacket pkt{density, {0, 0, 0}};
    Net_Send(conn, EMsg::CrowdCfg, &pkt, sizeof(pkt));
}

void Net_BroadcastEmote(uint32_t peerId, uint8_t emoteId)
{
    EmotePacket pkt{peerId, emoteId, {0, 0, 0}};
    Net_Broadcast(EMsg::Emote, &pkt, sizeof(pkt));
}

void Net_BroadcastCrowdChatterStart(uint32_t npcA, uint32_t npcB, uint32_t lineId, uint32_t seed)
{
    CrowdChatterStartPacket pkt{npcA, npcB, lineId, seed};
    Net_Broadcast(EMsg::CrowdChatterStart, &pkt, sizeof(pkt));
}

void Net_BroadcastCrowdChatterEnd(uint32_t convId)
{
    CrowdChatterEndPacket pkt{convId};
    Net_Broadcast(EMsg::CrowdChatterEnd, &pkt, sizeof(pkt));
}

void Net_BroadcastHoloSeed(uint64_t sectorHash, uint64_t seed64)
{
    HoloSeedPacket pkt{sectorHash, seed64};
    Net_Broadcast(EMsg::HoloSeed, &pkt, sizeof(pkt));
}

void Net_BroadcastHoloNextAd(uint64_t sectorHash, uint32_t adId)
{
    HoloNextAdPacket pkt{sectorHash, adId};
    Net_Broadcast(EMsg::HoloNextAd, &pkt, sizeof(pkt));
}

void Net_BroadcastDoorBreachStart(uint32_t doorId, uint32_t phaseId, uint32_t seed)
{
    DoorBreachStartPacket pkt{doorId, phaseId, seed};
    Net_Broadcast(EMsg::DoorBreachStart, &pkt, sizeof(pkt));
}

void Net_BroadcastDoorBreachTick(uint32_t doorId, uint8_t percent)
{
    DoorBreachTickPacket pkt{doorId, percent, {0, 0, 0}};
    Net_Broadcast(EMsg::DoorBreachTick, &pkt, sizeof(pkt));
}

void Net_BroadcastDoorBreachSuccess(uint32_t doorId)
{
    DoorBreachSuccessPacket pkt{doorId};
    Net_Broadcast(EMsg::DoorBreachSuccess, &pkt, sizeof(pkt));
}

void Net_BroadcastDoorBreachAbort(uint32_t doorId)
{
    DoorBreachAbortPacket pkt{doorId};
    Net_Broadcast(EMsg::DoorBreachAbort, &pkt, sizeof(pkt));
}

void Net_BroadcastHTableOpen(uint32_t sceneId)
{
    HTableOpenPacket pkt{sceneId};
    Net_Broadcast(EMsg::HTableOpen, &pkt, sizeof(pkt));
}

void Net_BroadcastHTableScrub(uint32_t timestampMs)
{
    HTableScrubPacket pkt{timestampMs};
    Net_Broadcast(EMsg::HTableScrub, &pkt, sizeof(pkt));
}

void Net_BroadcastQuestGadgetFire(uint32_t questId, QuestGadgetType type, uint8_t charge, uint32_t targetId)
{
    QuestGadgetFirePacket pkt{questId, static_cast<uint8_t>(type), charge, targetId, 0};
    Net_Broadcast(EMsg::QuestGadgetFire, &pkt, sizeof(pkt));
}

void Net_BroadcastItemGrab(uint32_t peerId, uint32_t itemId)
{
    ItemGrabPacket pkt{peerId, itemId};
    Net_Broadcast(EMsg::ItemGrab, &pkt, sizeof(pkt));
}

void Net_BroadcastItemDrop(uint32_t peerId, uint32_t itemId, const RED4ext::Vector3& pos)
{
    ItemDropPacket pkt{peerId, itemId, pos};
    Net_Broadcast(EMsg::ItemDrop, &pkt, sizeof(pkt));
}

void Net_BroadcastItemStore(uint32_t peerId, uint32_t itemId)
{
    ItemStorePacket pkt{peerId, itemId};
    Net_Broadcast(EMsg::ItemStore, &pkt, sizeof(pkt));
}

void Net_BroadcastMetroBoard(uint32_t peerId, uint32_t lineId, uint8_t carIdx)
{
    MetroBoardPacket pkt{peerId, lineId, carIdx, {0, 0, 0}};
    Net_Broadcast(EMsg::MetroBoard, &pkt, sizeof(pkt));
}

void Net_BroadcastMetroArrive(uint32_t peerId, uint32_t stationId)
{
    MetroArrivePacket pkt{peerId, stationId};
    Net_Broadcast(EMsg::MetroArrive, &pkt, sizeof(pkt));
}

void Net_BroadcastRadioChange(uint32_t vehId, uint8_t stationId, uint32_t offsetSec)
{
    RadioChangePacket pkt{vehId, stationId, 0, offsetSec};
    Net_Broadcast(EMsg::RadioChange, &pkt, sizeof(pkt));
}

void Net_BroadcastCamHijack(uint32_t camId, uint32_t peerId)
{
    CamHijackPacket pkt{camId, peerId};
    Net_Broadcast(EMsg::CamHijack, &pkt, sizeof(pkt));
}

void Net_BroadcastCamFrameStart(uint32_t camId)
{
    CamFrameStartPacket pkt{camId};
    Net_Broadcast(EMsg::CamFrameStart, &pkt, sizeof(pkt));
}

void Net_BroadcastCarryBegin(uint32_t carrierId, uint32_t entityId)
{
    CarryBeginPacket pkt{carrierId, entityId};
    Net_Broadcast(EMsg::CarryBegin, &pkt, sizeof(pkt));
}

void Net_BroadcastCarrySnap(uint32_t entityId, const RED4ext::Vector3& pos, const RED4ext::Vector3& vel)
{
    CarrySnapPacket pkt{entityId, pos, vel};
    Net_Broadcast(EMsg::CarrySnap, &pkt, sizeof(pkt));
}

void Net_BroadcastCarryEnd(uint32_t entityId, const RED4ext::Vector3& pos, const RED4ext::Vector3& vel)
{
    CarryEndPacket pkt{entityId, pos, vel};
    Net_Broadcast(EMsg::CarryEnd, &pkt, sizeof(pkt));
}

void Net_BroadcastGrenadePrime(uint32_t entityId, uint32_t startTick)
{
    GrenadePrimePacket pkt{entityId, startTick};
    Net_Broadcast(EMsg::GrenadePrime, &pkt, sizeof(pkt));
}

void Net_BroadcastGrenadeSnap(uint32_t entityId, const RED4ext::Vector3& pos, const RED4ext::Vector3& vel)
{
    GrenadeSnapPacket pkt{entityId, pos, vel};
    Net_Broadcast(EMsg::GrenadeSnap, &pkt, sizeof(pkt));
}

void Net_BroadcastSmartCamStart(uint32_t projId)
{
    SmartCamStartPacket pkt{projId};
    for (auto& e : g_Peers)
    {
        if (!e.conn->lowBWMode)
            Net_Send(e.conn, EMsg::SmartCamStart, &pkt, sizeof(pkt));
    }
}

void Net_BroadcastSmartCamEnd(uint32_t projId)
{
    SmartCamEndPacket pkt{projId};
    for (auto& e : g_Peers)
    {
        if (!e.conn->lowBWMode)
            Net_Send(e.conn, EMsg::SmartCamEnd, &pkt, sizeof(pkt));
    }
}

void Net_BroadcastArcadeStart(uint32_t cabId, uint32_t peerId, uint32_t seed)
{
    ArcadeStartPacket pkt{cabId, peerId, seed};
    Net_Broadcast(EMsg::ArcadeStart, &pkt, sizeof(pkt));
}

void Net_SendArcadeInput(uint32_t frame, uint8_t buttonMask)
{
    ArcadeInputPacket pkt{frame, buttonMask, {0, 0, 0}};
    if (g_Peers.size() > 0)
        Net_Send(g_Peers[0].conn, EMsg::ArcadeInput, &pkt, sizeof(pkt));
}

void Net_BroadcastArcadeScore(uint32_t peerId, uint32_t score)
{
    ArcadeScorePacket pkt{peerId, score};
    Net_Broadcast(EMsg::ArcadeScore, &pkt, sizeof(pkt));
}

void Net_SendPluginRPC(Connection* conn, uint16_t pluginId, uint32_t fnHash,
                       const char* json, uint16_t len)
{
    std::vector<uint8_t> buf(sizeof(PluginRPCPacket) - 1 + len);
    auto* pkt = reinterpret_cast<PluginRPCPacket*>(buf.data());
    pkt->pluginId = pluginId;
    pkt->fnHash = fnHash;
    pkt->jsonBytes = len;
    memcpy(pkt->json, json, len);
    Net_Send(conn, EMsg::PluginRPC, buf.data(), static_cast<uint16_t>(buf.size()));
}

void Net_BroadcastPluginRPC(uint16_t pluginId, uint32_t fnHash, const char* json,
                            uint16_t len)
{
    std::vector<uint8_t> buf(sizeof(PluginRPCPacket) - 1 + len);
    auto* pkt = reinterpret_cast<PluginRPCPacket*>(buf.data());
    pkt->pluginId = pluginId;
    pkt->fnHash = fnHash;
    pkt->jsonBytes = len;
    memcpy(pkt->json, json, len);
    Net_Broadcast(EMsg::PluginRPC, buf.data(), static_cast<uint16_t>(buf.size()));
}

void Net_BroadcastAssetBundle(uint16_t pluginId, const std::vector<uint8_t>& data)
{
    const uint32_t total = static_cast<uint32_t>(data.size());
    uint16_t chunk = 0;
    size_t offset = 0;
    while (offset < data.size())
    {
        uint16_t len = static_cast<uint16_t>(std::min<size_t>(32 * 1024, data.size() - offset));
        std::vector<uint8_t> buf(sizeof(AssetBundlePacket) - 1 + len);
        auto* pkt = reinterpret_cast<AssetBundlePacket*>(buf.data());
        pkt->pluginId = pluginId;
        pkt->totalBytes = total;
        pkt->chunkId = chunk++;
        pkt->dataBytes = len;
        memcpy(pkt->data, data.data() + offset, len);
        Net_Broadcast(EMsg::AssetBundle, buf.data(), static_cast<uint16_t>(buf.size()));
        offset += len;
    }
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

void Net_SendAptPurchase(uint32_t aptId)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        AptPurchasePacket pkt{aptId};
        Net_Send(conns[0], EMsg::AptPurchase, &pkt, sizeof(pkt));
    }
}

void Net_SendAptEnterReq(uint32_t aptId, uint32_t ownerPhaseId)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        AptEnterReqPacket pkt{aptId, ownerPhaseId};
        Net_Send(conns[0], EMsg::AptEnterReq, &pkt, sizeof(pkt));
    }
}

void Net_SendAptPermChange(uint32_t aptId, uint32_t targetPeerId, bool allow)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        AptPermChangePacket pkt{aptId, targetPeerId, static_cast<uint8_t>(allow), {0, 0, 0}};
        Net_Send(conns[0], EMsg::AptPermChange, &pkt, sizeof(pkt));
    }
}

void Net_SendAptPurchaseAck(Connection* conn, uint32_t aptId, bool success, uint64_t balance)
{
    if (!conn)
        return;
    AptPurchaseAckPacket pkt{aptId, balance, static_cast<uint8_t>(success), {0, 0, 0}};
    Net_Send(conn, EMsg::AptPurchaseAck, &pkt, sizeof(pkt));
}

void Net_SendAptEnterAck(Connection* conn, bool allow, uint32_t phaseId, uint32_t interiorSeed)
{
    if (!conn)
        return;
    AptEnterAckPacket pkt{static_cast<uint8_t>(allow), {0, 0, 0}, phaseId, interiorSeed};
    Net_Send(conn, EMsg::AptEnterAck, &pkt, sizeof(pkt));
}
