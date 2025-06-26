public class SnapshotInterpolator {
    public static let defaultInterpMs: Uint16 = 100u;
    public static var interpDelayMs: Uint16 = defaultInterpMs;
    public static var tickMs: Uint16 = 32u;

    public var buffer: array<TransformSnap>;
    private var ticks: array<Uint64>;
    // Recent snapshots kept for melee rollback checks (~10 entries)
    public var history: array<TransformSnap>;
    private var historyTicks: array<Uint64>;

    // Store a snapshot with its tick for later interpolation.
    public func Push(tick: Uint64, snap: ref<TransformSnap>) -> Void {
        ticks.PushBack(tick);
        buffer.PushBack(snap^);
        history.PushBack(snap^);
        historyTicks.PushBack(tick);
        if history.Size() > 10 {
            history.Erase(0);
            historyTicks.Erase(0);
        }
        let maxAge: Int32 = CeilF(Cast<Float>(interpDelayMs) / Cast<Float>(tickMs)) + 4; // keep margin
        while ticks.Size() > maxAge {
            ticks.Erase(0);
            buffer.Erase(0);
        }
    }

    // Catmull‑Rom Hermite interpolation. Requires **two** snapshots before
    // and after the sample point; otherwise falls back to simple linear
    // blending. Formula reference:
    //   H(p1,p2,m1,m2,t) = (2t^3-3t^2+1)p1 + (t^3-2t^2+t)m1
    //                    + (-2t^3+3t^2)p2 + (t^3-t^2)m2
    // see https://en.wikipedia.org/wiki/Cubic_Hermite_spline
    public func Sample(nowTick: Uint64, out snap: ref<TransformSnap>) -> Void {
        if buffer.Size() == 0 {
            return;
        }

        let count: Int32 = ticks.Size();
        if count == 1 {
            snap = buffer[0];
            return;
        }

        var idx: Int32 = 0;
        while idx < count - 1 && nowTick > ticks[idx + 1] {
            idx += 1;
        }

        if idx >= count - 1 {
            snap = buffer[count - 1];
            return;
        }

        let t0: Uint64 = ticks[idx];
        let t1: Uint64 = ticks[idx + 1];
        let alpha: Float = Cast<Float>(nowTick - t0) / Cast<Float>(t1 - t0);

        // Need two snapshots before and after for stable Catmull-Rom tangents
        if idx >= 2 && idx + 2 < count {
            // Catmull-Rom using positions of four sequential snapshots.
            let p0: Vector3 = buffer[idx - 1].pos;
            let p1: Vector3 = buffer[idx].pos;
            let p2: Vector3 = buffer[idx + 1].pos;
            let p3: Vector3 = buffer[idx + 2].pos;
            let m0: Vector3 = (p2 - p0) * 0.5;
            let m1: Vector3 = (p3 - p1) * 0.5;
            let t: Float = alpha;
            let t2: Float = t * t;
            let t3: Float = t2 * t;
            let h00: Float = 2.0 * t3 - 3.0 * t2 + 1.0;
            let h10: Float = t3 - 2.0 * t2 + t;
            let h01: Float = -2.0 * t3 + 3.0 * t2;
            let h11: Float = t3 - t2;
            snap.pos = p1 * h00 + m0 * h10 + p2 * h01 + m1 * h11;

            // Squad-style rotation interpolation using Catmull-Rom tangents.
            let r0: Quaternion = buffer[idx - 1].rot;
            let r1: Quaternion = buffer[idx].rot;
            let r2: Quaternion = buffer[idx + 1].rot;
            let r3: Quaternion = buffer[idx + 2].rot;
            let a: Quaternion = QuatSlerp(r1, r2, t);
            let pre: Quaternion = QuatSlerp(r0, r2, 0.5);
            let post: Quaternion = QuatSlerp(r1, r3, 0.5);
            let b: Quaternion = QuatSlerp(pre, post, t);
            snap.rot = QuatSlerp(a, b, 2.0 * t * (1.0 - t));
        } else {
            // Fallback to linear interpolation.
            snap.pos = buffer[idx].pos * (1.0 - alpha) + buffer[idx + 1].pos * alpha;
            snap.rot = QuatSlerp(buffer[idx].rot, buffer[idx + 1].rot, alpha);
        }

        snap.vel = buffer[idx].vel * (1.0 - alpha) + buffer[idx + 1].vel * alpha;
        snap.health = RoundF(Lerp(Cast<Float>(buffer[idx].health), Cast<Float>(buffer[idx + 1].health), alpha));
        snap.armor = RoundF(Lerp(Cast<Float>(buffer[idx].armor), Cast<Float>(buffer[idx + 1].armor), alpha));
    }

    // Retrieve the closest snapshot from history used for melee rollback.
    public func GetSnapshotAt(tick: Uint64) -> ref<TransformSnap> {
        let closest: ref<TransformSnap> = null;
        let bestDelta: Uint64 = 0xFFFFFFFFFFFFFFFFul;
        for i in 0 ..< history.Size() {
            let delta: Uint64 = Abs(historyTicks[i] - tick);
            if delta < bestDelta {
                bestDelta = delta;
                closest = history[i];
            }
        }
        return closest;
    }

    // Called when server adjusts tick length so interpolation remains smooth.
    public static func OnTickRateChange(ms: Uint16) -> Void {
        tickMs = ms;
        interpDelayMs = Max(80u, ms * 2u);
    }
}

public static func SnapshotInterpolator_OnTickRateChange(ms: Uint16) -> Void {
    SnapshotInterpolator.OnTickRateChange(ms);
}
