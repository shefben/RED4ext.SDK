#include "PhaseTriggerController.hpp"
#include "../core/Red4extUtils.hpp"
#include <RED4ext/RED4ext.hpp>
#include <unordered_map>
#include <vector>
#include <mutex>

namespace CoopNet
{

static std::unordered_map<uint32_t, std::vector<uint32_t>> g_phaseTriggerIds;
static std::mutex g_ptMutex;

void PhaseTrigger_Spawn(uint32_t baseEntId, uint32_t phaseId)
{
    RED4EXT_EXECUTE("TriggerSystem", "SpawnPhaseTrigger", nullptr, baseEntId, phaseId);
    std::lock_guard lock(g_ptMutex);
    g_phaseTriggerIds[phaseId].push_back(baseEntId);
}

void PhaseTrigger_Clear(uint32_t phaseId)
{
    std::vector<uint32_t> ids;
    {
        std::lock_guard lock(g_ptMutex);
        auto it = g_phaseTriggerIds.find(phaseId);
        if (it == g_phaseTriggerIds.end())
            return;
        ids = it->second;
        g_phaseTriggerIds.erase(it);
    }
    for (uint32_t id : ids)
        RED4EXT_EXECUTE("TriggerSystem", "DestroyTrigger", nullptr, id);
}

} // namespace CoopNet
