public class ClientPluginProxy {
    public static let callbacks: ref<inkHashMap> = new inkHashMap();

    public static func register_rpc(hash: Uint32, cb: func(json: String)) -> Void {
        callbacks.Insert(hash, cb);
    }

    private static func _popup(json: String) -> Void {
        CoopNotice.Show(json);
    }

    public static func Init() -> Void {
        register_rpc(CoopNet.Fnv1a32("popup"), _popup);
    }

    public static func OnRpc(pkt: ref<PluginRPCPacket>) -> Void {
        let fn = callbacks.Get(pkt.fnHash) as func(String);
        if IsDefined(fn) {
            let text = String.FromBytes(pkt.json, Cast<Int32>(pkt.jsonBytes));
            fn(text);
        } else {
            LogChannel(n"plugin", "unhandled rpc=" + IntToString(Cast<Int32>(pkt.fnHash)));
        };
    }
}

public static func ClientPluginProxy_OnRpc(pkt: ref<PluginRPCPacket>) -> Void {
    ClientPluginProxy.OnRpc(pkt);
}

public static func ClientPluginProxy_Init() -> Void {
    ClientPluginProxy.Init();
}
