public class AvatarProxy extends gameObject {
    public var peerId: Uint32;
    public var isLocal: Bool;
    public var pos: Vector3;
    public var vel: Vector3;
    public var rot: Quaternion;
    public var health: Uint16;
    public var armor: Uint16;
    public var currentSector: Uint64;
    // Future tickets will push per-tick input states here for client prediction
    // and server reconciliation.
    // Buffered movement commands awaiting server acknowledgement.
    public struct MoveCmd {
        public var seq: Uint16;
        public var move: Vector3;
        public var rot: Quaternion;
    }
    public var pendingInputs: array<MoveCmd>;
    private var nextSeq: Uint16;

    private var bar: ref<HealthBar>;

    public func Spawn(peer: Uint32, local: Bool, snap: ref<TransformSnap>) -> Void {
        peerId = peer;
        isLocal = local;
        pos = snap.pos;
        rot = snap.rot;
        health = snap.health;
        armor = snap.armor;
        LogChannel(n"DEBUG", "Avatar spawned: " + IntToString(peerId));
        bar = new HealthBar();
        bar.AttachTo(this);
    }

    public func Despawn() -> Void {
        LogChannel(n"DEBUG", "Avatar despawned: " + IntToString(peerId));
        if HasMethod(this, n"SetRagdollMode") {
            SetRagdollMode(true);
        } else {
            LogChannel(n"DEBUG", "Ragdoll not supported");
        }
        GameInstance.GetDelaySystem(GetGame()).DelayCallback(this, n"DestroySelf", 5.0);
    }

    public func DestroySelf() -> Void {
        LogChannel(n"DEBUG", "Avatar removed: " + IntToString(peerId));
    }

    public func ClearInvuln() -> Void {
        if HasMethod(this, n"SetGodMode") { this.SetGodMode(false); };
    }

    public static func OnEject(id: Uint32, vel: Vector3) -> Void {
        let avatar = GameInstance.GetPlayerSystem(GetGame()).FindObject(id) as AvatarProxy;
        if IsDefined(avatar) {
            if HasMethod(avatar, n"SetRagdollMode") {
                avatar.SetRagdollMode(true);
                if HasMethod(avatar, n"SetLinearVelocity") {
                    avatar.SetLinearVelocity(vel);
                };
            };
            avatar.health = Max(0u, avatar.health - 50u);
            avatar.OnVitalsChanged();
        };
    }

    public func ApplyTransform(snap: ref<TransformSnap>) -> Void {
        pos = snap.pos;
        vel = snap.vel;
        rot = snap.rot;
        health = snap.health;
        armor = snap.armor;
        nextSeq = snap.seq + 1u;
        OnVitalsChanged();
    }

    public func OnVitalsChanged() -> Void {
        if IsDefined(bar) {
            bar.Update(health, armor, 100u, 100u);
        }
    }

    // Advance client-side estimate and queue the input for reconciliation.
    public func Predict(dt: Float) -> Void {
        let cmd: MoveCmd;
        cmd.seq = nextSeq;
        let input = GameInstance.GetInputSystem(GetGame());
        var moveVec: Vector3 = Vector3.EmptyVector();
        if input.IsActionHeld(EInputKey.IK_W) { moveVec.Y += 1.0; };
        if input.IsActionHeld(EInputKey.IK_S) { moveVec.Y -= 1.0; };
        if input.IsActionHeld(EInputKey.IK_A) { moveVec.X -= 1.0; };
        if input.IsActionHeld(EInputKey.IK_D) { moveVec.X += 1.0; };
        if Length(moveVec) > 0.0 { moveVec = Vector3.Normalize(moveVec); };
        let yaw: Float = ArcTan2(2.0 * (rot.W * rot.Z + rot.X * rot.Y),
                                 1.0 - 2.0 * (rot.Y * rot.Y + rot.Z * rot.Z));
        let cosY: Float = Cos(yaw);
        let sinY: Float = Sin(yaw);
        let worldMove = Vector3{moveVec.X * cosY - moveVec.Y * sinY,
                               moveVec.X * sinY + moveVec.Y * cosY, 0.0};
        cmd.move = worldMove * dt * 6.0;
        cmd.rot = rot;

        pendingInputs.PushBack(cmd);
        if pendingInputs.Size() > 30 {
            pendingInputs.Erase(0);
        }
        nextSeq += 1u;

        pos += cmd.move;
    }

    // Rewind to server state then reapply unconfirmed inputs.
    public func Reconcile(authoritativeSnap: ref<TransformSnap>) -> Void {
        var idx: Int32 = 0;
        while idx < pendingInputs.Size() && pendingInputs[idx].seq <= authoritativeSnap.seq {
            idx += 1;
        }
        if idx > 0 {
            pendingInputs.Erase(0, idx);
        }

        let oldPos: Vector3 = pos;

        pos = authoritativeSnap.pos;
        rot = authoritativeSnap.rot;
        vel = authoritativeSnap.vel;

        // Re-simulate remaining inputs
        for cmd in pendingInputs {
            pos += cmd.move;
            rot = cmd.rot;
        }

        var dist: Float = VectorDistance(oldPos, pos);
        LogChannel(n"DEBUG", "reconcile diff=" + FloatToString(dist));
        if dist > 1.5 {
            let dir: Vector3 = pos - oldPos;
            let len: Float = VectorDistance(pos, oldPos);
            if len > 0.0001 {
                pos = oldPos + dir * (1.5 / len);
            }
        }

        health = authoritativeSnap.health;
        armor = authoritativeSnap.armor;
        OnVitalsChanged();
    }

    // Called by world streaming system when the new sector fully loads.
    public static func OnStreamingDone(hash: Uint64) -> Void {
        LogChannel(n"DEBUG", "Streaming done sector=" + IntToString(Cast<Int32>(hash)));
        Net_SendSectorReady(hash);
        ElevatorSync.OnStreamingDone(hash);
    }

    public static func SpawnRemote(peerId: Uint32, local: Bool, snap: ref<TransformSnap>) -> Void {
        let playerSys = GameInstance.GetPlayerSystem(GetGame());
        let avatar = playerSys.FindObject(peerId) as AvatarProxy;
        if !IsDefined(avatar) {
            avatar = new AvatarProxy();
            playerSys.RegisterPlayer(peerId, avatar);
        };
        avatar.Spawn(peerId, local, snap);
    }

    public static func DespawnRemote(peerId: Uint32) -> Void {
        let playerSys = GameInstance.GetPlayerSystem(GetGame());
        let avatar = playerSys.FindObject(peerId) as AvatarProxy;
        if IsDefined(avatar) {
            avatar.Despawn();
        };
    }
}

public static func AvatarProxy_OnEject(peerId: Uint32, vel: Vector3) -> Void {
    AvatarProxy.OnEject(peerId, vel);
}
