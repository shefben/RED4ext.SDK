#include "NatClient.hpp"
#include <iostream>
#include <chrono>
#include <thread>

namespace CoopNet
{
static CandidateCallback g_callback;
static juice_agent_t* g_agent = nullptr;
static std::string g_remoteCandidate;
static bool g_connected = false;
static uint64_t g_relayBytes = 0;
static std::string g_turnHost;
static int g_turnPort = 0;
static std::string g_turnUser;
static std::string g_turnPass;
static bool g_haveTurn = false;

void Nat_SetCandidateCallback(CandidateCallback cb)
{
    g_callback = cb;
}

void Nat_Start()
{
    juice_agent_config_t cfg = JUICE_AGENT_CONFIG_DEFAULT;
    cfg.stun_server_host = "stun.l.google.com";
    cfg.stun_server_port = 19302;
    cfg.cb_candidate = [](juice_agent_t*, const char* sdp, void*) {
        if (g_callback)
            g_callback(sdp);
    };
    cfg.cb_state_changed = [](juice_agent_t*, juice_state_t state, void*) {
        if (state == JUICE_STATE_CONNECTED)
            g_connected = true;
    };
    if (juice_create(&cfg, &g_agent) != 0)
    {
        std::cerr << "juice_create failed" << std::endl;
        return;
    }
    juice_gather_candidates(g_agent);
}

static bool RequestTurnCreds(std::string& host, int& port,
                             std::string& user, std::string& pass)
{
    if (!g_haveTurn)
        return false;
    host = g_turnHost;
    port = g_turnPort;
    user = g_turnUser;
    pass = g_turnPass;
    return true;
}

uint64_t Nat_GetRelayBytes()
{
    return g_relayBytes;
}

void Nat_PerformHandshake(Connection* conn)
{
    if (!conn)
        return;

    std::cout << "Nat_PerformHandshake" << std::endl;
    g_connected = false;
    g_relayBytes = 0;
    if (g_agent && !g_remoteCandidate.empty())
    {
        juice_set_remote_description(g_agent, g_remoteCandidate.c_str());
        juice_connect(g_agent);
    auto start = std::chrono::steady_clock::now();
    while (!g_connected)
    {
            juice_poll(g_agent);
            if (std::chrono::steady_clock::now() - start > std::chrono::seconds(5))
            {
                std::cout << "ICE failed, trying TURN" << std::endl;
                std::string host, user, pass;
                int port = 0;
                if (RequestTurnCreds(host, port, user, pass))
                {
                    juice_destroy(g_agent);
                    juice_agent_config_t cfg = JUICE_AGENT_CONFIG_DEFAULT;
                    cfg.stun_server_host = "stun.l.google.com";
                    cfg.stun_server_port = 19302;
                    cfg.turn_server_host = host.c_str();
                    cfg.turn_server_port = port;
                    cfg.turn_username = user.c_str();
                    cfg.turn_password = pass.c_str();
                    cfg.cb_candidate = [](juice_agent_t*, const char* sdp, void*) {
                        if (g_callback)
                            g_callback(sdp);
                    };
                    cfg.cb_state_changed = [](juice_agent_t*, juice_state_t state, void*) {
                        if (state == JUICE_STATE_CONNECTED)
                            g_connected = true;
                    };
                    if (juice_create(&cfg, &g_agent) == 0)
                    {
                        juice_set_remote_description(g_agent, g_remoteCandidate.c_str());
                        juice_connect(g_agent);
                        start = std::chrono::steady_clock::now();
                    }
                }
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if (g_connected && conn)
        {
            // Placeholder bandwidth accounting
            g_relayBytes += 5000; // NT-3: obtain stats from libjuice
            conn->relayBytes += g_relayBytes;
            conn->rttMs = std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - start).count();
            conn->usingRelay = g_relayBytes > 0;
            std::cout << "TURN relay bytes=" << conn->relayBytes << std::endl;
        }
    }
}

void Nat_AddRemoteCandidate(const char* cand)
{
    if (cand)
        g_remoteCandidate = cand;
}

void Nat_SetTurnCreds(const std::string& host, int port,
                      const std::string& user, const std::string& pass)
{
    g_turnHost = host;
    g_turnPort = port;
    g_turnUser = user;
    g_turnPass = pass;
    g_haveTurn = true;
}

bool Nat_GetTurnCreds(std::string& host, int& port,
                      std::string& user, std::string& pass)
{
    if (!g_haveTurn)
        return false;
    host = g_turnHost;
    port = g_turnPort;
    user = g_turnUser;
    pass = g_turnPass;
    return true;
}

} // namespace CoopNet
