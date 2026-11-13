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
    def test_release_dry_run_is_available_for_dispatch_and_workflow_calls(self) -> None:
        workflow = yaml.safe_load(RELEASE_WORKFLOW.read_text(encoding="utf-8"))
        for trigger in ("workflow_dispatch", "workflow_call"):
            with self.subTest(trigger=trigger):
                dry_run = workflow[True][trigger]["inputs"]["dry_run"]
                self.assertEqual(dry_run["type"], "boolean")
                self.assertIs(dry_run["default"], False)
                workflow_source = workflow[True][trigger]["inputs"]["build_from_workflow_source"]
                self.assertEqual(workflow_source["type"], "boolean")
                self.assertIs(workflow_source["default"], False)

        jobs = workflow["jobs"]
        version_step = next(
            step for step in jobs["prepare-release-context"]["steps"]
            if step.get("id") == "version"
        )
        self.assertEqual(
            version_step["env"]["BUILD_FROM_WORKFLOW_SOURCE"],
            "${{ inputs.build_from_workflow_source }}",
        )
        self.assertIn("automatically enable dry-run mode", version_step["run"])
        push_policy = next(
            step for step in jobs["prepare-release-context"]["steps"]
            if step.get("id") == "push-policy"
        )
        self.assertEqual(
            push_policy["env"]["DRY_RUN"],
            "${{ inputs.dry_run }}",
        )
        self.assertEqual(
            push_policy["env"]["BUILD_FROM_WORKFLOW_SOURCE"],
            "${{ inputs.build_from_workflow_source }}",
        )
        build = jobs["build-platforms"]["with"]
        self.assertEqual(
            build["dry_run"],
            "${{ needs.prepare-release-context.outputs.dry_run == 'true' }}",
        )
        self.assertIn("inputs.dry_run != true", jobs["mark-release-pending"]["if"])
        self.assertIn("inputs.dry_run != true", jobs["finalize-release-status"]["if"])
        self.assertIn("inputs.build_from_workflow_source != true", jobs["mark-release-pending"]["if"])
        self.assertIn("inputs.build_from_workflow_source != true", jobs["finalize-release-status"]["if"])
        self.assertIn(
            "inputs.tag_name != ''",
            jobs["prepare-release-context"]["steps"][
                next(
                    index for index, step in enumerate(jobs["prepare-release-context"]["steps"])
                    if step.get("id") == "docker-source-preflight"
                )
            ]["if"],
        )

    def test_reusable_dry_run_builds_and_scans_without_registry_publication(self) -> None:
        workflow = yaml.safe_load(WORKFLOW.read_text(encoding="utf-8"))
        inputs = workflow[True]["workflow_call"]["inputs"]
        self.assertEqual(inputs["dry_run"]["type"], "boolean")
        self.assertIs(inputs["dry_run"]["default"], False)

        steps = workflow["jobs"]["build"]["steps"]
        by_name = {step["name"]: step for step in steps}
        self.assertIn("!inputs.dry_run", by_name["Log in to Docker Hub"]["if"])
        self.assertEqual(
            by_name["Log in to GitHub Container Registry (OIDC)"]["if"],
            "inputs.push_to_registry && !inputs.dry_run",
        )
        build = by_name["Build platform image once for local scan and optional registry push"]["with"]
        self.assertIn("!inputs.dry_run", build["push"])
        self.assertIn("!inputs.dry_run", build["load"])
        self.assertIn("!inputs.dry_run", build["provenance"])
        self.assertEqual(by_name["Trivy vulnerability scan"]["if"], "inputs.push_to_registry || inputs.dry_run")
        self.assertEqual(
            by_name["Validate Docker vcpkg bootstrap architecture"]["if"],
            "${{ inputs.platform == 'linux/arm64' && (inputs.push_to_registry || inputs.dry_run) }}",
        )
        self.assertIn("!inputs.dry_run", by_name["Write digest artifact"]["if"])
        self.assertIn("!inputs.dry_run", by_name["Upload platform digest"]["if"])

    def test_published_candidate_is_pulled_by_digest_before_scanning(self) -> None:
        workflow = yaml.safe_load(WORKFLOW.read_text(encoding="utf-8"))
        steps = workflow["jobs"]["build"]["steps"]
        pull = next(step for step in steps if step.get("name") == "Pull pushed image locally for release-gate scan")
        self.assertEqual(pull["env"]["IMAGE_DIGEST"], "${{ steps.build-candidate.outputs.digest }}")
        self.assertLess(steps.index(pull), next(
            i for i, step in enumerate(steps) if step.get("name") == "Trivy vulnerability scan"
        ))
        for digest in ("sha256:published-manifest", ""):
            with self.subTest(digest=digest), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp)
                bash_env = root / "functions"
                bash_env.write_text('docker() { printf "%s\\n" "$*" >> "$CALLS"; }\n', encoding="utf-8")
                tag = "example.invalid/themisdb:84-linux-arm64"
                result = subprocess.run(
                    ["bash", "-euo", "pipefail", "-c", pull["run"]],
                    env={**os.environ, "BASH_ENV": str(bash_env), "CALLS": str(root / "calls"),
                         "REGISTRY_TAG": tag, "IMAGE_DIGEST": digest},
                    capture_output=True, text=True,
                )
                if digest:
                    self.assertEqual(result.returncode, 0, result.stderr)
                    calls = (root / "calls").read_text(encoding="utf-8").splitlines()
                    self.assertEqual(calls, [
                        f"pull {tag}@{digest}",
                        f"tag {tag}@{digest} {tag}",
                        f"image inspect {tag}",
                    ])
                else:
                    self.assertNotEqual(result.returncode, 0)
                    self.assertFalse((root / "calls").exists())

    def test_runtime_gate_uses_local_image_id_not_build_manifest_digest(self) -> None:
        workflow = yaml.safe_load(WORKFLOW.read_text(encoding="utf-8"))
        step = next(
            step for step in workflow["jobs"]["build"]["steps"]
            if step.get("name") == "Validate final image runtime without build-host libraries"
        )
        self.assertNotIn("IMAGE_DIGEST", step["env"])
        for platform in ("linux-amd64", "linux-arm64"):
            for inspect_ok in (True, False):
                with self.subTest(platform=platform, inspect_ok=inspect_ok), tempfile.TemporaryDirectory() as tmp:
                    root = Path(tmp)
                    bash_env = root / "functions"
                    bash_env.write_text(
                        'git() { :; }\n'
                        'docker() { printf "%s\\n" "$*" >> "$CALLS"; '
                        'if [ "$INSPECT_OK" = true ]; then echo sha256:local-image-id; '
                        'else return 1; fi; }\n'
                        'python3() { printf "%s\\n" "$*" >> "$CALLS"; }\n',
                        encoding="utf-8",
                    )
                    run = step["run"].replace("${{ steps.platform-tag.outputs.value }}", platform)
                    image = f"example.invalid/themisdb:84-{platform}"
                    result = subprocess.run(
                        ["bash", "-euo", "pipefail", "-c", run],
                        env={**os.environ, "BASH_ENV": str(bash_env), "CALLS": str(root / "calls"),
                             "INSPECT_OK": str(inspect_ok).lower(), "RUNNER_TEMP": tmp,
                             "WORKFLOW_SHA": "source-sha", "IMAGE_REF": image,
                             "IMAGE_DIGEST": "sha256:unpublished-manifest"},
                        capture_output=True, text=True,
                    )
                    calls = (root / "calls").read_text(encoding="utf-8")
                    self.assertIn(f"image inspect --format {{{{.Id}}}} {image}", calls)
                    if inspect_ok:
                        self.assertEqual(result.returncode, 0, result.stderr)
                        self.assertIn("--image sha256:local-image-id", calls)
                        self.assertNotIn("sha256:unpublished-manifest", calls)
                    else:
                        self.assertNotEqual(result.returncode, 0)
                        self.assertNotIn("--image", calls)

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
            ("Validate Docker Python autoreconf prerequisites", None),
            (
                "Validate Docker vcpkg bootstrap architecture",
                "${{ inputs.platform == 'linux/arm64' && (inputs.push_to_registry || inputs.dry_run) }}",
            ),
            ("Enforce public edition boundary", None),
            ("Build platform image once for local scan and optional registry push", None),
            ("Trivy vulnerability scan", "inputs.push_to_registry || inputs.dry_run"),
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

    def test_relocated_validator_rejects_missing_docker_configure_preset(self) -> None:
        self._run_relocated_validator(False, invalid_preset=True)

    def test_workflow_rejects_missing_docker_configure_preset(self) -> None:
        self._run_relocated_validator(False, invalid_preset=True, execute_workflow=True)

    def test_bootstrap_gate_accepts_native_bootstrap_in_older_checkout(self) -> None:
        self._run_relocated_validator(False, execute_workflow=True, bootstrap_gate=True)

    def test_bootstrap_gate_rejects_x86_binary_without_changing_source(self) -> None:
        self._run_relocated_validator(
            False, execute_workflow=True, bootstrap_gate=True, invalid_bootstrap=True,
        )

    def test_autoreconf_gate_accepts_required_packages_in_older_checkout(self) -> None:
        self._run_relocated_validator(False, execute_workflow=True, autoreconf_gate=True)

    def test_autoreconf_gate_rejects_missing_archive_without_changing_source(self) -> None:
        self._run_relocated_validator(
            False, execute_workflow=True, autoreconf_gate=True, invalid_autoreconf=True,
        )

    def test_autoreconf_gate_runs_before_buildx_without_override_condition(self) -> None:
        steps = yaml.safe_load(WORKFLOW.read_text(encoding="utf-8"))["jobs"]["build"]["steps"]
        names = [step.get("name") for step in steps]
        gate = steps[names.index("Validate Docker Python autoreconf prerequisites")]
        self.assertNotIn("if", gate)
        self.assertLess(names.index(gate["name"]), names.index("Set up Docker Buildx"))
        self.assertEqual(gate["env"]["WORKFLOW_SHA"], "${{ github.sha }}")
        self.assertEqual(gate["env"]["THEMIS_SOURCE_ROOT"], "${{ github.workspace }}")

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
            "${{ steps.version.outputs.source_is_tag == 'true' || github.ref_type == 'tag' || inputs.tag_name != '' || github.event_name == 'release' }}",
        )
        self.assertEqual(
            preflight["env"]["OVERRIDE_SOURCE_PREFLIGHT"],
            "${{ inputs.override_docker_source_preflight }}",
        )
        self.assertEqual(preflight["env"]["WORKFLOW_SHA"], "${{ github.sha }}")
        self.assertEqual(preflight["env"]["THEMIS_SOURCE_ROOT"], "${{ github.workspace }}")
        subprocess.run(["bash", "-n", "-c", preflight["run"]], check=True)

    def test_release_source_preflight_covers_tagged_prereleases_without_publication(self) -> None:
        jobs = yaml.safe_load(RELEASE_WORKFLOW.read_text(encoding="utf-8"))["jobs"]
        steps = jobs["prepare-release-context"]["steps"]
        preflight = next(step for step in steps if step.get("id") == "docker-source-preflight")
        warning = next(
            step for step in steps
            if step.get("name") == "Report Docker source preflight override"
        )

        for step in (preflight, warning):
            with self.subTest(step=step.get("id", step.get("name"))):
                self.assertNotIn("release_type", step["if"])
                self.assertNotIn("push_to_registry", step["if"])
                self.assertNotIn("dry_run", step["if"])
                self.assertIn("github.ref_type == 'tag'", step["if"])
                self.assertIn("inputs.tag_name != ''", step["if"])
                self.assertIn("github.event_name == 'release'", step["if"])
                self.assertIn("steps.version.outputs.source_is_tag == 'true'", step["if"])

        condition = preflight["if"].removeprefix("${{ ").removesuffix(" }}")
        condition = condition.replace("steps.version.outputs.source_is_tag", '"$SOURCE_IS_TAG"')
        condition = condition.replace("github.ref_type", '"$REF_TYPE"')
        condition = condition.replace("inputs.tag_name", '"$TAG_NAME"')
        condition = condition.replace("github.event_name", '"$EVENT_NAME"')
        for channel in ("stable", "alpha", "beta", "rc", "nightly"):
            tag = "v2.4.0" if channel == "stable" else f"v2.4.0-{channel}"
            for ref_type, tag_name, event, source_is_tag, expected in (
                ("branch", tag, "workflow_dispatch", "false", True),
                ("tag", "", "workflow_dispatch", "true", True),
                ("branch", "", "release", "true", True),
                ("branch", "", "workflow_dispatch", "true", True),
                ("branch", "", "workflow_call", "true", True),
                ("branch", "", "workflow_dispatch", "false", False),
                ("branch", "", "workflow_call", "false", False),
            ):
                with self.subTest(channel=channel, ref_type=ref_type, tag=tag_name,
                                  event=event, source_is_tag=source_is_tag):
                    result = subprocess.run(
                        ["bash", "-c", f"[[ {condition} ]]"],
                        env={**os.environ, "REF_TYPE": ref_type, "TAG_NAME": tag_name,
                             "EVENT_NAME": event, "SOURCE_IS_TAG": source_is_tag},
                        capture_output=True, text=True,
                    )
                    self.assertEqual(result.returncode, 0 if expected else 1, result.stderr)

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

    def test_release_override_rejects_missing_python_autoreconf_prerequisites(self) -> None:
        self._run_relocated_validator(
            False, execute_workflow=True, release_preflight=True,
            bootstrap_gate=True, invalid_autoreconf=True,
        )

    def _run_relocated_validator(
        self, invalid_runtime: bool, execute_workflow: bool = False,
        release_preflight: bool = False,
        bootstrap_gate: bool = False, invalid_bootstrap: bool = False,
        invalid_preset: bool = False,
        autoreconf_gate: bool = False, invalid_autoreconf: bool = False,
    ) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "source"
            (source / "docker").mkdir(parents=True)
            for name in ("Dockerfile.unified", "vcpkg-community.json"):
                shutil.copyfile(REPO_ROOT / "docker" / name, source / "docker" / name)
            for name in ("Dockerfile", "docker-compose.yml"):
                shutil.copyfile(REPO_ROOT / name, source / name)
            shutil.copyfile(REPO_ROOT / ".dockerignore", source / ".dockerignore")
            shutil.copyfile(REPO_ROOT / "CMakePresets.json", source / "CMakePresets.json")
            shutil.copyfile(REPO_ROOT / ".dockerignore", source / ".dockerignore")
            for directory in ("docs", "ai_context"):
                (source / directory).mkdir()
                (source / directory / ".gitkeep").touch()
            if invalid_autoreconf:
                dockerfile = source / "docker" / "Dockerfile.unified"
                dockerfile.write_text(
                    dockerfile.read_text(encoding="utf-8").replace(" autoconf-archive", ""),
                    encoding="utf-8",
                )
            if invalid_preset:
                dockerfile = source / "docker" / "Dockerfile.unified"
                dockerfile.write_text(
                    dockerfile.read_text(encoding="utf-8").replace(
                        "cmake --preset linux-release", "cmake --preset docker-release",
                    ),
                    encoding="utf-8",
                )
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
                git(source, "add", "Dockerfile", "docker-compose.yml", "docker",
                    "scripts", "CMakePresets.json", ".dockerignore", "docs", "ai_context")
                git(source, "commit", "-m", "Source without validator")
                source_sha = git(source, "rev-parse", "HEAD")
                (source / "tests").mkdir()
                shutil.copyfile(VALIDATOR, source / "tests" / VALIDATOR.name)
                git(source, "add", "tests")
                git(source, "commit", "-m", "Workflow validator")
                workflow_sha = git(source, "rev-parse", "HEAD")
                if release_preflight and (invalid_runtime or invalid_bootstrap or invalid_autoreconf):
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
                        "Validate Docker Python autoreconf prerequisites" if autoreconf_gate
                        else "Validate Docker vcpkg bootstrap architecture" if bootstrap_gate
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
            if autoreconf_gate:
                self.assertFalse(list(root.glob("docker-autoreconf-*.py")))
            if invalid_autoreconf:
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("autoconf-archive", result.stderr)
                self.assertIn("Publish a new release tag", result.stdout)
                if release_preflight:
                    self.assertIn(source_sha, result.stdout)
            elif invalid_preset:
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("Docker configure preset 'docker-release' does not exist", result.stderr)
            elif invalid_bootstrap:
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("builds vcpkg natively on ARM64", result.stderr)
                self.assertIn("Publish a new release tag", result.stderr)
                self.assertFalse(list(root.glob("docker-vcpkg-bootstrap-*.py")))
                if release_preflight:
                    self.assertIn("Publish a new release tag", result.stdout)
                    self.assertIn(source_sha, result.stdout)
                    self.assertIn("build_from_workflow_source=true", result.stdout)
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
