public class AirVehicleProxy extends gameObject {
    public var vehicleId: Uint32;
    public var path: array<Vector3>;
    public var idx: Int32;
    public var state: TransformSnap;
    public static let proxies: array<ref<AirVehicleProxy>>;

    public func Spawn(id: Uint32, points: array<Vector3>) -> Void {
        vehicleId = id;
        path = points;
        idx = 0;
    }

    public func UpdateSnap(t: ref<TransformSnap>) -> Void {
        state = t^;
    }
}

public static func AirVehicleProxy_Spawn(id: Uint32, count: Uint8, points: ref<Vector3>) -> Void {
    let p: array<Vector3>;
    for i in 0u .. count {
        p.PushBack(points[i]);
    }
    let av = new AirVehicleProxy();
    av.Spawn(id, p);
    proxies.PushBack(av);
}

public static func AirVehicleProxy_Update(id: Uint32, t: ref<TransformSnap>) -> Void {
    for v in proxies {
        if v.vehicleId == id {
            v.UpdateSnap(t);
            break;
        };
    }
}
