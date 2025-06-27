// Synchronizes time of day and weather between players.
public struct WorldState {
    public var sunAngleDeg: Uint16;   // rounded degrees
    public var weatherId: Uint8;
}

public class WeatherSync {
    public static let lastBroadcast: Float = 0.0;
    private static let kInterval: Float = 30.0;

    // Called on server when world state changes or every 30 s.
    public static func Broadcast(state: ref<WorldState>) -> Void {
        // NetCore.BroadcastWorldState(state);
        lastBroadcast = EngineTime.ToFloat(GameInstance.GetTimeSystem(GetGame()).GetGameTime());
        LogChannel(n"weather", "Sun " + IntToString(state.sunAngleDeg) + "  Weather " + IntToString(state.weatherId));
    }

    public static func Apply(state: ref<WorldState>) -> Void {
        let ts = GameInstance.GetTimeSystem(GetGame());
        ts.SetSunRotation(Cast<Float>(state.sunAngleDeg));
        let ws = GameInstance.GetWeatherSystem(GetGame());
        ws.SetWeather(state.weatherId);
    }
}
