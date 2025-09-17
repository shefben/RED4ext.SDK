# Phase 0 - Initial Skeleton

```yaml
ticket_id: ""
summary: ""
context_files: []
spec: >
  :
    • 
hints:
  - 
```
```yaml
ticket_id: "P0-2"
summary: "Hot-reload harness"
context_files:
  - path: src/Main.reds
    excerpt: |
      // cp2077-coop entry point (currently logs on load)
spec: >
  Add a hot-reload harness:
    • Implement an OnReload() function in `src/Main.reds` that logs 
      "cp2077-coop reloaded" each time the mod is live-reloaded (RED4ext ReloadAllMods).
    • Document in a header comment how RED4ext triggers the reload hook.
hints:
  - Use REDscript's `OnReload()` pattern (no extra files required).
  - Keep all code inside `src/Main.reds`.
```
```yaml
ticket_id: "P0-3"
summary: "Networking stub library"
context_files: []
spec: >
  Vendor a lightweight networking stub:
    • Add directory `src/net/`.
    • Within it create `Net.hpp` and `Net.cpp` that expose 
        Net_Init()  -> void  
        Net_Shutdown() -> void
      Each function should simply log its call for now.
    • Include ENet headers in `third_party/enet/include/` 
      and note the link step in CMake (no full build, just paths and comments).
    • Update `CMakeLists.txt` to compile `Net.cpp` as part of the mod binary.
hints:
  - Do not implement real networking yet; stubs only.
  - Absolute paths are not required—use `${PROJECT_SOURCE_DIR}` in CMake placeholders.
```
```yaml
ticket_id: "P0-4"
summary: "Headless server scaffold"
context_files: []
spec: >
  Create a headless dedicated-server scaffold:
    • Add `src/server/` and a file `DedicatedMain.cpp` containing a minimal
      `int main(int argc, char** argv)` that:
        1) calls Net_Init();
        2) prints "Dedicated up";
        3) exits 0 if `--help` is passed.
    • Introduce a new CMake target named `coop_dedicated` that builds the
      executable from `DedicatedMain.cpp` and links against the same Net stub.
    • Update project README header comment in `CMakeLists.txt` to mention the new target.
hints:
  - Keep server code platform-neutral (no Windows-specific APIs).
  - The executable should compile cleanly when tool-chain integration is added later.
```
