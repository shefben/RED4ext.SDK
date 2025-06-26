public class CoopMap extends inkHUDLayer {
    private static let s_instance: ref<CoopMap>;
    private let icons: array<ref<inkImage>>;

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
        // Placeholder pan/zoom controls without pausing the game.
        let playerSys = GameInstance.GetPlayerSystem(GetGame());
        let players = playerSys.GetPlayers();
        var i: Int32 = 0;
        while i < players.Size() && i < icons.Size() {
            let p = players[i] as AvatarProxy;
            if IsDefined(p) {
                let screen = GameInstance.GetViewportManager(GetGame()).WorldToScreen(p.pos);
                icons[i].SetMargin(screen.X, screen.Y);
            };
            i += 1;
        }
    }
}
