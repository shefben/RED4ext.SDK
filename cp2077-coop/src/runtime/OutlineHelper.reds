public class OutlineHelper {
    public static func AddPing(id: Uint32, dur: Uint16) -> Void {
        let container = GameInstance.GetScriptableSystemsContainer(GetGame());
        let outlineSys = container.Get(n"OutlineSystem") as OutlineSystem;
        if IsDefined(outlineSys) {
            outlineSys.RequestPulse(id, Cast<Float>(dur));
        };
        CoopNotice.Show("Ping " + IntToString(Cast<Int32>(id)));
        LogChannel(n"ping", "outline " + IntToString(Cast<Int32>(id)) + " dur=" + IntToString(Cast<Int32>(dur)));
    }
}
