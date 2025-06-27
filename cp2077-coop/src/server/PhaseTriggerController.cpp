#include "PhaseTriggerController.hpp"
#include <RED4ext/RED4ext.hpp>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace CoopNet
{

static std::unordered_map<uint32_t, std::vector<uint32_t>> g_phaseTriggerIds;

void PhaseTrigger_Spawn(uint32_t baseEntId, uint32_t phaseId)
{
    RED4ext::ExecuteFunction("TriggerSystem", "SpawnPhaseTrigger", nullptr, baseEntId, phaseId);
    g_phaseTriggerIds[phaseId].push_back(baseEntId);
}

void PhaseTrigger_Clear(uint32_t phaseId)
{
    auto it = g_phaseTriggerIds.find(phaseId);
    if (it == g_phaseTriggerIds.end())
        return;
    for (uint32_t id : it->second)
        RED4ext::ExecuteFunction("TriggerSystem", "DestroyTrigger", nullptr, id);
    g_phaseTriggerIds.erase(it);
}

} // namespace CoopNet
