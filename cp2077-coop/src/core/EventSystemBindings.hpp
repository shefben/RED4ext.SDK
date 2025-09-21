#pragma once

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/Scripting/Functions.hpp>
#include <string>
#include <vector>
#include <unordered_map>

namespace CoopNet {

// Event system bindings for automatic campaign event integration
class EventSystemBindings {
public:
    // Register all native function bindings
    static void RegisterBindings();

private:
    // Core binding registration (temporarily disabled)
    // static void RegisterNativeFunction(RED4ext::CClass* scriptClass,
    //                                  const std::string& functionName,
    //                                  RED4ext::CClassFunction::ScriptedFunction function,
    //                                  const std::vector<RED4ext::CBaseRTTIType*>& returnTypes = {});

    static void RegisterGlobalEventFunctions();

    // Automatic event discovery and hooking
    static void SetupAutomaticEventHooks();
    static void SetupQuestSystemHooks();
    static void SetupProgressionSystemHooks();
    static void SetupInventorySystemHooks();
    static void SetupCombatSystemHooks();
    static void SetupDialogueSystemHooks();
    static void SetupEconomySystemHooks();
    static void SetupPlayerSystemHooks();
    static void SetupVehicleSystemHooks();
    static void SetupWorldSystemHooks();

    // Native function implementations for CampaignEventHooks class (temporarily disabled)
    // static bool CampaignEventHooks_InitializeNativeHooks(
    //     RED4ext::IScriptable* context, RED4ext::CStackFrame* frame, bool* ret, int flags);

    // static void CampaignEventHooks_ShutdownNativeHooks(
    //     RED4ext::IScriptable* context, RED4ext::CStackFrame* frame, void* ret, int flags);

    // static void CampaignEventHooks_TriggerNativeEvent(
    //     RED4ext::IScriptable* context, RED4ext::CStackFrame* frame, void* ret, int flags);

    // Global function implementations (temporarily disabled)
    // static void Global_TriggerCampaignEvent(
    //     RED4ext::IScriptable* context, RED4ext::CStackFrame* frame, void* ret, int flags);

    // static bool Global_GetCampaignEventStats(
    //     RED4ext::IScriptable* context, RED4ext::CStackFrame* frame, bool* ret, int flags);

    // Utility functions (temporarily disabled)
    // static uint32_t GetPlayerIdFromContext(RED4ext::IScriptable* context);

    // Specific event hook implementations
    static void HookQuestCompletion();
    static void HookQuestStart();
    static void HookQuestObjectiveUpdate();
    static void HookLevelProgression();
    static void HookAttributeProgression();
    static void HookPerkUnlock();
    static void HookSkillProgression();
    static void HookItemAcquisition();
    static void HookItemLoss();
    static void HookItemCrafting();
    static void HookCombatStateChanges();
    static void HookEnemyDefeated();
    static void HookPlayerDeath();
    static void HookWeaponUsage();
    static void HookDialogueChoice();
    static void HookRomanceProgression();
    static void HookNPCInteraction();
    static void HookCurrencyChanges();
    static void HookShopTransactions();
    static void HookVehiclePurchases();
    static void HookLocationDiscovery();
    static void HookFastTravelUnlock();
    static void HookVehicleAcquisition();
    static void HookWorldInteraction();
};

} // namespace CoopNet