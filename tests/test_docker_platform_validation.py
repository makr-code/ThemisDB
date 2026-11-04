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
RELEASE_WORKFLOW = REPO_ROOT / ".github" / "workflows" / "release-docker-image.yml"


class DockerPlatformValidationTests(unittest.TestCase):
    def test_release_forwards_source_preflight_override_to_platforms(self) -> None:
        jobs = yaml.safe_load(RELEASE_WORKFLOW.read_text(encoding="utf-8"))["jobs"]
        build = jobs["build-platforms"]
        self.assertEqual(
            build["uses"], "./.github/workflows/reusable-docker-image-platform.yml"
        )
        self.assertEqual(
            build["with"]["override_docker_source_preflight"],
            "${{ inputs.override_docker_source_preflight }}",
        )
        self.assertEqual(
            build["with"]["source_sha"],
            "${{ needs.prepare-release-context.outputs.source_sha }}",
        )

    def test_platform_source_preflight_defaults_to_enabled(self) -> None:
        workflow = yaml.safe_load(WORKFLOW.read_text(encoding="utf-8"))
        # PyYAML's YAML 1.1 loader parses the workflow's "on" key as True.
        inputs = workflow[True]["workflow_call"]["inputs"]
        override = inputs["override_docker_source_preflight"]
        self.assertEqual(override["type"], "boolean")
        self.assertIs(override["default"], False)
        self.assertIs(override["required"], False)
        steps = workflow["jobs"]["build"]["steps"]
        validation = next(
            step for step in steps
            if step.get("name") == "Validate Docker dependency providers and runtime packages"
        )
        warning = next(
            step for step in steps
            if step.get("name") == "Report Docker source preflight override"
        )
        # Exact native boolean expressions: omitted/false validates; only true warns.
        self.assertEqual(validation["if"], "${{ !inputs.override_docker_source_preflight }}")
        self.assertEqual(warning["if"], "${{ inputs.override_docker_source_preflight }}")

    def test_platform_override_warning_uses_environment_for_source_and_platform(self) -> None:
        workflow = yaml.safe_load(WORKFLOW.read_text(encoding="utf-8"))
        warning = next(
            step for step in workflow["jobs"]["build"]["steps"]
            if step.get("name") == "Report Docker source preflight override"
        )
        self.assertEqual(warning["env"], {
            "RELEASE_VERSION": "${{ inputs.version }}",
            "SOURCE_SHA": "${{ inputs.source_sha || github.sha }}",
            "PLATFORM": "${{ inputs.platform }}",
        })
        self.assertNotIn("${{", warning["run"])
        subprocess.run(["bash", "-n", "-c", warning["run"]], check=True)
        version = "v2.4.0-alpha'; $(exit 91)"
        for platform in ("linux/amd64", "linux/arm64"):
            with self.subTest(platform=platform):
                result = subprocess.run(
                    ["bash", "-euo", "pipefail", "-c", warning["run"]],
                    env={**os.environ, "RELEASE_VERSION": version,
                         "SOURCE_SHA": "old-source-sha", "PLATFORM": platform},
                    check=True, capture_output=True, text=True,
                )
                self.assertIn("::warning::", result.stdout)
                self.assertIn(version, result.stdout)
                self.assertIn("old-source-sha", result.stdout)
                self.assertIn(platform, result.stdout)
                self.assertIn("broken image", result.stdout)
                self.assertIn("selected source commit was not changed", result.stdout)

    def test_platform_override_does_not_bypass_other_guards(self) -> None:
        workflow = yaml.safe_load(WORKFLOW.read_text(encoding="utf-8"))
        build = workflow["jobs"]["build"]
        self.assertNotIn("override_docker_source_preflight", build["if"])
        steps = build["steps"]
        for name, condition in (
            ("Validate Dockerfile model-case syntax", None),
            (
                "Validate Docker vcpkg bootstrap architecture",
                "${{ inputs.platform == 'linux/arm64' && inputs.push_to_registry }}",
            ),
            ("Enforce public edition boundary", None),
            ("Build platform image once for local scan and optional registry push", None),
            ("Trivy vulnerability scan", "inputs.push_to_registry"),
            ("Validate final image runtime without build-host libraries", None),
        ):
            with self.subTest(step=name):
                step = next(step for step in steps if step.get("name") == name)
                self.assertEqual(step.get("if"), condition)
        override_steps = [
            step["name"] for step in steps
            if "override_docker_source_preflight" in step.get("if", "")
        ]
        self.assertEqual(override_steps, [
            "Validate Docker dependency providers and runtime packages",
            "Report Docker source preflight override",
        ])

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

    def test_bootstrap_gate_accepts_native_bootstrap_in_older_checkout(self) -> None:
        self._run_relocated_validator(False, execute_workflow=True, bootstrap_gate=True)

    def test_bootstrap_gate_rejects_x86_binary_without_changing_source(self) -> None:
        self._run_relocated_validator(
            False, execute_workflow=True, bootstrap_gate=True, invalid_bootstrap=True,
        )

    def test_workflow_validates_older_checkout_without_validator(self) -> None:
        self._run_relocated_validator(invalid_runtime=False, execute_workflow=True)

    def test_workflow_rejects_invalid_older_checkout_without_validator(self) -> None:
        self._run_relocated_validator(invalid_runtime=True, execute_workflow=True)

    def test_release_preflight_runs_after_source_selection_before_platform_jobs(self) -> None:
        jobs = yaml.safe_load(RELEASE_WORKFLOW.read_text(encoding="utf-8"))["jobs"]
        steps = jobs["prepare-release-context"]["steps"]
        ids = [step.get("id") for step in steps]
        self.assertLess(ids.index("version"), ids.index("docker-source-preflight"))
        self.assertLess(ids.index("docker-source-preflight"), ids.index("tags"))
        self.assertIn("prepare-release-context", jobs["build-platforms"]["needs"])
        preflight = next(step for step in steps if step.get("id") == "docker-source-preflight")
        self.assertEqual(
            preflight["if"],
            "${{ inputs.push_to_registry == true || inputs.push_to_registry == 'true' }}",
        )
        self.assertEqual(
            preflight["env"]["OVERRIDE_SOURCE_PREFLIGHT"],
            "${{ inputs.override_docker_source_preflight }}",
        )
        self.assertEqual(preflight["env"]["WORKFLOW_SHA"], "${{ github.sha }}")
        self.assertEqual(preflight["env"]["THEMIS_SOURCE_ROOT"], "${{ github.workspace }}")
        subprocess.run(["bash", "-n", "-c", preflight["run"]], check=True)

    def test_release_preflight_accepts_valid_tagged_source_without_validator(self) -> None:
        self._run_relocated_validator(False, execute_workflow=True, release_preflight=True)

    def test_release_preflight_rejects_obsolete_tag_without_changing_source(self) -> None:
        self._run_relocated_validator(True, execute_workflow=True, release_preflight=True)

    def test_release_override_accepts_native_bootstrap(self) -> None:
        self._run_relocated_validator(
            False, execute_workflow=True, release_preflight=True, bootstrap_gate=True,
        )

    def test_release_override_rejects_x86_bootstrap_before_platform_dispatch(self) -> None:
        self._run_relocated_validator(
            False, execute_workflow=True, release_preflight=True,
            bootstrap_gate=True, invalid_bootstrap=True,
        )

    def test_release_override_skips_only_dependency_and_runtime_checks(self) -> None:
        self._run_relocated_validator(
            True, execute_workflow=True, release_preflight=True, bootstrap_gate=True,
        )

    def _run_relocated_validator(
        self, invalid_runtime: bool, execute_workflow: bool = False,
        release_preflight: bool = False,
        bootstrap_gate: bool = False, invalid_bootstrap: bool = False,
    ) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "source"
            (source / "docker").mkdir(parents=True)
            for name in ("Dockerfile.unified", "vcpkg-community.json"):
                shutil.copyfile(REPO_ROOT / "docker" / name, source / "docker" / name)
            manifest = source / "scripts/release/runtime-dependencies-ubuntu-24.04.txt"
            manifest.parent.mkdir(parents=True)
            shutil.copyfile(REPO_ROOT / manifest.relative_to(source), manifest)
            if invalid_bootstrap:
                dockerfile = source / "docker" / "Dockerfile.unified"
                dockerfile.write_text(
                    dockerfile.read_text(encoding="utf-8").replace(
                        "${VCPKG_ROOT}/bootstrap-vcpkg.sh -disableMetrics",
                        "install -m 0755 /tmp/vcpkg-glibc ${VCPKG_ROOT}/vcpkg",
                    ),
                    encoding="utf-8",
                )
            if invalid_runtime:
                dockerfile = source / "docker" / "Dockerfile.unified"
                dockerfile.write_text(
                    dockerfile.read_text(encoding="utf-8").replace("librocksdb8.9", "librocksdb7"),
                    encoding="utf-8",
                )
                manifest.write_text(
                    manifest.read_text(encoding="utf-8").replace("librocksdb8.9", "librocksdb7"),
                    encoding="utf-8",
                )
            validator = root / "validator.py"
            shutil.copyfile(VALIDATOR, validator)
            self.assertFalse((source / "tests").exists())
            command = ["python3", str(validator), "-v"]
            env = {**os.environ, "THEMIS_SOURCE_ROOT": str(source)}
            if execute_workflow:
                def git(directory: Path, *args: str) -> str:
                    return subprocess.run(
                        ["git", "-C", str(directory), *args],
                        check=True, capture_output=True, text=True,
                    ).stdout.strip()

                git(source, "init")
                git(source, "config", "user.name", "Regression Test")
                git(source, "config", "user.email", "test@example.invalid")
                git(source, "add", "docker", "scripts")
                git(source, "commit", "-m", "Source without validator")
                source_sha = git(source, "rev-parse", "HEAD")
                (source / "tests").mkdir()
                shutil.copyfile(VALIDATOR, source / "tests" / VALIDATOR.name)
                git(source, "add", "tests")
                git(source, "commit", "-m", "Workflow validator")
                workflow_sha = git(source, "rev-parse", "HEAD")
                if release_preflight and (invalid_runtime or invalid_bootstrap):
                    dockerfile = source / "docker" / "Dockerfile.unified"
                    shutil.copyfile(REPO_ROOT / "docker" / "Dockerfile.unified", dockerfile)
                    shutil.copyfile(REPO_ROOT / manifest.relative_to(source), manifest)
                    git(source, "add", "docker", "scripts")
                    git(source, "commit", "-m", "Fix Docker packages after release")
                    workflow_sha = git(source, "rev-parse", "HEAD")
                git(source, "tag", "v2.4.0-alpha", source_sha)

                checkout = root / "checkout"
                checkout.mkdir()
                git(checkout, "init")
                git(checkout, "remote", "add", "origin", str(source))
                git(checkout, "fetch", "--depth=1", "origin", source_sha)
                git(checkout, "checkout", "--detach", "FETCH_HEAD")
                self.assertFalse((checkout / "tests").exists())
                workflow = yaml.safe_load(WORKFLOW.read_text(encoding="utf-8"))
                validation = next(
                    step for step in workflow["jobs"]["build"]["steps"]
                    if step.get("name") == (
                        "Validate Docker vcpkg bootstrap architecture" if bootstrap_gate
                        else "Validate Docker dependency providers and runtime packages"
                    )
                )
                if release_preflight:
                    release = yaml.safe_load(RELEASE_WORKFLOW.read_text(encoding="utf-8"))
                    validation = next(
                        step for step in release["jobs"]["prepare-release-context"]["steps"]
                        if step.get("id") == "docker-source-preflight"
                    )
                    git(checkout, "fetch", "origin", workflow_sha)
                    git(checkout, "fetch", "origin", "tag", "v2.4.0-alpha")
                command = ["bash", "-c", validation["run"]]
                env.update(
                    WORKFLOW_SHA=workflow_sha,
                    THEMIS_SOURCE_ROOT=str(checkout),
                    RUNNER_TEMP=str(root),
                    RELEASE_VERSION="v2.4.0-alpha",
                    SOURCE_SHA=source_sha,
                    OVERRIDE_SOURCE_PREFLIGHT="true" if bootstrap_gate else "false",
                )
            result = subprocess.run(
                command,
                cwd=checkout if execute_workflow else root,
                env=env,
                capture_output=True,
                text=True,
            )
            if execute_workflow:
                self.assertEqual(git(checkout, "rev-parse", "HEAD"), source_sha)
                self.assertFalse((checkout / "tests").exists())
                if release_preflight:
                    self.assertEqual(
                        git(checkout, "rev-parse", "v2.4.0-alpha^{commit}"), source_sha
                    )
                    self.assertFalse(list(root.glob("docker-release-source-*.py")))
            if invalid_bootstrap:
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("builds vcpkg natively on ARM64", result.stderr)
                self.assertIn("Publish a new release tag", result.stderr)
                self.assertFalse(list(root.glob("docker-vcpkg-bootstrap-*.py")))
                if release_preflight:
                    self.assertIn("Publish a new release tag", result.stdout)
                    self.assertIn(source_sha, result.stdout)
            elif invalid_runtime and not (release_preflight and bootstrap_gate):
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("librocksdb7", result.stderr)
                if release_preflight:
                    self.assertIn("Publish a new release tag", result.stdout)
                    self.assertIn(source_sha, result.stdout)
            else:
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertIn("OK", result.stderr)


if __name__ == "__main__":
    unittest.main()
