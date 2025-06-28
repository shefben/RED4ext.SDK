# Phase 19

```yaml
###############################################################################
# ███  PHASE SX — SKILL-XP & RELIC TREE  (SX-1 … SX-2)                       █
###############################################################################

- ticket_id: "SX-1"
  summary: "Skill-XP sync & server calc"
  context_files: []
  spec: >
    • Add `EMsg::SkillXP {peerId, skillId, deltaXP}` (Athletics, Stealth, Hacking … 12 ids).
    • Server holds `skillTable[peerId][skillId]`; on packet: clamp delta (±500),
      update table, rebroadcast to all peers.
    • XP gain is now added ONLY on the server: hook `StatsSystem.AddSkillXP`
      detour to send request → wait ack.
  hints:
    - skillId enumeration in `SkillIds.hpp`.

- ticket_id: "SX-2"
  summary: "Relic perks authority"
  context_files: []
  spec: >
    • Extend PerkSync with `treeId==Relic` range (1000–1015).
    • Relic perk “Data Tunneling” adds +10 % quickhack dmg—update
      DamageValidator to use relicTable[peerId].
  hints:
    - UI reuses existing perk HUD.



###############################################################################
# ███  PHASE WM — WEAPON BENCH & MOD RE-ROLL  (WM-1)                         █
###############################################################################

- ticket_id: "WM-1"
  summary: "Server-authoritative weapon re-roll"
  context_files: []
  spec: >
    • On `WorkTable::ReRollMods()` client sends `ReRollRequest {itemId,seed}`.
    • Server validates mats, rolls new mod stats via seed, returns
      `ReRollResult {itemSnap}`; client applies.
    • ItemSnap delta sent to all peers in phase/0.
  hints:
    - seed = FNV(itemId, serverTick).



###############################################################################
# ███  PHASE RP — RIPPERDOC CINEMATIC & INSTALL  (RP-1)                      █
###############################################################################

- ticket_id: "RP-1"
  summary: "Local-phase ripperdoc chair cutscene"
  context_files: []
  spec: >
    • Hook `RipperdocSystem.BeginInstall()`; server sends
      `CineStart {sceneId='ripper_chair',phaseId=peerId,solo=true}`.
    • Client plays cutscene locally; other peers unaffected.
    • Upon close, server fires `CyberEquip` packet.
  hints:
    - Use CineStart solo flag from PX-4.



###############################################################################
# ███  PHASE MG — MINI-GAMES  (MG-1 … MG-3)                                  █
###############################################################################

- ticket_id: "MG-1"
  summary: "Breach tile mini-puzzle sync"
  context_files: []
  spec: >
    • When player starts shard hack, server sends `TileGameStart {phaseId,seed}`.
    • Clients build 5×5 grid using seed RNG.
    • `TileSelect {row,col}` packets broadcast only to phase peers.
  hints:
    - Re-use BH grid code if exists.

- ticket_id: "MG-2"
  summary: "Shard decrypt timer"
  context_files: []
  spec: >
    • Decrypt progress bar ticks server-side; broadcasts
      `ShardProgress {percent}` every 0.5 s.
  hints:
    - Cancel on attack/combat.

- ticket_id: "MG-3"
  summary: "Braindance editor spectator safe-mode"
  context_files: []
  spec: >
    • Block world pause: BDSystem.EnterEditor → if multiplayer,
      freeze **local** timeScale only; fade rest of HUD for that player.
  hints:
    - Use PX solo cinematic logic.



###############################################################################
# ███  PHASE TRD — PLAYER TRADING  (TRD-1)                                   █
###############################################################################

- ticket_id: "TRD-1"
  summary: "Secure 1-to-1 trade window"
  context_files: []
  spec: >
    • New GUI `TradeWindow.reds` opened via `/trade <peer>`.
    • Packet flow: `TradeInit`, `TradeOffer {items[],eddies}`,
      `TradeAccept`, `TradeFinalize`.
    • Server validates ownership & space, then moves ItemSnap between
      phase inventories; rebroadcast inventory deltas.
  hints:
    - Cancel on combat, distance >5 m.



###############################################################################
# ███  PHASE RM — ROMANCE SCENES  (RM-1)                                     █
###############################################################################

- ticket_id: "RM-1"
  summary: "Private romance flagging"
  context_files: []
  spec: >
    • Romance quest nodes tagged `"privateScene":true` in JSON table.
    • QuestSync sends CineStart solo for ownerPhase; others receive
      faded-glass overlay “<name> busy”.
  hints:
    - Block seat/bed interaction for spectators.



###############################################################################
# ███  PHASE EG — ENDING SYNC  (EG-1)                                         █
###############################################################################

- ticket_id: "EG-1"
  summary: "Credit roll convergence"
  context_files: []
  spec: >
    • When any phase reaches `quest="main_ending", stage=FINAL`,
      begin `EndingVoteStart` (similar to PX critical vote).
    • Majority “Yes” teleports all players to rooftop, sets global
      `phaseId=0` for end cutscene, locks input.
  hints:
    - Auto-save before teleport.



###############################################################################
# ███  PHASE TK — UNIVERSAL FINISHER KILLS  (TK-1)                           █
###############################################################################

- ticket_id: "TK-1"
  summary: "Non-katana finisher sync"
  context_files: []
  spec: >
    • Hook `FinisherSystem.StartFinisher()` for weapon class != katana.
    • Broadcast `FinisherStart {actor,victim,finisherType}`.
    • Clients play same montage; lock victim ragdoll.
  hints:
    - finisherType enum pistol=1,knife=2,revolver=3.



###############################################################################
# ███  PHASE VT — VEHICLE POLISH  (VT-1 … VT-4)                               █
###############################################################################

- ticket_id: "VT-1"
  summary: "Motorbike lean replication"
  context_files: []
  spec: >
    • Add `leanAngle` float to VehicleSnap when `class==bike`.
    • Serialize every 100 ms; interpolate client-side.
  hints:
    - clamp ±45°.

- ticket_id: "VT-2"
  summary: "Mounted turret aim sync"
  context_files: []
  spec: >
    • For vehicles with turret seat, send `TurretAim {vehId,yaw,pitch}` 30 Hz.
    • Client blends into bone transform.
  hints:
    - Use VehicleProxy.HasTurret flag.

- ticket_id: "VT-3"
  summary: "Air police AV path"
  context_files: []
  spec: >
    • Spawn `AirVehicleProxy` with Waypoint array; broadcast transforms 10 Hz.
    • Reuse existing deterministic physics but Z axis only.
  hints:
    - Turret packets from VT-2 apply.

- ticket_id: "VT-4"
  summary: "Paint & plate personalization"
  context_files: []
  spec: >
    • `VehiclePaintChange {vehId,colorId,plateId}`.
    • Client applies material parameter, updates plate text.
  hints:
    - PlateId UTF-8 up to 8 chars.



###############################################################################
# ███  PHASE AI — ADVANCED AI BEHAVIOR  (AI-1 … AI-3)                         █
###############################################################################

- ticket_id: "AI-1"
  summary: "Civilian panic propagation"
  context_files: []
  spec: >
    • On gunfire, server seeds `PanicEvent {pos,seed}`; clients run
      deterministic panic burst for nearest 30 crowd NPCs.
  hints:
    - Use seed for flee vector.

- ticket_id: "AI-2"
  summary: "Enemy netrunner quickhacks"
  context_files: []
  spec: >
    • When AI runner casts Overheat, server sends `AIHack {target,effectId}`.
    • StatusEffect system handles ticks.
  hints:
    - reuse SE-1.

- ticket_id: "AI-3"
  summary: "Boss phase switch sync"
  context_files: []
  spec: >
    • For bosses flagged `multiPhase`, server sends `BossPhase {npcId,phaseIdx}`.
    • Clients update mesh, AI behaviour tree.
  hints:
    - Oda cloak, Smasher jetpack.



###############################################################################
# ███  PHASE EU — ECONOMY & UI  (EU-1 … EU-3)                                 █
###############################################################################

- ticket_id: "EU-1"
  summary: "Vendor daily stock refresh timer"
  context_files: []
  spec: >
    • At 04:00 in-game time, server resets vendor.stock[phaseId].
    • Broadcast `VendorRefresh {vendorId}` to phase.
  hints:
    - time from TimeSystem.

- ticket_id: "EU-2"
  summary: "Stash delta sync after offline play"
  context_files: []
  spec: >
    • On session join, client sends `StashHash {crc32}`.
    • If mismatch, server transmits `StashDelta` array.
  hints:
    - Only for ownerPhase.

- ticket_id: "EU-3"
  summary: "Fixer Gig Board phase filter"
  context_files: []
  spec: >
    • Page shows only gigs where `quest.phaseId==localPhase`.
    • Completed gigs greyed; clicking opens assist menu if another phase.
  hints:
    - AJAX JS change in WebView.



###############################################################################
# ███  PHASE NS — NETWORK SECURITY & RESILIENCE  (NS-1 … NS-3)               █
###############################################################################

- ticket_id: "NS-1"
  summary: "AES-GCM packet encryption"
  context_files: []
  spec: >
    • Pre-shared 256-bit key established on connect via Diffie-Hellman
      in join handshake.
    • Encrypt entire payload after PacketHeader.
  hints:
    - Use libsodium (crypto_secretbox_easy).

- ticket_id: "NS-2"
  summary: "Replay-attack nonce & window"
  context_files: []
  spec: >
    • Each encrypted packet prepends `uint32 nonce`.
    • Connection stores sliding window (last 1024 nonces); drop dupes.
  hints:
    - nonce increments per sender.

- ticket_id: "NS-3"
  summary: "Rate-limit & flood guard"
  context_files: []
  spec: >
    • Per peer token bucket: 30 packets burst, 20 pkt/s refill.
    • Excess packets dropped, log WARN.
  hints:
    - Don’t count voice.



###############################################################################
# ███  PHASE CL — CLOUD BACKUP & JOURNALING  (CL-1 … CL-2)                   █
###############################################################################

- ticket_id: "CL-1"
  summary: "Steam/GoG cloud sync delta"
  context_files: []
  spec: >
    • After SaveFork writes zst, compute md5; if changed, upload via
      platform cloud API path `coop/<session>/<phaseId>.zst`.
  hints:
    - SteamRemoteStorage/FileShare.

- ticket_id: "CL-2"
  summary: "Change-log journal"
  context_files: []
  spec: >
    • Append JSON line `{tick,peerId,action,entityId,delta}` to
      `journal.log` for every state-changing packet (QuestStage, Purchase).
    • Zip with zstd every 10 MB.
  hints:
    - Path “logs/journal/”.



###############################################################################
# ███  PHASE VOX — VOICE MODERATION  (VOX-1)                                 █
###############################################################################

- ticket_id: "VOX-1"
  summary: "Admin mute/unmute voice"
  context_files:
    - path: src/server/AdminController.cpp
      excerpt: |
        // existing Kick/Ban
  spec: >
    • Add commands `mute <peerId> [mins]` and `unmute <peerId>`.
    • Connection.voiceMuted flag checked before sending Voice packets.
    • StatHUD (HU-2) shows mic icon greyed when muted.
  hints:
    - 0 mins = permanent until restart.



###############################################################################
# ███  PHASE PRF — PERFORMANCE GUARDS  (PRF-1 … PRF-2)                       █
###############################################################################

- ticket_id: "PRF-1"
  summary: "Dynamic sector LOD drop"
  context_files: []
  spec: >
    • If ServerMem >80 % OR VRAM >95 %, send `SectorLOD {sectorHash, lod=LOW}`.
    • Clients load LOD 1 assets; restore when mem <70 %.
  hints:
    - Re-use MB-3 GPU bias.

- ticket_id: "PRF-2"
  summary: "Degraded snapshot mode for low-bw peers"
  context_files: []
  spec: >
    • If Connection.pingMs >250 OR packet loss >15 %, server switches peer
      to `snapRate=2 Hz`, only AvatarTransform + essential NPCs.
    • Packet `LowBWMode {enable}` notifies peer to stretch interp buffer.
  hints:
    - Review every 30 s.



###############################################################################
# ███  PHASE EM — EMOTE WHEEL  (EM-1 … EM-2)                                 █
###############################################################################

- ticket_id: "EM-1"
  summary: "Emote packet & assets"
  context_files: []
  spec: >
    • `EMsg::Emote {peerId, emoteId}` (12 canned emotes).
    • Add `/assets/animations/emotes/*.anim`.
  hints:
    - emoteId enum: wave=1, thumbsUp=2…

- ticket_id: "EM-2"
  summary: "Ink radial menu & keybind"
  context_files: []
  spec: >
    • Hold `C` opens radial wheel; RightStick (controller) alike.
    • Selecting slot sends Emote pkt; AvatarProxy plays anim + VO line.
    • Wheel disabled if weapon drawn and in combat.
  hints:
    - Use inkRadial widget from Witcher3 assets prototype.
```