// Codeware.UI import commented out to make it optional
// import Codeware.UI

// Import required coop classes
import ConnectionManager
import CoopUIManager
import CoopSettings

public class MainMenuInjection {
    public static func Inject() -> Void {
        let uiSys = GameInstance.GetUISystem(GetGame());
        if !IsDefined(uiSys) {
            LogChannel(n"DEBUG", "[MainMenuInjection] UISystem not found");
            return;
        };
        
        // Check if Codeware is available before using it
        LogChannel(n"INFO", "[MainMenuInjection] Basic UI injection (Codeware disabled)");

        let menu = uiSys.GetMenu(n"MainMenu") as wref<inkMenuLayer>;
        if !IsDefined(menu) {
            LogChannel(n"DEBUG", "[MainMenuInjection] main menu not yet available");
            return;
        };

        let ctrl = menu.GetController() as MainMenuController;
        if !IsDefined(ctrl) {
            LogChannel(n"DEBUG", "[MainMenuInjection] controller cast failed");
            return;
        };

        if IsDefined(ctrl.GetRootCompoundWidget().GetWidget(n"coopBtn")) {
            return; // already injected
        };

        let coopBtn = new inkButton();
        coopBtn.SetName(n"coopBtn");
        coopBtn.SetText("🌐 COOPERATIVE");
        coopBtn.SetStyle(n"BaseButtonMedium");
        coopBtn.RegisterToCallback(n"OnRelease", ctrl, n"OnCoop");
        ctrl.GetRootCompoundWidget().AddChild(coopBtn);
        LogChannel(n"DEBUG", "[MainMenuInjection] CO-OP button added");
    }
}

@wrapMethod(MainMenuController, OnInitialize)
public func OnInitialize() -> Void {
    wrappedMethod();
    
    // Initialize connection manager
    ConnectionManager.Initialize();
    
    // Create modern coop button
    let coopBtn = new inkButton();
    coopBtn.SetName(n"coopBtn");
    coopBtn.SetText("🌐 COOPERATIVE");
    coopBtn.SetStyle(n"BaseButtonMedium");
    coopBtn.SetSize(new Vector2(200.0, 50.0));
    
    // Style the button with CP2077 theme
    let buttonController = coopBtn.GetController() as inkButtonController;
    if IsDefined(buttonController) {
        buttonController.SetHoverTintColor(new HDRColor(0.368627, 0.964706, 1.0, 0.8));
        buttonController.SetPressTintColor(new HDRColor(0.368627, 0.964706, 1.0, 1.0));
    }
    
    coopBtn.RegisterToCallback(n"OnRelease", this, n"OnCoop");
    this.GetRootCompoundWidget().AddChild(coopBtn);
    
    LogChannel(n"COOP_UI", "Modern cooperative button added to main menu");
}

@addMethod(MainMenuController)
public func OnCoop(e: ref<inkPointerEvent>) -> Void {
    // Ensure gameplay continues even if the pause menu invoked this button
    GameInstance.GetTimeSystem(GetGame()).SetLocalTimeDilation(1.0);
    
    // Play button sound
    let audioSystem = GameInstance.GetAudioSystem(GetGame());
    if IsDefined(audioSystem) {
        let audioEvent = new AudioEvent();
        audioEvent.eventName = n"ui_menu_onpress";
        audioSystem.Play(audioEvent);
    }
    
    // Show modern coop UI
    CoopUIManager.Show();
    
    LogChannel(n"COOP_UI", "Coop UI opened from main menu");
}

// Initialize systems when the game starts
@wrapMethod(MainMenuController, OnSetMenuEventDispatcher)
public func OnSetMenuEventDispatcher(menuEventDispatcher: wref<inkMenuEventDispatcher>) -> Void {
    wrappedMethod(menuEventDispatcher);
    
    // Initialize coop systems
    LogChannel(n"COOP_UI", "Initializing cooperative mode systems...");
    
    // Initialize connection manager
    ConnectionManager.Initialize();
    
    // Initialize other systems as needed
    LogChannel(n"COOP_UI", "Cooperative mode systems initialized");
}
