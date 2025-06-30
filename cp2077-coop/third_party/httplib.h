#ifndef CPP_HTTPLIB_H
#define CPP_HTTPLIB_H
#include <string>
#include <cstring>
#include <curl/curl.h> // FIX: replace popen-based HTTP with libcurl

namespace httplib {

struct Result {
    int status;
    std::string body;
};

namespace detail {
inline size_t WriteCB(char* ptr, size_t size, size_t nm, void* data)
{
    auto& out = *static_cast<std::string*>(data);
    out.append(ptr, size * nm);
    return size * nm;
}

inline Result Request(const std::string& url, const char* method,
                      const std::string& payload, const char* type)
{
    Result r{0, {}};
    CURL* curl = curl_easy_init();
    if (!curl)
        return r;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCB);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &r.body);
    if (payload.size() > 0 || (method && std::strcmp(method, "POST") == 0)) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
        struct curl_slist* headers = nullptr;
        if (type) {
            std::string h = std::string("Content-Type: ") + type;
            headers = curl_slist_append(headers, h.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        }
    }
    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        long code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        r.status = static_cast<int>(code);
    }
    curl_easy_cleanup(curl);
    return r;
}
} // namespace detail

class SSLClient {
    std::string host;
    int port;

public:
    explicit SSLClient(const std::string& h, int p = 443)
        : host(h), port(p) {}

    Result Get(const char* path) const {
        std::string url = "https://" + host;
        if (port != 443)
            url += ":" + std::to_string(port);
        url += path;
        return detail::Request(url, "GET", {}, nullptr);
    }

    Result Post(const char* path, const std::string& body, const char* type) const {
        std::string url = "https://" + host;
        if (port != 443)
            url += ":" + std::to_string(port);
        url += path;
        return detail::Request(url, "POST", body, type);
    }
};

class Client {
    std::string host;
    int port;

public:
    explicit Client(const std::string& h, int p = 80)
        : host(h), port(p) {}

    Result Get(const char* path) const {
        std::string url = "http://" + host;
        if (port != 80)
            url += ":" + std::to_string(port);
        url += path;
        return detail::Request(url, "GET", {}, nullptr);
    }

    Result Post(const char* path, const std::string& body, const char* type) const {
        std::string url = "http://" + host;
        if (port != 80)
            url += ":" + std::to_string(port);
        url += path;
        return detail::Request(url, "POST", body, type);
    }
};

} // namespace httplib

#endif
