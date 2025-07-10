from enum import IntEnum

class MockConn:
    def __init__(self):
        self.logs = []

    def handle_worldmarkers(self, size_hdr, blob):
        expected = 4 + blob
        if size_hdr != expected:
            self.logs.append("WARN: WorldMarkers size mismatch")
            return
        self.logs.append("processed")

def test_worldmarkers_size_mismatch():
    c = MockConn()
    c.handle_worldmarkers(10, 5)
    assert c.logs == ["WARN: WorldMarkers size mismatch"]


def test_worldmarkers_size_ok():
    c = MockConn()
    c.handle_worldmarkers(9, 5)
    assert c.logs == ["processed"]

