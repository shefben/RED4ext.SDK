// Cooperative Story Mission System
// REDscript interface for synchronized story mission gameplay - automatically integrates with singleplayer quest system

// Cooperative Mission Manager - handles multiplayer story missions
public class CooperativeMissionManager extends ScriptableSystem {
    private static let s_instance: ref<CooperativeMissionManager>;
    private let m_isActive: Bool = false;
    private let m_isHost: Bool = false;
    private let m_currentMission: ref<CooperativeMissionInstance>;
    private let m_participants: array<ref<CoopMissionParticipant>>;
    private let m_missionState: CooperativeMissionState;
    private let m_questJournal: ref<QuestJournalManager>;
    private let m_syncTimer: Float = 0.0;
    private let m_syncInterval: Float = 0.5; // 2 FPS for mission state updates
    private let m_missionProgress: QuestProgressTracker;
    private let m_objectiveStates: array<ObjectiveState>;
    private let m_dialogueState: DialogueState;
    private let m_localPlayer: wref<PlayerPuppet>;

    // Mission synchronization settings
    private let m_autoSync: Bool = true;
    private let m_syncChoices: Bool = true;
    private let m_syncObjectives: Bool = true;
    private let m_syncDialogue: Bool = true;
    private let m_allowIndependentExploration: Bool = true;
    private let m_maxDistanceFromMission: Float = 500.0;

    public static func GetInstance() -> ref<CooperativeMissionManager> {
        if !IsDefined(CooperativeMissionManager.s_instance) {
            CooperativeMissionManager.s_instance = new CooperativeMissionManager();
        }
        return CooperativeMissionManager.s_instance;
    }

    public func Initialize(player: ref<PlayerPuppet>) -> Void {
        this.m_localPlayer = player;
        this.m_questJournal = GameInstance.GetQuestsSystem(GetGameInstance()).GetQuestJournalManager();

        // Initialize mission state
        this.InitializeMissionState();

        // Register for quest system callbacks
        Native_RegisterMissionCallbacks();

        LogChannel(n"CoopMission", s"[CoopMission] Cooperative Mission Manager initialized");
    }

    private func InitializeMissionState() -> Void {
        this.m_missionState.currentQuest = "";
        this.m_missionState.currentPhase = "";
        this.m_missionState.isInMission = false;
        this.m_missionState.missionStartTime = 0.0;
        this.m_missionState.syncVersion = 0u;
        this.m_missionState.hostPlayerId = 0u;

        ArrayClear(this.m_participants);
        ArrayClear(this.m_objectiveStates);

        this.m_dialogueState.currentSpeaker = "";
        this.m_dialogueState.currentLine = "";
        this.m_dialogueState.isInDialogue = false;
        this.m_dialogueState.availableChoices = "";
        this.m_dialogueState.timeoutTime = 0.0;

        this.m_missionProgress.completedObjectives = 0u;
        this.m_missionProgress.totalObjectives = 0u;
        this.m_missionProgress.progressPercentage = 0.0;
    }

    public func OnUpdate(deltaTime: Float) -> Void {
        if !this.m_isActive {
            return;
        }

        this.m_syncTimer += deltaTime;

        if this.m_syncTimer >= this.m_syncInterval {
            this.UpdateMissionState();
            this.CheckMissionProximity();

            if this.m_isHost {
                this.SyncMissionStateToParticipants();
            }

            this.m_syncTimer = 0.0;
        }

        // Update participants
        this.UpdateParticipants(deltaTime);
    }

    // Mission Management
    public func StartCooperativeMission(questId: String, participants: array<Uint32>) -> Bool {
        if this.m_isActive {
            return false;
        }

        // Validate quest exists
        if !this.IsValidQuest(questId) {
            this.ShowMissionError("Invalid quest: " + questId);
            return false;
        }

        // Check if all participants are ready
        if !this.ValidateParticipants(participants)) {
            this.ShowMissionError("Not all participants are ready");
            return false;
        }

        this.m_isActive = true;
        this.m_isHost = true;
        this.m_missionState.currentQuest = questId;
        this.m_missionState.isInMission = true;
        this.m_missionState.missionStartTime = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));
        this.m_missionState.hostPlayerId = this.GetLocalPlayerId();
        this.m_missionState.syncVersion = 1u;

        // Initialize participants
        this.InitializeParticipants(participants);

        // Create mission instance
        this.m_currentMission = new CooperativeMissionInstance();
        this.m_currentMission.Initialize(questId, participants);

        // Sync all participants to mission start state
        this.SyncAllToMissionStart(questId);

        // Notify mission start
        this.OnMissionStarted(questId);

        LogChannel(n"CoopMission", s"[CoopMission] Started cooperative mission: " + questId);
        return true;
    }

    public func JoinCooperativeMission(hostPlayerId: Uint32, missionData: CooperativeMissionState) -> Bool {
        if this.m_isActive {
            return false;
        }

        this.m_isActive = true;
        this.m_isHost = false;
        this.m_missionState = missionData;

        // Create mission instance
        this.m_currentMission = new CooperativeMissionInstance();
        this.m_currentMission.InitializeFromState(missionData);

        // Sync to current mission state
        this.SyncToMissionState(missionData);

        LogChannel(n"CoopMission", s"[CoopMission] Joined cooperative mission: " + missionData.currentQuest);
        return true;
    }

    public func LeaveMission() -> Bool {
        if !this.m_isActive {
            return false;
        }

        // If host, transfer host or end mission
        if this.m_isHost && ArraySize(this.m_participants) > 1 {
            this.TransferHost();
        } else if this.m_isHost {
            this.EndMission();
        }

        // Notify departure
        this.NotifyMissionDeparture();

        // Reset state
        this.m_isActive = false;
        this.m_isHost = false;
        this.m_currentMission = null;

        // Return to singleplayer quest state
        this.ReturnToSingleplayerMode();

        LogChannel(n"CoopMission", s"[CoopMission] Left cooperative mission");
        return true;
    }

    public func EndMission() -> Bool {
        if !this.m_isHost {
            return false;
        }

        // Notify all participants
        for participant in this.m_participants {
            this.NotifyMissionEnd(participant.GetPlayerId());
        }

        // Reset all states
        this.InitializeMissionState();
        this.m_isActive = false;
        this.m_isHost = false;
        this.m_currentMission = null;

        LogChannel(n"CoopMission", s"[CoopMission] Ended cooperative mission");
        return true;
    }

    // Quest System Integration - automatically hooks into singleplayer quest system
    @wrapMethod(QuestJournalManager)
    public func StartQuest(questId: String) -> Bool {
        let result = wrappedMethod(questId);

        // If in cooperative mode and host, sync quest start
        if this.m_isActive && this.m_isHost {
            this.SyncQuestStart(questId);
        }

        return result;
    }

    @wrapMethod(QuestJournalManager)
    public func SetObjectiveState(questId: String, objectiveId: String, state: gameJournalEntryState) -> Void {
        wrappedMethod(questId, objectiveId, state);

        // If in cooperative mode and auto-sync enabled
        if this.m_isActive && this.m_syncObjectives {
            this.SyncObjectiveState(questId, objectiveId, state);
        }
    }

    @wrapMethod(QuestJournalManager)
    public func CompleteQuest(questId: String) -> Bool {
        let result = wrappedMethod(questId);

        // If in cooperative mode and host, sync quest completion
        if this.m_isActive && this.m_isHost {
            this.SyncQuestCompletion(questId);
        }

        return result;
    }

    // Dialogue System Integration
    @wrapMethod(DialogueSystem)
    public func StartDialogue(speakerId: String, dialogueId: String) -> Void {
        wrappedMethod(speakerId, dialogueId);

        // If in cooperative mode, sync dialogue state
        if this.m_isActive && this.m_syncDialogue {
            this.SyncDialogueStart(speakerId, dialogueId);
        }
    }

    @wrapMethod(DialogueSystem)
    public func MakeDialogueChoice(choiceIndex: Int32) -> Void {
        // If in cooperative mode, check if choice sync is enabled
        if this.m_isActive && this.m_syncChoices {
            if this.m_isHost {
                // Host makes the choice
                wrappedMethod(choiceIndex);
                this.SyncDialogueChoice(choiceIndex);
            } else {
                // Non-host waits for host's choice
                this.RequestDialogueChoice(choiceIndex);
                return;
            }
        } else {
            wrappedMethod(choiceIndex);
        }
    }

    // Checkpoint and Save Synchronization
    @wrapMethod(SaveSystem)
    public func CreateCheckpoint(checkpointName: String) -> Bool {
        let result = wrappedMethod(checkpointName);

        // If in cooperative mode and host, create synchronized checkpoint
        if this.m_isActive && this.m_isHost && result {
            this.CreateCooperativeCheckpoint(checkpointName);
        }

        return result;
    }

    // Mission State Synchronization
    private func UpdateMissionState() -> Void {
        if !IsDefined(this.m_currentMission) {
            return;
        }

        // Update current quest state
        let currentQuest = this.GetCurrentQuest();
        if !Equals(this.m_missionState.currentQuest, currentQuest) {
            this.m_missionState.currentQuest = currentQuest;
            this.m_missionState.syncVersion += 1u;
        }

        // Update current phase
        let currentPhase = this.GetCurrentQuestPhase();
        if !Equals(this.m_missionState.currentPhase, currentPhase) {
            this.m_missionState.currentPhase = currentPhase;
            this.m_missionState.syncVersion += 1u;
        }

        // Update objective states
        this.UpdateObjectiveStates();

        // Update dialogue state
        this.UpdateDialogueState();

        // Update progress
        this.UpdateMissionProgress();
    }

    private func UpdateObjectiveStates() -> Void {
        let questSystem = GameInstance.GetQuestsSystem(GetGameInstance());

        if !Equals(this.m_missionState.currentQuest, "") {
            let objectives = Native_GetQuestObjectives(this.m_missionState.currentQuest);

            for objective in objectives {
                let currentState = questSystem.GetObjectiveState(this.m_missionState.currentQuest, objective.id);

                // Find or create objective state
                let existingIndex = this.FindObjectiveStateIndex(objective.id);
                if existingIndex >= 0 {
                    if !Equals(this.m_objectiveStates[existingIndex].state, currentState) {
                        this.m_objectiveStates[existingIndex].state = currentState;
                        this.m_missionState.syncVersion += 1u;
                    }
                } else {
                    let newState: ObjectiveState;
                    newState.objectiveId = objective.id;
                    newState.state = currentState;
                    newState.isOptional = objective.isOptional;
                    ArrayPush(this.m_objectiveStates, newState);
                    this.m_missionState.syncVersion += 1u;
                }
            }
        }
    }

    private func UpdateDialogueState() -> Void {
        let dialogueSystem = GameInstance.GetDialogueSystem(GetGameInstance());

        if dialogueSystem.IsInDialogue() {
            this.m_dialogueState.isInDialogue = true;
            this.m_dialogueState.currentSpeaker = dialogueSystem.GetCurrentSpeaker();
            this.m_dialogueState.currentLine = dialogueSystem.GetCurrentLine();
            this.m_dialogueState.availableChoices = dialogueSystem.GetAvailableChoices();
        } else {
            if this.m_dialogueState.isInDialogue {
                this.m_dialogueState.isInDialogue = false;
                this.m_missionState.syncVersion += 1u;
            }
        }
    }

    private func UpdateMissionProgress() -> Void {
        if !Equals(this.m_missionState.currentQuest, "") {
            let totalObjectives = Cast<Uint32>(ArraySize(this.m_objectiveStates));
            let completedObjectives = 0u;

            for objective in this.m_objectiveStates {
                if Equals(objective.state, gameJournalEntryState.Succeeded) {
                    completedObjectives += 1u;
                }
            }

            this.m_missionProgress.completedObjectives = completedObjectives;
            this.m_missionProgress.totalObjectives = totalObjectives;

            if totalObjectives > 0u {
                this.m_missionProgress.progressPercentage = Cast<Float>(completedObjectives) / Cast<Float>(totalObjectives);
            }
        }
    }

    private func CheckMissionProximity() -> Void {
        if !this.m_allowIndependentExploration {
            return;
        }

        let missionLocation = this.GetCurrentMissionLocation();

        for participant in this.m_participants {
            let playerPosition = participant.GetPosition();
            let distance = Vector4.Distance(playerPosition, missionLocation);

            if distance > this.m_maxDistanceFromMission {
                this.WarnPlayerTooFar(participant.GetPlayerId(), distance);
            }
        }
    }

    // Network Synchronization
    private func SyncMissionStateToParticipants() -> Void {
        if !this.m_isHost {
            return;
        }

        for participant in this.m_participants {
            if participant.GetPlayerId() != this.GetLocalPlayerId() {
                Native_SyncMissionState(participant.GetPlayerId(), this.m_missionState, this.m_objectiveStates, this.m_dialogueState);
            }
        }
    }

    private func SyncQuestStart(questId: String) -> Void {
        for participant in this.m_participants {
            if participant.GetPlayerId() != this.GetLocalPlayerId() {
                Native_SyncQuestStart(participant.GetPlayerId(), questId);
            }
        }
    }

    private func SyncObjectiveState(questId: String, objectiveId: String, state: gameJournalEntryState) -> Void {
        for participant in this.m_participants {
            if participant.GetPlayerId() != this.GetLocalPlayerId() {
                Native_SyncObjectiveState(participant.GetPlayerId(), questId, objectiveId, state);
            }
        }
    }

    private func SyncDialogueStart(speakerId: String, dialogueId: String) -> Void {
        for participant in this.m_participants {
            if participant.GetPlayerId() != this.GetLocalPlayerId() {
                Native_SyncDialogueStart(participant.GetPlayerId(), speakerId, dialogueId);
            }
        }
    }

    private func SyncDialogueChoice(choiceIndex: Int32) -> Void {
        for participant in this.m_participants {
            if participant.GetPlayerId() != this.GetLocalPlayerId() {
                Native_SyncDialogueChoice(participant.GetPlayerId(), choiceIndex);
            }
        }
    }

    // Mission Management
    private func InitializeParticipants(participantIds: array<Uint32>) -> Void {
        ArrayClear(this.m_participants);

        for playerId in participantIds {
            let participant = new CoopMissionParticipant();
            participant.Initialize(playerId);
            ArrayPush(this.m_participants, participant);
        }
    }

    private func ValidateParticipants(participantIds: array<Uint32>) -> Bool {
        // Check if all participants are online and not in other missions
        for playerId in participantIds {
            if !Native_IsPlayerOnline(playerId) {
                return false;
            }

            if Native_IsPlayerInMission(playerId) {
                return false;
            }
        }

        return true;
    }

    private func TransferHost() -> Void {
        // Find suitable new host
        for participant in this.m_participants {
            if participant.GetPlayerId() != this.GetLocalPlayerId() && participant.IsConnected() {
                Native_TransferMissionHost(participant.GetPlayerId(), this.m_missionState);
                break;
            }
        }
    }

    // Event Handlers
    public func OnMissionStateReceived(newState: CooperativeMissionState, objectives: array<ObjectiveState>, dialogue: DialogueState) -> Void {
        if this.m_isHost {
            return; // Hosts don't receive state updates
        }

        // Apply received state
        this.m_missionState = newState;
        this.m_objectiveStates = objectives;
        this.m_dialogueState = dialogue;

        // Sync local game state
        this.ApplyMissionState();
    }

    public func OnQuestStartReceived(questId: String) -> Void {
        if this.m_isHost {
            return;
        }

        // Start the quest locally
        this.m_questJournal.StartQuest(questId);
    }

    public func OnObjectiveStateReceived(questId: String, objectiveId: String, state: gameJournalEntryState) -> Void {
        if this.m_isHost {
            return;
        }

        // Update objective state locally
        this.m_questJournal.SetObjectiveState(questId, objectiveId, state);
    }

    public func OnDialogueChoiceReceived(choiceIndex: Int32) -> Void {
        if this.m_isHost {
            return;
        }

        // Apply the host's dialogue choice
        let dialogueSystem = GameInstance.GetDialogueSystem(GetGameInstance());
        dialogueSystem.MakeDialogueChoice(choiceIndex);
    }

    public func OnPlayerJoinedMission(playerId: Uint32, playerName: String) -> Void {
        // Add participant
        let participant = new CoopMissionParticipant();
        participant.Initialize(playerId);
        ArrayPush(this.m_participants, participant);

        // Sync current state to new player
        if this.m_isHost {
            Native_SyncMissionState(playerId, this.m_missionState, this.m_objectiveStates, this.m_dialogueState);
        }

        this.ShowParticipantJoinedNotification(playerName);
        LogChannel(n"CoopMission", s"[CoopMission] Player joined mission: " + playerName);
    }

    public func OnPlayerLeftMission(playerId: Uint32, playerName: String) -> Void {
        // Remove participant
        let index = this.FindParticipantIndex(playerId);
        if index >= 0 {
            ArrayRemove(this.m_participants, this.m_participants[index]);
        }

        this.ShowParticipantLeftNotification(playerName);
        LogChannel(n"CoopMission", s"[CoopMission] Player left mission: " + playerName);
    }

    // Helper Methods
    private func IsValidQuest(questId: String) -> Bool {
        return Native_IsValidQuest(questId);
    }

    private func GetCurrentQuest() -> String {
        return Native_GetCurrentQuest();
    }

    private func GetCurrentQuestPhase() -> String {
        return Native_GetCurrentQuestPhase();
    }

    private func GetCurrentMissionLocation() -> Vector4 {
        return Native_GetMissionLocation(this.m_missionState.currentQuest);
    }

    private func FindObjectiveStateIndex(objectiveId: String) -> Int32 {
        for i in Range(ArraySize(this.m_objectiveStates)) {
            if Equals(this.m_objectiveStates[i].objectiveId, objectiveId) {
                return i;
            }
        }
        return -1;
    }

    private func FindParticipantIndex(playerId: Uint32) -> Int32 {
        for i in Range(ArraySize(this.m_participants)) {
            if this.m_participants[i].GetPlayerId() == playerId {
                return i;
            }
        }
        return -1;
    }

    private func GetLocalPlayerId() -> Uint32 {
        return Native_GetLocalPlayerId();
    }

    // UI and Notification Methods
    private func ShowMissionError(message: String) -> Void {
        // Show error dialog
    }

    private func ShowParticipantJoinedNotification(playerName: String) -> Void {
        // Show notification that player joined mission
    }

    private func ShowParticipantLeftNotification(playerName: String) -> Void {
        // Show notification that player left mission
    }

    private func WarnPlayerTooFar(playerId: Uint32, distance: Float) -> Void {
        // Show warning about being too far from mission
    }

    // Mission State Application
    private func ApplyMissionState() -> Void {
        // Apply quest state
        if !Equals(this.m_missionState.currentQuest, "") {
            Native_SetCurrentQuest(this.m_missionState.currentQuest);
            Native_SetCurrentQuestPhase(this.m_missionState.currentPhase);
        }

        // Apply objective states
        for objective in this.m_objectiveStates {
            this.m_questJournal.SetObjectiveState(this.m_missionState.currentQuest, objective.objectiveId, objective.state);
        }

        // Apply dialogue state
        if this.m_dialogueState.isInDialogue {
            Native_SyncDialogueState(this.m_dialogueState);
        }
    }

    private func SyncAllToMissionStart(questId: String) -> Void {
        for participant in this.m_participants {
            Native_SyncToMissionStart(participant.GetPlayerId(), questId);
        }
    }

    private func SyncToMissionState(missionState: CooperativeMissionState) -> Void {
        Native_SyncToMissionState(missionState);
    }

    private func ReturnToSingleplayerMode() -> Void {
        // Clean up cooperative mission state and return to normal singleplayer quest system
        Native_ReturnToSingleplayerMode();
    }

    private func CreateCooperativeCheckpoint(checkpointName: String) -> Void {
        // Create checkpoint for all participants
        for participant in this.m_participants {
            Native_CreateCooperativeCheckpoint(participant.GetPlayerId(), checkpointName);
        }
    }

    private func RequestDialogueChoice(choiceIndex: Int32) -> Void {
        // Send choice request to host
        Native_RequestDialogueChoice(this.m_missionState.hostPlayerId, choiceIndex);
    }

    private func NotifyMissionDeparture() -> Void {
        Native_NotifyMissionDeparture(this.GetLocalPlayerId());
    }

    private func NotifyMissionEnd(playerId: Uint32) -> Void {
        Native_NotifyMissionEnd(playerId);
    }

    private func OnMissionStarted(questId: String) -> Void {
        // Handle mission start event
    }

    private func UpdateParticipants(deltaTime: Float) -> Void {
        for participant in this.m_participants {
            participant.Update(deltaTime);
        }
    }

    // Public Getters
    public func IsInCooperativeMission() -> Bool {
        return this.m_isActive;
    }

    public func IsHost() -> Bool {
        return this.m_isHost;
    }

    public func GetCurrentMission() -> ref<CooperativeMissionInstance> {
        return this.m_currentMission;
    }

    public func GetMissionState() -> CooperativeMissionState {
        return this.m_missionState;
    }

    public func GetParticipants() -> array<ref<CoopMissionParticipant>> {
        return this.m_participants;
    }

    public func GetMissionProgress() -> QuestProgressTracker {
        return this.m_missionProgress;
    }

    // Configuration Methods
    public func SetAutoSync(enabled: Bool) -> Void {
        this.m_autoSync = enabled;
    }

    public func SetSyncChoices(enabled: Bool) -> Void {
        this.m_syncChoices = enabled;
    }

    public func SetSyncObjectives(enabled: Bool) -> Void {
        this.m_syncObjectives = enabled;
    }

    public func SetSyncDialogue(enabled: Bool) -> Void {
        this.m_syncDialogue = enabled;
    }

    public func SetAllowIndependentExploration(allowed: Bool) -> Void {
        this.m_allowIndependentExploration = allowed;
    }

    public func SetMaxMissionDistance(distance: Float) -> Void {
        this.m_maxDistanceFromMission = distance;
    }
}

// Cooperative Mission Participant
public class CoopMissionParticipant extends ScriptableComponent {
    private let m_playerId: Uint32;
    private let m_playerName: String;
    private let m_isConnected: Bool;
    private let m_isReady: Bool;
    private let m_position: Vector4;
    private let m_lastActivity: Float;
    private let m_questProgress: QuestProgressTracker;

    public func Initialize(playerId: Uint32) -> Void {
        this.m_playerId = playerId;
        this.m_playerName = Native_GetPlayerName(playerId);
        this.m_isConnected = true;
        this.m_isReady = false;
        this.m_lastActivity = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));
    }

    public func Update(deltaTime: Float) -> Void {
        // Update participant state
        this.UpdatePosition();
        this.UpdateConnectionStatus();
    }

    private func UpdatePosition() -> Void {
        this.m_position = Native_GetPlayerPosition(this.m_playerId);
    }

    private func UpdateConnectionStatus() -> Void {
        this.m_isConnected = Native_IsPlayerConnected(this.m_playerId);
    }

    // Getters
    public func GetPlayerId() -> Uint32 { return this.m_playerId; }
    public func GetPlayerName() -> String { return this.m_playerName; }
    public func IsConnected() -> Bool { return this.m_isConnected; }
    public func IsReady() -> Bool { return this.m_isReady; }
    public func GetPosition() -> Vector4 { return this.m_position; }
    public func GetLastActivity() -> Float { return this.m_lastActivity; }

    public func SetReady(ready: Bool) -> Void { this.m_isReady = ready; }
}

// Cooperative Mission Instance
public class CooperativeMissionInstance extends ScriptableComponent {
    private let m_questId: String;
    private let m_participants: array<Uint32>;
    private let m_startTime: Float;
    private let m_isCompleted: Bool;

    public func Initialize(questId: String, participants: array<Uint32>) -> Void {
        this.m_questId = questId;
        this.m_participants = participants;
        this.m_startTime = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));
        this.m_isCompleted = false;
    }

    public func InitializeFromState(missionState: CooperativeMissionState) -> Void {
        this.m_questId = missionState.currentQuest;
        this.m_startTime = missionState.missionStartTime;
        this.m_isCompleted = false;
    }

    // Getters
    public func GetQuestId() -> String { return this.m_questId; }
    public func GetParticipants() -> array<Uint32> { return this.m_participants; }
    public func GetStartTime() -> Float { return this.m_startTime; }
    public func IsCompleted() -> Bool { return this.m_isCompleted; }
}

// Data Structures
public struct CooperativeMissionState {
    public let currentQuest: String;
    public let currentPhase: String;
    public let isInMission: Bool;
    public let missionStartTime: Float;
    public let syncVersion: Uint32;
    public let hostPlayerId: Uint32;
}

public struct ObjectiveState {
    public let objectiveId: String;
    public let state: gameJournalEntryState;
    public let isOptional: Bool;
}

public struct DialogueState {
    public let currentSpeaker: String;
    public let currentLine: String;
    public let isInDialogue: Bool;
    public let availableChoices: String;
    public let timeoutTime: Float;
}

public struct QuestProgressTracker {
    public let completedObjectives: Uint32;
    public let totalObjectives: Uint32;
    public let progressPercentage: Float;
}

public struct QuestObjectiveData {
    public let id: String;
    public let description: String;
    public let isOptional: Bool;
    public let state: gameJournalEntryState;
}

// Native function declarations for C++ integration
native func Native_RegisterMissionCallbacks() -> Void;
native func Native_IsValidQuest(questId: String) -> Bool;
native func Native_GetCurrentQuest() -> String;
native func Native_GetCurrentQuestPhase() -> String;
native func Native_GetQuestObjectives(questId: String) -> array<QuestObjectiveData>;
native func Native_GetMissionLocation(questId: String) -> Vector4;
native func Native_GetLocalPlayerId() -> Uint32;
native func Native_GetPlayerName(playerId: Uint32) -> String;
native func Native_GetPlayerPosition(playerId: Uint32) -> Vector4;
native func Native_IsPlayerOnline(playerId: Uint32) -> Bool;
native func Native_IsPlayerConnected(playerId: Uint32) -> Bool;
native func Native_IsPlayerInMission(playerId: Uint32) -> Bool;

native func Native_SyncMissionState(playerId: Uint32, missionState: CooperativeMissionState, objectives: array<ObjectiveState>, dialogue: DialogueState) -> Void;
native func Native_SyncQuestStart(playerId: Uint32, questId: String) -> Void;
native func Native_SyncObjectiveState(playerId: Uint32, questId: String, objectiveId: String, state: gameJournalEntryState) -> Void;
native func Native_SyncDialogueStart(playerId: Uint32, speakerId: String, dialogueId: String) -> Void;
native func Native_SyncDialogueChoice(playerId: Uint32, choiceIndex: Int32) -> Void;
native func Native_SyncDialogueState(dialogue: DialogueState) -> Void;
native func Native_SyncToMissionStart(playerId: Uint32, questId: String) -> Void;
native func Native_SyncToMissionState(missionState: CooperativeMissionState) -> Void;
native func Native_ReturnToSingleplayerMode() -> Void;

native func Native_SetCurrentQuest(questId: String) -> Void;
native func Native_SetCurrentQuestPhase(phase: String) -> Void;
native func Native_TransferMissionHost(newHostId: Uint32, missionState: CooperativeMissionState) -> Void;
native func Native_CreateCooperativeCheckpoint(playerId: Uint32, checkpointName: String) -> Void;
native func Native_RequestDialogueChoice(hostId: Uint32, choiceIndex: Int32) -> Void;
native func Native_NotifyMissionDeparture(playerId: Uint32) -> Void;
native func Native_NotifyMissionEnd(playerId: Uint32) -> Void;

// Network event handlers
@addMethod(PlayerPuppet)
public func OnNetworkMissionStateReceived(missionState: CooperativeMissionState, objectives: array<ObjectiveState>, dialogue: DialogueState) -> Void {
    CooperativeMissionManager.GetInstance().OnMissionStateReceived(missionState, objectives, dialogue);
}

@addMethod(PlayerPuppet)
public func OnNetworkQuestStartReceived(questId: String) -> Void {
    CooperativeMissionManager.GetInstance().OnQuestStartReceived(questId);
}

@addMethod(PlayerPuppet)
public func OnNetworkObjectiveStateReceived(questId: String, objectiveId: String, state: gameJournalEntryState) -> Void {
    CooperativeMissionManager.GetInstance().OnObjectiveStateReceived(questId, objectiveId, state);
}

@addMethod(PlayerPuppet)
public func OnNetworkDialogueChoiceReceived(choiceIndex: Int32) -> Void {
    CooperativeMissionManager.GetInstance().OnDialogueChoiceReceived(choiceIndex);
}

@addMethod(PlayerPuppet)
public func OnNetworkPlayerJoinedMission(playerId: Uint32, playerName: String) -> Void {
    CooperativeMissionManager.GetInstance().OnPlayerJoinedMission(playerId, playerName);
}

@addMethod(PlayerPuppet)
public func OnNetworkPlayerLeftMission(playerId: Uint32, playerName: String) -> Void {
    CooperativeMissionManager.GetInstance().OnPlayerLeftMission(playerId, playerName);
}

// Integration with game initialization
@wrapMethod(PlayerPuppetPS)
protected cb func OnGameAttached() -> Void {
    wrappedMethod();

    // Initialize cooperative mission system when player is fully loaded
    let player = GameInstance.GetPlayerSystem(GetGameInstance()).GetLocalPlayerMainGameObject() as PlayerPuppet;
    if IsDefined(player) {
        CooperativeMissionManager.GetInstance().Initialize(player);
    }
}

// Console commands for testing
@addMethod(PlayerPuppet)
public func StartCoopMission(questId: String) -> Void {
    let participants: array<Uint32>;
    ArrayPush(participants, 1u); // Add local player
    CooperativeMissionManager.GetInstance().StartCooperativeMission(questId, participants);
}

@addMethod(PlayerPuppet)
public func LeaveMission() -> Void {
    CooperativeMissionManager.GetInstance().LeaveMission();
}

@addMethod(PlayerPuppet)
public func ToggleSyncChoices() -> Void {
    let manager = CooperativeMissionManager.GetInstance();
    manager.SetSyncChoices(!manager.GetCurrentMission().IsCompleted()); // Toggle based on mission state
}