// Buff and Debuff Synchronization System
// Synchronizes all temporary effects, boosts, penalties, and modifiers across multiplayer sessions

// Buff/Debuff Synchronization Manager - coordinates all temporary effect synchronization
public class BuffDebuffManager extends ScriptableSystem {
    private static let s_instance: ref<BuffDebuffManager>;
    private let m_activeBuffs: array<ref<ActiveBuff>>;
    private let m_activeDebuffs: array<ref<ActiveDebuff>>;
    private let m_playerEffectStates: array<ref<PlayerEffectState>>;
    private let m_effectTracker: ref<EffectTracker>;
    private let m_syncTimer: Float = 0.0;
    private let m_effectSyncInterval: Float = 0.2; // 5 FPS for effect updates
    private let m_localPlayer: wref<PlayerPuppet>;

    public static func GetInstance() -> ref<BuffDebuffManager> {
        if !IsDefined(BuffDebuffManager.s_instance) {
            BuffDebuffManager.s_instance = new BuffDebuffManager();
        }
        return BuffDebuffManager.s_instance;
    }

    public func Initialize(player: ref<PlayerPuppet>) -> Void {
        this.m_localPlayer = player;
        this.InitializeEffectTracking();
        LogChannel(n"BuffSync", s"[BuffSync] Buff/Debuff Manager initialized");
    }

    private func InitializeEffectTracking() -> Void {
        this.m_effectTracker = new EffectTracker();
        this.m_effectTracker.Initialize(this.m_localPlayer);

        LogChannel(n"BuffSync", s"[BuffSync] Effect tracking initialized");
    }

    public func OnUpdate(deltaTime: Float) -> Void {
        this.m_syncTimer += deltaTime;

        if this.m_syncTimer >= this.m_effectSyncInterval {
            this.SynchronizeLocalEffects();
            this.UpdateRemoteEffects();
            this.CleanupExpiredEffects();
            this.m_effectTracker.Update(deltaTime);
            this.m_syncTimer = 0.0;
        }
    }

    private func SynchronizeLocalEffects() -> Void {
        if !IsDefined(this.m_localPlayer) {
            return;
        }

        // Gather current buffs and debuffs
        let currentBuffs = this.GatherActiveBuffs();
        let currentDebuffs = this.GatherActiveDebuffs();

        // Check for new buffs
        for buffType in currentBuffs {
            if !this.HasActiveBuff(buffType) {
                this.AddBuff(buffType);
            }
        }

        // Check for new debuffs
        for debuffType in currentDebuffs {
            if !this.HasActiveDebuff(debuffType) {
                this.AddDebuff(debuffType);
            }
        }
    }

    private func GatherActiveBuffs() -> array<EBuffType> {
        let buffs: array<EBuffType>;

        if !IsDefined(this.m_localPlayer) {
            return buffs;
        }

        let statsSystem = GameInstance.GetStatsSystem(this.m_localPlayer.GetGame());
        let playerID = Cast<StatsObjectID>(this.m_localPlayer.GetEntityID());

        // Check for various buff types
        if this.HasStatBoost(gamedataStatType.Strength) { ArrayPush(buffs, EBuffType.StrengthBoost); }
        if this.HasStatBoost(gamedataStatType.Reflexes) { ArrayPush(buffs, EBuffType.ReflexesBoost); }
        if this.HasStatBoost(gamedataStatType.TechnicalAbility) { ArrayPush(buffs, EBuffType.TechAbilityBoost); }
        if this.HasStatBoost(gamedataStatType.Intelligence) { ArrayPush(buffs, EBuffType.IntelligenceBoost); }
        if this.HasStatBoost(gamedataStatType.Cool) { ArrayPush(buffs, EBuffType.CoolBoost); }

        // Combat buffs
        if this.HasDamageBoost() { ArrayPush(buffs, EBuffType.DamageBoost); }
        if this.HasArmorBoost() { ArrayPush(buffs, EBuffType.ArmorBoost); }
        if this.HasSpeedBoost() { ArrayPush(buffs, EBuffType.SpeedBoost); }
        if this.HasStealthBoost() { ArrayPush(buffs, EBuffType.StealthBoost); }

        // Consumable buffs
        if this.HasConsumableBuff(n"alcohol_buff") { ArrayPush(buffs, EBuffType.AlcoholBuff); }
        if this.HasConsumableBuff(n"stim_buff") { ArrayPush(buffs, EBuffType.StimulantBuff); }
        if this.HasConsumableBuff(n"food_buff") { ArrayPush(buffs, EBuffType.FoodBuff); }

        // Cyberware buffs
        if this.HasCyberwareBuff() { ArrayPush(buffs, EBuffType.CyberwareBoost); }

        return buffs;
    }

    private func GatherActiveDebuffs() -> array<EDebuffType> {
        let debuffs: array<EDebuffType>;

        if !IsDefined(this.m_localPlayer) {
            return debuffs;
        }

        let statusSystem = GameInstance.GetStatusEffectSystem(this.m_localPlayer.GetGame());
        let playerID = this.m_localPlayer.GetEntityID();

        // Status effect debuffs
        if statusSystem.HasStatusEffect(playerID, gamedataStatusEffectType.Bleeding) {
            ArrayPush(debuffs, EDebuffType.Bleeding);
        }
        if statusSystem.HasStatusEffect(playerID, gamedataStatusEffectType.Poisoned) {
            ArrayPush(debuffs, EDebuffType.Poisoned);
        }
        if statusSystem.HasStatusEffect(playerID, gamedataStatusEffectType.Burning) {
            ArrayPush(debuffs, EDebuffType.Burning);
        }
        if statusSystem.HasStatusEffect(playerID, gamedataStatusEffectType.Electrified) {
            ArrayPush(debuffs, EDebuffType.Electrified);
        }
        if statusSystem.HasStatusEffect(playerID, gamedataStatusEffectType.Stunned) {
            ArrayPush(debuffs, EDebuffType.Stunned);
        }
        if statusSystem.HasStatusEffect(playerID, gamedataStatusEffectType.Blinded) {
            ArrayPush(debuffs, EDebuffType.Blinded);
        }
        if statusSystem.HasStatusEffect(playerID, gamedataStatusEffectType.Slowed) {
            ArrayPush(debuffs, EDebuffType.Slowed);
        }
        if statusSystem.HasStatusEffect(playerID, gamedataStatusEffectType.Weakened) {
            ArrayPush(debuffs, EDebuffType.Weakened);
        }

        // Environmental debuffs
        if this.HasEnvironmentalDebuff(n"radiation") { ArrayPush(debuffs, EDebuffType.Radiation); }
        if this.HasEnvironmentalDebuff(n"toxic_air") { ArrayPush(debuffs, EDebuffType.ToxicAir); }
        if this.HasEnvironmentalDebuff(n"extreme_heat") { ArrayPush(debuffs, EDebuffType.ExtremeHeat); }
        if this.HasEnvironmentalDebuff(n"extreme_cold") { ArrayPush(debuffs, EDebuffType.ExtremeCold); }

        // Combat debuffs
        if this.HasCombatDebuff(n"suppressed") { ArrayPush(debuffs, EDebuffType.Suppressed); }
        if this.HasCombatDebuff(n"disoriented") { ArrayPush(debuffs, EDebuffType.Disoriented); }
        if this.HasCombatDebuff(n"overheated") { ArrayPush(debuffs, EDebuffType.Overheated); }

        // Drug/Alcohol debuffs
        if this.HasSubstanceDebuff(n"alcohol_penalty") { ArrayPush(debuffs, EDebuffType.AlcoholPenalty); }
        if this.HasSubstanceDebuff(n"drug_crash") { ArrayPush(debuffs, EDebuffType.DrugCrash); }
        if this.HasSubstanceDebuff(n"withdrawal") { ArrayPush(debuffs, EDebuffType.Withdrawal); }

        return debuffs;
    }

    // Helper methods for detecting specific effects
    private func HasStatBoost(statType: gamedataStatType) -> Bool {
        let statsSystem = GameInstance.GetStatsSystem(this.m_localPlayer.GetGame());
        let baseValue = statsSystem.GetStatValue(Cast<StatsObjectID>(this.m_localPlayer.GetEntityID()), statType);

        // This is a simplified check - in practice would compare against base stats
        return false; // Placeholder
    }

    private func HasDamageBoost() -> Bool {
        // Check for temporary damage modifiers
        return false; // Placeholder
    }

    private func HasArmorBoost() -> Bool {
        // Check for temporary armor bonuses
        return false; // Placeholder
    }

    private func HasSpeedBoost() -> Bool {
        // Check for movement speed buffs
        return false; // Placeholder
    }

    private func HasStealthBoost() -> Bool {
        // Check for stealth enhancement effects
        return false; // Placeholder
    }

    private func HasConsumableBuff(buffName: CName) -> Bool {
        // Check for consumable item effects
        return false; // Placeholder - would check active item effects
    }

    private func HasCyberwareBuff() -> Bool {
        // Check for active cyberware enhancement effects
        return false; // Placeholder
    }

    private func HasEnvironmentalDebuff(debuffName: CName) -> Bool {
        // Check for environmental status effects
        return false; // Placeholder
    }

    private func HasCombatDebuff(debuffName: CName) -> Bool {
        // Check for combat-related debuffs
        return false; // Placeholder
    }

    private func HasSubstanceDebuff(debuffName: CName) -> Bool {
        // Check for substance-related penalties
        return false; // Placeholder
    }

    private func HasActiveBuff(buffType: EBuffType) -> Bool {
        for activeBuff in this.m_activeBuffs {
            if Equals(activeBuff.GetBuffType(), buffType) {
                return true;
            }
        }
        return false;
    }

    private func HasActiveDebuff(debuffType: EDebuffType) -> Bool {
        for activeDebuff in this.m_activeDebuffs {
            if Equals(activeDebuff.GetDebuffType(), debuffType) {
                return true;
            }
        }
        return false;
    }

    private func AddBuff(buffType: EBuffType) -> Void {
        let newBuff = new ActiveBuff();
        newBuff.Initialize(buffType, this.GetBuffDuration(buffType), this.GetBuffIntensity(buffType));
        ArrayPush(this.m_activeBuffs, newBuff);

        // Send buff update to other players
        let buffData: BuffData;
        buffData.playerId = 1u; // Local player ID
        buffData.buffType = buffType;
        buffData.isActive = true;
        buffData.duration = newBuff.GetDuration();
        buffData.intensity = newBuff.GetIntensity();
        buffData.timestamp = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

        Net_SendBuffUpdate(buffData);

        LogChannel(n"BuffSync", s"[BuffSync] Buff added: " + EnumValueToString("EBuffType", Cast<Int64>(EnumInt(buffType))));
    }

    private func AddDebuff(debuffType: EDebuffType) -> Void {
        let newDebuff = new ActiveDebuff();
        newDebuff.Initialize(debuffType, this.GetDebuffDuration(debuffType), this.GetDebuffIntensity(debuffType));
        ArrayPush(this.m_activeDebuffs, newDebuff);

        // Send debuff update to other players
        let debuffData: DebuffData;
        debuffData.playerId = 1u; // Local player ID
        debuffData.debuffType = debuffType;
        debuffData.isActive = true;
        debuffData.duration = newDebuff.GetDuration();
        debuffData.intensity = newDebuff.GetIntensity();
        debuffData.timestamp = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

        Net_SendDebuffUpdate(debuffData);

        LogChannel(n"BuffSync", s"[BuffSync] Debuff added: " + EnumValueToString("EDebuffType", Cast<Int64>(EnumInt(debuffType))));
    }

    private func GetBuffDuration(buffType: EBuffType) -> Float {
        // Return duration based on buff type
        switch buffType {
            case EBuffType.StrengthBoost: return 300.0; // 5 minutes
            case EBuffType.DamageBoost: return 180.0; // 3 minutes
            case EBuffType.SpeedBoost: return 120.0; // 2 minutes
            case EBuffType.StimulantBuff: return 240.0; // 4 minutes
            case EBuffType.AlcoholBuff: return 600.0; // 10 minutes
            default: return 60.0; // Default 1 minute
        }
    }

    private func GetDebuffDuration(debuffType: EDebuffType) -> Float {
        // Return duration based on debuff type
        switch debuffType {
            case EDebuffType.Bleeding: return 30.0; // 30 seconds
            case EDebuffType.Poisoned: return 45.0; // 45 seconds
            case EDebuffType.Burning: return 15.0; // 15 seconds
            case EDebuffType.Stunned: return 5.0; // 5 seconds
            case EDebuffType.AlcoholPenalty: return 900.0; // 15 minutes
            default: return 30.0; // Default 30 seconds
        }
    }

    private func GetBuffIntensity(buffType: EBuffType) -> Float {
        // Return intensity/strength of the buff effect
        switch buffType {
            case EBuffType.StrengthBoost: return 1.25; // 25% increase
            case EBuffType.DamageBoost: return 1.5; // 50% increase
            case EBuffType.SpeedBoost: return 1.3; // 30% increase
            case EBuffType.ArmorBoost: return 1.4; // 40% increase
            default: return 1.1; // Default 10% increase
        }
    }

    private func GetDebuffIntensity(debuffType: EDebuffType) -> Float {
        // Return intensity/severity of the debuff effect
        switch debuffType {
            case EDebuffType.Bleeding: return 2.0; // 2 HP/sec damage
            case EDebuffType.Poisoned: return 1.5; // 1.5 HP/sec damage
            case EDebuffType.Burning: return 5.0; // 5 HP/sec damage
            case EDebuffType.Slowed: return 0.7; // 30% speed reduction
            case EDebuffType.Weakened: return 0.8; // 20% damage reduction
            default: return 1.0; // Default intensity
        }
    }

    private func UpdateRemoteEffects() -> Void {
        for effectState in this.m_playerEffectStates {
            effectState.Update();
        }
    }

    private func CleanupExpiredEffects() -> Void {
        let currentTime = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));
        let expiredBuffs: array<ref<ActiveBuff>>;
        let expiredDebuffs: array<ref<ActiveDebuff>>;

        // Check for expired buffs
        for activeBuff in this.m_activeBuffs {
            if activeBuff.IsExpired(currentTime) {
                ArrayPush(expiredBuffs, activeBuff);
            }
        }

        // Check for expired debuffs
        for activeDebuff in this.m_activeDebuffs {
            if activeDebuff.IsExpired(currentTime) {
                ArrayPush(expiredDebuffs, activeDebuff);
            }
        }

        // Remove expired buffs
        for expiredBuff in expiredBuffs {
            this.RemoveBuff(expiredBuff.GetBuffType());
        }

        // Remove expired debuffs
        for expiredDebuff in expiredDebuffs {
            this.RemoveDebuff(expiredDebuff.GetDebuffType());
        }
    }

    private func RemoveBuff(buffType: EBuffType) -> Void {
        let index = -1;
        for i in Range(ArraySize(this.m_activeBuffs)) {
            if Equals(this.m_activeBuffs[i].GetBuffType(), buffType) {
                index = i;
                break;
            }
        }

        if index >= 0 {
            ArrayRemove(this.m_activeBuffs, this.m_activeBuffs[index]);

            // Send buff removal to other players
            let buffData: BuffData;
            buffData.playerId = 1u; // Local player ID
            buffData.buffType = buffType;
            buffData.isActive = false;
            buffData.timestamp = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

            Net_SendBuffUpdate(buffData);

            LogChannel(n"BuffSync", s"[BuffSync] Buff removed: " + EnumValueToString("EBuffType", Cast<Int64>(EnumInt(buffType))));
        }
    }

    private func RemoveDebuff(debuffType: EDebuffType) -> Void {
        let index = -1;
        for i in Range(ArraySize(this.m_activeDebuffs)) {
            if Equals(this.m_activeDebuffs[i].GetDebuffType(), debuffType) {
                index = i;
                break;
            }
        }

        if index >= 0 {
            ArrayRemove(this.m_activeDebuffs, this.m_activeDebuffs[index]);

            // Send debuff removal to other players
            let debuffData: DebuffData;
            debuffData.playerId = 1u; // Local player ID
            debuffData.debuffType = debuffType;
            debuffData.isActive = false;
            debuffData.timestamp = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

            Net_SendDebuffUpdate(debuffData);

            LogChannel(n"BuffSync", s"[BuffSync] Debuff removed: " + EnumValueToString("EDebuffType", Cast<Int64>(EnumInt(debuffType))));
        }
    }

    // Network event handlers
    public func OnRemoteBuffUpdate(buffData: BuffData) -> Void {
        LogChannel(n"BuffSync", s"[BuffSync] Received buff update for player: " + buffData.playerId);

        let playerState = this.FindOrCreatePlayerEffectState(buffData.playerId);
        playerState.UpdateBuff(buffData);
    }

    public func OnRemoteDebuffUpdate(debuffData: DebuffData) -> Void {
        LogChannel(n"BuffSync", s"[BuffSync] Received debuff update for player: " + debuffData.playerId);

        let playerState = this.FindOrCreatePlayerEffectState(debuffData.playerId);
        playerState.UpdateDebuff(debuffData);
    }

    private func FindOrCreatePlayerEffectState(playerId: Uint32) -> ref<PlayerEffectState> {
        // Find existing state
        for effectState in this.m_playerEffectStates {
            if effectState.GetPlayerId() == playerId {
                return effectState;
            }
        }

        // Create new state
        let newState = new PlayerEffectState();
        newState.Initialize(playerId);
        ArrayPush(this.m_playerEffectStates, newState);
        return newState;
    }

    // Public API
    public func ForceEffectSync() -> Void {
        LogChannel(n"BuffSync", s"[BuffSync] Forcing effect synchronization");
        this.SynchronizeLocalEffects();
    }

    public func GetActiveBuffs() -> array<ref<ActiveBuff>> {
        return this.m_activeBuffs;
    }

    public func GetActiveDebuffs() -> array<ref<ActiveDebuff>> {
        return this.m_activeDebuffs;
    }
}

// Active Buff representation
public class ActiveBuff extends ScriptableComponent {
    private let m_buffType: EBuffType;
    private let m_startTime: Float;
    private let m_duration: Float;
    private let m_intensity: Float;

    public func Initialize(buffType: EBuffType, duration: Float, intensity: Float) -> Void {
        this.m_buffType = buffType;
        this.m_startTime = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));
        this.m_duration = duration;
        this.m_intensity = intensity;
    }

    public func GetBuffType() -> EBuffType { return this.m_buffType; }
    public func GetStartTime() -> Float { return this.m_startTime; }
    public func GetDuration() -> Float { return this.m_duration; }
    public func GetIntensity() -> Float { return this.m_intensity; }

    public func GetRemainingTime() -> Float {
        let currentTime = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));
        let elapsedTime = currentTime - this.m_startTime;
        return MaxF(0.0, this.m_duration - elapsedTime);
    }

    public func IsExpired(currentTime: Float) -> Bool {
        return (currentTime - this.m_startTime) >= this.m_duration;
    }
}

// Active Debuff representation
public class ActiveDebuff extends ScriptableComponent {
    private let m_debuffType: EDebuffType;
    private let m_startTime: Float;
    private let m_duration: Float;
    private let m_intensity: Float;

    public func Initialize(debuffType: EDebuffType, duration: Float, intensity: Float) -> Void {
        this.m_debuffType = debuffType;
        this.m_startTime = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));
        this.m_duration = duration;
        this.m_intensity = intensity;
    }

    public func GetDebuffType() -> EDebuffType { return this.m_debuffType; }
    public func GetStartTime() -> Float { return this.m_startTime; }
    public func GetDuration() -> Float { return this.m_duration; }
    public func GetIntensity() -> Float { return this.m_intensity; }

    public func GetRemainingTime() -> Float {
        let currentTime = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));
        let elapsedTime = currentTime - this.m_startTime;
        return MaxF(0.0, this.m_duration - elapsedTime);
    }

    public func IsExpired(currentTime: Float) -> Bool {
        return (currentTime - this.m_startTime) >= this.m_duration;
    }
}

// Player Effect State - tracks effects for remote players
public class PlayerEffectState extends ScriptableComponent {
    private let m_playerId: Uint32;
    private let m_remoteBuffs: array<ref<ActiveBuff>>;
    private let m_remoteDebuffs: array<ref<ActiveDebuff>>;
    private let m_lastUpdateTime: Float;

    public func Initialize(playerId: Uint32) -> Void {
        this.m_playerId = playerId;
        this.m_lastUpdateTime = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));
    }

    public func GetPlayerId() -> Uint32 {
        return this.m_playerId;
    }

    public func UpdateBuff(buffData: BuffData) -> Void {
        if buffData.isActive {
            // Add or update buff
            this.AddOrUpdateBuff(buffData);
        } else {
            // Remove buff
            this.RemoveBuff(buffData.buffType);
        }

        this.m_lastUpdateTime = buffData.timestamp;
    }

    public func UpdateDebuff(debuffData: DebuffData) -> Void {
        if debuffData.isActive {
            // Add or update debuff
            this.AddOrUpdateDebuff(debuffData);
        } else {
            // Remove debuff
            this.RemoveDebuff(debuffData.debuffType);
        }

        this.m_lastUpdateTime = debuffData.timestamp;
    }

    private func AddOrUpdateBuff(buffData: BuffData) -> Void {
        // Find existing buff
        let existingBuff: ref<ActiveBuff>;
        for buff in this.m_remoteBuffs {
            if Equals(buff.GetBuffType(), buffData.buffType) {
                existingBuff = buff;
                break;
            }
        }

        if !IsDefined(existingBuff) {
            // Create new buff
            let newBuff = new ActiveBuff();
            newBuff.Initialize(buffData.buffType, buffData.duration, buffData.intensity);
            ArrayPush(this.m_remoteBuffs, newBuff);

            LogChannel(n"BuffSync", s"[BuffSync] Remote buff added for player " + this.m_playerId + ": " + EnumValueToString("EBuffType", Cast<Int64>(EnumInt(buffData.buffType))));
        }
    }

    private func AddOrUpdateDebuff(debuffData: DebuffData) -> Void {
        // Find existing debuff
        let existingDebuff: ref<ActiveDebuff>;
        for debuff in this.m_remoteDebuffs {
            if Equals(debuff.GetDebuffType(), debuffData.debuffType) {
                existingDebuff = debuff;
                break;
            }
        }

        if !IsDefined(existingDebuff) {
            // Create new debuff
            let newDebuff = new ActiveDebuff();
            newDebuff.Initialize(debuffData.debuffType, debuffData.duration, debuffData.intensity);
            ArrayPush(this.m_remoteDebuffs, newDebuff);

            LogChannel(n"BuffSync", s"[BuffSync] Remote debuff added for player " + this.m_playerId + ": " + EnumValueToString("EDebuffType", Cast<Int64>(EnumInt(debuffData.debuffType))));
        }
    }

    private func RemoveBuff(buffType: EBuffType) -> Void {
        let index = -1;
        for i in Range(ArraySize(this.m_remoteBuffs)) {
            if Equals(this.m_remoteBuffs[i].GetBuffType(), buffType) {
                index = i;
                break;
            }
        }

        if index >= 0 {
            ArrayRemove(this.m_remoteBuffs, this.m_remoteBuffs[index]);
            LogChannel(n"BuffSync", s"[BuffSync] Remote buff removed for player " + this.m_playerId + ": " + EnumValueToString("EBuffType", Cast<Int64>(EnumInt(buffType))));
        }
    }

    private func RemoveDebuff(debuffType: EDebuffType) -> Void {
        let index = -1;
        for i in Range(ArraySize(this.m_remoteDebuffs)) {
            if Equals(this.m_remoteDebuffs[i].GetDebuffType(), debuffType) {
                index = i;
                break;
            }
        }

        if index >= 0 {
            ArrayRemove(this.m_remoteDebuffs, this.m_remoteDebuffs[index]);
            LogChannel(n"BuffSync", s"[BuffSync] Remote debuff removed for player " + this.m_playerId + ": " + EnumValueToString("EDebuffType", Cast<Int64>(EnumInt(debuffType))));
        }
    }

    public func Update() -> Void {
        let currentTime = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

        // Clean up expired effects
        this.CleanupExpiredBuffs(currentTime);
        this.CleanupExpiredDebuffs(currentTime);
    }

    private func CleanupExpiredBuffs(currentTime: Float) -> Void {
        let expiredBuffs: array<ref<ActiveBuff>>;

        for buff in this.m_remoteBuffs {
            if buff.IsExpired(currentTime) {
                ArrayPush(expiredBuffs, buff);
            }
        }

        for expiredBuff in expiredBuffs {
            ArrayRemove(this.m_remoteBuffs, expiredBuff);
        }
    }

    private func CleanupExpiredDebuffs(currentTime: Float) -> Void {
        let expiredDebuffs: array<ref<ActiveDebuff>>;

        for debuff in this.m_remoteDebuffs {
            if debuff.IsExpired(currentTime) {
                ArrayPush(expiredDebuffs, debuff);
            }
        }

        for expiredDebuff in expiredDebuffs {
            ArrayRemove(this.m_remoteDebuffs, expiredDebuff);
        }
    }
}

// Effect Tracker - monitors effect changes
public class EffectTracker extends ScriptableComponent {
    private let m_localPlayer: wref<PlayerPuppet>;
    private let m_lastEffectCheck: Float = 0.0;
    private let m_effectCheckInterval: Float = 0.5; // Check every 0.5 seconds

    public func Initialize(player: wref<PlayerPuppet>) -> Void {
        this.m_localPlayer = player;
        LogChannel(n"BuffSync", s"[BuffSync] Effect Tracker initialized");
    }

    public func Update(deltaTime: Float) -> Void {
        let currentTime = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

        if (currentTime - this.m_lastEffectCheck) >= this.m_effectCheckInterval {
            this.TrackEffectChanges();
            this.m_lastEffectCheck = currentTime;
        }
    }

    private func TrackEffectChanges() -> Void {
        // Monitor for effect application/removal events
        // This integrates with the game's status effect system
    }
}

// Data Structures
public struct BuffData {
    public let playerId: Uint32;
    public let buffType: EBuffType;
    public let isActive: Bool;
    public let duration: Float;
    public let intensity: Float;
    public let timestamp: Float;
}

public struct DebuffData {
    public let playerId: Uint32;
    public let debuffType: EDebuffType;
    public let isActive: Bool;
    public let duration: Float;
    public let intensity: Float;
    public let timestamp: Float;
}

// Enumerations
public enum EBuffType : Uint16 {
    // Attribute Buffs
    StrengthBoost = 0,
    ReflexesBoost = 1,
    TechnicalAbilityBoost = 2,
    IntelligenceBoost = 3,
    CoolBoost = 4,

    // Combat Buffs
    DamageBoost = 10,
    ArmorBoost = 11,
    CriticalChanceBoost = 12,
    CriticalDamageBoost = 13,
    AccuracyBoost = 14,
    ReloadSpeedBoost = 15,

    // Movement Buffs
    SpeedBoost = 20,
    JumpBoost = 21,
    StaminaBoost = 22,
    ClimbingBoost = 23,

    // Stealth Buffs
    StealthBoost = 30,
    HackingBoost = 31,
    DetectionReduction = 32,

    // Consumable Buffs
    FoodBuff = 40,
    AlcoholBuff = 41,
    StimulantBuff = 42,
    MedicationBuff = 43,

    // Cyberware Buffs
    CyberwareBoost = 50,
    OpticsEnhancement = 51,
    ProcessingBoost = 52,
    MemoryBoost = 53,

    // Environmental Buffs
    EnvironmentalAdaptation = 60,
    TemperatureResistance = 61,
    RadiationResistance = 62
}

public enum EDebuffType : Uint16 {
    // Status Effect Debuffs
    Bleeding = 0,
    Poisoned = 1,
    Burning = 2,
    Electrified = 3,
    Stunned = 4,
    Blinded = 5,
    Slowed = 6,
    Weakened = 7,

    // Environmental Debuffs
    Radiation = 10,
    ToxicAir = 11,
    ExtremeHeat = 12,
    ExtremeCold = 13,
    LowOxygen = 14,

    // Combat Debuffs
    Suppressed = 20,
    Disoriented = 21,
    Overheated = 22,
    WeaponJammed = 23,
    ArmorDamaged = 24,

    // Substance Debuffs
    AlcoholPenalty = 30,
    DrugCrash = 31,
    Withdrawal = 32,
    Overdose = 33,

    // Cyberware Debuffs
    CyberwareMalfunction = 40,
    SystemError = 41,
    MemoryLeak = 42,
    ProcessingLag = 43,

    // Psychological Debuffs
    Fear = 50,
    Confusion = 51,
    Hallucination = 52,
    Panic = 53
}

// Native function declarations
native func Net_SendBuffUpdate(buffData: BuffData) -> Void;
native func Net_SendDebuffUpdate(debuffData: DebuffData) -> Void;

// Integration with game systems - buff/debuff hooks
@wrapMethod(StatusEffectSystem)
public func ApplyStatusEffect(target: ref<GameObject>, statusEffect: ref<StatusEffect>) -> Void {
    wrappedMethod(target, statusEffect);

    // Check if this is the local player
    let player = target as PlayerPuppet;
    if IsDefined(player) {
        let buffDebuffManager = BuffDebuffManager.GetInstance();
        if IsDefined(buffDebuffManager) {
            buffDebuffManager.ForceEffectSync();
        }
    }
}

@wrapMethod(StatusEffectSystem)
public func RemoveStatusEffect(target: ref<GameObject>, statusEffectID: TweakDBID) -> Void {
    wrappedMethod(target, statusEffectID);

    // Check if this is the local player
    let player = target as PlayerPuppet;
    if IsDefined(player) {
        let buffDebuffManager = BuffDebuffManager.GetInstance();
        if IsDefined(buffDebuffManager) {
            buffDebuffManager.ForceEffectSync();
        }
    }
}

// Callback functions for network events
@addMethod(PlayerPuppet)
public func OnNetworkBuffUpdate(buffData: BuffData) -> Void {
    BuffDebuffManager.GetInstance().OnRemoteBuffUpdate(buffData);
}

@addMethod(PlayerPuppet)
public func OnNetworkDebuffUpdate(debuffData: DebuffData) -> Void {
    BuffDebuffManager.GetInstance().OnRemoteDebuffUpdate(debuffData);
}