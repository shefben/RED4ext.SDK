public class LootMarkers extends inkHUDLayer {
    public static let s_instance: ref<LootMarkers>;
    private let root: ref<inkVerticalPanel>;
    private let lines: array<ref<inkText>>;
    private let timers: array<Float>;
    private const life: Float = 4.0;

    public static func Instance() -> ref<LootMarkers> {
        if !IsDefined(s_instance) {
            s_instance = new LootMarkers();
            GameInstance.GetHUDManager(GetGame()).AddLayer(s_instance);
        };
        return s_instance;
    }

    public static func Push(msg: String) -> Void {
        Instance().Add(msg);
    }

    private func Add(msg: String) -> Void {
        if !IsDefined(root) {
            root = new inkVerticalPanel();
            root.SetAnchor(inkEAnchor.TopCenter);
            root.SetTranslation(new Vector2(0.0, 300.0));
            AddChild(root);
        };
        let t = new inkText();
        t.SetText(msg);
        t.SetOpacity(1.0);
        root.AddChild(t);
        lines.PushBack(t);
        timers.PushBack(life);
    }

    public func OnUpdate(dt: Float) -> Void {
        var i: Int32 = 0;
        while i < timers.Size() {
            timers[i] -= dt;
            if timers[i] <= 0.0 {
                root.RemoveChild(lines[i]);
                lines.Erase(i);
                timers.Erase(i);
            } else {
                if timers[i] < 1.0 { lines[i].SetOpacity(timers[i]); };
                i += 1;
            };
        };
    }
}
