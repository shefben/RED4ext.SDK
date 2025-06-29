#include "InfoServer.hpp"
#include "../net/Net.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include <atomic>
#include <sstream>
#include <vector>

namespace CoopNet {
static std::thread g_thread;
static std::atomic<bool> g_running{false};
static int g_sock = -1;

static std::string BuildInfo()
{
    std::stringstream ss;
    size_t cur = Net_GetConnections().size();
    ss << "{\"name\":\"Co-op\",\"cur\":" << cur
       << ",\"max\":4,\"password\":false,\"mode\":\"Coop\"}";
    return ss.str();
}

static void Loop()
{
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(7777);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    g_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (g_sock < 0)
        return;
    int yes = 1;
    setsockopt(g_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if (bind(g_sock, (sockaddr*)&addr, sizeof(addr)) < 0)
        return;
    if (listen(g_sock, 4) < 0)
        return;

    while (g_running)
    {
        int client = accept(g_sock, nullptr, nullptr);
        if (client < 0)
            continue;
        char buf[256];
        int len = recv(client, buf, sizeof(buf) - 1, 0);
        if (len <= 0)
        {
            close(client);
            continue;
        }
        buf[len] = 0;
        std::string req(buf, len);
        if (req.rfind("GET /info", 0) == 0)
        {
            std::string body = BuildInfo();
            std::string hdr = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n";
            send(client, hdr.c_str(), hdr.size(), 0);
            send(client, body.c_str(), body.size(), 0);
        }
        else
        {
            const char* resp = "HTTP/1.1 404 Not Found\r\n\r\n";
            send(client, resp, 24, 0);
        }
        close(client);
    }
    close(g_sock);
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
    shutdown(g_sock, SHUT_RDWR);
    if (g_thread.joinable())
        g_thread.join();
}

} // namespace CoopNet
