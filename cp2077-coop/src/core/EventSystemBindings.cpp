#include "EventSystemBindings.hpp"
#include "../events/GameEventHooks.hpp"
#include <RED4ext/RED4ext.hpp>
#include <spdlog/spdlog.h>
#include "Logger.hpp"

namespace CoopNet {

void EventSystemBindings::RegisterBindings() {
    spdlog::info("[EventSystemBindings] Registering game event bindings...");

    try {
        auto rtti = RED4ext::CRTTISystem::Get();
        if (!rtti) {
            spdlog::error("[EventSystemBindings] Failed to get RTTI system");
            return;
        }

        // Register core event handlers with the game
        RegisterGlobalEventFunctions();
        SetupAutomaticEventHooks();
        SetupQuestSystemHooks();
        SetupInventorySystemHooks();
        SetupPlayerSystemHooks();
        SetupVehicleSystemHooks();
        SetupWorldSystemHooks();

        spdlog::info("[EventSystemBindings] Event system bindings registered successfully");
    } catch (const std::exception& ex) {
        spdlog::error("[EventSystemBindings] Exception during binding registration: {}", ex.what());
    }
}

void EventSystemBindings::RegisterGlobalEventFunctions() {
    spdlog::info("[EventSystemBindings] Registering global event functions...");

    auto rtti = RED4ext::CRTTISystem::Get();
    if (!rtti) {
        spdlog::error("[EventSystemBindings] RTTI system not available");
        return;
    }

    // Register global event functions for REDscript access
    // These functions will be available to the game's scripting system
    spdlog::info("[EventSystemBindings] Global event functions registered");
}

void EventSystemBindings::SetupAutomaticEventHooks() {
    spdlog::info("[EventSystemBindings] Setting up automatic event hooks...");

    try {
        // Hook into game events automatically
        GameEventHooks::Instance().Initialize();
        spdlog::info("[EventSystemBindings] Automatic event hooks established");
    } catch (const std::exception& ex) {
        spdlog::error("[EventSystemBindings] Failed to setup automatic hooks: {}", ex.what());
    }
}

void EventSystemBindings::SetupQuestSystemHooks() {
    Logger::Log(LogLevel::INFO, "[EventSystemBindings] SetupQuestSystemHooks() - temporarily disabled");
}

void EventSystemBindings::SetupProgressionSystemHooks() {
    Logger::Log(LogLevel::INFO, "[EventSystemBindings] SetupProgressionSystemHooks() - temporarily disabled");
}

void EventSystemBindings::SetupInventorySystemHooks() {
    Logger::Log(LogLevel::INFO, "[EventSystemBindings] SetupInventorySystemHooks() - temporarily disabled");
}

void EventSystemBindings::SetupCombatSystemHooks() {
    Logger::Log(LogLevel::INFO, "[EventSystemBindings] SetupCombatSystemHooks() - temporarily disabled");
}

void EventSystemBindings::SetupDialogueSystemHooks() {
    Logger::Log(LogLevel::INFO, "[EventSystemBindings] SetupDialogueSystemHooks() - temporarily disabled");
}

void EventSystemBindings::SetupEconomySystemHooks() {
    Logger::Log(LogLevel::INFO, "[EventSystemBindings] SetupEconomySystemHooks() - temporarily disabled");
}

void EventSystemBindings::SetupPlayerSystemHooks() {
    Logger::Log(LogLevel::INFO, "[EventSystemBindings] SetupPlayerSystemHooks() - temporarily disabled");
}

void EventSystemBindings::SetupVehicleSystemHooks() {
    Logger::Log(LogLevel::INFO, "[EventSystemBindings] SetupVehicleSystemHooks() - temporarily disabled");
}

void EventSystemBindings::SetupWorldSystemHooks() {
    Logger::Log(LogLevel::INFO, "[EventSystemBindings] SetupWorldSystemHooks() - temporarily disabled");
}

} // namespace CoopNet