#include "Connection.hpp"
#include "StatBatch.hpp"
#include <iostream> // RED4ext logger stub


// Temporary proxies for script methods.
static void AvatarProxy_Spawn(uint32_t peerId, bool isLocal)
{
    std::cout << "AvatarProxy::Spawn " << peerId << " local=" << isLocal << std::endl;
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
    // TODO(next ticket): invoke ChatOverlay.PushGlobal via RTTI
    std::cout << "Chat: " << msg << std::endl;
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

namespace CoopNet
{

Connection::Connection()
    : state(ConnectionState::Disconnected)
    , lastPingSent(0)
    , lastRecvTime(0)
{
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
        }
        break;
    case EMsg::Disconnect:
        Killfeed_Push("0 disconnected");
        Transition(ConnectionState::Disconnected);
        break;
    case EMsg::AvatarSpawn:
        AvatarProxy_Spawn(0, false); // FIXME(next ticket: parse payload)
        break;
    case EMsg::AvatarDespawn:
        AvatarProxy_Despawn(0); // FIXME(next ticket: parse payload)
        Killfeed_Push("0 disconnected");
        break;
    case EMsg::Chat:
        ChatOverlay_Push("peer: msg"); // FIXME(next ticket: parse payload)
        break;
    case EMsg::QuestStage:
        QuestSync_ApplyQuestStage(n"stubQuest"); // FIXME(next ticket: parse payload)
        break;
    case EMsg::SceneTrigger:
        QuestSync_ApplySceneTrigger("0", true); // FIXME(next ticket: parse payload)
        break;
    case EMsg::ScoreUpdate:
        if (size >= sizeof(ScoreUpdatePacket))
        {
            const ScoreUpdatePacket* pkt = reinterpret_cast<const ScoreUpdatePacket*>(payload);
            DMScoreboard_OnScorePacket(pkt->peerId, pkt->k, pkt->d);
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
    // At end of tick loop, send any batched score packets.
    CoopNet::FlushStats();
}

void Connection::EnqueuePacket(const RawPacket& pkt)
{
    m_incoming.Push(pkt);
    lastRecvTime = 0; // activity timestamp placeholder
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

