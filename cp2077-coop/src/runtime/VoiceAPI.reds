public class CoopVoice {
    public static native func StartCapture(device: String, sampleRate: Uint32, bitrate: Uint32) -> Bool
    public static native func EncodeFrame(pcm: script_ref<Int16>, buf: script_ref<Uint8>) -> Int32
    public static native func StopCapture() -> Void
    public static native func SetVolume(vol: Float) -> Void
}
