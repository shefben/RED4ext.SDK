# Phase 21 — Python VM

```yaml
###############################################################################
# ███  PHASE PY — PYTHON PLUGIN FRAMEWORK (PY-1 … PY-12)                      █
# Goal: Dedicated server can hot-load .py “plugins”; their effects propagate  #
# to every client with **zero client-side install**. Expose high-level API    #
# (events, world edit, UI, entities).                                         #
###############################################################################

- ticket_id: "PY-1"
  summary: "Embed CPython 3.11 into dedicated"
  context_files: []
  spec: >
    • Add `third_party/cpython-3.11` as submodule; compile static.
    • `src/plugin/PythonVM.cpp`:
        bool PyVM_Init();
        bool PyVM_Shutdown();
        bool PyVM_RunString(const char*);
    • Init VM on server start; shutdown on exit.
  hints:
    - Build with `-DPy_ENABLE_SHARED=0`.

- ticket_id: "PY-2"
  summary: "Plugin discovery & hot-load"
  context_files: []
  spec: >
    • `PluginManager.cpp` scans `plugins/*.py` on launch + every 60 s.
    • `PluginMetadata = {name:str, version:str, hash:str}` via top-level
      `__plugin__` dict in each script.
    • Hot-reload if file hash changes; keep one global interpreter.
  hints:
    - Use `importlib.reload`.

- ticket_id: "PY-3"
  summary: "Sandbox / security layer"
  context_files: []
  spec: >
    • Expose curated built-ins: no `open`, `socket`, `subprocess`.
    • Provide safe `game` module via `PyImport_AddModule("game")`.
    • Each plugin executed in its own `types.ModuleType` namespace.
  hints:
    - Use `PyEval_SetRestricted()` pattern.

- ticket_id: "PY-4"
  summary: "Event bus binding"
  context_files: []
  spec: >
    • C++ `EventBus` already exists: OnTick, OnNpcSpawn, OnChatMsg, etc.
    • `game.on(event_name)(callback)` decorator registers CB.
    • Dispatcher marshals C++ structs ⇄ PyDict.
  hints:
    - Store listeners in `std::unordered_map<std::string,PyObject*>`.

- ticket_id: "PY-5"
  summary: "Command registration"
  context_files: []
  spec: >
    • `game.register_command(name:str, help:str)(fn(peerId,args))`.
    • Commands appear in `/help`; executed server-side when any peer
      runs `/name args…` in chat.
  hints:
    - Use Admin privilege flag in connection.

- ticket_id: "PY-6"
  summary: "World edit API stubs"
  context_files: []
  spec: >
    • Provide in `game`:
        spawn_npc(tpl:str,pos:tuple,phase:int)->int
        teleport_peer(peerId,pos,rot)
        set_weather(id:int)
        show_popup(peerId,text:str,dur:float)
    • Underlying implementations call existing C++ helpers & packets.
  hints:
    - Return entityId for later manipulation.

- ticket_id: "PY-7"
  summary: "Reliable Plugin→Client RPC channel"
  context_files: []
  spec: >
    • New packet `PluginRPC {pluginId:uint16, fnHash:uint32, json}`.
    • `game.send_rpc(peerId, fn:str, payload:dict)` from python.
    • Client loads tiny builtin `ClientPluginProxy.reds` that receives
      PluginRPC and executes matching REDscript stub in `plugins/client`.
  hints:
    - For security: whitelist fnHash table embedded in server packet.

- ticket_id: "PY-8"
  summary: "Asset push for zero-install"
  context_files: []
  spec: >
    • `plugins/<name>/assets/` can contain `.ent`, `.inkwidget`, `.anim`.
    • On plugin load, server zstd-packs assets ≤5 MB & streams via
      `AssetBundle {pluginId,len,chunkId}` packets.
    • Client writes to `runtime_cache/plugins/<id>/` and loads at runtime.
  hints:
    - Chunk size 32 KB; SHA-256 verify after complete.

- ticket_id: "PY-9"
  summary: "Example plugin #1 – Popup greet zone"
  context_files: []
  spec: >
    • Create `plugins/popup_zone.py`:
        __plugin__ = {"name":"PopupZone","version":"1.0"}
        @game.on("OnTick")
        def _(_:float):
            for peer,pos in game.get_peer_positions():
                if game.dist(pos,(123,456,0))<2:
                    game.show_popup(peer,"Welcome to HexBar!",5)
    • Add to repo; ensure it hot-loads and broadcasts popup.
  hints:
    - Reference world coord sample.

- ticket_id: "PY-10"
  summary: "Example plugin #2 – Mobile command vehicle"
  context_files: []
  spec: >
    • `plugins/mobile_cc.py` spawns custom `veh_tpl="veh_commandcenter"` via
      `spawn_vehicle` when admin types `/spawncc`.
    • Listeners teleport interior when peer uses door.
    • Asset bundle includes interior `.streamingsector` in assets/.
  hints:
    - Use phaseId=0 (global) by default.

- ticket_id: "PY-11"
  summary: "Plugin error isolation"
  context_files: []
  spec: >
    • Wrap every callback in try/except; log traceback to
      `logs/plugins/<name>.log`; plugin marked disabled after 5 fatal errors.
    • Send admin chat “[Plugin XYZ disabled – error]”.
  hints:
    - Maintain `plugin.state` property.

- ticket_id: "PY-12"
  summary: "Documentation generator"
  context_files: []
  spec: >
    • `tools/gen_plugin_docs.py` introspects game module (dir()) +
      YAML meta of each plugin → outputs `DOCS.md` with API and examples.
  hints:
    - Run script in CI after build.

###############################################################################
# ███  CLIENT-SIDE SUPPORT (PCS-1 … PCS-3)                                    #
###############################################################################

- ticket_id: "PCS-1"
  summary: "Client PluginProxy REDscript"
  context_files: []
  spec: >
    • `plugins/ClientPluginProxy.reds` maintains dictionary
      `callbacks : Dict<UInt32, func(json:String)>`.
    • Provides `register_rpc(fnHash,cb)` used by asset-pushed client
      scripts to attach UI or world edits.
  hints:
    - auto-register default popup handler.

- ticket_id: "PCS-2"
  summary: "Auto-load pushed client REDscripts"
  context_files: []
  spec: >
    • On receipt of AssetBundle complete, call
      `ModSystem.ReloadScriptsFrom("runtime_cache/plugins/<id>/")`.
  hints:
    - Avoid infinite reload loops by hash check.

- ticket_id: "PCS-3"
  summary: "Sandbox client scripts"
  context_files: []
  spec: >
    • Client-side scripts can *only* call `register_rpc` and basic ink UI.
      No persistence or file IO; enforced by ModSystem mount flags.
  hints:
    - mount as read-only.

###############################################################################
# ★ Deployment Flow
# 1. PY-1 → PY-4  (VM & manager)
# 2. PY-5 → PY-8  (API + RPC + asset push)
# 3. PCS-1 → PCS-3 (client plumbing)
# 4. PY-9 / PY-10 examples
# 5. PY-11 → PY-12 polish & docs
###############################################################################
```