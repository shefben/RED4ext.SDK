#include "WebDash.hpp"
#include "../net/Net.hpp"
#include "../net/Connection.hpp"
#include "../core/GameClock.hpp"
#include "../core/ThreadSafeQueue.hpp"
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <thread>
#include <atomic>
#include <vector>
#include <sstream>
#include <iostream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace CoopNet
{
#ifdef _WIN32
using SocketType = SOCKET;
#else
using SocketType = int;
#endif
static std::thread g_thread;
static std::atomic<bool> g_running{false};
#ifdef _WIN32
static SOCKET g_listenSock = INVALID_SOCKET;
#else
static int g_listenSock = -1;
#endif
static std::vector<SocketType> g_wsClients;
static ThreadSafeQueue<std::string> g_events;

static bool GetHeaderValue(const std::string& req, const char* key, std::string& out)
{
    std::string k = std::string(key);
    size_t p = req.find(k);
    if (p == std::string::npos)
        return false;
    p += k.size();
    while (p < req.size() && (req[p] == ' ' || req[p] == '\t')) ++p;
    size_t end = req.find('\r', p);
    if (end == std::string::npos)
        end = req.find('\n', p);
    if (end == std::string::npos)
        return false;
    out.assign(req.data() + p, end - p);
    return true;
}

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

static bool SendWSFrame(SocketType s, const std::string& payload)
{
    std::string header;
    size_t len = payload.size();
    if (len <= 125)
    {
        header.resize(2);
        header[0] = static_cast<char>(0x81);
        header[1] = static_cast<char>(len);
    }
    else if (len <= 65535)
    {
        header.resize(4);
        header[0] = static_cast<char>(0x81);
        header[1] = 126;
        header[2] = static_cast<char>((len >> 8) & 0xFF);
        header[3] = static_cast<char>(len & 0xFF);
    }
    else
    {
        header.resize(10);
        header[0] = static_cast<char>(0x81);
        header[1] = 127;
        uint64_t l = static_cast<uint64_t>(len);
        for (int i = 0; i < 8; ++i)
            header[9 - i] = static_cast<char>((l >> (8 * i)) & 0xFF);
    }
    if (send(s, header.c_str(), static_cast<int>(header.size()), 0) < 0)
        return false;
    return send(s, payload.c_str(), static_cast<int>(payload.size()), 0) >= 0;
}

static void ServerLoop()
{
#ifdef _WIN32
    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return;
#endif
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(7788);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    
#ifdef _WIN32
    g_listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_listenSock == INVALID_SOCKET)
        return;
    BOOL yes = TRUE;
    setsockopt(g_listenSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));
    if (bind(g_listenSock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
        return;
    if (listen(g_listenSock, 4) == SOCKET_ERROR)
        return;
#else
    g_listenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (g_listenSock < 0)
        return;
    int yes = 1;
    setsockopt(g_listenSock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if (bind(g_listenSock, (sockaddr*)&addr, sizeof(addr)) < 0)
        return;
    if (listen(g_listenSock, 4) < 0)
        return;
#endif

    while (g_running)
    {
        
#ifdef _WIN32
        SOCKET client = accept(g_listenSock, nullptr, nullptr);
        if (client == INVALID_SOCKET)
            continue;
#else
        int client = accept(g_listenSock, nullptr, nullptr);
        if (client < 0)
            continue;
#endif
        char buf[1024];
        int len = recv(client, buf, sizeof(buf) - 1, 0);
        if (len <= 0)
        {
            
#ifdef _WIN32
            closesocket(client);
#else
            close(client);
#endif
            continue;
        }
        buf[len] = 0;
        std::string req(buf, len);
        // Basic request validation: only allow GET for /, /status, or /ws
        if (!(req.rfind("GET ", 0) == 0))
        {
            const char* resp = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
            send(client, resp, 33, 0);
            
#ifdef _WIN32
            closesocket(client);
#else
            close(client);
#endif
            continue;
        }
        std::string body;
        std::string hdr;
        if (req.rfind("GET /ws", 0) == 0 && req.find("Upgrade: websocket") != std::string::npos)
        {
            std::string key;
            if (GetHeaderValue(req, "Sec-WebSocket-Key:", key))
            {
                key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
                unsigned char sha[20];
                SHA1(reinterpret_cast<const unsigned char*>(key.c_str()), key.size(), sha);
                char b64[32];
                EVP_EncodeBlock(reinterpret_cast<unsigned char*>(b64), sha, 20);
                std::string accept(b64);
                hdr = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: " + accept + "\r\n\r\n";
                send(client, hdr.c_str(), static_cast<int>(hdr.size()), 0);
                g_wsClients.push_back(static_cast<SocketType>(client));
                continue;
            }
        }
        else if (req.rfind("GET /status", 0) == 0)
        {
            body = BuildStatus();
            hdr = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n";
            send(client, hdr.c_str(), static_cast<int>(hdr.size()), 0);
            send(client, body.c_str(), static_cast<int>(body.size()), 0);
        }
        else
        {
            body = kPage;
            hdr = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
            send(client, hdr.c_str(), static_cast<int>(hdr.size()), 0);
            send(client, body.c_str(), static_cast<int>(body.size()), 0);
        }
        
#ifdef _WIN32
        closesocket(client);
#else
        close(client);
#endif
    }
    for (SocketType ws : g_wsClients)
    {
        std::string status = BuildStatus();
        SendWSFrame(ws, status);
        std::string evt;
        while (g_events.Pop(evt))
        {
            SendWSFrame(ws, evt);
        }
    }
    
#ifdef _WIN32
    closesocket(g_listenSock);
    WSACleanup();
#else
    close(g_listenSock);
#endif
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
    
#ifdef _WIN32
    shutdown(g_listenSock, SD_BOTH);
#else
    shutdown(g_listenSock, SHUT_RDWR);
#endif
    if (g_thread.joinable())
        g_thread.join();
}

void WebDash_PushEvent(const std::string& json)
{
    g_events.Push(json);
}

} // namespace CoopNet
