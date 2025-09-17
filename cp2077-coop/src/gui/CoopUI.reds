// Modern, user-friendly UI system for CP2077 Coop
// Provides intuitive interface for server browsing, hosting, and connection management

import Codeware.UI

public enum CoopUIState {
    MainMenu = 0,
    ServerBrowser = 1,
    ServerHost = 2,
    Connecting = 3,
    Connected = 4,
    Settings = 5
}

public class CoopUI extends inkGameController {
    private var currentState: CoopUIState;
    private var rootContainer: wref<inkCanvas>;
    private var titleBar: wref<inkHorizontalPanel>;
    private var contentArea: wref<inkCanvas>;
    private var statusBar: wref<inkHorizontalPanel>;
    
    // UI Components
    private var mainMenu: ref<CoopMainMenu>;
    private var serverBrowser: ref<ModernServerBrowser>;
    private var serverHost: ref<ServerHostDialog>;
    private var settingsPanel: ref<CoopSettings>;
    private var connectingDialog: ref<ConnectingDialog>;
    
    // Animation
    private var fadeProxy: ref<inkAnimProxy>;
    private var slideProxy: ref<inkAnimProxy>;
    
    // Audio
    private var audioSystem: wref<inkAudioSystem>;
    
    protected cb func OnInitialize() -> Bool {
        this.SetClassName(n"CoopUI");
        this.CreateRootStructure();
        this.InitializeComponents();
        this.SetState(CoopUIState.MainMenu);
        
        this.audioSystem = GameInstance.GetAudioSystem(GetGame());
        
        // Register input handlers
        this.RegisterToGlobalInputCallback(n"OnPostOnRelease", this, n"OnGlobalInput");
        
        LogChannel(n"COOP_UI", "CoopUI initialized successfully");
        return true;
    }
    
    private func CreateRootStructure() -> Void {
        // Root container
        this.rootContainer = new inkCanvas();
        this.rootContainer.SetName(n"RootContainer");
        this.rootContainer.SetAnchor(inkEAnchor.Fill);
        this.rootContainer.SetMargin(new inkMargin(40.0, 40.0, 40.0, 40.0));
        this.SetRootWidget(this.rootContainer);
        
        // Background overlay
        let bgOverlay = new inkRectangle();
        bgOverlay.SetName(n"BackgroundOverlay");
        bgOverlay.SetAnchor(inkEAnchor.Fill);
        bgOverlay.SetTintColor(new HDRColor(0.0, 0.0, 0.0, 0.8));
        this.rootContainer.AddChild(bgOverlay);
        
        // Title bar
        this.titleBar = new inkHorizontalPanel();
        this.titleBar.SetName(n"TitleBar");
        this.titleBar.SetAnchor(inkEAnchor.TopFillHorizontal);
        this.titleBar.SetSize(new Vector2(0.0, 80.0));
        this.titleBar.SetChildMargin(new inkMargin(10.0, 10.0, 10.0, 10.0));
        this.rootContainer.AddChild(this.titleBar);
        
        let titleText = new inkText();
        titleText.SetName(n"TitleText");
        titleText.SetText("CYBERPUNK 2077 - COOPERATIVE MODE");
        titleText.SetFontFamily("base\\gameplay\\gui\\fonts\\raj\\raj.inkfontfamily");
        titleText.SetFontSize(32);
        titleText.SetTintColor(new HDRColor(0.368627, 0.964706, 1.0, 1.0)); // CP2077 cyan
        titleText.SetHorizontalAlignment(textHorizontalAlignment.Center);
        titleText.SetVerticalAlignment(textVerticalAlignment.Center);
        this.titleBar.AddChild(titleText);
        
        let closeButton = new inkButton();
        closeButton.SetName(n"CloseButton");
        closeButton.SetText("✕");
        closeButton.SetSize(new Vector2(50.0, 50.0));
        closeButton.SetStyle(n"Button");
        closeButton.Reparent(this.titleBar);
        closeButton.RegisterToCallback(n"OnRelease", this, n"OnCloseClicked");
        
        // Content area
        this.contentArea = new inkCanvas();
        this.contentArea.SetName(n"ContentArea");
        this.contentArea.SetAnchor(inkEAnchor.Fill);
        this.contentArea.SetMargin(new inkMargin(0.0, 90.0, 0.0, 60.0));
        this.rootContainer.AddChild(this.contentArea);
        
        // Status bar
        this.statusBar = new inkHorizontalPanel();
        this.statusBar.SetName(n"StatusBar");
        this.statusBar.SetAnchor(inkEAnchor.BottomFillHorizontal);
        this.statusBar.SetSize(new Vector2(0.0, 50.0));
        this.statusBar.SetChildMargin(new inkMargin(10.0, 5.0, 10.0, 5.0));
        this.rootContainer.AddChild(this.statusBar);
        
        let versionText = new inkText();
        versionText.SetName(n"VersionText");
        versionText.SetText("Version: " + VersionCheck.GetVersionString());
        versionText.SetFontSize(18);
        versionText.SetTintColor(new HDRColor(0.7, 0.7, 0.7, 1.0));
        this.statusBar.AddChild(versionText);
        
        let connectionStatus = new inkText();
        connectionStatus.SetName(n"ConnectionStatus");
        connectionStatus.SetText("Offline");
        connectionStatus.SetFontSize(18);
        connectionStatus.SetTintColor(new HDRColor(1.0, 0.4, 0.4, 1.0));
        connectionStatus.SetHorizontalAlignment(textHorizontalAlignment.Right);
        this.statusBar.AddChild(connectionStatus);
    }
    
    private func InitializeComponents() -> Void {
        this.mainMenu = new CoopMainMenu();
        this.mainMenu.Initialize(this.contentArea, this);
        
        this.serverBrowser = new ModernServerBrowser();
        this.serverBrowser.Initialize(this.contentArea, this);
        
        this.serverHost = new ServerHostDialog();
        this.serverHost.Initialize(this.contentArea, this);
        
        this.settingsPanel = new CoopSettings();
        this.settingsPanel.Initialize(this.contentArea, this);
        
        this.connectingDialog = new ConnectingDialog();
        this.connectingDialog.Initialize(this.contentArea, this);
    }
    
    public func SetState(newState: CoopUIState) -> Void {
        if this.currentState == newState {
            return;
        }
        
        // Hide current component
        this.HideCurrentComponent();
        
        // Update state
        this.currentState = newState;
        
        // Show new component with animation
        this.ShowCurrentComponent();
        
        // Play transition sound
        this.PlayTransitionSound();
    }
    
    private func HideCurrentComponent() -> Void {
        switch this.currentState {
            case CoopUIState.MainMenu:
                if IsDefined(this.mainMenu) { this.mainMenu.Hide(); }
                break;
            case CoopUIState.ServerBrowser:
                if IsDefined(this.serverBrowser) { this.serverBrowser.Hide(); }
                break;
            case CoopUIState.ServerHost:
                if IsDefined(this.serverHost) { this.serverHost.Hide(); }
                break;
            case CoopUIState.Settings:
                if IsDefined(this.settingsPanel) { this.settingsPanel.Hide(); }
                break;
            case CoopUIState.Connecting:
                if IsDefined(this.connectingDialog) { this.connectingDialog.Hide(); }
                break;
        }
    }
    
    private func ShowCurrentComponent() -> Void {
        switch this.currentState {
            case CoopUIState.MainMenu:
                if IsDefined(this.mainMenu) { this.mainMenu.Show(); }
                break;
            case CoopUIState.ServerBrowser:
                if IsDefined(this.serverBrowser) { this.serverBrowser.Show(); }
                break;
            case CoopUIState.ServerHost:
                if IsDefined(this.serverHost) { this.serverHost.Show(); }
                break;
            case CoopUIState.Settings:
                if IsDefined(this.settingsPanel) { this.settingsPanel.Show(); }
                break;
            case CoopUIState.Connecting:
                if IsDefined(this.connectingDialog) { this.connectingDialog.Show(); }
                break;
        }
    }
    
    private func PlayTransitionSound() -> Void {
        if IsDefined(this.audioSystem) {
            let audioEvent = new AudioEvent();
            audioEvent.eventName = n"ui_menu_onpress";
            this.audioSystem.Play(audioEvent);
        }
    }
    
    public func UpdateConnectionStatus(isConnected: Bool, serverName: String) -> Void {
        let statusWidget = this.statusBar.GetWidget(n"ConnectionStatus") as inkText;
        if IsDefined(statusWidget) {
            if isConnected {
                statusWidget.SetText("Connected to: " + serverName);
                statusWidget.SetTintColor(new HDRColor(0.4, 1.0, 0.4, 1.0)); // Green
            } else {
                statusWidget.SetText("Offline");
                statusWidget.SetTintColor(new HDRColor(1.0, 0.4, 0.4, 1.0)); // Red
            }
        }
    }
    
    protected cb func OnCloseClicked(e: ref<inkPointerEvent>) -> Bool {
        this.PlaySound(n"ui_menu_onpress");
        this.Close();
        return true;
    }
    
    protected cb func OnGlobalInput(e: ref<inkPointerEvent>) -> Bool {
        // Handle ESC key to go back
        if Equals(e.GetInputKey(), EInputKey.IK_Escape) {
            if this.currentState == CoopUIState.MainMenu {
                this.Close();
            } else {
                this.SetState(CoopUIState.MainMenu);
            }
            return true;
        }
        return false;
    }
    
    public func Close() -> Void {
        // Clean up components
        if IsDefined(this.mainMenu) { this.mainMenu.Cleanup(); }
        if IsDefined(this.serverBrowser) { this.serverBrowser.Cleanup(); }
        if IsDefined(this.serverHost) { this.serverHost.Cleanup(); }
        if IsDefined(this.settingsPanel) { this.settingsPanel.Cleanup(); }
        if IsDefined(this.connectingDialog) { this.connectingDialog.Cleanup(); }
        
        // Unregister callbacks
        this.UnregisterFromGlobalInputCallback(n"OnPostOnRelease", this, n"OnGlobalInput");
        
        // Close UI
        let hudManager = GameInstance.GetHUDManager(GetGame());
        if IsDefined(hudManager) {
            hudManager.RemoveLayer(this);
        }
    }
    
    private func PlaySound(soundName: CName) -> Void {
        if IsDefined(this.audioSystem) {
            let audioEvent = new AudioEvent();
            audioEvent.eventName = soundName;
            this.audioSystem.Play(audioEvent);
        }
    }
}

// Main Menu Component
public class CoopMainMenu {
    private var container: wref<inkCanvas>;
    private var parentUI: wref<CoopUI>;
    private var buttonContainer: wref<inkVerticalPanel>;
    
    public func Initialize(parent: wref<inkCanvas>, ui: wref<CoopUI>) -> Void {
        this.parentUI = ui;
        this.CreateUI(parent);
    }
    
    private func CreateUI(parent: wref<inkCanvas>) -> Void {
        this.container = new inkCanvas();
        this.container.SetName(n"MainMenuContainer");
        this.container.SetAnchor(inkEAnchor.Fill);
        parent.AddChild(this.container);
        
        // Center panel for buttons
        this.buttonContainer = new inkVerticalPanel();
        this.buttonContainer.SetName(n"ButtonContainer");
        this.buttonContainer.SetAnchor(inkEAnchor.CenterFillVertical);
        this.buttonContainer.SetSize(new Vector2(400.0, 500.0));
        this.buttonContainer.SetChildMargin(new inkMargin(0.0, 10.0, 0.0, 10.0));
        this.container.AddChild(this.buttonContainer);
        
        // Join Server Button
        let joinButton = this.CreateMenuButton("JOIN SERVER", "Browse and connect to multiplayer servers");
        joinButton.RegisterToCallback(n"OnRelease", this, n"OnJoinServerClicked");
        this.buttonContainer.AddChild(joinButton);
        
        // Host Server Button
        let hostButton = this.CreateMenuButton("HOST SERVER", "Create your own multiplayer server");
        hostButton.RegisterToCallback(n"OnRelease", this, n"OnHostServerClicked");
        this.buttonContainer.AddChild(hostButton);
        
        // Quick Match Button
        let quickButton = this.CreateMenuButton("QUICK MATCH", "Find and join the best available server");
        quickButton.RegisterToCallback(n"OnRelease", this, n"OnQuickMatchClicked");
        this.buttonContainer.AddChild(quickButton);
        
        // Settings Button
        let settingsButton = this.CreateMenuButton("SETTINGS", "Configure coop options and preferences");
        settingsButton.RegisterToCallback(n"OnRelease", this, n"OnSettingsClicked");
        this.buttonContainer.AddChild(settingsButton);
        
        // Tutorial Button
        let tutorialButton = this.CreateMenuButton("TUTORIAL", "Learn how to use cooperative mode");
        tutorialButton.RegisterToCallback(n"OnRelease", this, n"OnTutorialClicked");
        this.buttonContainer.AddChild(tutorialButton);
    }
    
    private func CreateMenuButton(text: String, tooltip: String) -> ref<inkButton> {
        let buttonWrapper = new inkVerticalPanel();
        
        let button = new inkButton();
        button.SetText(text);
        button.SetSize(new Vector2(380.0, 60.0));
        button.SetStyle(n"Button");
        button.SetFontSize(24);
        
        // Style the button
        let buttonBg = button.GetController() as inkButtonController;
        if IsDefined(buttonBg) {
            buttonBg.SetHoverTintColor(new HDRColor(0.368627, 0.964706, 1.0, 0.8));
            buttonBg.SetPressTintColor(new HDRColor(0.368627, 0.964706, 1.0, 1.0));
        }
        
        let tooltipText = new inkText();
        tooltipText.SetText(tooltip);
        tooltipText.SetFontSize(16);
        tooltipText.SetTintColor(new HDRColor(0.8, 0.8, 0.8, 1.0));
        tooltipText.SetHorizontalAlignment(textHorizontalAlignment.Center);
        tooltipText.SetMargin(new inkMargin(0.0, 5.0, 0.0, 0.0));
        
        buttonWrapper.AddChild(button);
        buttonWrapper.AddChild(tooltipText);
        
        return button;
    }
    
    protected cb func OnJoinServerClicked(e: ref<inkPointerEvent>) -> Bool {
        this.parentUI.SetState(CoopUIState.ServerBrowser);
        return true;
    }
    
    protected cb func OnHostServerClicked(e: ref<inkPointerEvent>) -> Bool {
        this.parentUI.SetState(CoopUIState.ServerHost);
        return true;
    }
    
    protected cb func OnQuickMatchClicked(e: ref<inkPointerEvent>) -> Bool {
        // Start quick match process
        QuickMatchManager.StartQuickMatch();
        this.parentUI.SetState(CoopUIState.Connecting);
        return true;
    }
    
    protected cb func OnSettingsClicked(e: ref<inkPointerEvent>) -> Bool {
        this.parentUI.SetState(CoopUIState.Settings);
        return true;
    }
    
    protected cb func OnTutorialClicked(e: ref<inkPointerEvent>) -> Bool {
        TutorialManager.ShowCoopTutorial();
        return true;
    }
    
    public func Show() -> Void {
        this.container.SetVisible(true);
        this.PlayFadeInAnimation();
    }
    
    public func Hide() -> Void {
        this.container.SetVisible(false);
    }
    
    private func PlayFadeInAnimation() -> Void {
        let animDef = new inkAnimDef();
        let fadeInterp = new inkAnimTransparency();
        fadeInterp.SetStartTransparency(0.0);
        fadeInterp.SetEndTransparency(1.0);
        fadeInterp.SetDuration(0.3);
        fadeInterp.SetType(inkanimInterpolationType.Quadratic);
        fadeInterp.SetMode(inkanimInterpolationMode.EaseOut);
        animDef.AddInterpolator(fadeInterp);
        
        this.container.PlayAnimation(animDef);
    }
    
    public func Cleanup() -> Void {
        // Unregister callbacks and clean up resources
        if IsDefined(this.buttonContainer) {
            let childCount = this.buttonContainer.GetNumChildren();
            for i in Range(childCount) {
                let child = this.buttonContainer.GetChild(i) as inkButton;
                if IsDefined(child) {
                    child.UnregisterFromAllCallbacks(this);
                }
            }
        }
    }
}

// Static UI Manager
public class CoopUIManager {
    private static var currentInstance: wref<CoopUI>;
    
    public static func Show() -> Void {
        if IsDefined(CoopUIManager.currentInstance) {
            return; // Already showing
        }
        
        let hudManager = GameInstance.GetHUDManager(GetGame());
        if !IsDefined(hudManager) {
            LogChannel(n"COOP_UI", "HUD Manager not available");
            return;
        }
        
        let uiInstance = new CoopUI();
        CoopUIManager.currentInstance = uiInstance;
        
        hudManager.AddLayer(uiInstance);
        LogChannel(n"COOP_UI", "CoopUI shown successfully");
    }
    
    public static func Hide() -> Void {
        if IsDefined(CoopUIManager.currentInstance) {
            CoopUIManager.currentInstance.Close();
            CoopUIManager.currentInstance = null;
        }
    }
    
    public static func IsShowing() -> Bool {
        return IsDefined(CoopUIManager.currentInstance);
    }
    
    public static func GetInstance() -> wref<CoopUI> {
        return CoopUIManager.currentInstance;
    }
}

// Quick Match System
public class QuickMatchManager {
    private static var isSearching: Bool = false;
    private static var searchStartTime: Float = 0.0;
    private static var maxSearchTime: Float = 30.0; // 30 seconds
    
    public static func StartQuickMatch() -> Void {
        if QuickMatchManager.isSearching {
            return;
        }
        
        QuickMatchManager.isSearching = true;
        QuickMatchManager.searchStartTime = EngineTime.ToFloat(GameInstance.GetTimeSystem(GetGame()).GetGameTime());
        
        LogChannel(n"COOP_UI", "Starting quick match search...");
        
        // Start searching for best server
        ServerBrowserAPI.RefreshServerList();
    }
    
    public static func Update() -> Void {
        if !QuickMatchManager.isSearching {
            return;
        }
        
        let currentTime = EngineTime.ToFloat(GameInstance.GetTimeSystem(GetGame()).GetGameTime());
        let searchTime = currentTime - QuickMatchManager.searchStartTime;
        
        if searchTime > QuickMatchManager.maxSearchTime {
            // Timeout
            QuickMatchManager.StopQuickMatch();
            NotificationManager.ShowError("Quick match failed: No suitable servers found");
            return;
        }
        
        // Check if we found a good server
        let bestServer = ServerBrowserAPI.GetBestAvailableServer();
        if IsDefined(bestServer) {
            QuickMatchManager.ConnectToBestServer(bestServer);
        }
    }
    
    private static func ConnectToBestServer(server: ref<ServerInfo>) -> Void {
        QuickMatchManager.StopQuickMatch();
        
        LogChannel(n"COOP_UI", "Connecting to best server: " + server.name);
        ConnectionManager.ConnectToServer(server);
    }
    
    public static func StopQuickMatch() -> Void {
        QuickMatchManager.isSearching = false;
        QuickMatchManager.searchStartTime = 0.0;
    }
    
    public static func IsSearching() -> Bool {
        return QuickMatchManager.isSearching;
    }
}

// Tutorial System
public class TutorialManager {
    public static func ShowCoopTutorial() -> Void {
        let tutorialSteps: array<String> = [
            "Welcome to Cyberpunk 2077 Cooperative Mode! This tutorial will guide you through the basics.",
            "To join a server: Click 'JOIN SERVER' and select a server from the list. Look for servers with good ping and available slots.",
            "To host a server: Click 'HOST SERVER' and configure your server settings. Your friends can join using your IP address.",
            "Quick Match: Use this option to automatically find and join the best available server based on your connection.",
            "In-game: Use F1 to open the coop menu, manage your party, and access multiplayer features.",
            "Have fun exploring Night City with your friends!"
        ];
        
        TutorialOverlay.Show(tutorialSteps);
    }
}

// Notification System
public class NotificationManager {
    public static func ShowSuccess(message: String) -> Void {
        let notification = new CoopNotification();
        notification.ShowSuccess(message);
    }
    
    public static func ShowError(message: String) -> Void {
        let notification = new CoopNotification();
        notification.ShowError(message);
    }
    
    public static func ShowInfo(message: String) -> Void {
        let notification = new CoopNotification();
        notification.ShowInfo(message);
    }
}

public class CoopNotification extends inkGameController {
    private var container: wref<inkCanvas>;
    private var messageText: wref<inkText>;
    private var fadeProxy: ref<inkAnimProxy>;
    
    public func ShowSuccess(message: String) -> Void {
        this.Show(message, new HDRColor(0.4, 1.0, 0.4, 1.0));
    }
    
    public func ShowError(message: String) -> Void {
        this.Show(message, new HDRColor(1.0, 0.4, 0.4, 1.0));
    }
    
    public func ShowInfo(message: String) -> Void {
        this.Show(message, new HDRColor(0.368627, 0.964706, 1.0, 1.0));
    }
    
    private func Show(message: String, color: HDRColor) -> Void {
        this.CreateUI();
        this.messageText.SetText(message);
        this.messageText.SetTintColor(color);
        
        // Add to HUD
        let hudManager = GameInstance.GetHUDManager(GetGame());
        if IsDefined(hudManager) {
            hudManager.AddLayer(this);
        }
        
        // Auto-hide after 3 seconds
        GameInstance.GetDelaySystem(GetGame()).DelayCallback(this, n"HideNotification", 3.0);
    }
    
    private func CreateUI() -> Void {
        this.container = new inkCanvas();
        this.container.SetAnchor(inkEAnchor.TopCenter);
        this.container.SetMargin(new inkMargin(0.0, 100.0, 0.0, 0.0));
        this.SetRootWidget(this.container);
        
        let background = new inkRectangle();
        background.SetSize(new Vector2(400.0, 60.0));
        background.SetTintColor(new HDRColor(0.0, 0.0, 0.0, 0.8));
        this.container.AddChild(background);
        
        this.messageText = new inkText();
        this.messageText.SetFontSize(20);
        this.messageText.SetHorizontalAlignment(textHorizontalAlignment.Center);
        this.messageText.SetVerticalAlignment(textVerticalAlignment.Center);
        this.messageText.SetMargin(new inkMargin(10.0, 10.0, 10.0, 10.0));
        this.container.AddChild(this.messageText);
        
        this.PlayFadeInAnimation();
    }
    
    private func PlayFadeInAnimation() -> Void {
        let animDef = new inkAnimDef();
        let fadeInterp = new inkAnimTransparency();
        fadeInterp.SetStartTransparency(0.0);
        fadeInterp.SetEndTransparency(1.0);
        fadeInterp.SetDuration(0.3);
        animDef.AddInterpolator(fadeInterp);
        
        this.fadeProxy = this.container.PlayAnimation(animDef);
    }
    
    protected cb func HideNotification() -> Void {
        let animDef = new inkAnimDef();
        let fadeInterp = new inkAnimTransparency();
        fadeInterp.SetStartTransparency(1.0);
        fadeInterp.SetEndTransparency(0.0);
        fadeInterp.SetDuration(0.3);
        animDef.AddInterpolator(fadeInterp);
        
        this.fadeProxy = this.container.PlayAnimation(animDef);
        this.fadeProxy.RegisterToCallback(inkanimEventType.OnFinish, this, n"OnFadeOutFinished");
    }
    
    protected cb func OnFadeOutFinished(proxy: ref<inkAnimProxy>) -> Bool {
        let hudManager = GameInstance.GetHUDManager(GetGame());
        if IsDefined(hudManager) {
            hudManager.RemoveLayer(this);
        }
        return true;
    }
}