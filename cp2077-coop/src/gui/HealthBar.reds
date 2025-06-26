public class HealthBar {
    private let widget: wref<inkWidget>;
    private let hpBar: wref<inkRectangle>;
    private let armorBar: wref<inkRectangle>;
    private let curHpFrac: Float;
    private let targetHpFrac: Float;

    public func AttachTo(proxy: ref<AvatarProxy>) -> Void {
        widget = GameInstance.GetUI(GetGame()).SpawnExternal(n"ui/healthbar.inkwidget") as inkWidget;
        hpBar = widget.GetWidget(n"hp") as inkRectangle;
        armorBar = widget.GetWidget(n"armor") as inkRectangle;
        curHpFrac = 1.0;
        targetHpFrac = 1.0;
        Update( proxy.health, proxy.armor, 100u, 100u );
    }

    public func Update(hp: Uint16, armor: Uint16, maxHp: Uint16, maxArmor: Uint16) -> Void {
        targetHpFrac = Cast<Float>(hp) / Cast<Float>(Max(1u, maxHp));
        curHpFrac = Lerp(curHpFrac, targetHpFrac, 0.3);
        hpBar.SetScale(new Vector2(curHpFrac, 1.0));
        let c: HDRColor;
        if curHpFrac > 0.5 {
            c = HDRColor.FromLinear(new Color(1.0 - (curHpFrac - 0.5) * 2.0, 1.0, 0.0));
        } else {
            c = HDRColor.FromLinear(new Color(1.0, curHpFrac * 2.0, 0.0));
        };
        hpBar.SetTintColor(c);
        let armorFrac: Float = Cast<Float>(armor) / Cast<Float>(Max(1u, maxArmor));
        armorBar.SetScale(new Vector2(armorFrac, 1.0));
        armorBar.SetTintColor(HDRColor.FromLinear(new Color(0.3, 0.5, 1.0)));
        if curHpFrac < 0.25 {
            widget.PlayAnimation(n"shake");
        }
    }
}
