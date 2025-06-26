public class HoloBars extends inkHUDLayer {
    private static let s_instance: ref<HoloBars>;
    private let top: ref<inkRectangle>;
    private let bottom: ref<inkRectangle>;

    public static func Show() -> Void {
        if IsDefined(s_instance) { return; };
        let layer = new HoloBars();
        s_instance = layer;
        layer.top = new inkRectangle();
        layer.top.SetSize(1920.0, 120.0);
        layer.top.SetTintColor(new HDRColor(0.0, 0.0, 0.0, 1.0));
        layer.top.SetAnchor(inkEAnchor.TopCenter);
        layer.AddChild(layer.top);

        layer.bottom = new inkRectangle();
        layer.bottom.SetSize(1920.0, 120.0);
        layer.bottom.SetTintColor(new HDRColor(0.0, 0.0, 0.0, 1.0));
        layer.bottom.SetAnchor(inkEAnchor.BottomCenter);
        layer.AddChild(layer.bottom);

        let hud = GameInstance.GetHUDManager(GetGame());
        // Adds cinematic black bars without pausing gameplay.
        hud.AddLayer(layer);
    }

    public static func Hide() -> Void {
        if !IsDefined(s_instance) { return; };
        let hud = GameInstance.GetHUDManager(GetGame());
        hud.RemoveLayer(s_instance);
        s_instance = null;
    }
}
