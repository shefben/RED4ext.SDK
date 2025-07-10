public struct ModEntry {
    public var name: String;
    public var version: String;
    public var enabled: Bool;
}

public class ModManager {
    public static let kPrefsPath: String = "coop.ini";
    private static let mods: array<ModEntry> = [];

    private static native func ModManager_ListMods() -> array<ModEntry>;
    private static native func ModManager_SetEnabled(name: String, enable: Bool) -> Void;
    private static native func ModManager_SaveIni(path: String, lines: script_ref<String>) -> Void;
    private static native func ModManager_Restart() -> Void;

    public static func Refresh() -> Void {
        mods = ModManager_ListMods();
    }

    public static func GetMods() -> array<ModEntry> {
        if mods.Size() == 0 {
            Refresh();
        };
        return mods;
    }

    public static func Toggle(index: Int32) -> Void {
        if index < 0 || index >= mods.Size() { return; };
        let m = mods[index];
        let newState = !m.enabled;
        ModManager_SetEnabled(m.name, newState);
        m.enabled = newState;
        mods[index] = m;
        SavePrefs();
        ModManager_Restart();
    }

    private static func SavePrefs() -> Void {
        var lines: array<String>;
        lines.Push("[Plugins]");
        let count = mods.Size();
        let i = 0;
        while i < count {
            let p = mods[i];
            lines.Push(p.name + "=" + BoolToString(p.enabled));
            i += 1;
        };
        ModManager_SaveIni(kPrefsPath, lines[0]);
    }
}

public static func ModManager_GetMods() -> array<ModEntry> {
    return ModManager.GetMods();
}

public static func ModManager_Toggle(index: Int32) -> Void {
    ModManager.Toggle(index);
}
