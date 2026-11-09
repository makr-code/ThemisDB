import hashlib
import importlib.util
import io
import json
import sys
import tempfile
import unittest
from contextlib import redirect_stdout
from pathlib import Path
from unittest.mock import patch


SCRIPT_PATH = (
    Path(__file__).resolve().parents[1]
    / "tools"
    / "ci"
    / "validate_c5_evidence_manifest.py"
)
SPEC = importlib.util.spec_from_file_location("validate_c5_evidence_manifest", SCRIPT_PATH)
VALIDATOR = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(VALIDATOR)


def valid_manifest() -> dict:
    return {
        "generated_at": "2026-11-09T17:00:00+00:00",
        "release_id": "v2.5.0-rc-candidate",
        "workflow": "compliance-supply-chain.yml",
        "run_id": "42321944686",
        "ref": "refs/heads/develop",
        "scope": "bsi-c5-2026-delta",
        "control_status": {"BCM-03": "partial"},
        "evidence_references": {
            "workflow_run": "https://github.com/makr-code/ThemisDB/actions/runs/42321944686"
        },
        "notes": "Evidence gaps remain open.",
    }


class C5EvidenceManifestValidationTests(unittest.TestCase):
    def test_accepts_well_formed_manifest_with_numeric_run_reference(self) -> None:
        self.assertEqual(VALIDATOR.validate_manifest(valid_manifest()), [])

    def test_rejects_manual_run_placeholder(self) -> None:
        manifest = valid_manifest()
        manifest["run_id"] = "manual-2026-11-09"

        errors = VALIDATOR.validate_manifest(manifest)

        self.assertIn("run_id must not use a manual-* placeholder", errors)

    def test_rejects_workflow_url_for_a_different_run(self) -> None:
        manifest = valid_manifest()
        manifest["evidence_references"]["workflow_run"] = (
            "https://github.com/makr-code/ThemisDB/actions/runs/123"
        )

        errors = VALIDATOR.validate_manifest(manifest)

        self.assertIn(
            "evidence_references.workflow_run run ID must match run_id",
            errors,
        )

    def test_rejects_missing_fields_and_unzoned_timestamps(self) -> None:
        manifest = valid_manifest()
        manifest.pop("scope")
        manifest["generated_at"] = "2026-11-09T17:00:00"

        errors = VALIDATOR.validate_manifest(manifest)

        self.assertIn("missing required field: scope", errors)
        self.assertIn("generated_at must include a timezone", errors)

    def test_validation_report_binds_result_to_manifest_digest(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            manifest_path = Path(tmp) / "manifest.json"
            report_path = Path(tmp) / "report.json"
            manifest_path.write_text(json.dumps(valid_manifest()), encoding="utf-8")

            with patch.object(
                sys,
                "argv",
                [
                    "validate_c5_evidence_manifest.py",
                    str(manifest_path),
                    "--report-json",
                    str(report_path),
                ],
            ), redirect_stdout(io.StringIO()):
                result = VALIDATOR.main()

            report = json.loads(report_path.read_text(encoding="utf-8"))
            self.assertEqual(result, 0)
            self.assertTrue(report["pass"])
            self.assertEqual(
                report["manifest_sha256"],
                hashlib.sha256(manifest_path.read_bytes()).hexdigest(),
            )


if __name__ == "__main__":
    unittest.main()
