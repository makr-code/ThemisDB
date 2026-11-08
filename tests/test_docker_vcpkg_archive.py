#!/usr/bin/env python3

import os
import re
import shlex
import shutil
import subprocess
import tempfile
import threading
import unittest
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
DOCKERFILE = ROOT / "docker" / "Dockerfile.unified"


class DockerVcpkgArchiveTests(unittest.TestCase):
    def test_pinned_archive_download_retries_transient_errors(self) -> None:
        text = DOCKERFILE.read_text(encoding="utf-8")
        base = text.split("FROM base AS deps", 1)[0]
        self.assertNotRegex(base, r"(?m)^ADD\s+https://.*vcpkg")
        match = re.search(r"curl -fsSL --retry 5 --retry-delay 3 --connect-timeout 30 --max-time 120 \\\n"
                          r'\s+"(https://github\.com/microsoft/vcpkg/archive/\$\{VCPKG_COMMIT\}\.tar\.gz)" \\\n'
                          r"\s+-o /tmp/vcpkg\.tar\.gz", base)
        self.assertIsNotNone(match)
        self.assertLess(base.index("update-ca-certificates"), match.start())
        self.assertLess(match.end(), base.index("tar -xzf /tmp/vcpkg.tar.gz"))

        if shutil.which("curl") is None:
            self.skipTest("curl is required to exercise the Docker download command")

        class Handler(BaseHTTPRequestHandler):
            planned_statuses = []
            requests = 0

            def do_GET(self):
                type(self).requests += 1
                status = type(self).planned_statuses.pop(0)
                self.send_response(status)
                self.end_headers()
                self.wfile.write(b"archive" if status == 200 else b"error")

            def log_message(self, format, *args):
                pass

        server = HTTPServer(("127.0.0.1", 0), Handler)
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        self.addCleanup(thread.join)
        self.addCleanup(server.server_close)
        self.addCleanup(server.shutdown)

        command = shlex.split(match.group().replace("\\\n", " "))
        command[command.index("--retry-delay") + 1] = "1"
        command[command.index(match.group(1))] = f"http://127.0.0.1:{server.server_port}/archive"
        with tempfile.TemporaryDirectory() as directory:
            command[command.index("/tmp/vcpkg.tar.gz")] = str(Path(directory) / "vcpkg.tar.gz")
            for responses, expected_code, expected_requests in (
                ([504, 200], 0, 2),
                ([503] * 6, 22, 6),
                ([404], 22, 1),
            ):
                with self.subTest(responses=responses):
                    Handler.planned_statuses = responses.copy()
                    Handler.requests = 0
                    result = subprocess.run(
                        command, capture_output=True, text=True,
                        env={**os.environ, "NO_PROXY": "*", "no_proxy": "*"},
                        timeout=15,
                    )
                    self.assertEqual(result.returncode, expected_code, result.stderr)
                    self.assertEqual(Handler.requests, expected_requests)
                    if expected_code == 0:
                        self.assertEqual(Path(command[-1]).read_bytes(), b"archive")


if __name__ == "__main__":
    unittest.main()
