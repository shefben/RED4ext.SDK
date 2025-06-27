public class FixerCallSync {
    public static var currentId: Uint32 = 0u;

    public static func OnStart(id: Uint32) -> Void {
        currentId = id;
        LogChannel(n"fixer", "Call start id=" + IntToString(Cast<Int32>(id)));
    }

    public static func OnEnd(id: Uint32) -> Void {
        LogChannel(n"fixer", "Call end id=" + IntToString(Cast<Int32>(id)));
    }
}

public static func FixerCallSync_OnStart(id: Uint32) -> Void {
    FixerCallSync.OnStart(id);
}

public static func FixerCallSync_OnEnd(id: Uint32) -> Void {
    FixerCallSync.OnEnd(id);
}
