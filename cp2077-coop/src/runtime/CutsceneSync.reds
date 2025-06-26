public class CutsceneSync {
  private struct VisemeEvent {
    var npcId: Uint32;
    var visemeId: Uint8;
    var timeMs: Uint32;
  }

  private static let queuedVisemes: array<VisemeEvent>;

  public static func OnCineStart(sceneId: Uint32, startTimeMs: Uint32) -> Void {
    LogChannel(n"DEBUG", "CineStart " + IntToString(Cast<Int32>(sceneId)));
    if !Net_IsAuthoritative() {
      let camSys = GameInstance.GetCameraSystem(GetGame());
      camSys.Seek(sceneId, startTimeMs);
    };
  }

  public static func OnViseme(npcId: Uint32, visemeId: Uint8, timeMs: Uint32) -> Void {
    LogChannel(n"DEBUG", "Viseme " + IntToString(Cast<Int32>(visemeId)));
    if Net_IsAuthoritative() {
      Net_BroadcastViseme(npcId, visemeId, timeMs);
    } else {
      let ev: VisemeEvent;
      ev.npcId = npcId;
      ev.visemeId = visemeId;
      ev.timeMs = timeMs;
      ArrayPush(queuedVisemes, ev);
    };
  }

  public static func OnDialogChoice(peerId: Uint32, idx: Uint8) -> Void {
    LogChannel(n"DEBUG", "DialogChoice " + IntToString(Cast<Int32>(idx)));
    if !Net_IsAuthoritative() {
      Net_SendDialogChoice(idx);
    } else {
      if ApplyDialogChoice(idx) {
        Net_BroadcastDialogChoice(peerId, idx);
      };
    };
  }

  public static func FetchVisemeEvents(out events: array<VisemeEvent>) -> Void {
    events = queuedVisemes;
    queuedVisemes.Clear();
  }
}

public static func ApplyDialogChoice(idx: Uint8) -> Bool {
  let conv = GameInstance.GetConversationStateMachine(GetGame());
  if !IsDefined(conv) {
    return false;
  };
  if conv.IsChoiceValid(idx) {
    conv.AdvanceChoice(idx);
    return true;
  };
  return false;
}

public static func CutsceneSync_CineStart(sceneId: Uint32, startMs: Uint32) -> Void {
  CutsceneSync.OnCineStart(sceneId, startMs);
}

public static func CutsceneSync_Viseme(npcId: Uint32, visemeId: Uint8, timeMs: Uint32) -> Void {
  CutsceneSync.OnViseme(npcId, visemeId, timeMs);
}

public static func CutsceneSync_DialogChoice(peerId: Uint32, idx: Uint8) -> Void {
  CutsceneSync.OnDialogChoice(peerId, idx);
}
