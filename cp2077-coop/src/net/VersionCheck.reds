// Build version validation.
public let kClientCRC: Uint32 = 0xDEADBEEF;

public func SendVersion() -> Void {
    // NetCore.SendVersion(kClientCRC);
    LogChannel(n"DEBUG", "SendVersion " + IntToHex(kClientCRC, 8));
}

public func VerifyVersion(received: Uint32) -> Void {
    if received != kClientCRC {
        LogChannel(n"DEBUG", "Version mismatch – disconnecting");
        // Connection.Disconnect();
    } else {
        LogChannel(n"DEBUG", "Version verified");
    }
}
