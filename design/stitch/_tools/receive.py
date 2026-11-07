"""Tiny local receiver for font_bake.html: POST /<name> stores the body in tools/ui2/fonts/<name>.

Run:  python design/stitch/_tools/receive.py   (listens on 127.0.0.1:8766 only)
Only file names made of letters, digits, '_', '-' and '.' are accepted.
"""
import http.server
import os
import re

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
OUT = os.path.join(ROOT, "tools", "ui2", "fonts")


class Handler(http.server.BaseHTTPRequestHandler):
    def _cors(self):
        self.send_header("Access-Control-Allow-Origin", "http://localhost:8765")
        self.send_header("Access-Control-Allow-Methods", "POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")

    def do_OPTIONS(self):
        self.send_response(204)
        self._cors()
        self.end_headers()

    def do_POST(self):
        name = self.path.strip("/")
        if not re.fullmatch(r"[A-Za-z0-9_.-]+", name):
            self.send_response(400)
            self._cors()
            self.end_headers()
            return
        data = self.rfile.read(int(self.headers.get("Content-Length", 0)))
        os.makedirs(OUT, exist_ok=True)
        with open(os.path.join(OUT, name), "wb") as f:
            f.write(data)
        self.send_response(200)
        self._cors()
        self.end_headers()
        self.wfile.write(str(len(data)).encode())


if __name__ == "__main__":
    http.server.HTTPServer(("127.0.0.1", 8766), Handler).serve_forever()
