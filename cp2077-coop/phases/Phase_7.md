# 🏅 Phase 7 — Simple Deathmatch Mode

```yaml
ticket_id: "P7-1"
summary: "DM ruleset toggle"
context_files: []
spec: >
  Switch game mode at runtime:
    • Create `src/runtime/GameModeManager.reds`:
        enum GameMode { Coop, DM }
        var current : GameMode = GameMode.Coop;
        func SetMode(m: GameMode);
    • Add `/gamemode dm` console command stub that calls SetMode(DM).
    • When mode switches, log "DM enabled".
hints:
  - Console command integration can be comment placeholder.
```
```yaml
ticket_id: "P7-2"
summary: "Frag counter & HUD"
context_files: []
spec: >
  Show score board:
    • Add `src/gui/DMScoreboard.reds` with:
        var kills : Uint16;
        var deaths : Uint16;
        func Show();
        func Update(peerId: Uint32, k: Uint16, d: Uint16);
      Show when `Tab` pressed (input hook comment).
    • Append `ScoreUpdate` to EMsg with fields {peerId,k,d}.
hints:
  - HUD draw can be log placeholders.
```
```yaml
ticket_id: "P7-3"
summary: "Respawn system"
context_files: []
spec: >
  Handle player respawns:
    • `src/runtime/Respawn.reds`:
        const kRespawnDelayMs = 5000;
        var spawnPoints : array<Vector3>;
        func RequestRespawn(peerId: Uint32);
        func PerformRespawn(peerId: Uint32);
      RequestRespawn logs and starts timer (comment).
hints:
  - Fill spawnPoints with three dummy vectors.
```
```yaml
ticket_id: "P7-4"
summary: "Match timer & win condition"
context_files: []
spec: >
  End DM rounds cleanly:
    • In `GameModeManager.reds`, add fields:
        var matchTimeMs : Uint32;
        var fragLimit   : Uint16;
    • func StartDM(); func TickDM(dtMs: Uint32);
      Default: 10-min timer, 30 frag limit.
    • Log "Match over – {winnerPeer}" when either condition met.
hints:
  - TickDM will be called from server loop later.
```
```yaml
ticket_id: "P7-5"
summary: "Stat packet batching"
context_files: []
spec: >
  Reduce network spam:
    • Add `src/net/StatBatch.cpp` + `.hpp`:
        struct BatchedStats { array<Uint32> peerId; array<Uint16> k; array<Uint16> d; };
        void FlushStats();
      Comments explain batching per tick to <=1 KB/s per player.
    • Extend Connection logic to call FlushStats() at end of tick loop (comment stub).
hints:
  - Actual send code arrives later.
```