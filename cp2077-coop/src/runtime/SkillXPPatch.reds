@wrapMethod(StatsSystem)
public final func AddSkillXP(skill: gamedataProficiencyType, amount: Float, reason: Int32, owner: wref<GameObject>) -> Void {
    if Net_IsAuthoritative() {
        wrappedMethod(skill, amount, reason, owner);
    } else {
        SkillSync.RequestXP(Cast<Uint16>(skill), Cast<Int16>(RoundF(amount)));
    };
}
