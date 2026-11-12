from __future__ import annotations

import importlib.util
import json
import os
import sys
import tempfile
import unittest
from datetime import datetime, timedelta, timezone
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
SCRIPT_PATH = REPO_ROOT / ".github" / "scripts" / "verify_maturity_gates.py"
SPEC = importlib.util.spec_from_file_location("verify_maturity_gates", SCRIPT_PATH)
verifier = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = verifier
SPEC.loader.exec_module(verifier)


class VerifyMaturityGatesTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory()
        self.repo_root = Path(self.temp_dir.name)
        self.now = datetime(2026, 11, 12, tzinfo=timezone.utc)
        self.artifact = self.repo_root / "evidence.md"
        self.artifact.write_text("# GATE-TEST\n## Evidence\nPASS\n", encoding="utf-8")
        os.utime(self.artifact, (self.now.timestamp(), self.now.timestamp()))
        self.manifest = {
            "schema_version": "1.0",
            "gates": {
                "wave_7_performance": [
                    {
                        "gate_id": "GATE-TEST",
                        "status": "PASS",
                        "artifact_path": "evidence.md",
                        "section_marker": "## Evidence",
                        "validation_rule": (
                            "file_exists AND file_modified_within(7_days) "
                            "AND contains_gate_id AND section_contains('PASS')"
                        ),
                    }
                ]
            },
        }

    def tearDown(self) -> None:
        self.temp_dir.cleanup()

    def test_passes_supported_rules_for_grouped_manifest(self) -> None:
        report = verifier.evaluate_manifest(
            self.manifest, self.repo_root, groups=["wave_7_performance"], now=self.now
        )

        self.assertTrue(report["pass"])
        self.assertEqual(report["groups"]["wave_7_performance"]["pass"], 1)

    def test_contains_regex_matches_across_lines(self) -> None:
        roadmap = self.repo_root / "roadmap.md"
        roadmap.write_text("### Phase 1: Design\n- [x] complete\n", encoding="utf-8")
        entry = {
            "status": "PASS",
            "artifact_path": "roadmap.md",
            "validation_rule": r"file_exists AND contains('Phase 1.*\[x\]')",
        }

        passed, details = verifier._evaluate_rule(
            entry["validation_rule"], entry, self.repo_root, self.now
        )

        self.assertTrue(passed, details)

    def test_fails_when_artifact_is_stale(self) -> None:
        old_time = (self.now - timedelta(days=8)).timestamp()
        os.utime(self.artifact, (old_time, old_time))

        report = verifier.evaluate_manifest(self.manifest, self.repo_root, now=self.now)

        self.assertFalse(report["pass"])
        self.assertIn("8.0 days old", report["checks"][0]["details"])

    def test_pending_gate_is_reported_but_not_misrepresented_as_pass(self) -> None:
        self.manifest["gates"]["wave_7_performance"][0]["status"] = "PENDING"

        report = verifier.evaluate_manifest(self.manifest, self.repo_root, now=self.now)

        self.assertTrue(report["pass"])
        self.assertEqual(report["checks"][0]["status"], "PENDING")
        self.assertEqual(report["groups"]["wave_7_performance"]["pass"], 0)
        self.assertEqual(report["groups"]["wave_7_performance"]["pending"], 1)

    def test_unsupported_rule_fails_closed(self) -> None:
        self.manifest["gates"]["wave_7_performance"][0]["validation_rule"] = "future_rule"

        report = verifier.evaluate_manifest(self.manifest, self.repo_root, now=self.now)

        self.assertFalse(report["pass"])
        self.assertIn("unsupported validation rule", report["checks"][0]["details"])

    def test_rejects_artifact_path_outside_repository(self) -> None:
        self.manifest["gates"]["wave_7_performance"][0]["artifact_path"] = "../outside.md"

        report = verifier.evaluate_manifest(self.manifest, self.repo_root, now=self.now)

        self.assertFalse(report["pass"])
        self.assertIn("outside the repository", report["checks"][0]["details"])

    def test_repository_manifest_is_read_as_grouped_gate_records(self) -> None:
        manifest_path = REPO_ROOT / "docs" / "governance" / "MATURITY_EVIDENCE_MANIFEST.json"
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))

        report = verifier.evaluate_manifest(manifest, REPO_ROOT, now=self.now)

        self.assertEqual(set(report["groups"]), set(manifest["gates"]))
        self.assertEqual(len(report["checks"]), sum(len(entries) for entries in manifest["gates"].values()))

    def test_cli_writes_report_and_returns_failure(self) -> None:
        self.manifest["gates"]["wave_7_performance"][0]["status"] = "FAIL"
        manifest_path = self.repo_root / "manifest.json"
        output_path = self.repo_root / "report.json"
        manifest_path.write_text(json.dumps(self.manifest), encoding="utf-8")

        result = verifier.main(
            [
                "--repo-root",
                str(self.repo_root),
                "--manifest",
                str(manifest_path),
                "--output",
                str(output_path),
            ]
        )

        self.assertEqual(result, 1)
        report = json.loads(output_path.read_text(encoding="utf-8"))
        self.assertFalse(report["pass"])
        self.assertEqual(report["checks"][0]["status"], "FAIL")


if __name__ == "__main__":
    unittest.main()
