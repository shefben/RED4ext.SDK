import random
from test_spatial_grid import SpatialGrid

# rough bandwidth estimate comparing full vs interest-based snapshots

def test_interest_bandwidth():
    random.seed(0)
    grid = SpatialGrid()
    npc_count = 200
    for i in range(npc_count):
        x = random.uniform(-512, 512)
        y = random.uniform(-512, 512)
        grid.insert(i, (x, y))

    players = [(-100, -100), (50, 75), (200, -150), (300, 300)]
    full_bytes = npc_count * len(players) * 64
    interest_bytes = 0
    for p in players:
        ids = grid.query(p, 80.0)
        interest_bytes += len(ids) * 64
    assert interest_bytes < full_bytes
