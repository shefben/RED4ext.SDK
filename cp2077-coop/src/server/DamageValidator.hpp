#pragma once
#include <cstdint>
#include "CombatStateManager.hpp"

namespace CoopNet
{
// Enhanced DamageValidator with CombatStateManager integration
class DamageValidator
{
public:
    static DamageValidator& GetInstance();

    // Enhanced damage validation with combat state awareness
    uint16_t FilterDamage(uint32_t sourcePeer, uint32_t targetPeer, bool targetIsNpc, uint16_t rawDmg, uint16_t targetArmor, bool invulnerable);
    bool ValidateDamageContext(uint32_t attackerId, uint32_t targetId, float damage) const;
    bool ValidateWeaponDamage(uint32_t attackerId, uint64_t weaponId, float damage) const;
    bool IsPlayerInCombatRange(uint32_t attackerId, uint32_t targetId) const;

    // Integration with CombatStateManager
    void ProcessDamageEvent(uint32_t attackerId, uint32_t targetId, float damage, bool isHeadshot = false, bool isCritical = false);
    void OnCombatStateChanged(uint32_t playerId, RED4ext::CombatState oldState, RED4ext::CombatState newState);

private:
    DamageValidator() = default;
    ~DamageValidator() = default;
    DamageValidator(const DamageValidator&) = delete;
    DamageValidator& operator=(const DamageValidator&) = delete;

    void Initialize();
    uint32_t PeerIdToPlayerId(uint32_t peerId) const;
    uint32_t PlayerIdToPeerId(uint32_t playerId) const;

    bool m_initialized = false;
};

// Legacy function wrapper for compatibility
uint16_t FilterDamage(uint32_t sourcePeer, uint32_t targetPeer, bool targetIsNpc, uint16_t rawDmg, uint16_t targetArmor, bool invulnerable);
}
