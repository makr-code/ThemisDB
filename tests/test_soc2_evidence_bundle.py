#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
WORKFLOW_PATH = REPO_ROOT / ".github" / "workflows" / "compliance-supply-chain.yml"
VERIFY_SCRIPT = REPO_ROOT / "scripts" / "verify-evidence-bundle.py"


class VerifyEvidenceBundleTests(unittest.TestCase):
    def run_verify(self, payload: dict) -> subprocess.CompletedProcess[str]:
        with tempfile.TemporaryDirectory() as temp_dir:
            bundle_path = Path(temp_dir) / "bundle.json"
            bundle_path.write_text(json.dumps(payload), encoding="utf-8")
            return subprocess.run(
                [sys.executable, str(VERIFY_SCRIPT), str(bundle_path)],
                check=False,
                text=True,
                capture_output=True,
            )

    def test_accepts_current_security_evidence_collector_schema(self) -> None:
        result = self.run_verify(
            {
                "bundle_id": "bundle-1",
                "collected_at_ms": 1,
                "window_from_ms": 2,
                "window_to_ms": 3,
                "within_retention_window": True,
                "audit_log": {
                    "from_ms": 2,
                    "to_ms": 3,
                    "entries": [],
                },
                "metrics": {"collected_at_ms": 1},
                "key_rotations": [],
                "access_control": {"generated_at_ms": 1},
            }
        )

        self.assertEqual(result.returncode, 0, msg=result.stderr)
        self.assertIn("bundle-1", result.stdout)

    def test_accepts_legacy_event_bundle_schema(self) -> None:
        result = self.run_verify(
            {
                "generated_at": "2026-09-26T00:00:00Z",
                "window_start": "2026-09-19T00:00:00Z",
                "window_end": "2026-09-26T00:00:00Z",
                "endpoint": "http://localhost:18080",
                "events": [{"timestamp": "2026-09-26T00:00:00Z", "event_type": "audit"}],
            }
        )

        self.assertEqual(result.returncode, 0, msg=result.stderr)
        self.assertIn("1 event(s)", result.stdout)

    def test_rejects_current_bundle_without_audit_entries_array(self) -> None:
        result = self.run_verify(
            {
                "bundle_id": "bundle-2",
                "collected_at_ms": 1,
                "window_from_ms": 2,
                "window_to_ms": 3,
                "within_retention_window": True,
                "audit_log": {
                    "from_ms": 2,
                    "to_ms": 3,
                    "entries": {},
                },
                "metrics": {"collected_at_ms": 1},
                "key_rotations": [],
                "access_control": {"generated_at_ms": 1},
            }
        )

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("audit_log.entries", result.stderr)


class Soc2WorkflowRegressionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.workflow_text = WORKFLOW_PATH.read_text(encoding="utf-8")

    def test_workflow_targets_current_local_service(self) -> None:
        self.assertIn("THEMIS_ADMIN_ENDPOINT: 'http://localhost:18080'", self.workflow_text)
        self.assertIn("build themis", self.workflow_text)
        self.assertIn("up -d --no-build themis", self.workflow_text)

    def test_workflow_uses_api_health_probe(self) -> None:
        self.assertIn('${THEMIS_ADMIN_ENDPOINT}/api/health', self.workflow_text)
        self.assertIn("logs --tail=200 themis", self.workflow_text)


if __name__ == "__main__":
    unittest.main()
