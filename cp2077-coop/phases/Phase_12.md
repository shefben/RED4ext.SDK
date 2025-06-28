# Phase 12

```yaml
###############################################################################
# ███  “STB” SERIES – FILLING EARLY PLACEHOLDERS / STUBS                      █
###############################################################################
# Copy-ready Codex o3 tickets that replace earlier “stub / placeholder” notes.
# Numbering: STB-1 … STB-6  (add more if needed).
###############################################################################

- ticket_id: "STB-1"
  summary: "Quadtree upgrade for SpatialGrid"
  context_files:
    - path: src/runtime/SpatialGrid.reds
      excerpt: |
        // current flat grid with 32-m cells
  spec: >
    Replace flat grid with hierarchical quadtree:
      • New struct `QuadNode { AABB bounds; array<Uint32> ids; QuadNode* child[4]; }`.
      • SpatialGrid now has:
          const kNodeCapacity = 32;
          func Insert(id,pos);
          func Remove(id,pos);
          func Move(id,oldPos,newPos);
          func QueryCircle(center,radius,out ids);
      • When a node exceeds kNodeCapacity, subdivide into 4 children.
      • Provide simple depth-first iterator for debug draw.
      • Update interest diff routine (IMT-2) to call `QueryCircle`.
  hints:
    - Depth limited to 6 levels (~1 km² at 32-m base).

- ticket_id: "STB-2"
  summary: "HealthBar ink widget implementation"
  context_files:
    - path: src/gui/HealthBar.reds
      excerpt: |
        // stubbed draw call
  spec: >
    Build a functional floating health bar:
      • Create `inkCanvas` prefab `ui/healthbar.inkwidget`.
      • HealthBar class:
          func AttachTo(proxy);
          func Update(hp,armor,maxHp,maxArmor);
      • Colour code: health = green→yellow→red; armor = blue overlay.
      • Smooth lerp on hp change over 0.3 s; shake animation at <25 %.
  hints:
    - Use `inkAnim` scale-pulse for “damage taken” feedback.

- ticket_id: "STB-3"
  summary: "Synchronized explosion visual FX"
  context_files:
    - path: src/runtime/VehicleProxy.reds
      excerpt: |
        // placeholder FX id "veh_explode_small"
  spec: >
    Fully network explosion sequence:
      • Replace placeholder with call to `EffectSystem.SpawnEffect(vfxId,pos)`.
      • `VehicleExplode` packet now carries `vfxId` and `seed`.
      • Clients spawn same effect with deterministic RNG via seed.
      • Add debris spawning: 5–10 chunks with impulse based on seed.
  hints:
    - Use game VFX `"veh_explosion_big.ent"` as default.

- ticket_id: "STB-4"
  summary: "Voice playback via OpenAL Soft"
  context_files:
    - path: src/voice/VoiceDecoder.cpp
      excerpt: |
        // writes PCM to ring-buffer (placeholder)
  spec: >
    Output decoded PCM to speakers:
      • Link `openal-soft` (FindAL.cmake).
      • Init AL device “Generic Software” on first playback.
      • Create streaming buffer queue (4 buffers × 20 ms each).
      • DecodeFrame() writes PCM to next AL buffer; queue to source.
      • Drop frame if queue >8 buffers (400 ms) to avoid latency creep.
  hints:
    - Use 48 kHz mono; format AL_FORMAT_MONO16.

- ticket_id: "STB-5"
  summary: "HTTP client for heartbeat & TURN creds"
  context_files:
    - path: src/server/Heartbeat.cpp
      excerpt: |
        // JSON string assembly only
  spec: >
    Replace logging with real HTTP POST:
      • Add header-only lib `cpp-httplib` under `third_party/`.
      • Heartbeat.Send(sessionJson) sends POST to
        `https://coop-master/api/heartbeat`.
      • Parse JSON reply `{ "ok":true,"turn":{"url":"turn:...","u":"x","p":"y"} }`.
      • On success, cache TURN creds for NT-3.
      • Implements exponential back-off if server unreachable.
  hints:
    - SSL on; verify cert via builtin root store.

- ticket_id: "STB-6"
  summary: "Full ink Breach puzzle grid"
  context_files:
    - path: src/gui/BreachHud.reds
      excerpt: |
        // grid render pseudo
  spec: >
    Build interactive hex-grid UI:
      • Generate `inkFlex` grid W×H using `ico_hex_cell.inkwidget`.
      • Each cell shows code string (e.g., "1C", "55", "BD").
      • Cell states: default, hovered, selected, disabled.
      • Mouse / gamepad input highlights cell; OnSelect sends BreachInput.
      • Draw scrolling buffer row at top; show remaining attempts & timer.
      • On success/fail, play corresponding inkAnim flash (green/red).
  hints:
    - Use `inkGameController` to route input for both KBM & pad.
```