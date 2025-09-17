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

// Hook disabled - gamevision class may not exist in current game version
// TODO: Find correct cinematic system class to hook
// @hook(gamevision.StartCinematic)
// protected func gamevision_StartCinematic(original: func(ref<gamevision>, Uint32, Uint32), self: ref<gamevision>, sceneId: Uint32, startMs: Uint32) -> Void {
//   original(self, sceneId, startMs);
//   CutsceneSync.OnCineStart(sceneId, startMs);
//   if Net_IsAuthoritative() {
//     Net_BroadcastCineStart(sceneId, startMs, QuestSync.localPhase, false);
//   };
// }

// Alternative approach - monitor cutscene events through other means
public static func RegisterCutsceneCallbacks() -> Void {
    LogChannel(n"DEBUG", "CutsceneSync: Registering cutscene monitoring (hooks disabled)");
    // Implementation would use alternative event monitoring
}
