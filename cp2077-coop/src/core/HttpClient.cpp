#include "HttpClient.hpp"
#include "../../third_party/httplib.h"
#include "ThreadSafeQueue.hpp"
#include <string>
#include <cstdlib>
#include <thread>
#include <atomic>

namespace CoopNet {

static bool ParseUrl(const std::string& url, std::string& host, int& port, std::string& path)
{
    bool https = false;
    size_t start = 0;
    if (url.rfind("https://", 0) == 0) {
        https = true;
        start = 8;
    } else if (url.rfind("http://", 0) == 0) {
        start = 7;
    }
    size_t slash = url.find('/', start);
    host = url.substr(start, slash - start);
    path = slash != std::string::npos ? url.substr(slash) : "/";
    port = https ? 443 : 80;
    size_t colon = host.find(':');
    if (colon != std::string::npos) {
        port = std::stoi(host.substr(colon + 1));
        host = host.substr(0, colon);
    }
    return https;
}

HttpResponse Http_Get(const std::string& url)
{
    std::string host, path;
    int port;
    bool https = ParseUrl(url, host, port, path);
    httplib::Result res{};
    if (https) {
        httplib::SSLClient cli(host, port);
        res = cli.Get(path.c_str());
    } else {
        httplib::Client cli(host, port);
        res = cli.Get(path.c_str());
    }
    if (res.status == 0)
        return {0, {}};
    return {static_cast<uint16_t>(res.status), res.body};
}

HttpResponse Http_Get(const std::string& url, int timeoutMs)
{
    std::string host, path;
    int port;
    bool https = ParseUrl(url, host, port, path);
    httplib::Result res{};
    if (https) {
        httplib::SSLClient cli(host, port);
        cli.set_connection_timeout(timeoutMs / 1000, (timeoutMs % 1000) * 1000);
        cli.set_read_timeout(timeoutMs / 1000, (timeoutMs % 1000) * 1000);
        res = cli.Get(path.c_str());
    } else {
        httplib::Client cli(host, port);
        cli.set_connection_timeout(timeoutMs / 1000, (timeoutMs % 1000) * 1000);
        cli.set_read_timeout(timeoutMs / 1000, (timeoutMs % 1000) * 1000);
        res = cli.Get(path.c_str());
    }
    if (res.status == 0)
        return {0, {}};
    return {static_cast<uint16_t>(res.status), res.body};
}

HttpResponse Http_Post(const std::string& url, const std::string& body, const std::string& contentType)
{
    std::string host, path;
    int port;
    bool https = ParseUrl(url, host, port, path);
    httplib::Result res{};
    if (https) {
        httplib::SSLClient cli(host, port);
        res = cli.Post(path.c_str(), body, contentType.c_str());
    } else {
        httplib::Client cli(host, port);
        res = cli.Post(path.c_str(), body, contentType.c_str());
    }
    if (res.status == 0)
        return {0, {}};
    return {static_cast<uint16_t>(res.status), res.body};
}

static std::atomic<uint32_t> g_nextToken{1};
static ThreadSafeQueue<HttpAsyncResult> g_asyncQueue;

uint32_t Http_GetAsync(const std::string& url, int timeoutMs, int retries)
{
    uint32_t id = g_nextToken.fetch_add(1, std::memory_order_relaxed);
    std::thread([url, id, timeoutMs, retries]() {
        HttpResponse r{};
        for (int i = 0; i <= retries; ++i) {
            r = Http_Get(url, timeoutMs);
            if (r.status != 0)
                break;
        }
        g_asyncQueue.Push({id, r});
    }).detach();
    return id;
}

bool Http_PollAsync(HttpAsyncResult& out)
{
    return g_asyncQueue.Pop(out);
}

} // namespace CoopNet
