#include "Net.hpp"
#include "Packets.hpp"
#include "../core/GameClock.hpp"
#include "../core/Hash.hpp"
#include "../core/SessionState.hpp"
#include <memory>
// #include "../runtime/QuestSync.reds" // REMOVED: Cannot include .reds in C++
#include "../server/AdminController.hpp"
#include "../server/Journal.hpp"
#include "../server/PoliceDispatch.hpp"
#include "../server/QuestWatchdog.hpp"
#include "Connection.hpp"
#include "NatClient.hpp"
#include "../voice/VoiceEncoder.hpp"
#include "NetConfig.hpp"
#include "Packets.hpp"
#include "../core/AssetStreamer.hpp"
#include <algorithm>
#include <cstring>
#include <enet/enet.h>
#include <iostream>
#include <new>
#include <sodium.h>
#include <vector>
#include <unordered_set>
#include <mutex>

using CoopNet::Connection;
using CoopNet::EMsg;
using CoopNet::PacketHeader;
using CoopNet::TransformSnap;
using CoopNet::ItemSnap;
using CoopNet::NpcSnap;
using CoopNet::VehicleSnap;

// Packet type declarations
using CoopNet::CraftRequestPacket;
using CoopNet::AttachModRequestPacket;
using CoopNet::PurchaseRequestPacket;
using CoopNet::VehicleSummonRequestPacket;
using CoopNet::BreachInputPacket;
using CoopNet::ElevatorCallPacket;
using CoopNet::VehicleExplodePacket;
using CoopNet::VehiclePartDetachPacket;
using CoopNet::EjectOccupantPacket;
using CoopNet::VehicleSpawnPacket;
using CoopNet::SeatRequestPacket;
using CoopNet::SeatAssignPacket;
using CoopNet::VehicleHitPacket;
using CoopNet::BreachStartPacket;
using CoopNet::BreachResultPacket;
using CoopNet::HeatPacket;
using CoopNet::ElevatorArrivePacket;
using CoopNet::TeleportAckPacket;
using CoopNet::QuestStagePacket;
using CoopNet::QuestStageP2PPacket;
using CoopNet::QuestResyncRequestPacket;
using CoopNet::QuestFullSyncPacket;
using CoopNet::HolocallStartPacket;
using CoopNet::HolocallEndPacket;
using CoopNet::TickRateChangePacket;
using CoopNet::RuleChangePacket;
using CoopNet::SpectatePacket;
using CoopNet::ScoreUpdatePacket;
using CoopNet::MatchOverPacket;
using CoopNet::ChatPacket;
using CoopNet::KillfeedPacket;
using CoopNet::AdminCmdPacket;
using CoopNet::NatCandidatePacket;
using CoopNet::CineStartPacket;
using CoopNet::VisemePacket;
using CoopNet::DialogChoicePacket;
using CoopNet::SceneTriggerPacket;
using CoopNet::VoiceCapsPacket;
using CoopNet::VoicePacket;
using CoopNet::WorldStatePacket;
using CoopNet::GlobalEventPacket;
using CoopNet::NpcReputationPacket;
using CoopNet::DynamicEventPacket;
using CoopNet::CrowdSeedPacket;
using CoopNet::VendorStockPacket;
using CoopNet::VendorStockUpdatePacket;
using CoopNet::VendorRefreshPacket;
using CoopNet::WorldMarkersPacket;
using CoopNet::NpcSpawnCruiserPacket;
using CoopNet::NpcStatePacket;
using CoopNet::CrimeEventSpawnPacket;
using CoopNet::CyberEquipPacket;
using CoopNet::SlowMoStartPacket;
using CoopNet::PerkUnlockPacket;
using CoopNet::PerkRespecRequestPacket;
using CoopNet::PerkRespecAckPacket;
using CoopNet::SkillXPPacket;
using CoopNet::StatusApplyPacket;
using CoopNet::StatusTickPacket;
using CoopNet::TrafficSeedPacket;
using CoopNet::TrafficDespawnPacket;
using CoopNet::PropBreakPacket;
using CoopNet::PropIgnitePacket;
using CoopNet::VOPlayPacket;
using CoopNet::FixerCallPacket;
using CoopNet::GigSpawnPacket;
using CoopNet::VehicleSummonPacket;
using CoopNet::AppearancePacket;
using CoopNet::PingOutlinePacket;
using CoopNet::LootRollPacket;
using CoopNet::AptPermChangePacket;
using CoopNet::AptInteriorStatePacket;
using CoopNet::VehicleTowRequestPacket;
using CoopNet::VehicleTowAckPacket;
using CoopNet::ReRollRequestPacket;
using CoopNet::ReRollResultPacket;
using CoopNet::RipperInstallRequestPacket;
using CoopNet::TileSelectPacket;
using CoopNet::TileGameStartPacket;
using CoopNet::ShardProgressPacket;
using CoopNet::TradeInitPacket;
using CoopNet::TradeOfferPacket;
using CoopNet::TradeAcceptPacket;
using CoopNet::TradeFinalizePacket;
using CoopNet::EndingVoteStartPacket;
using CoopNet::EndingVoteCastPacket;
using CoopNet::PartyInfoPacket;
using CoopNet::PartyInvitePacket;
using CoopNet::PartyLeavePacket;
using CoopNet::PartyKickPacket;
using CoopNet::DealerBuyPacket;
using CoopNet::VehicleUnlockPacket;
using CoopNet::VehicleHitHighSpeedPacket;
using CoopNet::VehicleSnapshotPacket;
using CoopNet::TurretAimPacket;
using CoopNet::AirVehSpawnPacket;
using CoopNet::AirVehUpdatePacket;
using CoopNet::VehiclePaintChangePacket;
using CoopNet::PanicEventPacket;
using CoopNet::AIHackPacket;
using CoopNet::BossPhasePacket;
using CoopNet::WeaponInspectPacket;
using CoopNet::FinisherStartPacket;
using CoopNet::FinisherEndPacket;
using CoopNet::SlowMoFinisherPacket;
using CoopNet::TextureBiasPacket;
using CoopNet::SectorLODPacket;
using CoopNet::LowBWModePacket;
using CoopNet::CrowdCfgPacket;
using CoopNet::EmotePacket;
using CoopNet::CrowdChatterStartPacket;
using CoopNet::CrowdChatterEndPacket;
using CoopNet::HoloSeedPacket;
using CoopNet::HoloNextAdPacket;
using CoopNet::DoorBreachStartPacket;
using CoopNet::DoorBreachTickPacket;
using CoopNet::DoorBreachSuccessPacket;
using CoopNet::DoorBreachAbortPacket;
using CoopNet::HTableOpenPacket;
using CoopNet::HTableScrubPacket;
using CoopNet::QuestGadgetFirePacket;
using CoopNet::ItemGrabPacket;
using CoopNet::ItemDropPacket;
using CoopNet::ItemStorePacket;
using CoopNet::MetroBoardPacket;
using CoopNet::MetroArrivePacket;
using CoopNet::RadioChangePacket;
using CoopNet::CamHijackPacket;
using CoopNet::CamFrameStartPacket;
using CoopNet::CarryBeginPacket;
using CoopNet::CarrySnapPacket;
using CoopNet::CarryEndPacket;
using CoopNet::GrenadePrimePacket;
using CoopNet::GrenadeSnapPacket;
using CoopNet::SmartCamStartPacket;
using CoopNet::SmartCamEndPacket;
using CoopNet::ArcadeStartPacket;
using CoopNet::ArcadeInputPacket;
using CoopNet::ArcadeScorePacket;
using CoopNet::ArcadeHighScorePacket;
using CoopNet::PluginRPCPacket;
using CoopNet::AssetBundlePacket;
using CoopNet::CriticalVoteStartPacket;
using CoopNet::CriticalVoteCastPacket;
using CoopNet::BranchVoteStartPacket;
using CoopNet::BranchVoteCastPacket;
using CoopNet::PhaseBundlePacket;
using CoopNet::AptPurchasePacket;
using CoopNet::AptEnterReqPacket;
using CoopNet::AptShareChangePacket;
using CoopNet::AptPurchaseAckPacket;
using CoopNet::AptEnterAckPacket;

// NAT function declarations
using CoopNet::Nat_SetCandidateCallback;
using CoopNet::Nat_PerformHandshake;
using CoopNet::Nat_GetRelayBytes;
using CoopNet::Nat_GetLocalCandidate;
using CoopNet::Nat_AddRemoteCandidate;
using CoopNet::QuestGadgetType;

// Additional function imports
using CoopNet::PoliceDispatch_OnHeatChange;
using CoopNet::GameClock;
using CoopNet::Journal_Log;

// Simple QuestSync namespace for local phase tracking
namespace QuestSync {
    static uint32_t localPhase = 0;
}

namespace
{
struct PeerEntry
{
    ENetPeer* peer;
    CoopNet::Connection* conn;
    uint32_t peerId;
};

ENetHost* g_Host = nullptr;
std::vector<PeerEntry> g_Peers;
static uint32_t g_nextPeerId = 1;
static uint32_t g_nextSnapshotId = 1;
static uint32_t g_MaxPlayers = 0;
static std::string g_ServerPassword;

// Thread safety protection for networking globals
std::mutex g_NetMutex;

// Helper used by world streaming to match sector hashing in the game.
} // namespace

void Net_Init()
{
    if (enet_initialize() != 0)
    {
        std::cout << "enet_initialize failed" << std::endl;
        return;
    }

    std::lock_guard<std::mutex> lock(g_NetMutex);
    g_Host = enet_host_create(nullptr, 8, 2, 0, 0);
    Nat_SetCandidateCallback(
        [](const char* cand)
        {
            std::cout << "Local candidate: " << cand << std::endl;
            Net_BroadcastNatCandidate(cand);
        });
    CoopNet::Nat_Start();
    CoopNet::GetAssetStreamer().Start();
    std::cout << "Net_Init complete" << std::endl;
}

void Net_Shutdown()
{
    std::lock_guard<std::mutex> lock(g_NetMutex);
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

    CoopNet::GetAssetStreamer().Stop();

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
            if (!evt.peer) {
                std::cerr << "[Net] Connect event with null peer" << std::endl;
                break;
            }
            
            PeerEntry e;
            e.peer = evt.peer;
            e.conn = new(std::nothrow) Connection();
            if (!e.conn) {
                std::cerr << "[Net] Failed to allocate Connection object" << std::endl;
                enet_peer_disconnect(evt.peer, 0);
                break;
            }
            {
                std::lock_guard<std::mutex> lock(g_NetMutex);
                e.conn->peerId = g_nextPeerId++;
            }
            e.conn->peer = evt.peer;  // Link connection to ENet peer
            e.conn->SetState(CoopNet::ConnectionState::Handshaking);

            // Check if player is banned
            if (Net_IsPlayerBanned(e.conn->peerId))
            {
                std::cout << "[Net] Rejected banned player ID " << e.conn->peerId << std::endl;
                enet_peer_disconnect(evt.peer, 0);
                delete e.conn;
            }
            else
            {
                {
                    std::lock_guard<std::mutex> lock(g_NetMutex);
                    g_Peers.push_back(e);
                }
                std::cout << "[Net] Peer connected ID=" << e.conn->peerId << std::endl;

                // Set connection to connected state
                e.conn->SetState(CoopNet::ConnectionState::Connected);

                // Initialize player synchronization
                Net_HandlePlayerJoin(e.conn->peerId, "Player_" + std::to_string(e.conn->peerId));
            }
            break;
        }
        case ENET_EVENT_TYPE_DISCONNECT:
        {
            if (!evt.peer) {
                std::cerr << "[Net] Disconnect event with null peer" << std::endl;
                break;
            }
            
            auto it =
                std::find_if(g_Peers.begin(), g_Peers.end(), [&](const PeerEntry& p) { return p.peer == evt.peer; });
            if (it != g_Peers.end())
            {
                if (it->conn) {
                    uint32_t peerId = it->conn->peerId;
                    std::cout << "[Net] Peer disconnected ID=" << peerId << std::endl;

                    // Handle player leave with synchronization
                    Net_HandlePlayerLeave(peerId, "Connection lost");

                    delete it->conn;
                }
                {
                    std::lock_guard<std::mutex> lock(g_NetMutex);
                    g_Peers.erase(it);
                }
            }
            else
            {
                std::cout << "[Net] Unknown peer disconnected" << std::endl;
            }
            break;
        }
        case ENET_EVENT_TYPE_RECEIVE:
        {
            if (!evt.packet) {
                std::cerr << "[Net] Receive event with null packet" << std::endl;
                break;
            }
            if (!evt.packet->data) {
                std::cerr << "[Net] Receive event with null packet data" << std::endl;
                enet_packet_destroy(evt.packet);
                break;
            }
            if (evt.packet->dataLength < sizeof(CoopNet::PacketHeader)) {
                std::cerr << "[Net] Packet too small: " << evt.packet->dataLength 
                          << " < " << sizeof(CoopNet::PacketHeader) << std::endl;
                enet_packet_destroy(evt.packet);
                break;
            }
            {
                auto it = std::find_if(g_Peers.begin(), g_Peers.end(),
                                       [&](const PeerEntry& p) { return p.peer == evt.peer; });
                if (it != g_Peers.end() && it->conn)
                {
                    Connection::RawPacket pkt;
                    pkt.hdr = *reinterpret_cast<CoopNet::PacketHeader*>(evt.packet->data);
                    const uint8_t* payload = evt.packet->data + sizeof(CoopNet::PacketHeader);
                    uint16_t psize = evt.packet->dataLength - sizeof(CoopNet::PacketHeader);
                    if (it->conn->hasKey && pkt.hdr.type != static_cast<uint16_t>(CoopNet::EMsg::Hello) &&
                        pkt.hdr.type != static_cast<uint16_t>(CoopNet::EMsg::Welcome))
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
    std::lock_guard<std::mutex> lock(g_NetMutex);
    return !g_Peers.empty();
}

std::vector<CoopNet::Connection*> Net_GetConnections()
{
    std::vector<CoopNet::Connection*> out;
    {
        std::lock_guard<std::mutex> lock(g_NetMutex);
        out.reserve(g_Peers.size());
        for (auto& e : g_Peers)
            if (e.conn) out.push_back(e.conn);
    }
    return out;
}

std::vector<uint32_t> Net_GetConnectionPeerIds()
{
    std::vector<uint32_t> ret;
    {
        std::lock_guard<std::mutex> lock(g_NetMutex);
        ret.reserve(g_Peers.size());
        for (const auto& p : g_Peers)
            if (p.conn)
                ret.push_back(p.peerId);
    }
    return ret;
}

void Net_Send(CoopNet::Connection* conn, CoopNet::EMsg type, const void* data, uint16_t size)
{
    if (!g_Host || !conn)
        return;
    
    auto it = std::find_if(g_Peers.begin(), g_Peers.end(), [&](const PeerEntry& p) { return p.conn == conn; });
    if (it == g_Peers.end())
        return;

    std::vector<uint8_t> outBuf;
    uint16_t finalSize = size;
    if (conn->hasKey && type != CoopNet::EMsg::Hello && type != CoopNet::EMsg::Welcome)
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
    ENetPacket* pkt = enet_packet_create(nullptr, sizeof(CoopNet::PacketHeader) + finalSize, ENET_PACKET_FLAG_RELIABLE);
    CoopNet::PacketHeader hdr{static_cast<uint16_t>(type), finalSize};
    std::memcpy(pkt->data, &hdr, sizeof(hdr));
    if (finalSize > 0 && data)
        std::memcpy(pkt->data + sizeof(hdr), data, finalSize);
    enet_peer_send(it->peer, 0, pkt);
}

void Net_Broadcast(CoopNet::EMsg type, const void* data, uint16_t size)
{
    if (!g_Host)
        return;
    {
        std::lock_guard<std::mutex> lock(g_NetMutex);
        for (auto& e : g_Peers)
        {
            Net_Send(e.conn, type, data, size);
        }
    }
}

void Net_SendUnreliableToAll(CoopNet::EMsg type, const void* data, uint16_t size)
{
    if (!g_Host)
        return;
    {
        std::lock_guard<std::mutex> lock(g_NetMutex);
        for (auto& e : g_Peers)
        {
            ENetPacket* pkt = enet_packet_create(nullptr, sizeof(CoopNet::PacketHeader) + size, 0);
            CoopNet::PacketHeader hdr{static_cast<uint16_t>(type), size};
            std::memcpy(pkt->data, &hdr, sizeof(hdr));
            if (size > 0 && data)
                std::memcpy(pkt->data + sizeof(hdr), data, size);
            enet_peer_send(e.peer, 0, pkt);
        }
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
        Net_Send(conns[0], CoopNet::EMsg::CraftRequest, &pkt, sizeof(pkt));
    }
}

void Net_SendAttachRequest(uint64_t itemId, uint8_t slotIdx, uint64_t attachmentId)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        AttachModRequestPacket pkt{0u, itemId, slotIdx, {0, 0, 0}, attachmentId};
        Net_Send(conns[0], CoopNet::EMsg::AttachModRequest, &pkt, sizeof(pkt));
    }
}

void Net_SendPurchaseRequest(uint32_t vendorId, uint32_t itemId, uint64_t nonce)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        PurchaseRequestPacket pkt{vendorId, itemId, nonce};
        Net_Send(conns[0], CoopNet::EMsg::PurchaseRequest, &pkt, sizeof(pkt));
    }
}

void Net_SendVehicleSummonRequest(uint32_t vehId, const TransformSnap& pos)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        VehicleSummonRequestPacket pkt{vehId, pos};
        Net_Send(conns[0], CoopNet::EMsg::VehicleSummonRequest, &pkt, sizeof(pkt));
    }
}

void Net_SendBreachInput(uint8_t index)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        BreachInputPacket pkt{0u, index, {0, 0, 0}};
        Net_Send(conns[0], CoopNet::EMsg::BreachInput, &pkt, sizeof(pkt));
    }
}

void Net_SendElevatorCall(uint32_t elevatorId, uint8_t floorIdx)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        ElevatorCallPacket pkt{0u, elevatorId, floorIdx, {0, 0, 0}};
        Net_Send(conns[0], CoopNet::EMsg::ElevatorCall, &pkt, sizeof(pkt));
    }
}

void Net_BroadcastVehicleExplode(uint32_t vehicleId, uint32_t vfxId, uint32_t seed)
{
    VehicleExplodePacket pkt{vehicleId, vfxId, seed};
    Net_Broadcast(CoopNet::EMsg::VehicleExplode, &pkt, sizeof(pkt));
}

void Net_BroadcastPartDetach(uint32_t vehicleId, uint8_t partId)
{
    VehiclePartDetachPacket pkt{vehicleId, partId, {0, 0, 0}};
    Net_Broadcast(CoopNet::EMsg::VehiclePartDetach, &pkt, sizeof(pkt));
}

void Net_BroadcastEject(uint32_t peerId, const RED4ext::Vector3& vel)
{
    EjectOccupantPacket pkt{peerId, vel};
    Net_Broadcast(CoopNet::EMsg::EjectOccupant, &pkt, sizeof(pkt));
}

void Net_BroadcastVehicleSpawn(uint32_t vehicleId, uint32_t archetypeId, uint32_t paintId, uint32_t phaseId,
                               const TransformSnap& t)
{
    VehicleSpawnPacket pkt{vehicleId, archetypeId, paintId, phaseId, t};
    Net_Broadcast(CoopNet::EMsg::VehicleSpawn, &pkt, sizeof(pkt));
}

void Net_SendSeatRequest(uint32_t vehicleId, uint8_t seatIdx)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        SeatRequestPacket pkt{vehicleId, seatIdx};
        Net_Send(conns[0], CoopNet::EMsg::SeatRequest, &pkt, sizeof(pkt));
    }
}

void Net_BroadcastSeatAssign(uint32_t peerId, uint32_t vehicleId, uint8_t seatIdx)
{
    SeatAssignPacket pkt{peerId, vehicleId, seatIdx};
    Net_Broadcast(CoopNet::EMsg::SeatAssign, &pkt, sizeof(pkt));
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
    {
        std::lock_guard<std::mutex> lock(g_NetMutex);
        for (auto& e : g_Peers)
            CoopNet::QuestWatchdog_Record(e.conn->peerId, nameHash, stage);
    }
    Journal_Log(GameClock::GetCurrentTick(), 0, "questStage", nameHash, stage);
    Net_Broadcast(EMsg::QuestStage, &pkt, sizeof(pkt));
}

void Net_BroadcastQuestStageP2P(uint32_t phaseId, uint32_t questHash, uint16_t stage)
{
    QuestStageP2PPacket pkt{phaseId, questHash, stage, 0};
    {
        std::lock_guard<std::mutex> lock(g_NetMutex);
        for (auto& e : g_Peers)
            CoopNet::QuestWatchdog_Record(phaseId, questHash, stage);
    }
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


void Net_SetConnectionAvatarPos(uint32_t peerId, const RED4ext::Vector3& pos)
{
    CoopNet::Connection* conn = Net_FindConnection(peerId);
    if (conn)
        conn->avatarPos = pos;
}

RED4ext::Vector3 Net_GetConnectionAvatarPos(uint32_t peerId)
{
    CoopNet::Connection* conn = Net_FindConnection(peerId);
    if (conn)
        return conn->avatarPos;
    return {0.0f, 0.0f, 0.0f};
}

uint32_t Net_GetConnectionPeerId(CoopNet::Connection* conn)
{
    if (conn)
        return conn->peerId;
    return 0;
}

void Net_SendPluginRPCToPeer(uint32_t peerId, uint16_t pluginId, uint32_t fnHash, const char* json, uint16_t len)
{
    CoopNet::Connection* conn = Net_FindConnection(peerId);
    if (conn)
        Net_SendPluginRPC(conn, pluginId, fnHash, json, len);
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

void Net_BroadcastAvatarSpawn(uint32_t peerId, const TransformSnap& snap)
{
    CoopNet::AvatarSpawnPacket pkt{peerId, snap, 0u};
    Net_Broadcast(EMsg::AvatarSpawn, &pkt, sizeof(pkt));
}

void Net_BroadcastAvatarDespawn(uint32_t peerId)
{
    CoopNet::AvatarDespawnPacket pkt{peerId, 0u};
    Net_Broadcast(EMsg::AvatarDespawn, &pkt, sizeof(pkt));
}

void Net_BroadcastPlayerUpdate(uint32_t peerId,
                               const RED4ext::Vector3& pos,
                               const RED4ext::Vector3& vel,
                               const RED4ext::Quaternion& rot,
                               uint16_t health,
                               uint16_t armor)
{
    // Build a minimal transform snapshot using SnapshotWriter
    CoopNet::SnapshotWriter writer;
    CoopNet::SnapshotHeader hdr{g_nextSnapshotId++, (g_nextSnapshotId > 1) ? (g_nextSnapshotId - 1) : 0u};
    writer.Begin(hdr);
    writer.Write(0, pos);
    writer.Write(1, vel);
    writer.Write(2, rot);
    writer.Write(3, health);
    writer.Write(4, armor);
    uint32_t owner = peerId;
    writer.Write(5, owner);
    uint16_t seq = static_cast<uint16_t>(hdr.id);
    writer.Write(6, seq);

    std::vector<uint8_t> buf(sizeof(CoopNet::SnapshotHeader) + sizeof(CoopNet::SnapshotFieldFlags) + sizeof(TransformSnap));
    size_t bytes = writer.End(buf.data(), buf.size());
    if (bytes == 0)
        return;
    Net_Broadcast(EMsg::Snapshot, buf.data(), static_cast<uint16_t>(bytes));
}

void Net_SendAdminCmd(CoopNet::Connection* conn, uint8_t cmdType, uint64_t param)
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

void Net_BroadcastSceneTrigger(uint32_t phaseId, uint32_t nameHash, bool start)
{
    SceneTriggerPacket pkt{phaseId, nameHash, static_cast<uint8_t>(start), {0, 0, 0}};
    Net_Broadcast(EMsg::SceneTrigger, &pkt, sizeof(pkt));
}

void Net_SendVoiceCaps(CoopNet::Connection* conn, uint16_t maxBytes)
{
    if (!conn)
        return;
    VoiceCapsPacket pkt{maxBytes, {0, 0}};
    Net_Send(conn, EMsg::VoiceCaps, &pkt, sizeof(pkt));
}

void Net_SendVoice(const uint8_t* data, uint16_t size, uint16_t seq)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        auto* c = conns[0];
        if (c->voiceMuted)
            return;
        uint16_t cap = c->voiceFrameBytes ? c->voiceFrameBytes : CoopVoice::GetFrameBytes();
        uint16_t offset = 0;
        while (offset < size)
        {
            uint16_t chunk = std::min<uint16_t>(cap, static_cast<uint16_t>(size - offset));
            VoicePacket pkt{0u, seq, chunk, {0}};
            std::memcpy(pkt.data, data + offset, chunk);
            Net_Send(c, EMsg::Voice, &pkt, static_cast<uint16_t>(sizeof(pkt)));
            c->voiceBytes += sizeof(pkt);
            offset += chunk;
            if (offset < size)
                ++seq;
        }
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

void Net_SendWorldState(CoopNet::Connection* conn, uint16_t sunAngleDeg, uint8_t weatherId, uint16_t particleSeed)
{
    if (!conn)
        return;
    WorldStatePacket pkt{sunAngleDeg, weatherId, particleSeed};
    Net_Send(conn, EMsg::WorldState, &pkt, sizeof(pkt));
}

void Net_SendGlobalEvent(CoopNet::Connection* conn, uint32_t eventId, uint8_t phase, bool start, uint32_t seed)
{
    if (!conn)
        return;
    GlobalEventPacket pkt{eventId, seed, phase, static_cast<uint8_t>(start), {0,0}};
    Net_Send(conn, EMsg::GlobalEvent, &pkt, sizeof(pkt));
}

void Net_SendNpcReputation(CoopNet::Connection* conn, uint32_t npcId, int16_t value)
{
    if (!conn)
        return;
    NpcReputationPacket pkt{npcId, value, {0,0}};
    Net_Send(conn, EMsg::NpcReputation, &pkt, sizeof(pkt));
}

void Net_BroadcastNpcReputation(uint32_t npcId, int16_t value)
{
    NpcReputationPacket pkt{npcId, value, {0,0}};
    Net_Broadcast(EMsg::NpcReputation, &pkt, sizeof(pkt));
}

void Net_BroadcastGlobalEvent(uint32_t eventId, uint8_t phase, bool start, uint32_t seed)
{
    GlobalEventPacket pkt{eventId, seed, phase, static_cast<uint8_t>(start), {0, 0}};
    Net_Broadcast(EMsg::GlobalEvent, &pkt, sizeof(pkt));
}

void Net_BroadcastDynamicEvent(uint8_t eventType, uint32_t seed)
{
    DynamicEventPacket pkt{eventType, {0,0,0}, seed};
    Net_Broadcast(EMsg::DynamicEvent, &pkt, sizeof(pkt));
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

void Net_SendWorldMarkers(CoopNet::Connection* conn, const std::vector<uint8_t>& blob)
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

void Net_BroadcastLootRoll(uint32_t containerId, uint32_t seed, const std::vector<uint64_t>& items)
{
    if (items.size() > 16)
        return;
    LootRollPacket pkt{};
    pkt.containerId = containerId;
    pkt.seed = seed;
    pkt.count = static_cast<uint8_t>(items.size());
    std::fill(std::begin(pkt._pad), std::end(pkt._pad), 0);
    for (size_t i = 0; i < items.size(); ++i)
        pkt.itemIds[i] = items[i];
    Net_Broadcast(EMsg::LootRoll, &pkt, 12 + sizeof(uint64_t) * pkt.count);
}

void Net_BroadcastAptPermChange(uint32_t aptId, uint32_t targetPeerId, bool allow)
{
    AptPermChangePacket pkt{aptId, targetPeerId, static_cast<uint8_t>(allow), {0, 0, 0}};
    Net_Broadcast(EMsg::AptPermChange, &pkt, sizeof(pkt));
}

void Net_BroadcastAptInteriorState(uint32_t phaseId, const char* json, uint16_t len)
{
    std::vector<uint8_t> buf(sizeof(AptInteriorStatePacket) + len);
    auto* pkt = reinterpret_cast<AptInteriorStatePacket*>(buf.data());
    pkt->phaseId = phaseId;
    pkt->blobBytes = len;
    std::memcpy(pkt->json, json, len);
    Net_Broadcast(EMsg::AptInteriorState, pkt, static_cast<uint16_t>(buf.size()));
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

void Net_SendVehicleTowAck(CoopNet::Connection* conn, uint32_t ownerId, bool ok)
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

void Net_SendReRollResult(CoopNet::Connection* conn, const ItemSnap& snap)
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
    ShardProgressPacket pkt{0u, phaseId, 0, 0, percent, {0}};
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

void Net_BroadcastPartyInfo(const uint32_t* ids, uint8_t count)
{
    PartyInfoPacket pkt{};
    pkt.count = count > 8 ? 8 : count;
    for (uint8_t i = 0; i < pkt.count; ++i)
        pkt.peerIds[i] = ids[i];
    Net_Broadcast(EMsg::PartyInfo, &pkt, sizeof(pkt));
}

void Net_SendPartyInvite(uint32_t targetPeerId)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        PartyInvitePacket pkt{Net_GetPeerId(), targetPeerId};
        Net_Send(conns[0], EMsg::PartyInvite, &pkt, sizeof(pkt));
    }
}

void Net_SendPartyLeave()
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        PartyLeavePacket pkt{Net_GetPeerId()};
        Net_Send(conns[0], EMsg::PartyLeave, &pkt, sizeof(pkt));
    }
}

void Net_SendPartyKick(uint32_t peerId)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        PartyKickPacket pkt{peerId};
        Net_Send(conns[0], EMsg::PartyKick, &pkt, sizeof(pkt));
    }
}

void Net_BroadcastPartyInvite(uint32_t fromId, uint32_t toId)
{
    PartyInvitePacket pkt{fromId, toId};
    Net_Broadcast(EMsg::PartyInvite, &pkt, sizeof(pkt));
}

void Net_BroadcastPartyLeave(uint32_t peerId)
{
    PartyLeavePacket pkt{peerId};
    Net_Broadcast(EMsg::PartyLeave, &pkt, sizeof(pkt));
}

void Net_BroadcastPartyKick(uint32_t peerId)
{
    PartyKickPacket pkt{peerId};
    Net_Broadcast(EMsg::PartyKick, &pkt, sizeof(pkt));
}

void Net_SendDealerBuy(uint32_t vehicleTpl, uint32_t price)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        DealerBuyPacket pkt{Net_GetPeerId(), vehicleTpl, price};
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
    QuestGadgetFirePacket pkt{0u, questId, static_cast<uint8_t>(type), charge, targetId, {0, 0, 0}};
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
    {
        std::lock_guard<std::mutex> lock(g_NetMutex);
        for (auto& e : g_Peers)
        {
            if (!e.conn->lowBWMode)
                Net_Send(e.conn, EMsg::SmartCamStart, &pkt, sizeof(pkt));
        }
    }
}

void Net_BroadcastSmartCamEnd(uint32_t projId)
{
    SmartCamEndPacket pkt{projId};
    {
        std::lock_guard<std::mutex> lock(g_NetMutex);
        for (auto& e : g_Peers)
        {
            if (!e.conn->lowBWMode)
                Net_Send(e.conn, EMsg::SmartCamEnd, &pkt, sizeof(pkt));
        }
    }
}

void Net_BroadcastArcadeStart(uint32_t cabId, uint32_t peerId, uint32_t seed)
{
    ArcadeStartPacket pkt{cabId, peerId, seed};
    Net_Broadcast(EMsg::ArcadeStart, &pkt, sizeof(pkt));
}

void Net_SendArcadeInput(uint32_t frame, uint8_t buttonMask)
{
    ArcadeInputPacket pkt{0u, frame, buttonMask, {0, 0, 0}};
    if (g_Peers.size() > 0)
        Net_Send(g_Peers[0].conn, EMsg::ArcadeInput, &pkt, sizeof(pkt));
}

void Net_BroadcastArcadeScore(uint32_t peerId, uint32_t score)
{
    ArcadeScorePacket pkt{peerId, score};
    Net_Broadcast(EMsg::ArcadeScore, &pkt, sizeof(pkt));
}

void Net_BroadcastArcadeHighScore(uint32_t cabId, uint32_t peerId, uint32_t score)
{
    ArcadeHighScorePacket pkt{cabId, peerId, score};
    Net_Broadcast(EMsg::ArcadeHighScore, &pkt, sizeof(pkt));
}

void Net_SendPluginRPC(CoopNet::Connection* conn, uint16_t pluginId, uint32_t fnHash,
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

void Net_BroadcastBranchVoteStart(uint32_t questHash, uint16_t stage)
{
    BranchVoteStartPacket pkt{questHash, stage, {0,0}};
    Net_Broadcast(EMsg::BranchVoteStart, &pkt, sizeof(pkt));
}

void Net_SendBranchVoteCast(bool yes)
{
    BranchVoteCastPacket pkt{0u, static_cast<uint8_t>(yes), {0,0,0}};
    auto conns = Net_GetConnections();
    if (!conns.empty())
        Net_Send(conns[0], EMsg::BranchVoteCast, &pkt, sizeof(pkt));
}

void Net_SendPhaseBundle(CoopNet::Connection* conn, uint32_t phaseId, const std::vector<uint8_t>& blob)
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

void Net_SendAptShareChange(uint32_t aptId, uint32_t targetPeerId, bool allow)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        AptShareChangePacket pkt{aptId, targetPeerId, static_cast<uint8_t>(allow), {0, 0, 0}};
        Net_Send(conns[0], EMsg::AptShareChange, &pkt, sizeof(pkt));
    }
}

void Net_SendAptInteriorStateReq(const char* json, uint32_t len)
{
    auto conns = Net_GetConnections();
    if (!conns.empty())
    {
        std::vector<uint8_t> buf(sizeof(AptInteriorStatePacket) + len);
        auto* pkt = reinterpret_cast<AptInteriorStatePacket*>(buf.data());
        pkt->phaseId = 0; // filled by server
        pkt->blobBytes = static_cast<uint16_t>(len);
        std::memcpy(pkt->json, json, len);
        Net_Send(conns[0], EMsg::AptInteriorState, pkt, static_cast<uint16_t>(buf.size()));
    }
}

void Net_SendAptPurchaseAck(CoopNet::Connection* conn, uint32_t aptId, bool success, uint64_t balance)
{
    if (!conn)
        return;
    AptPurchaseAckPacket pkt{aptId, balance, static_cast<uint8_t>(success), {0, 0, 0}};
    Net_Send(conn, EMsg::AptPurchaseAck, &pkt, sizeof(pkt));
}

void Net_SendAptEnterAck(CoopNet::Connection* conn, bool allow, uint32_t phaseId, uint32_t interiorSeed)
{
    if (!conn)
        return;
    AptEnterAckPacket pkt{static_cast<uint8_t>(allow), {0, 0, 0}, phaseId, interiorSeed};
    Net_Send(conn, EMsg::AptEnterAck, &pkt, sizeof(pkt));
}

// ===== MISSING SERVER FUNCTIONS - IMPLEMENTATION =====

bool Net_StartServer(uint32_t port, uint32_t maxPlayers)
{
    std::cout << "[Net_StartServer] Starting server on port " << port << " for " << maxPlayers << " players" << std::endl;
    
    // Create server host
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;
    
    if (g_Host) {
        enet_host_destroy(g_Host);
    }
    
    g_Host = enet_host_create(&address, maxPlayers, 2, 0, 0);
    g_MaxPlayers = maxPlayers;
    if (!g_Host) {
        std::cerr << "[Net_StartServer] Failed to create server host on port " << port << std::endl;
        return false;
    }
    
    std::cout << "[Net_StartServer] Server successfully started on port " << port << std::endl;
    return true;
}

void Net_StopServer()
{
    std::lock_guard<std::mutex> lock(g_NetMutex);
    if (!g_Host)
        return;
    for (auto& e : g_Peers)
    {
        if (e.peer)
            enet_peer_disconnect(e.peer, 0);
        if (e.conn)
            delete e.conn;
    }
    g_Peers.clear();
    enet_host_destroy(g_Host);
    g_Host = nullptr;
}

void Net_SetServerPassword(const std::string& password)
{
    std::lock_guard<std::mutex> lock(g_NetMutex);
    g_ServerPassword = password;
}

ServerInfo Net_GetServerInfo()
{
    std::lock_guard<std::mutex> lock(g_NetMutex);
    ServerInfo info{};
    info.name = "cp2077-coop";
    info.playerCount = static_cast<uint32_t>(g_Peers.size());
    info.maxPlayers = g_MaxPlayers;
    info.hasPassword = !g_ServerPassword.empty();
    info.mode = "Coop";
    return info;
}

void InitializeGameSystems()
{
    std::cout << "[InitializeGameSystems] Initializing core game systems..." << std::endl;
    
    // Initialize session state
    CoopNet::SessionState_SetParty(std::vector<uint32_t>());
    
    std::cout << "[InitializeGameSystems] Game systems initialized successfully" << std::endl;
}

void LoadServerPlugins()
{
    std::cout << "[LoadServerPlugins] Loading server plugins..." << std::endl;
    
    // TODO: Implement actual plugin loading
    // For now, just log that we're ready for plugins
    
    std::cout << "[LoadServerPlugins] Server ready for plugin connections" << std::endl;
}

bool Net_ConnectToServer(const char* host, uint32_t port)
{
    std::cout << "[Net_ConnectToServer] Connecting to " << host << ":" << port << std::endl;
    
    if (!g_Host) {
        std::cerr << "[Net_ConnectToServer] Network not initialized, call Net_Init() first" << std::endl;
        return false;
    }
    
    ENetAddress address;
    enet_address_set_host(&address, host);
    address.port = port;
    
    ENetPeer* peer = enet_host_connect(g_Host, &address, 2, 0);
    if (!peer) {
        std::cerr << "[Net_ConnectToServer] Failed to create connection peer" << std::endl;
        return false;
    }
    
    std::cout << "[Net_ConnectToServer] Connection attempt initiated to " << host << ":" << port << std::endl;
    return true;
}

uint32_t Net_GetPeerId()
{
    // For client, return a fixed peer ID for now
    // In a full implementation, this would be assigned by the server
    return 1;
}

// Stub implementations for missing functions
bool Net_IsPlayerBanned(uint32_t peerId) {
    return CoopNet::AdminController_IsBanned(peerId);
}

void Net_HandlePlayerJoin(uint32_t peerId, const std::string& playerName) {
    std::cout << "[Net] Player " << playerName << " (ID: " << peerId << ") joined" << std::endl;
}

void Net_HandlePlayerLeave(uint32_t peerId, const std::string& reason) {
    std::cout << "[Net] Player ID " << peerId << " left: " << reason << std::endl;
}

CoopNet::Connection* Net_FindConnection(uint32_t peerId) {
    auto it = std::find_if(g_Peers.begin(), g_Peers.end(),
        [peerId](const PeerEntry& p) { return p.conn && p.conn->peerId == peerId; });

    if (it != g_Peers.end() && it->conn) {
        return it->conn;
    }
    return nullptr;
}
