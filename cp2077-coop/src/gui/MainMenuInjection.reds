import Codeware.UI

public class MainMenuInjection {
    public static func Inject() -> Void {
        let uiSys = GameInstance.GetUISystem(GetGame());
        if !IsDefined(uiSys) {
            LogChannel(n"DEBUG", "[MainMenuInjection] UISystem not found");
            return;
        };

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
        coopBtn.SetText("CO-OP");
        coopBtn.SetStyle(n"BaseButtonMedium");
        coopBtn.RegisterToCallback(n"OnRelease", ctrl, n"OnCoop");
        ctrl.GetRootCompoundWidget().AddChild(coopBtn);
        LogChannel(n"DEBUG", "[MainMenuInjection] CO-OP button added");
    }
}

@wrapMethod(MainMenuController, OnInitialize)
public func OnInitialize() -> Void {
    wrappedMethod();
    let coopBtn = new inkButton();
    coopBtn.SetName(n"coopBtn");
    coopBtn.SetText("CO-OP");
    coopBtn.SetStyle(n"BaseButtonMedium");
    coopBtn.RegisterToCallback(n"OnRelease", this, n"OnCoop");
    this.GetRootCompoundWidget().AddChild(coopBtn);
}

@addMethod(MainMenuController)
public func OnCoop(e: ref<inkPointerEvent>) -> Void {
    // ensure gameplay continues even if the pause menu invoked this button
    GameInstance.GetTimeSystem(GetGame()).SetLocalTimeDilation(1.0);
    ServerBrowser.Show();
}
