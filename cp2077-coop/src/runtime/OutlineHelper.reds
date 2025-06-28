public class OutlineHelper {
    public static func AddPing(id: Uint32, dur: Uint16) -> Void {
        CoopNotice.Show("Ping " + IntToString(Cast<Int32>(id)));
        LogChannel(n"ping", "outline " + IntToString(Cast<Int32>(id)) + " dur=" + IntToString(Cast<Int32>(dur)));
    }
}
