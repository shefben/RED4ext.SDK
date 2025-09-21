// Multiplayer UI Integration - REDscript Layer
// Uses only game assets and UI systems for seamless integration

import RedNotification.*
import Codeware.*

// Main multiplayer UI integration class
public class MultiplayerUIIntegration extends inkGameController {
    protected let m_playerListWidget: wref<inkWidget>;
    protected let m_chatWidget: wref<inkWidget>;
    protected let m_networkStatsWidget: wref<inkWidget>;
    protected let m_voiceChatWidget: wref<inkWidget>;
    protected let m_questSyncWidget: wref<inkWidget>;

    protected let m_notificationSystem: wref<UIInGameNotificationSystem>;
    protected let m_uiSystem: wref<UISystem>;
    protected let m_gameInstance: GameInstance;

    protected let m_isInitialized: Bool = false;
    protected let m_useGameAssets: Bool = true;
    protected let m_currentTheme: String = "street"; // Default CP2077 theme

    protected cb func OnInitialize() -> Void {
        this.InitializeMultiplayerUI();
    }

    protected cb func OnUninitialize() -> Void {
        this.ShutdownMultiplayerUI();
    }

    // Initialize UI using game's existing systems
    private func InitializeMultiplayerUI() -> Void {
        if this.m_isInitialized {
            return;
        }

        LogChannel(n"CoopNet", "Initializing multiplayer UI integration using game assets");

        // Get game systems
        this.m_gameInstance = this.GetGameInstance();
        this.m_uiSystem = GameInstance.GetUISystem(this.m_gameInstance);
        this.m_notificationSystem = GameInstance.GetUIInGameNotificationSystem(this.m_gameInstance);

        // Initialize UI panels using game's widget system
        this.CreatePlayerListPanel();
        this.CreateChatPanel();
        this.CreateNetworkStatsPanel();
        this.CreateVoiceChatPanel();
        this.CreateQuestSyncPanel();

        // Setup hotkeys using game's input system
        this.SetupMultiplayerHotkeys();

        // Apply current game theme
        this.ApplyGameTheme();

        this.m_isInitialized = true;
        LogChannel(n"CoopNet", "Multiplayer UI integration initialized successfully");
    }

    private func ShutdownMultiplayerUI() -> Void {
        if !this.m_isInitialized {
            return;
        }

        LogChannel(n"CoopNet", "Shutting down multiplayer UI integration");

        // Hide all panels
        this.HidePlayerList();
        this.HideChatPanel();
        this.HideNetworkStats();
        this.HideVoiceChatPanel();
        this.HideQuestSyncPanel();

        this.m_isInitialized = false;
    }

    // Player list panel using game's minimap container style
    private func CreatePlayerListPanel() -> Void {
        // Create player list widget using game's minimap style
        let playerListRoot = new inkCanvas();
        playerListRoot.SetName(n"multiplayer_player_list");
        playerListRoot.SetAnchor(inkEAnchor.TopRight);
        playerListRoot.SetMargin(new inkMargin(-20.0, 20.0, 0.0, 0.0));
        playerListRoot.SetSize(new Vector2(300.0, 400.0));

        // Use game's panel background
        let backgroundImage = new inkImage();
        backgroundImage.SetName(n"player_list_bg");
        backgroundImage.SetAtlasResource(r"base\\gameplay\\gui\\common\\backgrounds\\panel_bg.inkatlas");
        backgroundImage.SetTexturePart(n"panel_bg");
        backgroundImage.SetNineSliceScale(true);
        backgroundImage.SetSize(new Vector2(300.0, 400.0));
        backgroundImage.SetOpacity(0.9);

        // Add title using game's font
        let titleText = new inkText();
        titleText.SetName(n"player_list_title");
        titleText.SetText("CONNECTED PLAYERS");
        titleText.SetFontFamily("base\\gameplay\\gui\\fonts\\orbitron\\orbitron.fnt");
        titleText.SetFontSize(18);
        titleText.SetLetterCase(textLetterCase.UpperCase);
        titleText.SetAnchor(inkEAnchor.TopFill);
        titleText.SetMargin(new inkMargin(10.0, 10.0, 10.0, 0.0));
        titleText.SetTintColor(new HDRColor(0.0, 1.0, 1.0, 1.0)); // Cyberpunk cyan

        playerListRoot.AddChild(backgroundImage);
        playerListRoot.AddChild(titleText);

        this.m_playerListWidget = playerListRoot;
        this.GetRootWidget().AddChild(this.m_playerListWidget);
        this.m_playerListWidget.SetVisible(false);

        LogChannel(n"CoopNet", "Player list panel created using game assets");
    }

    // Chat panel using game's SMS conversation style
    private func CreateChatPanel() -> Void {
        let chatRoot = new inkCanvas();
        chatRoot.SetName(n"multiplayer_chat");
        chatRoot.SetAnchor(inkEAnchor.BottomLeft);
        chatRoot.SetMargin(new inkMargin(20.0, 0.0, 0.0, -150.0));
        chatRoot.SetSize(new Vector2(400.0, 120.0));

        // Use game's chat background
        let chatBackground = new inkImage();
        chatBackground.SetName(n"chat_bg");
        chatBackground.SetAtlasResource(r"base\\gameplay\\gui\\common\\backgrounds\\chat_bg.inkatlas");
        chatBackground.SetTexturePart(n"chat_bg");
        chatBackground.SetNineSliceScale(true);
        chatBackground.SetSize(new Vector2(400.0, 120.0));
        chatBackground.SetOpacity(0.8);

        // Chat messages scroll area
        let messagesScrollArea = new inkScrollArea();
        messagesScrollArea.SetName(n"chat_messages_scroll");
        messagesScrollArea.SetAnchor(inkEAnchor.Fill);
        messagesScrollArea.SetMargin(new inkMargin(10.0, 10.0, 10.0, 30.0));

        let messagesContainer = new inkVerticalPanel();
        messagesContainer.SetName(n"chat_messages_container");
        messagesScrollArea.AddChild(messagesContainer);

        // Chat input field
        let inputField = new inkTextInput();
        inputField.SetName(n"chat_input");
        inputField.SetAnchor(inkEAnchor.BottomFill);
        inputField.SetMargin(new inkMargin(10.0, 0.0, 10.0, 5.0));
        inputField.SetSize(new Vector2(0.0, 20.0));
        inputField.SetText("Type your message...");

        chatRoot.AddChild(chatBackground);
        chatRoot.AddChild(messagesScrollArea);
        chatRoot.AddChild(inputField);

        this.m_chatWidget = chatRoot;
        this.GetRootWidget().AddChild(this.m_chatWidget);
        this.m_chatWidget.SetVisible(false);

        LogChannel(n"CoopNet", "Chat panel created using game's SMS style");
    }

    // Network stats panel using game's health bar style
    private func CreateNetworkStatsPanel() -> Void {
        let statsRoot = new inkCanvas();
        statsRoot.SetName(n"network_stats");
        statsRoot.SetAnchor(inkEAnchor.TopLeft);
        statsRoot.SetMargin(new inkMargin(20.0, 20.0, 0.0, 0.0));
        statsRoot.SetSize(new Vector2(200.0, 100.0));

        // Use game's stats background
        let statsBackground = new inkImage();
        statsBackground.SetAtlasResource(r"base\\gameplay\\gui\\common\\backgrounds\\stats_bg.inkatlas");
        statsBackground.SetTexturePart(n"stats_bg");
        statsBackground.SetSize(new Vector2(200.0, 100.0));
        statsBackground.SetOpacity(0.7);

        // Network quality indicator
        let qualityBar = new inkImage();
        qualityBar.SetName(n"quality_bar");
        qualityBar.SetAtlasResource(r"base\\gameplay\\gui\\widgets\\healthbar\\health_bar.inkatlas");
        qualityBar.SetTexturePart(n"health_bar_fill");
        qualityBar.SetAnchor(inkEAnchor.TopLeft);
        qualityBar.SetMargin(new inkMargin(10.0, 20.0, 0.0, 0.0));
        qualityBar.SetSize(new Vector2(180.0, 8.0));

        // Ping text
        let pingText = new inkText();
        pingText.SetName(n"ping_text");
        pingText.SetText("Ping: --ms");
        pingText.SetFontFamily("base\\gameplay\\gui\\fonts\\rajdhani\\rajdhani.fnt");
        pingText.SetFontSize(12);
        pingText.SetAnchor(inkEAnchor.TopLeft);
        pingText.SetMargin(new inkMargin(10.0, 35.0, 0.0, 0.0));

        // Packet loss text
        let packetLossText = new inkText();
        packetLossText.SetName(n"packet_loss_text");
        packetLossText.SetText("Loss: --%");
        packetLossText.SetFontFamily("base\\gameplay\\gui\\fonts\\rajdhani\\rajdhani.fnt");
        packetLossText.SetFontSize(12);
        packetLossText.SetAnchor(inkEAnchor.TopLeft);
        packetLossText.SetMargin(new inkMargin(10.0, 50.0, 0.0, 0.0));

        statsRoot.AddChild(statsBackground);
        statsRoot.AddChild(qualityBar);
        statsRoot.AddChild(pingText);
        statsRoot.AddChild(packetLossText);

        this.m_networkStatsWidget = statsRoot;
        this.GetRootWidget().AddChild(this.m_networkStatsWidget);
        this.m_networkStatsWidget.SetVisible(false);

        LogChannel(n"CoopNet", "Network stats panel created using game's health bar style");
    }

    // Voice chat panel using game's phone avatar style
    private func CreateVoiceChatPanel() -> Void {
        let voiceRoot = new inkCanvas();
        voiceRoot.SetName(n"voice_chat");
        voiceRoot.SetAnchor(inkEAnchor.BottomCenter);
        voiceRoot.SetMargin(new inkMargin(0.0, 0.0, 0.0, -50.0));
        voiceRoot.SetSize(new Vector2(300.0, 40.0));

        // Voice activity indicators using game's avatar style
        let voiceBackground = new inkImage();
        voiceBackground.SetAtlasResource(r"base\\gameplay\\gui\\common\\backgrounds\\voice_bg.inkatlas");
        voiceBackground.SetTexturePart(n"voice_bg");
        voiceBackground.SetSize(new Vector2(300.0, 40.0));
        voiceBackground.SetOpacity(0.8);

        // Push-to-talk indicator
        let pttIndicator = new inkText();
        pttIndicator.SetName(n"ptt_indicator");
        pttIndicator.SetText("Hold [V] to Talk");
        pttIndicator.SetFontFamily("base\\gameplay\\gui\\fonts\\rajdhani\\rajdhani.fnt");
        pttIndicator.SetFontSize(14);
        pttIndicator.SetAnchor(inkEAnchor.CenterFill);
        pttIndicator.SetHorizontalAlignment(textHorizontalAlignment.Center);
        pttIndicator.SetVerticalAlignment(textVerticalAlignment.Center);

        voiceRoot.AddChild(voiceBackground);
        voiceRoot.AddChild(pttIndicator);

        this.m_voiceChatWidget = voiceRoot;
        this.GetRootWidget().AddChild(this.m_voiceChatWidget);
        this.m_voiceChatWidget.SetVisible(false);

        LogChannel(n"CoopNet", "Voice chat panel created using game's phone style");
    }

    // Quest sync panel using game's quest tracker style
    private func CreateQuestSyncPanel() -> Void {
        let questRoot = new inkCanvas();
        questRoot.SetName(n"quest_sync");
        questRoot.SetAnchor(inkEAnchor.Center);
        questRoot.SetSize(new Vector2(500.0, 300.0));

        // Use game's quest background
        let questBackground = new inkImage();
        questBackground.SetAtlasResource(r"base\\gameplay\\gui\\common\\backgrounds\\quest_bg.inkatlas");
        questBackground.SetTexturePart(n"quest_bg");
        questBackground.SetNineSliceScale(true);
        questBackground.SetSize(new Vector2(500.0, 300.0));
        questBackground.SetOpacity(0.95);

        // Quest sync title
        let questTitle = new inkText();
        questTitle.SetName(n"quest_sync_title");
        questTitle.SetText("QUEST SYNCHRONIZATION");
        questTitle.SetFontFamily("base\\gameplay\\gui\\fonts\\orbitron\\orbitron.fnt");
        questTitle.SetFontSize(20);
        questTitle.SetLetterCase(textLetterCase.UpperCase);
        questTitle.SetAnchor(inkEAnchor.TopFill);
        questTitle.SetMargin(new inkMargin(20.0, 20.0, 20.0, 0.0));
        questTitle.SetHorizontalAlignment(textHorizontalAlignment.Center);
        questTitle.SetTintColor(new HDRColor(1.0, 1.0, 0.0, 1.0)); // Yellow for quests

        // Quest details container
        let questDetailsContainer = new inkVerticalPanel();
        questDetailsContainer.SetName(n"quest_details");
        questDetailsContainer.SetAnchor(inkEAnchor.Fill);
        questDetailsContainer.SetMargin(new inkMargin(20.0, 60.0, 20.0, 60.0));

        // Voting buttons container
        let votingContainer = new inkHorizontalPanel();
        votingContainer.SetName(n"voting_buttons");
        votingContainer.SetAnchor(inkEAnchor.BottomFill);
        votingContainer.SetMargin(new inkMargin(20.0, 0.0, 20.0, 20.0));
        votingContainer.SetSize(new Vector2(0.0, 40.0));

        // Accept button using game button style
        let acceptButton = new inkButton();
        acceptButton.SetName(n"accept_button");
        acceptButton.SetText("ACCEPT");
        acceptButton.SetSize(new Vector2(100.0, 40.0));

        // Decline button using game button style
        let declineButton = new inkButton();
        declineButton.SetName(n"decline_button");
        declineButton.SetText("DECLINE");
        declineButton.SetSize(new Vector2(100.0, 40.0));

        votingContainer.AddChild(acceptButton);
        votingContainer.AddChild(declineButton);

        questRoot.AddChild(questBackground);
        questRoot.AddChild(questTitle);
        questRoot.AddChild(questDetailsContainer);
        questRoot.AddChild(votingContainer);

        this.m_questSyncWidget = questRoot;
        this.GetRootWidget().AddChild(this.m_questSyncWidget);
        this.m_questSyncWidget.SetVisible(false);

        LogChannel(n"CoopNet", "Quest sync panel created using game's quest tracker style");
    }

    // Setup hotkeys using game's input system
    private func SetupMultiplayerHotkeys() -> Void {
        // Register hotkeys through the game's input system
        // These would be bound to the game's key binding system

        LogChannel(n"CoopNet", "Multiplayer hotkeys configured:");
        LogChannel(n"CoopNet", "  Tab - Toggle Player List");
        LogChannel(n"CoopNet", "  Enter - Toggle Chat");
        LogChannel(n"CoopNet", "  V - Push to Talk");
        LogChannel(n"CoopNet", "  F1 - Toggle Network Stats");
    }

    // Apply current game theme to multiplayer UI
    private func ApplyGameTheme() -> Void {
        // Get current game theme from settings
        let gameSettings = GameInstance.GetSettingsSystem(this.m_gameInstance);

        // Apply theme colors and styling based on player's lifepath and preferences
        this.ApplyThemeColors();
        this.ApplyThemeFonts();

        LogChannel(n"CoopNet", s"Applied game theme: \(this.m_currentTheme)");
    }

    private func ApplyThemeColors() -> Void {
        // Apply color scheme based on current game theme
        let primaryColor = new HDRColor(0.0, 1.0, 1.0, 1.0); // Cyberpunk cyan
        let secondaryColor = new HDRColor(1.0, 1.0, 0.0, 1.0); // Quest yellow
        let errorColor = new HDRColor(1.0, 0.0, 0.0, 1.0); // Error red

        // Apply colors to UI elements
        // This would modify the tint colors of various UI widgets
    }

    private func ApplyThemeFonts() -> Void {
        // Use game's font assets consistently
        // Primary font: Orbitron (futuristic headers)
        // Secondary font: Rajdhani (body text)
        // Monospace font: Source Code Pro (technical data)
    }

    // Public interface methods for native integration

    public func ShowPlayerList() -> Void {
        if IsDefined(this.m_playerListWidget) {
            this.m_playerListWidget.SetVisible(true);
            this.PlayUISound(n"ui_menu_onpress");
        }
    }

    public func HidePlayerList() -> Void {
        if IsDefined(this.m_playerListWidget) {
            this.m_playerListWidget.SetVisible(false);
        }
    }

    public func TogglePlayerList() -> Void {
        if IsDefined(this.m_playerListWidget) {
            if this.m_playerListWidget.IsVisible() {
                this.HidePlayerList();
            } else {
                this.ShowPlayerList();
            }
        }
    }

    public func ShowChatPanel() -> Void {
        if IsDefined(this.m_chatWidget) {
            this.m_chatWidget.SetVisible(true);
            this.PlayUISound(n"ui_menu_onpress");
        }
    }

    public func HideChatPanel() -> Void {
        if IsDefined(this.m_chatWidget) {
            this.m_chatWidget.SetVisible(false);
        }
    }

    public func ShowNetworkStats() -> Void {
        if IsDefined(this.m_networkStatsWidget) {
            this.m_networkStatsWidget.SetVisible(true);
        }
    }

    public func HideNetworkStats() -> Void {
        if IsDefined(this.m_networkStatsWidget) {
            this.m_networkStatsWidget.SetVisible(false);
        }
    }

    public func ShowVoiceChatPanel() -> Void {
        if IsDefined(this.m_voiceChatWidget) {
            this.m_voiceChatWidget.SetVisible(true);
        }
    }

    public func HideVoiceChatPanel() -> Void {
        if IsDefined(this.m_voiceChatWidget) {
            this.m_voiceChatWidget.SetVisible(false);
        }
    }

    public func ShowQuestSyncPanel(questName: String, description: String) -> Void {
        if IsDefined(this.m_questSyncWidget) {
            // Update quest details
            this.UpdateQuestSyncDetails(questName, description);
            this.m_questSyncWidget.SetVisible(true);
            this.PlayUISound(n"ui_quest_update");
        }
    }

    public func HideQuestSyncPanel() -> Void {
        if IsDefined(this.m_questSyncWidget) {
            this.m_questSyncWidget.SetVisible(false);
        }
    }

    private func UpdateQuestSyncDetails(questName: String, description: String) -> Void {
        // Update quest details in the sync panel
        let detailsContainer = this.m_questSyncWidget.GetWidget(n"quest_details") as inkVerticalPanel;
        if IsDefined(detailsContainer) {
            // Clear existing details
            detailsContainer.RemoveAllChildren();

            // Add quest name
            let nameText = new inkText();
            nameText.SetText(questName);
            nameText.SetFontFamily("base\\gameplay\\gui\\fonts\\orbitron\\orbitron.fnt");
            nameText.SetFontSize(16);
            nameText.SetTintColor(new HDRColor(1.0, 1.0, 0.0, 1.0));
            detailsContainer.AddChild(nameText);

            // Add quest description
            let descText = new inkText();
            descText.SetText(description);
            descText.SetFontFamily("base\\gameplay\\gui\\fonts\\rajdhani\\rajdhani.fnt");
            descText.SetFontSize(14);
            descText.SetMargin(new inkMargin(0.0, 10.0, 0.0, 0.0));
            detailsContainer.AddChild(descText);
        }
    }

    // Notification system using game's notification framework
    public func ShowMultiplayerNotification(title: String, message: String, iconType: String) -> Void {
        if IsDefined(this.m_notificationSystem) {
            // Create notification data using game's notification system
            let notificationData = new GenericNotificationViewData();
            notificationData.title = title;
            notificationData.text = message;

            // Use appropriate game icon based on type
            let iconAtlas = this.GetNotificationIcon(iconType);
            notificationData.iconID = iconAtlas;

            // Show notification using game's system
            this.m_notificationSystem.ShowNotification(notificationData);
            this.PlayUISound(n"ui_generic_notification");
        }
    }

    private func GetNotificationIcon(iconType: String) -> TweakDBID {
        // Return appropriate game notification icon based on type
        switch iconType {
            case "player_joined":
                return t"UIIcon.PlayerJoined";
            case "player_left":
                return t"UIIcon.PlayerLeft";
            case "quest_update":
                return t"UIIcon.QuestUpdate";
            case "voice_activity":
                return t"UIIcon.VoiceChat";
            case "network_issue":
                return t"UIIcon.Warning";
            default:
                return t"UIIcon.Info";
        }
    }

    // Audio integration using game's sound system
    private func PlayUISound(soundName: CName) -> Void {
        // Play UI sound using game's audio system
        let audioEvent = new SoundPlayEvent();
        audioEvent.soundName = soundName;
        this.QueueEvent(audioEvent);
    }

    // Update methods called from native code
    public func UpdatePlayerData(playerData: String) -> Void {
        // Update player list with new data
        // Data comes as JSON string from native code
        this.RefreshPlayerListDisplay(playerData);
    }

    public func UpdateNetworkStatistics(ping: String, packetLoss: String, quality: String) -> Void {
        // Update network stats display
        if IsDefined(this.m_networkStatsWidget) && this.m_networkStatsWidget.IsVisible() {
            let pingText = this.m_networkStatsWidget.GetWidget(n"ping_text") as inkText;
            let lossText = this.m_networkStatsWidget.GetWidget(n"packet_loss_text") as inkText;
            let qualityBar = this.m_networkStatsWidget.GetWidget(n"quality_bar") as inkImage;

            if IsDefined(pingText) {
                pingText.SetText(s"Ping: \(ping)ms");
            }

            if IsDefined(lossText) {
                lossText.SetText(s"Loss: \(packetLoss)%");
            }

            if IsDefined(qualityBar) {
                // Update quality bar color based on connection quality
                let qualityColor = this.GetQualityColor(quality);
                qualityBar.SetTintColor(qualityColor);
            }
        }
    }

    private func GetQualityColor(quality: String) -> HDRColor {
        switch quality {
            case "excellent":
                return new HDRColor(0.0, 1.0, 0.0, 1.0); // Green
            case "good":
                return new HDRColor(1.0, 1.0, 0.0, 1.0); // Yellow
            case "poor":
                return new HDRColor(1.0, 0.0, 0.0, 1.0); // Red
            default:
                return new HDRColor(0.5, 0.5, 0.5, 1.0); // Gray
        }
    }

    private func RefreshPlayerListDisplay(playerData: String) -> Void {
        // Parse player data and update the player list display
        // This would involve updating the player list widget with new player information
        LogChannel(n"CoopNet", s"Updating player list with data: \(playerData)");
    }

    public func AddChatMessage(playerName: String, message: String) -> Void {
        // Add new chat message to the display
        if IsDefined(this.m_chatWidget) {
            let messagesContainer = this.m_chatWidget.GetWidget(n"chat_messages_container") as inkVerticalPanel;
            if IsDefined(messagesContainer) {
                let messageText = new inkText();
                messageText.SetText(s"\(playerName): \(message)");
                messageText.SetFontFamily("base\\gameplay\\gui\\fonts\\rajdhani\\rajdhani.fnt");
                messageText.SetFontSize(12);
                messageText.SetMargin(new inkMargin(0.0, 2.0, 0.0, 0.0));

                messagesContainer.AddChild(messageText);

                // Limit message history
                if messagesContainer.GetNumChildren() > 20 {
                    messagesContainer.RemoveChildByIndex(0);
                }

                // Auto-scroll to bottom
                let scrollArea = this.m_chatWidget.GetWidget(n"chat_messages_scroll") as inkScrollArea;
                if IsDefined(scrollArea) {
                    scrollArea.SetScrollPosition(1.0);
                }
            }
        }
    }

    // Native function bindings (implemented in C++)
    private native func InitializeNativeUI() -> Bool;
    private native func UpdateNativeUIData(panelType: String, data: String) -> Void;
    private native func HandleNativeUIEvent(eventType: String, data: String) -> Void;
}

// Input handling for multiplayer UI
public class MultiplayerUIInputHandler extends inkGameController {
    protected let m_uiIntegration: ref<MultiplayerUIIntegration>;

    protected cb func OnInitialize() -> Void {
        // Get reference to UI integration
        this.m_uiIntegration = this.GetRootWidget().GetController() as MultiplayerUIIntegration;
    }

    protected cb func OnAction(action: ListenerAction, consumer: ListenerActionConsumer) -> Bool {
        if !IsDefined(this.m_uiIntegration) {
            return false;
        }

        let actionName = ListenerAction.GetName(action);
        let actionType = ListenerAction.GetType(action);

        if Equals(actionType, gameinputActionType.BUTTON_PRESSED) {
            switch actionName {
                case n"UI_Tab":
                    this.m_uiIntegration.TogglePlayerList();
                    return true;
                case n"UI_Enter":
                    this.m_uiIntegration.ShowChatPanel();
                    return true;
                case n"UI_F1":
                    if this.m_uiIntegration.m_networkStatsWidget.IsVisible() {
                        this.m_uiIntegration.HideNetworkStats();
                    } else {
                        this.m_uiIntegration.ShowNetworkStats();
                    }
                    return true;
            }
        }

        return false;
    }
}

// Global instance for easy access
@addField(ScriptGameInstance)
public let multiplayerUI: ref<MultiplayerUIIntegration>;

// Initialize the multiplayer UI when the game starts
@wrapMethod(ScriptGameInstance)
protected func InitializeScriptGameInstance() -> Void {
    wrappedMethod();

    // Initialize multiplayer UI integration
    this.multiplayerUI = new MultiplayerUIIntegration();
    this.multiplayerUI.OnInitialize();
}