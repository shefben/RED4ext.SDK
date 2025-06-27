public class PhaseTrigger {
  public static let spawned : ref<inkHashMap> = new inkHashMap();

  public static func SpawnPhaseTrigger(baseEntId: Uint32, phaseId: Uint32) -> Void {
    if !Net_IsAuthoritative() { return; };
    RED4ext.ExecuteFunction("TriggerSystem", "SpawnPhaseTrigger", null, baseEntId, phaseId);
    var list = spawned.Get(phaseId) as array<Uint32>;
    if !IsDefined(list) { list = []; };
    ArrayPush(list, baseEntId);
    spawned.Insert(phaseId, list);
  }

  public static func ClearPhaseTriggers(phaseId: Uint32) -> Void {
    let arr = spawned.Get(phaseId) as array<Uint32>;
    if IsDefined(arr) {
      for id in arr { RED4ext.ExecuteFunction("TriggerSystem", "DestroyTrigger", null, id); };
      spawned.Erase(phaseId);
    };
  }
}
