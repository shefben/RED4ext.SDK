// User-configurable coop settings.
public var tickRate: Uint16 = 30;
public var interpMs: Uint16 = 100;
public var pushToTalk: EKey = EKey.T;
public var friendlyFire: Bool = false;
public var sharedLoot: Bool = true;
public var difficultyScaling: Bool = false;
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
    let json = "{\"friendlyFire\":" + BoolToString(friendlyFire) + ",\"sharedLoot\":" + BoolToString(sharedLoot) + ",\"difficultyScaling\":" + BoolToString(difficultyScaling) + "}";
    SaveSettings(json);
    LogChannel(n"DEBUG", "Saved settings");
}
