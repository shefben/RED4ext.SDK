public class SpectatorCam {
    private static let hud: ref<SpectatorHUD>;
    private static let target: Uint32;
    // Switches the local player into spectator mode and disables standard HUD.
    public static func Enter(peerId: Uint32) -> Void {
        GameModeManager.current = GameModeManager.GameMode.Spectate;
        let player = GameInstance.GetPlayerSystem(GetGame()).GetLocalPlayerMainGameObject();
        if IsDefined(player) {
            if HasMethod(player, n"DisableCollision") {
                player.DisableCollision();
            };
        };
        let hudMgr = GameInstance.GetHUDManager(GetGame());
        let list = hudMgr.GetLayers();
        for layer in list { layer.SetVisible(false); };
        hud = hudMgr.SpawnChildFromExternal(n"SpectatorHUD") as SpectatorHUD;
        if IsDefined(hud) { hud.SetTarget(peerId); };
        target = peerId;
        LogChannel(n"DEBUG", "EnterSpectate " + IntToString(peerId));
    }

    // Very simple free-fly camera controls using WASD.
    public static func UpdateInput(dt: Float) -> Void {
        if GameModeManager.current != GameModeManager.GameMode.Spectate { return; };
        let inputSys = GameInstance.GetInputSystem(GetGame());
        let player = GameInstance.GetPlayerSystem(GetGame()).GetLocalPlayerMainGameObject();
        if !IsDefined(player) { return; };
        var dir = Vector4.EmptyVector();
        if inputSys.IsActionHeld(EInputKey.IK_W) { dir.Y += 1.0; };
        if inputSys.IsActionHeld(EInputKey.IK_S) { dir.Y -= 1.0; };
        if inputSys.IsActionHeld(EInputKey.IK_A) { dir.X -= 1.0; };
        if inputSys.IsActionHeld(EInputKey.IK_D) { dir.X += 1.0; };
        if Length(dir) > 0.0 {
            dir = Vector4.Normalize(dir);
            player.SetWorldPosition(player.GetWorldPosition() + AsVector3(dir) * (dt * 6.0));
        };
        if IsDefined(hud) { hud.OnUpdate(dt); };
    }

    public static func CycleTarget() -> Void {
        let conns = Net_GetConnections();
        if conns.Size() == 0 { return; };
        var idx: Int32 = 0;
        for i in 0 ..< conns.Size() {
            if conns[i].peerId == target { idx = i + 1; break; };
        };
        if idx >= conns.Size() { idx = 0; };
        target = conns[idx].peerId;
        if IsDefined(hud) { hud.SetTarget(target); };
    }
}

// Console command: /spectate <peerId>
public static exec func Spectate(peerId: Int32) -> Void {
    Net_SendSpectateRequest(Cast<Uint32>(peerId));
}

public static func SpectatorCam_Enter(peerId: Uint32) -> Void {
    SpectatorCam.Enter(peerId);
}

