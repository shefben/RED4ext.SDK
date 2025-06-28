# 🖥️ Phase 3 — UI & Lobby Flow

```yaml
ticket_id: "P3-1"
summary: "Main-menu CO-OP button"
context_files: []
spec: >
  Inject a new button into the game’s title menu:
    • Add `src/gui/MainMenuInjection.reds`.
    • On load, locate existing main menu controller via RTTI and
      insert an inkButton labelled "CO-OP".
    • Button callback should open a placeholder panel
      `CoopRootPanel.Show()`.
hints:
  - Panel implementation arrives in next ticket; just open/log for now.
```
```yaml
ticket_id: "P3-2"
summary: "Server-browser panel"
context_files:
  - path: src/gui/MainMenuInjection.reds
    excerpt: |
      CoopRootPanel.Show()
spec: >
  Build a scrollable server list UI:
    • Create `src/gui/ServerBrowser.reds` implementing
        func Refresh(jsonList: String);
        func OnRowClicked(serverId: Uint32);
    • Use placeholder JSON fixture at `tests/static/serverlist.json`
      (add the file with three sample servers).
    • Populate rows with server name, player count "2/4".
    • OnRowClicked() logs "Selected {serverId}" for now.
hints:
  - Widget layout can be pseudo-code comments.
```
```yaml
ticket_id: "P3-3"
summary: "Join & Host buttons"
context_files:
  - path: src/gui/ServerBrowser.reds
    excerpt: |
      func OnRowClicked(serverId: Uint32)
spec: >
  Add connection flow:
    • In `ServerBrowser.reds`, implement "Join" button that sends
      JoinRequest to the selected serverId via NetCore (stub call).
    • Implement "Host" button: spawns a local `coop_dedicated`
      subprocess (document cmd line) then auto-joins 127.0.0.1.
    • Update log comments showing expected sequence.
hints:
  - Use pseudo code for subprocess spawn; actual OS call later.
```
```yaml
ticket_id: "P3-4"
summary: "In-game chat overlay"
context_files: []
spec: >
  Provide a basic chat system:
    • Append `Chat` to EMsg list if not already present.
    • Add `src/gui/ChatOverlay.reds`:
        class ChatOverlay extends inkHUDLayer {
          var lines : array<String>;
          func PushLine(txt: String);
          func Toggle();
        }
      Toggle on `Enter` key; keep max 50 lines in buffer.
    • Connection logic: when `ChatMsg` arrives, call PushLine().
hints:
  - Rendering can be stubbed; focus on data handling & toggle flag.
```