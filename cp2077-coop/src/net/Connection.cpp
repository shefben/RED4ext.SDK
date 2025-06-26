#include "Connection.hpp"
#include "../core/GameClock.hpp"
#include "../server/BreachController.hpp"
#include "../server/NpcController.hpp"
#include "../voice/VoiceDecoder.hpp"
#include "../core/SessionState.hpp"
#include "Net.hpp"
#include "StatBatch.hpp"
#include "../core/Hash.hpp"
#include <RED4ext/RED4ext.hpp>
#include <iostream>

// Temporary proxies for script methods.
static void AvatarProxy_Spawn(uint32_t peerId, const CoopNet::TransformSnap& snap, bool isLocal)
{
    std::cout << "AvatarProxy::Spawn " << peerId << " local=" << isLocal
              << " pos=" << snap.pos.X << "," << snap.pos.Y << std::endl;
}

static void AvatarProxy_Despawn(uint32_t peerId)
{
    std::cout << "AvatarProxy::Despawn " << peerId << std::endl;
}

static void Killfeed_Push(const char* msg)
{
    std::cout << "Killfeed: " << msg << std::endl;
}

static void ChatOverlay_Push(const char* msg)
{
    RED4ext::CString s(msg);
    RED4ext::ExecuteFunction("ChatOverlay", "PushGlobal", nullptr, &s);
}

static void QuestSync_ApplyQuestStage(const char* name)
{
    std::cout << "Quest stage " << name << std::endl;
}

static void QuestSync_ApplySceneTrigger(const char* id, bool start)
{
    std::cout << "SceneTrigger " << id << " start=" << start << std::endl;
}

static void DMScoreboard_OnScorePacket(uint32_t peerId, uint16_t k, uint16_t d)
{
    std::cout << "ScoreUpdate " << peerId << " " << k << "/" << d << std::endl;
}

static void NpcProxy_Spawn(const CoopNet::NpcSnap& snap)
{
    std::cout << "NpcProxy::Spawn " << snap.npcId << std::endl;
}

static void NpcProxy_Despawn(uint32_t npcId)
{
    std::cout << "NpcProxy::Despawn " << npcId << std::endl;
}

static void NpcProxy_ApplySnap(const CoopNet::NpcSnap& snap)
{
    CoopNet::NpcController_ClientApplySnap(snap);
}

static void Cutscene_OnCineStart(uint32_t sceneId, uint32_t startMs)
{
    CutsceneSync_CineStart(sceneId, startMs);
}

static void Cutscene_OnViseme(uint32_t npcId, uint8_t visemeId, uint32_t timeMs)
{
    CutsceneSync_Viseme(npcId, visemeId, timeMs);
}

static void Cutscene_OnDialogChoice(uint32_t peerId, uint8_t idx)
{
    CutsceneSync_DialogChoice(peerId, idx);
}

static void Inventory_OnItemSnap(const CoopNet::ItemSnap& snap)
{
    std::cout << "ItemSnap " << snap.itemId << std::endl;
}

static void Inventory_OnCraftResult(const CoopNet::ItemSnap& snap)
{
    std::cout << "CraftResult item=" << snap.itemId << std::endl;
}

static void Inventory_OnAttachResult(const CoopNet::ItemSnap& snap, bool success)
{
    std::cout << "AttachResult item=" << snap.itemId << " success=" << success << std::endl;
}

static void AvatarProxy_OnSectorChange(uint32_t peerId, uint64_t hash)
{
    std::cout << "SectorChange " << peerId << " -> " << hash << std::endl;
}

static void VehicleProxy_Explode(uint32_t id, uint32_t vfx, uint32_t seed)
{
    std::cout << "Vehicle explode " << id << " vfx=" << vfx << " seed=" << seed << std::endl;
}

static void VehicleProxy_Detach(uint32_t id, uint8_t part)
{
    std::cout << "Vehicle detach " << id << " part " << static_cast<int>(part) << std::endl;
}

static void AvatarProxy_OnEject(uint32_t peerId)
{
    std::cout << "Eject occupant " << peerId << std::endl;
}

static void BreachHud_Start(uint32_t peerId, uint32_t seed, uint8_t w, uint8_t h)
{
    std::cout << "Breach start seed=" << seed << " w=" << static_cast<int>(w) << " h=" << static_cast<int>(h)
              << std::endl;
}

static void BreachHud_Input(uint32_t peerId, uint8_t idx)
{
    std::cout << "Breach input peer=" << peerId << " idx=" << static_cast<int>(idx) << std::endl;
}

static void Quickhack_BreachResult(uint32_t peerId, uint8_t mask)
{
    std::cout << "Breach result mask=" << static_cast<int>(mask) << std::endl;
}

static void SpectatorCam_Enter(uint32_t peerId)
{
    std::cout << "Enter spectate " << peerId << std::endl;
}

static void ElevatorSync_OnArrive(uint32_t id, uint64_t hash, const RED4ext::Vector3& pos)
{
    (void)id;
    (void)hash;
    (void)pos;
}

static void UIPauseAudit_OnHoloStart(uint32_t peerId)
{
    std::cout << "HoloCall start " << peerId << std::endl;
}

static void UIPauseAudit_OnHoloEnd(uint32_t peerId)
{
    std::cout << "HoloCall end " << peerId << std::endl;
}

static void SnapshotInterpolator_OnTickRateChange(uint16_t ms)
{
    std::cout << "TickRateChange " << ms << " ms" << std::endl;
}

namespace CoopNet
{

Connection::Connection()
    : state(ConnectionState::Disconnected)
    , lastPingSent(0)
    , lastRecvTime(0)
    , avatarPos{0.f, 0.f, 0.f}
    , currentSector(0)
    , sectorReady(true)
{
}

void Connection::SendSectorChange(uint64_t hash)
{
    SectorChangePacket pkt{0u, hash};
    Net_Send(this, EMsg::SectorChange, &pkt, sizeof(pkt));
    sectorReady = false;
    currentSector = hash;
    lastSectorChangeTick = CoopNet::GameClock::GetCurrentTick();
}

void Connection::SendSectorReady(uint64_t hash)
{
    SectorReadyPacket pkt{hash};
    Net_Send(this, EMsg::SectorReady, &pkt, sizeof(pkt));
    sectorReady = true;
    currentSector = hash;
}

void Connection::StartHandshake()
{
    Transition(ConnectionState::Handshaking);
}

void Connection::HandlePacket(const PacketHeader& hdr, const void* payload, uint16_t size)
{
    (void)payload;
    (void)size;
    switch (static_cast<EMsg>(hdr.type))
    {
    case EMsg::Welcome:
        if (state == ConnectionState::Handshaking)
        {
            Transition(ConnectionState::Lobby);
        }
        break;
    case EMsg::JoinAccept:
        if (state == ConnectionState::Lobby)
        {
            Transition(ConnectionState::InGame);
            uint64_t hash = CoopNet::Fnv1a64Pos(avatarPos.X, avatarPos.Y);
            SectorChangePacket pkt{0u, hash};
            Net_Send(this, EMsg::SectorChange, &pkt, sizeof(pkt));
            std::vector<uint32_t> ids;
            for (auto* c : Net_GetConnections())
                ids.push_back(c->peerId);
            CoopNet::SessionState_SetParty(ids);
        }
        break;
    case EMsg::Disconnect:
        Killfeed_Push("0 disconnected");
        Transition(ConnectionState::Disconnected);
        break;
    case EMsg::AvatarSpawn:
        if (size >= sizeof(AvatarSpawnPacket))
        {
            const AvatarSpawnPacket* pkt = reinterpret_cast<const AvatarSpawnPacket*>(payload);
            AvatarProxy_Spawn(pkt->peerId, pkt->snap, pkt->peerId == 0);
            uint64_t hash = CoopNet::Fnv1a64Pos(pkt->snap.pos.X, pkt->snap.pos.Y);
            SectorChangePacket sp{pkt->peerId, hash};
            Net_Send(this, EMsg::SectorChange, &sp, sizeof(sp));
        }
        break;
    case EMsg::AvatarDespawn:
        if (size >= sizeof(AvatarDespawnPacket))
        {
            const AvatarDespawnPacket* pkt = reinterpret_cast<const AvatarDespawnPacket*>(payload);
            AvatarProxy_Despawn(pkt->peerId);
        }
        Killfeed_Push("0 disconnected");
        break;
    case EMsg::Chat:
        if (Net_IsAuthoritative())
        {
            if (CoopNet::GameClock::GetTimeMs() < muteUntilMs)
                break;
            if (size >= sizeof(ChatPacket))
            {
                const ChatPacket* pkt = reinterpret_cast<const ChatPacket*>(payload);
                ChatPacket out{peerId, {0}};
                std::strncpy(out.msg, pkt->msg, sizeof(out.msg)-1);
                Net_Broadcast(EMsg::Chat, &out, sizeof(out));
            }
        }
        if (size >= sizeof(ChatPacket))
        {
            const ChatPacket* pkt = reinterpret_cast<const ChatPacket*>(payload);
            ChatOverlay_Push(pkt->msg);
        }
        break;
    case EMsg::QuestStage:
        QuestSync_ApplyQuestStage(n "tempQuest"); // P4-1: parse payload
        break;
    case EMsg::SceneTrigger:
        QuestSync_ApplySceneTrigger("0", true); // P4-2: parse payload
        break;
    case EMsg::NpcSpawn:
        if (size >= sizeof(NpcSpawnPacket))
        {
            const NpcSpawnPacket* pkt = reinterpret_cast<const NpcSpawnPacket*>(payload);
            NpcProxy_Spawn(pkt->snap);
        }
        break;
    case EMsg::NpcSnapshot:
        if (size >= sizeof(NpcSnapshotPacket))
        {
            const NpcSnapshotPacket* pkt = reinterpret_cast<const NpcSnapshotPacket*>(payload);
            NpcProxy_ApplySnap(pkt->snap);
        }
        break;
    case EMsg::NpcDespawn:
        if (size >= sizeof(NpcDespawnPacket))
        {
            const NpcDespawnPacket* pkt = reinterpret_cast<const NpcDespawnPacket*>(payload);
            NpcProxy_Despawn(pkt->npcId);
        }
        break;
    case EMsg::SectorChange:
        if (size >= sizeof(SectorChangePacket))
        {
            const SectorChangePacket* pkt = reinterpret_cast<const SectorChangePacket*>(payload);
            AvatarProxy_OnSectorChange(pkt->peerId, pkt->sectorHash);
            sectorReady = false;
            currentSector = pkt->sectorHash;
            lastSectorChangeTick = CoopNet::GameClock::GetCurrentTick();
            // Ack will be sent from OnStreamingDone hook.
        }
        break;
    case EMsg::SectorReady:
        if (size >= sizeof(SectorReadyPacket))
        {
            const SectorReadyPacket* pkt = reinterpret_cast<const SectorReadyPacket*>(payload);
            sectorReady = true;
            currentSector = pkt->sectorHash;
        }
        break;
    case EMsg::ScoreUpdate:
        if (size >= sizeof(ScoreUpdatePacket))
        {
            const ScoreUpdatePacket* pkt = reinterpret_cast<const ScoreUpdatePacket*>(payload);
            DMScoreboard_OnScorePacket(pkt->peerId, pkt->k, pkt->d);
        }
        break;
    case EMsg::ItemSnap:
        if (size >= sizeof(ItemSnapPacket))
        {
            const ItemSnapPacket* pkt = reinterpret_cast<const ItemSnapPacket*>(payload);
            Inventory_OnItemSnap(pkt->snap);
        }
        break;
    case EMsg::CraftResult:
        if (size >= sizeof(CraftResultPacket))
        {
            const CraftResultPacket* pkt = reinterpret_cast<const CraftResultPacket*>(payload);
            Inventory_OnCraftResult(pkt->item);
        }
        break;
    case EMsg::AttachModResult:
        if (size >= sizeof(AttachModResultPacket))
        {
            const AttachModResultPacket* pkt = reinterpret_cast<const AttachModResultPacket*>(payload);
            Inventory_OnAttachResult(pkt->item, pkt->success != 0);
        }
        break;
    case EMsg::VehicleExplode:
        if (size >= sizeof(VehicleExplodePacket))
        {
            const VehicleExplodePacket* pkt = reinterpret_cast<const VehicleExplodePacket*>(payload);
            VehicleProxy_Explode(pkt->vehicleId, pkt->vfxId, pkt->seed);
        }
        break;
    case EMsg::VehiclePartDetach:
        if (size >= sizeof(VehiclePartDetachPacket))
        {
            const VehiclePartDetachPacket* pkt = reinterpret_cast<const VehiclePartDetachPacket*>(payload);
            VehicleProxy_Detach(pkt->vehicleId, pkt->partId);
        }
        break;
    case EMsg::EjectOccupant:
        if (size >= sizeof(EjectOccupantPacket))
        {
            const EjectOccupantPacket* pkt = reinterpret_cast<const EjectOccupantPacket*>(payload);
            AvatarProxy_OnEject(pkt->peerId);
        }
        break;
    case EMsg::BreachStart:
        if (size >= sizeof(BreachStartPacket))
        {
            const BreachStartPacket* pkt = reinterpret_cast<const BreachStartPacket*>(payload);
            BreachHud_Start(pkt->peerId, pkt->seed, pkt->gridW, pkt->gridH);
        }
        break;
    case EMsg::BreachInput:
        if (size >= sizeof(BreachInputPacket))
        {
            const BreachInputPacket* pkt = reinterpret_cast<const BreachInputPacket*>(payload);
            BreachHud_Input(pkt->peerId, pkt->index);
            if (Net_IsAuthoritative())
                CoopNet::BreachController_HandleInput(pkt->peerId, pkt->index);
        }
        break;
    case EMsg::BreachResult:
        if (size >= sizeof(BreachResultPacket))
        {
            const BreachResultPacket* pkt = reinterpret_cast<const BreachResultPacket*>(payload);
            Quickhack_BreachResult(pkt->peerId, pkt->daemonsMask);
        }
        break;
    case EMsg::ElevatorCall:
        if (size >= sizeof(ElevatorCallPacket))
        {
            const ElevatorCallPacket* pkt = reinterpret_cast<const ElevatorCallPacket*>(payload);
            if (Net_IsAuthoritative())
                CoopNet::ElevatorController_OnCall(pkt->peerId, pkt->elevatorId, pkt->floorIdx);
        }
        break;
    case EMsg::ElevatorArrive:
        if (size >= sizeof(ElevatorArrivePacket))
        {
            const ElevatorArrivePacket* pkt = reinterpret_cast<const ElevatorArrivePacket*>(payload);
            ElevatorSync_OnArrive(pkt->elevatorId, pkt->sectorHash, pkt->pos);
        }
        break;
    case EMsg::TeleportAck:
        if (size >= sizeof(TeleportAckPacket) && Net_IsAuthoritative())
        {
            const TeleportAckPacket* pkt = reinterpret_cast<const TeleportAckPacket*>(payload);
            CoopNet::ElevatorController_OnAck(this, pkt->elevatorId);
        }
        break;
    case EMsg::HoloCallStart:
        if (size >= sizeof(HoloCallPacket))
        {
            const HoloCallPacket* pkt = reinterpret_cast<const HoloCallPacket*>(payload);
            UIPauseAudit_OnHoloStart(pkt->peerId);
        }
        break;
    case EMsg::HoloCallEnd:
        if (size >= sizeof(HoloCallPacket))
        {
            const HoloCallPacket* pkt = reinterpret_cast<const HoloCallPacket*>(payload);
            UIPauseAudit_OnHoloEnd(pkt->peerId);
        }
        break;
    case EMsg::SpectateRequest:
        if (size >= sizeof(SpectatePacket) && Net_IsAuthoritative())
        {
            const SpectatePacket* pkt = reinterpret_cast<const SpectatePacket*>(payload);
            Net_SendSpectateGranted(pkt->peerId);
        }
        break;
    case EMsg::SpectateGranted:
        if (size >= sizeof(SpectatePacket))
        {
            const SpectatePacket* pkt = reinterpret_cast<const SpectatePacket*>(payload);
            SpectatorCam_Enter(pkt->peerId);
        }
        break;
    case EMsg::NatCandidate:
        if (size >= sizeof(NatCandidatePacket))
        {
            const NatCandidatePacket* pkt = reinterpret_cast<const NatCandidatePacket*>(payload);
            CoopNet::Nat_AddRemoteCandidate(pkt->sdp);
            CoopNet::Nat_PerformHandshake(this);
        }
        break;
    case EMsg::CineStart:
        if (size >= sizeof(CineStartPacket))
        {
            const CineStartPacket* pkt = reinterpret_cast<const CineStartPacket*>(payload);
            Cutscene_OnCineStart(pkt->sceneId, pkt->startTimeMs);
        }
        break;
    case EMsg::Viseme:
        if (size >= sizeof(VisemePacket))
        {
            const VisemePacket* pkt = reinterpret_cast<const VisemePacket*>(payload);
            Cutscene_OnViseme(pkt->npcId, pkt->visemeId, pkt->timeMs);
        }
        break;
    case EMsg::DialogChoice:
        if (size >= sizeof(DialogChoicePacket))
        {
            const DialogChoicePacket* pkt = reinterpret_cast<const DialogChoicePacket*>(payload);
            uint32_t sender = Net_IsAuthoritative() ? peerId : pkt->peerId;
            Cutscene_OnDialogChoice(sender, pkt->choiceIdx);
            if (Net_IsAuthoritative())
                Net_BroadcastDialogChoice(sender, pkt->choiceIdx);
        }
        break;
    case EMsg::Voice:
        if (size >= sizeof(VoicePacket))
        {
            const VoicePacket* pkt = reinterpret_cast<const VoicePacket*>(payload);
            if (Net_IsAuthoritative())
            {
                Net_BroadcastVoice(peerId, pkt->data, pkt->size, pkt->seq);
            }
            else
            {
                CoopVoice::PushPacket(pkt->seq, pkt->data, pkt->size);
            }
        }
        break;
    case EMsg::AdminCmd:
        if (size >= sizeof(AdminCmdPacket))
        {
            const AdminCmdPacket* pkt = reinterpret_cast<const AdminCmdPacket*>(payload);
            std::cout << "AdminCmd type=" << static_cast<int>(pkt->cmdType) << " param=" << pkt->param << std::endl;
        }
        break;
    case EMsg::TickRateChange:
        if (size >= sizeof(TickRateChangePacket))
        {
            const TickRateChangePacket* pkt = reinterpret_cast<const TickRateChangePacket*>(payload);
            SnapshotInterpolator_OnTickRateChange(pkt->tickMs);
        }
        break;
    case EMsg::CraftRequest:
        if (size >= sizeof(CraftRequestPacket) && Net_IsAuthoritative())
        {
            const CraftRequestPacket* pkt = reinterpret_cast<const CraftRequestPacket*>(payload);
            CoopNet::Inventory_HandleCraftRequest(this, pkt->recipeId);
        }
        break;
    case EMsg::AttachModRequest:
        if (size >= sizeof(AttachModRequestPacket) && Net_IsAuthoritative())
        {
            const AttachModRequestPacket* pkt = reinterpret_cast<const AttachModRequestPacket*>(payload);
            CoopNet::Inventory_HandleAttachRequest(this, pkt->itemId, pkt->slotIdx, pkt->attachmentId);
        }
        break;
    default:
        break;
    }
}

void Connection::Update(uint64_t nowMs)
{
    // Store ping timestamp; actual ping logic will come later.
    lastPingSent = nowMs;

    RawPacket pkt;
    while (m_incoming.Pop(pkt))
    {
        HandlePacket(pkt.hdr, pkt.data.data(), static_cast<uint16_t>(pkt.data.size()));
    }

    int16_t pcm[960];
    if (CoopVoice::DecodeFrame(pcm) > 0)
    {
        // PCM would be sent to audio output here
    }

    if (!sectorReady)
    {
        const uint64_t timeoutTicks = static_cast<uint64_t>(10000.f / CoopNet::kFixedDeltaMs);
        if (CoopNet::GameClock::GetCurrentTick() - lastSectorChangeTick > timeoutTicks)
        {
            std::cout << "SectorReady timeout" << std::endl;
            sectorReady = true;
        }
    }
    // At end of tick loop, send any batched score packets.
    CoopNet::FlushStats();
}

void Connection::EnqueuePacket(const RawPacket& pkt)
{
    m_incoming.Push(pkt);
    lastRecvTime = 0; // NT-2: track activity time
}

bool Connection::PopPacket(RawPacket& out)
{
    return m_incoming.Pop(out);
}

void Connection::Transition(ConnectionState next)
{
    if (state != next)
    {
        state = next;
        std::cout << "Connection state -> " << static_cast<int>(state) << std::endl;
    }
}

} // namespace CoopNet
