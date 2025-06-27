public class VendorSync {
    public static let stock : ref<inkHashMap> = new inkHashMap();

    public static func OnStock(pkt: ref<VendorStockPacket>) -> Void {
        stock.Insert(pkt.vendorId, pkt);
    }

    public static func OnStockUpdate(pkt: ref<VendorStockUpdatePacket>) -> Void {
        let entry = stock.Get(pkt.vendorId) as VendorStockPacket;
        if IsDefined(entry) {
            let count: Int32 = Cast<Int32>(entry.count);
            var i: Int32 = 0;
            while i < count {
                if entry.items[i].itemId == pkt.itemId {
                    entry.items[i].qty = pkt.qty;
                    break;
                };
                i += 1;
            };
        };
    }
}
