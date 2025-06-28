# Phase 22 — Asset Transfer

```yaml
###############################################################################
# ███  PHASE AT — ASSET-TRANSFER PIPELINE (AT-1 … AT-6)                      █
# Goal: Push plugin assets to clients **asynchronously** over an HTTP side   #
# channel—never clogging the real-time ENet socket. Works for textures,      #
# models, REDscripts, audio, shaders.                                        #
###############################################################################

- ticket_id: "AT-1"
  summary: "Manifest & bundle layout"
  context_files: []
  spec: >
    • For every plugin folder, generate `assets.manifest.json` at build:
        {
          "pluginId" : 7,
          "version"  : "1.0.3",
          "files" : [
            {"path":"models/veh_cc.ent",     "size":18324, "sha256":"…"},
            {"path":"textures/cc_dif.dds",   "size":982k,  "sha256":"…"},
            …
          ]
        }
    • Manifest placed at `plugins/<name>/manifest` alongside zipped
      `assets.<sha256>.zip` (zstd level 6).
  hints:
    - Use deterministic zip order for reproducibility.

- ticket_id: "AT-2"
  summary: "Lightweight HTTP file server (side channel)"
  context_files: []
  spec: >
    • On dedicated startup, spin `AsyncHTTPServer` (port 37015).
    • Serves `GET /plugins/<id>/manifest` and
      `/plugins/<id>/<zipName>` with `ETag` header = sha256.
    • Runs on separate worker thread; uses sendfile/zero-copy if OS supports.
  hints:
    - Bind on 0.0.0.0; allow keep-alive.

- ticket_id: "AT-3"
  summary: "Asset prefetch handshake packet"
  context_files: []
  spec: >
    • New realtime packet `AssetManifest {pluginId,url,sha256}` small (≤128B).
    • Sent to client **once** after plugin hot-load OR version bump.
    • url points to HTTP endpoint; hostname uses server public IP.
  hints:
    - Use HTTPS if TLS cert present.

- ticket_id: "AT-4"
  summary: "Client async downloader & cache"
  context_files: []
  spec: >
    • C++ helper `HttpDownloader.cpp` using `libcurl` async multi API.
    • Downloads manifest; if local cache missing or sha mismatch,
      fetches zip to `cache/plugins/<sha>.zip`.
    • Verifies sha, unzips to `runtime_cache/plugins/<id>/`.
    • Signals ClientPluginProxy to `ReloadScriptsFrom(path)` once unzip ends.
  hints:
    - Show progress bar  bottom-left; throttle at 8 MB/s default.

- ticket_id: "AT-5"
  summary: "Streaming rate limiter & low-BW skip"
  context_files: []
  spec: >
    • Downloader monitors ENet latency; if ping>250 ms or LowBWMode active,
      pauses HTTP fetch and resumes when network idle.
    • Server includes `minVersion` field; gameplay logic disabled until
      assets present.  Show “Downloading plugin assets…” overlay.
  hints:
    - Resume with HTTP Range requests.

- ticket_id: "AT-6"
  summary: "CI helper to pack & sign assets"
  context_files: []
  spec: >
    • `tools/pack_plugin_assets.py`:
        – zstd-compress folder → zip
        – compute sha256
        – emit manifest
    • Called in build pipeline; copies output to `plugins/`.
  hints:
    - --sign <priv.pem> optional; embeds RSA sig for future integrity check.

###############################################################################
# ★ Flow
# 1. AT-1: manifest spec & builder
# 2. AT-2: side-channel HTTP server
# 3. AT-3: realtime packet with URL
# 4. AT-4: client async download / unzip / hot-reload
# 5. AT-5: rate-limit + grace mode
# 6. AT-6: CI packaging helper
###############################################################################
```