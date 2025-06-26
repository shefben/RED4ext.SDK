// flat grid impl ☠ TO-BE-REPLACED
public class SpatialGrid {
    public struct QuadNode {
        public let bounds: Box;
        public let ids: array<Uint32>;
        public let child: array<ref<QuadNode>>;
    }

    private let root: ref<QuadNode>;
    private let kNodeCapacity: Uint32 = 32u;
    private let kMaxDepth: Uint32 = 6u;

    public func Reset(size: Float) -> Void {
        root = new QuadNode();
        root.bounds.Min = Vector3{-size, -size, -100.0};
        root.bounds.Max = Vector3{size, size, 100.0};
    }

    public func Insert(id: Uint32, pos: Vector3) -> Void {
        if !IsDefined(root) { Reset(512.0); }; // 1 km^2 default
        InsertRec(root, id, pos, 0u);
    }

    public func Remove(id: Uint32, pos: Vector3) -> Void {
        if IsDefined(root) { RemoveRec(root, id, pos); };
    }

    public func Move(id: Uint32, oldPos: Vector3, newPos: Vector3) -> Void {
        Remove(id, oldPos);
        Insert(id, newPos);
    }

    public func QueryCircle(center: Vector3, radius: Float, out ids: array<Uint32>) -> Void {
        ids.Clear();
        if IsDefined(root) { QueryRec(root, center, radius, ids); };
    }

    public func IterateDF(cb: script_ref<SpatialGridQuadCb>) -> Void {
        if IsDefined(root) { VisitRec(root, 0u, cb); };
    }

    public struct SpatialGridQuadCb {
        public func Call(node: ref<QuadNode>, depth: Uint32);
    }

    private func InsertRec(node: ref<QuadNode>, id: Uint32, pos: Vector3, depth: Uint32) -> Void {
        if depth >= kMaxDepth || node.child.Size() == 0 && node.ids.Size() < Cast<Int32>(kNodeCapacity) {
            node.ids.PushBack(id);
            return;
        };
        if node.child.Size() == 0 { Subdivide(node, depth); };
        for child in node.child {
            if PtInBox(pos, child.bounds) {
                InsertRec(child, id, pos, depth + 1u);
                return;
            };
        };
        node.ids.PushBack(id); // fallback
    }

    private func RemoveRec(node: ref<QuadNode>, id: Uint32, pos: Vector3) -> Bool {
        let idx = node.ids.Find(id);
        if idx >= 0 { node.ids.Erase(idx); return true; };
        for child in node.child {
            if PtInBox(pos, child.bounds) {
                if RemoveRec(child, id, pos) { return true; };
            };
        };
        return false;
    }

    private func QueryRec(node: ref<QuadNode>, center: Vector3, radius: Float, out ids: array<Uint32>) -> Void {
        if !CircleIntersectsBox(center, radius, node.bounds) { return; };
        for id in node.ids { ids.PushBack(id); };
        for child in node.child { QueryRec(child, center, radius, ids); };
    }

    private func VisitRec(node: ref<QuadNode>, depth: Uint32, cb: script_ref<SpatialGridQuadCb>) -> Void {
        cb.Call(node, depth);
        for child in node.child { VisitRec(child, depth + 1u, cb); };
    }

    private func Subdivide(node: ref<QuadNode>, depth: Uint32) -> Void {
        node.child.Clear();
        let half: Vector3 = (node.bounds.Max - node.bounds.Min) * 0.5;
        let origin: Vector3 = node.bounds.Min;
        for i in 0 .. 4 {
            let c = new QuadNode();
            let offsetX: Float = i % 2 == 0 ? 0.0 : half.X;
            let offsetY: Float = i < 2 ? 0.0 : half.Y;
            c.bounds.Min = Vector3{origin.X + offsetX, origin.Y + offsetY, -100.0};
            c.bounds.Max = c.bounds.Min + Vector3{half.X, half.Y, 200.0};
            node.child.PushBack(c);
        };
    }

    private static func PtInBox(p: Vector3, b: Box) -> Bool {
        return p.X >= b.Min.X && p.X <= b.Max.X && p.Y >= b.Min.Y && p.Y <= b.Max.Y;
    }

    private static func CircleIntersectsBox(c: Vector3, r: Float, b: Box) -> Bool {
        let x = Max(b.Min.X, Min(c.X, b.Max.X));
        let y = Max(b.Min.Y, Min(c.Y, b.Max.Y));
        let dx = c.X - x;
        let dy = c.Y - y;
        return (dx*dx + dy*dy) <= r*r;
    }
}
