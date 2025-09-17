@wrapMethod(DoorHackComponent)
protected func StartBreach() -> Void {
    if GameModeManager.current != GameMode.Coop {
        wrappedMethod();
        return;
    };
    let doorId: Uint32 = Owner.GetEntityID();
    if Net_IsAuthoritative() {
        Net_DoorBreachController_Start(doorId, QuestSync.localPhase);
    };
    DoorBreachSync.OnStart(doorId, QuestSync.localPhase, 0u);
    wrappedMethod();
}
