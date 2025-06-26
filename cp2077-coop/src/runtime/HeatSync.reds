// Shares NCPD heat level among peers.
public class HeatSync {
    public static let heatLevel: Uint8 = 0;
    public static let armorScale: Float = 1.0;
    public static let damageScale: Float = 1.0;

    public static func BroadcastHeat(level: Uint8) -> Void {
        heatLevel = level;
        Net_BroadcastHeat(level);
        LogChannel(n"DEBUG", "BroadcastHeat " + IntToString(level));
    }

    public static func ApplyHeat(level: Uint8) -> Void {
        heatLevel = level;
        LogChannel(n"DEBUG", "ApplyHeat " + IntToString(level));
    }

    public static func ApplyArmorDebuff(scale: Float) -> Void {
        armorScale = scale;
        LogChannel(n"DEBUG", "ApplyArmorDebuff scale=" + FloatToString(scale));
    }

    public static func ApplyDamageBuff(scale: Float) -> Void {
        damageScale = scale;
        LogChannel(n"DEBUG", "ApplyDamageBuff scale=" + FloatToString(scale));
    }
}
