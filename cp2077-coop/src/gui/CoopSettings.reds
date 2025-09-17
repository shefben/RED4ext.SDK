// User-configurable coop settings wrapped in proper class structure
public class CoopSettings {
    private static var tickRate: Uint16 = 30u;
    private static var minTickRate: Uint16 = 20u;
    private static var maxTickRate: Uint16 = 50u;
    private static var interpMs: Uint16 = 100u;
    private static var pushToTalk: EInputKey = EInputKey.IK_T;
    private static var voiceSampleRate: Uint32 = 48000u;
    private static var voiceBitrate: Uint32 = 24000u;
    private static var voiceUseOpus: Bool = true;
    private static var voiceVolume: Float = 1.0;
    private static var friendlyFire: Bool = false;
    private static var sharedLoot: Bool = true;
    private static var difficultyScaling: Bool = false;
    private static var difficultyLevel: Uint8 = 1u;
    private static var dynamicEvents: Bool = true;
    private static var bundleCacheLimitMb: Uint32 = 128u;
    private static var sectorTimeoutSec: Float = 10.0;
    private static var mapSize: Float = 512.0;
    private static let kDefaultSettingsPath: String = "coop.ini";

    public static func GetSectorTimeoutSec() -> Float { return sectorTimeoutSec; }
    public static func GetMapSize() -> Float { return mapSize; }
    public static func GetBundleCacheLimitMb() -> Uint32 { return bundleCacheLimitMb; }
    public static func GetPushToTalk() -> EInputKey { return pushToTalk; }
    public static func GetVoiceVolume() -> Float { return voiceVolume; }
    public static func GetFriendlyFire() -> Bool { return friendlyFire; }
    public static func GetSharedLoot() -> Bool { return sharedLoot; }

    private static native func SaveSettings(json: String) -> Void;

    public static func Show() -> Void {
        // UI-4: open settings ink panel
        LogChannel(n"DEBUG", "CoopSettings.Show");
    }

    public static func Apply() -> Void {
        GameModeManager.SetFriendlyFire(friendlyFire);
        CoopVoice.SetVolume(voiceVolume);
    }

public static func Save(path: String) -> Void {
        let json = "{\"friendlyFire\":" + BoolToString(friendlyFire) +
                    ",\"sharedLoot\":" + BoolToString(sharedLoot) +
                    ",\"difficultyScaling\":" + BoolToString(difficultyScaling) +
                    ",\"dynamicEvents\":" + BoolToString(dynamicEvents) +
                    ",\"difficulty\":" + IntToString(Cast<Int32>(difficultyLevel)) +
                    ",\"minTickRate\":" + IntToString(Cast<Int32>(minTickRate)) +
                    ",\"maxTickRate\":" + IntToString(Cast<Int32>(maxTickRate)) +
                    ",\"sectorTimeoutSec\":" + FloatToString(sectorTimeoutSec) +
                    ",\"mapSize\":" + FloatToString(mapSize) +
                    ",\"voiceVolume\":" + FloatToString(voiceVolume) + "}";
        SaveSettings(json);
        LogChannel(n"DEBUG", "Saved settings");
    }
}
