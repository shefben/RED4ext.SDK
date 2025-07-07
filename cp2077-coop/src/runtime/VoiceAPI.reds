public class CoopVoice {
    public static native func StartCapture(device: String) -> Bool
    public static native func EncodeFrame(pcm: script_ref<Int16>, buf: script_ref<Uint8>) -> Int32
    public static native func StopCapture() -> Void
}
