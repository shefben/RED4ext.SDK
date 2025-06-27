public class EmoteWheel extends inkHUDLayer {
    private static let s_instance: ref<EmoteWheel>;
    private let radial: ref<inkRadial>;
    private var open: Bool;
    private var current: Uint8;

    public static func Instance() -> ref<EmoteWheel> {
        if !IsDefined(s_instance) {
            s_instance = new EmoteWheel();
            GameInstance.GetHUDManager(GetGame()).AddLayer(s_instance);
        };
        return s_instance;
    }

    public static func Tick() -> Void {
        let input = GameInstance.GetInputSystem(GetGame());
        let want = input.IsActionHeld(EInputKey.IK_C) || input.IsActionHeld(EInputKey.IK_Pad_RightThumb);
        let inst = Instance();
        if want && !inst.open {
            inst.ShowWheel();
        } else if !want && inst.open {
            inst.HideWheel();
        };
        if inst.open {
            for i in 0u ..< 12u {
                if input.IsActionJustPressed(Cast<EInputKey>(EInputKey.IK_1 + i)) {
                    inst.current = Cast<Uint8>(i + 1u);
                    CoopNet.Net_BroadcastEmote(Net_GetPeerId(), inst.current);
                };
            };
        };
    }

    private func ShowWheel() -> Void {
        open = true;
        if !IsDefined(radial) {
            radial = new inkRadial();
            radial.SetSlotCount(12);
            AddChild(radial);
        };
        radial.SetVisible(true);
    }

    private func HideWheel() -> Void {
        open = false;
        if IsDefined(radial) {
            radial.SetVisible(false);
        };
    }
}
