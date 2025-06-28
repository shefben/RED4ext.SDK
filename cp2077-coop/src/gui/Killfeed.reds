public class Killfeed extends inkHUDLayer {
    public static let s_instance: ref<Killfeed>;
    private var root: ref<inkVerticalPanel>;
    private var lines: array<ref<inkText>>;
    private var timers: array<Float>;
    private const maxLines: Int32 = 20;

    public static func Instance() -> ref<Killfeed> {
        if !IsDefined(s_instance) {
            s_instance = new Killfeed();
        };
        return s_instance;
    }

    public static func Push(msg: String) -> Void {
        Instance().AddLine(msg);
        LogChannel(n"DEBUG", "Killfeed: " + msg);
    }

    private func AddLine(msg: String) -> Void {
        if !IsDefined(root) {
            root = new inkVerticalPanel();
            root.SetAnchor(inkEAnchor.TopRight);
            root.SetTranslation(new Vector2(-20.0, 200.0));
            AddChild(root);
        };
        let t = new inkText();
        t.SetText(msg);
        t.SetOpacity(1.0);
        root.AddChild(t);
        lines.PushBack(t);
        timers.PushBack(5.0);
        while lines.Size() > maxLines {
            root.RemoveChild(lines[0]);
            lines.Erase(0);
            timers.Erase(0);
        };
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
                if timers[i] < 1.0 {
                    lines[i].SetOpacity(timers[i]);
                };
                i += 1;
            };
        };
    }
}
