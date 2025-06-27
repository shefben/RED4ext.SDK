#pragma once

#include "Packets.hpp"
#include "core/ThreadSafeQueue.hpp"
#include <RED4ext/Scripting/Natives/Generated/Vector3.hpp>
#include <cstdint>
#include <unordered_set>
#include <vector>

namespace CoopNet
{

enum class ConnectionState
{
    Disconnected,
    Handshaking,
    Lobby,
    InGame
};

class Connection
{
public:
    Connection();

    struct RawPacket
    {
        PacketHeader hdr;
        std::vector<uint8_t> data;
    };

    void StartHandshake();
    void HandlePacket(const PacketHeader& hdr, const void* payload, uint16_t size);
    void EnqueuePacket(const RawPacket& pkt);
    void Update(uint64_t nowMs);

    bool PopPacket(RawPacket& out);

    void SendSectorChange(uint64_t hash);
    void SendSectorReady(uint64_t hash);

    ConnectionState GetState() const
    {
        return state;
    }

private:
    void Transition(ConnectionState next);

    ConnectionState state;
    ThreadSafeQueue<RawPacket> m_incoming; // avoids cross-thread deadlocks
public:
    uint64_t lastPingSent;
    uint64_t lastRecvTime;
    uint32_t peerId = 0;
    uint64_t muteUntilMs = 0;
    RED4ext::Vector3 avatarPos;
    uint64_t currentSector = 0;
    bool sectorReady = true;
    uint64_t lastSectorChangeTick = 0;
    std::unordered_set<uint32_t> subscribedNpcs;
    uint64_t relayBytes = 0;
    bool usingRelay = false;
    float rttMs = 0.f;
    float rttHist[16]{};
    uint8_t rttIndex = 0;
    float packetLoss = 0.f;
    uint64_t voiceBytes = 0;
    uint64_t snapBytes = 0;
    uint32_t voiceRecv = 0;
    uint32_t voiceDropped = 0;
    uint64_t lastStatTime = 0;
    uint64_t balance = 10000;
    uint64_t lastNonce = 0;
    uint64_t invulEndTick = 0;
};

} // namespace CoopNet
