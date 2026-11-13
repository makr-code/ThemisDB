import importlib.util
import json
import os
import shutil
import subprocess
import sys
import tarfile
import tempfile
import unittest
import zipfile
from pathlib import Path
from unittest import mock

import yaml


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts/release/runtime_smoke_gate.py"
sys.path.insert(0, str(SCRIPT.parent))
SPEC = importlib.util.spec_from_file_location("runtime_smoke_gate", SCRIPT)
GATE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(GATE)

SERVER = """#!{python}
import json
import sys
from http.server import BaseHTTPRequestHandler, HTTPServer
if "--version" in sys.argv:
    print("1.2.3")
    sys.exit(0)
config = json.load(open(sys.argv[sys.argv.index("--config") + 1]))
class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.end_headers()
        self.wfile.write(b'{{"status":"healthy"}}')
HTTPServer(("127.0.0.1", config["server"]["port"]), Handler).serve_forever()
"""


def create_server(root, text=SERVER):
    server = root / "bin/themis_server"
    server.parent.mkdir(parents=True)
    server.write_text(text.format(python=sys.executable))
    server.chmod(0o755)
    return server


class RuntimeSmokeGateTests(unittest.TestCase):
    def test_health_requires_json_and_exact_healthy_status(self):
        for body in (b'{"status":"healthy"}', b'{"status":"ok"}', b'{"healthy":true}'):
            self.assertTrue(GATE.healthy(body))
        for body in (b'{"status":"unhealthy"}', b'{"status":"not_ok"}',
                     b'{"status":"unhealthy","healthy":true}',
                     b'{"error":"healthy subsystem failed"}', b"healthy", b"[]"):
            self.assertFalse(GATE.healthy(body))

    def test_environment_does_not_borrow_build_libraries(self):
        with mock.patch.dict(os.environ, {"LD_LIBRARY_PATH": "/build/lib",
                                          "LD_PRELOAD": "/build/lib/test.so"}):
            env = GATE.runtime_environment()
        self.assertNotIn("LD_LIBRARY_PATH", env)
        self.assertNotIn("LD_PRELOAD", env)
        self.assertNotIn("/build", env["PATH"])
        with mock.patch.object(GATE.os, "name", "nt"), mock.patch.dict(
            os.environ, {"SYSTEMROOT": "C:/Windows", "PATH": "C:/vcpkg/bin"}, clear=True
        ):
            env = GATE.runtime_environment()
        self.assertEqual(env["PATH"], os.pathsep.join(
            (os.path.join("C:/Windows", "System32"), "C:/Windows")
        ))

    def test_windows_environment_requires_systemroot(self):
        with mock.patch.object(GATE.os, "name", "nt"), mock.patch.dict(
            os.environ, {}, clear=True
        ):
            with self.assertRaisesRegex(ValueError, "SYSTEMROOT is required"):
                GATE.runtime_environment()

    @unittest.skipIf(os.name == "nt", "POSIX executable fixture")
    def test_native_server_reaches_health_and_is_stopped(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            create_server(root)
            report = {}
            GATE.validate_tree(root, report, 5)
            self.assertEqual(report["version"], "1.2.3")
            self.assertIn("/health", report["health"]["endpoint"])
            self.assertIn("server_log", report)

    @unittest.skipIf(os.name == "nt", "POSIX executable fixture")
    def test_loader_success_does_not_hide_startup_failure(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            create_server(root, """#!{python}
import sys
if "--version" in sys.argv:
    print("1.2.3")
else:
    print("runtime module missing", flush=True)
    sys.exit(7)
""")
            report = {}
            with self.assertRaisesRegex(ValueError, "Server exited"):
                GATE.validate_tree(root, report, 3)
            self.assertIn("runtime module missing", report["server_log"])

    @unittest.skipUnless(sys.platform.startswith("linux") and shutil.which("cc"),
                         "Linux C compiler required")
    def test_real_dynamic_loader_rejects_missing_library(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            build = root / "build"
            package = root / "package"
            build.mkdir()
            package.mkdir()
            library = build / "dependency.c"
            library.write_text("int fixture(void) { return 42; }\n")
            source = build / "server.c"
            source.write_text(
                '#include <stdio.h>\nextern int fixture(void);\n'
                'int main(void) { puts("1.2.3"); return fixture() == 42 ? 0 : 1; }\n'
            )
            subprocess.run(["cc", "-shared", "-fPIC", str(library),
                            "-o", str(package / "libfixture.so")], check=True)
            subprocess.run(["cc", str(source), "-L" + str(package), "-lfixture",
                            "-Wl,-rpath,$ORIGIN", "-o", str(package / "themis_server")],
                           check=True)
            report = {}
            GATE.validate_tree(package, report, 3, dependencies_only=True)
            self.assertIn("libfixture.so", json.dumps(report["libraries"]))
            (package / "libfixture.so").unlink()
            with self.assertRaisesRegex(ValueError, "libfixture.so.*not found"):
                GATE.validate_tree(package, {}, 3, dependencies_only=True)

    @unittest.skipIf(os.name == "nt", "POSIX executable fixture")
    def test_each_final_archive_is_started_independently(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            create_server(root / "source")
            for suffix in ("zip", "tgz"):
                package = root / ("server." + suffix)
                if suffix == "zip":
                    with zipfile.ZipFile(package, "w") as archive:
                        archive.write(root / "source/bin/themis_server", "bin/themis_server")
                else:
                    with tarfile.open(package, "w:gz") as archive:
                        archive.add(root / "source", arcname="package")
                report = {}
                GATE.validate_package(package, report, 5, False)
                self.assertIn("health", report)

    def test_empty_package_selection_fails_with_persisted_report(self):
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "report.json"
            result = subprocess.run(
                [sys.executable, str(SCRIPT), "--packages", temporary,
                 "--output", str(output)], capture_output=True, text=True,
            )
            self.assertEqual(result.returncode, 1)
            self.assertFalse(json.loads(output.read_text())["checks"][0]["ok"])
            self.assertIn("No deployable packages", result.stderr)

    def test_archive_path_traversal_is_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            archive = root / "bad.zip"
            with zipfile.ZipFile(archive, "w") as output:
                output.writestr("../escape", "bad")
            with self.assertRaisesRegex(ValueError, "Unsafe package path"):
                GATE.validate_package(archive, {}, 3, False)

    def test_container_is_removed_even_when_log_collection_fails(self):
        calls = []

        def run(argv, **kwargs):
            calls.append(argv)
            if argv[1] == "port":
                return "127.0.0.1:18080"
            if argv[1] == "logs":
                raise ValueError("log collection failed")
            if "--version" in argv:
                return "1.2.3"
            return ""

        with mock.patch.object(GATE, "command", side_effect=run), mock.patch.object(
            GATE, "wait_for_health", return_value={"endpoint": "fixture"}
        ):
            with self.assertRaisesRegex(ValueError, "log collection"):
                GATE.validate_image("fixture", {}, 3, False)
        self.assertTrue(any(argv[1:3] == ["rm", "-f"] for argv in calls))
        startup = next(argv for argv in calls if argv[1] == "create")
        self.assertEqual(startup[-1], "fixture")
        self.assertNotIn("--entrypoint", startup)

    def test_container_start_failure_is_reported_and_cleaned_up(self):
        calls = []

        def run(argv, **kwargs):
            calls.append(argv)
            if argv[1] == "start":
                raise ValueError("start failed")
            if "--version" in argv:
                return "1.2.3"
            return ""

        with mock.patch.object(GATE, "command", side_effect=run):
            with self.assertRaisesRegex(ValueError, "start failed"):
                GATE.validate_image("fixture", {}, 3, False)
        self.assertTrue(any(argv[1:3] == ["rm", "-f"] for argv in calls))

    def test_release_matrix_requires_isolated_runtime_gate(self):
        workflow = yaml.safe_load((ROOT / ".github/workflows/release-build-matrix.yml").read_text())
        gate = workflow["jobs"]["runtime-artifact-gate"]
        self.assertIn("release-platform-matrix", gate["needs"])
        self.assertNotIn("continue-on-error", gate)
        linux = next(step["run"] for step in gate["steps"] if "clean Ubuntu" in step["name"])
        self.assertIn("ubuntu:24.04", linux)
        self.assertIn("runtime-dependencies-ubuntu-24.04.txt", linux)
        self.assertNotIn("vcpkg", linux)
        self.assertIn("--packages /packages", linux)

    def test_docker_digest_and_nightly_tags_require_runtime_validation(self):
        workflow = yaml.safe_load(
            (ROOT / ".github/workflows/reusable-docker-image-platform.yml").read_text()
        )
        steps = workflow["jobs"]["build"]["steps"]
        gate = next(i for i, step in enumerate(steps) if "final image runtime" in step["name"])
        digest = next(i for i, step in enumerate(steps) if step["name"] == "Write digest artifact")
        self.assertLess(gate, digest)
        nightly = yaml.safe_load((ROOT / ".github/workflows/release-nightly.yml").read_text())
        self.assertIn("publish-docker", nightly["jobs"]["publish-docker-manifest"]["needs"])
        build = next(step for step in nightly["jobs"]["publish-docker"]["steps"]
                     if step["name"] == "Build staged Docker runtime candidate")
        self.assertTrue(build["with"]["push"])
        tags = next(step["run"] for step in nightly["jobs"]["publish-docker"]["steps"]
                    if step["name"] == "Extract metadata for Docker")
        self.assertIn("runtime-candidate-", tags)
        self.assertNotIn('TAGS="themisdb/themisdb:nightly"', tags)
        promote = nightly["jobs"]["publish-docker-manifest"]["steps"][-1]["run"]
        self.assertIn('refs+=("$ref")', promote)
        self.assertNotIn("runtime-candidate-", promote)


if __name__ == "__main__":
    unittest.main()
