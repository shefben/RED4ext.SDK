public class CrimeSpawner {
    public static func OnEvent(pkt: ref<CrimeEventSpawnPacket>) -> Void {
        LogChannel(n"ncpd", "Crime event id=" + IntToString(Cast<Int32>(pkt.eventId)));
    }
}
