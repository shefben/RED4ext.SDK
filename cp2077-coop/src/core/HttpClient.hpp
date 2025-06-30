#pragma once
#include <string>
namespace CoopNet {
struct HttpResponse {
    uint16_t status;
    std::string body;
};
HttpResponse Http_Get(const std::string& url);
HttpResponse Http_Post(const std::string& url, const std::string& body, const std::string& contentType);
}
