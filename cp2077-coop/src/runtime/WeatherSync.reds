// Synchronizes time of day and weather between players.
public struct WorldState {
    public var sunAngle: Uint32; // degrees * 100
    public var weatherId: Uint8;
}

public class WeatherSync {
    public static let lastBroadcast: Float = 0.0;
    private static let kInterval: Float = 30.0;

    // Called on server when world state changes or every 30 s.
    public static func Broadcast(state: ref<WorldState>) -> Void {
        // NetCore.BroadcastWorldState(state);
        lastBroadcast = EngineTime.ToFloat(GameInstance.GetTimeSystem(GetGame()).GetGameTime());
        LogChannel(n"DEBUG", "BroadcastWorldState sun=" + IntToString(state.sunAngle));
    }

    public static func Apply(state: ref<WorldState>) -> Void {
        LogChannel(n"DEBUG", "ApplyWorldState sun=" + IntToString(state.sunAngle));
    }
}
