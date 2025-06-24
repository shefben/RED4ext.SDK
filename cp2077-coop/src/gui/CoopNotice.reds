public class CoopNotice extends inkHUDLayer {
    private static let s_instance: ref<CoopNotice>;
    private let label: ref<inkText>;

    public static func Show(popupName: String) -> Void {
        let message: String = popupName + " disabled in co-op";
        if IsDefined(s_instance) {
            s_instance.label.SetText(message);
            GameInstance.GetDelaySystem(GetGame()).DelayCallback(s_instance, n"Hide", 3.0);
            return;
        };
        let layer = new CoopNotice();
        s_instance = layer;
        layer.label = new inkText();
        layer.label.SetStyle(n"bold 36px");
        layer.label.SetText(message);
        layer.label.SetAnchor(inkEAnchor.Centered);
        layer.AddChild(layer.label);
        // Attach to HUD so the text appears in the center of the screen.
        let hud = GameInstance.GetHUDManager(GetGame());
        hud.AddLayer(layer);
        LogChannel(n"DEBUG", message);
        GameInstance.GetDelaySystem(GetGame()).DelayCallback(layer, n"Hide", 3.0);
    }

    protected func Hide() -> Void {
        let hud = GameInstance.GetHUDManager(GetGame());
        hud.RemoveLayer(this);
        s_instance = null;
    }
}
