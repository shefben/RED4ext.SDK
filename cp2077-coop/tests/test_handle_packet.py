from enum import IntEnum

class MockConnectionState(IntEnum):
    Disconnected = 0
    Handshaking = 1
    Lobby = 2
    InGame = 3

class MockConnection:
    def __init__(self):
        self.state = MockConnectionState.InGame
        self.logs = []

    def transition(self, next_state):
        self.state = next_state

    def handle_packet(self, pkt_id, size_hdr, size_actual):
        if pkt_id == 1:  # Hello
            return
        else:
            self.logs.append(f"WARN: unhandled packet id={pkt_id}")
            if size_hdr != size_actual:
                self.logs.append("WARN: malformed packet")
                self.transition(MockConnectionState.Disconnected)


def test_unknown_packet():
    c = MockConnection()
    c.handle_packet(9999, 4, 3)
    assert c.logs[0].startswith("WARN: unhandled packet id=9999")
    assert c.state == MockConnectionState.Disconnected

if __name__ == "__main__":
    test_unknown_packet()
