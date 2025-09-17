public class HealthBar {
    private let widget: wref<inkWidget>;
    private let hpBar: wref<inkRectangle>;
    private let armorBar: wref<inkRectangle>;
    private let curHpFrac: Float;
    private let targetHpFrac: Float;
    private let isAttached: Bool = false;

    public func AttachTo(proxy: ref<AvatarProxy>) -> Void {
        // Clean up any existing widget first
        if isAttached {
            Detach();
        }
        
        // Create health bar widget manually since UI file may not exist
        let hud = GameInstance.GetHUDManager(GetGame());
        if IsDefined(hud) {
            widget = new inkCanvas();
            widget.SetName(n"coop_healthbar");
            widget.SetSize(200.0, 40.0);
            
            hpBar = new inkRectangle();
            hpBar.SetName(n"hp_bar");
            hpBar.SetSize(180.0, 15.0);
            hpBar.SetMargin(10.0, 5.0, 10.0, 5.0);
            widget.AddChild(hpBar);
            
            armorBar = new inkRectangle();
            armorBar.SetName(n"armor_bar");
            armorBar.SetSize(180.0, 15.0);
            armorBar.SetMargin(10.0, 20.0, 10.0, 5.0);
            widget.AddChild(armorBar);
            
            curHpFrac = 1.0;
            targetHpFrac = 1.0;
            isAttached = true;
            Update(proxy.health, proxy.armor, 100u, 100u);
        }
    }
    
    public func Detach() -> Void {
        if isAttached && IsDefined(widget) {
            widget.UnregisterFromCallback(n"OnDestroy", this, n"OnWidgetDestroyed");
            // Remove widget from parent container
            let parent = widget.GetParentWidget();
            if IsDefined(parent) {
                parent.RemoveChild(widget);
            };
            widget = null;
            hpBar = null;
            armorBar = null;
            isAttached = false;
        }
    }
    
    private cb func OnWidgetDestroyed() -> Void {
        isAttached = false;
        widget = null;
        hpBar = null;
        armorBar = null;
    }

    public func Update(hp: Uint16, armor: Uint16, maxHp: Uint16, maxArmor: Uint16) -> Void {
        if !isAttached || !IsDefined(hpBar) || !IsDefined(armorBar) {
            return;
        }
        
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
