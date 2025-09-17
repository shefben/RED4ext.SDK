import Codeware.UI

public enum DialogSessionState {
    Inactive = 0,
    WaitingForPlayers = 1,
    PresentingChoices = 2,
    Voting = 3,
    ResolvingChoice = 4,
    Completed = 5
}

public enum DialogVoteType {
    Unanimous = 0,     // All players must agree
    Majority = 1,      // Simple majority wins
    Democratic = 2,    // Weighted by player contribution/level
    HostDecides = 3    // Host makes final choice if tied
}

public struct DialogChoice {
    public var choiceId: String;
    public var text: String;
    public var speaker: String;
    public var requirements: array<String>; // Life path, attribute requirements
    public var consequences: array<String>; // What this choice affects
    public var isAvailable: Bool;
    public var votes: array<String>; // Player UUIDs who voted for this
    public var weight: Int32; // Importance weight for decision making
}

public struct DialogSession {
    public var sessionId: String;
    public var questId: String;
    public var dialogNodeId: String;
    public var state: DialogSessionState;
    public var voteType: DialogVoteType;
    public var choices: array<DialogChoice>;
    public var participants: array<String>; // Player UUIDs
    public var startTime: Float;
    public var timeoutDuration: Float; // Seconds before auto-resolve
    public var selectedChoiceId: String;
    public var isStoryImportant: Bool; // Major story decision vs minor dialog
}

public struct PlayerDialogState {
    public var playerId: String;
    public var isReady: Bool;
    public var hasVoted: Bool;
    public var selectedChoiceId: String;
    public var lastActiveTime: Float;
}

public class MultiplayerDialog {
    private static var currentSession: DialogSession;
    private static var playerStates: array<PlayerDialogState>;
    private static var isInitialized: Bool = false;
    private static var dialogUI: ref<DialogSessionUI>;
    
    // Network callbacks
    private static cb func OnDialogSessionStarted(sessionData: String) -> Void;
    private static cb func OnDialogChoicePresented(choiceData: String) -> Void;
    private static cb func OnPlayerVoted(voteData: String) -> Void;
    private static cb func OnDialogSessionResolved(resultData: String) -> Void;
    
    public static func Initialize() -> Void {
        if isInitialized {
            return;
        }
        
        LogChannel(n"COOP_DIALOG", "Initializing multiplayer dialog system...");
        
        // Register network callbacks
        NetworkingSystem.RegisterCallback("dialog_session_start", MultiplayerDialog.OnDialogSessionStarted);
        NetworkingSystem.RegisterCallback("dialog_choice_present", MultiplayerDialog.OnDialogChoicePresented);
        NetworkingSystem.RegisterCallback("dialog_player_vote", MultiplayerDialog.OnPlayerVoted);
        NetworkingSystem.RegisterCallback("dialog_session_resolve", MultiplayerDialog.OnDialogSessionResolved);
        
        // Initialize default session
        MultiplayerDialog.ResetSession();
        
        isInitialized = true;
        LogChannel(n"COOP_DIALOG", "Multiplayer dialog system initialized");
    }
    
    public static func StartDialogSession(questId: String, dialogNodeId: String, choices: array<DialogChoice>, voteType: DialogVoteType, timeoutSecs: Float, isStoryImportant: Bool) -> String {
        // Generate unique session ID
        let sessionId = questId + "_" + dialogNodeId + "_" + ToString(GetGameTime());
        
        // Create new session
        currentSession.sessionId = sessionId;
        currentSession.questId = questId;
        currentSession.dialogNodeId = dialogNodeId;
        currentSession.state = DialogSessionState.WaitingForPlayers;
        currentSession.voteType = voteType;
        currentSession.choices = choices;
        currentSession.startTime = GetGameTime();
        currentSession.timeoutDuration = timeoutSecs;
        currentSession.isStoryImportant = isStoryImportant;
        currentSession.selectedChoiceId = "";
        
        // Get connected players
        currentSession.participants = NetworkingSystem.GetConnectedPlayerIds();
        
        // Initialize player states
        ArrayClear(playerStates);
        for playerId in currentSession.participants {
            let playerState: PlayerDialogState;
            playerState.playerId = playerId;
            playerState.isReady = false;
            playerState.hasVoted = false;
            playerState.selectedChoiceId = "";
            playerState.lastActiveTime = GetGameTime();
            ArrayPush(playerStates, playerState);
        }
        
        LogChannel(n"COOP_DIALOG", "Started dialog session: " + sessionId + " with " + ToString(ArraySize(currentSession.participants)) + " players");
        
        // Notify all players
        let sessionData = MultiplayerDialog.SerializeSession(currentSession);
        NetworkingSystem.BroadcastMessage("dialog_session_start", sessionData);
        
        // Show UI for local player
        MultiplayerDialog.ShowDialogUI();
        
        return sessionId;
    }
    
    public static func PresentChoices() -> Void {
        if currentSession.state != DialogSessionState.WaitingForPlayers {
            return;
        }
        
        // Filter choices based on player requirements
        MultiplayerDialog.FilterAvailableChoices();
        
        currentSession.state = DialogSessionState.PresentingChoices;
        
        LogChannel(n"COOP_DIALOG", "Presenting " + ToString(ArraySize(currentSession.choices)) + " dialog choices to players");
        
        // Send choices to all players
        let choiceData = MultiplayerDialog.SerializeChoices(currentSession.choices);
        NetworkingSystem.BroadcastMessage("dialog_choice_present", choiceData);
        
        // Update UI
        if IsDefined(dialogUI) {
            dialogUI.PresentChoices(currentSession.choices);
        }
        
        // Start voting phase after brief delay
        DelaySystem.DelayCallback(MultiplayerDialog.StartVoting, 2.0);
    }
    
    public static func StartVoting() -> Void {
        if currentSession.state != DialogSessionState.PresentingChoices {
            return;
        }
        
        currentSession.state = DialogSessionState.Voting;
        LogChannel(n"COOP_DIALOG", "Starting voting phase with " + ToString(currentSession.timeoutDuration) + " second timeout");
        
        if IsDefined(dialogUI) {
            dialogUI.StartVoting(currentSession.timeoutDuration);
        }
        
        // Set timeout for auto-resolution
        DelaySystem.DelayCallback(MultiplayerDialog.ResolveVoting, currentSession.timeoutDuration);
    }
    
    public static func SubmitVote(playerId: String, choiceId: String) -> Bool {
        if currentSession.state != DialogSessionState.Voting {
            LogChannel(n"COOP_DIALOG", "Vote rejected: not in voting phase");
            return false;
        }
        
        // Find player state
        let playerIndex = -1;
        for i in Range(ArraySize(playerStates)) {
            if Equals(playerStates[i].playerId, playerId) {
                playerIndex = i;
                break;
            }
        }
        
        if playerIndex == -1 {
            LogChannel(n"COOP_DIALOG", "Vote rejected: player not in session");
            return false;
        }
        
        // Validate choice exists
        let choiceExists = false;
        for choice in currentSession.choices {
            if Equals(choice.choiceId, choiceId) && choice.isAvailable {
                choiceExists = true;
                // Add vote to choice
                ArrayPush(choice.votes, playerId);
                break;
            }
        }
        
        if !choiceExists {
            LogChannel(n"COOP_DIALOG", "Vote rejected: invalid choice");
            return false;
        }
        
        // Update player state
        playerStates[playerIndex].hasVoted = true;
        playerStates[playerIndex].selectedChoiceId = choiceId;
        playerStates[playerIndex].lastActiveTime = GetGameTime();
        
        LogChannel(n"COOP_DIALOG", "Player " + playerId + " voted for choice: " + choiceId);
        
        // Broadcast vote to other players
        let voteData = playerId + "|" + choiceId;
        NetworkingSystem.BroadcastMessage("dialog_player_vote", voteData);
        
        // Update UI
        if IsDefined(dialogUI) {
            dialogUI.UpdateVoteStatus(MultiplayerDialog.GetVoteProgress());
        }
        
        // Check if all players have voted
        if (MultiplayerDialog.AllPlayersVoted()) {
            DelaySystem.DelayCallback(MultiplayerDialog.ResolveVoting, 1.0); // Brief delay to show final votes
        }
        
        return true;
    }
    
    public static func ResolveVoting() -> Void {
        if currentSession.state != DialogSessionState.Voting {
            return;
        }
        
        currentSession.state = DialogSessionState.ResolvingChoice;
        
        let winningChoice = MultiplayerDialog.DetermineWinningChoice();
        currentSession.selectedChoiceId = winningChoice.choiceId;
        
        LogChannel(n"COOP_DIALOG", "Dialog voting resolved. Winning choice: " + winningChoice.choiceId + " - " + winningChoice.text);
        
        // Notify CampaignSync if this is a story important choice
        if currentSession.isStoryImportant {
            let choiceOptions: array<String>;
            for choice in currentSession.choices {
                ArrayPush(choiceOptions, choice.text);
            }
            
            CampaignSync.RegisterStoryChoice(currentSession.sessionId, currentSession.questId, winningChoice.text, choiceOptions, winningChoice.choiceId);
        }
        
        // Broadcast result to all players
        let resultData = MultiplayerDialog.SerializeResult(winningChoice);
        NetworkingSystem.BroadcastMessage("dialog_session_resolve", resultData);
        
        // Update UI
        if IsDefined(dialogUI) {
            dialogUI.ShowResult(winningChoice, 3.0);
        }
        
        // Execute the chosen dialog option in game
        MultiplayerDialog.ExecuteDialogChoice(winningChoice);
        
        // Complete session after brief display
        DelaySystem.DelayCallback(MultiplayerDialog.CompleteSession, 3.0);
    }
    
    private static func DetermineWinningChoice() -> DialogChoice {
        let winningChoice: DialogChoice;
        let maxVotes = 0;
        let totalVotes = 0;
        
        // Count votes for each choice
        for choice in currentSession.choices {
            let voteCount = ArraySize(choice.votes);
            totalVotes += voteCount;
            
            if voteCount > maxVotes {
                maxVotes = voteCount;
                winningChoice = choice;
            }
        }
        
        // Handle vote resolution based on type
        switch currentSession.voteType {
            case DialogVoteType.Unanimous:
                // Require all players to vote for same option
                if maxVotes != ArraySize(currentSession.participants) {
                    winningChoice = MultiplayerDialog.GetDefaultChoice();
                }
                break;
                
            case DialogVoteType.Majority:
                // Simple majority wins (already handled above)
                break;
                
            case DialogVoteType.Democratic:
                // Weight votes by player contribution/level
                winningChoice = MultiplayerDialog.GetWeightedWinner();
                break;
                
            case DialogVoteType.HostDecides:
                // If tied, host decides
                if MultiplayerDialog.HasTie() {
                    winningChoice = MultiplayerDialog.GetHostChoice();
                }
                break;
        }
        
        // Fallback to first available choice if no clear winner
        if Equals(winningChoice.choiceId, "") {
            for choice in currentSession.choices {
                if choice.isAvailable {
                    winningChoice = choice;
                    break;
                }
            }
        }
        
        return winningChoice;
    }
    
    private static func FilterAvailableChoices() -> Void {
        let connectedPlayers = NetworkingSystem.GetConnectedPlayerIds();
        
        for choice in currentSession.choices {
            choice.isAvailable = true;
            
            // Check if any connected player meets requirements
            if ArraySize(choice.requirements) > 0 {
                choice.isAvailable = false;
                
                for playerId in connectedPlayers {
                    if MultiplayerDialog.PlayerMeetsRequirements(playerId, choice.requirements) {
                        choice.isAvailable = true;
                        break;
                    }
                }
            }
        }
    }
    
    private static func PlayerMeetsRequirements(playerId: String, requirements: array<String>) -> Bool {
        let playerData = NetworkingSystem.GetPlayerData(playerId);
        
        for requirement in requirements {
            if StrContains(requirement, "LifePath:") {
                let lifePath = StrAfter(requirement, "LifePath:");
                if !Equals(playerData.lifePath, lifePath) {
                    return false;
                }
            } else if StrContains(requirement, "Attribute:") {
                let attrReq = StrAfter(requirement, "Attribute:");
                let parts = StrSplit(attrReq, ">=");
                if ArraySize(parts) == 2 {
                    let attrName = parts[0];
                    let minValue = StringToInt(parts[1]);
                    if playerData.GetAttributeValue(attrName) < minValue {
                        return false;
                    }
                }
            }
        }
        
        return true;
    }
    
    private static func ExecuteDialogChoice(choice: DialogChoice) -> Void {
        LogChannel(n"COOP_DIALOG", "Executing dialog choice: " + choice.choiceId);
        
        // Get current dialog system
        let dialogSystem = GameInstance.GetScriptableSystemsContainer(GetGame()).Get(n"DialogSystem") as DialogSystem;
        if !IsDefined(dialogSystem) {
            LogChannel(n"COOP_DIALOG", "Error: Could not get dialog system");
            return;
        }
        
        // Execute the choice through game's dialog system
        let choiceEvent = new DialogChoiceEvent();
        choiceEvent.choiceId = choice.choiceId;
        choiceEvent.dialogNodeId = currentSession.dialogNodeId;
        dialogSystem.ProcessChoiceEvent(choiceEvent);
        
        // Apply consequences if any
        for consequence in choice.consequences {
            MultiplayerDialog.ApplyConsequence(consequence);
        }
    }
    
    private static func ApplyConsequence(consequence: String) -> Void {
        if StrContains(consequence, "Relationship:") {
            let relationshipData = StrAfter(consequence, "Relationship:");
            let parts = StrSplit(relationshipData, "|");
            if ArraySize(parts) >= 2 {
                let characterName = parts[0];
                let deltaStr = parts[1];
                let delta = StringToInt(deltaStr);
                let isRomance = ArraySize(parts) > 2 && Equals(parts[2], "romance");
                
                CampaignSync.UpdateRelationship(characterName, delta, isRomance);
            }
        } else if StrContains(consequence, "QuestFlag:") {
            let flagData = StrAfter(consequence, "QuestFlag:");
            CampaignSync.SetQuestFlag(currentSession.questId, flagData, true);
        }
    }
    
    public static func CompleteSession() -> Void {
        currentSession.state = DialogSessionState.Completed;
        
        LogChannel(n"COOP_DIALOG", "Dialog session completed: " + currentSession.sessionId);
        
        // Hide UI
        if IsDefined(dialogUI) {
            dialogUI.Hide();
        }
        
        // Reset for next session
        MultiplayerDialog.ResetSession();
    }
    
    private static func ResetSession() -> Void {
        currentSession.sessionId = "";
        currentSession.questId = "";
        currentSession.dialogNodeId = "";
        currentSession.state = DialogSessionState.Inactive;
        currentSession.voteType = DialogVoteType.Majority;
        ArrayClear(currentSession.choices);
        ArrayClear(currentSession.participants);
        currentSession.startTime = 0.0;
        currentSession.timeoutDuration = 30.0;
        currentSession.selectedChoiceId = "";
        currentSession.isStoryImportant = false;
        
        ArrayClear(playerStates);
    }
    
    // UI Integration
    private static func ShowDialogUI() -> Void {
        if !IsDefined(dialogUI) {
            dialogUI = new DialogSessionUI();
            dialogUI.Initialize();
        }
        
        dialogUI.Show(currentSession);
    }
    
    // Utility functions
    private static func AllPlayersVoted() -> Bool {
        for playerState in playerStates {
            if !playerState.hasVoted {
                return false;
            }
        }
        return true;
    }
    
    private static func GetVoteProgress() -> String {
        let voted = 0;
        for playerState in playerStates {
            if playerState.hasVoted {
                voted += 1;
            }
        }
        return ToString(voted) + "/" + ToString(ArraySize(playerStates));
    }
    
    private static func GetDefaultChoice() -> DialogChoice {
        for choice in currentSession.choices {
            if choice.isAvailable {
                return choice;
            }
        }
        return new DialogChoice();
    }
    
    private static func GetWeightedWinner() -> DialogChoice {
        // TODO: Implement weighted voting based on player level/contribution
        return MultiplayerDialog.GetDefaultChoice();
    }
    
    private static func HasTie() -> Bool {
        let maxVotes = 0;
        let tieCount = 0;
        
        for choice in currentSession.choices {
            let voteCount = ArraySize(choice.votes);
            if voteCount > maxVotes {
                maxVotes = voteCount;
                tieCount = 1;
            } else if voteCount == maxVotes && maxVotes > 0 {
                tieCount += 1;
            }
        }
        
        return tieCount > 1;
    }
    
    private static func GetHostChoice() -> DialogChoice {
        let hostId = NetworkingSystem.GetHostPlayerId();
        for playerState in playerStates {
            if Equals(playerState.playerId, hostId) {
                for choice in currentSession.choices {
                    if Equals(choice.choiceId, playerState.selectedChoiceId) {
                        return choice;
                    }
                }
            }
        }
        return MultiplayerDialog.GetDefaultChoice();
    }
    
    // Serialization functions
    private static func SerializeSession(session: DialogSession) -> String {
        // TODO: Implement proper JSON serialization
        return session.sessionId + "|" + session.questId + "|" + session.dialogNodeId + "|" + ToString(Cast<Int32>(session.voteType)) + "|" + ToString(session.timeoutDuration) + "|" + ToString(session.isStoryImportant);
    }
    
    private static func SerializeChoices(choices: array<DialogChoice>) -> String {
        let result = "";
        for choice in choices {
            if !Equals(result, "") {
                result += ";;";
            }
            result += choice.choiceId + "|" + choice.text + "|" + choice.speaker + "|" + ToString(choice.isAvailable);
        }
        return result;
    }
    
    private static func SerializeResult(choice: DialogChoice) -> String {
        return choice.choiceId + "|" + choice.text + "|" + choice.speaker;
    }
    
    // Network event handlers
    private static cb func OnDialogSessionStarted(sessionData: String) -> Void {
        LogChannel(n"COOP_DIALOG", "Received dialog session start: " + sessionData);
        // Parse and set up session for client
        // TODO: Implement session deserialization
    }
    
    private static cb func OnDialogChoicePresented(choiceData: String) -> Void {
        LogChannel(n"COOP_DIALOG", "Received dialog choices: " + choiceData);
        // Parse and display choices to client
        // TODO: Implement choice deserialization
    }
    
    private static cb func OnPlayerVoted(voteData: String) -> Void {
        let parts = StrSplit(voteData, "|");
        if ArraySize(parts) >= 2 {
            let playerId = parts[0];
            let choiceId = parts[1];
            LogChannel(n"COOP_DIALOG", "Player " + playerId + " voted for: " + choiceId);
            
            // Update UI to show other players' votes
            if IsDefined(dialogUI) {
                dialogUI.UpdatePlayerVote(playerId, choiceId);
            }
        }
    }
    
    private static cb func OnDialogSessionResolved(resultData: String) -> Void {
        let parts = StrSplit(resultData, "|");
        if ArraySize(parts) >= 2 {
            let choiceId = parts[0];
            let choiceText = parts[1];
            LogChannel(n"COOP_DIALOG", "Dialog resolved with choice: " + choiceText);
            
            // Show result and hide UI
            if IsDefined(dialogUI) {
                let winningChoice: DialogChoice;
                winningChoice.choiceId = choiceId;
                winningChoice.text = choiceText;
                dialogUI.ShowResult(winningChoice, 3.0);
            }
        }
    }
    
    // Public API for game integration
    public static func IsDialogSessionActive() -> Bool {
        return currentSession.state != DialogSessionState.Inactive && currentSession.state != DialogSessionState.Completed;
    }
    
    public static func GetCurrentSessionId() -> String {
        return currentSession.sessionId;
    }
    
    public static func ForceCompleteSession() -> Void {
        if MultiplayerDialog.IsDialogSessionActive() {
            LogChannel(n"COOP_DIALOG", "Force completing dialog session");
            MultiplayerDialog.CompleteSession();
        }
    }
}