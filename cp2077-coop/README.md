# cp2077-coop

This directory contains the early skeleton for the cooperative multiplayer mod.
Core systems already include a connection state machine, snapshot structures,
and various gameplay sync stubs:
* Lag compensation via `RewindPosition()`
* Snapshot interpolation with rollback history
* Quickhack, heat, and weather replication helpers
* Basic damage validation on the server
* Vehicle damage sync placeholder
* Deathmatch mode manager and respawn/scoreboard stubs
* Thread-safe queue utility for cross-thread tasks
* Crash capture packs dump and network log into a zip and prompts to send
* Build version check with CRC packet
* Master-server heartbeat JSON stub
* User settings panel storing tick rate and keys
* These hardening features were introduced during Phase 8

## Hardening & Polish
The current phase focuses on stability.
New modules improve crash reporting, ensure consistent build versions,
and prepare dedicated server listings via periodic heartbeats.

### Dedicated Server
Run `coop_dedicated` to host a match. Supported arguments:

* `--help` — print a short usage banner then exit.
* `--port <number>` — override the listen port (default `7777`).

### Packet Handling
All enums defined in `src/net/Packets.hpp` now have matching cases in `Connection.cpp`.
This ensures new packet types are automatically rejected if misused.
