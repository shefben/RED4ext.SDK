// User-configurable coop settings.
public var tickRate: Uint16 = 30;
public var interpMs: Uint16 = 100;
public var pushToTalk: EKey = EKey.T;
public let kDefaultSettingsPath: String = "coop.ini";

public func Show() -> Void {
    // UI-4: open settings ink panel
    LogChannel(n"DEBUG", "CoopSettings.Show");
}

public func Save(path: String) -> Void {
    LogChannel(n"DEBUG", "Saving settings to " + path);
}
