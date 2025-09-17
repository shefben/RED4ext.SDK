// Server hosting dialog with comprehensive configuration options
// Allows users to easily create and configure their own multiplayer servers

import Codeware.UI

public struct HostServerConfig {
    public var serverName: String;
    public var description: String;
    public var port: Uint32;
    public var maxPlayers: Uint32;
    public var password: String;
    public var gameMode: String;
    public var region: String;
    public var publicServer: Bool;
    public var enableAntiCheat: Bool;
    public var enableVoiceChat: Bool;
    public var welcomeMessage: String;
}

public class ServerHostDialog {
    private var container: wref<inkCanvas>;
    private var parentUI: wref<CoopUI>;
    
    // UI Components
    private var configPanel: wref<inkScrollArea>;
    private var actionPanel: wref<inkHorizontalPanel>;
    
    // Configuration inputs
    private var serverNameInput: wref<inkTextInput>;
    private var descriptionInput: wref<inkTextInput>;
    private var portInput: wref<inkTextInput>;
    private var maxPlayersSlider: wref<inkSlider>;
    private var passwordInput: wref<inkTextInput>;
    private var gameModeDropdown: wref<inkButton>; // Acts as dropdown
    private var regionDropdown: wref<inkButton>;
    private var publicServerToggle: wref<inkToggle>;
    private var antiCheatToggle: wref<inkToggle>;
    private var voiceChatToggle: wref<inkToggle>;
    private var welcomeMessageInput: wref<inkTextInput>;
    
    // Action buttons
    private var hostButton: wref<inkButton>;
    private var hostDedicatedButton: wref<inkButton>;
    private var loadPresetButton: wref<inkButton>;
    private var savePresetButton: wref<inkButton>;
    private var resetButton: wref<inkButton>;
    
    // Status display
    private var statusText: wref<inkText>;
    private var portCheckText: wref<inkText>;
    
    // Configuration state
    private var currentConfig: HostServerConfig;
    private var selectedGameMode: String;
    private var selectedRegion: String;
    
    public func Initialize(parent: wref<inkCanvas>, ui: wref<CoopUI>) -> Void {
        this.parentUI = ui;
        this.InitializeDefaults();
        this.CreateUI(parent);
    }
    
    private func InitializeDefaults() -> Void {
        this.currentConfig.serverName = "My Cyberpunk Server";
        this.currentConfig.description = "A cooperative multiplayer server";
        this.currentConfig.port = 7777u;
        this.currentConfig.maxPlayers = 8u;
        this.currentConfig.password = "";
        this.currentConfig.gameMode = "Cooperative";
        this.currentConfig.region = "Auto";
        this.currentConfig.publicServer = true;
        this.currentConfig.enableAntiCheat = true;
        this.currentConfig.enableVoiceChat = true;
        this.currentConfig.welcomeMessage = "Welcome to the server!";
        
        this.selectedGameMode = "Cooperative";
        this.selectedRegion = "Auto";
    }
    
    private func CreateUI(parent: wref<inkCanvas>) -> Void {
        this.container = new inkCanvas();
        this.container.SetName(n"ServerHostContainer");
        this.container.SetAnchor(inkEAnchor.Fill);
        this.container.SetVisible(false);
        parent.AddChild(this.container);
        
        // Main vertical layout
        let mainPanel = new inkVerticalPanel();
        mainPanel.SetAnchor(inkEAnchor.Fill);
        mainPanel.SetChildMargin(new inkMargin(0.0, 10.0, 0.0, 10.0));
        this.container.AddChild(mainPanel);
        
        // Title
        let titleText = new inkText();
        titleText.SetText("HOST SERVER");
        titleText.SetFontSize(28);
        titleText.SetTintColor(new HDRColor(0.368627, 0.964706, 1.0, 1.0));
        titleText.SetHorizontalAlignment(textHorizontalAlignment.Center);
        mainPanel.AddChild(titleText);
        
        // Configuration scroll area
        this.configPanel = new inkScrollArea();
        this.configPanel.SetSize(new Vector2(0.0, 400.0));
        mainPanel.AddChild(this.configPanel);
        
        let configContainer = new inkVerticalPanel();
        configContainer.SetChildMargin(new inkMargin(0.0, 5.0, 0.0, 5.0));
        this.configPanel.AddChild(configContainer);
        
        this.CreateConfigurationInputs(configContainer);
        
        // Status area
        this.CreateStatusArea(mainPanel);
        
        // Action buttons
        this.CreateActionButtons(mainPanel);
        
        // Load saved config if exists
        this.LoadSavedConfig();
    }
    
    private func CreateConfigurationInputs(parent: wref<inkVerticalPanel>) -> Void {
        // Server Identity Section
        this.CreateSectionHeader(parent, "SERVER IDENTITY");
        
        this.serverNameInput = this.CreateTextInput(parent, "Server Name:", this.currentConfig.serverName, "Enter a name for your server");
        this.serverNameInput.RegisterToCallback(n"OnInput", this, n"OnServerNameChanged");
        
        this.descriptionInput = this.CreateTextInput(parent, "Description:", this.currentConfig.description, "Describe your server");
        this.descriptionInput.RegisterToCallback(n"OnInput", this, n"OnDescriptionChanged");
        
        this.welcomeMessageInput = this.CreateTextInput(parent, "Welcome Message:", this.currentConfig.welcomeMessage, "Message shown to new players");
        this.welcomeMessageInput.RegisterToCallback(n"OnInput", this, n"OnWelcomeMessageChanged");
        
        // Network Settings Section
        this.CreateSectionHeader(parent, "NETWORK SETTINGS");
        
        this.portInput = this.CreateTextInput(parent, "Port:", IntToString(this.currentConfig.port), "Server port (1024-65535)");
        this.portInput.RegisterToCallback(n"OnInput", this, n"OnPortChanged");
        
        this.maxPlayersSlider = this.CreateSlider(parent, "Max Players:", Cast<Float>(this.currentConfig.maxPlayers), 2.0, 32.0, 1.0);
        this.maxPlayersSlider.RegisterToCallback(n"OnValueChanged", this, n"OnMaxPlayersChanged");
        
        this.passwordInput = this.CreateTextInput(parent, "Password (Optional):", this.currentConfig.password, "Leave empty for no password");
        this.passwordInput.RegisterToCallback(n"OnInput", this, n"OnPasswordChanged");
        
        // Game Settings Section
        this.CreateSectionHeader(parent, "GAME SETTINGS");
        
        this.gameModeDropdown = this.CreateDropdown(parent, "Game Mode:", this.selectedGameMode);
        this.gameModeDropdown.RegisterToCallback(n"OnRelease", this, n"OnGameModeClicked");
        
        this.regionDropdown = this.CreateDropdown(parent, "Region:", this.selectedRegion);
        this.regionDropdown.RegisterToCallback(n"OnRelease", this, n"OnRegionClicked");
        
        // Feature Toggles Section
        this.CreateSectionHeader(parent, "FEATURES");
        
        this.publicServerToggle = this.CreateToggle(parent, "Public Server:", this.currentConfig.publicServer, "List server in public browser");
        this.publicServerToggle.RegisterToCallback(n"OnValueChanged", this, n"OnPublicServerToggled");
        
        this.antiCheatToggle = this.CreateToggle(parent, "Anti-Cheat:", this.currentConfig.enableAntiCheat, "Enable cheat detection and prevention");
        this.antiCheatToggle.RegisterToCallback(n"OnValueChanged", this, n"OnAntiCheatToggled");
        
        this.voiceChatToggle = this.CreateToggle(parent, "Voice Chat:", this.currentConfig.enableVoiceChat, "Enable in-game voice communication");
        this.voiceChatToggle.RegisterToCallback(n"OnValueChanged", this, n"OnVoiceChatToggled");
    }
    
    private func CreateSectionHeader(parent: wref<inkVerticalPanel>, title: String) -> Void {
        let header = new inkText();
        header.SetText(title);
        header.SetFontSize(20);
        header.SetTintColor(new HDRColor(0.368627, 0.964706, 1.0, 1.0));
        header.SetMargin(new inkMargin(0.0, 15.0, 0.0, 5.0));
        parent.AddChild(header);
    }
    
    private func CreateTextInput(parent: wref<inkVerticalPanel>, label: String, value: String, tooltip: String) -> ref<inkTextInput> {
        let row = new inkHorizontalPanel();
        row.SetChildMargin(new inkMargin(10.0, 5.0, 10.0, 5.0));
        parent.AddChild(row);
        
        let labelText = new inkText();
        labelText.SetText(label);
        labelText.SetSize(new Vector2(150.0, 30.0));
        labelText.SetFontSize(16);
        labelText.SetVerticalAlignment(textVerticalAlignment.Center);
        row.AddChild(labelText);
        
        let input = new inkTextInput();
        input.SetText(value);
        input.SetSize(new Vector2(300.0, 30.0));
        row.AddChild(input);
        
        if tooltip != "" {
            let tooltipText = new inkText();
            tooltipText.SetText(tooltip);
            tooltipText.SetFontSize(12);
            tooltipText.SetTintColor(new HDRColor(0.7, 0.7, 0.7, 1.0));
            tooltipText.SetMargin(new inkMargin(10.0, 0.0, 0.0, 0.0));
            row.AddChild(tooltipText);
        }
        
        return input;
    }
    
    private func CreateSlider(parent: wref<inkVerticalPanel>, label: String, value: Float, min: Float, max: Float, step: Float) -> ref<inkSlider> {
        let row = new inkHorizontalPanel();
        row.SetChildMargin(new inkMargin(10.0, 5.0, 10.0, 5.0));
        parent.AddChild(row);
        
        let labelText = new inkText();
        labelText.SetText(label);
        labelText.SetSize(new Vector2(150.0, 30.0));
        labelText.SetFontSize(16);
        labelText.SetVerticalAlignment(textVerticalAlignment.Center);
        row.AddChild(labelText);
        
        let slider = new inkSlider();
        slider.SetValue(value);
        slider.SetMinValue(min);
        slider.SetMaxValue(max);
        slider.SetStep(step);
        slider.SetSize(new Vector2(200.0, 30.0));
        row.AddChild(slider);
        
        let valueText = new inkText();
        valueText.SetText(IntToString(Cast<Int32>(value)));
        valueText.SetName(n"SliderValue");
        valueText.SetSize(new Vector2(50.0, 30.0));
        valueText.SetFontSize(16);
        valueText.SetVerticalAlignment(textVerticalAlignment.Center);
        row.AddChild(valueText);
        
        return slider;
    }
    
    private func CreateDropdown(parent: wref<inkVerticalPanel>, label: String, value: String) -> ref<inkButton> {
        let row = new inkHorizontalPanel();
        row.SetChildMargin(new inkMargin(10.0, 5.0, 10.0, 5.0));
        parent.AddChild(row);
        
        let labelText = new inkText();
        labelText.SetText(label);
        labelText.SetSize(new Vector2(150.0, 30.0));
        labelText.SetFontSize(16);
        labelText.SetVerticalAlignment(textVerticalAlignment.Center);
        row.AddChild(labelText);
        
        let button = new inkButton();
        button.SetText(value + " ▼");
        button.SetSize(new Vector2(200.0, 30.0));
        button.SetHorizontalAlignment(textHorizontalAlignment.Left);
        row.AddChild(button);
        
        return button;
    }
    
    private func CreateToggle(parent: wref<inkVerticalPanel>, label: String, value: Bool, tooltip: String) -> ref<inkToggle> {
        let row = new inkHorizontalPanel();
        row.SetChildMargin(new inkMargin(10.0, 5.0, 10.0, 5.0));
        parent.AddChild(row);
        
        let labelText = new inkText();
        labelText.SetText(label);
        labelText.SetSize(new Vector2(150.0, 30.0));
        labelText.SetFontSize(16);
        labelText.SetVerticalAlignment(textVerticalAlignment.Center);
        row.AddChild(labelText);
        
        let toggle = new inkToggle();
        toggle.SetToggleValue(value);
        toggle.SetSize(new Vector2(50.0, 30.0));
        row.AddChild(toggle);
        
        if tooltip != "" {
            let tooltipText = new inkText();
            tooltipText.SetText(tooltip);
            tooltipText.SetFontSize(12);
            tooltipText.SetTintColor(new HDRColor(0.7, 0.7, 0.7, 1.0));
            tooltipText.SetMargin(new inkMargin(10.0, 0.0, 0.0, 0.0));
            row.AddChild(tooltipText);
        }
        
        return toggle;
    }
    
    private func CreateStatusArea(parent: wref<inkVerticalPanel>) -> Void {
        let statusPanel = new inkVerticalPanel();
        statusPanel.SetSize(new Vector2(0.0, 80.0));
        statusPanel.SetChildMargin(new inkMargin(0.0, 5.0, 0.0, 5.0));
        parent.AddChild(statusPanel);
        
        this.statusText = new inkText();
        this.statusText.SetText("Configure your server settings above");
        this.statusText.SetFontSize(16);
        this.statusText.SetHorizontalAlignment(textHorizontalAlignment.Center);
        this.statusText.SetTintColor(new HDRColor(0.8, 0.8, 0.8, 1.0));
        statusPanel.AddChild(this.statusText);
        
        this.portCheckText = new inkText();
        this.portCheckText.SetText("");
        this.portCheckText.SetFontSize(14);
        this.portCheckText.SetHorizontalAlignment(textHorizontalAlignment.Center);
        statusPanel.AddChild(this.portCheckText);
    }
    
    private func CreateActionButtons(parent: wref<inkVerticalPanel>) -> Void {
        this.actionPanel = new inkHorizontalPanel();
        this.actionPanel.SetChildMargin(new inkMargin(10.0, 10.0, 10.0, 10.0));
        this.actionPanel.SetAnchor(inkEAnchor.CenterFillHorizontal);
        parent.AddChild(this.actionPanel);
        
        // Main hosting buttons
        this.hostButton = new inkButton();
        this.hostButton.SetText("🎮 HOST SERVER");
        this.hostButton.SetSize(new Vector2(150.0, 50.0));
        this.hostButton.SetStyle(n"ButtonPrimary");
        this.hostButton.RegisterToCallback(n"OnRelease", this, n"OnHostServerClicked");
        this.actionPanel.AddChild(this.hostButton);
        
        this.hostDedicatedButton = new inkButton();
        this.hostDedicatedButton.SetText("🖥️ HOST DEDICATED");
        this.hostDedicatedButton.SetSize(new Vector2(170.0, 50.0));
        this.hostDedicatedButton.SetStyle(n"ButtonSecondary");
        this.hostDedicatedButton.RegisterToCallback(n"OnRelease", this, n"OnHostDedicatedClicked");
        this.actionPanel.AddChild(this.hostDedicatedButton);
        
        // Configuration management buttons
        this.loadPresetButton = new inkButton();
        this.loadPresetButton.SetText("📁 LOAD PRESET");
        this.loadPresetButton.SetSize(new Vector2(130.0, 50.0));
        this.loadPresetButton.RegisterToCallback(n"OnRelease", this, n"OnLoadPresetClicked");
        this.actionPanel.AddChild(this.loadPresetButton);
        
        this.savePresetButton = new inkButton();
        this.savePresetButton.SetText("💾 SAVE PRESET");
        this.savePresetButton.SetSize(new Vector2(130.0, 50.0));
        this.savePresetButton.RegisterToCallback(n"OnRelease", this, n"OnSavePresetClicked");
        this.actionPanel.AddChild(this.savePresetButton);
        
        this.resetButton = new inkButton();
        this.resetButton.SetText("🔄 RESET");
        this.resetButton.SetSize(new Vector2(100.0, 50.0));
        this.resetButton.RegisterToCallback(n"OnRelease", this, n"OnResetClicked");
        this.actionPanel.AddChild(this.resetButton);
    }
    
    // Event Handlers
    protected cb func OnServerNameChanged(widget: ref<inkWidget>) -> Bool {
        this.currentConfig.serverName = (widget as inkTextInput).GetText();
        this.UpdateStatus();
        return true;
    }
    
    protected cb func OnDescriptionChanged(widget: ref<inkWidget>) -> Bool {
        this.currentConfig.description = (widget as inkTextInput).GetText();
        return true;
    }
    
    protected cb func OnWelcomeMessageChanged(widget: ref<inkWidget>) -> Bool {
        this.currentConfig.welcomeMessage = (widget as inkTextInput).GetText();
        return true;
    }
    
    protected cb func OnPortChanged(widget: ref<inkWidget>) -> Bool {
        let portStr = (widget as inkTextInput).GetText();
        let port = StringToInt(portStr);
        if port >= 1024 && port <= 65535 {
            this.currentConfig.port = Cast<Uint32>(port);
            this.CheckPortAvailability();
        } else {
            this.portCheckText.SetText("Port must be between 1024-65535");
            this.portCheckText.SetTintColor(new HDRColor(1.0, 0.4, 0.4, 1.0));
        }
        return true;
    }
    
    protected cb func OnMaxPlayersChanged(widget: ref<inkWidget>) -> Bool {
        let slider = widget as inkSlider;
        let value = Cast<Int32>(slider.GetValue());
        this.currentConfig.maxPlayers = Cast<Uint32>(value);
        
        // Update value display
        let valueText = slider.GetParent().GetWidget(n"SliderValue") as inkText;
        if IsDefined(valueText) {
            valueText.SetText(IntToString(value));
        }
        
        this.UpdateStatus();
        return true;
    }
    
    protected cb func OnPasswordChanged(widget: ref<inkWidget>) -> Bool {
        this.currentConfig.password = (widget as inkTextInput).GetText();
        return true;
    }
    
    protected cb func OnGameModeClicked(e: ref<inkPointerEvent>) -> Bool {
        GameModeSelectionDialog.Show(this);
        return true;
    }
    
    protected cb func OnRegionClicked(e: ref<inkPointerEvent>) -> Bool {
        RegionSelectionDialog.Show(this);
        return true;
    }
    
    protected cb func OnPublicServerToggled(widget: ref<inkWidget>) -> Bool {
        this.currentConfig.publicServer = (widget as inkToggle).GetToggleValue();
        this.UpdateStatus();
        return true;
    }
    
    protected cb func OnAntiCheatToggled(widget: ref<inkWidget>) -> Bool {
        this.currentConfig.enableAntiCheat = (widget as inkToggle).GetToggleValue();
        return true;
    }
    
    protected cb func OnVoiceChatToggled(widget: ref<inkWidget>) -> Bool {
        this.currentConfig.enableVoiceChat = (widget as inkToggle).GetToggleValue();
        return true;
    }
    
    // Action button handlers
    protected cb func OnHostServerClicked(e: ref<inkPointerEvent>) -> Bool {
        this.ValidateAndHostServer(false);
        return true;
    }
    
    protected cb func OnHostDedicatedClicked(e: ref<inkPointerEvent>) -> Bool {
        this.ValidateAndHostServer(true);
        return true;
    }
    
    protected cb func OnLoadPresetClicked(e: ref<inkPointerEvent>) -> Bool {
        PresetSelectionDialog.Show(this);
        return true;
    }
    
    protected cb func OnSavePresetClicked(e: ref<inkPointerEvent>) -> Bool {
        PresetSaveDialog.Show(this.currentConfig);
        return true;
    }
    
    protected cb func OnResetClicked(e: ref<inkPointerEvent>) -> Bool {
        this.InitializeDefaults();
        this.RefreshUI();
        NotificationManager.ShowInfo("Configuration reset to defaults");
        return true;
    }
    
    // Core functionality
    private func ValidateAndHostServer(dedicated: Bool) -> Void {
        // Validate configuration
        if !this.ValidateConfig() {
            return;
        }
        
        // Save current config
        this.SaveCurrentConfig();
        
        // Start hosting
        if dedicated {
            this.StartDedicatedServer();
        } else {
            this.StartIntegratedServer();
        }
    }
    
    private func ValidateConfig() -> Bool {
        if this.currentConfig.serverName == "" {
            NotificationManager.ShowError("Server name cannot be empty");
            return false;
        }
        
        if this.currentConfig.port < 1024u || this.currentConfig.port > 65535u {
            NotificationManager.ShowError("Port must be between 1024-65535");
            return false;
        }
        
        if this.currentConfig.maxPlayers < 2u || this.currentConfig.maxPlayers > 32u {
            NotificationManager.ShowError("Max players must be between 2-32");
            return false;
        }
        
        return true;
    }
    
    private func StartIntegratedServer() -> Void {
        LogChannel(n"SERVER_HOST", "Starting integrated server: " + this.currentConfig.serverName);
        
        this.statusText.SetText("Starting server...");
        this.statusText.SetTintColor(new HDRColor(1.0, 1.0, 0.4, 1.0));
        
        // Disable UI during startup
        this.SetUIEnabled(false);
        
        // Start server through native integration
        if IntegratedServerManager.StartServer(this.currentConfig) {
            // Switch to connecting state
            this.parentUI.SetState(CoopUIState.Connecting);
            NotificationManager.ShowSuccess("Server started successfully!");
        } else {
            this.statusText.SetText("Failed to start server");
            this.statusText.SetTintColor(new HDRColor(1.0, 0.4, 0.4, 1.0));
            this.SetUIEnabled(true);
            NotificationManager.ShowError("Failed to start server");
        }
    }
    
    private func StartDedicatedServer() -> Void {
        LogChannel(n"SERVER_HOST", "Starting dedicated server: " + this.currentConfig.serverName);
        
        this.statusText.SetText("Starting dedicated server...");
        this.statusText.SetTintColor(new HDRColor(1.0, 1.0, 0.4, 1.0));
        
        // Create server configuration file
        if DedicatedServerManager.CreateConfigFile(this.currentConfig) {
            // Launch dedicated server executable
            if DedicatedServerManager.LaunchDedicatedServer() {
                NotificationManager.ShowSuccess("Dedicated server launched successfully!");
                this.parentUI.SetState(CoopUIState.MainMenu);
            } else {
                this.statusText.SetText("Failed to launch dedicated server");
                this.statusText.SetTintColor(new HDRColor(1.0, 0.4, 0.4, 1.0));
                NotificationManager.ShowError("Failed to launch dedicated server");
            }
        } else {
            this.statusText.SetText("Failed to create server configuration");
            this.statusText.SetTintColor(new HDRColor(1.0, 0.4, 0.4, 1.0));
            NotificationManager.ShowError("Failed to create server configuration");
        }
    }
    
    private func UpdateStatus() -> Void {
        let statusMsg = "Server: " + this.currentConfig.serverName + " | Players: " + IntToString(this.currentConfig.maxPlayers);
        if !this.currentConfig.publicServer {
            statusMsg += " | Private";
        }
        
        this.statusText.SetText(statusMsg);
        this.statusText.SetTintColor(new HDRColor(0.8, 0.8, 0.8, 1.0));
    }
    
    private func CheckPortAvailability() -> Void {
        // This would check if the port is available
        // For now, just show a status message
        this.portCheckText.SetText("Checking port availability...");
        this.portCheckText.SetTintColor(new HDRColor(1.0, 1.0, 0.4, 1.0));
        
        // Simulate port check (in real implementation this would be async)
        GameInstance.GetDelaySystem(GetGame()).DelayCallback(this, n"OnPortCheckComplete", 1.0);
    }
    
    protected cb func OnPortCheckComplete() -> Void {
        // Simulate successful port check
        this.portCheckText.SetText("Port " + IntToString(this.currentConfig.port) + " is available");
        this.portCheckText.SetTintColor(new HDRColor(0.4, 1.0, 0.4, 1.0));
    }
    
    private func SetUIEnabled(enabled: Bool) -> Void {
        this.hostButton.SetEnabled(enabled);
        this.hostDedicatedButton.SetEnabled(enabled);
        this.serverNameInput.SetEnabled(enabled);
        this.descriptionInput.SetEnabled(enabled);
        this.portInput.SetEnabled(enabled);
        this.maxPlayersSlider.SetEnabled(enabled);
        this.passwordInput.SetEnabled(enabled);
        this.gameModeDropdown.SetEnabled(enabled);
        this.regionDropdown.SetEnabled(enabled);
        this.publicServerToggle.SetEnabled(enabled);
        this.antiCheatToggle.SetEnabled(enabled);
        this.voiceChatToggle.SetEnabled(enabled);
        this.welcomeMessageInput.SetEnabled(enabled);
    }
    
    private func RefreshUI() -> Void {
        this.serverNameInput.SetText(this.currentConfig.serverName);
        this.descriptionInput.SetText(this.currentConfig.description);
        this.portInput.SetText(IntToString(this.currentConfig.port));
        this.maxPlayersSlider.SetValue(Cast<Float>(this.currentConfig.maxPlayers));
        this.passwordInput.SetText(this.currentConfig.password);
        this.publicServerToggle.SetToggleValue(this.currentConfig.publicServer);
        this.antiCheatToggle.SetToggleValue(this.currentConfig.enableAntiCheat);
        this.voiceChatToggle.SetToggleValue(this.currentConfig.enableVoiceChat);
        this.welcomeMessageInput.SetText(this.currentConfig.welcomeMessage);
        
        // Update dropdown texts
        this.gameModeDropdown.SetText(this.selectedGameMode + " ▼");
        this.regionDropdown.SetText(this.selectedRegion + " ▼");
        
        this.UpdateStatus();
    }
    
    // Configuration persistence
    private func SaveCurrentConfig() -> Void {
        ConfigManager.SaveHostConfig(this.currentConfig);
    }
    
    private func LoadSavedConfig() -> Void {
        let savedConfig = ConfigManager.LoadHostConfig();
        if IsDefined(savedConfig) {
            this.currentConfig = savedConfig;
            this.RefreshUI();
        }
    }
    
    // Public interface for dialogs
    public func SetGameMode(gameMode: String) -> Void {
        this.selectedGameMode = gameMode;
        this.currentConfig.gameMode = gameMode;
        this.gameModeDropdown.SetText(gameMode + " ▼");
    }
    
    public func SetRegion(region: String) -> Void {
        this.selectedRegion = region;
        this.currentConfig.region = region;
        this.regionDropdown.SetText(region + " ▼");
    }
    
    public func LoadPreset(preset: HostServerConfig) -> Void {
        this.currentConfig = preset;
        this.selectedGameMode = preset.gameMode;
        this.selectedRegion = preset.region;
        this.RefreshUI();
        NotificationManager.ShowSuccess("Preset loaded successfully");
    }
    
    public func Show() -> Void {
        this.container.SetVisible(true);
        this.UpdateStatus();
    }
    
    public func Hide() -> Void {
        this.container.SetVisible(false);
    }
    
    public func Cleanup() -> Void {
        // Unregister all callbacks and clean up resources
        LogChannel(n"SERVER_HOST", "ServerHostDialog cleaned up");
    }
}