#!/usr/bin/env python3

import os
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
import threading
import unittest
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
DOCKERFILE = ROOT / "docker" / "Dockerfile.unified"


class DockerVcpkgArchiveTests(unittest.TestCase):
    def test_boost_locale_overlay_features_do_not_depend_on_vcpkg_layout(self) -> None:
        overlay = ROOT / "ports" / "boost-locale"
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            shutil.copytree(overlay, root / "build" / "ports" / "boost-locale")
            portfile = root / "build" / "ports" / "boost-locale" / "portfile.cmake"
            script = root / "test.cmake"
            script.write_text(
                'cmake_minimum_required(VERSION 3.21)\n'
                'function(vcpkg_from_github)\nendfunction()\n'
                'function(vcpkg_check_features)\n'
                '  if(NOT "${ARGV}" STREQUAL "OUT_FEATURE_OPTIONS;FEATURE_OPTIONS;FEATURES;icu;BOOST_LOCALE_ENABLE_ICU")\n'
                '    message(FATAL_ERROR "Unexpected feature mapping: ${ARGV}")\n'
                '  endif()\n'
                '  if("icu" IN_LIST FEATURES)\n'
                '    set(FEATURE_OPTIONS "-DBOOST_LOCALE_ENABLE_ICU=ON" PARENT_SCOPE)\n'
                '  else()\n'
                '    set(FEATURE_OPTIONS "-DBOOST_LOCALE_ENABLE_ICU=OFF" PARENT_SCOPE)\n'
                '  endif()\n'
                'endfunction()\n'
                'function(boost_configure_and_install)\n'
                '  if(NOT "${ARGV}" STREQUAL "SOURCE_PATH;${SOURCE_PATH};OPTIONS;${FEATURE_OPTIONS}")\n'
                '    message(FATAL_ERROR "Options not passed to Boost configure: ${ARGV}")\n'
                '  endif()\n'
                '  file(WRITE "${OUTPUT}" "${FEATURE_OPTIONS}")\n'
                'endfunction()\n'
                f'include("{portfile.as_posix()}")\n',
                encoding="utf-8",
            )
            for name, flags, expected in (
                ("linux", [], ["ICU=OFF", "ICONV=on", "POSIX=on"]),
                ("linux-icu", ["-DFEATURES=icu"], ["ICU=ON", "ICONV=on", "POSIX=on"]),
                ("windows", ["-DVCPKG_TARGET_IS_WINDOWS=ON"], ["ICU=OFF", "ICONV=off", "POSIX=off"]),
                ("android", ["-DVCPKG_TARGET_IS_ANDROID=ON"], ["ICU=OFF", "ICONV=on", "POSIX=off"]),
                ("mingw", ["-DVCPKG_TARGET_IS_WINDOWS=ON", "-DVCPKG_TARGET_IS_MINGW=ON"],
                 ["ICU=OFF", "ICONV=off", "POSIX=off"]
                 + (["STD=OFF"] if sys.platform.startswith("linux") else [])),
            ):
                with self.subTest(target=name):
                    output = root / f"{name}.txt"
                    result = subprocess.run(
                        ["cmake", f"-DOUTPUT={output}", *flags, "-P", str(script)],
                        capture_output=True, text=True,
                    )
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    options = output.read_text().split(";")
                    self.assertEqual(options, [f"-DBOOST_LOCALE_ENABLE_{value}" for value in expected])

    def test_bootstrap_retries_are_bounded_and_fail_closed(self) -> None:
        base = DOCKERFILE.read_text(encoding="utf-8").split("FROM base AS deps", 1)[0]
        match = re.search(r"BOOTSTRAP_ATTEMPT=1 && \\\n.*?\bdone", base, re.DOTALL)
        self.assertIsNotNone(match)
        command = match.group().replace("\\\n", "\n")
        self.assertIn("${VCPKG_ROOT}/bootstrap-vcpkg.sh -disableMetrics", command)

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            bootstrap = root / "bootstrap-vcpkg.sh"
            bootstrap.write_text(
                '#!/bin/sh\n'
                '[ "$1" = "-disableMetrics" ] || exit 2\n'
                'count=$(cat "$VCPKG_ROOT/attempts" 2>/dev/null || echo 0)\n'
                'count=$((count + 1))\n'
                'echo "$count" > "$VCPKG_ROOT/attempts"\n'
                '[ "$count" -gt "$FAILURES" ]\n',
                encoding="utf-8",
            )
            bootstrap.chmod(0o755)
            for failures, expected_code, expected_attempts in (
                (0, 0, 1),
                (2, 0, 3),
                (5, 1, 5),
            ):
                with self.subTest(failures=failures):
                    (root / "attempts").unlink(missing_ok=True)
                    (root / "delays").unlink(missing_ok=True)
                    (root / "complete").unlink(missing_ok=True)
                    result = subprocess.run(
                        ["/bin/sh", "-c",
                         'sleep() { echo "$1" >> "$VCPKG_ROOT/delays"; }\n'
                         + command + ' && touch "$VCPKG_ROOT/complete"\n'],
                        capture_output=True, text=True, timeout=10,
                        env={**os.environ, "VCPKG_ROOT": directory,
                             "FAILURES": str(failures)},
                    )
                    self.assertEqual(result.returncode, expected_code, result.stderr)
                    self.assertEqual((root / "complete").exists(), expected_code == 0)
                    self.assertEqual(int((root / "attempts").read_text()), expected_attempts)
                    delays = (root / "delays").read_text().splitlines() if (root / "delays").exists() else []
                    self.assertEqual(delays, ["15"] * (expected_attempts - 1))
                    if expected_code:
                        self.assertIn("ERROR: vcpkg bootstrap failed after 5 attempts", result.stderr)

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
