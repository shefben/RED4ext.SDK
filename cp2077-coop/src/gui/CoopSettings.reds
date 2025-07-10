// User-configurable coop settings.
public var tickRate: Uint16 = 30;
public var minTickRate: Uint16 = 20;
public var maxTickRate: Uint16 = 50;
public var interpMs: Uint16 = 100;
public var pushToTalk: EKey = EKey.T;
public var voiceSampleRate: Uint32 = 48000u;
public var voiceBitrate: Uint32 = 24000u;
public var voiceUseOpus: Bool = true;
public var friendlyFire: Bool = false;
public var sharedLoot: Bool = true;
public var difficultyScaling: Bool = false;
public var difficultyLevel: Uint8 = 1u;
public var dynamicEvents: Bool = true;
public var bundleCacheLimitMb: Uint32 = 128u;
public var sectorTimeoutSec: Float = 10.0;
public var mapSize: Float = 512.0;
public static func GetSectorTimeoutSec() -> Float { return sectorTimeoutSec; }
public static func GetMapSize() -> Float { return mapSize; }
public static func GetBundleCacheLimitMb() -> Uint32 { return bundleCacheLimitMb; }
public let kDefaultSettingsPath: String = "coop.ini";
private native func SaveSettings(json: String) -> Void

public func Show() -> Void {
    // UI-4: open settings ink panel
    LogChannel(n"DEBUG", "CoopSettings.Show");
}

public func Apply() -> Void {
    GameModeManager.SetFriendlyFire(friendlyFire);
}

public func Save(path: String) -> Void {
    let json = "{\"friendlyFire\":" + BoolToString(friendlyFire) +
                ",\"sharedLoot\":" + BoolToString(sharedLoot) +
                ",\"difficultyScaling\":" + BoolToString(difficultyScaling) +
                ",\"dynamicEvents\":" + BoolToString(dynamicEvents) +
                ",\"difficulty\":" + IntToString(Cast<Int32>(difficultyLevel)) +
                ",\"minTickRate\":" + IntToString(Cast<Int32>(minTickRate)) +
                ",\"maxTickRate\":" + IntToString(Cast<Int32>(maxTickRate)) +
                ",\"sectorTimeoutSec\":" + FloatToString(sectorTimeoutSec) +
                ",\"mapSize\":" + FloatToString(mapSize) + "}";
    SaveSettings(json);
    LogChannel(n"DEBUG", "Saved settings");
}
