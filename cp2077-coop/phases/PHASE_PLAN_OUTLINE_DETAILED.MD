```yaml
# ===== PHASE_PLAN_DETAILED =====
# Core boot-strap through World/Quest Sync

# ───────────────────────────────────────────────
# Phase 0 — Bootstrapping
# ───────────────────────────────────────────────
- ticket_id: "P0-1"
  summary: "Blank mod skeleton"
  spec: >
    • Create /src /assets /build /phases  
    • CMakeLists.txt, manifest.json  
    • Main.reds logs "cp2077-coop loaded"

- ticket_id: "P0-2"
  summary: "Hot-reload harness"
  spec: >
    • Add OnReload() in Main.reds → logs "reloaded"

- ticket_id: "P0-3"
  summary: "Networking stub lib"
  spec: >
    • Net_Init / Net_Shutdown stubs  
    • ENet headers added; CMake link note

- ticket_id: "P0-4"
  summary: "Headless server scaffold"
  spec: >
    • coop_dedicated.exe with --help banner  
    • Calls Net_Init; prints "Dedicated up"

# ───────────────────────────────────────────────
# Phase 1 — Core Session Pipeline
# ───────────────────────────────────────────────
- ticket_id: "P1-1"
  summary: "Packet enums & 4-byte header"
  spec: >
    • enum EMsg list  
    • struct PacketHeader{u16 type,u16 size}  
    • helpers Packet_GetSize / Packet_SetSize

- ticket_id: "P1-2"
  summary: "Connection state machine"
  spec: >
    • States: Disconnected → Handshaking → Lobby → InGame  
    • Log every transition

- ticket_id: "P1-3"
  summary: "Authoritative host flag"
  spec: >
    • NetConfig.hpp → constexpr bool kDedicatedAuthority = true

- ticket_id: "P1-4"
  summary: "Fixed tick-lock"
  spec: >
    • GameClock::Tick(dtMs) (32 ms)  
    • GameClock::GetCurrentTick()

- ticket_id: "P1-5"
  summary: "RNG seed packet"
  spec: >
    • EMsg::Seed & SeedPacket{uint32 seed}  
    • Doc example exchange

- ticket_id: "P1-6"
  summary: "Snapshot skeleton"
  spec: >
    • SnapshotWriter/Reader stubs  
    • Dirty-bit array; baseline chaining comments

# ───────────────────────────────────────────────
# Phase 2 — Player Replication Basics
# ───────────────────────────────────────────────
- ticket_id: "P2-1"
  summary: "Spawn & despawn RPCs"
  spec: >
    • AvatarSpawn / AvatarDespawn messages  
    • AvatarProxy.reds with peerId & isLocal

- ticket_id: "P2-2"
  summary: "Transform snapshot"
  spec: >
    • TransformSnap{Vector3 pos,vel; Quaternion rot}

- ticket_id: "P2-3"
  summary: "Interpolation buffer"
  spec: >
    • SnapshotInterpolator Push/Sample (linear lerp placeholder)

- ticket_id: "P2-4"
  summary: "Prediction & reconciliation"
  spec: >
    • pendingInputs[], Predict(dt), Reconcile(snap)  
    • Snap if error >0.25 m

- ticket_id: "P2-5"
  summary: "Health & armor bars"
  spec: >
    • Extend TransformSnap with health/armor  
    • HealthBar.reds floating UI stub

- ticket_id: "P2-6"
  summary: "Ragdoll on disconnect & killfeed"
  spec: >
    • AvatarProxy.Despond() → SetRagdollMode(true)  
    • Killfeed.Push("{peerId} disconnected")

# ───────────────────────────────────────────────
# Phase 3 — UI & Lobby Flow
# ───────────────────────────────────────────────
- ticket_id: "P3-1"
  summary: "Main-menu CO-OP button"
  spec: >
    • Inject inkButton "CO-OP" into title screen  
    • Opens CoopRootPanel.Show()

- ticket_id: "P3-2"
  summary: "Server-browser panel"
  spec: >
    • ServerBrowser.reds Refresh(jsonList)  
    • tests/static/serverlist.json fixture

- ticket_id: "P3-3"
  summary: "Join & Host buttons"
  spec: >
    • Host ⇒ spawn coop_dedicated subprocess  
    • Join ⇒ JoinRequest packet, auto-connect

- ticket_id: "P3-4"
  summary: "In-game chat overlay"
  spec: >
    • ChatOverlay.reds toggle on Enter  
    • Reliable ChatMsg packet; 50-line history

# ───────────────────────────────────────────────
# Phase 4 — World & Quest Sync
# ───────────────────────────────────────────────
- ticket_id: "P4-1"
  summary: "Quest stage broadcast"
  spec: >
    • EMsg::QuestStage; QuestSync.reds OnAdvanceStage → SendQuestStageMsg

- ticket_id: "P4-2"
  summary: "Trigger / scene sync"
  spec: >
    • EMsg::SceneTrigger start/stop; ApplyScene on clients

- ticket_id: "P4-3"
  summary: "Inventory authority"
  spec: >
    • ownerId field in ItemSnap  
    • LootAuthority.CanPickup()

- ticket_id: "P4-4"
  summary: "Net-safe pop-ups"
  spec: >
    • PopupGuard.reds IsPopupBlocked / ReplaceWithCoopNotice

- ticket_id: "P4-5"
  summary: "Savegame forking"
  spec: >
    • co-op saves under SavedGames/Coop/  
    • SaveFork.GetSessionSavePath()

# ───────────────────────────────────────────────
# Phase 5 — Combat Fidelity
# ───────────────────────────────────────────────
- ticket_id: "P5-1"
  summary: "Hitscan lag compensation"
  spec: >
    • Rewind target N ms on server when confirming hit

- ticket_id: "P5-2"
  summary: "Melee rollback buffer"
  spec: >
    • 250 ms client rollback window for sword swings

- ticket_id: "P5-3"
  summary: "Quickhack sync"
  spec: >
    • HackInfo{target,hackId,duration}; server validates

- ticket_id: "P5-4"
  summary: "Armor damage validation"
  spec: >
    • Damage ≤ (armor*4+200) else cheat log

- ticket_id: "P5-5"
  summary: "Shared NCPD heat"
  spec: >
    • HeatSync broadcasts star level & wave spawns

- ticket_id: "P5-6"
  summary: "World state (time & weather)"
  spec: >
    • WorldState pkt {sunAngleDeg,weatherId}; 30 s broadcast

# ───────────────────────────────────────────────
# Phase 6 — Vehicles & Mounts
# ───────────────────────────────────────────────
- ticket_id: "P6-1"
  summary: "Vehicle spawn replication"
  spec: >
    • VehicleProxy.Spawn(id,transform)

- ticket_id: "P6-2"
  summary: "Seat assignment RPC"
  spec: >
    • SeatAssign{peerId,vehicleId,seatIdx}

- ticket_id: "P6-3"
  summary: "Deterministic car physics"
  spec: >
    • ServerSimulate / ClientPredict fixed-step Euler

- ticket_id: "P6-4"
  summary: "Collision damage sync"
  spec: >
    • VehicleHitMsg {vehicleId,dmg}; client accumulates

# ───────────────────────────────────────────────
# Phase 7 — Simple Death-Match
# ───────────────────────────────────────────────
- ticket_id: "P7-1"
  summary: "DM ruleset toggle"
  spec: >
    • /gamemode dm → GameModeManager.SetMode(DM)

- ticket_id: "P7-2"
  summary: "Frag counter & HUD"
  spec: >
    • DMScoreboard.reds; Tab shows leaderboard

- ticket_id: "P7-3"
  summary: "Respawn system"
  spec: >
    • 5 s timer; random point teleport

- ticket_id: "P7-4"
  summary: "Match timer & win"
  spec: >
    • 10-min or 30 frags → Match over broadcast

- ticket_id: "P7-5"
  summary: "Stat packet batching"
  spec: >
    • BatchedStats flush ≤1 KB / s

# ───────────────────────────────────────────────
# Phase 8 — Hardening & Polish
# ───────────────────────────────────────────────
- ticket_id: "P8-1"
  summary: "Thread-safe queue"
  spec: >
    • core/ThreadSafeQueue<T> with mutex

- ticket_id: "P8-2"
  summary: "Crash telemetry"
  spec: >
    • CaptureCrash(reason); zip netlog + dump

- ticket_id: "P8-3"
  summary: "Version hash check"
  spec: >
    • Version pkt {CRC}; disconnect on mismatch

- ticket_id: "P8-4"
  summary: "Master-server heartbeat"
  spec: >
    • JSON POST /register every 30 s

- ticket_id: "P8-5"
  summary: "Settings panel"
  spec: >
    • CoopSettings.reds tickRate/interp/push-to-talk

# ───────────────────────────────────────────────
# Extended Phase NR — NPC & Crowd
# ───────────────────────────────────────────────
- ticket_id: "NR-1"
  summary: "NpcSnap schema"
  spec: >
    • npcId, templateId, pos, rot, state, health, appearanceSeed

- ticket_id: "NR-2"
  summary: "Server-side NPC authority loop"
  spec: >
    • NpcController.ServerTick(); broadcast NpcSnapshot

- ticket_id: "NR-3"
  summary: "Client NPC proxy & LOD"
  spec: >
    • NpcProxy.reds spawn mesh; despawn >120 m

- ticket_id: "NR-4"
  summary: "Interest management distance filter"
  spec: >
    • Only send if <80 m or state==Combat

# ───────────────────────────────────────────────
# Extended Phase SL — Streaming / Sectors
# ───────────────────────────────────────────────
- ticket_id: "SL-1"
  summary: "Sector identifier broadcast"
  spec: >
    • SectorChange {peerId,sectorHash}; AvatarProxy.currentSector

- ticket_id: "SL-2"
  summary: "Sector filter for NPC snaps"
  spec: >
    • NpcSnap.sectorHash; send if matches client sector

- ticket_id: "SL-3"
  summary: "Asset-ready handshake"
  spec: >
    • Client sends SectorReady after streaming; server queues snaps

# ───────────────────────────────────────────────
# Extended Phase IC — Inventory & Crafting
# ───────────────────────────────────────────────
- ticket_id: "IC-1"
  summary: "Item stat replication"
  spec: >
    • ItemSnap{itemId,ownerId,tpl,level,quality,rolls[4]}

- ticket_id: "IC-2"
  summary: "Craft request/response"
  spec: >
    • CraftRequest recipeId → CraftResult ItemSnap

- ticket_id: "IC-3"
  summary: "Upgrade & attachment sync"
  spec: >
    • slotMask + attachmentIds[4]; AttachModRequest/Result

# ───────────────────────────────────────────────
# Extended Phase VD — Vehicle Destruction
# ───────────────────────────────────────────────
- ticket_id: "VD-1"
  summary: "Vehicle explosion sequence"
  spec: >
    • Broadcast VehicleExplode; 10 s despawn delay

- ticket_id: "VD-2"
  summary: "Door detach & debris"
  spec: >
    • VehiclePartDetach {vehicleId,partId}

- ticket_id: "VD-3"
  summary: "Occupant ejection ragdoll"
  spec: >
    • EjectOccupant{peerId} on decel>12 m/s²

# ───────────────────────────────────────────────
# Extended Phase BH — Breach Mini-game
# ───────────────────────────────────────────────
- ticket_id: "BH-1"
  summary: "Breach puzzle seed sync"
  spec: >
    • BreachStart{peerId,seed,gridW,H} → grid build

- ticket_id: "BH-2"
  summary: "Shared timer & input broadcast"
  spec: >
    • BreachInput{idx}; 45 s server timer

- ticket_id: "BH-3"
  summary: "Daemon effect application"
  spec: >
    • BreachResult{daemonsMask}; apply armour-break, cameras off, vuln

# ───────────────────────────────────────────────
# Extended Phase IE — Interior Teleports
# ───────────────────────────────────────────────
- ticket_id: "IE-1"
  summary: "Elevator call detection"
  spec: >
    • ElevatorCall{peerId,elevatorId,floorIdx}

- ticket_id: "IE-2"
  summary: "Streaming-layer switch"
  spec: >
    • ElevatorArrive{elevatorId,sectorHash,pos}

- ticket_id: "IE-3"
  summary: "Teleport soft-lock safeguard"
  spec: >
    • TeleportAck; retry up to 3

# ───────────────────────────────────────────────
# Extended Phase UI — Net-safe UI Cleanup
# ───────────────────────────────────────────────
- ticket_id: "UI-1"
  summary: "Blocking UI audit"
  spec: >
    • Log blocked menus; build array

- ticket_id: "UI-2"
  summary: "World map replacement"
  spec: >
    • CoopMap overlay no pause

- ticket_id: "UI-3"
  summary: "Shard & holo-call handling"
  spec: >
    • Replace shard reader; HoloCallStart/End packets

# ───────────────────────────────────────────────
# Phase IMT — Interest-Management & Adaptive Tick
# ───────────────────────────────────────────────
- ticket_id: "IMT-1"
  summary: "Spatial grid & subscription"
  spec: >
    • Add 32 m cell grid; `interestSet` per Connection.

- ticket_id: "IMT-2"
  summary: "Interest diff packets"
  spec: >
    • Send `InterestAdd` / `InterestRemove` each tick.

- ticket_id: "IMT-3"
  summary: "Adaptive tick-rate"
  spec: >
    • Lower to 25 Hz if frame > 25 ms; raise when fast.

- ticket_id: "IMT-4"
  summary: "Client interpolation re-tune"
  spec: >
    • `interpDelay = max(2×tick, 80 ms)` on TickRateChange.

# ───────────────────────────────────────────────
# Phase SA — Spectator & Admin Tools
# ───────────────────────────────────────────────
- ticket_id: "SA-1"
  summary: "Spectator freecam"
  spec: >
    • `/spectate <peer>` enters free-fly camera; WASD controls.

- ticket_id: "SA-2"
  summary: "Kick / ban / mute commands"
  spec: >
    • `AdminCmd{Kick,Ban,Mute}`; console + chat.

- ticket_id: "SA-3"
  summary: "Web dashboard"
  spec: >
    • WebSocket JSON peer list at `/status` every 2 s.

# ───────────────────────────────────────────────
# Phase PP — Persistence & Progress Merge
# ───────────────────────────────────────────────
- ticket_id: "PP-1"
  summary: "Session-end JSON snapshot"
  spec: >
    • Save XP, quest map, inventory to `sessionId.json`.

- ticket_id: "PP-2"
  summary: "Merge wizard CLI"
  spec: >
    • `coop_merge.exe` three-way merge of phase file and SP save.

- ticket_id: "PP-3"
  summary: "Conflict UI prompt"
  spec: >
    • `MergePrompt.reds` accept / skip per conflict.

# ───────────────────────────────────────────────
# Phase NT — NAT Traversal / Relay
# ───────────────────────────────────────────────
- ticket_id: "NT-1"
  summary: "STUN discovery"
  spec: >
    • `libjuice` StartNat(); obtain public IP/port.

- ticket_id: "NT-2"
  summary: "ICE handshake"
  spec: >
    • Wrap ENet channel in ICE transport sockets.

- ticket_id: "NT-3"
  summary: "TURN relay fallback"
  spec: >
    • Request relay creds; reconnect via TURN if ICE fails.

# ───────────────────────────────────────────────
# Phase CC — Cut-scene Camera & Lip-Sync
# ───────────────────────────────────────────────
- ticket_id: "CC-1"
  summary: "Cinematic time-code broadcast"
  spec: >
    • `CineStart{sceneId,startMs}`; clients seek camera.

- ticket_id: "CC-2"
  summary: "Viseme track packets"
  spec: >
    • `Viseme{npcId,visemeId,timeMs}` every 250 ms.

- ticket_id: "CC-3"
  summary: "Dialogue choice sync"
  spec: >
    • `DialogChoice{choiceIdx}` validation + broadcast.

# ───────────────────────────────────────────────
# Phase VC — Voice Chat
# ───────────────────────────────────────────────
- ticket_id: "VC-1"
  summary: "Opus encoder integration"
  spec: >
    • 20 ms mono frames; `Voice{seq,opusBytes}`.

- ticket_id: "VC-2"
  summary: "Jitter buffer & decoder"
  spec: >
    • 50-packet window; drop if > 200 ms late.

- ticket_id: "VC-3"
  summary: "Push-to-talk overlay"
  spec: >
    • Mic icon while transmitting; PTT key from settings.

# ───────────────────────────────────────────────
# Phase HJ — Hot-Join Prefetch
# ───────────────────────────────────────────────
- ticket_id: "HJ-1"
  summary: "World-marker blob"
  spec: >
    • zstd-compress markers ≤ 10 kB, send on join.

- ticket_id: "HJ-2"
  summary: "Late-join handshake"
  spec: >
    • Send after `SnapshotAck`; client ApplyMarkerBlob().

# ───────────────────────────────────────────────
# Phase QW — Quest Soft-Lock Watchdog
# ───────────────────────────────────────────────
- ticket_id: "QW-1"
  summary: "Divergence detector"
  spec: >
    • If stage variance > 1 for 15 s → resync request.

- ticket_id: "QW-2"
  summary: "Forced resync"
  spec: >
    • `QuestFullSyncPkt` to lagging peer.

# ───────────────────────────────────────────────
# Phase SC — Save Compression
# ───────────────────────────────────────────────
- ticket_id: "SC-1"
  summary: "zstd compression & backup rotation"
  spec: >
    • Write `.json.zst`; keep last 5.

# ───────────────────────────────────────────────
# Phase HU — Stat / Latency HUD
# ───────────────────────────────────────────────
- ticket_id: "HU-1"
  summary: "Net stat collector"
  spec: >
    • Track ping, packet loss, voice/snap kbps.

- ticket_id: "HU-2"
  summary: "HUD panel (F1)"
  spec: >
    • Ink table overlay colour-codes ping.

# ───────────────────────────────────────────────
# Phase SP — Spectator Cam Polish
# ───────────────────────────────────────────────
- ticket_id: "SP-1"
  summary: "POV cycle keybind"
  spec: >
    • `B` cycles target; mouse-wheel prev/next.

- ticket_id: "SP-2"
  summary: "Free-cam bounds"
  spec: >
    • Clamp 1 km from nearest avatar.

- ticket_id: "SP-3"
  summary: "Camera blend"
  spec: >
    • 0.6 s cubic ease in/out.

# ───────────────────────────────────────────────
# Phase GF — Grief & Friendly-Fire
# ───────────────────────────────────────────────
- ticket_id: "GF-1"
  summary: "friendly_fire cfg"
  spec: >
    • DamageValidator returns 0 if same team & flag off.

- ticket_id: "GF-2"
  summary: "Spawn-kill protection"
  spec: >
    • 5 s invul; shader flicker; clears on first shot.

# ───────────────────────────────────────────────
# Phase DD — Dynamic Difficulty
# ───────────────────────────────────────────────
- ticket_id: "DD-1"
  summary: "Health / DPS scale"
  spec: >
    • +25 % hp per extra player; dmg +15 %.

- ticket_id: "DD-2"
  summary: "AI reinforcements"
  spec: >
    • Extra wave of 2 NPCs every 30 s at alert High.

# ───────────────────────────────────────────────
# Phase PM — Photo-Mode Disable
# ───────────────────────────────────────────────
- ticket_id: "PM-1"
  summary: "Block photo-mode"
  spec: >
    • Replace with `CoopNotice`; no world pause.

# ───────────────────────────────────────────────
# Phase MB — Memory & GPU Guards
# ───────────────────────────────────────────────
- ticket_id: "MB-1"
  summary: "Snapshot heap monitor"
  spec: >
    • Flush baselines if > 2 GiB live.

- ticket_id: "MB-2"
  summary: "Voice buffer purge"
  spec: >
    • Keep ≤ 120 packets; warn if dropped.

- ticket_id: "MB-3"
  summary: "Adaptive texture LOD"
  spec: >
    • MipBias ±1 based on VRAM usage.

# ───────────────────────────────────────────────
# Phase PD — Advanced Police Behaviour
# ───────────────────────────────────────────────
- ticket_id: "PD-1"
  summary: "Dispatch cruiser waves"
  spec: >
    • Wave timer 30/15 s; spawn 4 NPC cruisers.

- ticket_id: "PD-2"
  summary: "Pursuit AI state sync"
  spec: >
    • `NpcState{Idle,Search,Pursuit,Combat}`.

- ticket_id: "PD-3"
  summary: "Scanner crime events"
  spec: >
    • `CrimeEventSpawn{eventId,seed}`.

- ticket_id: "PD-4"
  summary: "Max-Tac AV cinematic"
  spec: >
    • `CineStart` scene `maxtac_av` at 5 stars.

# ───────────────────────────────────────────────
# Phase CW — Cyberware Systems
# ───────────────────────────────────────────────
- ticket_id: "CW-1"
  summary: "EquipCyberware packet"
  spec: >
    • Validate slots; broadcast Equip.

- ticket_id: "CW-2"
  summary: "Slow-mo cyberware"
  spec: >
    • `SlowMoStart{factor,duration}`; scale anim Δt.

- ticket_id: "CW-3"
  summary: "Cyberware cooldown HUD"
  spec: >
    • Radial progress ink widget.

# ───────────────────────────────────────────────
# Phase PK — Perk Tree & Respec
# ───────────────────────────────────────────────
- ticket_id: "PK-1"
  summary: "PerkUnlock sync"
  spec: >
    • Server authoritative unlock & rebroadcast.

- ticket_id: "PK-2"
  summary: "Respec flow"
  spec: >
    • `/respec` refunds; cost €$100 000.

# ───────────────────────────────────────────────
# Phase SE — Status Effect Aura
# ───────────────────────────────────────────────
- ticket_id: "SE-1"
  summary: "DOT / CC sync"
  spec: >
    • `StatusApply` + `StatusTick` every 500 ms.

# ───────────────────────────────────────────────
# Phase TF — Dynamic Traffic
# ───────────────────────────────────────────────
- ticket_id: "TF-1"
  summary: "Ghost traffic seed"
  spec: >
    • `TrafficSeed{sectorHash,seed64}`.

- ticket_id: "TF-2"
  summary: "Traffic despawn consistency"
  spec: >
    • `TrafficDespawn{vehId}` on GC.

# ───────────────────────────────────────────────
# Phase PH — Environmental Destruction
# ───────────────────────────────────────────────
- ticket_id: "PH-1"
  summary: "Prop break pkt"
  spec: >
    • `PropBreak{entityId,seed}` → deterministic shards.

- ticket_id: "PH-2"
  summary: "Barrel chain-reaction"
  spec: >
    • `PropIgnite{entityId,delay}`.

# ───────────────────────────────────────────────
# Phase VO — VO Queue
# ───────────────────────────────────────────────
- ticket_id: "VO-1"
  summary: "Centralised VOPlay queue"
  spec: >
    • `VOPlay{lineId}` triggers Wwise event.

# ───────────────────────────────────────────────
# Phase FX — Fixer Holo-Calls
# ───────────────────────────────────────────────
- ticket_id: "FX-1"
  summary: "Shared fixer calls"
  spec: >
    • `FixerCallStart/End`; no input freeze.

# ───────────────────────────────────────────────
# Phase GJ — Gig Side-Job Spawns
# ───────────────────────────────────────────────
- ticket_id: "GJ-1"
  summary: "Server-only gig volumes"
  spec: >
    • `GigSpawn{questId,seed}` when trigger entered.

# ───────────────────────────────────────────────
# Phase CD — Personal Vehicle Summon
# ───────────────────────────────────────────────
- ticket_id: "CD-1"
  summary: "Vehicle summon ownership lock"
  spec: >
    • Deny duplicate; teleport existing if spawned.

# ───────────────────────────────────────────────
# Phase WD — Wardrobe / Transmog
# ───────────────────────────────────────────────
- ticket_id: "WD-1"
  summary: "AppearanceChange packet"
  spec: >
    • Reload mesh & tint on peers.

# ───────────────────────────────────────────────
# Phase SPG — Scan / Ping Overlay
# ───────────────────────────────────────────────
- ticket_id: "SPG-1"
  summary: "Ping outline sync"
  spec: >
    • `PingOutline{entityIds[],duration}`.

# ───────────────────────────────────────────────
# Phase LT — Loot RNG Control
# ───────────────────────────────────────────────
- ticket_id: "LT-1"
  summary: "Seeded loot roll"
  spec: >
    • `LootRoll{containerId,seed}` before UI opens.

# ───────────────────────────────────────────────
# Phase AD — Auto-Dealer Purchases
# ───────────────────────────────────────────────
- ticket_id: "AD-1"
  summary: "Dealer stock & purchase"
  spec: >
    • `DealerBuy{vehicleTpl,price}`; `VehicleUnlock`.

# ───────────────────────────────────────────────
# Phase WI — Weapon Inspect & Finisher
# ───────────────────────────────────────────────
- ticket_id: "WI-1"
  summary: "Inspect animation sync"
  spec: >
    • `WeaponInspectStart{peerId,animId}`.

- ticket_id: "WI-2"
  summary: "Finisher cinematic"
  spec: >
    • `FinisherStart{actor,victim,type}`.

# ───────────────────────────────────────────────
# Phase AT — HTTP Asset Transfer
# ───────────────────────────────────────────────
- ticket_id: "AT-1" … "AT-6" 
  summary: see asset-transfer pipeline
  
# ───────────────────────────────────────────────
# Phase EM — Emote Wheel
# ───────────────────────────────────────────────
- ticket_id: "EM-1"
  summary: "Emote packet"
  spec: >
    • `Emote{peerId,emoteId}` 12 canned emotes.

- ticket_id: "EM-2"
  summary: "Ink radial menu"
  spec: >
    • Hold C to open wheel; anim on pick.

# ───────────────────────────────────────────────
# Open-World Phasing & Apartments
# ───────────────────────────────────────────────
- ticket_id: "PX-1" … "PX-12" 
  summary: per-phase quest instances, snapshot filter
- ticket_id: "AP-1" … "AP-5" 
  summary: buyable apartments & permissions
- ticket_id: "CP-1" 
  summary: persist parked cars
- ticket_id: "VC-1" 
  summary: 50 ms vehicle collision rewind
- ticket_id: "TR-1" 
  summary: /unstuckcar & tow timer
- ticket_id: "VS-1" 
  summary: phase-aware vehicle spawns

# ───────────────────────────────────────────────
# Python Plugin System
# ───────────────────────────────────────────────
- ticket_id: "PY-1" … "PY-12" 
  summary: CPython embed, event bus, asset push
- ticket_id: "PCS-1" … "PCS-3" 
  summary: client proxy & hot-reload

# ───────────────────────────────────────────────
# Audits
# ───────────────────────────────────────────────
- ticket_id: "CLEANUP-FINAL"
  summary: "Resolve every stub"
  spec: >
    • Build final_todo.md → patch → zero TODOs.

- ticket_id: "AUDIT-EXPAND"
  summary: "Deep audit append"
  spec: >
    • Line-by-line scan; append unchecked items only.

# ───────────────────────────────────────────────
# Performance Autoscale (SA-autoscale)
# ───────────────────────────────────────────────
- ticket_id: "SA-1"
  summary: "Task graph & worker pool"
  spec: >
    • `TaskGraph` jthread pool; offload AI, diff, physics.

- ticket_id: "SA-2"
  summary: "Dynamic worker autoscale"
  spec: >
    • Add/remove worker based on avgFrameMs.

# ================= END Part 3 =================


# ============ End Phase Outline ============
```