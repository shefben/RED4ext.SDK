// ============================================================================
// Cooperative UI Manager
// ============================================================================
// Manages all cooperative multiplayer UI elements and interfaces

import Codeware.UI.*

public class CoopUIManager extends inkHUDGameController {
    private static var instance: ref<CoopUIManager>;
    private static var isVisible: Bool = false;
    private static var currentPanel: ref<inkWidget>;
    private static var rootPanel: ref<inkVerticalPanel>;
    private static var tabContainer: ref<inkHorizontalPanel>;
    
    // UI Panels
    private static var serverBrowserPanel: ref<inkWidget>;
    private static var settingsPanel: ref<inkWidget>;
    private static var statusPanel: ref<inkWidget>;
    private static var chatPanel: ref<inkWidget>;
    
    // Tab buttons
    private static var serverTab: ref<inkButton>;
    private static var settingsTab: ref<inkButton>;
    private static var statusTab: ref<inkButton>;
    private static var chatTab: ref<inkButton>;
    private static var closeButton: ref<inkButton>;
    
    public static func Show() -> Void {
        LogChannel(n"COOP_UI", "CoopUIManager.Show() called");
        
        if isVisible {
            LogChannel(n"COOP_UI", "Coop UI already visible");
            return;
        }
        
        if !IsDefined(instance) {
            CreateInstance();
        }
        
        if IsDefined(instance) {
            instance.SetVisible(true);
            isVisible = true;
            LogChannel(n"COOP_UI", "Coop UI shown");
        } else {
            LogChannel(n"ERROR", "Failed to create CoopUIManager instance");
        }
    }
    
    public static func Hide() -> Void {
        if !isVisible || !IsDefined(instance) {
            return;
        }
        
        instance.SetVisible(false);
        isVisible = false;
        LogChannel(n"COOP_UI", "Coop UI hidden");
    }
    
    public static func Toggle() -> Void {
        if isVisible {
            Hide();
        } else {
            Show();
        }
    }
    
    private static func CreateInstance() -> Void {
        try {
            LogChannel(n"COOP_UI", "Creating CoopUIManager instance...");
            
            // Create main UI container
            let hudManager = GameInstance.GetHUDManager(GetGame());
            if !IsDefined(hudManager) {
                LogChannel(n"ERROR", "HUD Manager not available");
                return;
            }
            
            instance = new CoopUIManager();
            hudManager.AddLayer(instance);
            
            // Build UI structure
            instance.BuildUI();
            
            LogChannel(n"COOP_UI", "CoopUIManager instance created successfully");
        } catch {
            LogChannel(n"ERROR", "Failed to create CoopUIManager instance");
            instance = null;
        }
    }
    
    private func BuildUI() -> Void {
        LogChannel(n"COOP_UI", "Building Coop UI structure...");
        
        // Create root panel
        rootPanel = new inkVerticalPanel();
        rootPanel.SetName(n"CoopRootPanel");
        rootPanel.SetSize(new Vector2(800.0, 600.0));
        rootPanel.SetAnchor(inkEAnchor.CenterCenter);
        rootPanel.SetMargin(new inkMargin(10.0, 10.0, 10.0, 10.0));
        
        // Style the root panel
        let bg = new inkRectangle();
        bg.SetTintColor(new HDRColor(0.0, 0.0, 0.0, 0.8));
        bg.SetSize(new Vector2(100.0, 100.0));
        rootPanel.AddChild(bg);
        
        this.SetRootWidget(rootPanel);
        
        // Create tab container
        CreateTabBar();
        
        // Create content panels
        CreateContentPanels();
        
        // Show server browser by default
        ShowServerBrowser();
        
        LogChannel(n"COOP_UI", "UI structure built successfully");
    }
    
    private func CreateTabBar() -> Void {
        tabContainer = new inkHorizontalPanel();
        tabContainer.SetName(n"CoopTabContainer");
        tabContainer.SetSize(new Vector2(100.0, 50.0));
        rootPanel.AddChild(tabContainer);
        
        // Create tab buttons
        serverTab = CreateTabButton("Server Browser", n"OnServerTabClick");
        settingsTab = CreateTabButton("Settings", n"OnSettingsTabClick");
        statusTab = CreateTabButton("Status", n"OnStatusTabClick");
        chatTab = CreateTabButton("Chat", n"OnChatTabClick");
        
        // Create close button
        closeButton = new inkButton();
        closeButton.SetText("✕");
        closeButton.SetSize(new Vector2(40.0, 40.0));
        closeButton.RegisterToCallback(n"OnRelease", this, n"OnCloseClick");
        tabContainer.AddChild(closeButton);
    }
    
    private func CreateTabButton(text: String, callback: CName) -> ref<inkButton> {
        let button = new inkButton();
        button.SetText(text);
        button.SetSize(new Vector2(120.0, 40.0));
        button.SetStyle(n"BaseButton");
        button.RegisterToCallback(n"OnRelease", this, callback);
        tabContainer.AddChild(button);
        return button;
    }
    
    private func CreateContentPanels() -> Void {
        // Create content area container
        let contentContainer = new inkVerticalPanel();
        contentContainer.SetName(n"CoopContentContainer");
        contentContainer.SetSize(new Vector2(100.0, -50.0)); // Fill remaining height
        rootPanel.AddChild(contentContainer);
        
        // Create individual panels (initially hidden)
        serverBrowserPanel = CreateServerBrowserPanel();
        settingsPanel = CreateSettingsPanel();
        statusPanel = CreateStatusPanel();
        chatPanel = CreateChatPanel();
        
        // Add all panels to container
        contentContainer.AddChild(serverBrowserPanel);
        contentContainer.AddChild(settingsPanel);
        contentContainer.AddChild(statusPanel);
        contentContainer.AddChild(chatPanel);
    }
    
    private func CreateServerBrowserPanel() -> ref<inkWidget> {
        let panel = new inkVerticalPanel();
        panel.SetName(n"ServerBrowserPanel");
        
        let label = new inkText();
        label.SetText("Server Browser");
        label.SetFontSize(24);
        panel.AddChild(label);
        
        // Add server browser functionality here
        let infoText = new inkText();
        infoText.SetText("Server browser functionality would be implemented here.");
        panel.AddChild(infoText);
        
        return panel;
    }
    
    private func CreateSettingsPanel() -> ref<inkWidget> {
        let panel = new inkVerticalPanel();
        panel.SetName(n"SettingsPanel");
        panel.SetVisible(false);
        
        let label = new inkText();
        label.SetText("Coop Settings");
        label.SetFontSize(24);
        panel.AddChild(label);
        
        // Add settings UI here
        let infoText = new inkText();
        infoText.SetText("Cooperative settings would be configured here.");
        panel.AddChild(infoText);
        
        return panel;
    }
    
    private func CreateStatusPanel() -> ref<inkWidget> {
        let panel = new inkVerticalPanel();
        panel.SetName(n"StatusPanel");
        panel.SetVisible(false);
        
        let label = new inkText();
        label.SetText("Connection Status");
        label.SetFontSize(24);
        panel.AddChild(label);
        
        let statusText = new inkText();
        if ConnectionManager.IsConnected() {
            statusText.SetText("Status: Connected\n" + ConnectionManager.GetConnectionInfo());
        } else {
            statusText.SetText("Status: Not connected");
        }
        panel.AddChild(statusText);
        
        return panel;
    }
    
    private func CreateChatPanel() -> ref<inkWidget> {
        let panel = new inkVerticalPanel();
        panel.SetName(n"ChatPanel");
        panel.SetVisible(false);
        
        let label = new inkText();
        label.SetText("Chat");
        label.SetFontSize(24);
        panel.AddChild(label);
        
        let infoText = new inkText();
        infoText.SetText("Chat functionality would be implemented here.");
        panel.AddChild(infoText);
        
        return panel;
    }
    
    // Tab click handlers
    protected cb func OnServerTabClick(e: ref<inkPointerEvent>) -> Bool {
        ShowServerBrowser();
        return true;
    }
    
    protected cb func OnSettingsTabClick(e: ref<inkPointerEvent>) -> Bool {
        ShowSettings();
        return true;
    }
    
    protected cb func OnStatusTabClick(e: ref<inkPointerEvent>) -> Bool {
        ShowStatus();
        return true;
    }
    
    protected cb func OnChatTabClick(e: ref<inkPointerEvent>) -> Bool {
        ShowChat();
        return true;
    }
    
    protected cb func OnCloseClick(e: ref<inkPointerEvent>) -> Bool {
        Hide();
        return true;
    }
    
    // Panel switching functions
    private func ShowServerBrowser() -> Void {
        HideAllPanels();
        if IsDefined(serverBrowserPanel) {
            serverBrowserPanel.SetVisible(true);
            currentPanel = serverBrowserPanel;
        }
    }
    
    private func ShowSettings() -> Void {
        HideAllPanels();
        if IsDefined(settingsPanel) {
            settingsPanel.SetVisible(true);
            currentPanel = settingsPanel;
        }
    }
    
    private func ShowStatus() -> Void {
        HideAllPanels();
        if IsDefined(statusPanel) {
            statusPanel.SetVisible(true);
            currentPanel = statusPanel;
            UpdateStatusPanel();
        }
    }
    
    private func ShowChat() -> Void {
        HideAllPanels();
        if IsDefined(chatPanel) {
            chatPanel.SetVisible(true);
            currentPanel = chatPanel;
        }
    }
    
    private func HideAllPanels() -> Void {
        if IsDefined(serverBrowserPanel) { serverBrowserPanel.SetVisible(false); }
        if IsDefined(settingsPanel) { settingsPanel.SetVisible(false); }
        if IsDefined(statusPanel) { statusPanel.SetVisible(false); }
        if IsDefined(chatPanel) { chatPanel.SetVisible(false); }
    }
    
    private func UpdateStatusPanel() -> Void {
        if !IsDefined(statusPanel) {
            return;
        }
        
        // Update status information
        // This would be called periodically to refresh connection status
        LogChannel(n"COOP_UI", "Status panel updated");
    }
    
    public func OnUpdate(dt: Float) -> Void {
        if !isVisible {
            return;
        }
        
        // Handle ESC key to close UI
        let input = GameInstance.GetInputSystem(GetGame());
        if IsDefined(input) && input.IsActionJustPressed(n"cancel") {
            Hide();
        }
        
        // Update connection manager
        ConnectionManager.Update();
        
        // Update status panel if visible
        if IsDefined(currentPanel) && Equals(currentPanel, statusPanel) {
            UpdateStatusPanel();
        }
    }
    
    public static func IsVisible() -> Bool {
        return isVisible;
    }
    
    public static func GetInstance() -> ref<CoopUIManager> {
        return instance;
    }
}