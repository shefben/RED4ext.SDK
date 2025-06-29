import Codeware.UI

public class MainMenuInjection {
    public static func Inject() -> Void {
        // legacy stub retained for compatibility
    }
}

@wrapMethod(MainMenuController, OnInitialize)
public func OnInitialize() -> Void {
    wrappedMethod();
    let coopBtn = new inkText();
    coopBtn.SetText("CO-OP");
    coopBtn.SetStyle(n"BaseButtonMedium");
    coopBtn.RegisterToCallback(n"OnRelease", this, n"OnCoop");
    this.GetRootCompoundWidget().AddChild(coopBtn);
}

public func OnCoop(e: ref<inkPointerEvent>) -> Void {
    // ensure gameplay continues even if the pause menu invoked this button
    GameInstance.GetTimeSystem(GetGame()).SetLocalTimeDilation(1.0);
    ServerBrowser.Show();
}
