public class VendorSync {
    public static let stock : ref<inkHashMap> = new inkHashMap();

    public static func OnStock(pkt: ref<VendorStockPacket>) -> Void {
        stock.Insert(pkt.vendorId, pkt);
    }
}
