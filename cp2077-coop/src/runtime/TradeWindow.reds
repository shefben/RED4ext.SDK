public class TradeWindow extends inkHUDLayer {
    private static let s_instance: ref<TradeWindow>;
    public static exec func Trade(peer: Int32) -> Void {
        CoopNotice.Show("Opening trade");
        CoopNet.Net_SendTradeInit(Cast<Uint32>(peer));
    }
    public static func OnInit(from: Uint32) -> Void {
        if IsDefined(s_instance) { return; };
        let hud = GameInstance.GetHUDManager(GetGame());
        s_instance = new TradeWindow();
        hud.AddLayer(s_instance);
    }
    public static func OnOffer(from: Uint32, items: script_ref<array<ItemSnap>>, count: Uint8, eddies: Uint32) -> Void {
        LogChannel(n"trade", "offer from=" + IntToString(Cast<Int32>(from)));
    }
    public static func OnAccept(peer: Uint32, ok: Bool) -> Void {
        LogChannel(n"trade", "accept from=" + IntToString(Cast<Int32>(peer)));
    }
    public static func OnFinalize(ok: Bool) -> Void {
        CoopNotice.Show("Trade " + (ok ? "complete" : "cancelled"));
        if IsDefined(s_instance) {
            let hud = GameInstance.GetHUDManager(GetGame());
            hud.RemoveLayer(s_instance);
            s_instance = null;
        };
    }
}

public static func TradeWindow_OnInit(peer: Uint32) -> Void {
    TradeWindow.OnInit(peer);
}
public static func TradeWindow_OnOffer(from: Uint32, items: script_ref<array<ItemSnap>>, count: Uint8, eddies: Uint32) -> Void {
    TradeWindow.OnOffer(from, items, count, eddies);
}
public static func TradeWindow_OnAccept(p: Uint32, ok: Bool) -> Void {
    TradeWindow.OnAccept(p, ok);
}
public static func TradeWindow_OnFinalize(ok: Bool) -> Void {
    TradeWindow.OnFinalize(ok);
}
