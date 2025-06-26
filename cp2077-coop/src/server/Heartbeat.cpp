#include "Heartbeat.hpp"
#include "../net/NatClient.hpp"
#include "../../third_party/httplib.h"
#include <chrono>
#include <thread>
#include <iostream>

namespace CoopNet {
static int g_backoff = 1;

void Heartbeat_Send(const std::string& sessionJson)
{
    httplib::SSLClient cli("coop-master", 443);
    auto res = cli.Post("/api/heartbeat", sessionJson, "application/json");
    if (!res || res->status != 200)
    {
        std::cerr << "Heartbeat failed" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(g_backoff));
        if (g_backoff < 32)
            g_backoff *= 2;
        return;
    }
    g_backoff = 1;
    const std::string& body = res->body;
    if (body.find("\"ok\":true") != std::string::npos)
    {
        size_t u = body.find("\"url\":\"");
        if (u != std::string::npos)
        {
            size_t start = u + 7;
            size_t end = body.find('"', start);
            std::string url = body.substr(start, end - start);
            size_t hostPos = url.find(':', 5);
            std::string host = url.substr(5, hostPos - 5);
            int port = std::stoi(url.substr(hostPos + 1));
            size_t uu = body.find("\"u\":\"", end);
            size_t uend = body.find('"', uu + 5);
            std::string user = body.substr(uu + 5, uend - (uu + 5));
            size_t pp = body.find("\"p\":\"", uend);
            size_t pend = body.find('"', pp + 5);
            std::string pass = body.substr(pp + 5, pend - (pp + 5));
            Nat_SetTurnCreds(host, port, user, pass);
        }
    }
}

void Heartbeat_Announce(const std::string& json)
{
    httplib::SSLClient cli("coop-master", 443);
    auto res = cli.Post("/announce", json, "application/json");
    if (!res || res->status != 200)
    {
        std::cerr << "Announce failed" << std::endl;
    }
}

} // namespace CoopNet
