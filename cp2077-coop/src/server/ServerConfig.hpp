#pragma once
#include <cstdint>
#include <string>
namespace CoopNet
{
extern bool g_cfgFriendlyFire;
extern std::string g_cfgMasterHost;
extern int g_cfgMasterPort;
void ServerConfig_Load();
} // namespace CoopNet
