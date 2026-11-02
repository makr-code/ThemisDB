import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

import yaml


REPO_ROOT = Path(__file__).resolve().parents[1]
VALIDATOR = REPO_ROOT / "tests" / "test_docker_runtime_package_regressions.py"
WORKFLOW = REPO_ROOT / ".github" / "workflows" / "reusable-docker-image-platform.yml"


class DockerPlatformValidationTests(unittest.TestCase):
    def test_workflow_uses_workflow_validator_without_changing_source_revision(self) -> None:
        workflow = yaml.safe_load(WORKFLOW.read_text(encoding="utf-8"))
        steps = workflow["jobs"]["build"]["steps"]
        checkout = next(step for step in steps if step.get("name") == "Checkout")
        validation = next(
            step for step in steps
            if step.get("name") == "Validate Docker dependency providers and runtime packages"
        )
        self.assertEqual(checkout["with"]["ref"], "${{ inputs.source_sha || github.sha }}")
        self.assertEqual(validation["env"]["WORKFLOW_SHA"], "${{ github.sha }}")
        self.assertEqual(validation["env"]["THEMIS_SOURCE_ROOT"], "${{ github.workspace }}")
        self.assertIn('git fetch --no-tags origin "$WORKFLOW_SHA"', validation["run"])
        self.assertIn(
            'git show "${WORKFLOW_SHA}:tests/test_docker_runtime_package_regressions.py"',
            validation["run"],
        )
        self.assertIn('python3 "$VALIDATOR" -v', validation["run"])
        subprocess.run(["bash", "-n", "-c", validation["run"]], check=True)

    def test_relocated_validator_checks_source_without_test_file(self) -> None:
        self._run_relocated_validator(invalid_runtime=False)

    def test_relocated_validator_still_rejects_invalid_source(self) -> None:
        self._run_relocated_validator(invalid_runtime=True)

    def _run_relocated_validator(self, invalid_runtime: bool) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "source"
            (source / "docker").mkdir(parents=True)
            for name in ("Dockerfile.unified", "vcpkg-community.json"):
                shutil.copyfile(REPO_ROOT / "docker" / name, source / "docker" / name)
            if invalid_runtime:
                dockerfile = source / "docker" / "Dockerfile.unified"
                dockerfile.write_text(
                    dockerfile.read_text(encoding="utf-8").replace("librocksdb8.9", "librocksdb7"),
                    encoding="utf-8",
                )
            validator = root / "validator.py"
            shutil.copyfile(VALIDATOR, validator)
            self.assertFalse((source / "tests").exists())
            result = subprocess.run(
                ["python3", str(validator), "-v"],
                cwd=root,
                env={**os.environ, "THEMIS_SOURCE_ROOT": str(source)},
                capture_output=True,
                text=True,
            )
            if invalid_runtime:
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("librocksdb7", result.stderr)
            else:
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertIn("OK", result.stderr)


if __name__ == "__main__":
    unittest.main()
