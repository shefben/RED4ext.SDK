// Handles vendor purchase results on the client.
public class Vendor {
    public static func OnPurchaseResult(itemId: Uint64, bal: Uint64, success: Bool) -> Void {
        if success {
            LogChannel(n"vendor", "Purchase ok");
        } else {
            LogChannel(n"vendor", "Purchase failed");
        };
        WalletHud.Update(Int64(bal));
    }
}
