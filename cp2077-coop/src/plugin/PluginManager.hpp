#pragma once
#include <string>

namespace CoopNet {
struct PluginMetadata {
    std::string name;
    std::string version;
    std::string hash;
    uint16_t id{0};
};

void PluginManager_RegisterCommand(const std::string& name, const std::string& help,
                                   PyObject* func, const std::string& plugin);
bool PluginManager_IsEnabled(const std::string& plugin);
void PluginManager_LogException(const std::string& plugin);
const PluginMetadata* PluginManager_GetInfo(const std::string& name);
bool PluginManager_GetData(const std::string& name, uint16_t& id,
                           const std::vector<uint32_t>*& whitelist);

bool PluginManager_Init();
void PluginManager_Shutdown();
void PluginManager_Tick(float dt);
void PluginManager_DispatchEvent(const std::string& name, PyObject* dict);
bool PluginManager_HandleChat(uint32_t peerId, const std::string& msg, bool isAdmin);
}
