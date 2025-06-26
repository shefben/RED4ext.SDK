// Handles delayed respawn of players in deathmatch.
public class Respawn {
    public static let kRespawnDelayMs: Uint32 = 5000u;
    public static var spawnPoints: array<Vector3> = [
        new Vector3(0.0, 0.0, 0.0),
        new Vector3(10.0, 0.0, 0.0),
        new Vector3(-10.0, 0.0, 0.0)
    ];

    public static func RequestRespawn(peerId: Uint32) -> Void {
        LogChannel(n"DEBUG", "RequestRespawn " + IntToString(peerId));
        GameInstance.GetDelaySystem(GetGame()).DelayCallback(Respawn, n"PerformRespawn", Cast<Float>(kRespawnDelayMs) / 1000.0, peerId);
    }

    public static func PerformRespawn(peerId: Uint32) -> Void {
        let players = GameInstance.GetPlayerSystem(GetGame()).GetPlayers();
        let bestIdx: Int32 = 0;
        var bestScore: Float = -1.0;
        for i in 0 ..< spawnPoints.Size() {
            var minDist: Float = 10000.0;
            for p in players {
                if p.peerId != peerId {
                    let d = VectorLength(spawnPoints[i] - p.pos);
                    if d < minDist { minDist = d; };
                };
            };
            if minDist > bestScore { bestScore = minDist; bestIdx = i; };
        };
        LogChannel(n"DEBUG", "PerformRespawn " + IntToString(peerId) + " -> " + VectorToString(spawnPoints[bestIdx]));
        let avatar = GameInstance.GetPlayerSystem(GetGame()).FindObject(peerId) as AvatarProxy;
        if IsDefined(avatar) {
            avatar.pos = spawnPoints[bestIdx];
            avatar.vel = new Vector3(0.0, 0.0, 0.0);
            avatar.health = 100u;
            avatar.armor = 100u;
            avatar.OnVitalsChanged();
            if HasMethod(avatar, n"SetGodMode") { avatar.SetGodMode(true); };
            GameInstance.GetDelaySystem(GetGame()).DelayCallback(avatar, n"ClearInvuln", 2.0);
        }
        let board = DMScoreboard.Instance();
        board.deaths += 1u;
        board.Update(peerId, board.kills, board.deaths);
        CoopNet.AddStats(peerId, board.kills, board.deaths, 0u, 0u, 0u);
    }
}
