public class ItemGrabSync {
    public static func OnGrab(pkt: ref<ItemGrabPacket>) -> Void {
        LogChannel(n"itemgrab", "grab=" + IntToString(Cast<Int32>(pkt.itemId)));
        if pkt.itemId == 0xBBCHIPu {
            RED4ext.ExecuteFunction("HUDNotifications", "ShowEvent", null, 0xBBCHIPu);
        };
    }
    public static func OnDrop(pkt: ref<ItemDropPacket>) -> Void {
        LogChannel(n"itemgrab", "drop=" + IntToString(Cast<Int32>(pkt.itemId)));
    }
    public static func OnStore(pkt: ref<ItemStorePacket>) -> Void {
        LogChannel(n"itemgrab", "store=" + IntToString(Cast<Int32>(pkt.itemId)));
    }
}

public static func ItemGrabSync_OnGrab(pkt: ref<ItemGrabPacket>) -> Void {
    ItemGrabSync.OnGrab(pkt);
}

public static func ItemGrabSync_OnDrop(pkt: ref<ItemDropPacket>) -> Void {
    ItemGrabSync.OnDrop(pkt);
}

public static func ItemGrabSync_OnStore(pkt: ref<ItemStorePacket>) -> Void {
    ItemGrabSync.OnStore(pkt);
}
