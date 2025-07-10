// User-configurable coop settings.
public var tickRate: Uint16 = 30;
public var minTickRate: Uint16 = 20;
public var maxTickRate: Uint16 = 50;
public var interpMs: Uint16 = 100;
public var pushToTalk: EKey = EKey.T;
public var voiceSampleRate: Uint32 = 48000u;
public var voiceBitrate: Uint32 = 24000u;
public var friendlyFire: Bool = false;
public var sharedLoot: Bool = true;
public var difficultyScaling: Bool = false;
public var dynamicEvents: Bool = true;
public var bundleCacheLimitMB: Uint32 = 100u;
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
                ",\"minTickRate\":" + IntToString(Cast<Int32>(minTickRate)) +
                ",\"maxTickRate\":" + IntToString(Cast<Int32>(maxTickRate)) + "}";
    SaveSettings(json);
    LogChannel(n"DEBUG", "Saved settings");
}

public func GetBundleCacheLimit() -> Uint32 {
    return bundleCacheLimitMB;
}
