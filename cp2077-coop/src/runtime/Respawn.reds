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
        // Start timer to call PerformRespawn after delay.
    }

    public static func PerformRespawn(peerId: Uint32) -> Void {
        let idx: Int32 = RandRange(0, spawnPoints.Size());
        LogChannel(n"DEBUG", "PerformRespawn " + IntToString(peerId) + " -> " + VectorToString(spawnPoints[idx]));
        let avatar = GameInstance.GetPlayerSystem(GetGame()).FindObject(peerId) as AvatarProxy;
        if IsDefined(avatar) {
            avatar.pos = spawnPoints[idx];
            avatar.vel = new Vector3(0.0, 0.0, 0.0);
            avatar.health = 100u;
            avatar.armor = 100u;
            avatar.OnVitalsChanged();
        }
        let board = DMScoreboard.Instance();
        board.deaths += 1u;
        board.Update(peerId, board.kills, board.deaths);
        CoopNet.AddScore(peerId, board.kills, board.deaths);
    }
}
