// Shares NCPD heat level among peers.
public class HeatSync {
    public static let heatLevel: Uint8 = 0;

    public static func BroadcastHeat(level: Uint8) -> Void {
        heatLevel = level;
        // NetCore.BroadcastHeat(level);
        LogChannel(n"DEBUG", "BroadcastHeat " + IntToString(level));
    }

    public static func ApplyHeat(level: Uint8) -> Void {
        heatLevel = level;
        LogChannel(n"DEBUG", "ApplyHeat " + IntToString(level));
    }
}
