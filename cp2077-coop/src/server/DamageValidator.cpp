#include "DamageValidator.hpp"
#include "PerkController.hpp"
#include "ServerConfig.hpp"
#include <iostream>
#include <cmath>

namespace CoopNet
{
// Enhanced DamageValidator Implementation
DamageValidator& DamageValidator::GetInstance()
{
    static DamageValidator instance;
    return instance;
}

void DamageValidator::Initialize()
{
    if (m_initialized) {
        return;
    }

    // Set up integration with CombatStateManager
    auto& combatManager = RED4ext::CombatStateManager::GetInstance();
    combatManager.SetDamageDealtCallback(
        [this](uint32_t attackerId, const RED4ext::DamageDealtData& damageData) {
            this->ProcessDamageEvent(attackerId, static_cast<uint32_t>(damageData.targetId),
                                   damageData.damage, damageData.isHeadshot, damageData.isCritical);
        });

    combatManager.SetCombatStateChangedCallback(
        [this](uint32_t playerId, RED4ext::CombatState oldState, RED4ext::CombatState newState) {
            this->OnCombatStateChanged(playerId, oldState, newState);
        });

    m_initialized = true;
    std::cout << "[DamageValidator] Enhanced damage validator initialized with CombatStateManager integration" << std::endl;
}

uint16_t DamageValidator::FilterDamage(uint32_t sourcePeer, uint32_t targetPeer, bool targetIsNpc, uint16_t rawDmg, uint16_t targetArmor, bool invulnerable)
{
    if (!m_initialized) {
        Initialize();
    }

    if (invulnerable) {
        return 0;
    }

    if (!g_cfgFriendlyFire && (sourcePeer == targetPeer || !targetIsNpc)) {
        return 0;
    }

    // Enhanced validation using CombatStateManager
    uint32_t attackerId = PeerIdToPlayerId(sourcePeer);
    uint32_t targetId = PeerIdToPlayerId(targetPeer);

    // Check combat state validity
    if (!ValidateDamageContext(attackerId, targetId, static_cast<float>(rawDmg))) {
        std::cout << "[DamageValidator] Invalid damage context for attacker " << attackerId << " -> target " << targetId << std::endl;
        return 0;
    }

    // Check combat range
    if (!IsPlayerInCombatRange(attackerId, targetId)) {
        std::cout << "[DamageValidator] Players not in combat range: " << attackerId << " -> " << targetId << std::endl;
        return static_cast<uint16_t>(static_cast<float>(rawDmg) * 0.5f); // Reduce damage for long range
    }

    // Legacy perk-based calculations
    float mult = PerkController_GetHealthMult(targetPeer);
    if (PerkController_HasRelic(sourcePeer, 1000)) { // Data Tunneling
        rawDmg = static_cast<uint16_t>(static_cast<float>(rawDmg) * 1.1f);
    }

    uint16_t maxAllowed = static_cast<uint16_t>((targetArmor * 4 + 200) * mult);
    if (rawDmg > maxAllowed) {
        std::cout << "[DamageValidator] Damage limit exceeded: " << rawDmg << " > " << maxAllowed
                  << " (Attacker: " << attackerId << ", Target: " << targetId << ")" << std::endl;
        return maxAllowed;
    }

    return rawDmg;
}

bool DamageValidator::ValidateDamageContext(uint32_t attackerId, uint32_t targetId, float damage) const
{
    auto& combatManager = RED4ext::CombatStateManager::GetInstance();

    // Check if attacker exists and is in valid combat state
    auto* attackerState = combatManager.GetPlayerCombatState(attackerId);
    if (!attackerState) {
        return false;
    }

    // Attacker must be in combat to deal damage
    if (attackerState->localState.combatState == RED4ext::CombatState::OutOfCombat) {
        return false;
    }

    // Check if attacker has a weapon drawn
    if (!attackerState->localState.weaponDrawn) {
        return false;
    }

    // Validate damage amount is reasonable
    if (damage <= 0.0f || damage > 10000.0f) {
        return false;
    }

    return true;
}

bool DamageValidator::ValidateWeaponDamage(uint32_t attackerId, uint64_t weaponId, float damage) const
{
    auto& combatManager = RED4ext::CombatStateManager::GetInstance();

    auto* attackerState = combatManager.GetPlayerCombatState(attackerId);
    if (!attackerState) {
        return false;
    }

    // Check if player has this weapon
    auto weaponIt = attackerState->weapons.find(weaponId);
    if (weaponIt == attackerState->weapons.end()) {
        return false;
    }

    auto& weaponState = weaponIt->second;

    // Check weapon state validity
    if (!weaponState->isDrawn) {
        return false;
    }

    // Check ammo count for non-melee weapons
    if (weaponState->ammoCount == 0 && weaponState->maxAmmo > 0) {
        return false;
    }

    return true;
}

bool DamageValidator::IsPlayerInCombatRange(uint32_t attackerId, uint32_t targetId) const
{
    auto& combatManager = RED4ext::CombatStateManager::GetInstance();

    auto* attackerState = combatManager.GetPlayerCombatState(attackerId);
    auto* targetState = combatManager.GetPlayerCombatState(targetId);

    if (!attackerState || !targetState) {
        return false;
    }

    // Calculate distance between players
    float distance = RED4ext::CombatUtils::CalculateDistance(
        attackerState->localState.position,
        targetState->localState.position
    );

    // Maximum combat range (100 meters for most weapons)
    const float MAX_COMBAT_RANGE = 100.0f;

    return distance <= MAX_COMBAT_RANGE;
}

void DamageValidator::ProcessDamageEvent(uint32_t attackerId, uint32_t targetId, float damage, bool isHeadshot, bool isCritical)
{
    std::cout << "[DamageValidator] Processing damage event: Attacker " << attackerId
              << " -> Target " << targetId << " (Damage: " << damage;

    if (isHeadshot) {
        std::cout << ", Headshot";
    }
    if (isCritical) {
        std::cout << ", Critical";
    }
    std::cout << ")" << std::endl;

    // Additional damage event processing could go here
    // e.g., logging for anti-cheat analysis, updating player statistics, etc.
}

void DamageValidator::OnCombatStateChanged(uint32_t playerId, RED4ext::CombatState oldState, RED4ext::CombatState newState)
{
    std::cout << "[DamageValidator] Combat state changed for player " << playerId
              << ": " << static_cast<int>(oldState) << " -> " << static_cast<int>(newState) << std::endl;

    // Additional combat state change processing could go here
    // e.g., resetting damage validation parameters, updating combat metrics, etc.
}

uint32_t DamageValidator::PeerIdToPlayerId(uint32_t peerId) const
{
    // Simple mapping - in a real implementation, this would use a player registry
    return peerId;
}

uint32_t DamageValidator::PlayerIdToPeerId(uint32_t playerId) const
{
    // Simple mapping - in a real implementation, this would use a player registry
    return playerId;
}

// Legacy function wrapper for compatibility
uint16_t FilterDamage(uint32_t sourcePeer, uint32_t targetPeer, bool targetIsNpc, uint16_t rawDmg, uint16_t targetArmor, bool invulnerable)
{
    auto& validator = DamageValidator::GetInstance();
    return validator.FilterDamage(sourcePeer, targetPeer, targetIsNpc, rawDmg, targetArmor, invulnerable);
}

} // namespace CoopNet
