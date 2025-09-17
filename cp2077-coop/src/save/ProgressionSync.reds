// Shared progression and save compatibility system for cooperative gameplay

public enum ProgressionSyncMode {
    HostProgression = 0,    // Use host's progression as authority
    ClientProgression = 1,  // Use joining client's progression
    MergedProgression = 2,  // Merge host and client progressions intelligently
    AskPlayer = 3          // Prompt player to choose which progression to use
}

public enum ProgressionConflictType {
    NoConflict = 0,
    QuestProgress = 1,     // Different quest completion states
    CharacterLevel = 2,    // Different character levels
    SkillPoints = 3,       // Different skill allocations
    Inventory = 4,         // Different inventories
    Relationships = 5,     // Different relationship states
    LifePath = 6,         // Different life path progress
    WorldState = 7        // Different world state flags
}

public struct ProgressionSnapshot {
    public var playerId: String;
    public var characterLevel: Int32;
    public var experience: Int32;
    public var streetCred: Int32;
    public var availableSkillPoints: Int32;
    public var questProgress: array<QuestProgressData>;
    public var worldFlags: array<WorldFlag>;
    public var relationshipStates: array<RelationshipState>;
    public var inventoryHash: String; // Hash of inventory state
    public var lifePath: gamedataLifePath;
    public var playTimeSeconds: Float;
    public var lastSaveTimestamp: String;
}

public struct QuestProgressData {
    public var questId: String;
    public var phase: String;
    public var isCompleted: Bool;
    public var isFailed: Bool;
    public var objectiveStates: array<ObjectiveState>;
    public var choiceHistory: array<String>;
}

public struct ObjectiveState {
    public var objectiveId: String;
    public var isCompleted: Bool;
    public var currentCount: Int32;
    public var targetCount: Int32;
}

public struct WorldFlag {
    public var flagName: String;
    public var flagValue: Bool;
    public var setTimestamp: Float;
}

public struct RelationshipState {
    public var characterName: String;
    public var relationshipLevel: Int32;
    public var isRomanceActive: Bool;
    public var romanceLevel: Int32;
    public var lastInteractionType: String;
}

public struct ProgressionConflict {
    public var conflictType: ProgressionConflictType;
    public var description: String;
    public var hostValue: String;
    public var clientValue: String;
    public var recommendedResolution: String;
    public var severity: Int32; // 1=minor, 2=moderate, 3=major
}

public struct SaveCompatibilityInfo {
    public var isCompatible: Bool;
    public var conflicts: array<ProgressionConflict>;
    public var recommendedMode: ProgressionSyncMode;
    public var backupRequired: Bool;
    public var mergeStrategy: String;
}

public class ProgressionSync {
    private static var isInitialized: Bool = false;
    private static var currentSnapshot: ProgressionSnapshot;
    private static var hostSnapshot: ProgressionSnapshot;
    private static var clientSnapshots: array<ProgressionSnapshot>;
    private static var syncMode: ProgressionSyncMode;
    private static var pendingConflicts: array<ProgressionConflict>;
    private static var saveBackupPath: String;
    
    public static func Initialize() -> Void {
        if isInitialized {
            return;
        }
        
        LogChannel(n"COOP_SAVE", "Initializing progression synchronization system...");
        
        // Set default sync mode
        syncMode = ProgressionSyncMode.HostProgression;
        
        // Set backup path
        saveBackupPath = "userdata/saves/coop_backups/";
        ProgressionSync.EnsureBackupDirectory();
        
        // Register network callbacks
        NetworkingSystem.RegisterCallback("progression_request", ProgressionSync.OnProgressionRequested);
        NetworkingSystem.RegisterCallback("progression_data", ProgressionSync.OnProgressionDataReceived);
        NetworkingSystem.RegisterCallback("progression_sync", ProgressionSync.OnProgressionSyncReceived);
        NetworkingSystem.RegisterCallback("save_state_request", ProgressionSync.OnSaveStateRequested);
        
        isInitialized = true;
        LogChannel(n"COOP_SAVE", "Progression sync system initialized");
    }
    
    public static func SetSyncMode(mode: ProgressionSyncMode) -> Void {
        syncMode = mode;
        LogChannel(n"COOP_SAVE", "Progression sync mode set to: " + ToString(Cast<Int32>(mode)));
    }
    
    public static func CaptureCurrentProgression() -> ProgressionSnapshot {
        let snapshot: ProgressionSnapshot;
        
        // Get player data
        let playerData = GetPlayer(GetGame()).GetPlayerData();
        if IsDefined(playerData) {
            snapshot.playerId = NetworkingSystem.GetLocalPlayerId();
            snapshot.characterLevel = playerData.GetLevel();
            snapshot.experience = playerData.GetCurrentXP();
            snapshot.streetCred = playerData.GetStreetCred();
            snapshot.availableSkillPoints = playerData.GetAvailableSkillPoints();
            snapshot.lifePath = playerData.GetLifePath();
        }
        
        // Get quest progress
        snapshot.questProgress = ProgressionSync.CaptureQuestProgress();
        
        // Get world flags
        snapshot.worldFlags = ProgressionSync.CaptureWorldFlags();
        
        // Get relationship states
        snapshot.relationshipStates = ProgressionSync.CaptureRelationshipStates();
        
        // Generate inventory hash
        snapshot.inventoryHash = ProgressionSync.GenerateInventoryHash();
        
        // Get playtime
        let gameTimeSystem = GameInstance.GetTimeSystem(GetGame());
        if IsDefined(gameTimeSystem) {
            snapshot.playTimeSeconds = gameTimeSystem.GetSimTime();
        }
        
        // Set timestamp
        snapshot.lastSaveTimestamp = ProgressionSync.GetCurrentTimestamp();
        
        currentSnapshot = snapshot;
        LogChannel(n"COOP_SAVE", "Captured progression snapshot for player: " + snapshot.playerId);
        
        return snapshot;
    }
    
    public static func RequestProgressionSync() -> Void {
        if !NetworkingSystem.IsConnected() {
            return;
        }
        
        LogChannel(n"COOP_SAVE", "Requesting progression synchronization...");
        
        // Capture current progression
        let mySnapshot = ProgressionSync.CaptureCurrentProgression();
        
        if NetworkingSystem.IsHost() {
            // As host, request all clients to send their progression
            hostSnapshot = mySnapshot;
            NetworkingSystem.BroadcastMessage("progression_request", "");
        } else {
            // As client, send progression to host
            let progressionData = ProgressionSync.SerializeSnapshot(mySnapshot);
            NetworkingSystem.SendToHost("progression_data", progressionData);
        }
    }
    
    public static func AnalyzeCompatibility(hostSnapshot: ProgressionSnapshot, clientSnapshot: ProgressionSnapshot) -> SaveCompatibilityInfo {
        let compatibilityInfo: SaveCompatibilityInfo;
        compatibilityInfo.isCompatible = true;
        compatibilityInfo.recommendedMode = ProgressionSyncMode.HostProgression;
        compatibilityInfo.backupRequired = true;
        compatibilityInfo.mergeStrategy = "host_priority";
        
        // Check for level differences
        let levelDiff = AbsI(hostSnapshot.characterLevel - clientSnapshot.characterLevel);
        if levelDiff > 5 {
            let conflict: ProgressionConflict;
            conflict.conflictType = ProgressionConflictType.CharacterLevel;
            conflict.description = "Significant character level difference detected";
            conflict.hostValue = ToString(hostSnapshot.characterLevel);
            conflict.clientValue = ToString(clientSnapshot.characterLevel);
            conflict.severity = levelDiff > 15 ? 3 : 2;
            conflict.recommendedResolution = "Use higher level character as baseline";
            ArrayPush(compatibilityInfo.conflicts, conflict);
        }
        
        // Check for life path conflicts
        if hostSnapshot.lifePath != clientSnapshot.lifePath {
            let conflict: ProgressionConflict;
            conflict.conflictType = ProgressionConflictType.LifePath;
            conflict.description = "Different life paths detected";
            conflict.hostValue = EnumValueToString("gamedataLifePath", Cast<Int64>(hostSnapshot.lifePath));
            conflict.clientValue = EnumValueToString("gamedataLifePath", Cast<Int64>(clientSnapshot.lifePath));
            conflict.severity = 2;
            conflict.recommendedResolution = "Life paths will be kept separate per player";
            ArrayPush(compatibilityInfo.conflicts, conflict);
        }
        
        // Check quest progress conflicts
        let questConflicts = ProgressionSync.AnalyzeQuestConflicts(hostSnapshot.questProgress, clientSnapshot.questProgress);
        for questConflict in questConflicts {
            ArrayPush(compatibilityInfo.conflicts, questConflict);
        }
        
        // Check relationship conflicts
        let relationshipConflicts = ProgressionSync.AnalyzeRelationshipConflicts(hostSnapshot.relationshipStates, clientSnapshot.relationshipStates);
        for relConflict in relationshipConflicts {
            ArrayPush(compatibilityInfo.conflicts, relConflict);
        }
        
        // Determine overall compatibility
        compatibilityInfo.isCompatible = ArraySize(compatibilityInfo.conflicts) == 0;
        
        // Recommend sync mode based on conflicts
        if ArraySize(compatibilityInfo.conflicts) > 5 {
            compatibilityInfo.recommendedMode = ProgressionSyncMode.AskPlayer;
        } else if ArraySize(compatibilityInfo.conflicts) > 2 {
            compatibilityInfo.recommendedMode = ProgressionSyncMode.MergedProgression;
        }
        
        LogChannel(n"COOP_SAVE", "Compatibility analysis complete: " + ToString(ArraySize(compatibilityInfo.conflicts)) + " conflicts found");
        
        return compatibilityInfo;
    }
    
    public static func SynchronizeProgression(hostSnapshot: ProgressionSnapshot, clientSnapshot: ProgressionSnapshot, mode: ProgressionSyncMode) -> Bool {
        LogChannel(n"COOP_SAVE", "Synchronizing progression with mode: " + ToString(Cast<Int32>(mode)));
        
        // Create backup before synchronization
        ProgressionSync.CreateProgressionBackup(currentSnapshot);
        
        let mergedSnapshot: ProgressionSnapshot;
        
        switch mode {
            case ProgressionSyncMode.HostProgression:
                mergedSnapshot = hostSnapshot;
                break;
                
            case ProgressionSyncMode.ClientProgression:
                mergedSnapshot = clientSnapshot;
                break;
                
            case ProgressionSyncMode.MergedProgression:
                mergedSnapshot = ProgressionSync.MergeProgressions(hostSnapshot, clientSnapshot);
                break;
                
            case ProgressionSyncMode.AskPlayer:
                // This should be handled by UI before calling this function
                mergedSnapshot = hostSnapshot; // Default fallback
                break;
        }
        
        // Apply synchronized progression
        let success = ProgressionSync.ApplyProgression(mergedSnapshot);
        
        if success {
            // Broadcast synchronized state to all players
            let syncData = ProgressionSync.SerializeSnapshot(mergedSnapshot);
            NetworkingSystem.BroadcastMessage("progression_sync", syncData);
            
            LogChannel(n"COOP_SAVE", "Progression synchronization completed successfully");
        } else {
            LogChannel(n"COOP_SAVE", "Progression synchronization failed, restoring backup");
            ProgressionSync.RestoreProgressionBackup();
        }
        
        return success;
    }
    
    private static func MergeProgressions(hostSnapshot: ProgressionSnapshot, clientSnapshot: ProgressionSnapshot) -> ProgressionSnapshot {
        let mergedSnapshot: ProgressionSnapshot;
        
        // Use higher level character as base
        if hostSnapshot.characterLevel >= clientSnapshot.characterLevel {
            mergedSnapshot = hostSnapshot;
        } else {
            mergedSnapshot = clientSnapshot;
        }
        
        // Merge quest progress (take the most advanced state for each quest)
        mergedSnapshot.questProgress = ProgressionSync.MergeQuestProgress(hostSnapshot.questProgress, clientSnapshot.questProgress);
        
        // Merge world flags (combine all set flags)
        mergedSnapshot.worldFlags = ProgressionSync.MergeWorldFlags(hostSnapshot.worldFlags, clientSnapshot.worldFlags);
        
        // Merge relationships (take highest relationship levels)
        mergedSnapshot.relationshipStates = ProgressionSync.MergeRelationshipStates(hostSnapshot.relationshipStates, clientSnapshot.relationshipStates);
        
        // Use host's player ID but update timestamp
        mergedSnapshot.playerId = hostSnapshot.playerId;
        mergedSnapshot.lastSaveTimestamp = ProgressionSync.GetCurrentTimestamp();
        
        LogChannel(n"COOP_SAVE", "Progression merge completed");
        
        return mergedSnapshot;
    }
    
    private static func MergeQuestProgress(hostQuests: array<QuestProgressData>, clientQuests: array<QuestProgressData>) -> array<QuestProgressData> {
        let mergedQuests: array<QuestProgressData>;
        
        // Create map of client quests for easy lookup
        let clientQuestMap: array<QuestProgressData>;
        for clientQuest in clientQuests {
            ArrayPush(clientQuestMap, clientQuest);
        }
        
        // Process all host quests
        for hostQuest in hostQuests {
            let clientQuest = ProgressionSync.FindQuestById(clientQuestMap, hostQuest.questId);
            
            if IsDefined(clientQuest) {
                // Both players have this quest, merge states
                let mergedQuest = ProgressionSync.MergeQuestStates(hostQuest, clientQuest);
                ArrayPush(mergedQuests, mergedQuest);
            } else {
                // Only host has this quest
                ArrayPush(mergedQuests, hostQuest);
            }
        }
        
        // Add client-only quests
        for clientQuest in clientQuests {
            if !ProgressionSync.QuestExistsInArray(mergedQuests, clientQuest.questId) {
                ArrayPush(mergedQuests, clientQuest);
            }
        }
        
        return mergedQuests;
    }
    
    private static func MergeQuestStates(hostQuest: QuestProgressData, clientQuest: QuestProgressData) -> QuestProgressData {
        let mergedQuest: QuestProgressData;
        mergedQuest.questId = hostQuest.questId;
        
        // Use the most advanced completion state
        if hostQuest.isCompleted || clientQuest.isCompleted {
            mergedQuest.isCompleted = true;
            mergedQuest.phase = hostQuest.isCompleted ? hostQuest.phase : clientQuest.phase;
        } else {
            // Compare phases to determine which is more advanced
            mergedQuest.isCompleted = false;
            mergedQuest.phase = ProgressionSync.GetMostAdvancedPhase(hostQuest.phase, clientQuest.phase);
        }
        
        mergedQuest.isFailed = hostQuest.isFailed || clientQuest.isFailed;
        
        // Merge objective states
        mergedQuest.objectiveStates = ProgressionSync.MergeObjectiveStates(hostQuest.objectiveStates, clientQuest.objectiveStates);
        
        // Combine choice histories
        mergedQuest.choiceHistory = hostQuest.choiceHistory;
        for clientChoice in clientQuest.choiceHistory {
            if !ProgressionSync.StringExistsInArray(mergedQuest.choiceHistory, clientChoice) {
                ArrayPush(mergedQuest.choiceHistory, clientChoice);
            }
        }
        
        return mergedQuest;
    }
    
    private static func ApplyProgression(snapshot: ProgressionSnapshot) -> Bool {
        LogChannel(n"COOP_SAVE", "Applying progression snapshot...");
        
        try {
            // Apply character progression
            let playerData = GetPlayer(GetGame()).GetPlayerData();
            if IsDefined(playerData) {
                playerData.SetLevel(snapshot.characterLevel);
                playerData.SetCurrentXP(snapshot.experience);
                playerData.SetStreetCred(snapshot.streetCred);
                playerData.SetAvailableSkillPoints(snapshot.availableSkillPoints);
            }
            
            // Apply quest progress
            ProgressionSync.ApplyQuestProgress(snapshot.questProgress);
            
            // Apply world flags
            ProgressionSync.ApplyWorldFlags(snapshot.worldFlags);
            
            // Apply relationship states
            ProgressionSync.ApplyRelationshipStates(snapshot.relationshipStates);
            
            // Update current snapshot
            currentSnapshot = snapshot;
            
            LogChannel(n"COOP_SAVE", "Progression applied successfully");
            return true;
            
        } catch {
            LogChannel(n"COOP_SAVE", "Error applying progression: " + ToString(e));
            return false;
        }
    }
    
    private static func CreateProgressionBackup(snapshot: ProgressionSnapshot) -> Void {
        let backupData = ProgressionSync.SerializeSnapshot(snapshot);
        let backupFileName = saveBackupPath + "progression_backup_" + ProgressionSync.GetCurrentTimestamp() + ".json";
        
        // Save backup to file
        FileSystem.WriteStringToFile(backupFileName, backupData);
        
        LogChannel(n"COOP_SAVE", "Progression backup created: " + backupFileName);
    }
    
    private static func RestoreProgressionBackup() -> Bool {
        // Find most recent backup
        let backupFiles = FileSystem.ListFiles(saveBackupPath, "*.json");
        if ArraySize(backupFiles) == 0 {
            LogChannel(n"COOP_SAVE", "No backup files found for restoration");
            return false;
        }
        
        // Get most recent backup
        let latestBackup = backupFiles[ArraySize(backupFiles) - 1];
        let backupData = FileSystem.ReadStringFromFile(saveBackupPath + latestBackup);
        
        if Equals(backupData, "") {
            LogChannel(n"COOP_SAVE", "Failed to read backup file");
            return false;
        }
        
        // Deserialize and apply backup
        let backupSnapshot = ProgressionSync.DeserializeSnapshot(backupData);
        let success = ProgressionSync.ApplyProgression(backupSnapshot);
        
        if success {
            LogChannel(n"COOP_SAVE", "Progression restored from backup: " + latestBackup);
        } else {
            LogChannel(n"COOP_SAVE", "Failed to restore progression from backup");
        }
        
        return success;
    }
    
    // Network event handlers
    private static cb func OnProgressionRequested(requestData: String) -> Void {
        LogChannel(n"COOP_SAVE", "Received progression request");
        
        // Capture current progression and send to requester
        let mySnapshot = ProgressionSync.CaptureCurrentProgression();
        let progressionData = ProgressionSync.SerializeSnapshot(mySnapshot);
        NetworkingSystem.SendToHost("progression_data", progressionData);
    }
    
    private static cb func OnProgressionDataReceived(progressionData: String) -> Void {
        LogChannel(n"COOP_SAVE", "Received progression data from client");
        
        // Deserialize client progression
        let clientSnapshot = ProgressionSync.DeserializeSnapshot(progressionData);
        ArrayPush(clientSnapshots, clientSnapshot);
        
        // If host, analyze compatibility and decide synchronization strategy
        if NetworkingSystem.IsHost() {
            let compatibility = ProgressionSync.AnalyzeCompatibility(hostSnapshot, clientSnapshot);
            
            if compatibility.isCompatible || compatibility.recommendedMode != ProgressionSyncMode.AskPlayer {
                // Auto-sync if compatible or clear recommendation
                ProgressionSync.SynchronizeProgression(hostSnapshot, clientSnapshot, compatibility.recommendedMode);
            } else {
                // Present conflicts to host for decision
                ProgressionSync.PresentCompatibilityDialog(compatibility);
            }
        }
    }
    
    private static cb func OnProgressionSyncReceived(syncData: String) -> Void {
        LogChannel(n"COOP_SAVE", "Received progression synchronization data");
        
        // Apply synchronized progression
        let syncedSnapshot = ProgressionSync.DeserializeSnapshot(syncData);
        ProgressionSync.ApplyProgression(syncedSnapshot);
    }
    
    private static cb func OnSaveStateRequested(requestData: String) -> Void {
        // Handle save state requests for cross-session compatibility
        LogChannel(n"COOP_SAVE", "Save state requested");
        ProgressionSync.CaptureCurrentProgression();
    }
    
    // Utility functions
    private static func CaptureQuestProgress() -> array<QuestProgressData> {
        let questProgress: array<QuestProgressData>;
        
        let questSystem = GameInstance.GetQuestsSystem(GetGame());
        if !IsDefined(questSystem) {
            return questProgress;
        }
        
        let allQuests = questSystem.GetAllQuests();
        for quest in allQuests {
            let questData: QuestProgressData;
            questData.questId = quest.GetId();
            questData.phase = quest.GetCurrentPhase();
            questData.isCompleted = quest.IsCompleted();
            questData.isFailed = quest.IsFailed();
            // Additional quest data extraction would be implemented here
            ArrayPush(questProgress, questData);
        }
        
        return questProgress;
    }
    
    private static func CaptureWorldFlags() -> array<WorldFlag> {
        let worldFlags: array<WorldFlag>;
        
        let factSystem = GameInstance.GetFactsDatabase(GetGame());
        if !IsDefined(factSystem) {
            return worldFlags;
        }
        
        // Get all set world flags
        let allFacts = factSystem.GetAllFacts();
        for fact in allFacts {
            let worldFlag: WorldFlag;
            worldFlag.flagName = fact.GetName();
            worldFlag.flagValue = fact.GetValue() > 0;
            worldFlag.setTimestamp = fact.GetTimestamp();
            ArrayPush(worldFlags, worldFlag);
        }
        
        return worldFlags;
    }
    
    private static func CaptureRelationshipStates() -> array<RelationshipState> {
        let relationships: array<RelationshipState>;
        
        // This would integrate with CP2077's relationship system
        // Simplified implementation for demonstration
        let relationshipSystem = GameInstance.GetScriptableSystemsContainer(GetGame()).Get(n"RelationshipSystem");
        if IsDefined(relationshipSystem) {
            // Capture relationship data
            // Implementation would depend on actual game relationship system
        }
        
        return relationships;
    }
    
    private static func GenerateInventoryHash() -> String {
        let inventorySystem = GameInstance.GetInventoryManager(GetGame());
        if !IsDefined(inventorySystem) {
            return "00000000";
        }
        
        // Generate hash of inventory state
        // This is a simplified implementation
        let itemCount = inventorySystem.GetTotalItemCount();
        let hash = ToString(itemCount) + "_" + ToString(GetGameTime());
        return hash;
    }
    
    private static func GetCurrentTimestamp() -> String {
        let gameTime = GetGameTime();
        return ToString(Cast<Int64>(gameTime));
    }
    
    private static func EnsureBackupDirectory() -> Void {
        // Ensure backup directory exists
        FileSystem.CreateDirectory(saveBackupPath);
    }
    
    private static func SerializeSnapshot(snapshot: ProgressionSnapshot) -> String {
        // Simple serialization - would use JSON in real implementation
        let data = snapshot.playerId + "|" + ToString(snapshot.characterLevel) + "|" + ToString(snapshot.experience);
        data += "|" + ToString(snapshot.streetCred) + "|" + ToString(snapshot.availableSkillPoints);
        data += "|" + EnumValueToString("gamedataLifePath", Cast<Int64>(snapshot.lifePath));
        data += "|" + snapshot.inventoryHash + "|" + snapshot.lastSaveTimestamp;
        return data;
    }
    
    private static func DeserializeSnapshot(data: String) -> ProgressionSnapshot {
        let snapshot: ProgressionSnapshot;
        let parts = StrSplit(data, "|");
        
        if ArraySize(parts) >= 8 {
            snapshot.playerId = parts[0];
            snapshot.characterLevel = StringToInt(parts[1]);
            snapshot.experience = StringToInt(parts[2]);
            snapshot.streetCred = StringToInt(parts[3]);
            snapshot.availableSkillPoints = StringToInt(parts[4]);
            snapshot.lifePath = IntToEnum(StringToInt(parts[5]), gamedataLifePath.Invalid);
            snapshot.inventoryHash = parts[6];
            snapshot.lastSaveTimestamp = parts[7];
        }
        
        return snapshot;
    }
    
    // Additional utility functions would be implemented here...
    
    // Public API
    public static func GetCurrentSnapshot() -> ProgressionSnapshot {
        return currentSnapshot;
    }
    
    public static func IsProgressionSynced() -> Bool {
        return NetworkingSystem.IsConnected() && !Equals(currentSnapshot.playerId, "");
    }
    
    public static func RequestManualSync() -> Void {
        ProgressionSync.RequestProgressionSync();
    }
}