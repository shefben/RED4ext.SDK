// NPC State Synchronization System
// Automatic synchronization of all NPCs including quest NPCs, police, enemies, civilians, vendors
// Seamlessly integrates with singleplayer AI systems without manual configuration

// NPC Synchronization Manager - coordinates all NPC state synchronization
public class NPCSyncManager extends ScriptableSystem {
    private static let s_instance: ref<NPCSyncManager>;
    private let m_trackedNPCs: array<ref<NPCStateTracker>>;
    private let m_questNPCs: array<ref<QuestNPCTracker>>;
    private let m_policeSystem: ref<PoliceSystemSync>;
    private let m_enemyGroups: ref<EnemyGroupSync>;
    private let m_civilianTraffic: ref<CivilianTrafficSync>;
    private let m_vendorSystem: ref<VendorSystemSync>;
    private let m_syncTimer: Float = 0.0;
    private let m_npcSyncInterval: Float = 0.1; // 10 FPS for NPC updates
    private let m_questSyncInterval: Float = 0.05; // 20 FPS for quest NPCs
    private let m_policeSyncInterval: Float = 0.05; // 20 FPS for police (high priority)
    private let m_localPlayer: wref<PlayerPuppet>;

    public static func GetInstance() -> ref<NPCSyncManager> {
        if !IsDefined(NPCSyncManager.s_instance) {
            NPCSyncManager.s_instance = new NPCSyncManager();
        }
        return NPCSyncManager.s_instance;
    }

    public func Initialize(player: ref<PlayerPuppet>) -> Void {
        this.m_localPlayer = player;
        this.InitializeNPCTracking();
        this.StartAutomaticSystems();
        LogChannel(n"NPCSync", s"[NPCSync] NPC Sync Manager initialized");
    }

    private func InitializeNPCTracking() -> Void {
        // Initialize police system sync
        this.m_policeSystem = new PoliceSystemSync();
        this.m_policeSystem.Initialize();

        // Initialize enemy group sync
        this.m_enemyGroups = new EnemyGroupSync();
        this.m_enemyGroups.Initialize();

        // Initialize civilian traffic sync
        this.m_civilianTraffic = new CivilianTrafficSync();
        this.m_civilianTraffic.Initialize();

        // Initialize vendor system sync
        this.m_vendorSystem = new VendorSystemSync();
        this.m_vendorSystem.Initialize();

        // Automatically scan for existing NPCs
        this.ScanExistingNPCs();

        LogChannel(n"NPCSync", s"[NPCSync] NPC tracking systems initialized");
    }

    private func StartAutomaticSystems() -> Void {
        // Enable automatic detection for all NPC systems
        this.m_policeSystem.EnableAutomaticDetection();
        this.m_civilianTraffic.EnableAutomaticDetection();
        this.m_vendorSystem.EnableAutomaticDetection();

        LogChannel(n"NPCSync", s"[NPCSync] Automatic NPC systems enabled");
    }

    private func ScanExistingNPCs() -> Void {
        // Automatically detect all NPCs in the world using game systems
        let npcSystem = GameInstance.GetNPCSystem(GetGameInstance());
        let allNPCs = npcSystem.GetAllNPCs();

        for npc in allNPCs {
            this.StartTrackingNPC(npc);
        }

        LogChannel(n"NPCSync", s"[NPCSync] Now tracking " + ArraySize(this.m_trackedNPCs) + " NPCs");
    }

    public func OnUpdate(deltaTime: Float) -> Void {
        this.m_syncTimer += deltaTime;

        if this.m_syncTimer >= this.m_npcSyncInterval {
            this.SynchronizeNPCStates();
            this.m_syncTimer = 0.0;
        }

        // Update subsystems at different frequencies
        this.m_policeSystem.Update(deltaTime);
        this.m_enemyGroups.Update(deltaTime);
        this.m_civilianTraffic.Update(deltaTime);
        this.m_vendorSystem.Update(deltaTime);
        this.UpdateQuestNPCs(deltaTime);
    }

    private func SynchronizeNPCStates() -> Void {
        for tracker in this.m_trackedNPCs {
            if tracker.HasStateChanged() {
                let syncData = this.CreateNPCSyncData(tracker);
                Net_SendNPCUpdate(syncData);
                tracker.MarkAsSynced();
            }
        }
    }

    private func UpdateQuestNPCs(deltaTime: Float) -> Void {
        // High-priority sync for quest NPCs
        for questNPC in this.m_questNPCs {
            questNPC.Update(deltaTime);
            if questNPC.HasCriticalStateChange() {
                let questData = questNPC.GetQuestSyncData();
                Net_SendQuestNPCUpdate(questData);
                questNPC.MarkQuestSynced();
            }
        }
    }

    private func CreateNPCSyncData(tracker: ref<NPCStateTracker>) -> NPCSyncData {
        let syncData: NPCSyncData;
        syncData.npcId = tracker.GetNPCId();
        syncData.position = tracker.GetPosition();
        syncData.rotation = tracker.GetRotation();
        syncData.animationState = tracker.GetAnimationState();
        syncData.behaviorState = tracker.GetBehaviorState();
        syncData.health = tracker.GetHealth();
        syncData.combatState = tracker.GetCombatState();
        syncData.dialogueState = tracker.GetDialogueState();
        syncData.isAlive = tracker.IsAlive();
        syncData.timestamp = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

        return syncData;
    }

    // Automatic NPC detection when NPCs spawn
    public func OnNPCSpawned(npc: ref<NPCPuppet>) -> Void {
        this.StartTrackingNPC(npc);

        // Determine NPC type and configure accordingly
        let npcType = this.DetermineNPCType(npc);
        this.ConfigureNPCForSync(npc, npcType);

        // Broadcast spawn to other players
        let spawnData: NPCSpawnData;
        spawnData.npcId = Cast<Uint64>(npc.GetEntityID());
        spawnData.npcRecord = npc.GetRecordID();
        spawnData.position = npc.GetWorldPosition();
        spawnData.rotation = npc.GetWorldOrientation();
        spawnData.npcType = npcType;
        spawnData.timestamp = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

        Net_SendNPCSpawn(spawnData);

        LogChannel(n"NPCSync", s"[NPCSync] NPC spawned and synced: " + spawnData.npcId + " Type: " + EnumValueToString("ENPCType", Cast<Int64>(EnumInt(npcType))));
    }

    public func OnNPCDespawned(npcId: Uint64) -> Void {
        this.StopTrackingNPC(npcId);

        // Broadcast despawn to other players
        let despawnData: NPCDespawnData;
        despawnData.npcId = npcId;
        despawnData.timestamp = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

        Net_SendNPCDespawn(despawnData);

        LogChannel(n"NPCSync", s"[NPCSync] NPC despawned: " + npcId);
    }

    private func StartTrackingNPC(npc: ref<NPCPuppet>) -> Void {
        let tracker = new NPCStateTracker();
        tracker.Initialize(npc);
        ArrayPush(this.m_trackedNPCs, tracker);

        // If this is a quest NPC, add to special tracking
        if this.IsQuestNPC(npc) {
            let questTracker = new QuestNPCTracker();
            questTracker.Initialize(npc);
            ArrayPush(this.m_questNPCs, questTracker);
        }

        // Route to specialized systems
        let npcType = this.DetermineNPCType(npc);
        switch npcType {
            case ENPCType.Police:
                this.m_policeSystem.RegisterNPC(npc);
                break;
            case ENPCType.Enemy:
                this.m_enemyGroups.RegisterNPC(npc);
                break;
            case ENPCType.Civilian:
                this.m_civilianTraffic.RegisterNPC(npc);
                break;
            case ENPCType.Vendor:
                this.m_vendorSystem.RegisterNPC(npc);
                break;
        }
    }

    private func StopTrackingNPC(npcId: Uint64) -> Void {
        // Remove from main tracking
        let index = -1;
        for i in Range(ArraySize(this.m_trackedNPCs)) {
            if this.m_trackedNPCs[i].GetNPCId() == npcId {
                index = i;
                break;
            }
        }

        if index >= 0 {
            ArrayRemove(this.m_trackedNPCs, this.m_trackedNPCs[index]);
        }

        // Remove from quest tracking
        let questIndex = -1;
        for i in Range(ArraySize(this.m_questNPCs)) {
            if this.m_questNPCs[i].GetNPCId() == npcId {
                questIndex = i;
                break;
            }
        }

        if questIndex >= 0 {
            ArrayRemove(this.m_questNPCs, this.m_questNPCs[questIndex]);
        }

        // Remove from specialized systems
        this.m_policeSystem.UnregisterNPC(npcId);
        this.m_enemyGroups.UnregisterNPC(npcId);
        this.m_civilianTraffic.UnregisterNPC(npcId);
        this.m_vendorSystem.UnregisterNPC(npcId);
    }

    private func DetermineNPCType(npc: ref<NPCPuppet>) -> ENPCType {
        // Automatically determine NPC type from game data
        let npcRecord = npc.GetRecord();

        if this.IsPoliceNPC(npc) {
            return ENPCType.Police;
        } else if this.IsEnemyNPC(npc) {
            return ENPCType.Enemy;
        } else if this.IsVendorNPC(npc) {
            return ENPCType.Vendor;
        } else if this.IsQuestNPC(npc) {
            return ENPCType.QuestNPC;
        }

        return ENPCType.Civilian;
    }

    private func IsPoliceNPC(npc: ref<NPCPuppet>) -> Bool {
        // Check if NPC is part of police system
        let npcRecord = npc.GetRecord();
        return npcRecord.Affiliation().Type() == gamedataAffiliation.NCPD;
    }

    private func IsEnemyNPC(npc: ref<NPCPuppet>) -> Bool {
        // Check if NPC is hostile
        let attitudeSystem = GameInstance.GetAttitudeSystem(npc.GetGame());
        let playerID = GetPlayer(npc.GetGame()).GetEntityID();
        let attitude = attitudeSystem.GetAttitudeTowards(npc.GetEntityID(), playerID);

        return Equals(attitude, EAIAttitude.AIA_Hostile);
    }

    private func IsVendorNPC(npc: ref<NPCPuppet>) -> Bool {
        // Check if NPC has vendor component
        let vendorComponent = npc.GetComponent(n"VendorComponent");
        return IsDefined(vendorComponent);
    }

    private func IsQuestNPC(npc: ref<NPCPuppet>) -> Bool {
        // Check if NPC is involved in any quests
        let questSystem = GameInstance.GetQuestSystem(npc.GetGame());
        return questSystem.IsNPCInvolvedInQuest(npc.GetEntityID());
    }

    private func ConfigureNPCForSync(npc: ref<NPCPuppet>, npcType: ENPCType) -> Void {
        // Configure NPC based on type to maintain singleplayer behavior
        switch npcType {
            case ENPCType.Police:
                this.ConfigurePoliceNPC(npc);
                break;
            case ENPCType.Enemy:
                this.ConfigureEnemyNPC(npc);
                break;
            case ENPCType.QuestNPC:
                this.ConfigureQuestNPC(npc);
                break;
            case ENPCType.Vendor:
                this.ConfigureVendorNPC(npc);
                break;
            case ENPCType.Civilian:
                this.ConfigureCivilianNPC(npc);
                break;
        }
    }

    private func ConfigurePoliceNPC(npc: ref<NPCPuppet>) -> Void {
        // Maintain police AI behavior from singleplayer
        let aiComponent = npc.GetAIControllerComponent();
        if IsDefined(aiComponent) {
            aiComponent.SetBehaviorArgument(n"PoliceAI", ToVariant(true));
            aiComponent.SetBehaviorArgument(n"respondToWantedLevel", ToVariant(true));
        }
    }

    private func ConfigureEnemyNPC(npc: ref<NPCPuppet>) -> Void {
        // Maintain enemy AI behavior from singleplayer
        let aiComponent = npc.GetAIControllerComponent();
        if IsDefined(aiComponent) {
            aiComponent.SetBehaviorArgument(n"HostileAI", ToVariant(true));
            aiComponent.SetBehaviorArgument(n"engagePlayer", ToVariant(true));
        }
    }

    private func ConfigureQuestNPC(npc: ref<NPCPuppet>) -> Void {
        // Maintain quest NPC behavior from singleplayer
        let aiComponent = npc.GetAIControllerComponent();
        if IsDefined(aiComponent) {
            aiComponent.SetBehaviorArgument(n"QuestNPC", ToVariant(true));
            aiComponent.SetBehaviorArgument(n"followQuestLogic", ToVariant(true));
        }
    }

    private func ConfigureVendorNPC(npc: ref<NPCPuppet>) -> Void {
        // Maintain vendor behavior from singleplayer
        let vendorComponent = npc.GetComponent(n"VendorComponent");
        if IsDefined(vendorComponent) {
            // Vendor AI remains the same as singleplayer
        }
    }

    private func ConfigureCivilianNPC(npc: ref<NPCPuppet>) -> Void {
        // Maintain civilian AI behavior from singleplayer
        let aiComponent = npc.GetAIControllerComponent();
        if IsDefined(aiComponent) {
            aiComponent.SetBehaviorArgument(n"CivilianAI", ToVariant(true));
            aiComponent.SetBehaviorArgument(n"reactToViolence", ToVariant(true));
        }
    }

    // Network event handlers
    public func OnRemoteNPCUpdate(syncData: NPCSyncData) -> Void {
        let tracker = this.FindNPCTracker(syncData.npcId);
        if IsDefined(tracker) {
            tracker.UpdateFromRemote(syncData);
        }
    }

    public func OnRemoteNPCSpawn(spawnData: NPCSpawnData) -> Void {
        LogChannel(n"NPCSync", s"[NPCSync] Remote NPC spawn: " + spawnData.npcId);
        this.SpawnNPCFromRemote(spawnData);
    }

    public func OnRemoteNPCDespawn(despawnData: NPCDespawnData) -> Void {
        LogChannel(n"NPCSync", s"[NPCSync] Remote NPC despawn: " + despawnData.npcId);
        this.DespawnNPCFromRemote(despawnData.npcId);
    }

    private func SpawnNPCFromRemote(spawnData: NPCSpawnData) -> Void {
        // Spawn NPC using the same systems as singleplayer
        let npcSystem = GameInstance.GetNPCSystem(GetGameInstance());

        let spawnTransform: WorldTransform;
        spawnTransform.Position = spawnData.position;
        spawnTransform.Orientation = spawnData.rotation;

        // Spawn NPC with same AI behavior as singleplayer
        let npc = npcSystem.SpawnNPC(spawnData.npcRecord, spawnTransform);

        if IsDefined(npc) {
            this.StartTrackingNPC(npc);
            this.ConfigureNPCForSync(npc, spawnData.npcType);
        }
    }

    private func DespawnNPCFromRemote(npcId: Uint64) -> Void {
        let tracker = this.FindNPCTracker(npcId);
        if IsDefined(tracker) {
            let npc = tracker.GetNPC();
            if IsDefined(npc) {
                let npcSystem = GameInstance.GetNPCSystem(GetGameInstance());
                npcSystem.DespawnNPC(npc);
            }
        }
    }

    private func FindNPCTracker(npcId: Uint64) -> ref<NPCStateTracker> {
        for tracker in this.m_trackedNPCs {
            if tracker.GetNPCId() == npcId {
                return tracker;
            }
        }
        return null;
    }
}

// Individual NPC State Tracker
public class NPCStateTracker extends ScriptableComponent {
    private let m_npc: wref<NPCPuppet>;
    private let m_npcId: Uint64;
    private let m_lastPosition: Vector3;
    private let m_lastRotation: Quaternion;
    private let m_lastAnimState: CName;
    private let m_lastBehaviorState: gamedataNPCBehaviorState;
    private let m_lastHealth: Float;
    private let m_lastCombatState: ENPCCombatState;
    private let m_hasStateChanged: Bool;

    public func Initialize(npc: ref<NPCPuppet>) -> Void {
        this.m_npc = npc;
        this.m_npcId = Cast<Uint64>(npc.GetEntityID());
        this.UpdateCurrentState();
        this.m_hasStateChanged = true; // Initial state change for first sync
    }

    public func UpdateCurrentState() -> Void {
        if !IsDefined(this.m_npc) {
            return;
        }

        let currentPosition = this.m_npc.GetWorldPosition();
        let currentRotation = this.m_npc.GetWorldOrientation();
        let currentAnimState = this.GetCurrentAnimation();
        let currentBehaviorState = this.GetBehaviorState();
        let currentHealth = this.GetNPCHealth();
        let currentCombatState = this.GetCombatState();

        // Check for significant changes
        if Vector4.Distance(this.m_lastPosition, currentPosition) > 0.2 ||
           !this.QuaternionEquals(this.m_lastRotation, currentRotation, 0.02) ||
           !Equals(this.m_lastAnimState, currentAnimState) ||
           !Equals(this.m_lastBehaviorState, currentBehaviorState) ||
           AbsF(this.m_lastHealth - currentHealth) > 5.0 ||
           !Equals(this.m_lastCombatState, currentCombatState) {
            this.m_hasStateChanged = true;
        }

        this.m_lastPosition = currentPosition;
        this.m_lastRotation = currentRotation;
        this.m_lastAnimState = currentAnimState;
        this.m_lastBehaviorState = currentBehaviorState;
        this.m_lastHealth = currentHealth;
        this.m_lastCombatState = currentCombatState;
    }

    public func UpdateFromRemote(syncData: NPCSyncData) -> Void {
        if !IsDefined(this.m_npc) {
            return;
        }

        // Apply remote state while maintaining AI behavior
        this.InterpolatePosition(syncData.position);
        this.InterpolateRotation(syncData.rotation);
        this.UpdateAnimation(syncData.animationState);
        this.UpdateBehaviorState(syncData.behaviorState);
        this.UpdateHealth(syncData.health);
        this.UpdateCombatState(syncData.combatState);
    }

    private func InterpolatePosition(targetPosition: Vector3) -> Void {
        // Smooth position interpolation for NPCs
        let currentPosition = this.m_npc.GetWorldPosition();
        let distance = Vector4.Distance(currentPosition, targetPosition);

        // Only teleport if distance is significant to avoid jitter
        if distance > 1.0 {
            this.m_npc.Teleport(targetPosition);
        }
    }

    private func InterpolateRotation(targetRotation: Quaternion) -> Void {
        // Smooth rotation interpolation
        this.m_npc.SetWorldOrientation(targetRotation);
    }

    private func UpdateAnimation(animState: CName) -> Void {
        // Update NPC animation state
        let animationComponent = this.m_npc.GetAnimationComponent();
        if IsDefined(animationComponent) {
            animationComponent.SetAnimationState(animState);
        }
    }

    private func UpdateBehaviorState(behaviorState: gamedataNPCBehaviorState) -> Void {
        // Update NPC behavior state while preserving AI
        let aiComponent = this.m_npc.GetAIControllerComponent();
        if IsDefined(aiComponent) {
            aiComponent.SetBehaviorArgument(n"behaviorState", ToVariant(EnumInt(behaviorState)));
        }
    }

    private func UpdateHealth(newHealth: Float) -> Void {
        let healthSystem = GameInstance.GetStatPoolsSystem(this.m_npc.GetGame());
        let npcID = Cast<StatsObjectID>(this.m_npc.GetEntityID());
        healthSystem.RequestSettingStatPoolValue(npcID, gamedataStatPoolType.Health, newHealth, null);
    }

    private func UpdateCombatState(combatState: ENPCCombatState) -> Void {
        // Update combat state while maintaining AI behavior
        let aiComponent = this.m_npc.GetAIControllerComponent();
        if IsDefined(aiComponent) {
            aiComponent.SetBehaviorArgument(n"combatState", ToVariant(EnumInt(combatState)));
        }
    }

    private func GetCurrentAnimation() -> CName {
        let animationComponent = this.m_npc.GetAnimationComponent();
        if IsDefined(animationComponent) {
            return animationComponent.GetCurrentAnimationState();
        }
        return n"";
    }

    private func GetBehaviorState() -> gamedataNPCBehaviorState {
        // Get current behavior state from AI component
        return gamedataNPCBehaviorState.Normal; // Placeholder
    }

    private func GetNPCHealth() -> Float {
        let healthSystem = GameInstance.GetStatPoolsSystem(this.m_npc.GetGame());
        let npcID = Cast<StatsObjectID>(this.m_npc.GetEntityID());
        let healthPool = healthSystem.GetStatPoolValue(npcID, gamedataStatPoolType.Health);
        return healthPool.current;
    }

    private func GetCombatState() -> ENPCCombatState {
        let psmSystem = GameInstance.GetPlayerStateMachineSystem(this.m_npc.GetGame());
        if psmSystem.IsInCombat(this.m_npc.GetEntityID()) {
            return ENPCCombatState.InCombat;
        }
        return ENPCCombatState.Passive;
    }

    private func QuaternionEquals(q1: Quaternion, q2: Quaternion, tolerance: Float) -> Bool {
        return AbsF(q1.i - q2.i) < tolerance &&
               AbsF(q1.j - q2.j) < tolerance &&
               AbsF(q1.k - q2.k) < tolerance &&
               AbsF(q1.r - q2.r) < tolerance;
    }

    // Getters
    public func GetNPCId() -> Uint64 { return this.m_npcId; }
    public func GetNPC() -> ref<NPCPuppet> { return this.m_npc; }
    public func GetPosition() -> Vector3 { return this.m_lastPosition; }
    public func GetRotation() -> Quaternion { return this.m_lastRotation; }
    public func GetAnimationState() -> CName { return this.m_lastAnimState; }
    public func GetBehaviorState() -> gamedataNPCBehaviorState { return this.m_lastBehaviorState; }
    public func GetHealth() -> Float { return this.m_lastHealth; }
    public func GetCombatState() -> ENPCCombatState { return this.m_lastCombatState; }
    public func GetDialogueState() -> ENPCDialogueState { return ENPCDialogueState.Available; } // Placeholder
    public func IsAlive() -> Bool { return this.m_lastHealth > 0.0; }

    public func HasStateChanged() -> Bool { return this.m_hasStateChanged; }
    public func MarkAsSynced() -> Void { this.m_hasStateChanged = false; }
}

// Quest NPC Tracker - special handling for quest-important NPCs
public class QuestNPCTracker extends ScriptableComponent {
    private let m_npc: wref<NPCPuppet>;
    private let m_npcId: Uint64;
    private let m_questIds: array<String>;
    private let m_dialogueState: ENPCDialogueState;
    private let m_questPhase: String;
    private let m_hasCriticalChange: Bool;

    public func Initialize(npc: ref<NPCPuppet>) -> Void {
        this.m_npc = npc;
        this.m_npcId = Cast<Uint64>(npc.GetEntityID());
        this.ScanForQuestInvolvement();
    }

    public func Update(deltaTime: Float) -> Void {
        if !IsDefined(this.m_npc) {
            return;
        }

        this.CheckForQuestStateChanges();
        this.CheckForDialogueStateChanges();
    }

    private func ScanForQuestInvolvement() -> Void {
        let questSystem = GameInstance.GetQuestSystem(this.m_npc.GetGame());
        this.m_questIds = questSystem.GetNPCQuestInvolvement(this.m_npc.GetEntityID());
    }

    private func CheckForQuestStateChanges() -> Void {
        // Monitor quest phase changes
        let questSystem = GameInstance.GetQuestSystem(this.m_npc.GetGame());

        for questId in this.m_questIds {
            let currentPhase = questSystem.GetQuestPhase(StringToName(questId));
            if !Equals(currentPhase, StringToName(this.m_questPhase)) {
                this.m_questPhase = NameToString(currentPhase);
                this.m_hasCriticalChange = true;
            }
        }
    }

    private func CheckForDialogueStateChanges() -> Void {
        // Monitor dialogue availability changes
        let previousState = this.m_dialogueState;
        this.m_dialogueState = this.GetCurrentDialogueState();

        if !Equals(previousState, this.m_dialogueState) {
            this.m_hasCriticalChange = true;
        }
    }

    private func GetCurrentDialogueState() -> ENPCDialogueState {
        // Check if NPC has available dialogue
        let dialogueComponent = this.m_npc.GetComponent(n"DialogueComponent");
        if IsDefined(dialogueComponent) {
            if dialogueComponent.HasAvailableChoices() {
                return ENPCDialogueState.Available;
            } else if dialogueComponent.IsInDialogue() {
                return ENPCDialogueState.InProgress;
            }
        }
        return ENPCDialogueState.None;
    }

    public func GetQuestSyncData() -> QuestNPCSyncData {
        let syncData: QuestNPCSyncData;
        syncData.npcId = this.m_npcId;
        syncData.questIds = this.m_questIds;
        syncData.questPhase = this.m_questPhase;
        syncData.dialogueState = this.m_dialogueState;
        syncData.timestamp = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

        return syncData;
    }

    public func GetNPCId() -> Uint64 { return this.m_npcId; }
    public func HasCriticalStateChange() -> Bool { return this.m_hasCriticalChange; }
    public func MarkQuestSynced() -> Void { this.m_hasCriticalChange = false; }
}

// Police System Synchronization
public class PoliceSystemSync extends ScriptableComponent {
    private let m_policeNPCs: array<Uint64>;
    private let m_wantedLevel: Int32;
    private let m_lastWantedLevel: Int32;
    private let m_automaticDetection: Bool = false;

    public func Initialize() -> Void {
        LogChannel(n"NPCSync", s"[NPCSync] Police System Sync initialized");
    }

    public func EnableAutomaticDetection() -> Void {
        this.m_automaticDetection = true;
        this.MonitorWantedLevel();
    }

    public func Update(deltaTime: Float) -> Void {
        if this.m_automaticDetection {
            this.CheckWantedLevelChanges();
            this.UpdatePoliceResponse();
        }
    }

    private func MonitorWantedLevel() -> Void {
        // Monitor player wanted level from singleplayer system
        let wantedSystem = GameInstance.GetWantedSystem(GetGameInstance());
        this.m_wantedLevel = wantedSystem.GetWantedLevel();
    }

    private func CheckWantedLevelChanges() -> Void {
        this.MonitorWantedLevel();

        if this.m_wantedLevel != this.m_lastWantedLevel {
            // Sync wanted level change
            let wantedData: WantedLevelData;
            wantedData.newWantedLevel = this.m_wantedLevel;
            wantedData.timestamp = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

            Net_SendWantedLevelUpdate(wantedData);
            this.m_lastWantedLevel = this.m_wantedLevel;

            LogChannel(n"NPCSync", s"[NPCSync] Wanted level changed to: " + this.m_wantedLevel);
        }
    }

    private func UpdatePoliceResponse() -> Void {
        // Let the singleplayer police system handle response
        // Just ensure all spawned police are synchronized
    }

    public func RegisterNPC(npc: ref<NPCPuppet>) -> Void {
        let npcId = Cast<Uint64>(npc.GetEntityID());
        ArrayPush(this.m_policeNPCs, npcId);
    }

    public func UnregisterNPC(npcId: Uint64) -> Void {
        ArrayRemove(this.m_policeNPCs, npcId);
    }
}

// Enemy Group Synchronization
public class EnemyGroupSync extends ScriptableComponent {
    private let m_enemyGroups: array<ref<EnemyGroup>>;
    private let m_combatZones: array<ref<CombatZone>>;

    public func Initialize() -> Void {
        LogChannel(n"NPCSync", s"[NPCSync] Enemy Group Sync initialized");
    }

    public func Update(deltaTime: Float) -> Void {
        this.UpdateEnemyGroups();
        this.UpdateCombatZones();
    }

    private func UpdateEnemyGroups() -> Void {
        for group in this.m_enemyGroups {
            group.Update();
            if group.HasStateChanged() {
                let groupData = group.GetSyncData();
                Net_SendEnemyGroupUpdate(groupData);
                group.MarkSynced();
            }
        }
    }

    private func UpdateCombatZones() -> Void {
        for zone in this.m_combatZones {
            zone.Update();
        }
    }

    public func RegisterNPC(npc: ref<NPCPuppet>) -> Void {
        // Add to appropriate enemy group
        let group = this.FindOrCreateEnemyGroup(npc);
        group.AddMember(npc);
    }

    public func UnregisterNPC(npcId: Uint64) -> Void {
        for group in this.m_enemyGroups {
            group.RemoveMember(npcId);
        }
    }

    private func FindOrCreateEnemyGroup(npc: ref<NPCPuppet>) -> ref<EnemyGroup> {
        // Find existing group or create new one based on faction/location
        return new EnemyGroup(); // Placeholder
    }
}

// Civilian Traffic Synchronization
public class CivilianTrafficSync extends ScriptableComponent {
    private let m_civilianNPCs: array<Uint64>;
    private let m_automaticDetection: Bool = false;

    public func Initialize() -> Void {
        LogChannel(n"NPCSync", s"[NPCSync] Civilian Traffic Sync initialized");
    }

    public func EnableAutomaticDetection() -> Void {
        this.m_automaticDetection = true;
    }

    public func Update(deltaTime: Float) -> Void {
        if this.m_automaticDetection {
            this.DetectNewCivilians();
        }
    }

    private func DetectNewCivilians() -> Void {
        // Automatically detect civilian NPCs spawned by crowd system
        let crowdSystem = GameInstance.GetCrowdSystem(GetGameInstance());
        // This would interface with the crowd system
    }

    public func RegisterNPC(npc: ref<NPCPuppet>) -> Void {
        let npcId = Cast<Uint64>(npc.GetEntityID());
        ArrayPush(this.m_civilianNPCs, npcId);
    }

    public func UnregisterNPC(npcId: Uint64) -> Void {
        ArrayRemove(this.m_civilianNPCs, npcId);
    }
}

// Vendor System Synchronization
public class VendorSystemSync extends ScriptableComponent {
    private let m_vendors: array<Uint64>;
    private let m_automaticDetection: Bool = false;

    public func Initialize() -> Void {
        LogChannel(n"NPCSync", s"[NPCSync] Vendor System Sync initialized");
    }

    public func EnableAutomaticDetection() -> Void {
        this.m_automaticDetection = true;
    }

    public func Update(deltaTime: Float) -> Void {
        // Vendors are mostly stationary, minimal updates needed
    }

    public func RegisterNPC(npc: ref<NPCPuppet>) -> Void {
        let npcId = Cast<Uint64>(npc.GetEntityID());
        ArrayPush(this.m_vendors, npcId);

        // Ensure vendor functionality works in multiplayer
        this.ConfigureVendorForMultiplayer(npc);
    }

    public func UnregisterNPC(npcId: Uint64) -> Void {
        ArrayRemove(this.m_vendors, npcId);
    }

    private func ConfigureVendorForMultiplayer(npc: ref<NPCPuppet>) -> Void {
        // Ensure vendor maintains singleplayer functionality
        let vendorComponent = npc.GetComponent(n"VendorComponent");
        if IsDefined(vendorComponent) {
            // Vendor should work the same as singleplayer for all players
        }
    }
}

// Support classes
public class EnemyGroup extends ScriptableComponent {
    private let m_members: array<Uint64>;
    private let m_groupId: Uint64;
    private let m_hasStateChanged: Bool = false;

    public func AddMember(npc: ref<NPCPuppet>) -> Void {
        let npcId = Cast<Uint64>(npc.GetEntityID());
        ArrayPush(this.m_members, npcId);
        this.m_hasStateChanged = true;
    }

    public func RemoveMember(npcId: Uint64) -> Void {
        ArrayRemove(this.m_members, npcId);
        this.m_hasStateChanged = true;
    }

    public func Update() -> Void {
        // Update group coordination
    }

    public func HasStateChanged() -> Bool { return this.m_hasStateChanged; }
    public func MarkSynced() -> Void { this.m_hasStateChanged = false; }
    public func GetSyncData() -> EnemyGroupData {
        let data: EnemyGroupData;
        data.groupId = this.m_groupId;
        data.memberIds = this.m_members;
        return data;
    }
}

public class CombatZone extends ScriptableComponent {
    private let m_zoneId: Uint64;
    private let m_bounds: Vector4;
    private let m_isActive: Bool;

    public func Update() -> Void {
        // Update combat zone state
    }
}

// Data Structures
public struct NPCSyncData {
    public let npcId: Uint64;
    public let position: Vector3;
    public let rotation: Quaternion;
    public let animationState: CName;
    public let behaviorState: gamedataNPCBehaviorState;
    public let health: Float;
    public let combatState: ENPCCombatState;
    public let dialogueState: ENPCDialogueState;
    public let isAlive: Bool;
    public let timestamp: Float;
}

public struct NPCSpawnData {
    public let npcId: Uint64;
    public let npcRecord: TweakDBID;
    public let position: Vector3;
    public let rotation: Quaternion;
    public let npcType: ENPCType;
    public let timestamp: Float;
}

public struct NPCDespawnData {
    public let npcId: Uint64;
    public let timestamp: Float;
}

public struct QuestNPCSyncData {
    public let npcId: Uint64;
    public let questIds: array<String>;
    public let questPhase: String;
    public let dialogueState: ENPCDialogueState;
    public let timestamp: Float;
}

public struct WantedLevelData {
    public let newWantedLevel: Int32;
    public let timestamp: Float;
}

public struct EnemyGroupData {
    public let groupId: Uint64;
    public let memberIds: array<Uint64>;
}

// Enumerations
public enum ENPCType : Uint8 {
    Civilian = 0,
    Police = 1,
    Enemy = 2,
    Vendor = 3,
    QuestNPC = 4,
    SecurityGuard = 5,
    Gangster = 6,
    Corporate = 7
}

public enum ENPCCombatState : Uint8 {
    Passive = 0,
    Alert = 1,
    InCombat = 2,
    Searching = 3,
    Fleeing = 4,
    Dead = 5
}

public enum ENPCDialogueState : Uint8 {
    None = 0,
    Available = 1,
    InProgress = 2,
    Completed = 3,
    Unavailable = 4
}

// Native function declarations
native func Net_SendNPCUpdate(syncData: NPCSyncData) -> Void;
native func Net_SendNPCSpawn(spawnData: NPCSpawnData) -> Void;
native func Net_SendNPCDespawn(despawnData: NPCDespawnData) -> Void;
native func Net_SendQuestNPCUpdate(questData: QuestNPCSyncData) -> Void;
native func Net_SendWantedLevelUpdate(wantedData: WantedLevelData) -> Void;
native func Net_SendEnemyGroupUpdate(groupData: EnemyGroupData) -> Void;

// Integration with game systems - automatic NPC detection
@wrapMethod(NPCSystem)
public func SpawnNPC(npcRecord: TweakDBID, transform: WorldTransform) -> ref<NPCPuppet> {
    let npc = wrappedMethod(npcRecord, transform);

    if IsDefined(npc) {
        // Automatically integrate with multiplayer sync
        let npcSyncManager = NPCSyncManager.GetInstance();
        if IsDefined(npcSyncManager) {
            npcSyncManager.OnNPCSpawned(npc);
        }
    }

    return npc;
}

@wrapMethod(NPCSystem)
public func DespawnNPC(npc: ref<NPCPuppet>) -> Void {
    let npcId = Cast<Uint64>(npc.GetEntityID());

    // Notify multiplayer sync before despawning
    let npcSyncManager = NPCSyncManager.GetInstance();
    if IsDefined(npcSyncManager) {
        npcSyncManager.OnNPCDespawned(npcId);
    }

    wrappedMethod(npc);
}

// Automatic crowd system integration
@wrapMethod(CrowdSystem)
public func SpawnCrowdNPC(spawnData: ref<CrowdSpawnData>) -> ref<NPCPuppet> {
    let npc = wrappedMethod(spawnData);

    if IsDefined(npc) {
        // Automatically sync crowd NPCs
        let npcSyncManager = NPCSyncManager.GetInstance();
        if IsDefined(npcSyncManager) {
            npcSyncManager.OnNPCSpawned(npc);
        }
    }

    return npc;
}

// Network event callbacks
@addMethod(PlayerPuppet)
public func OnNetworkNPCUpdate(syncData: NPCSyncData) -> Void {
    NPCSyncManager.GetInstance().OnRemoteNPCUpdate(syncData);
}

@addMethod(PlayerPuppet)
public func OnNetworkNPCSpawn(spawnData: NPCSpawnData) -> Void {
    NPCSyncManager.GetInstance().OnRemoteNPCSpawn(spawnData);
}

@addMethod(PlayerPuppet)
public func OnNetworkNPCDespawn(despawnData: NPCDespawnData) -> Void {
    NPCSyncManager.GetInstance().OnRemoteNPCDespawn(despawnData);
}