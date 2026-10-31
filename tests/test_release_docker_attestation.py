import os
import subprocess
import tempfile
import unittest
from pathlib import Path

import yaml


REPO_ROOT = Path(__file__).resolve().parents[1]
WORKFLOW_PATH = REPO_ROOT / ".github" / "workflows" / "release-docker-image.yml"


class ReleaseDockerAttestationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        workflow = yaml.safe_load(WORKFLOW_PATH.read_text(encoding="utf-8"))
        manifest_steps = workflow["jobs"]["merge-image-manifest"]["steps"]
        cls.manifest_script = next(
            step["run"] for step in manifest_steps if step.get("id") == "manifest"
        )
        cls.attestation_step = next(
            step
            for step in manifest_steps
            if step.get("uses", "").startswith("actions/attest-build-provenance@")
        )

    def test_attestation_subject_is_qualified_and_tagless_for_supported_registries(self) -> None:
        subject_script = self.manifest_script[
            self.manifest_script.index('PRIMARY_IMAGE="${PRIMARY_TAG%:*}"') :
            self.manifest_script.index('echo "subject_name=${ATTESTATION_SUBJECT}"')
            + len('echo "subject_name=${ATTESTATION_SUBJECT}" >> "${GITHUB_OUTPUT}"')
        ]
        cases = {
            "themisdb/themisdb:2.4.0-alpha": "index.docker.io/themisdb/themisdb",
            "ghcr.io/makr-code/themisdb:2.4.0-alpha": "ghcr.io/makr-code/themisdb",
        }

        self.assertEqual(
            self.attestation_step["with"]["subject-name"],
            "${{ steps.manifest.outputs.subject_name }}",
        )

        for primary_tag, expected_subject in cases.items():
            with self.subTest(primary_tag=primary_tag), tempfile.NamedTemporaryFile() as output:
                subprocess.run(
                    ["bash", "-euo", "pipefail", "-c", subject_script],
                    check=True,
                    env={
                        **os.environ,
                        "PRIMARY_TAG": primary_tag,
                        "GITHUB_OUTPUT": output.name,
                    },
                )
                output.seek(0)
                self.assertIn(
                    f"subject_name={expected_subject}",
                    output.read().decode("utf-8"),
                )


if __name__ == "__main__":
    unittest.main()
