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
        // Implement weighted voting based on player level/contribution
        let weightedChoices: array<ref<WeightedChoice>>;

        // Calculate weighted votes for each choice
        for choice in currentSession.choices {
            let weightedChoice = new WeightedChoice();
            weightedChoice.choice = choice;
            weightedChoice.totalWeight = 0.0;

            for vote in choice.votes {
                let weight = MultiplayerDialog.CalculatePlayerWeight(vote.playerId);
                weightedChoice.totalWeight += weight;
            }

            ArrayPush(weightedChoices, weightedChoice);
        }

        // Find choice with highest weighted vote
        let bestChoice = new DialogChoice();
        let bestWeight = 0.0;

        for weighted in weightedChoices {
            if weighted.totalWeight > bestWeight {
                bestWeight = weighted.totalWeight;
                bestChoice = weighted.choice;
            }
        }

        LogChannel(n"MultiplayerDialog", s"[GetWeightedWinner] Selected choice with weight: \(bestWeight)");
        return bestChoice;
    }

    private static func CalculatePlayerWeight(playerId: Uint32) -> Float {
        // Calculate voting weight based on multiple factors
        let baseWeight = 1.0;
        let levelMultiplier = MultiplayerDialog.GetPlayerLevelMultiplier(playerId);
        let contributionMultiplier = MultiplayerDialog.GetPlayerContributionMultiplier(playerId);
        let reputationMultiplier = MultiplayerDialog.GetPlayerReputationMultiplier(playerId);

        let totalWeight = baseWeight * levelMultiplier * contributionMultiplier * reputationMultiplier;

        LogChannel(n"MultiplayerDialog", s"[CalculatePlayerWeight] Player \(playerId) weight: \(totalWeight)");
        return totalWeight;
    }

    private static func GetPlayerLevelMultiplier(playerId: Uint32) -> Float {
        // Get player level and convert to multiplier (1.0 - 2.0 range)
        let playerLevel = MultiplayerDialog.GetPlayerLevel(playerId);
        let maxLevel = 50.0; // Cyberpunk 2077 max level
        let levelRatio = Cast<Float>(playerLevel) / maxLevel;

        // Level contributes 0% to 50% bonus
        return 1.0 + (levelRatio * 0.5);
    }

    private static func GetPlayerContributionMultiplier(playerId: Uint32) -> Float {
        // Get player's contribution to current quest/session
        let contribution = MultiplayerDialog.GetPlayerQuestContribution(playerId);

        // Contribution ranges from 0.8 to 1.5 multiplier
        return 0.8 + (contribution * 0.7);
    }

    private static func GetPlayerReputationMultiplier(playerId: Uint32) -> Float {
        // Get player's overall reputation/karma score
        let reputation = MultiplayerDialog.GetPlayerReputation(playerId);

        // Reputation ranges from 0.5 to 1.2 multiplier
        return 0.5 + (reputation * 0.7);
    }

    private static func GetPlayerLevel(playerId: Uint32) -> Uint32 {
        // Get player level from game system
        // This would integrate with the actual player stats
        return 25u; // Placeholder - would get actual level
    }

    private static func GetPlayerQuestContribution(playerId: Uint32) -> Float {
        // Calculate player's contribution to current quest (0.0 - 1.0)
        // Based on kills, objectives completed, items found, etc.
        return 0.5; // Placeholder - would calculate actual contribution
    }

    private static func GetPlayerReputation(playerId: Uint32) -> Float {
        // Get player's reputation score (0.0 - 1.0)
        // Based on past behavior, team play, etc.
        return 0.7; // Placeholder - would get actual reputation
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
        // Implement proper JSON serialization
        let json = "{";
        json += "\"sessionId\":\"" + MultiplayerDialog.EscapeJson(session.sessionId) + "\",";
        json += "\"questId\":\"" + MultiplayerDialog.EscapeJson(session.questId) + "\",";
        json += "\"dialogNodeId\":\"" + MultiplayerDialog.EscapeJson(session.dialogNodeId) + "\",";
        json += "\"voteType\":" + ToString(Cast<Int32>(session.voteType)) + ",";
        json += "\"timeoutDuration\":" + ToString(session.timeoutDuration) + ",";
        json += "\"isStoryImportant\":" + (session.isStoryImportant ? "true" : "false") + ",";
        json += "\"startTime\":" + ToString(session.startTime) + ",";
        json += "\"isActive\":" + (session.isActive ? "true" : "false") + ",";
        json += "\"playerCount\":" + ToString(ArraySize(session.players)) + ",";
        json += "\"choiceCount\":" + ToString(ArraySize(session.choices)) + ",";
        json += "\"players\":[";

        for i in Range(ArraySize(session.players)) {
            if i > 0 { json += ","; }
            json += "\"" + MultiplayerDialog.EscapeJson(session.players[i]) + "\"";
        }

        json += "],";
        json += "\"choices\":[" + MultiplayerDialog.SerializeChoicesJson(session.choices) + "]";
        json += "}";

        return json;
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
        let session = MultiplayerDialog.DeserializeSession(sessionData);
        if IsDefined(session) {
            MultiplayerDialog.currentSession = session;
            LogChannel(n"COOP_DIALOG", s"[OnDialogSessionStarted] Session \(session.sessionId) initialized");

            // Set up local client for this session
            MultiplayerDialog.SetupClientSession(session);
        } else {
            LogChannel(n"COOP_DIALOG", "[OnDialogSessionStarted] Failed to deserialize session data");
        }
    }

    private static cb func OnDialogChoicePresented(choiceData: String) -> Void {
        LogChannel(n"COOP_DIALOG", "Received dialog choices: " + choiceData);

        // Parse and display choices to client
        let choices = MultiplayerDialog.DeserializeChoices(choiceData);
        if ArraySize(choices) > 0 {
            LogChannel(n"COOP_DIALOG", s"[OnDialogChoicePresented] Received \(ArraySize(choices)) choices");

            // Update current session with choices
            if IsDefined(MultiplayerDialog.currentSession) {
                MultiplayerDialog.currentSession.choices = choices;

                // Display choices to player
                MultiplayerDialog.DisplayChoicesToPlayer(choices);
            }
        } else {
            LogChannel(n"COOP_DIALOG", "[OnDialogChoicePresented] Failed to deserialize choices");
        }
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

    // JSON serialization utility functions
    private static func SerializeChoicesJson(choices: array<DialogChoice>) -> String {
        let json = "";
        for i in Range(ArraySize(choices)) {
            if i > 0 { json += ","; }
            json += "{";
            json += "\"choiceId\":\"" + MultiplayerDialog.EscapeJson(choices[i].choiceId) + "\",";
            json += "\"text\":\"" + MultiplayerDialog.EscapeJson(choices[i].text) + "\",";
            json += "\"speaker\":\"" + MultiplayerDialog.EscapeJson(choices[i].speaker) + "\",";
            json += "\"isAvailable\":" + (choices[i].isAvailable ? "true" : "false") + ",";
            json += "\"requiresCheck\":" + (choices[i].requiresCheck ? "true" : "false") + ",";
            json += "\"checkType\":\"" + MultiplayerDialog.EscapeJson(choices[i].checkType) + "\",";
            json += "\"difficulty\":" + ToString(choices[i].difficulty) + ",";
            json += "\"voteCount\":" + ToString(ArraySize(choices[i].votes));
            json += "}";
        }
        return json;
    }

    private static func EscapeJson(text: String) -> String {
        // Escape special JSON characters
        let escaped = StrReplace(text, "\\", "\\\\");
        escaped = StrReplace(escaped, "\"", "\\\"");
        escaped = StrReplace(escaped, "\n", "\\n");
        escaped = StrReplace(escaped, "\r", "\\r");
        escaped = StrReplace(escaped, "\t", "\\t");
        return escaped;
    }

    private static func DeserializeSession(jsonData: String) -> ref<DialogSession> {
        // Parse JSON session data
        let session = new DialogSession();

        // Extract session ID
        session.sessionId = MultiplayerDialog.ExtractJsonString(jsonData, "sessionId");
        session.questId = MultiplayerDialog.ExtractJsonString(jsonData, "questId");
        session.dialogNodeId = MultiplayerDialog.ExtractJsonString(jsonData, "dialogNodeId");

        // Extract numeric values
        let voteTypeInt = MultiplayerDialog.ExtractJsonInt(jsonData, "voteType");
        session.voteType = IntEnum<DialogVoteType>(voteTypeInt);
        session.timeoutDuration = MultiplayerDialog.ExtractJsonFloat(jsonData, "timeoutDuration");
        session.startTime = MultiplayerDialog.ExtractJsonFloat(jsonData, "startTime");

        // Extract boolean values
        session.isStoryImportant = MultiplayerDialog.ExtractJsonBool(jsonData, "isStoryImportant");
        session.isActive = MultiplayerDialog.ExtractJsonBool(jsonData, "isActive");

        LogChannel(n"COOP_DIALOG", s"[DeserializeSession] Parsed session: \(session.sessionId)");
        return session;
    }

    private static func DeserializeChoices(jsonData: String) -> array<DialogChoice> {
        let choices: array<DialogChoice>;

        // Extract choices array from JSON
        let choicesJsonArray = MultiplayerDialog.ExtractJsonArray(jsonData, "choices");
        if !Equals(choicesJsonArray, "") {
            // Parse each choice - simplified parsing for now
            let choiceStrings = StrSplit(choicesJsonArray, "},{");

            for choiceStr in choiceStrings {
                let choice = new DialogChoice();
                choice.choiceId = MultiplayerDialog.ExtractJsonString(choiceStr, "choiceId");
                choice.text = MultiplayerDialog.ExtractJsonString(choiceStr, "text");
                choice.speaker = MultiplayerDialog.ExtractJsonString(choiceStr, "speaker");
                choice.isAvailable = MultiplayerDialog.ExtractJsonBool(choiceStr, "isAvailable");
                choice.requiresCheck = MultiplayerDialog.ExtractJsonBool(choiceStr, "requiresCheck");
                choice.checkType = MultiplayerDialog.ExtractJsonString(choiceStr, "checkType");
                choice.difficulty = MultiplayerDialog.ExtractJsonInt(choiceStr, "difficulty");

                ArrayPush(choices, choice);
            }
        }

        LogChannel(n"COOP_DIALOG", s"[DeserializeChoices] Parsed \(ArraySize(choices)) choices");
        return choices;
    }

    // Simple JSON extraction utilities
    private static func ExtractJsonString(json: String, key: String) -> String {
        let pattern = "\"" + key + "\":\"";
        let startIndex = StrFindFirst(json, pattern);
        if startIndex >= 0 {
            startIndex += StrLen(pattern);
            let endIndex = StrFindFirst(json, "\"", startIndex);
            if endIndex > startIndex {
                return StrMid(json, startIndex, endIndex - startIndex);
            }
        }
        return "";
    }

    private static func ExtractJsonInt(json: String, key: String) -> Int32 {
        let pattern = "\"" + key + "\":";
        let startIndex = StrFindFirst(json, pattern);
        if startIndex >= 0 {
            startIndex += StrLen(pattern);
            let endIndex = StrFindFirst(json, ",", startIndex);
            if endIndex < 0 {
                endIndex = StrFindFirst(json, "}", startIndex);
            }
            if endIndex > startIndex {
                let numStr = StrMid(json, startIndex, endIndex - startIndex);
                return StringToInt(numStr);
            }
        }
        return 0;
    }

    private static func ExtractJsonFloat(json: String, key: String) -> Float {
        let intVal = MultiplayerDialog.ExtractJsonInt(json, key);
        return Cast<Float>(intVal);
    }

    private static func ExtractJsonBool(json: String, key: String) -> Bool {
        let pattern = "\"" + key + "\":";
        let startIndex = StrFindFirst(json, pattern);
        if startIndex >= 0 {
            startIndex += StrLen(pattern);
            return StrFindFirst(json, "true", startIndex) == startIndex;
        }
        return false;
    }

    private static func ExtractJsonArray(json: String, key: String) -> String {
        let pattern = "\"" + key + "\":[";
        let startIndex = StrFindFirst(json, pattern);
        if startIndex >= 0 {
            startIndex += StrLen(pattern);
            let depth = 1;
            let i = startIndex;
            while depth > 0 && i < StrLen(json) {
                let char = StrMid(json, i, 1);
                if Equals(char, "[") {
                    depth += 1;
                } else if Equals(char, "]") {
                    depth -= 1;
                }
                i += 1;
            }
            if depth == 0 {
                return StrMid(json, startIndex, i - startIndex - 1);
            }
        }
        return "";
    }

    // Client session setup functions
    private static func SetupClientSession(session: ref<DialogSession>) -> Void {
        LogChannel(n"COOP_DIALOG", s"[SetupClientSession] Setting up client for session \(session.sessionId)");

        // Initialize client-side voting system
        // Set up UI elements
        // Register for vote events
    }

    private static func DisplayChoicesToPlayer(choices: array<DialogChoice>) -> Void {
        LogChannel(n"COOP_DIALOG", s"[DisplayChoicesToPlayer] Displaying \(ArraySize(choices)) choices to player");

        // Display choices in dialog UI
        if IsDefined(dialogUI) {
            dialogUI.ShowChoices(choices);
        }
    }
}

// Supporting data structure for weighted voting
public class WeightedChoice extends IScriptable {
    public let choice: DialogChoice;
    public let totalWeight: Float;
}