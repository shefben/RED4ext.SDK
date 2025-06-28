public class CoopMap extends inkHUDLayer {
    private static let s_instance: ref<CoopMap>;
    private let icons: array<ref<inkImage>>;
    private let offset: Vector2 = new Vector2(0.0, 0.0);
    private let zoom: Float = 1.0;

    public static func Show() -> Void {
        if IsDefined(s_instance) { return; };
        s_instance = new CoopMap();
        let hud = GameInstance.GetHUDManager(GetGame());
        hud.AddLayer(s_instance);
        s_instance.SpawnIcons();
        LogChannel(n"DEBUG", "CoopMap shown");
    }

    public static func Hide() -> Void {
        if !IsDefined(s_instance) { return; };
        let hud = GameInstance.GetHUDManager(GetGame());
        hud.RemoveLayer(s_instance);
        s_instance = null;
    }

    private func SpawnIcons() -> Void {
        // UI-2: verify that PlayerSystem.GetPlayers lists remote avatars
        let playerSys = GameInstance.GetPlayerSystem(GetGame());
        let players = playerSys.GetPlayers(); // returns all connected avatars
        for p in players {
            let img = new inkImage();
            img.SetAtlasResource(r"base/gameplay/gui/fullscreen/minimap/minimap_player.inkatlas");
            img.SetTexturePart(n"player_icon");
            img.SetSize(32.0, 32.0);
            icons.PushBack(img);
            AddChild(img);
        }
    }

    public func OnUpdate(dt: Float) -> Void {
        let input = GameInstance.GetInputSystem(GetGame());
        if input.IsActionPressed(n"IK_W") { offset.Y += 200.0 * dt; };
        if input.IsActionPressed(n"IK_S") { offset.Y -= 200.0 * dt; };
        if input.IsActionPressed(n"IK_A") { offset.X += 200.0 * dt; };
        if input.IsActionPressed(n"IK_D") { offset.X -= 200.0 * dt; };
        if input.IsActionJustPressed(n"IK_Plus") { zoom *= 1.1; };
        if input.IsActionJustPressed(n"IK_Minus") { zoom *= 0.9; };
        let playerSys = GameInstance.GetPlayerSystem(GetGame());
        let players = playerSys.GetPlayers();
        var i: Int32 = 0;
        while i < players.Size() && i < icons.Size() {
            let p = players[i] as AvatarProxy;
            if IsDefined(p) {
                let screen = GameInstance.GetViewportManager(GetGame()).WorldToScreen(p.pos);
                icons[i].SetMargin((screen.X * zoom) + offset.X, (screen.Y * zoom) + offset.Y);
            };
            i += 1;
        }
    }

    protected cb func OnPadTranslate(evt: ref<inkPadTranslateEvent>) -> Bool {
        offset.X += evt.GetDelta().X;
        offset.Y += evt.GetDelta().Y;
        return true;
    }

    protected cb func OnPadPinchStretch(evt: ref<inkPadPinchStretchEvent>) -> Bool {
        zoom *= 1.0 + evt.GetDelta();
        return true;
    }
}
