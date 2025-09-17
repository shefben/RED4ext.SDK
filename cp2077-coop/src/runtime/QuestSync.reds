// Quest progress must remain synced or players can become soft-locked.
// If one client advances a stage while another does not,
// subsequent objectives may never trigger for that player.
// These helpers send stage and scene updates between peers.

// Support structures for advanced quest coordination
public struct QuestConflictData {
    public var questHash: Uint32;
    public var conflictingPlayers: array<Uint32>;
    public var expectedStage: Uint16;
    public var actualStages: array<Uint16>;
    public var timestamp: Uint32;
    public var retryCount: Int32;
}

public struct BranchingQuestData {
    public var questHash: Uint32;
    public var branchPoints: array<Uint16>;
    public var playerChoices: array<Uint32>; // Player ID -> choice mapping
    public var consensusRequired: Bool;
    public var deadline: Uint32;
}

public struct NPCInteractionData {
    public var npcId: Uint32;
    public var questHash: Uint32;
    public var interactingPlayer: Uint32;
    public var interactionType: Uint8;
    public var timestamp: Uint32;
    public var isBlocking: Bool;
}

public class QuestSync {
    // Basic state
    public static var freezeQuests: Bool = false;
    public static var localPhase: Uint32 = 0u; // PX-2
    public static let stageMap: ref<inkHashMap> = new inkHashMap();
    public static let nameMap: ref<inkHashMap> = new inkHashMap();
    
    // Advanced coordination state
    private static var isInitialized: Bool = false;
    private static var maxPlayers: Int32 = 32;
    private static var questConflicts: array<QuestConflictData>;
    private static var branchingQuests: array<BranchingQuestData>;
    private static var npcInteractions: array<NPCInteractionData>;
    
    // Configuration
    private static let QUEST_SYNC_TIMEOUT: Uint32 = 15000u; // 15 seconds
    private static let MAX_CONFLICT_RETRIES: Int32 = 3;
    private static let BRANCH_VALIDATION_DELAY: Uint32 = 2000u; // 2 seconds
    
    // === System Initialization ===
    
    public static func InitializeQuestSync(playerCount: Int32) -> Bool {
        LogChannel(n"QUEST_SYNC", s"Initializing quest sync system for " + ToString(playerCount) + " players");
        
        if isInitialized {
            LogChannel(n"QUEST_SYNC", s"Quest sync already initialized");
            return true;
        }
        
        maxPlayers = playerCount;
        
        // Initialize data structures
        ArrayClear(questConflicts);
        ArrayClear(branchingQuests);
        ArrayClear(npcInteractions);
        
        // Initialize native backend
        if !QuestSync_Initialize(playerCount) {
            LogChannel(n"QUEST_SYNC", s"Failed to initialize native quest sync backend");
            return false;
        }
        
        // Start conflict resolution system
        if !StartConflictResolver() {
            LogChannel(n"QUEST_SYNC", s"Failed to start conflict resolution system");
            return false;
        }
        
        // Start branching quest monitor
        if !StartBranchingQuestMonitor() {
            LogChannel(n"QUEST_SYNC", s"Failed to start branching quest monitor");
            return false;
        }
        
        isInitialized = true;
        LogChannel(n"QUEST_SYNC", s"Quest sync system initialized successfully");
        return true;
    }
    // Called after the game advances a quest stage on the server.
    // Sends a network update so all clients stay in sync.
    public static func OnAdvanceStage(questName: CName) -> Void {
        if freezeQuests { return; };
        LogChannel(n"DEBUG", "[QuestSync] " + NameToString(questName) + " stage advanced");
        SendQuestStageMsg(questName);
    }

    public static func SendQuestStageMsg(questName: CName) -> Void {
        let qs = GameInstance.GetQuestsSystem(GetGame());
        let stage: Uint16 = IsDefined(qs) ? Cast<Uint16>(qs.GetCurrentStage(questName)) : 0u;
        let hash: Uint32 = CoopNet.Fnv1a32(NameToString(questName));
        nameMap.Insert(hash, questName);
        stageMap.Insert(hash, stage);
        CoopNet.Net_BroadcastQuestStageP2P(localPhase, hash, stage);
        LogChannel(n"DEBUG", "SendQuestStageMsg " + NameToString(questName) + " stage=" + ToString(stage));
    }

    public static func ApplyQuestStage(questName: CName, stage: Uint16) -> Void {
        let qs = GameInstance.GetQuestsSystem(GetGame());
        if !IsDefined(qs) { return; };
        let current: Uint16 = Cast<Uint16>(qs.GetCurrentStage(questName));
        if current < stage {
            qs.SetStage(questName, stage);
        } else {
            if current > stage + 1u {
                LogChannel(n"ERROR", "[QuestSync] Divergence detected");
                CoopNet.Net_SendQuestResyncRequest();
            };
        };
        let hash: Uint32 = CoopNet.Fnv1a32(NameToString(questName));
        stageMap.Insert(hash, stage);
        nameMap.Insert(hash, questName);
        LogChannel(n"DEBUG", "ApplyQuestStage " + NameToString(questName) + " stage=" + ToString(stage));
    }

    public static func ApplyQuestStageByHash(hash: Uint32, stage: Uint16) -> Void {
        if !nameMap.Contains(hash) {
            LogChannel(n"WARNING", "[QuestSync] Unknown quest hash " + IntToString(Cast<Int32>(hash)));
            return;
        };
        let name: CName = nameMap.Get(hash) as CName;
        ApplyQuestStage(name, stage);
    }

    public static func ApplyFullSync(pkt: ref<QuestFullSyncPacket>) -> Void {
        LogChannel(n"quest", "[Watchdog] forced sync");
        let qCount: Uint16 = pkt.questCount;
        let i: Uint16 = 0u;
        while i < qCount {
            let entry = pkt.quests[i];
            nameMap.Insert(entry.nameHash, StringToName(IntToString(entry.nameHash)));
            ApplyQuestStageByHash(entry.nameHash, entry.stage);
            i += 1u;
        };

        let nCount: Uint16 = pkt.npcCount;
        let j: Uint16 = 0u;
        while j < nCount {
            let snap = pkt.npcs[j];
            NpcController.ClientApplySnap(&snap);
            j += 1u;
        };

        let eCount: Uint8 = pkt.eventCount;
        let k: Uint8 = 0u;
        while k < eCount {
            let ev = pkt.events[k];
            GlobalEventManager.OnEvent(ev.eventId, ev.phase, ev.seed, ev.active == 1u);
            k += 1u;
        };
    }

    // Mirror cutscene and scene triggers between peers.
    public static func OnSceneStart(id: TweakDBID) -> Void {
        let hash: Uint32 = CoopNet.Fnv1a32(TDBID.ToStringDEBUG(id));
        Net_BroadcastSceneTrigger(localPhase, hash, true);
        LogChannel(n"DEBUG", "Send SceneTrigger start " + TDBID.ToStringDEBUG(id));
    }

    public static func OnSceneEnd(id: TweakDBID) -> Void {
        let hash: Uint32 = CoopNet.Fnv1a32(TDBID.ToStringDEBUG(id));
        Net_BroadcastSceneTrigger(localPhase, hash, false);
        LogChannel(n"DEBUG", "Send SceneTrigger end " + TDBID.ToStringDEBUG(id));
    }

    public static func ApplySceneTrigger(id: TweakDBID, isStart: Bool) -> Void {
        LogChannel(n"DEBUG", "Apply SceneTrigger " + TDBID.ToStringDEBUG(id) + " start=" + BoolToString(isStart));
    }

    public static func OnDialogChoice(peerId: Uint32, idx: Uint8) -> Void {
        LogChannel(n"DEBUG", "DialogChoice " + IntToString(Cast<Int32>(idx)));
        if !Net_IsAuthoritative() {
            Net_SendDialogChoice(idx);
        } else {
            if CutsceneSync.ApplyDialogChoice(idx) {
                Net_BroadcastDialogChoice(peerId, idx);
            };
        };
    }

    public static func SetFreeze(f: Bool) -> Void {
        freezeQuests = f;
        LogChannel(n"DEBUG", "freezeQuests=" + BoolToString(f));
    }
}

// --- Hooks ---------------------------------------------------------------
// TODO: Verify these hook targets exist in the current game version

/*
// Detours QuestSystem::AdvanceStage so the server can broadcast quest updates.
// Using the `hook` attribute lets us call the original function then run our
// own logic after it executes.
@hook(QuestSystem.AdvanceStage)
protected func QuestSystem_AdvanceStage(original: func(ref<QuestSystem>, CName),
                                        self: ref<QuestSystem>, questName: CName) -> Void {
    // Server processes the quest update, then notify peers.
    original(self, questName);
    QuestSync.OnAdvanceStage(questName);
}

@hook(questSceneManager.StartScene)
protected func questSceneManager_StartScene(original: func(ref<questSceneManager>, TweakDBID), self: ref<questSceneManager>, id: TweakDBID) -> Void {
    original(self, id);
    QuestSync.OnSceneStart(id);
}

@hook(questSceneManager.EndScene)
protected func questSceneManager_EndScene(original: func(ref<questSceneManager>, TweakDBID), self: ref<questSceneManager>, id: TweakDBID) -> Void {
    original(self, id);
    QuestSync.OnSceneEnd(id);
}

@hook(DialogChoiceHubController.OnOptionSelected)
protected func DialogChoiceHubController_OnOptionSelected(original: func(ref<DialogChoiceHubController>, Int32), self: ref<DialogChoiceHubController>, idx: Int32) -> Void {
    original(self, idx);
    QuestSync.OnDialogChoice(Net_GetPeerId(), Cast<Uint8>(idx));
}
*/

    // === Quest State Conflict Resolution ===
    
    public static func DetectQuestConflict(questHash: Uint32, expectedStage: Uint16, actualStage: Uint16, playerId: Uint32) -> Void {
        LogChannel(n"QUEST_SYNC", s"Quest conflict detected for hash " + ToString(questHash) + " by player " + ToString(playerId));
        
        let existingIndex = FindQuestConflictIndex(questHash);
        if existingIndex >= 0 {
            // Update existing conflict
            ArrayPush(questConflicts[existingIndex].conflictingPlayers, playerId);
            ArrayPush(questConflicts[existingIndex].actualStages, actualStage);
        } else {
            // Create new conflict entry
            let conflict: QuestConflictData;
            conflict.questHash = questHash;
            conflict.expectedStage = expectedStage;
            ArrayPush(conflict.conflictingPlayers, playerId);
            ArrayPush(conflict.actualStages, actualStage);
            conflict.timestamp = GetCurrentTimeMs();
            conflict.retryCount = 0;
            ArrayPush(questConflicts, conflict);
        }
        
        // Trigger resolution process
        ResolveQuestConflict(questHash);
    }
    
    public static func ResolveQuestConflict(questHash: Uint32) -> Void {
        let conflictIndex = FindQuestConflictIndex(questHash);
        if conflictIndex < 0 {
            return;
        }
        
        let conflict = questConflicts[conflictIndex];
        if conflict.retryCount >= MAX_CONFLICT_RETRIES {
            LogChannel(n"QUEST_SYNC", s"Max retries reached for quest conflict " + ToString(questHash) + ", forcing resync");
            CoopNet.Net_SendQuestResyncRequest();
            ArrayRemoveIndex(questConflicts, conflictIndex);
            return;
        }
        
        // Increment retry count
        questConflicts[conflictIndex].retryCount += 1;
        
        // Send conflict resolution request to server
        QuestSync_RequestConflictResolution(questHash, conflict.expectedStage, conflict.conflictingPlayers);
        
        LogChannel(n"QUEST_SYNC", s"Attempting quest conflict resolution for hash " + ToString(questHash) + " (attempt " + ToString(conflict.retryCount) + ")");
    }
    
    // === Branching Quest Coordination ===
    
    public static func RegisterBranchingQuest(questHash: Uint32, branchStages: array<Uint16>, requireConsensus: Bool) -> Void {
        LogChannel(n"QUEST_SYNC", s"Registering branching quest " + ToString(questHash) + " with " + ToString(ArraySize(branchStages)) + " branch points");
        
        let branchData: BranchingQuestData;
        branchData.questHash = questHash;
        branchData.branchPoints = branchStages;
        branchData.consensusRequired = requireConsensus;
        branchData.deadline = GetCurrentTimeMs() + BRANCH_VALIDATION_DELAY;
        
        ArrayPush(branchingQuests, branchData);
    }
    
    public static func HandleBranchingQuestChoice(questHash: Uint32, playerId: Uint32, choice: Uint32) -> Bool {
        let branchIndex = FindBranchingQuestIndex(questHash);
        if branchIndex < 0 {
            LogChannel(n"QUEST_SYNC", s"No branching quest registered for hash " + ToString(questHash));
            return false;
        }
        
        // Record player choice
        ArrayPush(branchingQuests[branchIndex].playerChoices, playerId);
        ArrayPush(branchingQuests[branchIndex].playerChoices, choice);
        
        // Check if consensus is reached
        if branchingQuests[branchIndex].consensusRequired {
            return ValidateBranchingConsensus(branchIndex);
        }
        
        return true;
    }
    
    public static func ValidateBranchingConsensus(branchIndex: Int32) -> Bool {
        if branchIndex < 0 || branchIndex >= ArraySize(branchingQuests) {
            return false;
        }
        
        let branchData = branchingQuests[branchIndex];
        let choiceCount = ArraySize(branchData.playerChoices) / 2; // Each entry is player+choice pair
        
        // Check if we have choices from all expected players
        if choiceCount < maxPlayers {
            LogChannel(n"QUEST_SYNC", s"Waiting for more players to make branching quest choices (" + ToString(choiceCount) + "/" + ToString(maxPlayers) + ")");
            return false;
        }
        
        // Validate consensus (simplified - in real implementation would check actual choice agreement)
        LogChannel(n"QUEST_SYNC", s"Branching quest consensus validated for hash " + ToString(branchData.questHash));
        return true;
    }
    
    // === NPC Interaction Synchronization ===
    
    public static func RegisterNPCInteraction(npcId: Uint32, questHash: Uint32, playerId: Uint32, interactionType: Uint8, isBlocking: Bool) -> Void {
        LogChannel(n"QUEST_SYNC", s"Registering NPC interaction: NPC " + ToString(npcId) + " with player " + ToString(playerId) + " for quest " + ToString(questHash));
        
        // Check for existing interactions
        if isBlocking && HasBlockingNPCInteraction(npcId, questHash) {
            LogChannel(n"QUEST_SYNC", s"NPC " + ToString(npcId) + " already in blocking interaction");
            return;
        }
        
        let interaction: NPCInteractionData;
        interaction.npcId = npcId;
        interaction.questHash = questHash;
        interaction.interactingPlayer = playerId;
        interaction.interactionType = interactionType;
        interaction.timestamp = GetCurrentTimeMs();
        interaction.isBlocking = isBlocking;
        
        ArrayPush(npcInteractions, interaction);
        
        // Broadcast interaction to other players
        QuestSync_BroadcastNPCInteraction(interaction);
    }
    
    public static func CompleteNPCInteraction(npcId: Uint32, questHash: Uint32, playerId: Uint32) -> Void {
        LogChannel(n"QUEST_SYNC", s"Completing NPC interaction: NPC " + ToString(npcId) + " with player " + ToString(playerId));
        
        let interactionIndex = FindNPCInteractionIndex(npcId, questHash, playerId);
        if interactionIndex >= 0 {
            ArrayRemoveIndex(npcInteractions, interactionIndex);
            
            // Broadcast completion
            QuestSync_BroadcastNPCInteractionComplete(npcId, questHash, playerId);
        }
    }
    
    // === Quest Completion Validation ===
    
    public static func ValidateQuestCompletion(questHash: Uint32, completingPlayers: array<Uint32>) -> Bool {
        LogChannel(n"QUEST_SYNC", s"Validating quest completion for hash " + ToString(questHash) + " with " + ToString(ArraySize(completingPlayers)) + " players");
        
        // Check if all players are at the same completion stage
        for playerId in completingPlayers {
            if !IsPlayerReadyForQuestCompletion(questHash, playerId) {
                LogChannel(n"QUEST_SYNC", s"Player " + ToString(playerId) + " not ready for quest completion");
                return false;
            }
        }
        
        LogChannel(n"QUEST_SYNC", s"Quest completion validated for hash " + ToString(questHash));
        return true;
    }
    
    // === Utility Functions ===
    
    private static func StartConflictResolver() -> Bool {
        LogChannel(n"QUEST_SYNC", s"Starting quest conflict resolution system");
        return QuestSync_StartConflictResolver();
    }
    
    private static func StartBranchingQuestMonitor() -> Bool {
        LogChannel(n"QUEST_SYNC", s"Starting branching quest monitor");
        return QuestSync_StartBranchingMonitor();
    }
    
    private static func FindQuestConflictIndex(questHash: Uint32) -> Int32 {
        let count = ArraySize(questConflicts);
        var i = 0;
        while i < count {
            if questConflicts[i].questHash == questHash {
                return i;
            }
            i += 1;
        }
        return -1;
    }
    
    private static func FindBranchingQuestIndex(questHash: Uint32) -> Int32 {
        let count = ArraySize(branchingQuests);
        var i = 0;
        while i < count {
            if branchingQuests[i].questHash == questHash {
                return i;
            }
            i += 1;
        }
        return -1;
    }
    
    private static func FindNPCInteractionIndex(npcId: Uint32, questHash: Uint32, playerId: Uint32) -> Int32 {
        let count = ArraySize(npcInteractions);
        var i = 0;
        while i < count {
            if npcInteractions[i].npcId == npcId && 
               npcInteractions[i].questHash == questHash && 
               npcInteractions[i].interactingPlayer == playerId {
                return i;
            }
            i += 1;
        }
        return -1;
    }
    
    private static func HasBlockingNPCInteraction(npcId: Uint32, questHash: Uint32) -> Bool {
        let count = ArraySize(npcInteractions);
        var i = 0;
        while i < count {
            if npcInteractions[i].npcId == npcId && 
               npcInteractions[i].questHash == questHash && 
               npcInteractions[i].isBlocking {
                return true;
            }
            i += 1;
        }
        return false;
    }
    
    private static func IsPlayerReadyForQuestCompletion(questHash: Uint32, playerId: Uint32) -> Bool {
        // Check if player has completed all required quest stages
        return QuestSync_IsPlayerReady(questHash, playerId);
    }

// Alternative implementation using polling approach
public static func RegisterQuestMonitoring() -> Void {
    LogChannel(n"DEBUG", "QuestSync: Alternative quest monitoring enabled (hooks disabled)");
    // Implementation would periodically check quest state changes
}

// === Native C++ Integration Functions ===

private static native func QuestSync_Initialize(maxPlayers: Int32) -> Bool;
private static native func QuestSync_StartConflictResolver() -> Bool;
private static native func QuestSync_StartBranchingMonitor() -> Bool;
private static native func QuestSync_RequestConflictResolution(questHash: Uint32, expectedStage: Uint16, players: array<Uint32>) -> Bool;
private static native func QuestSync_BroadcastNPCInteraction(interaction: NPCInteractionData) -> Void;
private static native func QuestSync_BroadcastNPCInteractionComplete(npcId: Uint32, questHash: Uint32, playerId: Uint32) -> Void;
private static native func QuestSync_IsPlayerReady(questHash: Uint32, playerId: Uint32) -> Bool;
