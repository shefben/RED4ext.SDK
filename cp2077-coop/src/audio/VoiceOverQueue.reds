public struct VOEvent {
    public let lineId: Uint32;
    public let startTs: Uint32;
}

public class VoiceOverQueue {
    public static let events: array<VOEvent>;

    public static func OnPlay(lineId: Uint32) -> Void {
        let ts = GameClock.GetTime();
        let evt: VOEvent;
        evt.lineId = lineId;
        evt.startTs = ts;
        events.PushBack(evt);
        let audio = GameInstance.GetAudioSystem(GetGame());
        if IsDefined(audio) { audio.TriggerEventById(lineId); };
    }
}
