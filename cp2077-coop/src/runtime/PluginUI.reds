public class PluginUI {
    public static let registry: ref<inkHashMap> = new inkHashMap();

    public static func Register(id: String, cb: func()) -> Void {
        registry.Insert(CoopNet.Fnv1a32(id), cb);
    }

    public static func Show(id: String) -> Void {
        let cb = registry.Get(CoopNet.Fnv1a32(id)) as func();
        if IsDefined(cb) {
            cb();
        } else {
            LogChannel(n"plugin", "unknown panel " + id);
        };
    }
}

public static func PluginUI_Register(id: String, cb: func()) -> Void {
    PluginUI.Register(id, cb);
}

public static func PluginUI_Show(id: String) -> Void {
    PluginUI.Show(id);
}
