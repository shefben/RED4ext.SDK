#include "WebDash.hpp"
#include "../net/Net.hpp"
#include "../net/Connection.hpp"
#include "../core/GameClock.hpp"
#include "../core/ThreadSafeQueue.hpp"
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include <atomic>
#include <vector>
#include <sstream>
#include <iostream>

namespace CoopNet
{
static std::thread g_thread;
static std::atomic<bool> g_running{false};
static int g_listenSock = -1;
static std::vector<int> g_wsClients;
static ThreadSafeQueue<std::string> g_events;

// Build JSON status payload describing connected peers.
static std::string BuildStatus()
{
    std::stringstream ss;
    ss << "{\"peers\":[";
    auto conns = Net_GetConnections();
    for (size_t i = 0; i < conns.size(); ++i)
    {
        auto* c = conns[i];
        ss << "{\"id\":" << c->peerId << ",\"hist\":[";
        for (int h = 0; h < 16; ++h)
        {
            ss << c->rttHist[h];
            if (h < 15) ss << ',';
        }
        ss << "],\"relay\":" << c->relayBytes
           << ",\"pos\":" << c->avatarPos.X << "," << c->avatarPos.Y
           << "}";
        if (i + 1 < conns.size())
            ss << ',';
    }
    ss << "]}";
    return ss.str();
}

static const char* kPage =
"<!DOCTYPE html><html><body><table id='peers'><tr><th>ID</th><th>Ping</th><th>Pos</th><th>Mode</th></tr></table>"
"<script>async function p(){let r=await fetch('/status');let d=await r.json();let t=document.getElementById('peers');t.innerHTML='<tr><th>ID</th><th>Ping</th><th>Pos</th><th>Mode</th></tr>';d.peers.forEach(function(e){let r=document.createElement('tr');r.innerHTML='<td>'+e.id+'</td><td>'+e.ping+'</td><td>'+e.pos+'</td><td>'+e.mode+'</td>';t.appendChild(r);});}setInterval(p,2000);p();</script>"
"</body></html>";

static void ServerLoop()
{
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(7788);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    g_listenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (g_listenSock < 0)
        return;
    int yes = 1;
    setsockopt(g_listenSock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if (bind(g_listenSock, (sockaddr*)&addr, sizeof(addr)) < 0)
        return;
    if (listen(g_listenSock, 4) < 0)
        return;

    while (g_running)
    {
        int client = accept(g_listenSock, nullptr, nullptr);
        if (client < 0)
            continue;
        char buf[1024];
        int len = recv(client, buf, sizeof(buf) - 1, 0);
        if (len <= 0)
        {
            close(client);
            continue;
        }
        buf[len] = 0;
        std::string req(buf, len);
        std::string body;
        std::string hdr;
        if (req.rfind("GET /ws", 0) == 0 && req.find("Upgrade: websocket") != std::string::npos)
        {
            size_t pos = req.find("Sec-WebSocket-Key: ");
            if (pos != std::string::npos)
            {
                pos += 19;
                size_t end = req.find('\r', pos);
                std::string key = req.substr(pos, end - pos);
                key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
                unsigned char sha[20];
                SHA1(reinterpret_cast<const unsigned char*>(key.c_str()), key.size(), sha);
                char b64[32];
                EVP_EncodeBlock(reinterpret_cast<unsigned char*>(b64), sha, 20);
                std::string accept(b64);
                hdr = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: " + accept + "\r\n\r\n";
                send(client, hdr.c_str(), hdr.size(), 0);
                g_wsClients.push_back(client);
                continue;
            }
        }
        else if (req.rfind("GET /status", 0) == 0)
        {
            body = BuildStatus();
            hdr = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n";
            send(client, hdr.c_str(), hdr.size(), 0);
            send(client, body.c_str(), body.size(), 0);
        }
        else
        {
            body = kPage;
            hdr = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
            send(client, hdr.c_str(), hdr.size(), 0);
            send(client, body.c_str(), body.size(), 0);
        }
        close(client);
    }
    for (int ws : g_wsClients)
    {
        std::string status = BuildStatus();
        uint8_t hdr[2]{0x81, static_cast<uint8_t>(status.size())};
        send(ws, hdr, 2, 0);
        send(ws, status.c_str(), status.size(), 0);
        std::string evt;
        while (g_events.Pop(evt))
        {
            uint8_t h[2]{0x81, static_cast<uint8_t>(evt.size())};
            send(ws, h, 2, 0);
            send(ws, evt.c_str(), evt.size(), 0);
        }
    }
    close(g_listenSock);
}

void WebDash_Start()
{
    if (g_running)
        return;
    g_running = true;
    g_thread = std::thread(ServerLoop);
}

void WebDash_Stop()
{
    if (!g_running)
        return;
    g_running = false;
    shutdown(g_listenSock, SHUT_RDWR);
    if (g_thread.joinable())
        g_thread.join();
}

void WebDash_PushEvent(const std::string& json)
{
    g_events.Push(json);
}

} // namespace CoopNet
