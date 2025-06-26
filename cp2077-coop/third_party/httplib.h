#ifndef CPP_HTTPLIB_H
#define CPP_HTTPLIB_H
#include <string>
namespace httplib {
struct Result { int status; std::string body; };
class SSLClient {
    std::string host; int port; bool valid;
public:
    SSLClient(const std::string& h, int p=443) : host(h), port(p), valid(true) {}
    Result Post(const char* path, const std::string& body, const char* type) const {
        (void)path;(void)body;(void)type;
        return {200, "{\"ok\":false}"};
    }
};
}
#endif
