public class MicIcon extends inkHUDLayer {
    private static let s_instance: ref<MicIcon>;
    private let rect: ref<inkRectangle>;

    public static func Show() -> Void {
        if IsDefined(s_instance) { return; };
        let l = new MicIcon();
        s_instance = l;
        l.rect = new inkRectangle();
        l.rect.SetSize(32.0, 32.0);
        l.rect.SetTintColor(new HDRColor(1.0, 1.0, 1.0, 1.0));
        l.rect.SetAnchor(inkEAnchor.BottomRight);
        l.AddChild(l.rect);
        GameInstance.GetHUDManager(GetGame()).AddLayer(l);
    }

    public static func Hide() -> Void {
        if !IsDefined(s_instance) { return; };
        GameInstance.GetHUDManager(GetGame()).RemoveLayer(s_instance);
        s_instance = null;
    }
}
