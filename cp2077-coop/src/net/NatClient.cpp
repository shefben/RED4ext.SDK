#include "NatClient.hpp"
#include "NatTraversal.hpp"
#include "Connection.hpp"
#include <iostream>
#include <chrono>
#include <thread>

namespace CoopNet
{
static CandidateCallback g_callback;
static NatTraversal g_traversal;
static std::string g_remoteCandidate;
static uint64_t g_relayBytes = 0;

void Nat_SetCandidateCallback(CandidateCallback cb)
{
    g_callback = cb;
    g_traversal.SetCandidateCallback(cb);
}

void Nat_Start()
{
    g_traversal.Start();
}

static bool RequestTurnCreds(std::string& host, int& port,
                             std::string& user, std::string& pass)
{
    CoopNet::TurnCreds creds;
    if (!g_traversal.GetTurnCreds(creds))
        return false;
    host = creds.host;
    port = creds.port;
    user = creds.user;
    pass = creds.pass;
    return true;
}

uint64_t Nat_GetRelayBytes()
{
    return g_relayBytes;
}

const std::string& Nat_GetLocalCandidate()
{
    return g_traversal.GetLocalCandidate();
}

void Nat_PerformHandshake(Connection* conn)
{
    if (!conn)
        return;

    std::cout << "Nat_PerformHandshake" << std::endl;
    g_relayBytes = 0;
    auto start = std::chrono::steady_clock::now();
    if (g_traversal.PerformHandshake(g_remoteCandidate, g_relayBytes))
    {
        conn->relayBytes += g_relayBytes;
        conn->rttMs = std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - start).count();
        conn->usingRelay = g_relayBytes > 0;
        std::cout << "TURN relay bytes=" << conn->relayBytes << std::endl;
        if (conn->usingRelay)
            std::cout << "NAT method=relay" << std::endl;
        else
            std::cout << "NAT method=direct" << std::endl;
    }
    // Scrub sensitive TURN credentials after handshake attempt
    g_traversal.ClearTurnCreds();
}

void Nat_AddRemoteCandidate(const char* cand)
{
    if (cand)
        g_remoteCandidate = cand;
}

void Nat_SetTurnCreds(const std::string& host, int port,
                      const std::string& user, const std::string& pass)
{
    TurnCreds c{host, port, user, pass};
    g_traversal.SetTurnCreds(c);
}

bool Nat_GetTurnCreds(std::string& host, int& port,
                      std::string& user, std::string& pass)
{
    TurnCreds c;
    if (!g_traversal.GetTurnCreds(c))
        return false;
    host = c.host;
    port = c.port;
    user = c.user;
    pass = c.pass;
    return true;
}

} // namespace CoopNet
