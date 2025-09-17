import Codeware.UI

public enum DialogUIState {
    Hidden = 0,
    WaitingForSession = 1,
    ShowingChoices = 2,
    VotingInProgress = 3,
    ShowingResult = 4
}

public class DialogSessionUI extends inkGameController {
    private var m_rootContainer: wref<inkWidget>;
    private var m_backgroundPanel: wref<inkRectangle>;
    private var m_titleText: wref<inkText>;
    private var m_descriptionText: wref<inkText>;
    private var m_choicesContainer: wref<inkVerticalPanel>;
    private var m_voteStatusContainer: wref<inkHorizontalPanel>;
    private var m_timerBar: wref<inkRectangle>;
    private var m_timerText: wref<inkText>;
    private var m_resultContainer: wref<inkVerticalPanel>;
    private var m_playerVotesContainer: wref<inkVerticalPanel>;
    
    private var m_currentSession: DialogSession;
    private var m_uiState: DialogUIState;
    private var m_selectedChoiceId: String;
    private var m_voteStartTime: Float;
    private var m_voteDuration: Float;
    private var m_choiceButtons: array<wref<inkButton>>;
    private var m_isVisible: Bool;
    
    public func Initialize() -> Void {
        this.CreateUI();
        this.m_uiState = DialogUIState.Hidden;
        this.m_isVisible = false;
        this.m_selectedChoiceId = "";
        
        LogChannel(n"COOP_DIALOG", "Dialog UI initialized");
    }
    
    private func CreateUI() -> Void {
        let rootCompound = this.GetRootCompoundWidget();
        
        // Main container
        this.m_rootContainer = new inkCanvas();
        this.m_rootContainer.SetName(n"DialogSessionRoot");
        this.m_rootContainer.SetSize(new Vector2(1920.0, 1080.0));
        this.m_rootContainer.SetAnchor(inkEAnchor.Fill);
        this.m_rootContainer.SetVisible(false);
        rootCompound.AddChild(this.m_rootContainer);
        
        // Semi-transparent background
        this.m_backgroundPanel = new inkRectangle();
        this.m_backgroundPanel.SetName(n"DialogBackground");
        this.m_backgroundPanel.SetSize(new Vector2(800.0, 600.0));
        this.m_backgroundPanel.SetAnchor(inkEAnchor.CenterCenter);
        this.m_backgroundPanel.SetMargin(new inkMargin(0.0, 0.0, 0.0, 0.0));
        this.m_backgroundPanel.SetTintColor(new HDRColor(0.0, 0.0, 0.0, 0.85));
        this.m_backgroundPanel.SetOpacity(0.9);
        this.m_rootContainer.AddChild(this.m_backgroundPanel);
        
        // Content container
        let contentPanel = new inkVerticalPanel();
        contentPanel.SetName(n"DialogContent");
        contentPanel.SetSize(new Vector2(780.0, 580.0));
        contentPanel.SetAnchor(inkEAnchor.CenterCenter);
        contentPanel.SetMargin(new inkMargin(10.0, 10.0, 10.0, 10.0));
        this.m_rootContainer.AddChild(contentPanel);
        
        // Title
        this.m_titleText = new inkText();
        this.m_titleText.SetName(n"DialogTitle");
        this.m_titleText.SetText("💬 MULTIPLAYER DIALOG");
        this.m_titleText.SetFontFamily(n"base\\gameplay\\gui\\fonts\\raj\\raj.inkfontfamily");
        this.m_titleText.SetFontSize(32);
        this.m_titleText.SetTintColor(new HDRColor(0.368627, 0.964706, 1.0, 1.0)); // CP2077 cyan
        this.m_titleText.SetHorizontalAlignment(textHorizontalAlignment.Center);
        this.m_titleText.SetVerticalAlignment(textVerticalAlignment.Center);
        this.m_titleText.SetMargin(new inkMargin(0.0, 0.0, 0.0, 20.0));
        contentPanel.AddChild(this.m_titleText);
        
        // Description
        this.m_descriptionText = new inkText();
        this.m_descriptionText.SetName(n"DialogDescription");
        this.m_descriptionText.SetText("Choose your response. All players will vote on the final decision.");
        this.m_descriptionText.SetFontFamily(n"base\\gameplay\\gui\\fonts\\raj\\raj.inkfontfamily");
        this.m_descriptionText.SetFontSize(18);
        this.m_descriptionText.SetTintColor(new HDRColor(0.8, 0.8, 0.8, 1.0));
        this.m_descriptionText.SetHorizontalAlignment(textHorizontalAlignment.Center);
        this.m_descriptionText.SetVerticalAlignment(textVerticalAlignment.Center);
        this.m_descriptionText.SetMargin(new inkMargin(0.0, 0.0, 0.0, 30.0));
        this.m_descriptionText.SetWrapping(true);
        this.m_descriptionText.SetOverflowPolicy(textOverflowPolicy.DotsEnd);
        contentPanel.AddChild(this.m_descriptionText);
        
        // Choices container
        this.m_choicesContainer = new inkVerticalPanel();
        this.m_choicesContainer.SetName(n"DialogChoices");
        this.m_choicesContainer.SetSize(new Vector2(780.0, 300.0));
        this.m_choicesContainer.SetMargin(new inkMargin(0.0, 0.0, 0.0, 20.0));
        contentPanel.AddChild(this.m_choicesContainer);
        
        // Vote status container
        this.m_voteStatusContainer = new inkHorizontalPanel();
        this.m_voteStatusContainer.SetName(n"VoteStatus");
        this.m_voteStatusContainer.SetSize(new Vector2(780.0, 40.0));
        this.m_voteStatusContainer.SetMargin(new inkMargin(0.0, 0.0, 0.0, 10.0));
        this.m_voteStatusContainer.SetVisible(false);
        contentPanel.AddChild(this.m_voteStatusContainer);
        
        // Timer bar
        this.m_timerBar = new inkRectangle();
        this.m_timerBar.SetName(n"TimerBar");
        this.m_timerBar.SetSize(new Vector2(600.0, 8.0));
        this.m_timerBar.SetTintColor(new HDRColor(0.368627, 0.964706, 1.0, 1.0));
        this.m_timerBar.SetAnchor(inkEAnchor.CenterLeft);
        this.m_voteStatusContainer.AddChild(this.m_timerBar);
        
        // Timer text
        this.m_timerText = new inkText();
        this.m_timerText.SetName(n"TimerText");
        this.m_timerText.SetText("30s");
        this.m_timerText.SetFontFamily(n"base\\gameplay\\gui\\fonts\\raj\\raj.inkfontfamily");
        this.m_timerText.SetFontSize(16);
        this.m_timerText.SetTintColor(new HDRColor(1.0, 1.0, 1.0, 1.0));
        this.m_timerText.SetHorizontalAlignment(textHorizontalAlignment.Center);
        this.m_timerText.SetVerticalAlignment(textVerticalAlignment.Center);
        this.m_timerText.SetMargin(new inkMargin(20.0, 0.0, 0.0, 0.0));
        this.m_voteStatusContainer.AddChild(this.m_timerText);
        
        // Player votes container
        this.m_playerVotesContainer = new inkVerticalPanel();
        this.m_playerVotesContainer.SetName(n"PlayerVotes");
        this.m_playerVotesContainer.SetSize(new Vector2(780.0, 100.0));
        this.m_playerVotesContainer.SetVisible(false);
        contentPanel.AddChild(this.m_playerVotesContainer);
        
        // Result container
        this.m_resultContainer = new inkVerticalPanel();
        this.m_resultContainer.SetName(n"DialogResult");
        this.m_resultContainer.SetSize(new Vector2(780.0, 200.0));
        this.m_resultContainer.SetVisible(false);
        contentPanel.AddChild(this.m_resultContainer);
    }
    
    public func Show(session: DialogSession) -> Void {
        this.m_currentSession = session;
        this.m_uiState = DialogUIState.WaitingForSession;
        this.m_isVisible = true;
        this.m_selectedChoiceId = "";
        
        this.m_rootContainer.SetVisible(true);
        this.UpdateTitleForSession();
        this.ClearChoices();
        this.HideVoteStatus();
        this.HideResult();
        
        LogChannel(n"COOP_DIALOG", "Dialog UI shown for session: " + session.sessionId);
    }
    
    public func Hide() -> Void {
        this.m_uiState = DialogUIState.Hidden;
        this.m_isVisible = false;
        this.m_rootContainer.SetVisible(false);
        this.ClearChoices();
        
        LogChannel(n"COOP_DIALOG", "Dialog UI hidden");
    }
    
    public func PresentChoices(choices: array<DialogChoice>) -> Void {
        this.m_uiState = DialogUIState.ShowingChoices;
        this.ClearChoices();
        
        this.m_descriptionText.SetText("Choose your response. Voting will begin shortly...");
        
        let choiceIndex = 0;
        for choice in choices {
            if choice.isAvailable {
                this.CreateChoiceButton(choice, choiceIndex);
                choiceIndex += 1;
            }
        }
        
        LogChannel(n"COOP_DIALOG", "Presented " + ToString(ArraySize(choices)) + " dialog choices");
    }
    
    public func StartVoting(duration: Float) -> Void {
        this.m_uiState = DialogUIState.VotingInProgress;
        this.m_voteStartTime = GetGameTime();
        this.m_voteDuration = duration;
        
        this.m_descriptionText.SetText("🗳️ Voting in progress... Select your choice!");
        this.ShowVoteStatus();
        this.ShowPlayerVotes();
        
        // Enable choice buttons for voting
        for button in this.m_choiceButtons {
            button.SetDisabled(false);
        }
        
        LogChannel(n"COOP_DIALOG", "Voting started with " + ToString(duration) + " second duration");
    }
    
    public func ShowResult(winningChoice: DialogChoice, displayDuration: Float) -> Void {
        this.m_uiState = DialogUIState.ShowingResult;
        
        this.ClearChoices();
        this.HideVoteStatus();
        this.HidePlayerVotes();
        this.ShowResult(winningChoice);
        
        this.m_descriptionText.SetText("✅ Decision made! Continuing with selected choice...");
        
        // Auto-hide after duration
        DelaySystem.DelayCallback(this.Hide, displayDuration);
        
        LogChannel(n"COOP_DIALOG", "Showing result: " + winningChoice.text);
    }
    
    private func CreateChoiceButton(choice: DialogChoice, index: Int32) -> Void {
        let choiceBtn = new inkButton();
        choiceBtn.SetName(StringToName("Choice_" + choice.choiceId));
        choiceBtn.SetText(ToString(index + 1) + ". " + choice.text);
        choiceBtn.SetStyle(n"BaseButtonLarge");
        choiceBtn.SetSize(new Vector2(760.0, 60.0));
        choiceBtn.SetMargin(new inkMargin(10.0, 5.0, 10.0, 5.0));
        choiceBtn.SetDisabled(true); // Will be enabled during voting
        
        // Style the button
        let buttonController = choiceBtn.GetController() as inkButtonController;
        if IsDefined(buttonController) {
            buttonController.SetHoverTintColor(new HDRColor(0.368627, 0.964706, 1.0, 0.3));
            buttonController.SetPressTintColor(new HDRColor(0.368627, 0.964706, 1.0, 0.5));
        }
        
        // Add choice requirements/speaker info if available
        if !Equals(choice.speaker, "") {
            let speakerText = " [" + choice.speaker + "]";
            choiceBtn.SetText(choiceBtn.GetText() + speakerText);
        }
        
        if ArraySize(choice.requirements) > 0 {
            let reqText = " 🔒";
            choiceBtn.SetText(choiceBtn.GetText() + reqText);
        }
        
        choiceBtn.RegisterToCallback(n"OnRelease", this, n"OnChoiceSelected");
        choiceBtn.SetUserData(choice.choiceId);
        
        this.m_choicesContainer.AddChild(choiceBtn);
        ArrayPush(this.m_choiceButtons, choiceBtn);
    }
    
    protected cb func OnChoiceSelected(e: ref<inkPointerEvent>) -> Void {
        let button = e.GetTarget() as inkButton;
        if !IsDefined(button) {
            return;
        }
        
        let choiceId = button.GetUserData() as String;
        if Equals(choiceId, "") {
            return;
        }
        
        // Prevent double-voting
        if !Equals(this.m_selectedChoiceId, "") {
            return;
        }
        
        this.m_selectedChoiceId = choiceId;
        
        // Visual feedback for selected choice
        this.HighlightSelectedChoice(button);
        
        // Submit vote through dialog system
        let localPlayerId = NetworkingSystem.GetLocalPlayerId();
        MultiplayerDialog.SubmitVote(localPlayerId, choiceId);
        
        // Play selection sound
        let audioSystem = GameInstance.GetAudioSystem(GetGame());
        if IsDefined(audioSystem) {
            let audioEvent = new AudioEvent();
            audioEvent.eventName = n"ui_menu_onpress";
            audioSystem.Play(audioEvent);
        }
        
        LogChannel(n"COOP_DIALOG", "Player selected choice: " + choiceId);
    }
    
    private func HighlightSelectedChoice(selectedButton: wref<inkButton>) -> Void {
        // Reset all buttons to normal state
        for button in this.m_choiceButtons {
            button.SetTintColor(new HDRColor(1.0, 1.0, 1.0, 1.0));
            button.SetDisabled(false);
        }
        
        // Highlight selected button
        selectedButton.SetTintColor(new HDRColor(0.368627, 0.964706, 1.0, 1.0));
        selectedButton.SetDisabled(true);
    }
    
    public func UpdateVoteStatus(progress: String) -> Void {
        if this.m_uiState != DialogUIState.VotingInProgress {
            return;
        }
        
        // Update timer
        let elapsed = GetGameTime() - this.m_voteStartTime;
        let remaining = this.m_voteDuration - elapsed;
        
        if remaining > 0.0 {
            this.m_timerText.SetText(ToString(Cast<Int32>(remaining)) + "s");
            
            // Update timer bar
            let progress = 1.0 - (remaining / this.m_voteDuration);
            let newWidth = 600.0 * progress;
            this.m_timerBar.SetSize(new Vector2(newWidth, 8.0));
        } else {
            this.m_timerText.SetText("0s");
            this.m_timerBar.SetSize(new Vector2(600.0, 8.0));
            this.m_timerBar.SetTintColor(new HDRColor(1.0, 0.5, 0.0, 1.0)); // Orange when time's up
        }
    }
    
    public func UpdatePlayerVote(playerId: String, choiceId: String) -> Void {
        // Find existing player vote display or create new one
        let playerVoteText = this.FindPlayerVoteWidget(playerId);
        if !IsDefined(playerVoteText) {
            playerVoteText = this.CreatePlayerVoteWidget(playerId);
        }
        
        // Update vote display
        let playerName = NetworkingSystem.GetPlayerName(playerId);
        let choiceText = this.GetChoiceText(choiceId);
        playerVoteText.SetText("👤 " + playerName + " voted for: " + choiceText);
        playerVoteText.SetTintColor(new HDRColor(0.0, 1.0, 0.0, 1.0)); // Green for voted
    }
    
    private func FindPlayerVoteWidget(playerId: String) -> wref<inkText> {
        let widgetName = StringToName("PlayerVote_" + playerId);
        return this.m_playerVotesContainer.GetWidget(widgetName) as inkText;
    }
    
    private func CreatePlayerVoteWidget(playerId: String) -> wref<inkText> {
        let playerVoteText = new inkText();
        playerVoteText.SetName(StringToName("PlayerVote_" + playerId));
        playerVoteText.SetText("👤 " + NetworkingSystem.GetPlayerName(playerId) + " - waiting...");
        playerVoteText.SetFontFamily(n"base\\gameplay\\gui\\fonts\\raj\\raj.inkfontfamily");
        playerVoteText.SetFontSize(14);
        playerVoteText.SetTintColor(new HDRColor(0.6, 0.6, 0.6, 1.0)); // Gray for waiting
        playerVoteText.SetMargin(new inkMargin(10.0, 2.0, 10.0, 2.0));
        this.m_playerVotesContainer.AddChild(playerVoteText);
        return playerVoteText;
    }
    
    private func GetChoiceText(choiceId: String) -> String {
        for choice in this.m_currentSession.choices {
            if Equals(choice.choiceId, choiceId) {
                return choice.text;
            }
        }
        return "Unknown Choice";
    }
    
    private func ShowResult(winningChoice: DialogChoice) -> Void {
        this.m_resultContainer.SetVisible(true);
        
        // Clear previous result content
        this.m_resultContainer.RemoveAllChildren();
        
        // Result title
        let resultTitle = new inkText();
        resultTitle.SetName(n"ResultTitle");
        resultTitle.SetText("🎯 CHOSEN RESPONSE");
        resultTitle.SetFontFamily(n"base\\gameplay\\gui\\fonts\\raj\\raj.inkfontfamily");
        resultTitle.SetFontSize(24);
        resultTitle.SetTintColor(new HDRColor(0.0, 1.0, 0.0, 1.0)); // Green
        resultTitle.SetHorizontalAlignment(textHorizontalAlignment.Center);
        resultTitle.SetMargin(new inkMargin(0.0, 0.0, 0.0, 15.0));
        this.m_resultContainer.AddChild(resultTitle);
        
        // Result text
        let resultText = new inkText();
        resultText.SetName(n"ResultText");
        resultText.SetText("\"" + winningChoice.text + "\"");
        resultText.SetFontFamily(n"base\\gameplay\\gui\\fonts\\raj\\raj.inkfontfamily");
        resultText.SetFontSize(18);
        resultText.SetTintColor(new HDRColor(1.0, 1.0, 1.0, 1.0));
        resultText.SetHorizontalAlignment(textHorizontalAlignment.Center);
        resultText.SetVerticalAlignment(textVerticalAlignment.Center);
        resultText.SetWrapping(true);
        resultText.SetMargin(new inkMargin(20.0, 10.0, 20.0, 15.0));
        this.m_resultContainer.AddChild(resultText);
        
        // Speaker info if available
        if !Equals(winningChoice.speaker, "") {
            let speakerText = new inkText();
            speakerText.SetName(n"SpeakerText");
            speakerText.SetText("- " + winningChoice.speaker);
            speakerText.SetFontFamily(n"base\\gameplay\\gui\\fonts\\raj\\raj.inkfontfamily");
            speakerText.SetFontSize(14);
            speakerText.SetTintColor(new HDRColor(0.368627, 0.964706, 1.0, 1.0));
            speakerText.SetHorizontalAlignment(textHorizontalAlignment.Center);
            speakerText.SetMargin(new inkMargin(0.0, 0.0, 0.0, 10.0));
            this.m_resultContainer.AddChild(speakerText);
        }
    }
    
    private func UpdateTitleForSession() -> Void {
        let title = "💬 MULTIPLAYER DIALOG";
        if this.m_currentSession.isStoryImportant {
            title = "⭐ IMPORTANT STORY CHOICE";
        }
        this.m_titleText.SetText(title);
    }
    
    private func ClearChoices() -> Void {
        this.m_choicesContainer.RemoveAllChildren();
        ArrayClear(this.m_choiceButtons);
        this.m_selectedChoiceId = "";
    }
    
    private func ShowVoteStatus() -> Void {
        this.m_voteStatusContainer.SetVisible(true);
    }
    
    private func HideVoteStatus() -> Void {
        this.m_voteStatusContainer.SetVisible(false);
    }
    
    private func ShowPlayerVotes() -> Void {
        this.m_playerVotesContainer.SetVisible(true);
        
        // Create initial player vote widgets
        for participantId in this.m_currentSession.participants {
            this.CreatePlayerVoteWidget(participantId);
        }
    }
    
    private func HidePlayerVotes() -> Void {
        this.m_playerVotesContainer.SetVisible(false);
        this.m_playerVotesContainer.RemoveAllChildren();
    }
    
    private func HideResult() -> Void {
        this.m_resultContainer.SetVisible(false);
        this.m_resultContainer.RemoveAllChildren();
    }
    
    public func IsVisible() -> Bool {
        return this.m_isVisible;
    }
    
    public func GetCurrentState() -> DialogUIState {
        return this.m_uiState;
    }
}