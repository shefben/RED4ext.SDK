public class SpectatorCam {
    // Switches the local player into spectator mode and disables standard HUD.
    public static func Enter(peerId: Uint32) -> Void {
        GameModeManager.current = GameModeManager.GameMode.Spectate;
        let player = GameInstance.GetPlayerSystem(GetGame()).GetLocalPlayerMainGameObject();
        if IsDefined(player) {
            if HasMethod(player, n"DisableCollision") {
                player.DisableCollision();
            };
        };
        let hud = GameInstance.GetHUDManager(GetGame());
        let list = hud.GetLayers();
        for layer in list { layer.SetVisible(false); };
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
    }
}

// Console command: /spectate <peerId>
public static exec func Spectate(peerId: Int32) -> Void {
    Net_SendSpectateRequest(Cast<Uint32>(peerId));
}

public static func SpectatorCam_Enter(peerId: Uint32) -> Void {
    SpectatorCam.Enter(peerId);
}

