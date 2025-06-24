public class AvatarProxy extends gameObject {
    public var peerId: Uint32;
    public var isLocal: Bool;
    public var pos: Vector3;
    public var vel: Vector3;
    public var rot: Quaternion;
    public var health: Uint16;
    public var armor: Uint16;
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

    public func Spawn(peer: Uint32, local: Bool) -> Void {
        peerId = peer;
        isLocal = local;
        LogChannel(n"DEBUG", "Avatar spawned: " + IntToString(peerId));
    }

    public func Despawn() -> Void {
        LogChannel(n"DEBUG", "Avatar despawned: " + IntToString(peerId));
        if HasMethod(this, n"SetRagdollMode") {
            SetRagdollMode(true);
        } else {
            LogChannel(n"DEBUG", "Ragdoll stub");
        }
        GameInstance.GetDelaySystem(GetGame()).DelayCallback(this, n"DestroySelf", 5.0);
    }

    public func DestroySelf() -> Void {
        LogChannel(n"DEBUG", "Avatar removed: " + IntToString(peerId));
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
            bar.Draw(this);
        }
    }

    // Advance client-side estimate and queue the input for reconciliation.
    public func Predict(dt: Float) -> Void {
        let cmd: MoveCmd;
        cmd.seq = nextSeq;
        cmd.move = vel * dt; // placeholder for actual movement input
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
}

