// Quest scaling system for cooperative multiplayer gameplay

public enum ScalingType {
    None = 0,
    Linear = 1,      // Direct multiplier based on player count  
    Logarithmic = 2, // Diminishing returns for large groups
    Custom = 3       // Quest-specific scaling rules
}

public enum ScalingCategory {
    Combat = 0,
    Rewards = 1,
    Objectives = 2,
    Enemies = 3,
    Difficulty = 4
}

public struct ScalingRule {
    public var category: ScalingCategory;
    public var scalingType: ScalingType;
    public var baseMultiplier: Float;   // Base scaling factor
    public var maxMultiplier: Float;    // Cap for scaling
    public var playerThreshold: Int32;  // Minimum players before scaling kicks in
    public var diminishingFactor: Float; // For logarithmic scaling
}

public struct QuestScalingProfile {
    public var questId: String;
    public var questType: gameJournalQuestType;
    public var scalingRules: array<ScalingRule>;
    public var isScalingEnabled: Bool;
    public var currentPlayerCount: Int32;
    public var baseRewardXP: Int32;
    public var baseRewardMoney: Int32;
    public var scaledValues: QuestScaledValues;
}

public struct QuestScaledValues {
    public var enemyHealthMultiplier: Float;
    public var enemyDamageMultiplier: Float;
    public var enemyCountMultiplier: Float;
    public var rewardXPMultiplier: Float;
    public var rewardMoneyMultiplier: Float;
    public var rewardItemQuality: Float;
    public var objectiveComplexity: Float;
    public var timeMultiplier: Float; // For timed objectives
}

public struct EnemyScalingData {
    public var entityId: EntityID;
    public var originalHealth: Float;
    public var originalDamage: Float;
    public var scaledHealth: Float;
    public var scaledDamage: Float;
    public var isScaled: Bool;
}

public class QuestScaling {
    private static var activeProfiles: array<QuestScalingProfile>;
    private static var scaledEnemies: array<EnemyScalingData>;
    private static var isInitialized: Bool = false;
    private static var defaultRules: array<ScalingRule>;
    
    public static func Initialize() -> Void {
        if isInitialized {
            return;
        }
        
        LogChannel(n"COOP_QUEST", "Initializing quest scaling system...");
        
        // Set up default scaling rules
        QuestScaling.CreateDefaultScalingRules();
        
        // Register for quest events
        let questSystem = GameInstance.GetQuestsSystem(GetGame());
        if IsDefined(questSystem) {
            // Hook into quest start/update events
            questSystem.RegisterListener(QuestScaling.OnQuestStarted, n"QuestStarted");
            questSystem.RegisterListener(QuestScaling.OnQuestUpdated, n"QuestUpdated");
            questSystem.RegisterListener(QuestScaling.OnQuestCompleted, n"QuestCompleted");
        }
        
        // Register for player count changes
        NetworkingSystem.RegisterCallback("player_joined", QuestScaling.OnPlayerJoined);
        NetworkingSystem.RegisterCallback("player_left", QuestScaling.OnPlayerLeft);
        
        isInitialized = true;
        LogChannel(n"COOP_QUEST", "Quest scaling system initialized");
    }
    
    private static func CreateDefaultScalingRules() -> Void {
        ArrayClear(defaultRules);
        
        // Combat scaling - enemy health
        let combatHealthRule: ScalingRule;
        combatHealthRule.category = ScalingCategory.Combat;
        combatHealthRule.scalingType = ScalingType.Logarithmic;
        combatHealthRule.baseMultiplier = 1.5; // 50% more health per player
        combatHealthRule.maxMultiplier = 4.0; // Cap at 400% health
        combatHealthRule.playerThreshold = 2; // Start scaling with 2+ players
        combatHealthRule.diminishingFactor = 0.7; // Diminishing returns
        ArrayPush(defaultRules, combatHealthRule);
        
        // Combat scaling - enemy damage  
        let combatDamageRule: ScalingRule;
        combatDamageRule.category = ScalingCategory.Combat;
        combatDamageRule.scalingType = ScalingType.Linear;
        combatDamageRule.baseMultiplier = 1.2; // 20% more damage per player
        combatDamageRule.maxMultiplier = 2.5; // Cap at 250% damage
        combatDamageRule.playerThreshold = 2;
        ArrayPush(defaultRules, combatDamageRule);
        
        // Enemy count scaling
        let enemyCountRule: ScalingRule;
        enemyCountRule.category = ScalingCategory.Enemies;
        enemyCountRule.scalingType = ScalingType.Linear;
        enemyCountRule.baseMultiplier = 1.3; // 30% more enemies per player
        enemyCountRule.maxMultiplier = 3.0; // Cap at 300% enemies
        enemyCountRule.playerThreshold = 3; // Only scale with 3+ players
        ArrayPush(defaultRules, enemyCountRule);
        
        // Reward scaling - XP
        let rewardXPRule: ScalingRule;
        rewardXPRule.category = ScalingCategory.Rewards;
        rewardXPRule.scalingType = ScalingType.Logarithmic;
        rewardXPRule.baseMultiplier = 1.1; // 10% more XP per player
        rewardXPRule.maxMultiplier = 2.0; // Cap at 200% XP
        rewardXPRule.playerThreshold = 2;
        rewardXPRule.diminishingFactor = 0.8;
        ArrayPush(defaultRules, rewardXPRule);
        
        // Reward scaling - Money
        let rewardMoneyRule: ScalingRule;
        rewardMoneyRule.category = ScalingCategory.Rewards;
        rewardMoneyRule.scalingType = ScalingType.Linear;
        rewardMoneyRule.baseMultiplier = 1.15; // 15% more money per player
        rewardMoneyRule.maxMultiplier = 2.5; // Cap at 250% money
        rewardMoneyRule.playerThreshold = 2;
        ArrayPush(defaultRules, rewardMoneyRule);
        
        LogChannel(n"COOP_QUEST", "Created " + ToString(ArraySize(defaultRules)) + " default scaling rules");
    }
    
    public static func StartQuestScaling(questId: String, questType: gameJournalQuestType) -> Void {
        if !NetworkingSystem.IsConnected() {
            return; // No scaling in single player
        }
        
        let playerCount = NetworkingSystem.GetConnectedPlayerCount();
        if playerCount < 2 {
            return; // No scaling needed for single player
        }
        
        LogChannel(n"COOP_QUEST", "Starting quest scaling for: " + questId + " with " + ToString(playerCount) + " players");
        
        // Check if already scaling this quest
        for profile in activeProfiles {
            if Equals(profile.questId, questId) {
                QuestScaling.UpdateQuestScaling(questId, playerCount);
                return;
            }
        }
        
        // Create new scaling profile
        let profile: QuestScalingProfile;
        profile.questId = questId;
        profile.questType = questType;
        profile.scalingRules = QuestScaling.GetScalingRulesForQuest(questId, questType);
        profile.isScalingEnabled = true;
        profile.currentPlayerCount = playerCount;
        
        // Get base quest rewards
        QuestScaling.GetBaseQuestRewards(questId, profile);
        
        // Calculate scaled values
        profile.scaledValues = QuestScaling.CalculateScaledValues(profile.scalingRules, playerCount);
        
        ArrayPush(activeProfiles, profile);
        
        // Apply scaling to quest
        QuestScaling.ApplyQuestScaling(profile);
        
        LogChannel(n"COOP_QUEST", "Quest scaling applied: " + questId);
    }
    
    public static func UpdateQuestScaling(questId: String, newPlayerCount: Int32) -> Void {
        let profileIndex = QuestScaling.FindActiveProfileIndex(questId);
        if profileIndex == -1 {
            return;
        }
        
        let profile = activeProfiles[profileIndex];
        let oldPlayerCount = profile.currentPlayerCount;
        
        if oldPlayerCount == newPlayerCount {
            return; // No change needed
        }
        
        LogChannel(n"COOP_QUEST", "Updating quest scaling: " + questId + " from " + ToString(oldPlayerCount) + " to " + ToString(newPlayerCount) + " players");
        
        // Recalculate scaled values
        profile.currentPlayerCount = newPlayerCount;
        profile.scaledValues = QuestScaling.CalculateScaledValues(profile.scalingRules, newPlayerCount);
        activeProfiles[profileIndex] = profile;
        
        // Reapply scaling
        QuestScaling.ApplyQuestScaling(profile);
        
        // Update existing scaled enemies
        QuestScaling.UpdateScaledEnemies(profile);
    }
    
    public static func StopQuestScaling(questId: String) -> Void {
        let profileIndex = QuestScaling.FindActiveProfileIndex(questId);
        if profileIndex == -1 {
            return;
        }
        
        let profile = activeProfiles[profileIndex];
        LogChannel(n"COOP_QUEST", "Stopping quest scaling for: " + questId);
        
        // Remove scaling from enemies
        QuestScaling.RemoveEnemyScaling(questId);
        
        // Remove profile
        ArrayErase(activeProfiles, profileIndex);
    }
    
    private static func CalculateScaledValues(rules: array<ScalingRule>, playerCount: Int32) -> QuestScaledValues {
        let values: QuestScaledValues;
        
        // Initialize to default values
        values.enemyHealthMultiplier = 1.0;
        values.enemyDamageMultiplier = 1.0;
        values.enemyCountMultiplier = 1.0;
        values.rewardXPMultiplier = 1.0;
        values.rewardMoneyMultiplier = 1.0;
        values.rewardItemQuality = 1.0;
        values.objectiveComplexity = 1.0;
        values.timeMultiplier = 1.0;
        
        for rule in rules {
            if playerCount < rule.playerThreshold {
                continue; // Skip rule if not enough players
            }
            
            let multiplier = QuestScaling.CalculateMultiplier(rule, playerCount);
            
            switch rule.category {
                case ScalingCategory.Combat:
                    if rule.scalingType == ScalingType.Linear && rule.baseMultiplier > 1.4 {
                        values.enemyHealthMultiplier *= multiplier; // Health rule
                    } else {
                        values.enemyDamageMultiplier *= multiplier; // Damage rule
                    }
                    break;
                    
                case ScalingCategory.Enemies:
                    values.enemyCountMultiplier *= multiplier;
                    break;
                    
                case ScalingCategory.Rewards:
                    if rule.baseMultiplier > 1.12 {
                        values.rewardMoneyMultiplier *= multiplier; // Money rule
                    } else {
                        values.rewardXPMultiplier *= multiplier; // XP rule
                    }
                    break;
                    
                case ScalingCategory.Objectives:
                    values.objectiveComplexity *= multiplier;
                    values.timeMultiplier *= (2.0 - multiplier); // More time for complex objectives
                    break;
                    
                case ScalingCategory.Difficulty:
                    values.rewardItemQuality *= multiplier;
                    break;
            }
        }
        
        return values;
    }
    
    private static func CalculateMultiplier(rule: ScalingRule, playerCount: Int32) -> Float {
        let effectivePlayers = Cast<Float>(playerCount - rule.playerThreshold + 1);
        let multiplier: Float;
        
        switch rule.scalingType {
            case ScalingType.Linear:
                multiplier = 1.0 + (rule.baseMultiplier - 1.0) * effectivePlayers;
                break;
                
            case ScalingType.Logarithmic:
                let logValue = LogF(1.0 + effectivePlayers * rule.diminishingFactor);
                multiplier = 1.0 + (rule.baseMultiplier - 1.0) * logValue;
                break;
                
            case ScalingType.Custom:
                multiplier = QuestScaling.CalculateCustomMultiplier(rule, playerCount);
                break;
                
            default:
                multiplier = 1.0;
                break;
        }
        
        // Apply maximum cap
        return MinF(multiplier, rule.maxMultiplier);
    }
    
    private static func CalculateCustomMultiplier(rule: ScalingRule, playerCount: Int32) -> Float {
        // Custom scaling logic for special cases
        // This could be expanded for quest-specific scaling needs
        return ClampF(1.0 + (Cast<Float>(playerCount) - 1.0) * 0.25, 1.0, rule.maxMultiplier);
    }
    
    private static func ApplyQuestScaling(profile: QuestScalingProfile) -> Void {
        LogChannel(n"COOP_QUEST", "Applying scaling to quest: " + profile.questId);
        
        // Scale existing enemies in quest area
        QuestScaling.ScaleQuestEnemies(profile);
        
        // Modify quest objectives if needed
        QuestScaling.ScaleQuestObjectives(profile);
        
        // Update reward pools
        QuestScaling.ScaleQuestRewards(profile);
        
        // Notify other players of scaling changes
        let scalingData = QuestScaling.SerializeScalingProfile(profile);
        NetworkingSystem.BroadcastMessage("quest_scaling_applied", scalingData);
    }
    
    private static func ScaleQuestEnemies(profile: QuestScalingProfile) -> Void {
        // Find enemies in quest area
        let questEnemies = QuestScaling.GetQuestEnemies(profile.questId);
        
        for enemy in questEnemies {
            let enemyData: EnemyScalingData;
            enemyData.entityId = enemy.GetEntityID();
            enemyData.originalHealth = enemy.GetMaxHealth();
            enemyData.originalDamage = enemy.GetBaseDamage();
            
            // Calculate scaled values
            enemyData.scaledHealth = enemyData.originalHealth * profile.scaledValues.enemyHealthMultiplier;
            enemyData.scaledDamage = enemyData.originalDamage * profile.scaledValues.enemyDamageMultiplier;
            enemyData.isScaled = true;
            
            // Apply scaling to enemy
            enemy.SetMaxHealth(enemyData.scaledHealth);
            enemy.SetCurrentHealth(enemyData.scaledHealth);
            enemy.SetBaseDamage(enemyData.scaledDamage);
            
            ArrayPush(scaledEnemies, enemyData);
            
            LogChannel(n"COOP_QUEST", "Scaled enemy: Health " + ToString(enemyData.originalHealth) + " -> " + ToString(enemyData.scaledHealth) + 
                      ", Damage " + ToString(enemyData.originalDamage) + " -> " + ToString(enemyData.scaledDamage));
        }
    }
    
    private static func ScaleQuestObjectives(profile: QuestScalingProfile) -> Void {
        if profile.scaledValues.objectiveComplexity <= 1.0 {
            return; // No objective scaling needed
        }
        
        let questSystem = GameInstance.GetQuestsSystem(GetGame());
        if !IsDefined(questSystem) {
            return;
        }
        
        // Get quest objectives
        let objectives = questSystem.GetQuestObjectives(profile.questId);
        
        for objective in objectives {
            // Scale collection objectives
            if objective.type == QuestObjectiveType.Collection {
                let newCount = Cast<Int32>(Cast<Float>(objective.targetCount) * profile.scaledValues.objectiveComplexity);
                objective.targetCount = newCount;
                LogChannel(n"COOP_QUEST", "Scaled collection objective to: " + ToString(newCount));
            }
            
            // Scale elimination objectives
            if objective.type == QuestObjectiveType.Elimination {
                let newCount = Cast<Int32>(Cast<Float>(objective.targetCount) * profile.scaledValues.enemyCountMultiplier);
                objective.targetCount = newCount;
                LogChannel(n"COOP_QUEST", "Scaled elimination objective to: " + ToString(newCount));
            }
            
            // Scale timed objectives
            if objective.hasTimer {
                let newTime = objective.timeLimit * profile.scaledValues.timeMultiplier;
                objective.timeLimit = newTime;
                LogChannel(n"COOP_QUEST", "Scaled time limit to: " + ToString(newTime) + " seconds");
            }
        }
        
        // Update quest system with modified objectives
        questSystem.UpdateQuestObjectives(profile.questId, objectives);
    }
    
    private static func ScaleQuestRewards(profile: QuestScalingProfile) -> Void {
        let questSystem = GameInstance.GetQuestsSystem(GetGame());
        if !IsDefined(questSystem) {
            return;
        }
        
        // Scale XP reward
        let newXP = Cast<Int32>(Cast<Float>(profile.baseRewardXP) * profile.scaledValues.rewardXPMultiplier);
        questSystem.SetQuestRewardXP(profile.questId, newXP);
        
        // Scale money reward
        let newMoney = Cast<Int32>(Cast<Float>(profile.baseRewardMoney) * profile.scaledValues.rewardMoneyMultiplier);
        questSystem.SetQuestRewardMoney(profile.questId, newMoney);
        
        // Improve item quality
        if profile.scaledValues.rewardItemQuality > 1.0 {
            questSystem.ScaleQuestRewardItems(profile.questId, profile.scaledValues.rewardItemQuality);
        }
        
        LogChannel(n"COOP_QUEST", "Scaled rewards: XP " + ToString(profile.baseRewardXP) + " -> " + ToString(newXP) + 
                  ", Money " + ToString(profile.baseRewardMoney) + " -> " + ToString(newMoney));
    }
    
    private static func UpdateScaledEnemies(profile: QuestScalingProfile) -> Void {
        // Update existing scaled enemies with new multipliers
        for i in Range(ArraySize(scaledEnemies)) {
            let enemyData = scaledEnemies[i];
            if !enemyData.isScaled {
                continue;
            }
            
            let enemy = GameInstance.FindEntityByID(GetGame(), enemyData.entityId) as NPCPuppet;
            if !IsDefined(enemy) {
                continue; // Enemy no longer exists
            }
            
            // Recalculate scaled values
            enemyData.scaledHealth = enemyData.originalHealth * profile.scaledValues.enemyHealthMultiplier;
            enemyData.scaledDamage = enemyData.originalDamage * profile.scaledValues.enemyDamageMultiplier;
            
            // Apply new scaling
            enemy.SetMaxHealth(enemyData.scaledHealth);
            enemy.SetCurrentHealth(enemyData.scaledHealth);
            enemy.SetBaseDamage(enemyData.scaledDamage);
            
            scaledEnemies[i] = enemyData;
        }
    }
    
    private static func RemoveEnemyScaling(questId: String) -> Void {
        // Reset enemies to original values
        for i in Range(ArraySize(scaledEnemies)) {
            let enemyData = scaledEnemies[i];
            if !enemyData.isScaled {
                continue;
            }
            
            let enemy = GameInstance.FindEntityByID(GetGame(), enemyData.entityId) as NPCPuppet;
            if IsDefined(enemy) {
                // Restore original values
                enemy.SetMaxHealth(enemyData.originalHealth);
                enemy.SetBaseDamage(enemyData.originalDamage);
            }
        }
        
        // Clear scaled enemy list
        ArrayClear(scaledEnemies);
    }
    
    // Event handlers
    private static cb func OnQuestStarted(questId: String) -> Void {
        if !NetworkingSystem.IsConnected() {
            return;
        }
        
        let questSystem = GameInstance.GetQuestsSystem(GetGame());
        if !IsDefined(questSystem) {
            return;
        }
        
        let questData = questSystem.GetQuestData(questId);
        QuestScaling.StartQuestScaling(questId, questData.type);
    }
    
    private static cb func OnQuestCompleted(questId: String) -> Void {
        QuestScaling.StopQuestScaling(questId);
    }
    
    private static cb func OnPlayerJoined(playerData: String) -> Void {
        let newPlayerCount = NetworkingSystem.GetConnectedPlayerCount();
        
        // Update scaling for all active quests
        for profile in activeProfiles {
            QuestScaling.UpdateQuestScaling(profile.questId, newPlayerCount);
        }
    }
    
    private static cb func OnPlayerLeft(playerData: String) -> Void {
        let newPlayerCount = NetworkingSystem.GetConnectedPlayerCount();
        
        // Update scaling for all active quests
        for profile in activeProfiles {
            QuestScaling.UpdateQuestScaling(profile.questId, newPlayerCount);
        }
    }
    
    // Utility functions
    private static func GetScalingRulesForQuest(questId: String, questType: gameJournalQuestType) -> array<ScalingRule> {
        // For now, use default rules for all quests
        // This could be expanded to have quest-specific rules
        return defaultRules;
    }
    
    private static func GetBaseQuestRewards(questId: String, profile: ref<QuestScalingProfile>) -> Void {
        let questSystem = GameInstance.GetQuestsSystem(GetGame());
        if !IsDefined(questSystem) {
            profile.baseRewardXP = 1000; // Default values
            profile.baseRewardMoney = 500;
            return;
        }
        
        let questData = questSystem.GetQuestData(questId);
        profile.baseRewardXP = questData.rewardXP;
        profile.baseRewardMoney = questData.rewardMoney;
    }
    
    private static func GetQuestEnemies(questId: String) -> array<ref<NPCPuppet>> {
        let enemies: array<ref<NPCPuppet>>;
        
        // Get all enemies in the game world
        let targetingSystem = GameInstance.GetTargetingSystem(GetGame());
        if !IsDefined(targetingSystem) {
            return enemies;
        }
        
        // Find enemies associated with this quest
        let allEnemies = targetingSystem.GetHostileEnemies();
        for enemy in allEnemies {
            if QuestScaling.IsEnemyInQuest(enemy, questId) {
                ArrayPush(enemies, enemy);
            }
        }
        
        return enemies;
    }
    
    private static func IsEnemyInQuest(enemy: ref<NPCPuppet>, questId: String) -> Bool {
        // Check if enemy is part of this quest
        // This would need integration with quest system to determine enemy associations
        return true; // Simplified for now
    }
    
    private static func FindActiveProfileIndex(questId: String) -> Int32 {
        for i in Range(ArraySize(activeProfiles)) {
            if Equals(activeProfiles[i].questId, questId) {
                return i;
            }
        }
        return -1;
    }
    
    private static func SerializeScalingProfile(profile: QuestScalingProfile) -> String {
        // Simple serialization for network transmission
        let data = profile.questId + "|" + ToString(profile.currentPlayerCount) + "|";
        data += ToString(profile.scaledValues.enemyHealthMultiplier) + "|";
        data += ToString(profile.scaledValues.enemyDamageMultiplier) + "|";
        data += ToString(profile.scaledValues.rewardXPMultiplier) + "|";
        data += ToString(profile.scaledValues.rewardMoneyMultiplier);
        return data;
    }
    
    // Public API
    public static func GetActiveQuestScaling(questId: String) -> QuestScalingProfile {
        let profileIndex = QuestScaling.FindActiveProfileIndex(questId);
        if profileIndex != -1 {
            return activeProfiles[profileIndex];
        }
        
        let emptyProfile: QuestScalingProfile;
        return emptyProfile;
    }
    
    public static func IsQuestScaled(questId: String) -> Bool {
        return QuestScaling.FindActiveProfileIndex(questId) != -1;
    }
    
    public static func GetScalingMultiplier(questId: String, category: ScalingCategory) -> Float {
        let profile = QuestScaling.GetActiveQuestScaling(questId);
        if Equals(profile.questId, "") {
            return 1.0;
        }
        
        switch category {
            case ScalingCategory.Combat:
                return profile.scaledValues.enemyHealthMultiplier;
            case ScalingCategory.Rewards:
                return profile.scaledValues.rewardXPMultiplier;
            case ScalingCategory.Enemies:
                return profile.scaledValues.enemyCountMultiplier;
            default:
                return 1.0;
        }
    }
}

// Placeholder structures for quest system integration
public enum QuestObjectiveType {
    Collection = 0,
    Elimination = 1,
    Escort = 2,
    Survival = 3,
    Interaction = 4
}

public struct QuestObjective {
    public var id: String;
    public var type: QuestObjectiveType;
    public var targetCount: Int32;
    public var currentCount: Int32;
    public var hasTimer: Bool;
    public var timeLimit: Float;
}

// Extended QuestData structure
public struct QuestDataExtended {
    public var id: String;
    public var type: gameJournalQuestType;
    public var rewardXP: Int32;
    public var rewardMoney: Int32;
    public var objectives: array<QuestObjective>;
}

// Integration hooks
@wrapMethod(NPCPuppet)
public func OnDeath() -> Void {
    // Check if this enemy was scaled and log the event
    let entityId = this.GetEntityID();
    for enemyData in QuestScaling.scaledEnemies {
        if Equals(enemyData.entityId, entityId) && enemyData.isScaled {
            LogChannel(n"COOP_QUEST", "Scaled enemy defeated with multiplied stats");
            break;
        }
    }
    
    wrappedMethod();
}

// Initialize quest scaling when game starts
@wrapMethod(QuestsSystem)
public func OnAttach() -> Void {
    wrappedMethod();
    
    // Initialize quest scaling system
    QuestScaling.Initialize();
}