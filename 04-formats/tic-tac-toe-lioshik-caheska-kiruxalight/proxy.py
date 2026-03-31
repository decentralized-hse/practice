#!/usr/bin/env python3
"""Proxy server that forwards requests to Beagle.

Beagle's HTTP parser has issues with browser requests (too many headers).
This proxy strips unnecessary headers before forwarding.
"""

import http.server
import urllib.request
import sys

BEAGLE_PORT = 8080
PROXY_PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 9090


class BeagleProxy(http.server.BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):
        sys.stderr.write(f"{args[0]}\n")

    def do_GET(self):
        self._forward()

    def do_POST(self):
        self._forward()

    def _forward(self):
        cl = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(cl) if cl > 0 else None

        req = urllib.request.Request(
            f"http://localhost:{BEAGLE_PORT}{self.path}",
            data=body,
            method=self.command,
        )

        try:
            resp = urllib.request.urlopen(req, timeout=120)
            data = resp.read()
            self.send_response(resp.status)
            self.send_header("Content-Length", len(data))
            self.send_header("Connection", "close")
            self.end_headers()
            self.wfile.write(data)
        except urllib.error.HTTPError as e:
            self.send_response(e.code)
            self.end_headers()
        except Exception:
            self.send_response(502)
            self.end_headers()


if __name__ == "__main__":
    print(f"Proxy listening on :{PROXY_PORT} -> Beagle :{BEAGLE_PORT}")
    http.server.HTTPServer(("", PROXY_PORT), BeagleProxy).serve_forever()
