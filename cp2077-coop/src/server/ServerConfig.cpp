#include "ServerConfig.hpp"
#include <algorithm>
#include <fstream>
#include <string>

namespace CoopNet
{

bool g_cfgFriendlyFire = false;

static bool ParseBool(const std::string& s)
{
    std::string v = s;
    std::transform(v.begin(), v.end(), v.begin(), ::tolower);
    return v == "1" || v == "true" || v == "yes";
}

void ServerConfig_Load()
{
    g_cfgFriendlyFire = false;
    std::ifstream in("coop_dedicated.ini");
    if (!in.is_open())
        return;
    std::string line;
    while (std::getline(in, line))
    {
        size_t eq = line.find('=');
        if (eq == std::string::npos)
            continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        if (key == "friendly_fire")
            g_cfgFriendlyFire = ParseBool(val);
    }
}

} // namespace CoopNet
