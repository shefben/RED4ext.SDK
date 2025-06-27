public class PerkSync {
    public static let hud : ref<inkHashMap> = new inkHashMap();

    public static func RequestUnlock(perkId: Uint32, rank: Uint8) -> Void {
        Net_SendPerkUnlock(perkId, rank);
    }

    public static func OnUnlock(peerId: Uint32, perkId: Uint32, rank: Uint8) -> Void {
        let map = hud.Get(peerId) as inkHashMap;
        if !IsDefined(map) {
            map = new inkHashMap();
            hud.Insert(peerId, map);
        };
        map.Insert(perkId, rank);
    }

    public static func OnRespecAck(peerId: Uint32, pts: Uint16) -> Void {
        LogChannel(n"perk", "Respec done newPoints=" + IntToString(pts));
    }
}
