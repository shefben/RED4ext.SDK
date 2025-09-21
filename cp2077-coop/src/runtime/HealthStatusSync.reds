// Real-time Health and Status Synchronization System
// Synchronizes player health, armor, status effects, and conditions across multiplayer sessions

// Health and Status Manager - coordinates all player health/status synchronization
public class HealthStatusManager extends ScriptableSystem {
    private static let s_instance: ref<HealthStatusManager>;
    private let m_playerHealthStates: array<ref<PlayerHealthState>>;
    private let m_statusEffectTracker: ref<StatusEffectTracker>;
    private let m_conditionMonitor: ref<ConditionMonitor>;
    private let m_syncTimer: Float = 0.0;
    private let m_healthSyncInterval: Float = 0.1; // 10 FPS for health updates
    private let m_lastHealthUpdate: Float = 0.0;
    private let m_localPlayer: wref<PlayerPuppet>;

    public static func GetInstance() -> ref<HealthStatusManager> {
        if !IsDefined(HealthStatusManager.s_instance) {
            HealthStatusManager.s_instance = new HealthStatusManager();
        }
        return HealthStatusManager.s_instance;
    }

    public func Initialize(player: ref<PlayerPuppet>) -> Void {
        this.m_localPlayer = player;
        this.InitializeSubsystems();
        LogChannel(n"HealthSync", s"[HealthSync] Health and Status Manager initialized");
    }

    private func InitializeSubsystems() -> Void {
        // Initialize status effect tracking
        this.m_statusEffectTracker = new StatusEffectTracker();
        this.m_statusEffectTracker.Initialize();

        // Initialize condition monitoring
        this.m_conditionMonitor = new ConditionMonitor();
        this.m_conditionMonitor.Initialize();

        LogChannel(n"HealthSync", s"[HealthSync] Health subsystems initialized");
    }

    public func OnUpdate(deltaTime: Float) -> Void {
        this.m_syncTimer += deltaTime;

        if this.m_syncTimer >= this.m_healthSyncInterval {
            this.SynchronizeLocalPlayerHealth();
            this.UpdateRemotePlayerHealth();
            this.m_statusEffectTracker.Update(deltaTime);
            this.m_conditionMonitor.Update(deltaTime);
            this.m_syncTimer = 0.0;
        }
    }

    private func SynchronizeLocalPlayerHealth() -> Void {
        if !IsDefined(this.m_localPlayer) {
            return;
        }

        let currentHealth = this.GetCurrentPlayerHealth();
        let currentArmor = this.GetCurrentPlayerArmor();
        let currentStamina = this.GetCurrentPlayerStamina();
        let healthPercentage = this.GetHealthPercentage();

        // Check if health changed significantly to avoid spam
        let currentTime = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));
        if this.HasHealthChanged(currentHealth, currentArmor, currentStamina) ||
           (currentTime - this.m_lastHealthUpdate) > 1.0 {

            let healthData = this.CreateHealthSyncData(currentHealth, currentArmor, currentStamina, healthPercentage);
            Net_SendHealthUpdate(healthData);
            this.m_lastHealthUpdate = currentTime;

            LogChannel(n"HealthSync", s"[HealthSync] Health sync: HP=" + currentHealth + " Armor=" + currentArmor + " Stamina=" + currentStamina);
        }
    }

    private func GetCurrentPlayerHealth() -> Float {
        if !IsDefined(this.m_localPlayer) {
            return 0.0;
        }

        let healthSystem = GameInstance.GetStatPoolsSystem(this.m_localPlayer.GetGame());
        let healthPool = healthSystem.GetStatPoolValue(Cast<StatsObjectID>(this.m_localPlayer.GetEntityID()), gamedataStatPoolType.Health);

        return healthPool.current;
    }

    private func GetCurrentPlayerArmor() -> Float {
        if !IsDefined(this.m_localPlayer) {
            return 0.0;
        }

        let statsSystem = GameInstance.GetStatsSystem(this.m_localPlayer.GetGame());
        return statsSystem.GetStatValue(Cast<StatsObjectID>(this.m_localPlayer.GetEntityID()), gamedataStatType.Armor);
    }

    private func GetCurrentPlayerStamina() -> Float {
        if !IsDefined(this.m_localPlayer) {
            return 0.0;
        }

        let healthSystem = GameInstance.GetStatPoolsSystem(this.m_localPlayer.GetGame());
        let staminaPool = healthSystem.GetStatPoolValue(Cast<StatsObjectID>(this.m_localPlayer.GetEntityID()), gamedataStatPoolType.Stamina);

        return staminaPool.current;
    }

    private func GetHealthPercentage() -> Float {
        if !IsDefined(this.m_localPlayer) {
            return 0.0;
        }

        let healthSystem = GameInstance.GetStatPoolsSystem(this.m_localPlayer.GetGame());
        let healthPool = healthSystem.GetStatPoolValue(Cast<StatsObjectID>(this.m_localPlayer.GetEntityID()), gamedataStatPoolType.Health);

        if healthPool.maximum > 0.0 {
            return healthPool.current / healthPool.maximum;
        }
        return 0.0;
    }

    private func HasHealthChanged(health: Float, armor: Float, stamina: Float) -> Bool {
        // Check if health values changed significantly since last sync
        let threshold: Float = 1.0; // 1 HP change threshold

        // This would compare against stored previous values
        // For now, assume change detection based on time intervals
        return true;
    }

    private func CreateHealthSyncData(health: Float, armor: Float, stamina: Float, healthPercentage: Float) -> HealthSyncData {
        let syncData: HealthSyncData;
        syncData.playerId = 1u; // Local player ID
        syncData.currentHealth = health;
        syncData.currentArmor = armor;
        syncData.currentStamina = stamina;
        syncData.healthPercentage = healthPercentage;
        syncData.timestamp = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

        // Add status conditions
        syncData.isInCombat = this.IsPlayerInCombat();
        syncData.isUnconscious = health <= 0.0;
        syncData.isBleeding = this.HasStatusEffect(gamedataStatusEffectType.Bleeding);
        syncData.isPoisoned = this.HasStatusEffect(gamedataStatusEffectType.Poisoned);
        syncData.isBurning = this.HasStatusEffect(gamedataStatusEffectType.Burning);
        syncData.isElectrified = this.HasStatusEffect(gamedataStatusEffectType.Electrified);

        return syncData;
    }

    private func IsPlayerInCombat() -> Bool {
        if !IsDefined(this.m_localPlayer) {
            return false;
        }

        let psmSystem = GameInstance.GetPlayerStateMachineSystem(this.m_localPlayer.GetGame());
        return psmSystem.IsInCombat(this.m_localPlayer.GetEntityID());
    }

    private func HasStatusEffect(effectType: gamedataStatusEffectType) -> Bool {
        if !IsDefined(this.m_localPlayer) {
            return false;
        }

        let statusEffectSystem = GameInstance.GetStatusEffectSystem(this.m_localPlayer.GetGame());
        return statusEffectSystem.HasStatusEffect(this.m_localPlayer.GetEntityID(), effectType);
    }

    private func UpdateRemotePlayerHealth() -> Void {
        for healthState in this.m_playerHealthStates {
            healthState.Update();
        }
    }

    // Network event handlers
    public func OnRemotePlayerHealthUpdate(healthData: HealthSyncData) -> Void {
        LogChannel(n"HealthSync", s"[HealthSync] Received health update for player: " + healthData.playerId);

        let playerState = this.FindOrCreatePlayerHealthState(healthData.playerId);
        playerState.UpdateFromSyncData(healthData);
    }

    private func FindOrCreatePlayerHealthState(playerId: Uint32) -> ref<PlayerHealthState> {
        // Find existing state
        for healthState in this.m_playerHealthStates {
            if healthState.GetPlayerId() == playerId {
                return healthState;
            }
        }

        // Create new state
        let newState = new PlayerHealthState();
        newState.Initialize(playerId);
        ArrayPush(this.m_playerHealthStates, newState);
        return newState;
    }

    public func OnRemotePlayerJoined(playerId: Uint32) -> Void {
        LogChannel(n"HealthSync", s"[HealthSync] Adding health state for player: " + playerId);
        this.FindOrCreatePlayerHealthState(playerId);
    }

    public func OnRemotePlayerLeft(playerId: Uint32) -> Void {
        LogChannel(n"HealthSync", s"[HealthSync] Removing health state for player: " + playerId);

        let index = -1;
        for i in Range(ArraySize(this.m_playerHealthStates)) {
            if this.m_playerHealthStates[i].GetPlayerId() == playerId {
                index = i;
                break;
            }
        }

        if index >= 0 {
            this.m_playerHealthStates[index].Cleanup();
            ArrayRemove(this.m_playerHealthStates, this.m_playerHealthStates[index]);
        }
    }

    // Emergency health synchronization for critical events
    public func SyncCriticalHealth() -> Void {
        LogChannel(n"HealthSync", s"[HealthSync] Critical health sync triggered");
        this.SynchronizeLocalPlayerHealth();
    }

    public func OnPlayerDowned() -> Void {
        LogChannel(n"HealthSync", s"[HealthSync] Player downed - syncing critical state");

        let criticalData = this.CreateCriticalHealthData();
        Net_SendCriticalHealthUpdate(criticalData);
    }

    public func OnPlayerRevived() -> Void {
        LogChannel(n"HealthSync", s"[HealthSync] Player revived - syncing recovery state");

        let recoveryData = this.CreateRecoveryHealthData();
        Net_SendHealthRecovery(recoveryData);
    }

    private func CreateCriticalHealthData() -> CriticalHealthData {
        let criticalData: CriticalHealthData;
        criticalData.playerId = 1u; // Local player ID
        criticalData.isUnconscious = true;
        criticalData.downedTime = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));
        criticalData.position = this.m_localPlayer.GetWorldPosition();
        criticalData.canBeRevived = true;
        return criticalData;
    }

    private func CreateRecoveryHealthData() -> RecoveryHealthData {
        let recoveryData: RecoveryHealthData;
        recoveryData.playerId = 1u; // Local player ID
        recoveryData.revivedHealth = this.GetCurrentPlayerHealth();
        recoveryData.reviveTime = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));
        return recoveryData;
    }
}

// Individual Player Health State Tracker
public class PlayerHealthState extends ScriptableComponent {
    private let m_playerId: Uint32;
    private let m_currentHealth: Float;
    private let m_currentArmor: Float;
    private let m_currentStamina: Float;
    private let m_healthPercentage: Float;
    private let m_lastUpdateTime: Float;

    // Status conditions
    private let m_isInCombat: Bool;
    private let m_isUnconscious: Bool;
    private let m_isBleeding: Bool;
    private let m_isPoisoned: Bool;
    private let m_isBurning: Bool;
    private let m_isElectrified: Bool;

    // Visual indicators
    private let m_healthBarWidget: wref<inkWidget>;
    private let m_statusIndicators: array<wref<inkWidget>>;

    public func Initialize(playerId: Uint32) -> Void {
        this.m_playerId = playerId;
        this.m_currentHealth = 100.0;
        this.m_currentArmor = 0.0;
        this.m_currentStamina = 100.0;
        this.m_healthPercentage = 1.0;
        this.m_lastUpdateTime = 0.0;

        this.InitializeStatusFlags();
        this.CreateVisualIndicators();
    }

    private func InitializeStatusFlags() -> Void {
        this.m_isInCombat = false;
        this.m_isUnconscious = false;
        this.m_isBleeding = false;
        this.m_isPoisoned = false;
        this.m_isBurning = false;
        this.m_isElectrified = false;
    }

    private func CreateVisualIndicators() -> Void {
        // Create visual health indicators for remote players
        // This would integrate with the UI system to show health bars above players
        LogChannel(n"HealthSync", s"[HealthSync] Creating visual indicators for player: " + this.m_playerId);
    }

    public func GetPlayerId() -> Uint32 {
        return this.m_playerId;
    }

    public func UpdateFromSyncData(syncData: HealthSyncData) -> Void {
        let previousHealth = this.m_currentHealth;

        this.m_currentHealth = syncData.currentHealth;
        this.m_currentArmor = syncData.currentArmor;
        this.m_currentStamina = syncData.currentStamina;
        this.m_healthPercentage = syncData.healthPercentage;
        this.m_lastUpdateTime = syncData.timestamp;

        // Update status conditions
        this.m_isInCombat = syncData.isInCombat;
        this.m_isUnconscious = syncData.isUnconscious;
        this.m_isBleeding = syncData.isBleeding;
        this.m_isPoisoned = syncData.isPoisoned;
        this.m_isBurning = syncData.isBurning;
        this.m_isElectrified = syncData.isElectrified;

        // Check for significant health changes
        if AbsF(this.m_currentHealth - previousHealth) > 10.0 {
            this.OnSignificantHealthChange(previousHealth, this.m_currentHealth);
        }

        // Update visual indicators
        this.UpdateVisualIndicators();
    }

    private func OnSignificantHealthChange(oldHealth: Float, newHealth: Float) -> Void {
        if newHealth < oldHealth {
            LogChannel(n"HealthSync", s"[HealthSync] Player " + this.m_playerId + " took " + (oldHealth - newHealth) + " damage");
            this.ShowDamageEffect();
        } else {
            LogChannel(n"HealthSync", s"[HealthSync] Player " + this.m_playerId + " healed " + (newHealth - oldHealth) + " health");
            this.ShowHealingEffect();
        }
    }

    private func ShowDamageEffect() -> Void {
        // Show visual damage effect for remote player
        // This could be screen shake, red flash, or damage numbers
    }

    private func ShowHealingEffect() -> Void {
        // Show visual healing effect for remote player
        // This could be green particles or healing numbers
    }

    private func UpdateVisualIndicators() -> Void {
        // Update health bar and status effect indicators above the player
        this.UpdateHealthBar();
        this.UpdateStatusEffectIndicators();
    }

    private func UpdateHealthBar() -> Void {
        // Update the visual health bar representation
        if IsDefined(this.m_healthBarWidget) {
            // Set health bar fill based on health percentage
            // Apply color coding based on health level (green > yellow > red)
        }
    }

    private func UpdateStatusEffectIndicators() -> Void {
        // Update status effect icons above player
        let activeEffects: array<String>;

        if this.m_isInCombat { ArrayPush(activeEffects, "combat"); }
        if this.m_isBleeding { ArrayPush(activeEffects, "bleeding"); }
        if this.m_isPoisoned { ArrayPush(activeEffects, "poisoned"); }
        if this.m_isBurning { ArrayPush(activeEffects, "burning"); }
        if this.m_isElectrified { ArrayPush(activeEffects, "electrified"); }

        // Update UI to show these effects
        LogChannel(n"HealthSync", s"[HealthSync] Player " + this.m_playerId + " active effects: " + ArraySize(activeEffects));
    }

    public func Update() -> Void {
        // Perform periodic updates on the health state
        let currentTime = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));
        let timeSinceUpdate = currentTime - this.m_lastUpdateTime;

        // If no update received for too long, mark as potentially disconnected
        if timeSinceUpdate > 5.0 {
            LogChannel(n"HealthSync", s"[HealthSync] No health update from player " + this.m_playerId + " for " + timeSinceUpdate + " seconds");
        }
    }

    public func Cleanup() -> Void {
        // Clean up visual indicators and resources
        if IsDefined(this.m_healthBarWidget) {
            this.m_healthBarWidget.SetVisible(false);
        }

        for indicator in this.m_statusIndicators {
            if IsDefined(indicator) {
                indicator.SetVisible(false);
            }
        }
    }

    // Getters for current state
    public func GetCurrentHealth() -> Float { return this.m_currentHealth; }
    public func GetCurrentArmor() -> Float { return this.m_currentArmor; }
    public func GetCurrentStamina() -> Float { return this.m_currentStamina; }
    public func GetHealthPercentage() -> Float { return this.m_healthPercentage; }
    public func IsInCombat() -> Bool { return this.m_isInCombat; }
    public func IsUnconscious() -> Bool { return this.m_isUnconscious; }
    public func HasBleeding() -> Bool { return this.m_isBleeding; }
    public func HasPoisoning() -> Bool { return this.m_isPoisoned; }
    public func IsBurning() -> Bool { return this.m_isBurning; }
    public func IsElectrified() -> Bool { return this.m_isElectrified; }
}

// Status Effect Tracker - monitors and synchronizes status effects
public class StatusEffectTracker extends ScriptableComponent {
    private let m_activeEffects: array<ref<ActiveStatusEffect>>;
    private let m_localPlayer: wref<PlayerPuppet>;
    private let m_lastEffectCheck: Float = 0.0;
    private let m_effectCheckInterval: Float = 0.5; // Check effects every 0.5 seconds

    public func Initialize() -> Void {
        LogChannel(n"HealthSync", s"[HealthSync] Status Effect Tracker initialized");
    }

    public func Update(deltaTime: Float) -> Void {
        let currentTime = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

        if (currentTime - this.m_lastEffectCheck) >= this.m_effectCheckInterval {
            this.CheckForStatusEffectChanges();
            this.CleanupExpiredEffects();
            this.m_lastEffectCheck = currentTime;
        }
    }

    private func CheckForStatusEffectChanges() -> Void {
        if !IsDefined(this.m_localPlayer) {
            return;
        }

        let statusEffectSystem = GameInstance.GetStatusEffectSystem(this.m_localPlayer.GetGame());

        // Check for new status effects
        let currentEffects = this.GetCurrentStatusEffects();

        for effectType in currentEffects {
            if !this.HasActiveEffect(effectType) {
                this.AddStatusEffect(effectType);
            }
        }
    }

    private func GetCurrentStatusEffects() -> array<gamedataStatusEffectType> {
        let effects: array<gamedataStatusEffectType>;

        if !IsDefined(this.m_localPlayer) {
            return effects;
        }

        let statusEffectSystem = GameInstance.GetStatusEffectSystem(this.m_localPlayer.GetGame());

        // Check for common status effects
        let effectsToCheck: array<gamedataStatusEffectType> = [
            gamedataStatusEffectType.Bleeding,
            gamedataStatusEffectType.Poisoned,
            gamedataStatusEffectType.Burning,
            gamedataStatusEffectType.Electrified,
            gamedataStatusEffectType.Stunned,
            gamedataStatusEffectType.Blinded,
            gamedataStatusEffectType.Slowed,
            gamedataStatusEffectType.Weakened,
            gamedataStatusEffectType.Berserker,
            gamedataStatusEffectType.Berserk
        ];

        for effectType in effectsToCheck {
            if statusEffectSystem.HasStatusEffect(this.m_localPlayer.GetEntityID(), effectType) {
                ArrayPush(effects, effectType);
            }
        }

        return effects;
    }

    private func HasActiveEffect(effectType: gamedataStatusEffectType) -> Bool {
        for activeEffect in this.m_activeEffects {
            if Equals(activeEffect.GetEffectType(), effectType) {
                return true;
            }
        }
        return false;
    }

    private func AddStatusEffect(effectType: gamedataStatusEffectType) -> Void {
        let newEffect = new ActiveStatusEffect();
        newEffect.Initialize(effectType);
        ArrayPush(this.m_activeEffects, newEffect);

        // Send status effect update to other players
        let effectData: StatusEffectData;
        effectData.playerId = 1u; // Local player ID
        effectData.effectType = effectType;
        effectData.isActive = true;
        effectData.timestamp = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

        Net_SendStatusEffectUpdate(effectData);

        LogChannel(n"HealthSync", s"[HealthSync] Status effect added: " + EnumValueToString("gamedataStatusEffectType", Cast<Int64>(EnumInt(effectType))));
    }

    private func RemoveStatusEffect(effectType: gamedataStatusEffectType) -> Void {
        let index = -1;
        for i in Range(ArraySize(this.m_activeEffects)) {
            if Equals(this.m_activeEffects[i].GetEffectType(), effectType) {
                index = i;
                break;
            }
        }

        if index >= 0 {
            ArrayRemove(this.m_activeEffects, this.m_activeEffects[index]);

            // Send status effect removal to other players
            let effectData: StatusEffectData;
            effectData.playerId = 1u; // Local player ID
            effectData.effectType = effectType;
            effectData.isActive = false;
            effectData.timestamp = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

            Net_SendStatusEffectUpdate(effectData);

            LogChannel(n"HealthSync", s"[HealthSync] Status effect removed: " + EnumValueToString("gamedataStatusEffectType", Cast<Int64>(EnumInt(effectType))));
        }
    }

    private func CleanupExpiredEffects() -> Void {
        if !IsDefined(this.m_localPlayer) {
            return;
        }

        let statusEffectSystem = GameInstance.GetStatusEffectSystem(this.m_localPlayer.GetGame());
        let expiredEffects: array<ref<ActiveStatusEffect>>;

        for activeEffect in this.m_activeEffects {
            if !statusEffectSystem.HasStatusEffect(this.m_localPlayer.GetEntityID(), activeEffect.GetEffectType()) {
                ArrayPush(expiredEffects, activeEffect);
            }
        }

        for expiredEffect in expiredEffects {
            this.RemoveStatusEffect(expiredEffect.GetEffectType());
        }
    }
}

// Active Status Effect representation
public class ActiveStatusEffect extends ScriptableComponent {
    private let m_effectType: gamedataStatusEffectType;
    private let m_startTime: Float;

    public func Initialize(effectType: gamedataStatusEffectType) -> Void {
        this.m_effectType = effectType;
        this.m_startTime = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));
    }

    public func GetEffectType() -> gamedataStatusEffectType {
        return this.m_effectType;
    }

    public func GetStartTime() -> Float {
        return this.m_startTime;
    }
}

// Condition Monitor - tracks player conditions and states
public class ConditionMonitor extends ScriptableComponent {
    private let m_localPlayer: wref<PlayerPuppet>;
    private let m_lastConditionCheck: Float = 0.0;
    private let m_conditionCheckInterval: Float = 1.0; // Check conditions every 1 second

    public func Initialize() -> Void {
        LogChannel(n"HealthSync", s"[HealthSync] Condition Monitor initialized");
    }

    public func Update(deltaTime: Float) -> Void {
        let currentTime = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

        if (currentTime - this.m_lastConditionCheck) >= this.m_conditionCheckInterval {
            this.MonitorPlayerConditions();
            this.m_lastConditionCheck = currentTime;
        }
    }

    private func MonitorPlayerConditions() -> Void {
        if !IsDefined(this.m_localPlayer) {
            return;
        }

        // Monitor various player conditions
        let conditions = this.GatherPlayerConditions();

        // Send condition update if changes detected
        if this.HasConditionsChanged(conditions) {
            Net_SendConditionUpdate(conditions);
        }
    }

    private func GatherPlayerConditions() -> PlayerConditions {
        let conditions: PlayerConditions;
        conditions.playerId = 1u; // Local player ID

        if !IsDefined(this.m_localPlayer) {
            return conditions;
        }

        // Gather condition information
        conditions.isMoving = this.IsPlayerMoving();
        conditions.isSprinting = this.IsPlayerSprinting();
        conditions.isCrouching = this.IsPlayerCrouching();
        conditions.isAiming = this.IsPlayerAiming();
        conditions.isInVehicle = this.IsPlayerInVehicle();
        conditions.isSwimming = this.IsPlayerSwimming();
        conditions.isClimbing = this.IsPlayerClimbing();
        conditions.currentStance = this.GetPlayerStance();
        conditions.timestamp = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

        return conditions;
    }

    private func IsPlayerMoving() -> Bool {
        if !IsDefined(this.m_localPlayer) {
            return false;
        }

        let velocity = this.m_localPlayer.GetMoveComponent().GetLinearVelocity();
        return Vector4.Length(velocity) > 0.1;
    }

    private func IsPlayerSprinting() -> Bool {
        if !IsDefined(this.m_localPlayer) {
            return false;
        }

        let psmSystem = GameInstance.GetPlayerStateMachineSystem(this.m_localPlayer.GetGame());
        return psmSystem.IsInLocomotion(this.m_localPlayer.GetEntityID(), n"sprint");
    }

    private func IsPlayerCrouching() -> Bool {
        if !IsDefined(this.m_localPlayer) {
            return false;
        }

        let psmSystem = GameInstance.GetPlayerStateMachineSystem(this.m_localPlayer.GetGame());
        return psmSystem.IsInBodyState(this.m_localPlayer.GetEntityID(), n"crouch");
    }

    private func IsPlayerAiming() -> Bool {
        if !IsDefined(this.m_localPlayer) {
            return false;
        }

        let psmSystem = GameInstance.GetPlayerStateMachineSystem(this.m_localPlayer.GetGame());
        return psmSystem.IsInCombat(this.m_localPlayer.GetEntityID());
    }

    private func IsPlayerInVehicle() -> Bool {
        if !IsDefined(this.m_localPlayer) {
            return false;
        }

        let psmSystem = GameInstance.GetPlayerStateMachineSystem(this.m_localPlayer.GetGame());
        return psmSystem.IsInVehicle(this.m_localPlayer.GetEntityID());
    }

    private func IsPlayerSwimming() -> Bool {
        if !IsDefined(this.m_localPlayer) {
            return false;
        }

        let psmSystem = GameInstance.GetPlayerStateMachineSystem(this.m_localPlayer.GetGame());
        return psmSystem.IsInSwimming(this.m_localPlayer.GetEntityID());
    }

    private func IsPlayerClimbing() -> Bool {
        if !IsDefined(this.m_localPlayer) {
            return false;
        }

        let psmSystem = GameInstance.GetPlayerStateMachineSystem(this.m_localPlayer.GetGame());
        return psmSystem.IsInClimbing(this.m_localPlayer.GetEntityID());
    }

    private func GetPlayerStance() -> EPlayerStance {
        if !IsDefined(this.m_localPlayer) {
            return EPlayerStance.Standing;
        }

        if this.IsPlayerCrouching() {
            return EPlayerStance.Crouching;
        } else if this.IsPlayerAiming() {
            return EPlayerStance.Combat;
        }

        return EPlayerStance.Standing;
    }

    private func HasConditionsChanged(conditions: PlayerConditions) -> Bool {
        // Compare with previous conditions to detect changes
        // For now, assume conditions change each update
        return true;
    }
}

// Data Structures
public struct HealthSyncData {
    public let playerId: Uint32;
    public let currentHealth: Float;
    public let currentArmor: Float;
    public let currentStamina: Float;
    public let healthPercentage: Float;
    public let timestamp: Float;
    public let isInCombat: Bool;
    public let isUnconscious: Bool;
    public let isBleeding: Bool;
    public let isPoisoned: Bool;
    public let isBurning: Bool;
    public let isElectrified: Bool;
}

public struct CriticalHealthData {
    public let playerId: Uint32;
    public let isUnconscious: Bool;
    public let downedTime: Float;
    public let position: Vector3;
    public let canBeRevived: Bool;
}

public struct RecoveryHealthData {
    public let playerId: Uint32;
    public let revivedHealth: Float;
    public let reviveTime: Float;
}

public struct StatusEffectData {
    public let playerId: Uint32;
    public let effectType: gamedataStatusEffectType;
    public let isActive: Bool;
    public let timestamp: Float;
}

public struct PlayerConditions {
    public let playerId: Uint32;
    public let isMoving: Bool;
    public let isSprinting: Bool;
    public let isCrouching: Bool;
    public let isAiming: Bool;
    public let isInVehicle: Bool;
    public let isSwimming: Bool;
    public let isClimbing: Bool;
    public let currentStance: EPlayerStance;
    public let timestamp: Float;
}

public enum EPlayerStance : Uint8 {
    Standing = 0,
    Crouching = 1,
    Prone = 2,
    Combat = 3,
    Vehicle = 4
}

// Native function declarations for network integration
native func Net_SendHealthUpdate(healthData: HealthSyncData) -> Void;
native func Net_SendCriticalHealthUpdate(criticalData: CriticalHealthData) -> Void;
native func Net_SendHealthRecovery(recoveryData: RecoveryHealthData) -> Void;
native func Net_SendStatusEffectUpdate(effectData: StatusEffectData) -> Void;
native func Net_SendConditionUpdate(conditions: PlayerConditions) -> Void;

// Integration with game systems
@wrapMethod(PlayerPuppet)
protected cb func OnStatusEffectApplied(evt: ref<ApplyStatusEffectEvent>) -> Bool {
    let result = wrappedMethod(evt);

    // Notify health sync system of status effect change
    let healthManager = HealthStatusManager.GetInstance();
    if IsDefined(healthManager) {
        healthManager.SyncCriticalHealth();
    }

    return result;
}

@wrapMethod(PlayerPuppet)
protected cb func OnStatusEffectRemoved(evt: ref<RemoveStatusEffect>) -> Bool {
    let result = wrappedMethod(evt);

    // Notify health sync system of status effect removal
    let healthManager = HealthStatusManager.GetInstance();
    if IsDefined(healthManager) {
        healthManager.SyncCriticalHealth();
    }

    return result;
}

@wrapMethod(PlayerPuppet)
protected cb func OnHit(hitEvent: ref<gameHitEvent>) -> Bool {
    let result = wrappedMethod(hitEvent);

    // Immediate health sync on damage
    let healthManager = HealthStatusManager.GetInstance();
    if IsDefined(healthManager) {
        healthManager.SyncCriticalHealth();
    }

    return result;
}

@wrapMethod(PlayerPuppet)
protected cb func OnDeath(evt: ref<gameDeathEvent>) -> Bool {
    let result = wrappedMethod(evt);

    // Handle player death synchronization
    let healthManager = HealthStatusManager.GetInstance();
    if IsDefined(healthManager) {
        healthManager.OnPlayerDowned();
    }

    return result;
}

// Callback functions for receiving network events
@addMethod(PlayerPuppet)
public func OnNetworkHealthUpdate(healthData: HealthSyncData) -> Void {
    HealthStatusManager.GetInstance().OnRemotePlayerHealthUpdate(healthData);
}

@addMethod(PlayerPuppet)
public func OnNetworkStatusEffectUpdate(effectData: StatusEffectData) -> Void {
    LogChannel(n"HealthSync", s"[HealthSync] Received status effect update for player: " + effectData.playerId);
}