public class TextureBiasSync {
    public static var bias: Uint8 = 0u;
    public static func Apply(newBias: Uint8) -> Void {
        bias = newBias;
        LogChannel(n"mem", "Mip bias=" + IntToString(Cast<Int32>(newBias)));
    }
}

public static func TextureBiasSync_OnChange(bias: Uint8) -> Void {
    TextureBiasSync.Apply(bias);
}
