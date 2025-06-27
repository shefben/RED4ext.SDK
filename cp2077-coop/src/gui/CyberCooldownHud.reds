public class CyberCooldownHud extends inkHUDLayer {
    public static let s_instance: ref<CyberCooldownHud>;
    private let canvas: ref<inkCanvas>;
    private let bars: array<ref<inkCircleProgress>>;
    private let times: array<Float>;
    private let maxTimes: array<Float>;

    public static func Instance() -> ref<CyberCooldownHud> {
        if !IsDefined(s_instance) {
            s_instance = new CyberCooldownHud();
            GameInstance.GetHUDManager(GetGame()).AddLayer(s_instance);
        };
        return s_instance;
    }

    public static func SetCooldown(slot: Uint8, sec: Float) -> Void {
        let inst = Instance();
        inst.EnsureUI(slot);
        inst.times[slot] = sec;
        inst.maxTimes[slot] = sec;
        inst.bars[slot].SetValue(1.0);
    }

    private func EnsureUI(slot: Uint8) -> Void {
        if !IsDefined(canvas) {
            canvas = new inkCanvas();
            canvas.SetAnchor(inkEAnchor.TopRight);
            canvas.SetMargin(new inkMargin(0.0, 200.0, 40.0, 0.0));
            AddChild(canvas);
        };
        while bars.Size() <= Cast<Int32>(slot) {
            let bar = new inkCircleProgress();
            bar.SetRadius(20.0);
            bar.SetStyle(n"HUDProgressCircle");
            bar.SetTintColor(new HDRColor(0.4,0.4,0.4,1.0));
            bar.SetMargin(new inkMargin(0.0, Cast<Float>(bars.Size())*50.0, 0.0, 0.0));
            canvas.AddChild(bar);
            bars.Push(bar);
            times.Push(0.0);
            maxTimes.Push(1.0);
        };
    }

    public func OnUpdate(dt: Float) -> Void {
        var i: Int32 = 0;
        while i < bars.Size() {
            if times[i] > 0.0 {
                times[i] -= dt;
                if times[i] <= 0.0 {
                    times[i] = 0.0;
                    bars[i].SetTintColor(new HDRColor(0.2, 1.0, 0.2, 1.0));
                    bars[i].SetValue(0.0);
                } else {
                    let base = MaxF(maxTimes[i], 0.001);
                    bars[i].SetValue((base - times[i]) / base);
                    bars[i].SetTintColor(new HDRColor(0.4, 0.4, 0.4, 1.0));
                };
            } else {
                let pulse = (Sin(GameInstance.GetSimTime(GetGame()) * 4.0) + 1.0) * 0.5;
                bars[i].SetTintColor(new HDRColor(0.1 + pulse * 0.3, 1.0, 0.1 + pulse * 0.3, 1.0));
            };
            i += 1;
        };
    }
}
