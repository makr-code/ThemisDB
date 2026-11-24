"""Tests for the opt-in local benchmark identity capture."""

import hashlib
import json
import shutil
import subprocess
import sys
import tempfile
import time
import unittest
from pathlib import Path
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools" / "ci" / "capture_benchmark_identity.py"
sys.path.insert(0, str(ROOT / "tools" / "ci"))
import capture_benchmark_identity as capture_identity

WORKLOAD = "themisdb:component/mmdb-secondary-index@1"


class BenchmarkIdentityCaptureTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(
            prefix=".identity-capture-", dir=ROOT, ignore_cleanup_errors=True)
        self.addCleanup(self.cleanup_temporary_directory)
        self.root = Path(self.temporary.name)
        self.paths = {
            "executable": self.root / "bench_mmdb.exe",
            "runner": self.root / "runner.py",
            "result": self.root / "raw-result.json",
            "configuration": self.root / "effective-config.json",
            "output": self.root / "identity.json",
        }
        self.paths["executable"].write_bytes(b"private synthetic executable bytes")
        self.paths["runner"].write_bytes(b"private synthetic runner bytes")
        self.paths["configuration"].write_text(
            '{"rate_limit":"secret-like-value","preset":"test"}', encoding="utf-8")
        self.paths["result"].write_text(json.dumps({
            "benchmarks": [
                {"name": "MMDBFixture/IndexedProductLookup/1000/real_time"},
                {"name": "MMDBFixture/IndexedProductLookup/1000/mean",
                 "run_type": "aggregate"},
            ],
        }), encoding="utf-8")

    def cleanup_temporary_directory(self):
        for attempt in range(20):
            try:
                shutil.rmtree(self.root)
                self.temporary.cleanup()
                return
            except PermissionError:
                if attempt == 19:
                    raise
                time.sleep(0.1)

    def command(self, output=None):
        command = [sys.executable, str(TOOL)]
        for name in ("executable", "runner", "result", "configuration"):
            command.extend(["--" + name, str(self.paths[name])])
        command.extend(["--target", "bench_mmdb",
                        "--output", str(output or self.paths["output"])])
        return command

    def test_cli_captures_independently_recomputed_hashes_without_paths_or_contents(self):
        result = subprocess.run(self.command(), capture_output=True, text=True, check=False)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("not attestation", result.stdout)
        record = json.loads(self.paths["output"].read_text(encoding="utf-8"))
        self.assertEqual(record["identity_scope"], "local-observation-not-attestation")
        self.assertEqual(record["workload"]["workload_id"], WORKLOAD)
        self.assertEqual(
            record["workload"]["catalog_sha256"],
            hashlib.sha256((ROOT / "benchmarks" / "workload_profiles.json").read_bytes()).hexdigest())
        expected = {
            "executable_sha256": self.paths["executable"],
            "runner_sha256": self.paths["runner"],
            "raw_result_sha256": self.paths["result"],
            "configuration_sha256": self.paths["configuration"],
        }
        for key, path in expected.items():
            with self.subTest(key=key):
                self.assertEqual(record["artifacts"][key],
                                 hashlib.sha256(path.read_bytes()).hexdigest())
        self.assertEqual(set(record["host"]), {"os", "hardware"})
        self.assertTrue(record["host"]["hardware"]["cpu_model"])
        self.assertGreater(record["host"]["hardware"]["memory_bytes"], 0)
        serialized = self.paths["output"].read_text(encoding="utf-8")
        self.assertNotIn(str(self.root), serialized)
        self.assertNotIn("secret-like-value", serialized)
        self.assertNotIn("bench_mmdb.exe", serialized)

    def test_changed_raw_case_fails_closed_instead_of_guessing_workload_identity(self):
        self.paths["result"].write_text(
            '{"benchmarks":[{"name":"unmapped case"}]}', encoding="utf-8")
        result = subprocess.run(self.command(), capture_output=True, text=True, check=False)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("no unique catalog link", result.stderr)
        self.assertFalse(self.paths["output"].exists())

    def test_changed_result_bytes_produce_a_different_independently_checkable_hash(self):
        first = subprocess.run(self.command(), capture_output=True, text=True, check=False)
        self.assertEqual(first.returncode, 0, first.stderr)
        original = json.loads(self.paths["output"].read_text(encoding="utf-8"))
        changed_result = json.loads(self.paths["result"].read_text(encoding="utf-8"))
        changed_result["benchmarks"][0]["real_time"] = 123.0
        self.paths["result"].write_text(json.dumps(changed_result), encoding="utf-8")
        second = subprocess.run(
            self.command(self.root / "identity-after-result-change.json"),
            capture_output=True, text=True, check=False)
        self.assertEqual(second.returncode, 0, second.stderr)
        changed = json.loads(
            (self.root / "identity-after-result-change.json").read_text(encoding="utf-8"))
        actual_digest = hashlib.sha256(self.paths["result"].read_bytes()).hexdigest()
        self.assertEqual(changed["artifacts"]["raw_result_sha256"], actual_digest)
        self.assertNotEqual(original["artifacts"]["raw_result_sha256"], actual_digest)

    def test_missing_artifact_and_existing_output_are_errors(self):
        self.paths["runner"].unlink()
        result = subprocess.run(self.command(), capture_output=True, text=True, check=False)
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse(self.paths["output"].exists())

        self.paths["runner"].write_bytes(b"runner")
        self.paths["output"].write_text("do not replace", encoding="utf-8")
        result = subprocess.run(self.command(), capture_output=True, text=True, check=False)
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.paths["output"].read_text(encoding="utf-8"), "do not replace")

    def test_target_must_match_executable(self):
        executable = self.root / "other.exe"
        executable.write_bytes(b"other executable")
        command = self.command()
        command[command.index("--executable") + 1] = str(executable)
        result = subprocess.run(command, capture_output=True, text=True, check=False)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("target must match", result.stderr)
        self.assertFalse(self.paths["output"].exists())

    def test_missing_hardware_identity_is_rejected(self):
        os_fingerprint = {
            "system": "Windows", "release": "11", "distribution": None,
            "distribution_version": "10.0.26100",
        }
        hardware = {
            "architecture": "AMD64", "cpu_model": None,
            "logical_processors": 8, "memory_bytes": 1024,
        }
        with self.assertRaisesRegex(ValueError, "CPU model"):
            capture_identity.validate_host_fingerprint(os_fingerprint, hardware)

        with mock.patch.object(capture_identity, "collect_host_fingerprint",
                               return_value={"os": os_fingerprint, "hardware": hardware}):
            with self.assertRaisesRegex(ValueError, "CPU model"):
                capture_identity.capture(
                    self.paths["executable"], self.paths["runner"], self.paths["result"],
                    self.paths["configuration"], "bench_mmdb")


if __name__ == "__main__":
    unittest.main()
