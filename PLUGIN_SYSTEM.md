# Plugin System

Third-party plugins live under the `plugins/` directory. Each plugin is a Python
module with a matching `manifest.json` describing permissions.

## Loading
- `PluginManager` scans `plugins/*.py` on startup and every 60 seconds.
- Modules are imported through the isolated Python VM defined in `PythonVM`.
- A unique numeric ID is assigned to every plugin and bundled assets are pushed
to clients.

## Sandboxing
- The Python interpreter is started with `isolated=1` and `site_import=0`.
- Dangerous built-ins such as `open`, `socket` and `subprocess` are removed.
- Scripts therefore cannot perform file I/O or spawn external processes.

## API versioning
- Plugins should import `plugins.api_v1` to access the stable API.
- `register_packet(name, callback)` reserves a custom packet ID and associates a
  handler.
- `register_panel(name, callback)` registers a factory used by in-game UI code.
- `get_version()` returns the API version number.

## Example
```python
from plugins import api_v1 as api

PACKET_ID = api.register_packet("MyPacket", lambda peer, data: print(data))

@api.register_panel("TestPanel")
def create_panel():
    game.show_popup(0, "Hello", 3)
```
