public class SyncProgress extends inkHUDLayer {
    private static let s_instance: ref<SyncProgress>;
    private let label: ref<inkText>;

    public static func Show() -> Void {
        if IsDefined(s_instance) { return; };
        let hud = GameInstance.GetHUDManager(GetGame());
        let o = new SyncProgress();
        o.label = new inkText();
        o.label.SetStyle(n"bold 36px");
        o.label.SetAnchor(inkEAnchor.Centered);
        o.label.SetText("Syncing 0%");
        o.AddChild(o.label);
        hud.AddLayer(o);
        s_instance = o;
    }

    public static func Update(pct: Int32) -> Void {
        if !IsDefined(s_instance) { return; };
        s_instance.label.SetText("Syncing " + IntToString(pct) + "%");
    }

    public static func Hide() -> Void {
        if !IsDefined(s_instance) { return; };
        let hud = GameInstance.GetHUDManager(GetGame());
        hud.RemoveLayer(s_instance);
        s_instance = null;
    }
}

public static func SyncProgress_Show() -> Void {
    SyncProgress.Show();
}

public static func SyncProgress_Update(pct: Int32) -> Void {
    SyncProgress.Update(pct);
}

public static func SyncProgress_Hide() -> Void {
    SyncProgress.Hide();
}
