// Manages cooperative vs deathmatch game modes.
public class GameModeManager {
    public enum GameMode {
        Coop,
        DM,
        Spectate
    }

    public static var current: GameMode = GameMode.Coop;
    public static var matchTimeMs: Uint32;
    public static var fragLimit: Uint16;
    public static var friendlyFire: Bool = false;
    // Track kills per peer for win condition (index by peerId).
    public static var fragCounts: array<Uint16>;
    public static var firstKillPeer: Uint32 = 0xFFFFFFFFu;

    public static func SetFriendlyFire(enable: Bool) -> Void {
        if friendlyFire == enable { return; };
        friendlyFire = enable;
        if CoopNet.IsAuthoritative() {
            CoopNet.BroadcastRuleChange(enable);
        };
    }

    public static func SetMode(m: GameMode) -> Void {
        if current == m {
            return;
        }
        current = m;
        if m == GameMode.DM {
            StartDM();
            LogChannel(n"DEBUG", "DM enabled");
        }
    }

    public static func StartDM() -> Void {
        matchTimeMs = 600000u; // 10 minutes
        fragLimit = 30u;
        fragCounts.Clear();
        firstKillPeer = 0xFFFFFFFFu;
    }

    public static func AddFrag(peerId: Uint32) -> Void {
        if peerId >= fragCounts.Size() {
            fragCounts.Resize(peerId + 1u);
        };
        fragCounts[peerId] += 1u;
        if firstKillPeer == 0xFFFFFFFFu {
            firstKillPeer = peerId;
        };
        CoopNet.AddStats(peerId, fragCounts[peerId], 0u, 0u, 0u, 0u);
    }

    public static func TickDM(dtMs: Uint32) -> Void {
        if current != GameMode.DM { return; }

        if matchTimeMs > dtMs {
            matchTimeMs -= dtMs;
        } else {
            matchTimeMs = 0u;
        }

        let winner: Uint32 = 0u;
        let best: Uint16 = 0u;
        for i in 0 ..< fragCounts.Size() {
            let frags = fragCounts[i];
            if frags > best {
                best = frags;
                winner = i;
            };
            if frags >= fragLimit {
                matchTimeMs = 0u;
            };
        }

        if matchTimeMs == 0u {
            if best > 0u && firstKillPeer != 0xFFFFFFFFu && fragCounts[firstKillPeer] == best {
                winner = firstKillPeer;
            };
            CoopNet.BroadcastMatchOver(winner);
            current = GameMode.Coop;
            return;
        }
    }
}

// P7-1: console command will call SetMode(GameMode.DM)
