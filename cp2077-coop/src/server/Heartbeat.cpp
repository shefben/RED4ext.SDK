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
extern std::string g_cfgMasterHost;
extern int g_cfgMasterPort;
static int g_backoff = 1;

static bool JsonGetString(const std::string& json, const char* key, std::string& out)
{
    std::string k = std::string("\"") + key + "\"";
    size_t p = json.find(k);
    if (p == std::string::npos)
        return false;
    p = json.find(':', p);
    if (p == std::string::npos)
        return false;
    while (p + 1 < json.size() && (json[p + 1] == ' ' || json[p + 1] == '\t'))
        ++p;
    if (p + 1 >= json.size() || json[p + 1] != '"')
        return false;
    size_t start = p + 2;
    size_t end = json.find('"', start);
    if (end == std::string::npos)
        return false;
    out.assign(json.data() + start, end - start);
    return true;
}

static bool JsonGetBool(const std::string& json, const char* key, bool& out)
{
    std::string k = std::string("\"") + key + "\"";
    size_t p = json.find(k);
    if (p == std::string::npos)
        return false;
    p = json.find(':', p);
    if (p == std::string::npos)
        return false;
    size_t s = p + 1;
    while (s < json.size() && (json[s] == ' ' || json[s] == '\t')) ++s;
    if (json.compare(s, 4, "true") == 0) { out = true; return true; }
    if (json.compare(s, 5, "false") == 0) { out = false; return true; }
    return false;
}

static std::string GetSecret()
{
    const char* env = std::getenv("COOP_SECRET");
    return env ? env : "changeme";
}

static std::string FetchNonce()
{
    httplib::SSLClient cli(g_cfgMasterHost.c_str(), g_cfgMasterPort > 0 ? g_cfgMasterPort : 443);
    auto res = cli.Get("/api/challenge");
    // FIX: `Result` is a struct, not a pointer
    if (res.status != 200)
        return {};
    const std::string& body = res.body;
    std::string nonce;
    if (!JsonGetString(body, "nonce", nonce))
        return {};
    return nonce;
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

    httplib::SSLClient cli(g_cfgMasterHost.c_str(), g_cfgMasterPort > 0 ? g_cfgMasterPort : 443);
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
    bool ok = false;
    if (!JsonGetBool(body, "ok", ok) || !ok)
        return;
    std::string url;
    if (JsonGetString(body, "url", url))
    {
        std::string host; int port = 443;
        size_t scheme = url.find("//");
        size_t hp = (scheme == std::string::npos) ? 0 : scheme + 2;
        size_t slash = url.find('/', hp);
        std::string hostport = url.substr(hp, (slash == std::string::npos) ? std::string::npos : slash - hp);
        size_t colon = hostport.rfind(':');
        if (colon != std::string::npos)
        {
            host = hostport.substr(0, colon);
            try { port = std::stoi(hostport.substr(colon + 1)); } catch (...) { port = 443; }
        }
        else host = hostport;
        std::string user, pass;
        JsonGetString(body, "u", user);
        JsonGetString(body, "p", pass);
        if (!host.empty())
            Nat_SetTurnCreds(host, port, user, pass);
        std::string rcand;
        if (JsonGetString(body, "cand", rcand) && !rcand.empty())
            Nat_AddRemoteCandidate(rcand.c_str());
    }
}

void Heartbeat_Announce(const std::string& json)
{
    httplib::SSLClient cli(g_cfgMasterHost.c_str(), g_cfgMasterPort > 0 ? g_cfgMasterPort : 443);
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
