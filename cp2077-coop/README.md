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
* Crash capture helper referencing last network log chunk
* Build version check with CRC packet
* Master-server heartbeat JSON stub
* User settings panel storing tick rate and keys
* These hardening features were introduced during Phase 8

## Hardening & Polish
The current phase focuses on stability.
New modules improve crash reporting, ensure consistent build versions,
and prepare dedicated server listings via periodic heartbeats.
