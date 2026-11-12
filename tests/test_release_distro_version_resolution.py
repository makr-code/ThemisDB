from __future__ import annotations

import unittest
from pathlib import Path

import yaml


ROOT = Path(__file__).resolve().parents[1]
WORKFLOWS = ROOT / ".github" / "workflows"


def load_workflow(name: str) -> dict:
    return yaml.load((WORKFLOWS / name).read_text(encoding="utf-8"), Loader=yaml.BaseLoader)


class ReleaseDistroVersionResolutionTests(unittest.TestCase):
    def test_manual_version_override_is_optional_but_reusable_input_stays_required(self) -> None:
        for name in ("release-linux-distribution.yml", "release-windows-distribution.yml"):
            with self.subTest(workflow=name):
                triggers = load_workflow(name)["on"]
                dispatch_version = triggers["workflow_dispatch"]["inputs"]["version"]
                call_version = triggers["workflow_call"]["inputs"]["version"]

                self.assertEqual(dispatch_version["type"], "string")
                self.assertEqual(dispatch_version["default"], "")
                self.assertEqual(dispatch_version["required"], "false")
                self.assertIn("canonical VERSION file", dispatch_version["description"])
                self.assertEqual(call_version["required"], "true")

    def test_version_resolution_uses_safe_environment_and_reports_source(self) -> None:
        expectations = {
            "release-linux-distribution.yml": ("metadata_profile", "release_tag"),
            "release-windows-distribution.yml": ("publish_environment", None),
        }

        for name, (additional_output, source_name) in expectations.items():
            with self.subTest(workflow=name):
                jobs = load_workflow(name)["jobs"]
                detect = jobs["detect-release"]
                detect_step = next(step for step in detect["steps"] if step.get("id") == "detect")
                detect_run = detect_step["run"]
                validate_step = next(
                    step for step in detect["steps"]
                    if step.get("name") == "Validate explicit version matches canonical contract"
                )
                summary_step = jobs["run-summary"]["steps"][1]

                self.assertEqual(detect_step["env"]["VERSION_OVERRIDE"], "${{ inputs.version }}")
                self.assertIn('RAW_VERSION="${VERSION_OVERRIDE}"', detect_run)
                self.assertIn('read-version)', detect_run)
                self.assertIn('VERSION_SOURCE="VERSION"', detect_run)
                self.assertIn('VERSION_SOURCE="operator_override"', detect_run)
                self.assertIn("echo \"version_source=${VERSION_SOURCE}\"", detect_run)
                self.assertNotIn("${{ inputs.version }}", detect_run)
                self.assertNotIn("${{ github.event.inputs.version }}", detect_run)
                self.assertEqual(validate_step["env"]["VERSION_OVERRIDE"], "${{ inputs.version }}")
                self.assertIn('validate-input-version "${VERSION_OVERRIDE}"', validate_step["run"])
                self.assertIn("version_source", detect["outputs"])
                self.assertIn("version ${{ needs.detect-release.outputs.version }}", summary_step["with"]["scope"])
                self.assertIn("source: ${{ needs.detect-release.outputs.version_source }}", summary_step["with"]["scope"])
                self.assertIn(additional_output, detect["outputs"])
                if source_name:
                    self.assertIn(f'VERSION_SOURCE="{source_name}"', detect_run)


if __name__ == "__main__":
    unittest.main()
