# Outstanding TODO/Placeholder items

|   | File | Line | Summary | Origin |
|---|------|-----:|---------|--------|
| [x] | cp2077-coop/src/gui/CoopMap.reds | 36 | Placeholder pan/zoom controls without pausing the game | comment |
| [x] | cp2077-coop/src/gui/Killfeed.reds | 3 | Placeholder UI for killfeed messages | comment |
| [x] | cp2077-coop/src/gui/ServerBrowser.reds | 50 | Placeholder networking call | comment |
| [x] | cp2077-coop/src/net/NatClient.cpp | 114 | Placeholder bandwidth accounting | comment |
| [x] | cp2077-coop/src/net/SnapshotWriter.cpp | 15 | Placeholder container of active entity snapshots | comment |
| [x] | cp2077-coop/src/runtime/AirVehicleProxy.reds | 28 | FIXME find proxy, update state | comment |
| [x] | cp2077-coop/src/runtime/BossPhaseSync.reds | 4 | FIXME change mesh and behavior | comment |
| [x] | cp2077-coop/src/runtime/CrowdCfgSync.reds | 7 | Placeholder for ini read | comment |
| [x] | cp2077-coop/src/runtime/NpcProxy.reds | 23 | Placeholder mesh spawn | comment |
| [x] | cp2077-coop/src/runtime/OutlineHelper.reds | 3 | Placeholder: hook engine outline later | comment |
| [x] | cp2077-coop/src/runtime/PanicSync.reds | 9 | FIXME spawn flee behavior | comment |
| [x] | cp2077-coop/src/server/ArcadeController.cpp | 27 | placeholder scoring | comment |
| [x] | cp2077-coop/src/server/InventoryController.cpp | 17 | Placeholder validation | comment |
| [x] | cp2077-coop/src/server/TextureGuard.cpp | 13 | Placeholder external functions | comment |
| [x] | cp2077-coop/src/server/VehicleController.cpp | 142 | placeholder returns input | comment |

## Fix plan
Implemented controls, UI hooks, join request, relay stats, snapshot list, air vehicle updates, boss notice, crowd density save, mesh spawning, outline ping, panic popup, bitcount scoring, recipe check range, cleaned texture guard, and nav road search.

## Deep Audit Additions
- [ ] cp2077-coop/src/net/Packets.hpp:146 — AIHack enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:142 — AirVehSpawn enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:143 — AirVehUpdate enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:179 — ArcadeStart enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:147 — BossPhase enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:169 — CamHijack enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:171 — CarryBegin enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:117 — CriticalVoteStart enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:150 — CrowdCfg enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:152 — CrowdChatterStart enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:156 — DoorBreachStart enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:151 — Emote enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:138 — EndingVoteStart enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:174 — GrenadePrime enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:160 — HTableOpen enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:35 — HitConfirm enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:34 — HitRequest enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:154 — HoloSeed enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:59 — InterestAdd enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:60 — InterestRemove enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:163 — ItemGrab enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:23 — JoinDeny enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:21 — JoinRequest enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:149 — LowBWMode enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:167 — MetroArrive enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:166 — MetroBoard enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:145 — PanicEvent enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:162 — QuestGadgetFire enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:29 — QuestStage enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:41 — Quickhack enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:168 — RadioChange enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:128 — ReRollRequest enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:148 — SectorLOD enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:18 — Seed enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:25 — SeedAck enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:133 — ShardProgress enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:96 — SkillXP enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:178 — SlowMoFinisher enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:176 — SmartCamStart enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:131 — TileGameStart enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:134 — TradeInit enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:141 — TurretAim enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:144 — VehiclePaintChange enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:140 — VehicleSnapshot enum unused in switch (scan)
- [ ] cp2077-coop/src/net/Packets.hpp:26 — Version enum unused in switch (scan)
- [ ] cp2077-coop/src/server/ArcadeController.cpp:53 — empty Arcade_Tick stub (scan)
- [ ] cp2077-coop/src/plugin/PythonVM.cpp:100 — teleport_peer stub prints only (scan)

- [ ] include/RED4ext/Common.hpp:23 — offset assert TODO  (scan)
- [ ] cp2077-coop/third_party/enet/include/enet/enet.h:4 — placeholder ENet header missing real library (scan)
- [ ] cp2077-coop/tools/coop_merge.cpp:159 — singleplayer JSON merge stub (scan)
- [ ] cp2077-coop/src/gui/DMScoreboard.reds:1 — scoreboard UI unimplemented (scan)
- [ ] cp2077-coop/assets/animations/emotes/1.anim:1 — placeholder animation asset (scan)
- [ ] cp2077-coop/ui/ico_hex_cell.inkwidget:3 — hex_placeholder texture (scan)
- [ ] cp2077-coop/README.md:10 — README lists unimplemented systems (scan)
