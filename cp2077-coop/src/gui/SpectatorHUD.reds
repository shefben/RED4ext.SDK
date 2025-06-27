public class SpectatorHUD extends inkGameController {
    private let hpText: wref<inkText>;
    private let warnText: wref<inkText>;
    private let target: Uint32;

    public func OnCreate() -> Void {
        let root = this.GetRootWidget();
        let c = new inkCanvas();
        root.AddChild(c);
        hpText = new inkText();
        c.AddChild(hpText);
        c.SetAnchor(inkEAnchor.CenterLeft);
        c.SetMargin(new inkMargin(20.0, 20.0, 0.0, 0.0));
        warnText = new inkText();
        warnText.SetMargin(new inkMargin(20.0, 60.0, 0.0, 0.0));
        warnText.SetVisible(false);
        c.AddChild(warnText);
    }

    public func SetTarget(id: Uint32) -> Void {
        target = id;
    }

    public func OnUpdate(dt: Float) -> Void {
        if target == 0u { return; };
        let avatar = GameInstance.GetPlayerSystem(GetGame()).FindObject(target) as AvatarProxy;
        if IsDefined(avatar) {
            hpText.SetText("HP: " + IntToString(avatar.health));
        };
    }

    public func ShowWarning(msg: String) -> Void {
        if IsDefined(warnText) {
            warnText.SetText(msg);
            warnText.SetVisible(true);
        };
    }

    public func HideWarning() -> Void {
        if IsDefined(warnText) {
            warnText.SetVisible(false);
        };
    }
}
