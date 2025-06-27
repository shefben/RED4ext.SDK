public class SpectatorCam {
    private static let hud: ref<SpectatorHUD>;
    private static let currentTargetId: Uint32;
    private static let freecam: Bool;
    private static let camSpeed: Float = 6.0;
    private static let blend: Float;
    private static let blendPos: Vector3;
    private static let blendRot: Quaternion;
    public static var spectatePhase: Uint32 = QuestSync.localPhase;
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
        currentTargetId = peerId;
        freecam = false;
        blend = 0.0;
        LogChannel(n"DEBUG", "EnterSpectate " + IntToString(peerId));
    }

    // Very simple free-fly camera controls using WASD.
    public static func UpdateInput(dt: Float) -> Void {
        if GameModeManager.current != GameModeManager.GameMode.Spectate { return; };
        let inputSys = GameInstance.GetInputSystem(GetGame());
        let player = GameInstance.GetPlayerSystem(GetGame()).GetLocalPlayerMainGameObject();
        if !IsDefined(player) { return; };
        if inputSys.IsActionJustPressed(EInputKey.IK_B) { CycleTarget(); };
        if inputSys.IsActionJustPressed(EInputKey.IK_N) { freecam = !freecam; };
        if inputSys.IsActionHeld(EInputKey.IK_Q) { camSpeed = MaxF(1.0, camSpeed - dt * 5.0); };
        if inputSys.IsActionHeld(EInputKey.IK_E) { camSpeed += dt * 5.0; };

        if blend > 0.0 {
            blend -= dt;
            let t: Float = 1.0 - blend / 0.6;
            t = t * t * (3.0 - 2.0 * t);
            let targetObj = GameInstance.GetPlayerSystem(GetGame()).FindObject(currentTargetId) as AvatarProxy;
            if IsDefined(targetObj) {
                let pos = Vector3.Lerp(blendPos, targetObj.pos, t);
                player.SetWorldPosition(pos);
            };
            if blend <= 0.0 { blend = 0.0; };
        } else {
            var dir = Vector4.EmptyVector();
            if inputSys.IsActionHeld(EInputKey.IK_W) { dir.Y += 1.0; };
            if inputSys.IsActionHeld(EInputKey.IK_S) { dir.Y -= 1.0; };
            if inputSys.IsActionHeld(EInputKey.IK_A) { dir.X -= 1.0; };
            if inputSys.IsActionHeld(EInputKey.IK_D) { dir.X += 1.0; };
            if Length(dir) > 0.0 {
                dir = Vector4.Normalize(dir);
                player.SetWorldPosition(player.GetWorldPosition() + AsVector3(dir) * (dt * camSpeed));
            };
        };

        if !freecam {
            let tgt = GameInstance.GetPlayerSystem(GetGame()).FindObject(currentTargetId) as AvatarProxy;
            if IsDefined(tgt) {
                player.SetWorldPosition(tgt.pos);
            };
        } else {
            let players = GameInstance.GetPlayerSystem(GetGame()).GetPlayers();
            var minDist: Float = 1e12;
            for p in players {
                let distSq = VectorDistanceSquared(p.GetWorldPosition(), player.GetWorldPosition());
                if distSq < minDist { minDist = distSq; };
            };
            if minDist > 1000000.0 {
                let factor = SqrtF(minDist);
                let dir = Vector3.Normalize(player.GetWorldPosition());
                player.SetWorldPosition(dir * 1000.0);
                if IsDefined(hud) { hud.ShowWarning("Out-of-range"); };
            } else {
                if IsDefined(hud) { hud.HideWarning(); };
            };
        };
        if IsDefined(hud) { hud.OnUpdate(dt); };
    }

    public static func CycleTarget() -> Void {
        let conns = Net_GetConnections();
        if conns.Size() == 0 { return; };
        var idx: Int32 = 0;
        for i in 0 ..< conns.Size() {
            if conns[i].peerId == currentTargetId { idx = i + 1; break; };
        };
        if idx >= conns.Size() { idx = 0; };
        currentTargetId = conns[idx].peerId;
        if IsDefined(hud) { hud.SetTarget(currentTargetId); };
        blend = 0.6;
        let player = GameInstance.GetPlayerSystem(GetGame()).GetLocalPlayerMainGameObject();
        if IsDefined(player) {
            blendPos = player.GetWorldPosition();
            blendRot = player.GetWorldOrientation();
        };
    }
}

// Console command: /spectate <peerId>
public static exec func Spectate(peerId: Int32) -> Void {
    Net_SendSpectateRequest(Cast<Uint32>(peerId));
}

// Console command: /assist <peerId|off>
public static exec func Assist(arg: String) -> Void {
    let player = GameInstance.GetPlayerSystem(GetGame()).GetLocalPlayerMainGameObject();
    if IsDefined(player) && HasMethod(player, n"IsInCombat") {
        if player.IsInCombat() { return; };
    };
    if arg == "off" {
        spectatePhase = QuestSync.localPhase;
    } else {
        let id = StringToInt(arg);
        spectatePhase = Cast<Uint32>(id);
        CoopNotice.Show("Assisting " + arg);
    };
}

public static func SpectatorCam_Enter(peerId: Uint32) -> Void {
    SpectatorCam.Enter(peerId);
}
