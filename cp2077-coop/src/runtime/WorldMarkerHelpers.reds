public class WorldMarkerHelpers {
    public static func GatherPositions() -> array<Vector3> {
        let res: array<Vector3>;
        let qs = QuestSystem.GetInstance(GetGame());
        if IsDefined(qs) {
            let gigs = qs.ListOpenActivities();
            for g in gigs { res.PushBack(g.position); };
        };
        let mm = MinimapSystem.GetInstance(GetGame());
        if IsDefined(mm) {
            let pins = mm.GetCustomPins();
            for p in pins { res.PushBack(p); };
        };
        let ncpd = NCPDSystem.GetInstance(GetGame());
        if IsDefined(ncpd) {
            let events = ncpd.GetNearbyEvents(500.0);
            for e in events { res.PushBack(e.pos); };
        };
        return res;
    }

    public static func ApplyPosition(pos: Vector3) -> Void {
        CoopMap.AddMarker(pos);
    }
}
