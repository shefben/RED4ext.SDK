#include "Net.hpp"
#include "Connection.hpp"
#include "NetConfig.hpp"
#include <enet/enet.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstring>

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
}

void Net_Init()
{
    if (enet_initialize() != 0)
    {
        std::cout << "enet_initialize failed" << std::endl;
        return;
    }

    g_Host = enet_host_create(nullptr, 8, 2, 0, 0);
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
            g_Peers.push_back(e);
            std::cout << "peer connected" << std::endl;
            break;
        }
        case ENET_EVENT_TYPE_DISCONNECT:
        {
            auto it = std::find_if(g_Peers.begin(), g_Peers.end(), [&](const PeerEntry& p){ return p.peer == evt.peer; });
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
                auto it = std::find_if(g_Peers.begin(), g_Peers.end(), [&](const PeerEntry& p){ return p.peer == evt.peer; });
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
