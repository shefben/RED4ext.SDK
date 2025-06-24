#pragma once

#include <cstdint>
#include <vector>
#include "Packets.hpp"
#include "core/ThreadSafeQueue.hpp"

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

    ConnectionState GetState() const { return state; }

private:
    void Transition(ConnectionState next);

    ConnectionState state;
    ThreadSafeQueue<RawPacket> m_incoming; // avoids cross-thread deadlocks
public:
    uint64_t lastPingSent;
    uint64_t lastRecvTime;
};

} // namespace CoopNet

