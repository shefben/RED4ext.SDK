import http.server
import json
import time
import os
import hashlib
import secrets
from typing import Dict, Any

SERVERS: Dict[str, Dict[str, Any]] = {}
STATS = []
SECRET = os.environ.get("COOP_SECRET", "changeme")
CHAL_MAP: Dict[str, str] = {}

class Handler(http.server.BaseHTTPRequestHandler):
    def _json_response(self, code: int, payload: Any) -> None:
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(json.dumps(payload).encode("utf-8"))

    def do_GET(self) -> None:
        if self.path == "/api/list":
            now = time.time()
            for sid, info in list(SERVERS.items()):
                if now - info.get("ts", now) > 420:
                    SERVERS.pop(sid, None)
            self._json_response(200, list(SERVERS.values()))
        elif self.path == "/api/challenge":
            nonce = secrets.token_hex(16)
            CHAL_MAP[nonce] = self.client_address[0]
            self._json_response(200, {"nonce": nonce})
        else:
            self.send_response(404)
            self.end_headers()

    def do_POST(self) -> None:
        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length).decode("utf-8") if length else ""
        if self.path in ("/api/heartbeat", "/api/disconnect", "/api/stats"):
            try:
                data = json.loads(body) if body else {}
            except json.JSONDecodeError:
                data = {}
            nonce = data.pop("nonce", "")
            auth = data.pop("auth", "")
            client = CHAL_MAP.pop(nonce, None)
            valid = False
            if client == self.client_address[0]:
                exp = hashlib.sha256((nonce + SECRET).encode()).hexdigest()
                if auth == exp:
                    valid = True
            if not valid:
                self._json_response(403, {"ok": False})
                return
            if self.path == "/api/heartbeat":
                sid = str(data.get("id"))
                if sid:
                    data["ts"] = time.time()
                    SERVERS[sid] = data
                    self._json_response(200, {"ok": True})
                else:
                    self._json_response(400, {"ok": False})
            elif self.path == "/api/disconnect":
                sid = str(data.get("id"))
                if sid and sid in SERVERS:
                    SERVERS.pop(sid, None)
                self._json_response(200, {"ok": True})
            else:  # /api/stats
                STATS.append(data)
                self._json_response(200, {"ok": True})
        elif self.path == "/announce":
            self._json_response(200, {"ok": True})
        else:
            self.send_response(404)
            self.end_headers()

def run(port: int = 8000) -> None:
    server = http.server.HTTPServer(("", port), Handler)
    print(f"Master server listening on {port}")
    server.serve_forever()

if __name__ == "__main__":
    run()
