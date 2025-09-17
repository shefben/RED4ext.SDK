// Build version validation using C++ generated version system
public class VersionCheck {
    // Get the current build CRC from C++ version system
    public static native func GetBuildCRC() -> Uint32;
    
    // Validate remote version compatibility
    public static native func ValidateRemoteVersion(remoteCRC: Uint32) -> Bool;
    
    // Get version string for display
    public static native func GetVersionString() -> String;
}

public func SendVersion() -> Void {
    let currentCRC = VersionCheck.GetBuildCRC();
    // NetCore.SendVersion(currentCRC);
    LogChannel(n"INFO", "SendVersion " + IntToHex(currentCRC, 8) + " (" + VersionCheck.GetVersionString() + ")");
}

public func VerifyVersion(received: Uint32) -> Bool {
    let isValid = VersionCheck.ValidateRemoteVersion(received);
    if !isValid {
        LogChannel(n"ERROR", "Version mismatch - received: " + IntToHex(received, 8) + ", expected: " + IntToHex(VersionCheck.GetBuildCRC(), 8));
        LogChannel(n"ERROR", "Local version: " + VersionCheck.GetVersionString());
        // Connection.Disconnect();
        return false;
    } else {
        LogChannel(n"INFO", "Version verified: " + VersionCheck.GetVersionString());
        return true;
    }
}
