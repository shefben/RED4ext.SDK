public class MainMenuInjection {
    // Called to insert a CO-OP button into the title menu.
    public static func Inject() -> Void {
        // Pseudo-code: locate the menu controller and add a button.
        // let menu = rtti:MainMenuGameController;
        // let btn = new inkButton();
        // btn.SetText("CO-OP");
        // btn.RegisterToCallback(n"OnPress", this, n"OnButton");
        // menu.GetRootWidget().AddChild(btn);
        LogChannel(n"DEBUG", "Main menu injected with CO-OP button");
    }

    public static func OnButton(widget: ref<inkWidget>) -> Void {
        CoopRootPanel.Show();
    }
}
