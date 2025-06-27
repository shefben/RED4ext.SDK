public class Apartments {
    public static func Purchase(aptId: Uint32) -> Void {
        Net_SendAptPurchase(aptId);
    }

    public static func OnPurchaseAck(aptId: Uint32, success: Bool, bal: Uint64) -> Void {
        if success {
            CoopNotice.Show("Apartment purchased");
        } else {
            CoopNotice.Show("Purchase failed");
        };
        WalletHud.Update(Int64(bal));
    }
}

public static func Apartments_OnPurchaseAck(aptId: Uint32, success: Bool, bal: Uint64) -> Void {
    Apartments.OnPurchaseAck(aptId, success, bal);
}

public static func Apartments_OnEnterAck(allow: Bool, phaseId: Uint32, seed: Uint32) -> Void {
    if allow {
        CoopNotice.Show("Entered apartment");
    } else {
        CoopNotice.Show("Entry denied");
    };
}
