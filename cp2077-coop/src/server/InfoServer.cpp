#include "InfoServer.hpp"
#include "../net/Net.hpp"
#include <thread>
#include <atomic>
#include <sstream>
#include <vector>

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

namespace CoopNet {
static std::thread g_thread;
static std::atomic<bool> g_running{false};
#ifdef _WIN32
static SOCKET g_sock = INVALID_SOCKET;
#else
static int g_sock = -1;
#endif

static std::string BuildInfo()
{
    std::stringstream ss;
    size_t cur = Net_GetConnections().size();
    ss << "{\"name\":\"Co-op\",\"cur\":" << cur
       << ",\"max\":4,\"password\":false,\"mode\":\"Coop\"}";
    return ss.str();
}

static bool ParseRequestLine(const std::string& req, std::string& method, std::string& path)
{
    size_t endline = req.find("\r\n");
    if (endline == std::string::npos)
        return false;
    std::string line = req.substr(0, endline);
    size_t s1 = line.find(' ');
    if (s1 == std::string::npos)
        return false;
    size_t s2 = line.find(' ', s1 + 1);
    if (s2 == std::string::npos)
        return false;
    method = line.substr(0, s1);
    path = line.substr(s1 + 1, s2 - (s1 + 1));
    return true;
}

static void Loop()
{
#ifdef _WIN32
    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return;
#endif
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(7777);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

#ifdef _WIN32
    g_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_sock == INVALID_SOCKET)
        return;
    BOOL yes = TRUE;
    setsockopt(g_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));
    if (bind(g_sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
        return;
    if (listen(g_sock, 4) == SOCKET_ERROR)
        return;
#else
    g_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (g_sock < 0)
        return;
    int yes = 1;
    setsockopt(g_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if (bind(g_sock, (sockaddr*)&addr, sizeof(addr)) < 0)
        return;
    if (listen(g_sock, 4) < 0)
        return;
#endif

    while (g_running)
    {
#ifdef _WIN32
        SOCKET client = accept(g_sock, nullptr, nullptr);
        if (client == INVALID_SOCKET)
            continue;
#else
        int client = accept(g_sock, nullptr, nullptr);
        if (client < 0)
            continue;
#endif
        char buf[256];
        int len = recv(client,
#ifdef _WIN32
                        buf,
#else
                        buf,
#endif
                        sizeof(buf) - 1, 0);
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
        std::string method, path;
        if (!ParseRequestLine(req, method, path) || method != "GET")
        {
            const char* resp = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
            send(client, resp, 33,
#ifdef _WIN32
                 0
#else
                 0
#endif
            );
#ifdef _WIN32
            closesocket(client);
#else
            close(client);
#endif
            continue;
        }
        if (path == "/info")
        {
            std::string body = BuildInfo();
            std::string hdr = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n";
            send(client, hdr.c_str(), static_cast<int>(hdr.size()), 0);
            send(client, body.c_str(), static_cast<int>(body.size()), 0);
        }
        else
        {
            const char* resp = "HTTP/1.1 404 Not Found\r\n\r\n";
            send(client, resp, 24, 0);
        }
#ifdef _WIN32
        closesocket(client);
#else
        close(client);
#endif
    }
#ifdef _WIN32
    closesocket(g_sock);
    WSACleanup();
#else
    close(g_sock);
#endif
}

void InfoServer_Start()
{
    if (g_running)
        return;
    g_running = true;
    g_thread = std::thread(Loop);
}

void InfoServer_Stop()
{
    if (!g_running)
        return;
    g_running = false;
#ifdef _WIN32
    shutdown(g_sock, SD_BOTH);
    closesocket(g_sock);
    if (g_thread.joinable())
        g_thread.join();
#else
    shutdown(g_sock, SHUT_RDWR);
    if (g_thread.joinable())
        g_thread.join();
#endif
}

} // namespace CoopNet
