#include "Connection.hpp"
#include "../core/GameClock.hpp"
#include "../core/Hash.hpp"
#include "../core/SessionState.hpp"
#include "../runtime/GameModeManager.reds"
#include "../runtime/TileGameSync.reds"
#include "../server/BreachController.hpp"
#include "../server/NpcController.hpp"
#include "../server/QuestWatchdog.hpp"
#include "../server/StatusController.hpp"
#include "../server/TrafficController.hpp"
#include "../voice/VoiceDecoder.hpp"
#include "../plugin/PluginManager.hpp"
#include "../third_party/zstd/zstd.h"
#include <openssl/sha.h>
#include <Python.h>
#include "Net.hpp"
#include "StatBatch.hpp"
#include <RED4ext/RED4ext.hpp>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <iostream>

// Temporary proxies for script methods.
static void AvatarProxy_SpawnRemote(uint32_t peerId, bool isLocal, const CoopNet::TransformSnap& snap)
{
    RED4ext::ExecuteFunction("AvatarProxy", "SpawnRemote", nullptr, peerId, isLocal, &snap);
}

static void AvatarProxy_DespawnRemote(uint32_t peerId)
{
    RED4ext::ExecuteFunction("AvatarProxy", "DespawnRemote", nullptr, peerId);
}

static void Killfeed_Push(const char* msg)
{
    RED4ext::CString s(msg);
    RED4ext::ExecuteFunction("Killfeed", "Push", nullptr, &s);
    std::cout << "Killfeed: " << msg << std::endl;
}

static void Killfeed_Broadcast(const char* msg)
{
    if (CoopNet::Net_IsAuthoritative())
        CoopNet::Net_BroadcastKillfeed(msg);
    Killfeed_Push(msg);
}

static void ChatOverlay_Push(const char* msg)
{
    RED4ext::CString s(msg);
    RED4ext::ExecuteFunction("ChatOverlay", "PushGlobal", nullptr, &s);
}

namespace
{
    struct BundleBuf
    {
        std::vector<uint8_t> data;
        uint32_t expected{0};
    };

    std::unordered_map<uint16_t, BundleBuf> g_bundle;
    std::unordered_map<uint16_t, std::string> g_bundleSha;

    void HandleBundleComplete(uint16_t pluginId, const std::vector<uint8_t>& comp)
    {
        namespace fs = std::filesystem;
        std::vector<uint8_t> raw(5u * 1024u * 1024u);
        size_t size = ZSTD_decompress(raw.data(), raw.size(), comp.data(), comp.size());
        if (ZSTD_isError(size))
            return;
        raw.resize(size);
        unsigned char sha[SHA256_DIGEST_LENGTH];
        SHA256(comp.data(), comp.size(), sha);
        std::string s(reinterpret_cast<char*>(sha), SHA256_DIGEST_LENGTH);
        if (g_bundleSha[pluginId] == s)
            return;
        g_bundleSha[pluginId] = s;
        fs::path base = fs::path("runtime_cache") / "plugins" / std::to_string(pluginId);
        fs::create_directories(base);
        const uint8_t* p = raw.data();
        const uint8_t* end = raw.data() + raw.size();
        while (p + 2 <= end)
        {
            uint16_t pathLen;
            memcpy(&pathLen, p, 2);
            p += 2;
            if (p + pathLen > end)
                break;
            std::string rel(reinterpret_cast<const char*>(p), pathLen);
            p += pathLen;
            if (p + 4 > end)
                break;
            uint32_t len;
            memcpy(&len, p, 4);
            p += 4;
            if (p + len > end)
                break;
            fs::path out = base / rel;
            fs::create_directories(out.parent_path());
            std::ofstream f(out, std::ios::binary);
            f.write(reinterpret_cast<const char*>(p), len);
            p += len;
        }
        RED4ext::CString path(base.string().c_str());
        bool ro = true; // sandbox client scripts
        RED4ext::ExecuteFunction("ModSystem", "Mount", nullptr, &path, &ro);
        RED4ext::ExecuteFunction("ModSystem", "ReloadScriptsFrom", nullptr, &path);
    }
} // namespace

static void QuestSync_ApplyQuestStage(uint32_t hash, uint16_t stage)
{
    RED4ext::ExecuteFunction("QuestSync", "ApplyQuestStageByHash", nullptr, &hash, &stage);
}

static void QuestSync_ApplySceneTrigger(const char* id, bool start)
{
    std::cout << "SceneTrigger " << id << " start=" << start << std::endl;
}

static void DMScoreboard_OnScorePacket(uint32_t peerId, uint16_t k, uint16_t d)
{
    std::cout << "ScoreUpdate " << peerId << " " << k << "/" << d << std::endl;
}

static void DMScoreboard_OnMatchOver(uint32_t winner)
{
    std::cout << "MatchOver " << winner << std::endl;
}

static void StatHud_OnStats(uint32_t peerId, const CoopNet::NetStats& s)
{
    RED4ext::ExecuteFunction("StatHud", "OnNetStats", nullptr, peerId, &s);
}

static void NpcProxy_Spawn(const CoopNet::NpcSnap& snap)
{
    CoopNet::NpcController_ClientApplySnap(snap);
}

static void NpcProxy_Despawn(uint32_t npcId)
{
    CoopNet::NpcController_Despawn(npcId);
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

static void Inventory_OnReRollResult(const CoopNet::ItemSnap& snap)
{
    RED4ext::ExecuteFunction("Inventory", "OnReRollResult", nullptr, &snap);
}

static void Inventory_OnPurchaseResult(uint64_t itemId, uint64_t balance, bool success)
{
    RED4ext::ExecuteFunction("Inventory", "OnPurchaseResult", nullptr, &itemId, &balance, &success);
}

static void Apartments_OnPurchaseAck(uint32_t aptId, uint64_t balance, bool success)
{
    RED4ext::ExecuteFunction("Apartments", "OnPurchaseAck", nullptr, &aptId, &success, &balance);
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

static void AvatarProxy_OnEject(uint32_t peerId, const RED4ext::Vector3& vel)
{
    std::cout << "Eject occupant " << peerId << " vel=" << vel.X << "," << vel.Y << "," << vel.Z << std::endl;
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

struct QuickhackPacket
{
    uint32_t targetId;
    uint32_t hackId;
    uint16_t durationMs;
    uint16_t _pad;
};

struct HackInfoNative
{
    uint32_t targetId;
    uint32_t hackId;
    uint16_t durationMs;
    uint16_t startHealth;
};

static void QuickhackSync_Apply(const HackInfoNative& info)
{
    RED4ext::ExecuteFunction("QuickhackSync", "ApplyHack", nullptr, &info);
}

namespace
{
    static std::unordered_map<uint32_t, float> g_lastHackMs;
}

static void TileGameSync_Start(uint32_t phaseId, uint32_t seed)
{
    RED4ext::ExecuteFunction("TileGameSync", "OnStart", nullptr, &phaseId, &seed);
}

static void TileGameSync_Select(uint32_t peerId, uint8_t row, uint8_t col)
{
    RED4ext::ExecuteFunction("TileGameSync", "OnSelect", nullptr, &peerId, &row, &col);
}

static void TileGameSync_Progress(uint8_t percent)
{
    RED4ext::ExecuteFunction("TileGameSync", "OnProgress", nullptr, &percent);
}

static void VendorSync_OnStock(const CoopNet::VendorStockPacket& pkt)
{
    RED4ext::ExecuteFunction("VendorSync", "OnStock", nullptr, &pkt);
}

static void VendorSync_OnStockUpdate(const CoopNet::VendorStockUpdatePacket& pkt)
{
    RED4ext::ExecuteFunction("VendorSync", "OnStockUpdate", nullptr, &pkt);
}

static void VendorSync_OnRefresh(uint32_t vendorId)
{
    RED4ext::ExecuteFunction("VendorSync", "OnRefresh", nullptr, &vendorId);
}

static void HeatSync_Apply(uint8_t level)
{
    std::cout << "Heat level " << static_cast<int>(level) << std::endl;
}

static void WeatherSync_Apply(const CoopNet::WorldStatePacket& pkt)
{
    RED4ext::ExecuteFunction("WeatherSync", "ApplyWorldState", nullptr, &pkt);
}

static void GlobalEvent_OnPacket(const CoopNet::GlobalEventPacket& pkt)
{
    std::cout << "Event " << pkt.eventId << " phase=" << static_cast<int>(pkt.phase) << (pkt.start ? " start" : " stop")
              << std::endl;
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

static void GameModeManager_SetFriendlyFire(bool enable)
{
    std::cout << "FriendlyFire=" << (enable ? "true" : "false") << std::endl;
}

static void PoliceDispatch_OnCruiserSpawn(uint8_t idx, const uint32_t* seeds)
{
    RED4ext::ExecuteFunction("PoliceDispatch", "OnCruiserSpawn", nullptr, idx, seeds[0], seeds[1], seeds[2], seeds[3]);
}

static void NpcProxy_OnAIState(uint32_t npcId, uint8_t state)
{
    RED4ext::ExecuteFunction("NpcProxy", "OnAIState", nullptr, npcId, state);
}

static void PerkSync_OnUnlock(uint32_t peerId, uint32_t perkId, uint8_t rank)
{
    RED4ext::ExecuteFunction("PerkSync", "OnUnlock", nullptr, peerId, perkId, rank);
}

static void PerkSync_OnRespecAck(uint32_t peerId, uint16_t pts)
{
    RED4ext::ExecuteFunction("PerkSync", "OnRespecAck", nullptr, peerId, pts);
}

static void StatusEffectSync_OnApply(uint32_t targetId, uint8_t effectId, uint16_t durMs, uint8_t amp)
{
    RED4ext::ExecuteFunction("StatusEffectSync", "OnApply", nullptr, targetId, effectId, durMs, amp);
}

static void StatusEffectSync_OnTick(uint32_t targetId, int16_t delta)
{
    RED4ext::ExecuteFunction("StatusEffectSync", "OnTick", nullptr, targetId, delta);
}

static void SkillSync_OnXP(uint32_t peerId, uint16_t skillId, int16_t delta)
{
    RED4ext::ExecuteFunction("SkillSync", "OnXP", nullptr, peerId, skillId, delta);
}

static void TrafficSync_OnSeed(uint64_t hash, uint64_t seed)
{
    RED4ext::ExecuteFunction("TrafficSync", "OnSeed", nullptr, hash, seed);
}

static void TrafficSync_OnDespawn(uint32_t id)
{
    RED4ext::ExecuteFunction("TrafficSync", "OnDespawn", nullptr, id);
}

static void CrimeSpawner_OnEvent(const CoopNet::CrimeEventSpawnPacket& pkt)
{
    RED4ext::ExecuteFunction("CrimeSpawner", "OnEvent", nullptr, &pkt);
}

static void PanicSync_OnEvent(const CoopNet::PanicEventPacket& pkt)
{
    RED4ext::ExecuteFunction("PanicSync", "OnEvent", nullptr, &pkt);
}

static void AIHackSync_OnHack(uint32_t target, uint8_t effectId)
{
    RED4ext::ExecuteFunction("AIHackSync", "OnHack", nullptr, target, effectId);
}

static void BossPhaseSync_OnSwitch(uint32_t npcId, uint8_t phase)
{
    RED4ext::ExecuteFunction("BossPhaseSync", "OnSwitch", nullptr, npcId, phase);
}

static void PropSync_OnBreak(uint32_t id, uint32_t seed)
{
    RED4ext::ExecuteFunction("PropSync", "OnBreak", nullptr, id, seed);
}

static void PropSync_OnIgnite(uint32_t id, uint16_t delay)
{
    RED4ext::ExecuteFunction("PropSync", "OnIgnite", nullptr, id, delay);
}

static void CrowdCfgSync_OnApply(uint8_t density)
{
    RED4ext::ExecuteFunction("CrowdCfgSync", "OnApply", nullptr, density);
}

static void CrowdCfgSync_OnRestore()
{
    RED4ext::ExecuteFunction("CrowdCfgSync", "OnRestore", nullptr);
}

static void VoiceOverQueue_OnPlay(uint32_t lineId)
{
    RED4ext::ExecuteFunction("VoiceOverQueue", "OnPlay", nullptr, lineId);
}

static void FixerCallSync_OnStart(uint32_t id)
{
    RED4ext::ExecuteFunction("FixerCallSync", "OnStart", nullptr, id);
}

static void FixerCallSync_OnEnd(uint32_t id)
{
    RED4ext::ExecuteFunction("FixerCallSync", "OnEnd", nullptr, id);
}

static void GigSpawner_OnSpawn(uint32_t questId, uint32_t seed)
{
    RED4ext::ExecuteFunction("GigSpawner", "OnSpawn", nullptr, questId, seed);
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
    , rttHist{}
    , rttIndex(0)
{
    crypto_kx_keypair(pubKey.data(), privKey.data());
}

void Connection::SendSectorChange(uint64_t hash)
{
    SectorChangePacket pkt{0u, hash};
    Net_Send(this, EMsg::SectorChange, &pkt, sizeof(pkt));
    CoopNet::NpcController_OnPlayerEnterSector(peerId, hash);
    CoopNet::BillboardController_OnSectorLoad(peerId, hash);
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
    HelloPacket pkt{};
    memcpy(pkt.pub, pubKey.data(), crypto_kx_PUBLICKEYBYTES);
    Net_Send(this, EMsg::Hello, &pkt, sizeof(pkt));
}

void Connection::HandlePacket(const PacketHeader& hdr, const void* payload, uint16_t size)
{
    (void)payload;
    (void)size;
    switch (static_cast<EMsg>(hdr.type))
    {
    case EMsg::Hello:
        if (size >= sizeof(HelloPacket))
        {
            const HelloPacket* pkt = reinterpret_cast<const HelloPacket*>(payload);
            unsigned char sec[crypto_scalarmult_BYTES];
            crypto_scalarmult(sec, privKey.data(), pkt->pub);
            crypto_generichash(key.data(), key.size(), sec, sizeof(sec), nullptr, 0);
            hasKey = true;
            WelcomePacket ack{};
            memcpy(ack.pub, pubKey.data(), crypto_kx_PUBLICKEYBYTES);
            Net_Send(this, EMsg::Welcome, &ack, sizeof(ack));
        }
        break;
    case EMsg::Ping:
        if (size >= sizeof(PingPacket))
        {
            const PingPacket* pkt = reinterpret_cast<const PingPacket*>(payload);
            PongPacket pong{pkt->timeMs};
            Net_Send(this, EMsg::Pong, &pong, sizeof(pong));
        }
        break;
    case EMsg::Pong:
        if (size >= sizeof(PongPacket))
        {
            const PongPacket* pkt = reinterpret_cast<const PongPacket*>(payload);
            uint64_t now = GameClock::GetTimeMs();
            rttMs = static_cast<float>(now - pkt->timeMs);
            rttHist[rttIndex % 16] = rttMs;
            rttIndex = (rttIndex + 1) % 16;
        }
        break;
    case EMsg::Welcome:
        if (size >= sizeof(WelcomePacket))
        {
            const WelcomePacket* pkt = reinterpret_cast<const WelcomePacket*>(payload);
            unsigned char sec[crypto_scalarmult_BYTES];
            crypto_scalarmult(sec, privKey.data(), pkt->pub);
            crypto_generichash(key.data(), key.size(), sec, sizeof(sec), nullptr, 0);
            hasKey = true;
            if (state == ConnectionState::Handshaking)
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
        Killfeed_Broadcast("0 disconnected");
        CoopNet::VehicleController_RemovePeer(peerId);
        Transition(ConnectionState::Disconnected);
        CrowdCfgSync_OnRestore();
        CoopNet::SaveSessionState(CoopNet::SessionState_GetId());
        break;
    case EMsg::AvatarSpawn:
        if (size >= sizeof(AvatarSpawnPacket))
        {
            const AvatarSpawnPacket* pkt = reinterpret_cast<const AvatarSpawnPacket*>(payload);
            AvatarProxy_SpawnRemote(pkt->peerId, pkt->peerId == 0, pkt->snap);
            avatarPos = pkt->snap.pos;
            uint64_t hash = CoopNet::Fnv1a64Pos(avatarPos.X, avatarPos.Y);
            currentSector = hash;
            SectorChangePacket sp{pkt->peerId, hash};
            Net_Send(this, EMsg::SectorChange, &sp, sizeof(sp));
        }
        break;
    case EMsg::AvatarDespawn:
        if (size >= sizeof(AvatarDespawnPacket))
        {
            const AvatarDespawnPacket* pkt = reinterpret_cast<const AvatarDespawnPacket*>(payload);
            AvatarProxy_DespawnRemote(pkt->peerId);
        }
        Killfeed_Broadcast("0 disconnected");
        break;
    case EMsg::Chat:
        if (Net_IsAuthoritative())
        {
            if (CoopNet::GameClock::GetTimeMs() < muteUntilMs)
                break;
            if (size >= sizeof(ChatPacket))
            {
                const ChatPacket* pkt = reinterpret_cast<const ChatPacket*>(payload);
                if (CoopNet::PluginManager_HandleChat(peerId, pkt->msg, false))
                    break;
                ChatPacket out{peerId, {0}};
                std::strncpy(out.msg, pkt->msg, sizeof(out.msg) - 1);
                Net_Broadcast(EMsg::Chat, &out, sizeof(out));
            }
        }
        if (size >= sizeof(ChatPacket))
        {
            const ChatPacket* pkt = reinterpret_cast<const ChatPacket*>(payload);
            ChatOverlay_Push(pkt->msg);
            PyObject* d = Py_BuildValue("{s:I,s:s}", "peerId", peerId, "text", pkt->msg);
            CoopNet::PluginManager_DispatchEvent("OnChatMsg", d);
            Py_DECREF(d);
        }
        break;
    case EMsg::QuestStageP2P:
        if (size >= sizeof(QuestStageP2PPacket))
        {
            const QuestStageP2PPacket* pkt = reinterpret_cast<const QuestStageP2PPacket*>(payload);
            CoopNet::QuestWatchdog_Record(pkt->phaseId, pkt->questHash, pkt->stage);
            if (pkt->phaseId == peerId)
                QuestSync_ApplyQuestStage(pkt->questHash, pkt->stage);
        }
        break;
    case EMsg::QuestResyncRequest:
        if (Net_IsAuthoritative())
        {
            QuestFullSyncPacket pkt{};
            CoopNet::QuestWatchdog_BuildFullSync(peerId, pkt);
            Net_SendQuestFullSync(this, pkt);
        }
        break;
    case EMsg::QuestFullSync:
        if (size >= sizeof(QuestFullSyncPacket))
        {
            const QuestFullSyncPacket* pkt = reinterpret_cast<const QuestFullSyncPacket*>(payload);
            RED4ext::ExecuteFunction("QuestSync", "ApplyFullSync", nullptr, pkt);
        }
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
    case EMsg::NpcSpawnCruiser:
        if (size >= sizeof(NpcSpawnCruiserPacket))
        {
            const NpcSpawnCruiserPacket* pkt = reinterpret_cast<const NpcSpawnCruiserPacket*>(payload);
            PoliceDispatch_OnCruiserSpawn(pkt->waveIdx, pkt->npcSeeds);
        }
        break;
    case EMsg::NpcState:
        if (size >= sizeof(NpcStatePacket))
        {
            const NpcStatePacket* pkt = reinterpret_cast<const NpcStatePacket*>(payload);
            NpcProxy_OnAIState(pkt->npcId, pkt->aiState);
        }
        break;
    case EMsg::CrimeEventSpawn:
        if (size >= sizeof(CrimeEventSpawnPacket))
        {
            const CrimeEventSpawnPacket* pkt = reinterpret_cast<const CrimeEventSpawnPacket*>(payload);
            CrimeSpawner_OnEvent(*pkt);
        }
        break;
    case EMsg::PanicEvent:
        if (size >= sizeof(PanicEventPacket))
        {
            const PanicEventPacket* pkt = reinterpret_cast<const PanicEventPacket*>(payload);
            PanicSync_OnEvent(*pkt);
        }
        break;
    case EMsg::BossPhase:
        if (size >= sizeof(BossPhasePacket))
        {
            const BossPhasePacket* pkt = reinterpret_cast<const BossPhasePacket*>(payload);
            BossPhaseSync_OnSwitch(pkt->npcId, pkt->phaseIdx);
        }
        break;
    case EMsg::GigSpawn:
        if (size >= sizeof(GigSpawnPacket))
        {
            const GigSpawnPacket* pkt = reinterpret_cast<const GigSpawnPacket*>(payload);
            GigSpawner_OnSpawn(pkt->questId, pkt->seed);
        }
        break;
    case EMsg::CyberEquip:
        if (size >= sizeof(CyberEquipPacket))
        {
            const CyberEquipPacket* pkt = reinterpret_cast<const CyberEquipPacket*>(payload);
            CyberwareSync_OnEquip(pkt->peerId, pkt->slotId, pkt->snap);
        }
        break;
    case EMsg::SlowMoStart:
        if (size >= sizeof(SlowMoStartPacket))
        {
            const SlowMoStartPacket* pkt = reinterpret_cast<const SlowMoStartPacket*>(payload);
            CyberwareSync_OnSlowMo(pkt->peerId, pkt->factor, pkt->durationMs);
        }
        break;
    case EMsg::PerkUnlock:
        if (size >= sizeof(PerkUnlockPacket))
        {
            const PerkUnlockPacket* pkt = reinterpret_cast<const PerkUnlockPacket*>(payload);
            if (Net_IsAuthoritative())
                CoopNet::PerkController_HandleUnlock(this, pkt->perkId, pkt->rank);
            else
                PerkSync_OnUnlock(pkt->peerId, pkt->perkId, pkt->rank);
        }
        break;
    case EMsg::PerkRespecRequest:
        if (size >= sizeof(PerkRespecRequestPacket) && Net_IsAuthoritative())
        {
            CoopNet::PerkController_HandleRespec(this);
        }
        break;
    case EMsg::PerkRespecAck:
        if (size >= sizeof(PerkRespecAckPacket) && !Net_IsAuthoritative())
        {
            const PerkRespecAckPacket* pkt = reinterpret_cast<const PerkRespecAckPacket*>(payload);
            PerkSync_OnRespecAck(pkt->peerId, pkt->newPoints);
        }
        break;
    case EMsg::SkillXP:
        if (size >= sizeof(SkillXPPacket))
        {
            const SkillXPPacket* pkt = reinterpret_cast<const SkillXPPacket*>(payload);
            if (Net_IsAuthoritative())
                CoopNet::SkillController_HandleXP(this, pkt->skillId, pkt->deltaXP);
            else
                SkillSync_OnXP(pkt->peerId, pkt->skillId, pkt->deltaXP);
        }
        break;
    case EMsg::StatusApply:
        if (size >= sizeof(StatusApplyPacket))
        {
            const StatusApplyPacket* pkt = reinterpret_cast<const StatusApplyPacket*>(payload);
            if (Net_IsAuthoritative())
                CoopNet::StatusController_OnApply(this, *pkt);
            else
                StatusEffectSync_OnApply(pkt->targetId, pkt->effectId, pkt->durMs, pkt->amp);
        }
        break;
    case EMsg::StatusTick:
        if (size >= sizeof(StatusTickPacket) && !Net_IsAuthoritative())
        {
            const StatusTickPacket* pkt = reinterpret_cast<const StatusTickPacket*>(payload);
            StatusEffectSync_OnTick(pkt->targetId, pkt->hpDelta);
        }
        break;
    case EMsg::AIHack:
        if (size >= sizeof(AIHackPacket))
        {
            const AIHackPacket* pkt = reinterpret_cast<const AIHackPacket*>(payload);
            AIHackSync_OnHack(pkt->targetId, pkt->effectId);
        }
        break;
    case EMsg::TrafficSeed:
        if (size >= sizeof(TrafficSeedPacket) && !Net_IsAuthoritative())
        {
            const TrafficSeedPacket* pkt = reinterpret_cast<const TrafficSeedPacket*>(payload);
            TrafficSync_OnSeed(pkt->sectorHash, pkt->seed64);
        }
        break;
    case EMsg::TrafficDespawn:
        if (size >= sizeof(TrafficDespawnPacket) && !Net_IsAuthoritative())
        {
            const TrafficDespawnPacket* pkt = reinterpret_cast<const TrafficDespawnPacket*>(payload);
            TrafficSync_OnDespawn(pkt->vehId);
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
    case EMsg::MatchOver:
        if (size >= sizeof(MatchOverPacket))
        {
            const MatchOverPacket* pkt = reinterpret_cast<const MatchOverPacket*>(payload);
            DMScoreboard_OnMatchOver(pkt->winnerId);
        }
        break;
    case EMsg::Killfeed:
        if (size >= sizeof(KillfeedPacket))
        {
            const KillfeedPacket* pkt = reinterpret_cast<const KillfeedPacket*>(payload);
            Killfeed_Push(pkt->msg);
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
    case EMsg::ReRollResult:
        if (size >= sizeof(ReRollResultPacket))
        {
            const ReRollResultPacket* pkt = reinterpret_cast<const ReRollResultPacket*>(payload);
            Inventory_OnReRollResult(pkt->snap);
        }
        break;
    case EMsg::HeatSync:
        if (size >= sizeof(HeatPacket))
        {
            const HeatPacket* pkt = reinterpret_cast<const HeatPacket*>(payload);
            HeatSync_Apply(pkt->level);
        }
        break;
    case EMsg::WorldState:
        if (size >= sizeof(WorldStatePacket))
        {
            const WorldStatePacket* pkt = reinterpret_cast<const WorldStatePacket*>(payload);
            WeatherSync_Apply(*pkt);
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
    case EMsg::PropBreak:
        if (size >= sizeof(PropBreakPacket))
        {
            const PropBreakPacket* pkt = reinterpret_cast<const PropBreakPacket*>(payload);
            if (Net_IsAuthoritative())
                Net_BroadcastPropBreak(pkt->entityId, pkt->seed);
            else
                PropSync_OnBreak(pkt->entityId, pkt->seed);
        }
        break;
    case EMsg::PropIgnite:
        if (size >= sizeof(PropIgnitePacket))
        {
            const PropIgnitePacket* pkt = reinterpret_cast<const PropIgnitePacket*>(payload);
            if (!Net_IsAuthoritative())
                PropSync_OnIgnite(pkt->entityId, pkt->delayMs);
        }
        break;
    case EMsg::VehicleSpawn:
        if (size >= sizeof(VehicleSpawnPacket))
        {
            const VehicleSpawnPacket* pkt = reinterpret_cast<const VehicleSpawnPacket*>(payload);
            VehicleProxy_Spawn(pkt->vehicleId, &pkt->transform, pkt->phaseId);
        }
        break;
    case EMsg::SeatAssign:
        if (size >= sizeof(SeatAssignPacket))
        {
            const SeatAssignPacket* pkt = reinterpret_cast<const SeatAssignPacket*>(payload);
            VehicleProxy_EnterSeat(pkt->peerId, pkt->seatIdx);
        }
        break;
    case EMsg::VehicleHit:
        if (size >= sizeof(VehicleHitPacket))
        {
            const VehicleHitPacket* pkt = reinterpret_cast<const VehicleHitPacket*>(payload);
            VehicleProxy_ApplyDamage(pkt->vehicleId, pkt->dmg, pkt->side != 0);
        }
        break;
    case EMsg::VehicleHitHighSpeed:
        if (size >= sizeof(VehicleHitHighSpeedPacket))
        {
            const VehicleHitHighSpeedPacket* pkt = reinterpret_cast<const VehicleHitHighSpeedPacket*>(payload);
            VehicleProxy_ApplyDamage(pkt->vehA, 0, false); // damage handled server-side
            VehicleProxy_ApplyDamage(pkt->vehB, 0, false);
        }
        break;
    case EMsg::SeatRequest:
        if (size >= sizeof(SeatRequestPacket) && Net_IsAuthoritative())
        {
            const SeatRequestPacket* pkt = reinterpret_cast<const SeatRequestPacket*>(payload);
            CoopNet::VehicleController_HandleSeatRequest(this, pkt->vehicleId, pkt->seatIdx);
        }
        break;
    case EMsg::VehicleSummonRequest:
        if (size >= sizeof(VehicleSummonRequestPacket) && Net_IsAuthoritative())
        {
            const VehicleSummonRequestPacket* pkt = reinterpret_cast<const VehicleSummonRequestPacket*>(payload);
            CoopNet::VehicleController_HandleSummon(this, pkt->vehId, pkt->pos);
        }
        break;
    case EMsg::VehicleSummon:
        if (size >= sizeof(VehicleSummonPacket))
        {
            const VehicleSummonPacket* pkt = reinterpret_cast<const VehicleSummonPacket*>(payload);
            VehicleProxy_Spawn(pkt->vehId, &pkt->pos, 0u);
        }
        break;
    case EMsg::VehicleTowRequest:
        if (size >= sizeof(VehicleTowRequestPacket) && Net_IsAuthoritative())
        {
            const VehicleTowRequestPacket* pkt = reinterpret_cast<const VehicleTowRequestPacket*>(payload);
            CoopNet::VehicleController_HandleTowRequest(this, pkt->pos);
        }
        break;
    case EMsg::VehicleTowAck:
        if (size >= sizeof(VehicleTowAckPacket) && !Net_IsAuthoritative())
        {
            const VehicleTowAckPacket* pkt = reinterpret_cast<const VehicleTowAckPacket*>(payload);
            ChatOverlay_Push(pkt->ok ? "[Tow] Car returned" : "[Tow] Failed");
        }
        break;
    case EMsg::Appearance:
        if (size >= sizeof(AppearancePacket))
        {
            const AppearancePacket* pkt = reinterpret_cast<const AppearancePacket*>(payload);
            AvatarProxy_OnAppearance(pkt->peerId, pkt->meshId, pkt->tintId);
        }
        break;
    case EMsg::EjectOccupant:
        if (size >= sizeof(EjectOccupantPacket))
        {
            const EjectOccupantPacket* pkt = reinterpret_cast<const EjectOccupantPacket*>(payload);
            AvatarProxy_OnEject(pkt->peerId, pkt->velocity);
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
    case EMsg::SnapshotAck:
        if (Net_IsAuthoritative())
        {
            auto blob = CoopNet::BuildMarkerBlob();
            Net_SendWorldMarkers(this, blob);
            for (uint32_t id : CoopNet::QuestWatchdog_ListPhases())
            {
                auto pb = CoopNet::BuildPhaseBundle(id);
                Net_SendPhaseBundle(this, id, pb);
            }
        }
        break;
    case EMsg::HoloCallStart:
        if (size >= sizeof(HolocallStartPacket))
        {
            const HolocallStartPacket* pkt = reinterpret_cast<const HolocallStartPacket*>(payload);
            RED4ext::ExecuteFunction("HoloCallSync", "OnStart", nullptr, pkt);
        }
        break;
    case EMsg::HoloCallEnd:
        if (size >= sizeof(HolocallEndPacket))
        {
            const HolocallEndPacket* pkt = reinterpret_cast<const HolocallEndPacket*>(payload);
            RED4ext::ExecuteFunction("HoloCallSync", "OnEnd", nullptr, &pkt->callId);
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
            if (pkt->solo && pkt->phaseId != peerId)
            {
                BusyOverlay_Show("Teammate busy");
            }
            else
            {
                Cutscene_OnCineStart(pkt->sceneId, pkt->startTimeMs);
            }
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
                if (!voiceMuted)
                    Net_BroadcastVoice(peerId, pkt->data, pkt->size, pkt->seq);
            }
            else
            {
                if (!voiceMuted)
                    CoopVoice::PushPacket(pkt->seq, pkt->data, pkt->size);
                voiceRecv++;
            }
        }
        break;
    case EMsg::VOPlay:
        if (size >= sizeof(VOPlayPacket))
        {
            const VOPlayPacket* pkt = reinterpret_cast<const VOPlayPacket*>(payload);
            if (!Net_IsAuthoritative())
                VoiceOverQueue_OnPlay(pkt->lineId);
        }
        break;
    case EMsg::FixerCallStart:
        if (size >= sizeof(FixerCallPacket))
        {
            const FixerCallPacket* pkt = reinterpret_cast<const FixerCallPacket*>(payload);
            FixerCallSync_OnStart(pkt->fixerId);
        }
        break;
    case EMsg::FixerCallEnd:
        if (size >= sizeof(FixerCallPacket))
        {
            const FixerCallPacket* pkt = reinterpret_cast<const FixerCallPacket*>(payload);
            FixerCallSync_OnEnd(pkt->fixerId);
        }
        break;
    case EMsg::GlobalEvent:
        if (size >= sizeof(GlobalEventPacket))
        {
            const GlobalEventPacket* pkt = reinterpret_cast<const GlobalEventPacket*>(payload);
            GlobalEvent_OnPacket(*pkt);
        }
        break;
    case EMsg::CrowdSeed:
        if (size >= sizeof(CrowdSeedPacket))
        {
            const CrowdSeedPacket* pkt = reinterpret_cast<const CrowdSeedPacket*>(payload);
            NpcController_ApplyCrowdSeed(pkt->sectorHash, pkt->seed);
        }
        break;
    case EMsg::VendorStock:
        if (size >= sizeof(VendorStockPacket))
        {
            const VendorStockPacket* pkt = reinterpret_cast<const VendorStockPacket*>(payload);
            VendorSync_OnStock(*pkt);
        }
        break;
    case EMsg::VendorStockUpdate:
        if (size >= sizeof(VendorStockUpdatePacket))
        {
            const VendorStockUpdatePacket* pkt = reinterpret_cast<const VendorStockUpdatePacket*>(payload);
            VendorSync_OnStockUpdate(*pkt);
        }
        break;
    case EMsg::VendorRefresh:
        if (size >= sizeof(VendorRefreshPacket))
        {
            const VendorRefreshPacket* pkt = reinterpret_cast<const VendorRefreshPacket*>(payload);
            VendorSync_OnRefresh(pkt->vendorId);
        }
        break;
    case EMsg::PingOutline:
        if (size >= sizeof(PingOutlinePacket))
        {
            const PingOutlinePacket* pkt = reinterpret_cast<const PingOutlinePacket*>(payload);
            array<Uint32> ids;
            let cnt : Int32 = Cast<Int32>(pkt->count);
            let i : Int32 = 0;
            while
                i < cnt&& i < 32
                {
                    ids.PushBack(pkt->entityIds[i]);
                    i += 1;
                }
            QuickhackSync_OnPingOutline(pkt->peerId, pkt->durationMs, ids);
        }
        break;
    case EMsg::WorldMarkers:
        if (size >= sizeof(uint16_t) && !Net_IsAuthoritative())
        {
            const WorldMarkersPacket* pkt = reinterpret_cast<const WorldMarkersPacket*>(payload);
            if (size >= sizeof(uint16_t) + pkt->blobBytes)
            {
                CoopNet::ApplyMarkerBlob(pkt->zstdBlob, pkt->blobBytes);
                std::cout << "[HotJoin] markers ready" << std::endl;
            }
        }
        break;
    case EMsg::AdminCmd:
        if (size >= sizeof(AdminCmdPacket))
        {
            const AdminCmdPacket* pkt = reinterpret_cast<const AdminCmdPacket*>(payload);
            std::cout << "AdminCmd type=" << static_cast<int>(pkt->cmdType) << " param=" << pkt->param << std::endl;
            if (pkt->cmdType == static_cast<uint8_t>(AdminCmdType::Mute))
            {
                voiceMuted = pkt->param != 0;
                RED4ext::ExecuteFunction("MicIcon", "SetMuted", nullptr, voiceMuted);
            }
        }
        break;
    case EMsg::TickRateChange:
        if (size >= sizeof(TickRateChangePacket))
        {
            const TickRateChangePacket* pkt = reinterpret_cast<const TickRateChangePacket*>(payload);
            SnapshotInterpolator_OnTickRateChange(pkt->tickMs);
        }
        break;
    case EMsg::RuleChange:
        if (size >= sizeof(RuleChangePacket))
        {
            const RuleChangePacket* pkt = reinterpret_cast<const RuleChangePacket*>(payload);
            GameModeManager_SetFriendlyFire(pkt->friendlyFire != 0u);
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
    case EMsg::ReRollRequest:
        if (size >= sizeof(ReRollRequestPacket) && Net_IsAuthoritative())
        {
            const ReRollRequestPacket* pkt = reinterpret_cast<const ReRollRequestPacket*>(payload);
            CoopNet::Inventory_HandleReRollRequest(this, pkt->itemId, pkt->seed);
        }
        break;
    case EMsg::RipperInstallRequest:
        if (size >= sizeof(RipperInstallRequestPacket) && Net_IsAuthoritative())
        {
            const RipperInstallRequestPacket* pkt = reinterpret_cast<const RipperInstallRequestPacket*>(payload);
            Net_BroadcastCineStart(CoopNet::Fnv1a32("ripper_chair"), 0u, peerId, true);
        }
        break;
    case EMsg::TileGameStart:
        if (size >= sizeof(TileGameStartPacket))
        {
            const TileGameStartPacket* pkt = reinterpret_cast<const TileGameStartPacket*>(payload);
            TileGameSync_OnStart(pkt->phaseId, pkt->seed);
        }
        break;
    case EMsg::TileSelect:
        if (size >= sizeof(TileSelectPacket))
        {
            const TileSelectPacket* pkt = reinterpret_cast<const TileSelectPacket*>(payload);
            TileGameSync_OnSelect(pkt->peerId, pkt->row, pkt->col);
            if (Net_IsAuthoritative())
                CoopNet::ShardController_HandleSelect(pkt->peerId, pkt->row, pkt->col);
        }
        break;
    case EMsg::ShardProgress:
        if (size >= sizeof(ShardProgressPacket))
        {
            const ShardProgressPacket* pkt = reinterpret_cast<const ShardProgressPacket*>(payload);
            TileGameSync_OnProgress(pkt->percent);
        }
        break;
    case EMsg::TradeInit:
        if (size >= sizeof(TradeInitPacket))
        {
            const TradeInitPacket* pkt = reinterpret_cast<const TradeInitPacket*>(payload);
            if (Net_IsAuthoritative())
                CoopNet::TradeController_Start(pkt->fromId, pkt->toId);
            else
                TradeWindow_OnInit(pkt->fromId);
        }
        break;
    case EMsg::TradeOffer:
        if (size >= sizeof(TradeOfferPacket))
        {
            const TradeOfferPacket* pkt = reinterpret_cast<const TradeOfferPacket*>(payload);
            if (Net_IsAuthoritative())
                CoopNet::TradeController_HandleOffer(this, *pkt);
            else
                TradeWindow_OnOffer(pkt->fromId, pkt->items, pkt->count, pkt->eddies);
        }
        break;
    case EMsg::TradeAccept:
        if (size >= sizeof(TradeAcceptPacket))
        {
            const TradeAcceptPacket* pkt = reinterpret_cast<const TradeAcceptPacket*>(payload);
            if (Net_IsAuthoritative())
                CoopNet::TradeController_HandleAccept(this, pkt->peerId, pkt->accept != 0);
            else
                TradeWindow_OnAccept(pkt->peerId, pkt->accept != 0);
        }
        break;
    case EMsg::TradeFinalize:
        if (size >= sizeof(TradeFinalizePacket))
        {
            const TradeFinalizePacket* pkt = reinterpret_cast<const TradeFinalizePacket*>(payload);
            TradeWindow_OnFinalize(pkt->success != 0);
        }
        break;
    case EMsg::PurchaseRequest:
        if (size >= sizeof(PurchaseRequestPacket) && Net_IsAuthoritative())
        {
            const PurchaseRequestPacket* pkt = reinterpret_cast<const PurchaseRequestPacket*>(payload);
            CoopNet::VendorController_HandlePurchase(this, pkt->vendorId, pkt->itemId, pkt->nonce);
        }
        break;
    case EMsg::DealerBuy:
        if (size >= sizeof(DealerBuyPacket) && Net_IsAuthoritative())
        {
            const DealerBuyPacket* pkt = reinterpret_cast<const DealerBuyPacket*>(payload);
            CoopNet::DealerController_HandleBuy(this, pkt->vehicleTpl, pkt->price);
        }
        break;
    case EMsg::AptPurchase:
        if (size >= sizeof(AptPurchasePacket) && Net_IsAuthoritative())
        {
            const AptPurchasePacket* pkt = reinterpret_cast<const AptPurchasePacket*>(payload);
            CoopNet::ApartmentController_HandlePurchase(this, pkt->aptId);
        }
        break;
    case EMsg::AptEnterReq:
        if (size >= sizeof(AptEnterReqPacket) && Net_IsAuthoritative())
        {
            const AptEnterReqPacket* pkt = reinterpret_cast<const AptEnterReqPacket*>(payload);
            CoopNet::ApartmentController_HandleEnter(this, pkt->aptId, pkt->ownerPhaseId);
        }
        break;
    case EMsg::AptPermChange:
        if (size >= sizeof(AptPermChangePacket))
        {
            const AptPermChangePacket* pkt = reinterpret_cast<const AptPermChangePacket*>(payload);
            if (Net_IsAuthoritative())
            {
                CoopNet::ApartmentController_HandlePermChange(this, pkt->aptId, pkt->targetPeerId, pkt->allow != 0);
            }
        }
        break;
    case EMsg::AptPurchaseAck:
        if (size >= sizeof(AptPurchaseAckPacket) && !Net_IsAuthoritative())
        {
            const AptPurchaseAckPacket* pkt = reinterpret_cast<const AptPurchaseAckPacket*>(payload);
            Apartments_OnPurchaseAck(pkt->aptId, pkt->balance, pkt->success != 0);
        }
        break;
    case EMsg::AptEnterAck:
        if (size >= sizeof(AptEnterAckPacket) && !Net_IsAuthoritative())
        {
            const AptEnterAckPacket* pkt = reinterpret_cast<const AptEnterAckPacket*>(payload);
            Apartments_OnEnterAck(pkt->allow != 0, pkt->phaseId, pkt->interiorSeed);
        }
        break;
    case EMsg::VehicleUnlock:
        if (size >= sizeof(VehicleUnlockPacket) && !Net_IsAuthoritative())
        {
            const VehicleUnlockPacket* pkt = reinterpret_cast<const VehicleUnlockPacket*>(payload);
            VehicleUnlockSync_OnUnlock(pkt->peerId, pkt->vehicleTpl);
        }
        break;
    case EMsg::WeaponInspectStart:
        if (size >= sizeof(WeaponInspectPacket))
        {
            const WeaponInspectPacket* pkt = reinterpret_cast<const WeaponInspectPacket*>(payload);
            WeaponSync_OnInspect(pkt->peerId, pkt->animId);
        }
        break;
    case EMsg::FinisherStart:
        if (size >= sizeof(FinisherStartPacket))
        {
            const FinisherStartPacket* pkt = reinterpret_cast<const FinisherStartPacket*>(payload);
            WeaponSync_OnFinisherStart(pkt->actorId, pkt->victimId, pkt->finisherType);
        }
        break;
    case EMsg::FinisherEnd:
        if (size >= sizeof(FinisherEndPacket))
        {
            const FinisherEndPacket* pkt = reinterpret_cast<const FinisherEndPacket*>(payload);
            WeaponSync_OnFinisherEnd(pkt->actorId);
        }
        break;
    case EMsg::SlowMoFinisher:
        if (size >= sizeof(SlowMoFinisherPacket))
        {
            const SlowMoFinisherPacket* pkt = reinterpret_cast<const SlowMoFinisherPacket*>(payload);
            if (Net_IsAuthoritative())
                Net_BroadcastSlowMoFinisher(pkt->peerId, pkt->targetId, pkt->durationMs);
            else
                RED4ext::ExecuteFunction("SlowMoFinisherSync", "OnStart", nullptr, pkt);
        }
        break;
    case EMsg::TextureBiasChange:
        if (size >= sizeof(TextureBiasPacket))
        {
            const TextureBiasPacket* pkt = reinterpret_cast<const TextureBiasPacket*>(payload);
            TextureBiasSync_OnChange(pkt->bias);
        }
        break;
    case EMsg::CrowdCfg:
        if (size >= sizeof(CrowdCfgPacket))
        {
            const CrowdCfgPacket* pkt = reinterpret_cast<const CrowdCfgPacket*>(payload);
            CrowdCfgSync_OnApply(pkt->density);
        }
        break;
    case EMsg::CriticalVoteStart:
        if (size >= sizeof(CriticalVoteStartPacket))
        {
            const CriticalVoteStartPacket* pkt = reinterpret_cast<const CriticalVoteStartPacket*>(payload);
            std::cout << "[Vote] critical quest " << pkt->questHash << std::endl;
        }
        break;
    case EMsg::CriticalVoteCast:
        if (size >= sizeof(CriticalVoteCastPacket) && Net_IsAuthoritative())
        {
            const CriticalVoteCastPacket* pkt = reinterpret_cast<const CriticalVoteCastPacket*>(payload);
            CoopNet::QuestWatchdog_HandleVote(peerId, pkt->yes != 0);
        }
        break;
    case EMsg::EndingVoteStart:
        if (size >= sizeof(EndingVoteStartPacket))
        {
            const EndingVoteStartPacket* pkt = reinterpret_cast<const EndingVoteStartPacket*>(payload);
            std::cout << "[Vote] ending triggered" << std::endl;
        }
        break;
    case EMsg::EndingVoteCast:
        if (size >= sizeof(EndingVoteCastPacket) && Net_IsAuthoritative())
        {
            const EndingVoteCastPacket* pkt = reinterpret_cast<const EndingVoteCastPacket*>(payload);
            CoopNet::QuestWatchdog_HandleEndingVote(peerId, pkt->yes != 0);
        }
        break;
    case EMsg::PhaseBundle:
        if (size >= sizeof(PhaseBundlePacket) && !Net_IsAuthoritative())
        {
            const PhaseBundlePacket* pkt = reinterpret_cast<const PhaseBundlePacket*>(payload);
            if (size >= sizeof(PhaseBundlePacket) + pkt->blobBytes)
                CoopNet::ApplyPhaseBundle(pkt->phaseId, pkt->zstdBlob, pkt->blobBytes);
        }
        break;
    case EMsg::LootRoll:
        if (size >= sizeof(LootRollPacket) && !Net_IsAuthoritative())
        {
            const LootRollPacket* pkt = reinterpret_cast<const LootRollPacket*>(payload);
            LootAuthority_OnLootRoll(pkt->containerId, pkt->seed);
        }
        break;
    case EMsg::PurchaseResult:
        if (size >= sizeof(PurchaseResultPacket) && !Net_IsAuthoritative())
        {
            const PurchaseResultPacket* pkt = reinterpret_cast<const PurchaseResultPacket*>(payload);
            Inventory_OnPurchaseResult(pkt->itemId, pkt->balance, pkt->success != 0);
        }
        break;
    case EMsg::Emote:
        if (size >= sizeof(EmotePacket))
        {
            const EmotePacket* pkt = reinterpret_cast<const EmotePacket*>(payload);
            EmoteSync_Play(pkt->peerId, pkt->emoteId);
        }
        break;
    case EMsg::CrowdChatterStart:
        if (size >= sizeof(CrowdChatterStartPacket))
        {
            const CrowdChatterStartPacket* pkt = reinterpret_cast<const CrowdChatterStartPacket*>(payload);
            CrowdChatterSync_OnStart(pkt->npcA, pkt->npcB, pkt->lineId, pkt->seed);
        }
        break;
    case EMsg::CrowdChatterEnd:
        if (size >= sizeof(CrowdChatterEndPacket))
        {
            const CrowdChatterEndPacket* pkt = reinterpret_cast<const CrowdChatterEndPacket*>(payload);
            CrowdChatterSync_OnEnd(pkt->convId);
        }
        break;
    case EMsg::HoloSeed:
        if (size >= sizeof(HoloSeedPacket))
        {
            const HoloSeedPacket* pkt = reinterpret_cast<const HoloSeedPacket*>(payload);
            BillboardSync_OnSeed(pkt->sectorHash, pkt->seed64);
        }
        break;
    case EMsg::HoloNextAd:
        if (size >= sizeof(HoloNextAdPacket))
        {
            const HoloNextAdPacket* pkt = reinterpret_cast<const HoloNextAdPacket*>(payload);
            BillboardSync_OnNextAd(pkt->sectorHash, pkt->adId);
        }
        break;
    case EMsg::DoorBreachStart:
        if (size >= sizeof(DoorBreachStartPacket))
        {
            const DoorBreachStartPacket* pkt = reinterpret_cast<const DoorBreachStartPacket*>(payload);
            DoorBreachSync_OnStart(pkt->doorId, pkt->phaseId, pkt->seed);
        }
        break;
    case EMsg::DoorBreachTick:
        if (size >= sizeof(DoorBreachTickPacket))
        {
            const DoorBreachTickPacket* pkt = reinterpret_cast<const DoorBreachTickPacket*>(payload);
            DoorBreachSync_OnTick(pkt->doorId, pkt->percent);
        }
        break;
    case EMsg::DoorBreachSuccess:
        if (size >= sizeof(DoorBreachSuccessPacket))
        {
            const DoorBreachSuccessPacket* pkt = reinterpret_cast<const DoorBreachSuccessPacket*>(payload);
            DoorBreachSync_OnSuccess(pkt->doorId);
        }
        break;
    case EMsg::DoorBreachAbort:
        if (size >= sizeof(DoorBreachAbortPacket))
        {
            const DoorBreachAbortPacket* pkt = reinterpret_cast<const DoorBreachAbortPacket*>(payload);
            DoorBreachSync_OnAbort(pkt->doorId);
        }
        break;
    case EMsg::HTableOpen:
        if (size >= sizeof(HTableOpenPacket))
        {
            const HTableOpenPacket* pkt = reinterpret_cast<const HTableOpenPacket*>(payload);
            RED4ext::ExecuteFunction("HoloTableSync", "OnOpen", nullptr, &pkt->sceneId);
        }
        break;
    case EMsg::HTableScrub:
        if (size >= sizeof(HTableScrubPacket))
        {
            const HTableScrubPacket* pkt = reinterpret_cast<const HTableScrubPacket*>(payload);
            RED4ext::ExecuteFunction("HoloTableSync", "OnScrub", nullptr, &pkt->timestampMs);
        }
        break;
    case EMsg::QuestGadgetFire:
        if (size >= sizeof(QuestGadgetFirePacket))
        {
            const QuestGadgetFirePacket* pkt = reinterpret_cast<const QuestGadgetFirePacket*>(payload);
            if (Net_IsAuthoritative())
                CoopNet::QuestGadget_HandleFire(this, *pkt);
            else
                RED4ext::ExecuteFunction("QuestGadgetSync", "OnFire", nullptr, pkt);
        }
        break;
    case EMsg::ItemGrab:
        if (size >= sizeof(ItemGrabPacket))
        {
            const ItemGrabPacket* pkt = reinterpret_cast<const ItemGrabPacket*>(payload);
            if (Net_IsAuthoritative())
                Net_BroadcastItemGrab(pkt->peerId, pkt->itemId);
            else
                RED4ext::ExecuteFunction("ItemGrabSync", "OnGrab", nullptr, pkt);
        }
        break;
    case EMsg::ItemDrop:
        if (size >= sizeof(ItemDropPacket))
        {
            const ItemDropPacket* pkt = reinterpret_cast<const ItemDropPacket*>(payload);
            if (Net_IsAuthoritative())
                Net_BroadcastItemDrop(pkt->peerId, pkt->itemId, pkt->pos);
            else
                RED4ext::ExecuteFunction("ItemGrabSync", "OnDrop", nullptr, pkt);
        }
        break;
    case EMsg::ItemStore:
        if (size >= sizeof(ItemStorePacket))
        {
            const ItemStorePacket* pkt = reinterpret_cast<const ItemStorePacket*>(payload);
            if (Net_IsAuthoritative())
                Net_BroadcastItemStore(pkt->peerId, pkt->itemId);
            else
                RED4ext::ExecuteFunction("ItemGrabSync", "OnStore", nullptr, pkt);
        }
        break;
    case EMsg::MetroBoard:
        if (size >= sizeof(MetroBoardPacket))
        {
            const MetroBoardPacket* pkt = reinterpret_cast<const MetroBoardPacket*>(payload);
            if (Net_IsAuthoritative())
                Net_BroadcastMetroBoard(pkt->peerId, pkt->lineId, pkt->carIdx);
            else
                RED4ext::ExecuteFunction("TransitSystem", "OnBoard", nullptr, pkt);
        }
        break;
    case EMsg::MetroArrive:
        if (size >= sizeof(MetroArrivePacket))
        {
            const MetroArrivePacket* pkt = reinterpret_cast<const MetroArrivePacket*>(payload);
            if (Net_IsAuthoritative())
                Net_BroadcastMetroArrive(pkt->peerId, pkt->stationId);
            else
                RED4ext::ExecuteFunction("TransitSystem", "OnArrive", nullptr, pkt);
        }
        break;
    case EMsg::RadioChange:
        if (size >= sizeof(RadioChangePacket))
        {
            const RadioChangePacket* pkt = reinterpret_cast<const RadioChangePacket*>(payload);
            if (Net_IsAuthoritative())
                Net_BroadcastRadioChange(pkt->vehId, pkt->stationId, pkt->offsetSec);
            else
                RED4ext::ExecuteFunction("RadioSync", "OnChange", nullptr, pkt);
        }
        break;
    case EMsg::CamHijack:
        if (size >= sizeof(CamHijackPacket))
        {
            const CamHijackPacket* pkt = reinterpret_cast<const CamHijackPacket*>(payload);
            if (Net_IsAuthoritative())
                Net_BroadcastCamHijack(pkt->camId, pkt->peerId);
            else
                RED4ext::ExecuteFunction("CamSync", "OnHijack", nullptr, pkt);
        }
        break;
    case EMsg::CamFrameStart:
        if (size >= sizeof(CamFrameStartPacket))
        {
            const CamFrameStartPacket* pkt = reinterpret_cast<const CamFrameStartPacket*>(payload);
            RED4ext::ExecuteFunction("CamSync", "OnFrame", nullptr, &pkt->camId);
        }
        break;
    case EMsg::CarryBegin:
        if (size >= sizeof(CarryBeginPacket))
        {
            const CarryBeginPacket* pkt = reinterpret_cast<const CarryBeginPacket*>(payload);
            if (Net_IsAuthoritative())
                Net_BroadcastCarryBegin(pkt->carrierId, pkt->entityId);
            else
                RED4ext::ExecuteFunction("CarrySync", "OnBegin", nullptr, pkt);
        }
        break;
    case EMsg::CarrySnap:
        if (size >= sizeof(CarrySnapPacket))
        {
            const CarrySnapPacket* pkt = reinterpret_cast<const CarrySnapPacket*>(payload);
            if (Net_IsAuthoritative())
                Net_BroadcastCarrySnap(pkt->entityId, pkt->pos, pkt->vel);
            else
                RED4ext::ExecuteFunction("CarrySync", "OnSnap", nullptr, pkt);
        }
        break;
    case EMsg::CarryEnd:
        if (size >= sizeof(CarryEndPacket))
        {
            const CarryEndPacket* pkt = reinterpret_cast<const CarryEndPacket*>(payload);
            if (Net_IsAuthoritative())
                Net_BroadcastCarryEnd(pkt->entityId, pkt->pos, pkt->vel);
            else
                RED4ext::ExecuteFunction("CarrySync", "OnEnd", nullptr, pkt);
        }
        break;
    case EMsg::GrenadePrime:
        if (size >= sizeof(GrenadePrimePacket))
        {
            const GrenadePrimePacket* pkt = reinterpret_cast<const GrenadePrimePacket*>(payload);
            if (Net_IsAuthoritative())
                Net_BroadcastGrenadePrime(pkt->entityId, pkt->startTick);
            else
                RED4ext::ExecuteFunction("GrenadeSync", "OnPrime", nullptr, pkt);
        }
        break;
    case EMsg::GrenadeSnap:
        if (size >= sizeof(GrenadeSnapPacket))
        {
            const GrenadeSnapPacket* pkt = reinterpret_cast<const GrenadeSnapPacket*>(payload);
            if (Net_IsAuthoritative())
                Net_BroadcastGrenadeSnap(pkt->entityId, pkt->pos, pkt->vel);
            else
                RED4ext::ExecuteFunction("GrenadeSync", "OnSnap", nullptr, pkt);
        }
        break;
    case EMsg::SmartCamStart:
        if (size >= sizeof(SmartCamStartPacket))
        {
            const SmartCamStartPacket* pkt = reinterpret_cast<const SmartCamStartPacket*>(payload);
            if (Net_IsAuthoritative())
                Net_BroadcastSmartCamStart(pkt->projId);
            else
                RED4ext::ExecuteFunction("SmartCamSync", "OnStart", nullptr, &pkt->projId);
        }
        break;
    case EMsg::SmartCamEnd:
        if (size >= sizeof(SmartCamEndPacket))
        {
            const SmartCamEndPacket* pkt = reinterpret_cast<const SmartCamEndPacket*>(payload);
            if (Net_IsAuthoritative())
                Net_BroadcastSmartCamEnd(pkt->projId);
            else
                RED4ext::ExecuteFunction("SmartCamSync", "OnEnd", nullptr, &pkt->projId);
        }
        break;
    case EMsg::ArcadeStart:
        if (size >= sizeof(ArcadeStartPacket))
        {
            const ArcadeStartPacket* pkt = reinterpret_cast<const ArcadeStartPacket*>(payload);
            if (Net_IsAuthoritative())
                CoopNet::Arcade_Start(pkt->cabId, pkt->peerId, pkt->seed);
            else
                RED4ext::ExecuteFunction("ArcadeSync", "OnStart", nullptr, pkt);
        }
        break;
    case EMsg::ArcadeInput:
        if (size >= sizeof(ArcadeInputPacket))
        {
            const ArcadeInputPacket* pkt = reinterpret_cast<const ArcadeInputPacket*>(payload);
            if (Net_IsAuthoritative())
                CoopNet::Arcade_Input(pkt->frame, pkt->buttonMask);
        }
        break;
    case EMsg::ArcadeScore:
        if (size >= sizeof(ArcadeScorePacket))
        {
            const ArcadeScorePacket* pkt = reinterpret_cast<const ArcadeScorePacket*>(payload);
            RED4ext::ExecuteFunction("ArcadeSync", "OnScore", nullptr, pkt);
        }
        break;

    // --- audit omissions ---------------------------------------------------
    case EMsg::AirVehSpawn:
        if (size >= sizeof(AirVehSpawnPacket))
        {
            const AirVehSpawnPacket* pkt = reinterpret_cast<const AirVehSpawnPacket*>(payload);
            RED4ext::ExecuteFunction("AirVehicleProxy", "AirVehicleProxy_Spawn", nullptr, pkt->vehId, pkt->count, pkt->points);
        }
        break;
    case EMsg::AirVehUpdate:
        if (size >= sizeof(AirVehUpdatePacket))
        {
            const AirVehUpdatePacket* pkt = reinterpret_cast<const AirVehUpdatePacket*>(payload);
            RED4ext::ExecuteFunction("AirVehicleProxy", "AirVehicleProxy_Update", nullptr, pkt->vehId, &pkt->snap);
        }
        break;
    case EMsg::AssetBundle:
        if (size >= sizeof(AssetBundlePacket) && !Net_IsAuthoritative())
        {
            const AssetBundlePacket* pkt = reinterpret_cast<const AssetBundlePacket*>(payload);
            auto& b = g_bundle[pkt->pluginId];
            if (b.data.empty())
                b.expected = pkt->totalBytes;
            b.data.insert(b.data.end(), pkt->data, pkt->data + pkt->dataBytes);
            if (b.data.size() >= b.expected)
            {
                HandleBundleComplete(pkt->pluginId, b.data);
                g_bundle.erase(pkt->pluginId);
            }
        }
        break;
    case EMsg::HitConfirm:
        if (size >= sizeof(HitConfirmPacket))
        {
            const HitConfirmPacket* pkt = reinterpret_cast<const HitConfirmPacket*>(payload);
            std::cout << "HitConfirm id=" << pkt->targetId << " dmg=" << pkt->appliedDamage << std::endl;
        }
        break;
    case EMsg::HitRequest:
        if (size >= sizeof(HitRequestPacket))
        {
            const HitRequestPacket* pkt = reinterpret_cast<const HitRequestPacket*>(payload);
            std::cout << "HitRequest id=" << pkt->targetId << " dmg=" << pkt->damage << std::endl;
        }
        break;
    case EMsg::InterestAdd:
        if (size >= sizeof(InterestPacket))
        {
            const InterestPacket* pkt = reinterpret_cast<const InterestPacket*>(payload);
            std::cout << "InterestAdd " << pkt->id << std::endl;
        }
        break;
    case EMsg::InterestRemove:
        if (size >= sizeof(InterestPacket))
        {
            const InterestPacket* pkt = reinterpret_cast<const InterestPacket*>(payload);
            std::cout << "InterestRemove " << pkt->id << std::endl;
        }
        break;
    case EMsg::JoinDeny:
        std::cout << "Join denied" << std::endl;
        Transition(ConnectionState::Disconnected);
        break;
    case EMsg::JoinRequest:
        std::cout << "Join request" << std::endl;
        break;
    case EMsg::LowBWMode:
        if (size >= sizeof(LowBWModePacket))
        {
            const LowBWModePacket* pkt = reinterpret_cast<const LowBWModePacket*>(payload);
            std::cout << "LowBWMode " << static_cast<int>(pkt->enable) << std::endl;
        }
        break;
    case EMsg::PluginRPC:
        if (size >= sizeof(PluginRPCPacket) && !Net_IsAuthoritative())
        {
            const PluginRPCPacket* pkt = reinterpret_cast<const PluginRPCPacket*>(payload);
            if (size >= sizeof(PluginRPCPacket) - 1 + pkt->jsonBytes)
                ClientPluginProxy_OnRpc(pkt);
        }
        break;
    case EMsg::QuestStage:
        if (size >= sizeof(QuestStagePacket))
        {
            const QuestStagePacket* pkt = reinterpret_cast<const QuestStagePacket*>(payload);
            QuestSync_ApplyQuestStage(pkt->questHash, pkt->stage);
        }
        break;
    case EMsg::Quickhack:
        if (size >= sizeof(QuickhackPacket))
        {
            const QuickhackPacket* pkt = reinterpret_cast<const QuickhackPacket*>(payload);
            if (Net_IsAuthoritative())
            {
                float now = CoopNet::GameClock::GetCurrentTick() * CoopNet::GameClock::currentTickMs;
                if (now - g_lastHackMs[peerId] >= 5000.f)
                {
                    g_lastHackMs[peerId] = now;
                    Net_Broadcast(EMsg::Quickhack, pkt, sizeof(*pkt));
                }
            }
            HackInfoNative info{pkt->targetId, pkt->hackId, pkt->durationMs, 0};
            QuickhackSync_Apply(info);
        }
        break;
    case EMsg::SectorLOD:
        if (size >= sizeof(SectorLODPacket))
        {
            const SectorLODPacket* pkt = reinterpret_cast<const SectorLODPacket*>(payload);
            std::cout << "SectorLOD " << pkt->sectorHash << " -> " << static_cast<int>(pkt->lod) << std::endl;
        }
        break;
    case EMsg::Seed:
        if (size >= sizeof(SeedPacket))
        {
            const SeedPacket* pkt = reinterpret_cast<const SeedPacket*>(payload);
            std::srand(pkt->seed);
        }
        break;
    case EMsg::SeedAck:
        std::cout << "SeedAck" << std::endl;
        break;
    case EMsg::TurretAim:
        if (size >= sizeof(TurretAimPacket))
        {
            const TurretAimPacket* pkt = reinterpret_cast<const TurretAimPacket*>(payload);
            VehicleProxy_SetTurretAim(pkt->vehId, pkt->yaw, pkt->pitch);
        }
        break;
    case EMsg::VehiclePaintChange:
        if (size >= sizeof(VehiclePaintChangePacket))
        {
            const VehiclePaintChangePacket* pkt = reinterpret_cast<const VehiclePaintChangePacket*>(payload);
            VehicleProxy_ApplyPaint(pkt->vehId, pkt->colorId, pkt->plateId);
        }
        break;
    case EMsg::VehicleSnapshot:
        if (size >= sizeof(VehicleSnapshotPacket))
        {
            const VehicleSnapshotPacket* pkt = reinterpret_cast<const VehicleSnapshotPacket*>(payload);
            RED4ext::ExecuteFunction("VehicleProxy", "UpdateSnapshot", nullptr, &pkt->snap);
        }
        break;
    case EMsg::Version:
        std::cout << "Version crc" << std::endl;
        break;
    default:
        break;
    }
}

void Connection::Update(uint64_t nowMs)
{
    if (voiceMuted && voiceMuteEndMs > 0 && nowMs >= voiceMuteEndMs)
    {
        voiceMuted = false;
        voiceMuteEndMs = 0;
        RED4ext::ExecuteFunction("MicIcon", "SetMuted", nullptr, false);
    }
    if (nowMs - lastPingSent >= 5000)
    {
        PingPacket ping{static_cast<uint32_t>(nowMs & 0xFFFFFFFFu)};
        Net_Send(this, EMsg::Ping, &ping, sizeof(ping));
        lastPingSent = nowMs;
    }

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
        const uint64_t timeoutTicks = static_cast<uint64_t>(10000.f / CoopNet::kVehicleStepMs);
        if (CoopNet::GameClock::GetCurrentTick() - lastSectorChangeTick > timeoutTicks)
        {
            std::cout << "SectorReady timeout" << std::endl;
            sectorReady = true;
        }
    }
    if (lastStatTime == 0)
        lastStatTime = nowMs;
    if (nowMs - lastStatTime >= 2000)
    {
        CoopNet::NetStats s{};
        s.ping = static_cast<uint32_t>(rttMs);
        s.loss = packetLoss;
        uint64_t dt = nowMs - lastStatTime;
        s.vKbps = static_cast<uint16_t>((voiceBytes * 1000 / dt) / 1024);
        s.sKbps = static_cast<uint16_t>((snapBytes * 1000 / dt) / 1024);
        s.dropPkts = CoopVoice::ConsumeDropPct();
        voiceBytes = 0;
        snapBytes = 0;
        lastStatTime = nowMs;
        voiceDropped = 0;
        voiceRecv = 0;
        StatHud_OnStats(peerId, s);
    }
    CoopNet::StatBatch_Tick(static_cast<float>(CoopNet::GameClock::GetTickMs()) / 1000.f);
}

void Connection::EnqueuePacket(const RawPacket& pkt)
{
    uint64_t now = GameClock::GetTimeMs();
    if (pkt.hdr.type != static_cast<uint16_t>(EMsg::Voice))
    {
        float dt = static_cast<float>(now - rateLastMs) / 1000.f;
        rateTokens = std::min(30.f, rateTokens + dt * 20.f);
        rateLastMs = now;
        if (rateTokens < 1.f)
        {
            std::cout << "WARN: rate limit drop peer=" << peerId << std::endl;
            return;
        }
        rateTokens -= 1.f;
    }
    m_incoming.Push(pkt);
    lastRecvTime = 0;
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
