#pragma once
#include <string>
namespace CoopNet {
struct HttpResponse {
    uint16_t status;
    std::string body;
};
struct HttpAsyncResult {
    uint32_t token;
    HttpResponse resp;
};

HttpResponse Http_Get(const std::string& url);
HttpResponse Http_Get(const std::string& url, int timeoutMs);
HttpResponse Http_Post(const std::string& url, const std::string& body, const std::string& contentType);
uint32_t Http_GetAsync(const std::string& url, int timeoutMs, int retries);
bool Http_PollAsync(HttpAsyncResult& out);
}
