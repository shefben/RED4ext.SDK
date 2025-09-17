public class VehicleProxy extends gameObject {
    public static let proxies: array<ref<VehicleProxy>>;
    public var vehicleId: Uint32;
    public var damage: Uint16;
    public var state: TransformSnap;
    public var destroyed: Bool;
    public var phaseId: Uint32;
    public var leanAngle: Float;
    private var targetLean: Float;
    public var turretYaw: Float;
    public var turretPitch: Float;
    public var paintId: Uint32;
    public var plateText: String;
    private var despawnDelay: Float;
    private var lastVel: Vector3;
    private var lastAccel: Vector3;
    private var occupantPeer: Uint32;
    private var physAcc: Float;

    private static func FindProxy(id: Uint32) -> ref<VehicleProxy> {
        for p in proxies { if p.vehicleId == id { return p; }; };
        return null;
    }

    public func Spawn(id: Uint32, phase: Uint32, transform: ref<TransformSnap>) -> Void {
        vehicleId = id;
        phaseId = phase;
        state = transform^;
        leanAngle = 0.0;
        targetLean = 0.0;
        turretYaw = 0.0;
        turretPitch = 0.0;
        paintId = 0u;
        plateText = "";
        LogChannel(n"DEBUG", "Vehicle spawned: " + IntToString(vehicleId));
    }

    public func UpdateAuthoritative(snap: ref<TransformSnap>) -> Void {
        state = snap^;
        lastVel = snap^.vel;
        // Acceleration will be derived from physics later
    }

    public func UpdateSnapshot(snap: ref<VehicleSnap>) -> Void {
        UpdateAuthoritative(&snap^.transform);
        targetLean = ClampF(snap^.leanAngle, -45.0, 45.0);
    }

    public func Reconcile(authoritative: ref<TransformSnap>) -> Void {
        let diff: Vector3 = authoritative.pos - state.pos;
        if VectorLength(diff) > 0.25 {
            state.pos = authoritative.pos;
            state.vel = authoritative.vel;
            state.rot = authoritative.rot;
            lastVel = authoritative.vel;
            lastAccel = Vector3.EmptyVector();
        };
    }

    // SeatIdx range 0-3
    public func EnterSeat(peerId: Uint32, idx: Uint8) -> Void {
        occupantPeer = peerId;
        LogChannel(n"DEBUG", IntToString(peerId) + " entered seat " + IntToString(idx));
    }

    public func RequestSeat(idx: Uint8) -> Void {
        CoopNet.Net_SendSeatRequest(vehicleId, idx);
    }

    public func DetachPart(partId: Uint8) -> Void {
        switch partId {
            case 0u:
                LogChannel(n"DEBUG", "Detach door_L");
                if HasComponent(n"door_L") {
                    SetMeshVisibility(n"door_L", false);
                };
            case 1u:
                LogChannel(n"DEBUG", "Detach door_R");
                if HasComponent(n"door_R") {
                    SetMeshVisibility(n"door_R", false);
                };
            case 2u:
                LogChannel(n"DEBUG", "Detach hood");
                if HasComponent(n"hood") {
                    SetMeshVisibility(n"hood", false);
                };
            case 3u:
                LogChannel(n"DEBUG", "Detach trunk");
                if HasComponent(n"trunk") {
                    SetMeshVisibility(n"trunk", false);
                };
            default:
        };
    }

    public func Explode(vfxId: Uint32, seed: Uint32) -> Void {
        if destroyed { return; };
        destroyed = true;
        despawnDelay = 10.0;
        let effSys = GameInstance.GetScriptableSystemsContainer(GetGame()).Get(n"EffectSystem") as EffectSystem;
        if IsDefined(effSys) {
            effSys.SpawnEffect(vfxId, state.pos);
        };
        // spawn debris deterministic using seed
        let debris = 5u + Cast<Uint32>(seed % 6u);
        let chunkVfx: Uint32 = CoopNet.Fnv1a32("veh_debris_chunk.ent");
        for i in 0u .. debris {
            let ang: Float = (Cast<Float>((seed >> ((i * 3u) & 15u)) & 255u) / 255.0) * 6.28;
            let dir: Vector3 = Vector3(Cos(ang), Sin(ang), 0.5);
            if IsDefined(effSys) {
                effSys.SpawnEffect(chunkVfx, state.pos + dir * 0.5);
            };
        };
    }

    public func ApplyDamage(d: Uint16, side: Bool) -> Void {
        damage += d;
        LogChannel(n"DEBUG", "Vehicle " + IntToString(vehicleId) + " damage=" + IntToString(damage));
        if side && d > 300u {
            CoopNet.Net_BroadcastPartDetach(vehicleId, 0u); // door_L only for now
        };
        if Net_IsAuthoritative() && !destroyed && damage >= 1000u {
            CoopNet.Net_BroadcastVehicleExplode(vehicleId, CoopNet.Fnv1a32("veh_explosion_big.ent"), damage);
            destroyed = true;
            despawnDelay = 10.0;
        };
    }

    // dtMs may vary; physics steps at CoopNet.kVehicleStepMs
    public func Tick(dtMs: Float) -> Void {
        if Net_IsAuthoritative() {
            physAcc += dtMs;
            while physAcc >= CoopNet.kVehicleStepMs {
                CoopNet.ServerSimulate(state, CoopNet.kVehicleStepMs);
                physAcc -= CoopNet.kVehicleStepMs;
            };
            let newVel: Vector3 = state.vel;
            let delta: Vector3 = newVel - lastVel;
            var along: Float = 0.0;
            if VectorLength(lastVel) > 0.01 {
                along = VectorDot(delta, VectorNormalize(lastVel));
            };
            let decel: Float = -along / (dtMs / 1000.0);
            if decel > 12.0 && occupantPeer != 0u {
                CoopNet.Net_BroadcastEject(occupantPeer, lastVel);
                occupantPeer = 0u;
            };
            lastAccel = newVel - lastVel;
            lastVel = newVel;
        } else {
            ClientPredict(dtMs);
            leanAngle += (targetLean - leanAngle) * MinF(dtMs / 100.0, 1.0);
        };
        if destroyed {
            despawnDelay -= dtMs / 1000.0;
            if despawnDelay <= 0.0 {
                LogChannel(n"DEBUG", "Vehicle despawn " + IntToString(vehicleId));
                var idx: Int32 = 0;
                while idx < VehicleProxy.proxies.Size() && VehicleProxy.proxies[idx] != this {
                    idx += 1;
                };
                if idx < VehicleProxy.proxies.Size() {
                    VehicleProxy.proxies.Erase(idx);
                };
            };
        };
        LogChannel(n"DEBUG", "Vehicle tick " + IntToString(vehicleId));
        ApplyLean();
    }

    private func ClientPredict(dtMs: Float) -> Void {
        state.pos += lastVel * (dtMs / 1000.0);
        state.vel += lastAccel * (dtMs / 1000.0);
    }

    private func ApplyLean() -> Void {
        if HasMethod(this, n"SetBikeLean") {
            this.SetBikeLean(leanAngle);
        };
    }

    public func SetTurretAim(yaw: Float, pitch: Float) -> Void {
        turretYaw = yaw;
        turretPitch = pitch;
        if HasMethod(this, n"SetTurretYaw") { this.SetTurretYaw(yaw); };
        if HasMethod(this, n"SetTurretPitch") { this.SetTurretPitch(pitch); };
    }
}

public static func VehicleProxy_Spawn(id: Uint32, transform: ref<TransformSnap>, phase: Uint32) -> Void {
    let v = new VehicleProxy();
    v.Spawn(id, phase, transform);
    VehicleProxy.proxies.PushBack(v);
}

public static func VehicleProxy_Explode(id: Uint32, vfxId: Uint32, seed: Uint32) -> Void {
    let v = VehicleProxy.FindProxy(id);
    if IsDefined(v) { v.Explode(vfxId, seed); };
}

public static func VehicleProxy_Detach(id: Uint32, part: Uint8) -> Void {
    let v = VehicleProxy.FindProxy(id);
    if IsDefined(v) { v.DetachPart(part); };
}

public static func VehicleProxy_EnterSeat(peerId: Uint32, seat: Uint8) -> Void {
    // Add bounds check before accessing array
    if ArraySize(VehicleProxy.proxies) == 0 {
        LogChannel(n"ERROR", "No vehicle proxies available for seat entry");
        return;
    }
    
    let v = VehicleProxy.proxies[0]; // assume single vehicle
    if IsDefined(v) { 
        v.EnterSeat(peerId, seat); 
    } else {
        LogChannel(n"ERROR", "Vehicle proxy at index 0 is null");
    }
}

public static func VehicleProxy_ApplyDamage(id: Uint32, d: Uint16, side: Bool) -> Void {
    let v = VehicleProxy.FindProxy(id);
    if IsDefined(v) { v.ApplyDamage(d, side); };
}

public static func VehicleProxy_UpdateSnap(id: Uint32, snap: ref<VehicleSnap>) -> Void {
    let v = VehicleProxy.FindProxy(id);
    if IsDefined(v) {
        v.UpdateSnapshot(snap);
        if !Net_IsAuthoritative() {
            v.Reconcile(&snap^.transform);
        };
    };
}

public static func VehicleProxy_SetTurretAim(id: Uint32, yaw: Float, pitch: Float) -> Void {
    let v = VehicleProxy.FindProxy(id);
    if IsDefined(v) { v.SetTurretAim(yaw, pitch); };
}

public static func VehicleProxy_ApplyPaint(id: Uint32, color: Uint32, plate: String) -> Void {
    let v = VehicleProxy.FindProxy(id);
    if IsDefined(v) {
        v.paintId = color;
        v.plateText = plate;
        if HasMethod(v, n"SetPaint") { v.SetPaint(color); };
        if HasMethod(v, n"SetPlateText") { v.SetPlateText(plate); };
    };
}

public static exec func UnstuckCar() -> Void {
    let player = GameInstance.GetPlayerSystem(GetGame()).GetLocalPlayerMainGameObject();
    if IsDefined(player) {
        CoopNet.Net_SendVehicleTowRequest(player.GetWorldPosition());
    };
}
