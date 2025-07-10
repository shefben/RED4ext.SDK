#include "Heartbeat.hpp"
#include "../net/NatClient.hpp"
#include "../../third_party/httplib.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <openssl/sha.h>
#include <cstdlib>
#include <iomanip>
#include <sstream>

namespace CoopNet {
static int g_backoff = 1;

static std::string GetSecret()
{
    const char* env = std::getenv("COOP_SECRET");
    return env ? env : "changeme";
}

static std::string FetchNonce()
{
    httplib::SSLClient cli("coop-master", 443);
    auto res = cli.Get("/api/challenge");
    // FIX: `Result` is a struct, not a pointer
    if (res.status != 200)
        return {};
    const std::string& body = res.body;
    size_t p = body.find("\"nonce\":\"");
    if (p == std::string::npos)
        return {};
    p += 9;
    size_t e = body.find('"', p);
    if (e == std::string::npos)
        return {};
    return body.substr(p, e - p);
}

static std::string Sign(const std::string& nonce)
{
    std::string msg = nonce + GetSecret();
    unsigned char sha[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(msg.data()), msg.size(), sha);
    std::ostringstream os;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        os << std::hex << std::setw(2) << std::setfill('0') << (int)sha[i];
    return os.str();
}

void Heartbeat_Send(const std::string& sessionJson)
{
    std::string nonce = FetchNonce();
    if (nonce.empty())
        return;
    std::string auth = Sign(nonce);
    std::string payload = sessionJson;
    std::string cand = Nat_GetLocalCandidate();
    if (!payload.empty() && payload.back() == '}')
        payload.pop_back();
    payload += ",\"cand\":\"" + cand + "\",\"nonce\":\"" + nonce + "\",\"auth\":\"" + auth + "\"}";

    httplib::SSLClient cli("coop-master", 443);
    auto res = cli.Post("/api/heartbeat", payload, "application/json");
    // FIX: check struct return, not pointer
    if (res.status != 200)
    {
        std::cerr << "Heartbeat failed" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(g_backoff));
        if (g_backoff < 32)
            g_backoff *= 2;
        return;
    }
    g_backoff = 1;
    const std::string& body = res.body;
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
            size_t cc = body.find("\"cand\":\"", pend);
            if (cc != std::string::npos)
            {
                size_t cend = body.find('"', cc + 8);
                std::string rcand = body.substr(cc + 8, cend - (cc + 8));
                Nat_AddRemoteCandidate(rcand.c_str());
            }
        }
    }
}

void Heartbeat_Announce(const std::string& json)
{
    httplib::SSLClient cli("coop-master", 443);
    auto res = cli.Post("/announce", json, "application/json");
    // FIX: validate struct result instead of pointer check
    if (res.status != 200)
    {
        std::cerr << "Announce failed" << std::endl;
    }
}

void Heartbeat_Disconnect(uint32_t sessionId)
{
    std::string nonce = FetchNonce();
    if (nonce.empty())
        return;
    std::string auth = Sign(nonce);
    std::string payload = "{\"id\":" + std::to_string(sessionId) + ",\"nonce\":\"" + nonce + "\",\"auth\":\"" + auth + "\"}";
    httplib::SSLClient cli("coop-master", 443);
    cli.Post("/api/disconnect", payload, "application/json");
}

} // namespace CoopNet
