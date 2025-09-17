public class BusyOverlay extends inkHUDLayer {
    private static let s_instance: ref<BusyOverlay>;
    private let label: ref<inkText>;

    public static func Show(msg: String) -> Void {
        if IsDefined(s_instance) { return; };
        let hud = GameInstance.GetHUDManager(GetGame());
        let o = new BusyOverlay();
        o.label = new inkText();
        o.label.SetText(msg);
        o.label.SetFontSize(40);
        o.label.SetAnchor(inkEAnchor.Centered);
        o.AddChild(o.label);
        hud.AddLayer(o);
        s_instance = o;
        GameInstance.GetDelaySystem(GetGame()).DelayCallback(o, n"Hide", 3.0);
    }

    protected func Hide() -> Void {
        let hud = GameInstance.GetHUDManager(GetGame());
        hud.RemoveLayer(this);
        s_instance = null;
    }
}

public static func BusyOverlay_Show(msg: String) -> Void {
    BusyOverlay.Show(msg);
}
