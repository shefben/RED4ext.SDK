public class WalletHud extends inkHUDLayer {
    public static let s_instance: ref<WalletHud>;
    private let balanceLabel: ref<inkText>;
    private let deltaLabel: ref<inkText>;
    private var fade: Float;
    public var eddies: Int64;

    public static func Update(newBal: Int64) -> Void {
        if !IsDefined(s_instance) {
            s_instance = new WalletHud();
            let hud = GameInstance.GetHUDManager(GetGame());
            hud.AddLayer(s_instance);
        };
        s_instance.ApplyUpdate(newBal);
    }

    private func ApplyUpdate(newBal: Int64) -> Void {
        if !IsDefined(balanceLabel) {
            balanceLabel = new inkText();
            balanceLabel.SetStyle(n"Medium 32px Bold");
            balanceLabel.SetTintColor(new HDRColor(1.0, 0.85, 0.231, 1.0));
            balanceLabel.SetAnchor(inkEAnchor.TopRight);
            balanceLabel.SetMargin(new inkMargin(0.0, 20.0, 40.0, 0.0));
            AddChild(balanceLabel);
        };
        let diff: Int64 = eddies - newBal;
        eddies = newBal;
        balanceLabel.SetText(Int64ToString(newBal));
        if diff > 0 {
            if !IsDefined(deltaLabel) {
                deltaLabel = new inkText();
                deltaLabel.SetStyle(n"Medium 32px Bold");
                deltaLabel.SetTintColor(new HDRColor(1.0, 0.85, 0.231, 1.0));
                deltaLabel.SetAnchor(inkEAnchor.TopRight);
                deltaLabel.SetMargin(new inkMargin(0.0, 60.0, 40.0, 0.0));
                AddChild(deltaLabel);
            };
            deltaLabel.SetText("-" + Int64ToString(diff) + " €$");
            deltaLabel.SetOpacity(1.0);
            fade = 1.0;
        };
    }

    public func OnUpdate(dt: Float) -> Void {
        if fade > 0.0 {
            fade -= dt * 2.0;
            if fade < 0.0 { fade = 0.0; };
            if IsDefined(deltaLabel) { deltaLabel.SetOpacity(fade); };
        };
    }
}
