// Romance system conflict resolution for multiplayer cooperative gameplay

public enum RomanceConflictType {
    NoConflict = 0,
    MultipleClaimants = 1,    // Multiple players interested in same character
    ProgressionMismatch = 2,  // Different romance progression levels
    ExclusiveChoice = 3,      // Romance choice that affects all players
    LifePathRestriction = 4,  // Life path specific romance requirements
    PlayerPreference = 5,     // Player-defined romance preferences
    StoryConflict = 6        // Romance conflicts with main story choices
}

public enum RomanceResolutionMode {
    FirstCome = 0,           // First player to initiate gets priority
    Democratic = 1,          // Players vote on resolution
    HostDecides = 2,         // Host makes final decision
    Separate = 3,            // Each player has separate romance states
    SharedProgression = 4,   // All players share romance progression
    TurnBased = 5           // Players take turns with romance content
}

public enum RomanceInteractionType {
    Flirt = 0,
    Kiss = 1,
    Intimate = 2,
    Gift = 3,
    DateActivity = 4,
    StoryChoice = 5,
    Commitment = 6,
    Breakup = 7
}

public struct RomanceState {
    public var characterName: String;
    public var playerOwnerId: String;        // Primary player for this romance
    public var relationshipLevel: Int32;     // 0-100 relationship level
    public var romanceStage: String;         // Current romance stage
    public var isActive: Bool;               // Is romance currently active
    public var isExclusive: Bool;            // Is this an exclusive relationship
    public var lastInteractionTime: Float;  // When last romance interaction occurred
    public var interactionHistory: array<RomanceInteraction>;
    public var conflictStatus: RomanceConflictType;
    public var resolutionMode: RomanceResolutionMode;
    public var involvedPlayers: array<String>; // Players involved in this romance
}

public struct RomanceInteraction {
    public var playerId: String;
    public var interactionType: RomanceInteractionType;
    public var characterName: String;
    public var timestamp: Float;
    public var questContext: String;
    public var choiceText: String;
    public var relationshipDelta: Int32;
    public var wasApproved: Bool; // Was this interaction approved by other players
}

public struct RomanceConflict {
    public var conflictId: String;
    public var conflictType: RomanceConflictType;
    public var characterName: String;
    public var involvedPlayers: array<String>;
    public var description: String;
    public var proposedResolution: String;
    public var playerVotes: array<RomanceVote>;
    public var resolutionDeadline: Float;
    public var isResolved: Bool;
    public var finalResolution: String;
}

public struct RomanceVote {
    public var playerId: String;
    public var preferredResolution: RomanceResolutionMode;
    public var reasoning: String;
    public var voteTime: Float;
}

public struct PlayerRomancePreferences {
    public var playerId: String;
    public var preferredCharacters: array<String>;
    public var allowedInteractionTypes: array<RomanceInteractionType>;
    public var sharingMode: RomanceResolutionMode;
    public var isOptedOut: Bool; // Player opts out of romance content
    public var respectOthers: Bool; // Respects other players' romance choices
}

public class RomanceConflictResolver {
    private static var isInitialized: Bool = false;
    private static var activeRomances: array<RomanceState>;
    private static var pendingConflicts: array<RomanceConflict>;
    private static var playerPreferences: array<PlayerRomancePreferences>;
    private static var conflictUI: ref<RomanceConflictUI>;
    private static var globalResolutionMode: RomanceResolutionMode;
    
    // Network callbacks
    private static cb func OnRomanceInteractionRequest(data: String) -> Void;
    private static cb func OnRomanceConflictVote(data: String) -> Void;
    private static cb func OnRomanceStateSync(data: String) -> Void;
    private static cb func OnPlayerPreferencesUpdate(data: String) -> Void;
    
    public static func Initialize() -> Void {
        if isInitialized {
            return;
        }
        
        LogChannel(n"COOP_ROMANCE", "Initializing romance conflict resolution system...");
        
        // Set default resolution mode
        globalResolutionMode = RomanceResolutionMode.Democratic;
        
        // Register network callbacks
        NetworkingSystem.RegisterCallback("romance_interaction_request", RomanceConflictResolver.OnRomanceInteractionRequest);
        NetworkingSystem.RegisterCallback("romance_conflict_vote", RomanceConflictResolver.OnRomanceConflictVote);
        NetworkingSystem.RegisterCallback("romance_state_sync", RomanceConflictResolver.OnRomanceStateSync);
        NetworkingSystem.RegisterCallback("player_romance_preferences", RomanceConflictResolver.OnPlayerPreferencesUpdate);
        
        // Initialize player preferences
        RomanceConflictResolver.InitializePlayerPreferences();
        
        isInitialized = true;
        LogChannel(n"COOP_ROMANCE", "Romance conflict resolution system initialized");
    }
    
    public static func SetGlobalResolutionMode(mode: RomanceResolutionMode) -> Void {
        globalResolutionMode = mode;
        LogChannel(n"COOP_ROMANCE", "Global romance resolution mode set to: " + ToString(Cast<Int32>(mode)));
        
        // Broadcast to other players
        let modeData = ToString(Cast<Int32>(mode));
        NetworkingSystem.BroadcastMessage("romance_resolution_mode", modeData);
    }
    
    public static func SetPlayerRomancePreferences(playerId: String, preferences: PlayerRomancePreferences) -> Void {
        // Find existing preferences or create new
        let existingIndex = -1;
        for i in Range(ArraySize(playerPreferences)) {
            if Equals(playerPreferences[i].playerId, playerId) {
                existingIndex = i;
                break;
            }
        }
        
        if existingIndex >= 0 {
            playerPreferences[existingIndex] = preferences;
        } else {
            ArrayPush(playerPreferences, preferences);
        }
        
        LogChannel(n"COOP_ROMANCE", "Updated romance preferences for player: " + playerId);
        
        // Broadcast preferences to other players
        let prefData = RomanceConflictResolver.SerializePreferences(preferences);
        NetworkingSystem.BroadcastMessage("player_romance_preferences", prefData);
    }
    
    public static func RequestRomanceInteraction(playerId: String, characterName: String, interactionType: RomanceInteractionType, questContext: String, choiceText: String) -> Bool {
        if !NetworkingSystem.IsConnected() {
            // Single player mode - allow interaction
            return true;
        }
        
        LogChannel(n"COOP_ROMANCE", "Romance interaction requested: " + playerId + " -> " + characterName + " (" + ToString(Cast<Int32>(interactionType)) + ")");
        
        // Check if player has opted out
        let playerPrefs = RomanceConflictResolver.GetPlayerPreferences(playerId);
        if playerPrefs.isOptedOut {
            LogChannel(n"COOP_ROMANCE", "Player has opted out of romance content");
            return false;
        }
        
        // Check for existing romance conflicts
        let existingRomance = RomanceConflictResolver.GetRomanceState(characterName);
        let conflictType = RomanceConflictResolver.DetectConflict(playerId, characterName, interactionType, existingRomance);
        
        if conflictType == RomanceConflictType.NoConflict {
            // No conflict - proceed with interaction
            return RomanceConflictResolver.ExecuteRomanceInteraction(playerId, characterName, interactionType, questContext, choiceText);
        }
        
        // Handle conflict based on resolution mode
        return RomanceConflictResolver.HandleRomanceConflict(playerId, characterName, interactionType, questContext, choiceText, conflictType);
    }
    
    private static func DetectConflict(playerId: String, characterName: String, interactionType: RomanceInteractionType, existingRomance: RomanceState) -> RomanceConflictType {
        // Check if character is already in a romance with another player
        if !Equals(existingRomance.characterName, "") {
            if !Equals(existingRomance.playerOwnerId, playerId) && existingRomance.isExclusive {
                return RomanceConflictType.MultipleClaimants;
            }
            
            if !Equals(existingRomance.playerOwnerId, playerId) && existingRomance.isActive {
                // Check interaction type severity
                if interactionType == RomanceInteractionType.Kiss || 
                   interactionType == RomanceInteractionType.Intimate || 
                   interactionType == RomanceInteractionType.Commitment {
                    return RomanceConflictType.ExclusiveChoice;
                }
            }
        }
        
        // Check if other players are interested in this character
        let interestedPlayers = RomanceConflictResolver.GetInterestedPlayers(characterName);
        if ArraySize(interestedPlayers) > 1 {
            for otherPlayerId in interestedPlayers {
                if !Equals(otherPlayerId, playerId) {
                    return RomanceConflictType.MultipleClaimants;
                }
            }
        }
        
        // Check life path restrictions
        if RomanceConflictResolver.HasLifePathRestriction(playerId, characterName)) {
            return RomanceConflictType.LifePathRestriction;
        }
        
        // Check player preferences conflicts
        if RomanceConflictResolver.ViolatesPlayerPreferences(playerId, characterName, interactionType)) {
            return RomanceConflictType.PlayerPreference;
        }
        
        return RomanceConflictType.NoConflict;
    }
    
    private static func HandleRomanceConflict(playerId: String, characterName: String, interactionType: RomanceInteractionType, questContext: String, choiceText: String, conflictType: RomanceConflictType) -> Bool {
        LogChannel(n"COOP_ROMANCE", "Handling romance conflict: " + ToString(Cast<Int32>(conflictType)));
        
        switch globalResolutionMode {
            case RomanceResolutionMode.FirstCome:
                // First player to show interest gets priority
                return RomanceConflictResolver.ResolveFirstCome(playerId, characterName, interactionType);
                
            case RomanceResolutionMode.HostDecides:
                // Host makes the decision
                return RomanceConflictResolver.ResolveHostDecision(playerId, characterName, interactionType);
                
            case RomanceResolutionMode.Separate:
                // Each player has separate romance states
                return RomanceConflictResolver.ResolveSeparateStates(playerId, characterName, interactionType, questContext, choiceText);
                
            case RomanceResolutionMode.SharedProgression:
                // All players share romance progression
                return RomanceConflictResolver.ResolveSharedProgression(playerId, characterName, interactionType);
                
            case RomanceResolutionMode.TurnBased:
                // Players take turns with romance content
                return RomanceConflictResolver.ResolveTurnBased(playerId, characterName, interactionType);
                
            case RomanceResolutionMode.Democratic:
            default:
                // Create conflict for voting
                return RomanceConflictResolver.CreateConflictForVoting(playerId, characterName, interactionType, questContext, choiceText, conflictType);
        }
    }
    
    private static func ResolveFirstCome(playerId: String, characterName: String, interactionType: RomanceInteractionType) -> Bool {
        let existingRomance = RomanceConflictResolver.GetRomanceState(characterName);
        
        if Equals(existingRomance.characterName, "") {
            // No existing romance - this player gets it
            return RomanceConflictResolver.ExecuteRomanceInteraction(playerId, characterName, interactionType, "", "");
        } else if Equals(existingRomance.playerOwnerId, playerId) {
            // Player already owns this romance
            return RomanceConflictResolver.ExecuteRomanceInteraction(playerId, characterName, interactionType, "", "");
        } else {
            // Another player got there first
            LogChannel(n"COOP_ROMANCE", "Romance denied - another player has priority");
            return false;
        }
    }
    
    private static func ResolveHostDecision(playerId: String, characterName: String, interactionType: RomanceInteractionType) -> Bool {
        if NetworkingSystem.IsHost() && Equals(playerId, NetworkingSystem.GetLocalPlayerId()) {
            // Host player gets to decide
            return RomanceConflictResolver.ExecuteRomanceInteraction(playerId, characterName, interactionType, "", "");
        } else if NetworkingSystem.IsHost() {
            // Host decides for other players - present choice to host
            RomanceConflictResolver.PresentHostDecision(playerId, characterName, interactionType);
            return false; // Pending host decision
        } else {
            // Non-host player - request decision from host
            let requestData = playerId + "|" + characterName + "|" + ToString(Cast<Int32>(interactionType));
            NetworkingSystem.SendToHost("romance_host_decision_request", requestData);
            return false; // Pending host decision
        }
    }
    
    private static func ResolveSeparateStates(playerId: String, characterName: String, interactionType: RomanceInteractionType, questContext: String, choiceText: String) -> Bool {
        // Each player maintains separate romance states
        // Create or update player-specific romance state
        let personalRomance = RomanceConflictResolver.GetPlayerSpecificRomance(playerId, characterName);
        
        if Equals(personalRomance.characterName, "") {
            // Create new personal romance state
            personalRomance.characterName = characterName;
            personalRomance.playerOwnerId = playerId;
            personalRomance.relationshipLevel = 0;
            personalRomance.romanceStage = "initial";
            personalRomance.isActive = true;
            personalRomance.isExclusive = false; // Not exclusive in separate mode
            personalRomance.resolutionMode = RomanceResolutionMode.Separate;
            ArrayPush(personalRomance.involvedPlayers, playerId);
            
            ArrayPush(activeRomances, personalRomance);
        }
        
        return RomanceConflictResolver.ExecuteRomanceInteraction(playerId, characterName, interactionType, questContext, choiceText);
    }
    
    private static func ResolveSharedProgression(playerId: String, characterName: String, interactionType: RomanceInteractionType) -> Bool {
        // All players contribute to shared romance progression
        let sharedRomance = RomanceConflictResolver.GetRomanceState(characterName);
        
        if Equals(sharedRomance.characterName, "") {
            // Create shared romance state
            sharedRomance.characterName = characterName;
            sharedRomance.playerOwnerId = "shared";
            sharedRomance.relationshipLevel = 0;
            sharedRomance.romanceStage = "initial";
            sharedRomance.isActive = true;
            sharedRomance.isExclusive = false;
            sharedRomance.resolutionMode = RomanceResolutionMode.SharedProgression;
            
            // Add all connected players
            sharedRomance.involvedPlayers = NetworkingSystem.GetConnectedPlayerIds();
            
            ArrayPush(activeRomances, sharedRomance);
        }
        
        // Add player to involved players if not already there
        if !RomanceConflictResolver.IsPlayerInvolved(sharedRomance, playerId) {
            ArrayPush(sharedRomance.involvedPlayers, playerId);
        }
        
        return RomanceConflictResolver.ExecuteRomanceInteraction(playerId, characterName, interactionType, "", "");
    }
    
    private static func ResolveTurnBased(playerId: String, characterName: String, interactionType: RomanceInteractionType) -> Bool {
        // Check if it's this player's turn
        let turnOrder = RomanceConflictResolver.GetRomanceTurnOrder(characterName);
        let currentTurnPlayer = RomanceConflictResolver.GetCurrentTurnPlayer(characterName, turnOrder);
        
        if Equals(currentTurnPlayer, playerId) {
            // It's this player's turn
            let success = RomanceConflictResolver.ExecuteRomanceInteraction(playerId, characterName, interactionType, "", "");
            if success {
                // Advance to next player's turn
                RomanceConflictResolver.AdvanceRomanceTurn(characterName, turnOrder);
            }
            return success;
        } else {
            // Not this player's turn
            LogChannel(n"COOP_ROMANCE", "Romance denied - not player's turn (current: " + currentTurnPlayer + ")");
            return false;
        }
    }
    
    private static func CreateConflictForVoting(playerId: String, characterName: String, interactionType: RomanceInteractionType, questContext: String, choiceText: String, conflictType: RomanceConflictType) -> Bool {
        // Create a conflict that requires player voting
        let conflict: RomanceConflict;
        conflict.conflictId = characterName + "_" + playerId + "_" + ToString(GetGameTime());
        conflict.conflictType = conflictType;
        conflict.characterName = characterName;
        conflict.involvedPlayers = RomanceConflictResolver.GetInterestedPlayers(characterName);
        conflict.description = RomanceConflictResolver.GenerateConflictDescription(playerId, characterName, interactionType, conflictType);
        conflict.proposedResolution = RomanceConflictResolver.GenerateProposedResolution(conflictType);
        conflict.resolutionDeadline = GetGameTime() + 30.0; // 30 second voting period
        conflict.isResolved = false;
        
        ArrayPush(pendingConflicts, conflict);
        
        LogChannel(n"COOP_ROMANCE", "Created romance conflict for voting: " + conflict.conflictId);
        
        // Present conflict to players
        RomanceConflictResolver.PresentConflictToPlayers(conflict);
        
        return false; // Interaction pending resolution
    }
    
    private static func ExecuteRomanceInteraction(playerId: String, characterName: String, interactionType: RomanceInteractionType, questContext: String, choiceText: String) -> Bool {
        LogChannel(n"COOP_ROMANCE", "Executing romance interaction: " + playerId + " -> " + characterName);
        
        // Create interaction record
        let interaction: RomanceInteraction;
        interaction.playerId = playerId;
        interaction.interactionType = interactionType;
        interaction.characterName = characterName;
        interaction.timestamp = GetGameTime();
        interaction.questContext = questContext;
        interaction.choiceText = choiceText;
        interaction.wasApproved = true;
        
        // Calculate relationship change
        interaction.relationshipDelta = RomanceConflictResolver.CalculateRelationshipDelta(interactionType);
        
        // Update romance state
        let romanceState = RomanceConflictResolver.GetRomanceState(characterName);
        if Equals(romanceState.characterName, "") {
            // Create new romance state
            romanceState.characterName = characterName;
            romanceState.playerOwnerId = playerId;
            romanceState.relationshipLevel = 0;
            romanceState.romanceStage = "initial";
            romanceState.isActive = true;
            romanceState.resolutionMode = globalResolutionMode;
            ArrayPush(romanceState.involvedPlayers, playerId);
            ArrayPush(activeRomances, romanceState);
        }
        
        // Apply relationship change
        romanceState.relationshipLevel += interaction.relationshipDelta;
        romanceState.relationshipLevel = MaxI(0, MinI(100, romanceState.relationshipLevel));
        romanceState.lastInteractionTime = GetGameTime();
        
        // Update romance stage based on level
        romanceState.romanceStage = RomanceConflictResolver.DetermineRomanceStage(romanceState.relationshipLevel);
        
        // Add interaction to history
        ArrayPush(romanceState.interactionHistory, interaction);
        
        // Sync state to all players
        let stateData = RomanceConflictResolver.SerializeRomanceState(romanceState);
        NetworkingSystem.BroadcastMessage("romance_state_sync", stateData);
        
        // Update campaign sync if significant
        if interaction.relationshipDelta >= 10 || interactionType == RomanceInteractionType.Commitment {
            CampaignSync.UpdateRelationship(characterName, interaction.relationshipDelta, true);
        }
        
        return true;
    }
    
    // Conflict resolution UI integration
    private static func PresentConflictToPlayers(conflict: RomanceConflict) -> Void {
        LogChannel(n"COOP_ROMANCE", "Presenting romance conflict to players");
        
        if !IsDefined(conflictUI) {
            conflictUI = new RomanceConflictUI();
            conflictUI.Initialize();
        }
        
        conflictUI.ShowConflict(conflict);
        
        // Broadcast conflict to all players
        let conflictData = RomanceConflictResolver.SerializeConflict(conflict);
        NetworkingSystem.BroadcastMessage("romance_conflict_presentation", conflictData);
    }
    
    public static func VoteOnConflict(conflictId: String, playerId: String, preferredResolution: RomanceResolutionMode, reasoning: String) -> Void {
        // Find the conflict
        let conflictIndex = -1;
        for i in Range(ArraySize(pendingConflicts)) {
            if Equals(pendingConflicts[i].conflictId, conflictId) {
                conflictIndex = i;
                break;
            }
        }
        
        if conflictIndex == -1 {
            LogChannel(n"COOP_ROMANCE", "Conflict not found for voting: " + conflictId);
            return;
        }
        
        let conflict = pendingConflicts[conflictIndex];
        
        // Check if player already voted
        for vote in conflict.playerVotes {
            if Equals(vote.playerId, playerId) {
                LogChannel(n"COOP_ROMANCE", "Player already voted on this conflict");
                return;
            }
        }
        
        // Add vote
        let vote: RomanceVote;
        vote.playerId = playerId;
        vote.preferredResolution = preferredResolution;
        vote.reasoning = reasoning;
        vote.voteTime = GetGameTime();
        
        ArrayPush(conflict.playerVotes, vote);
        pendingConflicts[conflictIndex] = conflict;
        
        LogChannel(n"COOP_ROMANCE", "Vote recorded: " + playerId + " -> " + ToString(Cast<Int32>(preferredResolution)));
        
        // Broadcast vote
        let voteData = conflictId + "|" + playerId + "|" + ToString(Cast<Int32>(preferredResolution)) + "|" + reasoning;
        NetworkingSystem.BroadcastMessage("romance_conflict_vote", voteData);
        
        // Check if voting is complete
        RomanceConflictResolver.CheckVotingCompletion(conflictIndex);
    }
    
    private static func CheckVotingCompletion(conflictIndex: Int32) -> Void {
        let conflict = pendingConflicts[conflictIndex];
        let totalPlayers = ArraySize(conflict.involvedPlayers);
        let votesReceived = ArraySize(conflict.playerVotes);
        
        // Check if all players voted or deadline passed
        let votingComplete = (votesReceived >= totalPlayers) || (GetGameTime() >= conflict.resolutionDeadline);
        
        if votingComplete {
            RomanceConflictResolver.ResolveConflictByVoting(conflictIndex);
        }
    }
    
    private static func ResolveConflictByVoting(conflictIndex: Int32) -> Void {
        let conflict = pendingConflicts[conflictIndex];
        
        // Tally votes
        let resolutionCounts: array<Int32> = [0, 0, 0, 0, 0, 0]; // One for each resolution mode
        for vote in conflict.playerVotes {
            let modeIndex = Cast<Int32>(vote.preferredResolution);
            if modeIndex >= 0 && modeIndex < ArraySize(resolutionCounts) {
                resolutionCounts[modeIndex] += 1;
            }
        }
        
        // Find winning resolution
        let maxVotes = 0;
        let winningResolution = RomanceResolutionMode.Democratic;
        for i in Range(ArraySize(resolutionCounts)) {
            if resolutionCounts[i] > maxVotes {
                maxVotes = resolutionCounts[i];
                winningResolution = IntToEnum(i, RomanceResolutionMode.Democratic);
            }
        }
        
        conflict.isResolved = true;
        conflict.finalResolution = ToString(Cast<Int32>(winningResolution));
        pendingConflicts[conflictIndex] = conflict;
        
        LogChannel(n"COOP_ROMANCE", "Conflict resolved by voting: " + conflict.conflictId + " -> " + conflict.finalResolution);
        
        // Apply the resolution
        RomanceConflictResolver.ApplyConflictResolution(conflict, winningResolution);
        
        // Remove resolved conflict
        ArrayErase(pendingConflicts, conflictIndex);
    }
    
    private static func ApplyConflictResolution(conflict: RomanceConflict, resolution: RomanceResolutionMode) -> Void {
        LogChannel(n"COOP_ROMANCE", "Applying conflict resolution: " + ToString(Cast<Int32>(resolution)));
        
        // Apply the chosen resolution mode for this specific romance
        let romanceState = RomanceConflictResolver.GetRomanceState(conflict.characterName);
        if Equals(romanceState.characterName, "") {
            // Create new romance state
            romanceState.characterName = conflict.characterName;
            romanceState.resolutionMode = resolution;
            romanceState.involvedPlayers = conflict.involvedPlayers;
            ArrayPush(activeRomances, romanceState);
        } else {
            romanceState.resolutionMode = resolution;
        }
        
        // Configure romance based on resolution
        switch resolution {
            case RomanceResolutionMode.FirstCome:
                romanceState.playerOwnerId = conflict.involvedPlayers[0]; // First player gets it
                romanceState.isExclusive = true;
                break;
                
            case RomanceResolutionMode.Separate:
                romanceState.isExclusive = false;
                // Create separate states for each player
                break;
                
            case RomanceResolutionMode.SharedProgression:
                romanceState.playerOwnerId = "shared";
                romanceState.isExclusive = false;
                break;
                
            default:
                romanceState.isExclusive = true;
                break;
        }
        
        // Sync updated state
        let stateData = RomanceConflictResolver.SerializeRomanceState(romanceState);
        NetworkingSystem.BroadcastMessage("romance_state_sync", stateData);
    }
    
    // Utility functions
    private static func GetRomanceState(characterName: String) -> RomanceState {
        for romance in activeRomances {
            if Equals(romance.characterName, characterName) {
                return romance;
            }
        }
        
        let emptyState: RomanceState;
        return emptyState;
    }
    
    private static func GetPlayerSpecificRomance(playerId: String, characterName: String) -> RomanceState {
        for romance in activeRomances {
            if Equals(romance.characterName, characterName) && Equals(romance.playerOwnerId, playerId) {
                return romance;
            }
        }
        
        let emptyState: RomanceState;
        return emptyState;
    }
    
    private static func GetPlayerPreferences(playerId: String) -> PlayerRomancePreferences {
        for prefs in playerPreferences {
            if Equals(prefs.playerId, playerId) {
                return prefs;
            }
        }
        
        // Return default preferences
        let defaultPrefs: PlayerRomancePreferences;
        defaultPrefs.playerId = playerId;
        defaultPrefs.sharingMode = globalResolutionMode;
        defaultPrefs.isOptedOut = false;
        defaultPrefs.respectOthers = true;
        return defaultPrefs;
    }
    
    private static func GetInterestedPlayers(characterName: String) -> array<String> {
        let interestedPlayers: array<String>;
        
        for prefs in playerPreferences {
            for preferredChar in prefs.preferredCharacters {
                if Equals(preferredChar, characterName) {
                    ArrayPush(interestedPlayers, prefs.playerId);
                    break;
                }
            }
        }
        
        return interestedPlayers;
    }
    
    private static func IsPlayerInvolved(romance: RomanceState, playerId: String) -> Bool {
        for involvedPlayer in romance.involvedPlayers {
            if Equals(involvedPlayer, playerId) {
                return true;
            }
        }
        return false;
    }
    
    private static func CalculateRelationshipDelta(interactionType: RomanceInteractionType) -> Int32 {
        switch interactionType {
            case RomanceInteractionType.Flirt:
                return 5;
            case RomanceInteractionType.Kiss:
                return 15;
            case RomanceInteractionType.Intimate:
                return 25;
            case RomanceInteractionType.Gift:
                return 10;
            case RomanceInteractionType.DateActivity:
                return 12;
            case RomanceInteractionType.Commitment:
                return 30;
            case RomanceInteractionType.Breakup:
                return -50;
            default:
                return 3;
        }
    }
    
    private static func DetermineRomanceStage(relationshipLevel: Int32) -> String {
        if relationshipLevel < 20 {
            return "initial";
        } else if relationshipLevel < 40 {
            return "interested";
        } else if relationshipLevel < 60 {
            return "dating";
        } else if relationshipLevel < 80 {
            return "committed";
        } else {
            return "deep_relationship";
        }
    }
    
    private static func InitializePlayerPreferences() -> Void {
        ArrayClear(playerPreferences);
        
        let connectedPlayers = NetworkingSystem.GetConnectedPlayerIds();
        for playerId in connectedPlayers {
            let prefs: PlayerRomancePreferences;
            prefs.playerId = playerId;
            prefs.sharingMode = globalResolutionMode;
            prefs.isOptedOut = false;
            prefs.respectOthers = true;
            ArrayPush(playerPreferences, prefs);
        }
    }
    
    // Additional utility functions would be implemented here...
    
    // Network event handlers
    private static cb func OnRomanceInteractionRequest(data: String) -> Void {
        // Handle romance interaction requests from other players
        LogChannel(n"COOP_ROMANCE", "Received romance interaction request: " + data);
    }
    
    private static cb func OnRomanceConflictVote(data: String) -> Void {
        let parts = StrSplit(data, "|");
        if ArraySize(parts) >= 4 {
            let conflictId = parts[0];
            let playerId = parts[1];
            let resolutionMode = IntToEnum(StringToInt(parts[2]), RomanceResolutionMode.Democratic);
            let reasoning = parts[3];
            
            RomanceConflictResolver.VoteOnConflict(conflictId, playerId, resolutionMode, reasoning);
        }
    }
    
    private static cb func OnRomanceStateSync(data: String) -> Void {
        // Handle romance state updates from other players
        let romanceState = RomanceConflictResolver.DeserializeRomanceState(data);
        RomanceConflictResolver.UpdateLocalRomanceState(romanceState);
    }
    
    private static cb func OnPlayerPreferencesUpdate(data: String) -> Void {
        let preferences = RomanceConflictResolver.DeserializePreferences(data);
        RomanceConflictResolver.UpdatePlayerPreferences(preferences);
    }
    
    // Serialization functions
    private static func SerializeRomanceState(state: RomanceState) -> String {
        // Simple serialization - would use JSON in production
        let data = state.characterName + "|" + state.playerOwnerId + "|" + ToString(state.relationshipLevel);
        data += "|" + state.romanceStage + "|" + ToString(state.isActive) + "|" + ToString(state.isExclusive);
        return data;
    }
    
    private static func DeserializeRomanceState(data: String) -> RomanceState {
        let state: RomanceState;
        let parts = StrSplit(data, "|");
        
        if ArraySize(parts) >= 6 {
            state.characterName = parts[0];
            state.playerOwnerId = parts[1];
            state.relationshipLevel = StringToInt(parts[2]);
            state.romanceStage = parts[3];
            state.isActive = StringToBool(parts[4]);
            state.isExclusive = StringToBool(parts[5]);
        }
        
        return state;
    }
    
    private static func SerializeConflict(conflict: RomanceConflict) -> String {
        let data = conflict.conflictId + "|" + ToString(Cast<Int32>(conflict.conflictType)) + "|" + conflict.characterName;
        data += "|" + conflict.description + "|" + conflict.proposedResolution;
        return data;
    }
    
    private static func SerializePreferences(prefs: PlayerRomancePreferences) -> String {
        let data = prefs.playerId + "|" + ToString(Cast<Int32>(prefs.sharingMode));
        data += "|" + ToString(prefs.isOptedOut) + "|" + ToString(prefs.respectOthers);
        return data;
    }
    
    private static func DeserializePreferences(data: String) -> PlayerRomancePreferences {
        let prefs: PlayerRomancePreferences;
        let parts = StrSplit(data, "|");
        
        if ArraySize(parts) >= 4 {
            prefs.playerId = parts[0];
            prefs.sharingMode = IntToEnum(StringToInt(parts[1]), RomanceResolutionMode.Democratic);
            prefs.isOptedOut = StringToBool(parts[2]);
            prefs.respectOthers = StringToBool(parts[3]);
        }
        
        return prefs;
    }
    
    // Public API
    public static func GetActiveRomances() -> array<RomanceState> {
        return activeRomances;
    }
    
    public static func GetRomanceForCharacter(characterName: String) -> RomanceState {
        return RomanceConflictResolver.GetRomanceState(characterName);
    }
    
    public static func IsRomanceAllowed(playerId: String, characterName: String) -> Bool {
        let playerPrefs = RomanceConflictResolver.GetPlayerPreferences(playerId);
        return !playerPrefs.isOptedOut;
    }
}