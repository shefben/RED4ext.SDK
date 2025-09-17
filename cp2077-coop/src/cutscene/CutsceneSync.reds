// Cutscene synchronization system for shared cinematic experiences

public enum CutsceneState {
    Inactive = 0,
    Preparing = 1,      // Waiting for all players to be ready
    Playing = 2,        // Cutscene is playing
    Paused = 3,         // Cutscene is paused
    SkipVoting = 4,     // Players are voting to skip
    Completed = 5,      // Cutscene finished
    Cancelled = 6       // Cutscene was cancelled/skipped
}

public enum CutsceneType {
    Story = 0,          // Main story cutscenes
    Dialog = 1,         // Dialog-driven scenes
    Combat = 2,         // Combat sequences
    Exploration = 3,    // World exploration scenes
    Romance = 4,        // Romance scenes
    Custom = 5          // Custom/mod cutscenes
}

public enum PlayerCutsceneRole {
    MainCharacter = 0,  // Primary player (usually host)
    Observer = 1,       // Watching player
    Participant = 2,    // Interactive participant
    Hidden = 3          // Player hidden during cutscene
}

public struct CutsceneSession {
    public var sessionId: String;
    public var cutsceneId: String;
    public var cutsceneType: CutsceneType;
    public var state: CutsceneState;
    public var duration: Float;
    public var currentTime: Float;
    public var startTimestamp: Float;
    public var participants: array<String>; // Player UUIDs
    public var playerRoles: array<PlayerCutsceneRole>;
    public var skipVotes: array<String>; // Players who voted to skip
    public var canSkip: Bool;
    public var requiresConsensus: Bool; // All players must agree to skip
    public var questId: String; // Associated quest
    public var isStoryImportant: Bool;
}

public struct PlayerCutsceneState {
    public var playerId: String;
    public var isReady: Bool;
    public var isLoaded: Bool;
    public var role: PlayerCutsceneRole;
    public var position: Vector3;
    public var rotation: Quaternion;
    public var isVisible: Bool;
    public var lastSyncTime: Float;
}

public struct CutsceneSkipVote {
    public var playerId: String;
    public var wantsToSkip: Bool;
    public var voteTime: Float;
}

public class CutsceneSync {
    private static var isInitialized: Bool = false;
    private static var currentSession: CutsceneSession;
    private static var playerStates: array<PlayerCutsceneState>;
    private static var isHost: Bool = false;
    private static var syncInterval: Float = 0.1; // Sync every 100ms
    private static var lastSyncTime: Float = 0.0;
    private static var cutsceneUI: ref<CutsceneSyncUI>;
    
    // Network callbacks
    private static cb func OnCutsceneStartRequest(data: String) -> Void;
    private static cb func OnCutsceneStateSync(data: String) -> Void;
    private static cb func OnPlayerReadyState(data: String) -> Void;
    private static cb func OnSkipVotecast(data: String) -> Void;
    private static cb func OnCutsceneCompleted(data: String) -> Void;
    
    public static func Initialize() -> Void {
        if isInitialized {
            return;
        }
        
        LogChannel(n"COOP_CUTSCENE", "Initializing cutscene synchronization system...");
        
        // Register network callbacks
        NetworkingSystem.RegisterCallback("cutscene_start_request", CutsceneSync.OnCutsceneStartRequest);
        NetworkingSystem.RegisterCallback("cutscene_state_sync", CutsceneSync.OnCutsceneStateSync);
        NetworkingSystem.RegisterCallback("cutscene_player_ready", CutsceneSync.OnPlayerReadyState);
        NetworkingSystem.RegisterCallback("cutscene_skip_vote", CutsceneSync.OnSkipVotecast);
        NetworkingSystem.RegisterCallback("cutscene_completed", CutsceneSync.OnCutsceneCompleted);
        
        // Initialize session
        CutsceneSync.ResetSession();
        
        isHost = NetworkingSystem.IsHost();
        isInitialized = true;
        
        LogChannel(n"COOP_CUTSCENE", "Cutscene sync system initialized (Host: " + ToString(isHost) + ")");
    }
    
    public static func StartCutscene(cutsceneId: String, cutsceneType: CutsceneType, questId: String, isStoryImportant: Bool) -> String {
        if !NetworkingSystem.IsConnected() {
            // Single player mode - let normal cutscene play
            LogChannel(n"COOP_CUTSCENE", "Single player mode, skipping sync for cutscene: " + cutsceneId);
            return "";
        }
        
        if currentSession.state != CutsceneState.Inactive {
            LogChannel(n"COOP_CUTSCENE", "Cannot start cutscene - another session is active");
            return "";
        }
        
        // Generate session ID
        let sessionId = cutsceneId + "_" + ToString(GetGameTime());
        
        // Initialize session
        currentSession.sessionId = sessionId;
        currentSession.cutsceneId = cutsceneId;
        currentSession.cutsceneType = cutsceneType;
        currentSession.state = CutsceneState.Preparing;
        currentSession.questId = questId;
        currentSession.isStoryImportant = isStoryImportant;
        currentSession.canSkip = !isStoryImportant; // Story cutscenes require consensus
        currentSession.requiresConsensus = isStoryImportant;
        currentSession.startTimestamp = GetGameTime();
        
        // Get connected players
        currentSession.participants = NetworkingSystem.GetConnectedPlayerIds();
        
        // Initialize player states
        CutsceneSync.InitializePlayerStates();
        
        LogChannel(n"COOP_CUTSCENE", "Starting cutscene session: " + sessionId + " with " + ToString(ArraySize(currentSession.participants)) + " players");
        
        // Get cutscene duration
        currentSession.duration = CutsceneSync.GetCutsceneDuration(cutsceneId);
        
        // Notify all players
        let sessionData = CutsceneSync.SerializeSession(currentSession);
        NetworkingSystem.BroadcastMessage("cutscene_start_request", sessionData);
        
        // Show preparation UI
        CutsceneSync.ShowCutsceneUI();
        
        return sessionId;
    }
    
    public static func SetPlayerReady(playerId: String, isReady: Bool) -> Void {
        // Find player state
        for i in Range(ArraySize(playerStates)) {
            if Equals(playerStates[i].playerId, playerId) {
                playerStates[i].isReady = isReady;
                playerStates[i].lastSyncTime = GetGameTime();
                break;
            }
        }
        
        LogChannel(n"COOP_CUTSCENE", "Player " + playerId + " ready state: " + ToString(isReady));
        
        // Broadcast ready state
        let readyData = playerId + "|" + ToString(isReady);
        NetworkingSystem.BroadcastMessage("cutscene_player_ready", readyData);
        
        // Check if all players are ready
        if CutsceneSync.AllPlayersReady() && isHost {
            DelaySystem.DelayCallback(CutsceneSync.BeginCutscenePlayback, 1.0);
        }
        
        // Update UI
        if IsDefined(cutsceneUI) {
            cutsceneUI.UpdatePlayerReadyStates(playerStates);
        }
    }
    
    public static func BeginCutscenePlayback() -> Void {
        if currentSession.state != CutsceneState.Preparing {
            return;
        }
        
        if !isHost {
            LogChannel(n"COOP_CUTSCENE", "Only host can begin cutscene playback");
            return;
        }
        
        LogChannel(n"COOP_CUTSCENE", "Beginning synchronized cutscene playback: " + currentSession.cutsceneId);
        
        // Position players for cutscene
        CutsceneSync.PositionPlayersForCutscene();
        
        // Update session state
        currentSession.state = CutsceneState.Playing;
        currentSession.currentTime = 0.0;
        
        // Start the cutscene
        CutsceneSync.StartGameCutscene(currentSession.cutsceneId);
        
        // Begin sync loop
        CutsceneSync.StartSyncLoop();
        
        // Broadcast state update
        let stateData = CutsceneSync.SerializeSessionState();
        NetworkingSystem.BroadcastMessage("cutscene_state_sync", stateData);
        
        // Update UI
        if IsDefined(cutsceneUI) {
            cutsceneUI.StartCutscenePlayback(currentSession);
        }
    }
    
    public static func VoteToSkip(playerId: String) -> Void {
        if currentSession.state != CutsceneState.Playing {
            return;
        }
        
        if !currentSession.canSkip {
            LogChannel(n"COOP_CUTSCENE", "Cutscene cannot be skipped");
            return;
        }
        
        // Check if player already voted
        for voteId in currentSession.skipVotes {
            if Equals(voteId, playerId) {
                return; // Already voted
            }
        }
        
        ArrayPush(currentSession.skipVotes, playerId);
        LogChannel(n"COOP_CUTSCENE", "Player " + playerId + " voted to skip cutscene");
        
        // Broadcast skip vote
        let voteData = playerId + "|true|" + ToString(GetGameTime());
        NetworkingSystem.BroadcastMessage("cutscene_skip_vote", voteData);
        
        // Check skip conditions
        CutsceneSync.CheckSkipConditions();
        
        // Update UI
        if IsDefined(cutsceneUI) {
            cutsceneUI.UpdateSkipVotes(currentSession.skipVotes, ArraySize(currentSession.participants));
        }
    }
    
    private static func CheckSkipConditions() -> Void {
        let totalPlayers = ArraySize(currentSession.participants);
        let skipVotes = ArraySize(currentSession.skipVotes);
        let shouldSkip = false;
        
        if currentSession.requiresConsensus {
            // All players must agree to skip
            shouldSkip = skipVotes >= totalPlayers;
        } else {
            // Simple majority
            shouldSkip = skipVotes > (totalPlayers / 2);
        }
        
        if shouldSkip && isHost {
            LogChannel(n"COOP_CUTSCENE", "Skip conditions met, ending cutscene");
            CutsceneSync.SkipCutscene();
        }
    }
    
    public static func SkipCutscene() -> Void {
        if currentSession.state != CutsceneState.Playing {
            return;
        }
        
        LogChannel(n"COOP_CUTSCENE", "Skipping cutscene: " + currentSession.cutsceneId);
        
        // Update state
        currentSession.state = CutsceneState.Cancelled;
        
        // Stop the game cutscene
        CutsceneSync.StopGameCutscene();
        
        // Notify all players
        let skipData = currentSession.sessionId + "|skipped";
        NetworkingSystem.BroadcastMessage("cutscene_completed", skipData);
        
        // Complete session
        CutsceneSync.CompleteCutsceneSession();
    }
    
    public static func CompleteCutscene() -> Void {
        if currentSession.state != CutsceneState.Playing {
            return;
        }
        
        LogChannel(n"COOP_CUTSCENE", "Cutscene completed: " + currentSession.cutsceneId);
        
        // Update state
        currentSession.state = CutsceneState.Completed;
        
        // Notify all players
        let completeData = currentSession.sessionId + "|completed";
        NetworkingSystem.BroadcastMessage("cutscene_completed", completeData);
        
        // Complete session
        CutsceneSync.CompleteCutsceneSession();
    }
    
    private static func CompleteCutsceneSession() -> Void {
        // Stop sync loop
        CutsceneSync.StopSyncLoop();
        
        // Restore player positions and visibility
        CutsceneSync.RestorePlayerPositions();
        
        // Hide UI
        if IsDefined(cutsceneUI) {
            cutsceneUI.Hide();
        }
        
        // Update campaign sync if this was a story cutscene
        if currentSession.isStoryImportant && !Equals(currentSession.questId, "") {
            CampaignSync.RegisterCutsceneViewed(currentSession.questId, currentSession.cutsceneId);
        }
        
        // Reset session
        CutsceneSync.ResetSession();
        
        LogChannel(n"COOP_CUTSCENE", "Cutscene session completed and cleaned up");
    }
    
    private static func StartSyncLoop() -> Void {
        // Start regular synchronization updates
        CutsceneSync.SyncCutsceneState();
        DelaySystem.DelayCallback(CutsceneSync.SyncLoop, syncInterval);
    }
    
    private static func SyncLoop() -> Void {
        if currentSession.state != CutsceneState.Playing {
            return; // Stop loop
        }
        
        if isHost {
            // Update current time
            let elapsed = GetGameTime() - currentSession.startTimestamp;
            currentSession.currentTime = elapsed;
            
            // Check if cutscene should end
            if currentSession.currentTime >= currentSession.duration {
                CutsceneSync.CompleteCutscene();
                return;
            }
            
            // Sync state to all players
            CutsceneSync.SyncCutsceneState();
        }
        
        // Continue loop
        DelaySystem.DelayCallback(CutsceneSync.SyncLoop, syncInterval);
    }
    
    private static func StopSyncLoop() -> Void {
        // Loop will naturally stop when state changes
        LogChannel(n"COOP_CUTSCENE", "Sync loop stopped");
    }
    
    private static func SyncCutsceneState() -> Void {
        if !isHost || currentSession.state != CutsceneState.Playing {
            return;
        }
        
        let currentTime = GetGameTime();
        if (currentTime - lastSyncTime) < syncInterval {
            return; // Not time to sync yet
        }
        
        // Broadcast current state
        let stateData = CutsceneSync.SerializeSessionState();
        NetworkingSystem.BroadcastMessage("cutscene_state_sync", stateData);
        
        lastSyncTime = currentTime;
    }
    
    private static func InitializePlayerStates() -> Void {
        ArrayClear(playerStates);
        
        for i in Range(ArraySize(currentSession.participants)) {
            let playerId = currentSession.participants[i];
            let playerState: PlayerCutsceneState;
            
            playerState.playerId = playerId;
            playerState.isReady = false;
            playerState.isLoaded = false;
            playerState.isVisible = true;
            playerState.lastSyncTime = GetGameTime();
            
            // Assign roles
            if i == 0 || Equals(playerId, NetworkingSystem.GetHostPlayerId()) {
                playerState.role = PlayerCutsceneRole.MainCharacter;
            } else {
                playerState.role = PlayerCutsceneRole.Observer;
            }
            
            ArrayPush(playerStates, playerState);
        }
        
        // Set current player as ready by default
        let localPlayerId = NetworkingSystem.GetLocalPlayerId();
        CutsceneSync.SetPlayerReady(localPlayerId, true);
    }
    
    private static func PositionPlayersForCutscene() -> Void {
        LogChannel(n"COOP_CUTSCENE", "Positioning players for cutscene");
        
        let cutsceneArea = CutsceneSync.GetCutsceneArea(currentSession.cutsceneId);
        let playerCount = ArraySize(playerStates);
        
        for i in Range(playerCount) {
            let playerState = playerStates[i];
            let player = NetworkingSystem.GetPlayerById(playerState.playerId);
            
            if !IsDefined(player) {
                continue;
            }
            
            // Calculate position based on role and index
            let position = CutsceneSync.CalculatePlayerPosition(cutsceneArea, playerState.role, i);
            let rotation = CutsceneSync.CalculatePlayerRotation(cutsceneArea, playerState.role);
            
            // Store original position for restoration
            playerState.position = position;
            playerState.rotation = rotation;
            
            // Position player
            if playerState.role == PlayerCutsceneRole.Hidden {
                // Hide player during cutscene
                player.SetVisible(false);
                playerState.isVisible = false;
            } else {
                // Position player in cutscene area
                player.Teleport(position, rotation);
                playerState.isVisible = true;
            }
            
            playerStates[i] = playerState;
        }
    }
    
    private static func RestorePlayerPositions() -> Void {
        LogChannel(n"COOP_CUTSCENE", "Restoring player positions after cutscene");
        
        for playerState in playerStates {
            let player = NetworkingSystem.GetPlayerById(playerState.playerId);
            if IsDefined(player) {
                // Restore visibility
                if !playerState.isVisible {
                    player.SetVisible(true);
                }
                
                // Allow players to move freely again
                player.EnableMovement(true);
            }
        }
    }
    
    // Integration with game's cutscene system
    private static func StartGameCutscene(cutsceneId: String) -> Void {
        let cutsceneSystem = GameInstance.GetScriptableSystemsContainer(GetGame()).Get(n"CutsceneSystem") as CutsceneSystem;
        if IsDefined(cutsceneSystem) {
            cutsceneSystem.PlayCutscene(cutsceneId);
            LogChannel(n"COOP_CUTSCENE", "Started game cutscene: " + cutsceneId);
        }
    }
    
    private static func StopGameCutscene() -> Void {
        let cutsceneSystem = GameInstance.GetScriptableSystemsContainer(GetGame()).Get(n"CutsceneSystem") as CutsceneSystem;
        if IsDefined(cutsceneSystem) {
            cutsceneSystem.StopCurrentCutscene();
            LogChannel(n"COOP_CUTSCENE", "Stopped game cutscene");
        }
    }
    
    private static func GetCutsceneDuration(cutsceneId: String) -> Float {
        // Get duration from game's cutscene system
        let cutsceneSystem = GameInstance.GetScriptableSystemsContainer(GetGame()).Get(n"CutsceneSystem") as CutsceneSystem;
        if IsDefined(cutsceneSystem) {
            return cutsceneSystem.GetCutsceneDuration(cutsceneId);
        }
        return 60.0; // Default 60 seconds
    }
    
    private static func GetCutsceneArea(cutsceneId: String) -> CutsceneArea {
        // Get cutscene area information
        let area: CutsceneArea;
        area.centerPosition = Vector3.Zero();
        area.radius = 10.0;
        area.cameraPosition = Vector3.Zero();
        // This would be populated from cutscene data
        return area;
    }
    
    private static func CalculatePlayerPosition(area: CutsceneArea, role: PlayerCutsceneRole, index: Int32) -> Vector3 {
        let basePosition = area.centerPosition;
        let angle = (Cast<Float>(index) / 4.0) * 360.0; // Distribute players in circle
        let distance = 3.0; // Distance from center
        
        let x = basePosition.X + CosF(angle) * distance;
        let y = basePosition.Y + SinF(angle) * distance;
        let z = basePosition.Z;
        
        return new Vector3(x, y, z);
    }
    
    private static func CalculatePlayerRotation(area: CutsceneArea, role: PlayerCutsceneRole) -> Quaternion {
        // Face towards camera position
        let direction = Vector3.Normalize(area.cameraPosition - area.centerPosition);
        return Quaternion.BuildFromDirectionVector(direction);
    }
    
    // Utility functions
    private static func AllPlayersReady() -> Bool {
        for playerState in playerStates {
            if !playerState.isReady {
                return false;
            }
        }
        return true;
    }
    
    private static func ResetSession() -> Void {
        currentSession.sessionId = "";
        currentSession.cutsceneId = "";
        currentSession.state = CutsceneState.Inactive;
        currentSession.duration = 0.0;
        currentSession.currentTime = 0.0;
        currentSession.startTimestamp = 0.0;
        ArrayClear(currentSession.participants);
        ArrayClear(currentSession.playerRoles);
        ArrayClear(currentSession.skipVotes);
        currentSession.canSkip = true;
        currentSession.requiresConsensus = false;
        currentSession.questId = "";
        currentSession.isStoryImportant = false;
        
        ArrayClear(playerStates);
    }
    
    private static func ShowCutsceneUI() -> Void {
        if !IsDefined(cutsceneUI) {
            cutsceneUI = new CutsceneSyncUI();
            cutsceneUI.Initialize();
        }
        
        cutsceneUI.Show(currentSession, playerStates);
    }
    
    // Serialization functions
    private static func SerializeSession(session: CutsceneSession) -> String {
        let data = session.sessionId + "|" + session.cutsceneId + "|" + ToString(Cast<Int32>(session.cutsceneType));
        data += "|" + ToString(session.duration) + "|" + ToString(session.canSkip) + "|" + ToString(session.requiresConsensus);
        data += "|" + session.questId + "|" + ToString(session.isStoryImportant);
        return data;
    }
    
    private static func SerializeSessionState() -> String {
        let data = currentSession.sessionId + "|" + ToString(Cast<Int32>(currentSession.state));
        data += "|" + ToString(currentSession.currentTime) + "|" + ToString(currentSession.duration);
        return data;
    }
    
    // Network event handlers
    private static cb func OnCutsceneStartRequest(data: String) -> Void {
        LogChannel(n"COOP_CUTSCENE", "Received cutscene start request: " + data);
        
        // Parse and prepare for cutscene
        let parts = StrSplit(data, "|");
        if ArraySize(parts) >= 3 {
            let sessionId = parts[0];
            let cutsceneId = parts[1];
            let cutsceneType = IntToEnum(StringToInt(parts[2]), CutsceneType.Story);
            
            // Set up local session
            currentSession.sessionId = sessionId;
            currentSession.cutsceneId = cutsceneId;
            currentSession.cutsceneType = cutsceneType;
            currentSession.state = CutsceneState.Preparing;
            
            if ArraySize(parts) >= 8 {
                currentSession.duration = StringToFloat(parts[3]);
                currentSession.canSkip = StringToBool(parts[4]);
                currentSession.requiresConsensus = StringToBool(parts[5]);
                currentSession.questId = parts[6];
                currentSession.isStoryImportant = StringToBool(parts[7]);
            }
            
            // Show UI and set ready
            CutsceneSync.ShowCutsceneUI();
            
            let localPlayerId = NetworkingSystem.GetLocalPlayerId();
            CutsceneSync.SetPlayerReady(localPlayerId, true);
        }
    }
    
    private static cb func OnCutsceneStateSync(data: String) -> Void {
        if isHost {
            return; // Host doesn't need to receive its own sync
        }
        
        let parts = StrSplit(data, "|");
        if ArraySize(parts) >= 4 {
            let sessionId = parts[0];
            let state = IntToEnum(StringToInt(parts[1]), CutsceneState.Inactive);
            let currentTime = StringToFloat(parts[2]);
            let duration = StringToFloat(parts[3]);
            
            if Equals(sessionId, currentSession.sessionId) {
                currentSession.state = state;
                currentSession.currentTime = currentTime;
                currentSession.duration = duration;
                
                if state == CutsceneState.Playing && currentSession.state != CutsceneState.Playing {
                    // Start local cutscene playback
                    CutsceneSync.StartGameCutscene(currentSession.cutsceneId);
                }
            }
        }
    }
    
    private static cb func OnPlayerReadyState(data: String) -> Void {
        let parts = StrSplit(data, "|");
        if ArraySize(parts) >= 2 {
            let playerId = parts[0];
            let isReady = StringToBool(parts[1]);
            
            CutsceneSync.SetPlayerReady(playerId, isReady);
        }
    }
    
    private static cb func OnSkipVotecast(data: String) -> Void {
        let parts = StrSplit(data, "|");
        if ArraySize(parts) >= 2 {
            let playerId = parts[0];
            let wantsToSkip = StringToBool(parts[1]);
            
            if wantsToSkip {
                ArrayPush(currentSession.skipVotes, playerId);
                CutsceneSync.CheckSkipConditions();
            }
        }
    }
    
    private static cb func OnCutsceneCompleted(data: String) -> Void {
        let parts = StrSplit(data, "|");
        if ArraySize(parts) >= 2 {
            let sessionId = parts[0];
            let completionType = parts[1];
            
            if Equals(sessionId, currentSession.sessionId) {
                if Equals(completionType, "skipped") {
                    currentSession.state = CutsceneState.Cancelled;
                    CutsceneSync.StopGameCutscene();
                } else {
                    currentSession.state = CutsceneState.Completed;
                }
                
                CutsceneSync.CompleteCutsceneSession();
            }
        }
    }
    
    // Public API
    public static func IsCutsceneActive() -> Bool {
        return currentSession.state == CutsceneState.Playing || currentSession.state == CutsceneState.Preparing;
    }
    
    public static func GetCurrentSession() -> CutsceneSession {
        return currentSession;
    }
    
    public static func ForceStopCutscene() -> Void {
        if CutsceneSync.IsCutsceneActive() {
            LogChannel(n"COOP_CUTSCENE", "Force stopping cutscene session");
            CutsceneSync.SkipCutscene();
        }
    }
}

// Supporting structures
public struct CutsceneArea {
    public var centerPosition: Vector3;
    public var radius: Float;
    public var cameraPosition: Vector3;
}

// Mock cutscene system for integration
public class CutsceneSystem extends ScriptableSystem {
    public func PlayCutscene(cutsceneId: String) -> Void {
        LogChannel(n"COOP_CUTSCENE", "Playing cutscene: " + cutsceneId);
        // This would integrate with CP2077's actual cutscene system
    }
    
    public func StopCurrentCutscene() -> Void {
        LogChannel(n"COOP_CUTSCENE", "Stopping current cutscene");
        // This would integrate with CP2077's actual cutscene system
    }
    
    public func GetCutsceneDuration(cutsceneId: String) -> Float {
        // Return actual cutscene duration
        return 30.0; // Default
    }
}

// Integration hooks
@wrapMethod(CutsceneSystem)
public func PlayCutscene(cutsceneId: String) -> Void {
    if NetworkingSystem.IsConnected() && NetworkingSystem.GetConnectedPlayerCount() > 1 {
        // Intercept for multiplayer synchronization
        let questId = CampaignSync.GetCurrentQuestId();
        let isStoryImportant = CutsceneSync.IsStoryCutscene(cutsceneId);
        
        let sessionId = CutsceneSync.StartCutscene(cutsceneId, CutsceneType.Story, questId, isStoryImportant);
        if !Equals(sessionId, "") {
            return; // Handled by sync system
        }
    }
    
    // Single player or sync failed - play normally
    wrappedMethod(cutsceneId);
}

// Initialize cutscene sync when game starts
@wrapMethod(GameInstance)
public static func GetScriptableSystemsContainer(gameInstance: GameInstance) -> ref<ScriptableSystemsContainer> {
    let container = wrappedMethod(gameInstance);
    
    if IsDefined(container) {
        CutsceneSync.Initialize();
    }
    
    return container;
}