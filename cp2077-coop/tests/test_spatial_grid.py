import random
from typing import List, Tuple

# Simple Python quadtree mirroring SpatialGrid.reds
class QuadNode:
    def __init__(self, bounds: Tuple[float, float, float, float]):
        self.bounds = bounds
        self.ids: List[int] = []
        self.child: List[QuadNode] = []

kNodeCapacity = 32
kMaxDepth = 6

class SpatialGrid:
    def __init__(self, size: float = 512.0):
        self.root = QuadNode((-size, -size, size, size))
        self.pos_map = {}

    def insert(self, idx: int, pos: Tuple[float, float]):
        self.pos_map[idx] = pos
        self._insert(self.root, idx, pos, 0)

    def _insert(self, node: QuadNode, idx: int, pos: Tuple[float, float], depth: int):
        if depth >= kMaxDepth or (not node.child and len(node.ids) < kNodeCapacity):
            node.ids.append(idx)
            return
        if not node.child:
            self._subdivide(node)
            old = list(node.ids)
            node.ids.clear()
            for e in old:
                p = self.pos_map[e]
                self._insert(node, e, p, depth)
        for ch in node.child:
            if self._pt_in_box(pos, ch.bounds):
                self._insert(ch, idx, pos, depth + 1)
                return
        node.ids.append(idx)

    def query(self, center: Tuple[float, float], radius: float) -> List[int]:
        res: List[int] = []
        self._query(self.root, center, radius, res)
        return res

    def _query(self, node: QuadNode, c: Tuple[float, float], r: float, out: List[int]):
        if not self._circle_intersects_box(c, r, node.bounds):
            return
        for idx in node.ids:
            p = self.pos_map[idx]
            dx = p[0] - c[0]
            dy = p[1] - c[1]
            if dx*dx + dy*dy <= r*r:
                out.append(idx)
        for ch in node.child:
            self._query(ch, c, r, out)

    def _subdivide(self, node: QuadNode):
        minx, miny, maxx, maxy = node.bounds
        hx = (maxx - minx) * 0.5
        hy = (maxy - miny) * 0.5
        for i in range(4):
            ox = minx + (hx if i % 2 else 0)
            oy = miny + (hy if i >= 2 else 0)
            node.child.append(QuadNode((ox, oy, ox + hx, oy + hy)))
        old = list(node.ids)
        node.ids.clear()
        for e in old:
            p = self.pos_map[e]
            self._insert(node, e, p, 0)

    @staticmethod
    def _pt_in_box(p: Tuple[float, float], b: Tuple[float, float, float, float]):
        return b[0] <= p[0] <= b[2] and b[1] <= p[1] <= b[3]

    @staticmethod
    def _circle_intersects_box(c: Tuple[float, float], r: float, b: Tuple[float, float, float, float]):
        x = max(b[0], min(c[0], b[2]))
        y = max(b[1], min(c[1], b[3]))
        dx = c[0] - x
        dy = c[1] - y
        return dx*dx + dy*dy <= r*r


def brute_force(points: List[Tuple[float, float]], center: Tuple[float, float], r: float) -> int:
    r2 = r * r
    return sum(1 for p in points if (p[0]-center[0])**2 + (p[1]-center[1])**2 <= r2)


def main():
    random.seed(0)
    grid = SpatialGrid()
    points = []
    for i in range(5000):
        x = random.uniform(-512, 512)
        y = random.uniform(-512, 512)
        points.append((x, y))
        grid.insert(i, (x, y))

    for _ in range(10):
        qx = random.uniform(-512, 512)
        qy = random.uniform(-512, 512)
        r = random.uniform(10, 100)
        brute = brute_force(points, (qx, qy), r)
        qres = len(set(grid.query((qx, qy), r)))
        assert brute == qres, f"mismatch: {brute} vs {qres}"


if __name__ == "__main__":
    main()

