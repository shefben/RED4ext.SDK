#include "HttpClient.hpp"
#include "../../third_party/httplib.h"
#include <string>
#include <cstdlib>

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

} // namespace CoopNet
