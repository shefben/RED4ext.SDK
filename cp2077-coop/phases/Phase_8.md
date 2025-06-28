# 🧹 Phase 8 — Hardening & Polish

```yaml
ticket_id: "P8-1"
summary: "Thread-safety pass"
context_files: []
spec: >
  Protect shared data:
    • Add `src/core/ThreadSafeQueue.hpp`:
        template<typename T> class ThreadSafeQueue { ... };
      Use mutex + std::queue; push/pop methods; comment usage.
    • Replace any existing comment TODOs about thread safety with
      `// FIXME(ThreadSafe pass P8-1 completed)`.
hints:
  - No code replacement yet—just queue class.
```
```yaml
ticket_id: "P8-2"
summary: "Crash telemetry"
context_files: []
spec: >
  Collect diagnostics:
    • Add `src/core/CrashHandler.cpp`:
        void CaptureCrash(const char* reason);
      Logs reason and copies last 1 MB of net log path
      `"crash/netlog_last.txt"` (comment placeholder).
hints:
  - No real file I/O; just string handling & comments.
```
```yaml
ticket_id: "P8-3"
summary: "Version hash check"
context_files: []
spec: >
  Reject mismatched builds:
    • `src/net/VersionCheck.reds`:
        const kClientCRC : Uint32 = 0xDEADBEEF; // placeholder
        func SendVersion();
        func VerifyVersion(received: Uint32);
      On mismatch, log "Version mismatch – disconnecting".
    • Add `Version` to EMsg with CRC payload.
hints:
  - CRC value will be generated at build time later.
```
```yaml
ticket_id: "P8-4"
summary: "Master-server heartbeat"
context_files: []
spec: >
  Keep server list fresh:
    • Create `src/server/Heartbeat.cpp`:
        void SendHeartbeat(sessionId: Uint64);
      Logs JSON `{"id":sessionId,"ts":...}` every 30 s (comment timer).
    • Add comment about removing entry after 90 s inactive.
hints:
  - No HTTP code; just JSON string assembly.
```
```yaml
ticket_id: "P8-5"
summary: "Settings panel"
context_files: []
spec: >
  User-configurable options:
    • `src/gui/CoopSettings.reds`:
        var tickRate   : Uint16 = 30;
        var interpMs   : Uint16 = 100;
        var pushToTalk : EKey  = EKey.T;
        func Show();
        func Save(path: String);
      Show() opens panel (placeholder) and Save() logs INI write path.
    • Default INI path `"coop.ini"` under mod root.
hints:
  - No actual disk write; focus on fields & log output.
```