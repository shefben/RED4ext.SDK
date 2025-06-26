public class SpectatorHUD extends inkGameController {
    private let hpText: wref<inkText>;
    private let target: Uint32;

    public func OnCreate() -> Void {
        let root = this.GetRootWidget();
        let c = new inkCanvas();
        root.AddChild(c);
        hpText = new inkText();
        c.AddChild(hpText);
        c.SetAnchor(inkEAnchor.CenterLeft);
        c.SetMargin(new inkMargin(20.0, 20.0, 0.0, 0.0));
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
}
