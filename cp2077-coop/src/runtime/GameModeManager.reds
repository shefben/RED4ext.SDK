// Manages cooperative vs deathmatch game modes.
public class GameModeManager {
    public enum GameMode {
        Coop,
        DM
    }

    public static var current: GameMode = GameMode.Coop;
    public static var matchTimeMs: Uint32;
    public static var fragLimit: Uint16;
    // Track kills per peer for win condition (index by peerId).
    public static var fragCounts: array<Uint16>;

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
    }

    public static func TickDM(dtMs: Uint32) -> Void {
        if current != GameMode.DM { return; }

        if matchTimeMs > dtMs {
            matchTimeMs -= dtMs;
        } else {
            LogChannel(n"DEBUG", "Match over – 0"); // FIXME(next ticket: winner)
            current = GameMode.Coop;
            return;
        }

        // Check frag limit; winnerPeer derived from fragCounts (placeholder).
        for i in 0 ..< fragCounts.Size() {
            if fragCounts[i] >= fragLimit {
                LogChannel(n"DEBUG", "Match over – " + IntToString(i));
                current = GameMode.Coop;
                break;
            }
        }
    }
}

// /gamemode dm console command placeholder will call SetMode(GameMode.DM).
