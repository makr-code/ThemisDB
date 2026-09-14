#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import json
import sys
import tempfile
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_PATH = REPO_ROOT / "tools" / "ci" / "validate_wave_closure_packages.py"


def _load_module():
    spec = importlib.util.spec_from_file_location("validate_wave_closure_packages", SCRIPT_PATH)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


validator = _load_module()


def _write_manifest(path: Path, payload: dict, *, create_md: bool = True) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    if create_md:
        path.with_suffix(".md").write_text("# closure package\n", encoding="utf-8")


def _manifest(wave: str, module: str, status: str, run_id: str = "123") -> dict:
    captured = status == "evidence-captured"
    return {
        "generated_at": "2026-09-14T15:20:00Z",
        "wave": wave,
        "module": module,
        "workflow": "test.yml",
        "run_id": run_id,
        "overall_status": status,
        "gates": {"gate": "success"},
        "evidence_references": {"artifact": "dummy"},
        "source_validation": {
            "code": captured,
            "tests": captured,
            "ci": captured,
            "benchmarks": captured,
        },
        "notes": "test",
    }


class WaveClosureValidationTests(unittest.TestCase):
    def test_validation_passes_with_all_waves_open_and_no_ordering_violation(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            manifests = root / "manifests"
            _write_manifest(manifests / "a_closure_manifest.json", _manifest("Wave A", "gpu", "open"))
            _write_manifest(manifests / "b_closure_manifest.json", _manifest("Wave B", "transaction", "open"))
            _write_manifest(manifests / "c_closure_manifest.json", _manifest("Wave C", "security", "open"))
            _write_manifest(manifests / "d_closure_manifest.json", _manifest("Wave D", "ops", "open"))

            output = root / "out.json"
            old_argv = sys.argv[:]
            try:
                sys.argv = [
                    "validate_wave_closure_packages.py",
                    "--manifest-dir",
                    str(manifests),
                    "--output-json",
                    str(output),
                ]
                code = validator.main()
            finally:
                sys.argv = old_argv

            payload = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(code, 0)
            self.assertTrue(payload["pass"])
            self.assertEqual(payload["wave_status"]["A"], "open")

    def test_validation_fails_on_missing_required_field(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            manifests = root / "manifests"
            broken = _manifest("Wave A", "gpu", "open")
            del broken["run_id"]
            _write_manifest(manifests / "a_closure_manifest.json", broken)

            output = root / "out.json"
            old_argv = sys.argv[:]
            try:
                sys.argv = [
                    "validate_wave_closure_packages.py",
                    "--manifest-dir",
                    str(manifests),
                    "--output-json",
                    str(output),
                ]
                code = validator.main()
            finally:
                sys.argv = old_argv

            payload = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(code, 1)
            self.assertFalse(payload["pass"])
            self.assertTrue(any("missing required fields" in v for v in payload["violations"]))

    def test_validation_fails_when_wave_order_is_violated(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            manifests = root / "manifests"
            _write_manifest(manifests / "a_closure_manifest.json", _manifest("Wave A", "gpu", "evidence-captured"))
            _write_manifest(manifests / "b_closure_manifest.json", _manifest("Wave B", "transaction", "open"))
            _write_manifest(manifests / "c_closure_manifest.json", _manifest("Wave C", "security", "evidence-captured"))

            output = root / "out.json"
            old_argv = sys.argv[:]
            try:
                sys.argv = [
                    "validate_wave_closure_packages.py",
                    "--manifest-dir",
                    str(manifests),
                    "--output-json",
                    str(output),
                ]
                code = validator.main()
            finally:
                sys.argv = old_argv

            payload = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(code, 1)
            self.assertFalse(payload["pass"])
            self.assertTrue(any("Wave C marked evidence-captured while Wave B" in v for v in payload["violations"]))

    def test_validation_fails_when_manifest_directory_is_missing(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            missing_dir = root / "does-not-exist"
            output = root / "out.json"
            old_argv = sys.argv[:]
            try:
                sys.argv = [
                    "validate_wave_closure_packages.py",
                    "--manifest-dir",
                    str(missing_dir),
                    "--output-json",
                    str(output),
                ]
                code = validator.main()
            finally:
                sys.argv = old_argv

            payload = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(code, 1)
            self.assertFalse(payload["pass"])
            self.assertTrue(any("No closure manifests found" in v for v in payload["validation_errors"]))

    def test_validation_fails_when_markdown_sidecar_is_missing(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            manifests = root / "manifests"
            _write_manifest(
                manifests / "a_closure_manifest.json",
                _manifest("Wave A", "gpu", "open"),
                create_md=False,
            )

            output = root / "out.json"
            old_argv = sys.argv[:]
            try:
                sys.argv = [
                    "validate_wave_closure_packages.py",
                    "--manifest-dir",
                    str(manifests),
                    "--output-json",
                    str(output),
                ]
                code = validator.main()
            finally:
                sys.argv = old_argv

            payload = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(code, 1)
            self.assertFalse(payload["pass"])
            self.assertTrue(any("missing markdown sidecar" in v for v in payload["violations"]))

    def test_validation_fails_when_required_manifest_from_policy_is_missing(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            manifests = root / "manifests"
            _write_manifest(manifests / "gpu_wave_a_closure_manifest.json", _manifest("Wave A", "gpu", "open"))

            policy = root / "policy.json"
            policy.write_text(
                json.dumps(
                    {
                        "required_manifests": [
                            {"file": "gpu_wave_a_closure_manifest.json", "wave": "Wave A", "module": "gpu"},
                            {
                                "file": "wave_b_module_hardening_closure_manifest.json",
                                "wave": "Wave B",
                                "module": "wave-b-module-hardening",
                            },
                        ]
                    }
                ),
                encoding="utf-8",
            )

            output = root / "out.json"
            old_argv = sys.argv[:]
            try:
                sys.argv = [
                    "validate_wave_closure_packages.py",
                    "--manifest-dir",
                    str(manifests),
                    "--policy-file",
                    str(policy),
                    "--output-json",
                    str(output),
                ]
                code = validator.main()
            finally:
                sys.argv = old_argv

            payload = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(code, 1)
            self.assertFalse(payload["pass"])
            self.assertTrue(any("Missing required closure manifest" in v for v in payload["violations"]))

    def test_validation_fails_when_required_manifest_metadata_mismatches_policy(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            manifests = root / "manifests"
            _write_manifest(
                manifests / "gpu_wave_a_closure_manifest.json",
                _manifest("Wave B", "gpu-wrong", "open"),
            )

            policy = root / "policy.json"
            policy.write_text(
                json.dumps(
                    {
                        "required_manifests": [
                            {"file": "gpu_wave_a_closure_manifest.json", "wave": "Wave A", "module": "gpu"}
                        ]
                    }
                ),
                encoding="utf-8",
            )

            output = root / "out.json"
            old_argv = sys.argv[:]
            try:
                sys.argv = [
                    "validate_wave_closure_packages.py",
                    "--manifest-dir",
                    str(manifests),
                    "--policy-file",
                    str(policy),
                    "--output-json",
                    str(output),
                ]
                code = validator.main()
            finally:
                sys.argv = old_argv

            payload = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(code, 1)
            self.assertFalse(payload["pass"])
            self.assertTrue(any("expected wave 'Wave A'" in v for v in payload["violations"]))
            self.assertTrue(any("expected module 'gpu'" in v for v in payload["violations"]))


if __name__ == "__main__":
    unittest.main()
