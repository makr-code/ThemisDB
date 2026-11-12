import hashlib
import json
import os
import shutil
import subprocess
import tempfile
import unittest
import zipfile
from pathlib import Path

import yaml


ROOT = Path(__file__).resolve().parents[1]


def workflow(name):
    return yaml.safe_load((ROOT / ".github/workflows" / name).read_text())


def step(name, job, step_id):
    return next(item for item in workflow(name)["jobs"][job]["steps"] if item.get("id") == step_id)


def run_bash(script, directory, **environment):
    output = directory / "output"
    output.write_text("")
    result = subprocess.run(
        ["bash", "-euo", "pipefail", "-c", script],
        cwd=directory,
        env={**os.environ, "GITHUB_OUTPUT": str(output), **environment},
        capture_output=True, text=True,
    )
    values = dict(line.split("=", 1) for line in output.read_text().splitlines() if "=" in line)
    return result, values


class DeploymentWorkflowTests(unittest.TestCase):
    def test_linux_bootstrap_retries_only_http_server_errors_and_fails_closed(self):
        script = step("release-build-matrix.yml", "prepare-release-matrix", "plan")["run"]
        with tempfile.TemporaryDirectory() as temp:
            result, values = run_bash(
                script, Path(temp), BUILD_MATRIX="all", OS_SELECTOR="all",
                ARCH_SELECTOR="all", EDITION="community", BUILD_TYPE="Release",
                PACKAGING="auto", DISTRIBUTION="all", RUN_NUMBER="123",
            )
        self.assertEqual(result.returncode, 0, result.stderr)
        entries = json.loads(values["matrix"])["include"]
        cases = [
            ([], 1, [], 0),
            (["curl: (22) The requested URL returned error: 502"], 2, ["5"], 0),
            (["curl: (22) The requested URL returned error: 503"], 2, ["5"], 0),
            (["curl: (22) The requested URL returned error: 504"] * 2, 3, ["5", "10"], 0),
            (["curl: (22) The requested URL returned error: 504"] * 3, 3, ["5", "10"], 37),
            (["curl: (22) The requested URL returned error: 404"], 1, [], 37),
            (["curl: (60) SSL peer certificate cannot be authenticated"], 1, [], 37),
            (["vcpkg bootstrap failed: missing compiler"], 1, [], 37),
            (["curl: (22) The requested URL returned error: 504", "checksum mismatch"],
             2, ["5"], 37),
        ]
        for entry in entries:
            if not entry["id"].startswith("linux"):
                continue
            for errors, attempts, waits, status in cases:
                with self.subTest(lane=entry["id"], errors=errors), \
                        tempfile.TemporaryDirectory(prefix="bootstrap space ") as temp:
                    directory = Path(temp)
                    tools = directory / "tools"
                    tools.mkdir()
                    for name in ["git", "sudo"]:
                        (tools / name).symlink_to("/bin/true")
                    sleep = tools / "sleep"
                    sleep.write_text('#!/bin/sh\nprintf "%s\\n" "$1" >> "$WAIT_LOG"\n')
                    sleep.chmod(0o755)
                    toolchain = directory / "vcpkg/scripts/buildsystems/vcpkg.cmake"
                    toolchain.parent.mkdir(parents=True)
                    toolchain.touch()
                    bootstrap = directory / "vcpkg/bootstrap-vcpkg.sh"
                    bootstrap.write_text(
                        '#!/bin/bash\nset -euo pipefail\n'
                        '[ "$#" -eq 1 ] && [ "$1" = "-disableMetrics" ] || exit 42\n'
                        'attempt=0\n'
                        '[ ! -f "$ATTEMPT_LOG" ] || read -r attempt < "$ATTEMPT_LOG"\n'
                        'attempt=$((attempt + 1))\n'
                        'printf "%s\\n" "$attempt" > "$ATTEMPT_LOG"\n'
                        'if [ "$attempt" -le "$FAILURES" ]; then\n'
                        '  sed -n "${attempt}p" "$ERROR_LOG" >&2\n'
                        '  exit 37\n'
                        'fi\n'
                    )
                    bootstrap.chmod(0o755)
                    downloader = directory / "scripts/download-tinyllama-hf.sh"
                    downloader.parent.mkdir()
                    downloader.write_text('touch "$GITHUB_WORKSPACE/model-step-ran"\n')
                    error_log = directory / "errors"
                    error_log.write_text("\n".join(errors) + "\n")
                    attempt_log = directory / "attempts"
                    wait_log = directory / "waits"
                    result, _ = run_bash(
                        entry["setup_command"], directory,
                        PATH=f"{tools}{os.pathsep}{os.environ['PATH']}",
                        GITHUB_WORKSPACE=str(directory), RUNNER_TEMP=str(directory),
                        FAILURES=str(len(errors)), ERROR_LOG=str(error_log),
                        ATTEMPT_LOG=str(attempt_log), WAIT_LOG=str(wait_log),
                    )
                    self.assertEqual(result.returncode, status, result.stderr)
                    self.assertEqual(int(attempt_log.read_text()), attempts)
                    self.assertEqual(wait_log.read_text().splitlines() if wait_log.exists() else [], waits)
                    self.assertEqual((directory / "model-step-ran").exists(), status == 0)
                    self.assertEqual(result.stdout.count("::warning::Transient vcpkg bootstrap"), len(waits))
                    for attempt, error in enumerate(errors[:attempts], 1):
                        bootstrap_log = directory / f"vcpkg-bootstrap-{attempt}.log"
                        self.assertTrue(bootstrap_log.is_file())
                        self.assertIn(error, bootstrap_log.read_text())

    def run_configure_recovery(self, directory, arch="X64", runner_os="Windows",
                               message="error: Download timed out.", retry_status=0,
                               initial_status=23, attempts=None):
        script = step("reusable-cmake-build.yml", "build", "configure")["run"]
        script = script.replace("${{ steps.resolve_mode.outputs.resolved_build_dir }}", "build")
        attempts = attempts or [(message, initial_status), ("", retry_status)]
        (directory / "messages").write_text("\n".join(item[0] for item in attempts) + "\n")
        (directory / "statuses").write_text("\n".join(str(item[1]) for item in attempts) + "\n")
        tools = directory / "configure-tools"
        tools.mkdir()
        sleep = tools / "sleep"
        sleep.write_text('#!/bin/sh\nprintf "%s\\n" "$1" >> "$WAIT_LOG"\n')
        sleep.chmod(0o755)
        command = (
            'attempt=0\n'
            '[ ! -f configure-attempted ] || read -r attempt < configure-attempted\n'
            'attempt=$((attempt + 1))\n'
            'printf "%s\\n" "$attempt" > configure-attempted\n'
            'if [ "$attempt" -gt 1 ]; then\n'
            '  echo retried >> configure-retries\n'
            'fi\n'
            'sed -n "${attempt}p" messages\n'
            'status=$(sed -n "${attempt}p" statuses)\n'
            'exit "${status:-99}"\n'
        )
        return run_bash(
            script, directory, WORKING_DIRECTORY=str(directory),
            RUNNER_TEMP=str(directory), RUNNER_ARCH=arch, RUNNER_OS=runner_os,
            CONFIGURE_COMMAND=command, PATH=f"{tools}{os.pathsep}{os.environ['PATH']}",
            WAIT_LOG=str(directory / "waits"),
        )[0]

    def test_configure_retries_download_timeouts_on_all_release_platforms(self):
        for arch, runner_os in [
            ("X64", "Windows"), ("X64", "Linux"),
            ("ARM64", "Linux"), ("ARM64", "macOS"),
        ]:
            with self.subTest(arch=arch, runner_os=runner_os), tempfile.TemporaryDirectory() as temp:
                directory = Path(temp)
                result = self.run_configure_recovery(directory, arch, runner_os)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual((directory / "configure-retries").read_text(), "retried\n")
                self.assertIn("retrying in 10s", result.stdout)
                self.assertEqual((directory / "waits").read_text(), "10\n")
                self.assertIn(
                    "Download timed out",
                    (directory / "cmake-build-configure-output.log").read_text(),
                )

    def test_configure_timeout_retry_is_bounded_and_preserves_final_failure(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            result = self.run_configure_recovery(
                directory, attempts=[("Download timed out", 37)] * 4,
            )
            self.assertEqual(result.returncode, 37, result.stderr)
            self.assertEqual((directory / "configure-retries").read_text(), "retried\n" * 3)
            self.assertEqual((directory / "waits").read_text().splitlines(), ["10", "30", "60"])

    def test_configure_backoff_stops_on_recovery_or_nontransient_failure(self):
        error = "error: curl operation failed with response code 504."
        for last_message, last_status in [("", 0), ("CMake Error: invalid configuration", 37)]:
            with self.subTest(last_status=last_status), tempfile.TemporaryDirectory() as temp:
                directory = Path(temp)
                result = self.run_configure_recovery(
                    directory, attempts=[(error, 23), (error, 23), (last_message, last_status)],
                )
                self.assertEqual(result.returncode, last_status, result.stderr)
                self.assertEqual((directory / "configure-retries").read_text(), "retried\n" * 2)
                self.assertEqual((directory / "waits").read_text().splitlines(), ["10", "30"])

    def test_configure_retries_vcpkg_ssl_connect_error_once(self):
        message = "error: curl operation failed with error code 35 (SSL connect error)."
        for arch, runner_os, retry_status in [
            ("X64", "Linux", 0), ("X64", "Windows", 37), ("ARM64", "macOS", 0),
        ]:
            with self.subTest(arch=arch, runner_os=runner_os), tempfile.TemporaryDirectory() as temp:
                directory = Path(temp)
                result = self.run_configure_recovery(
                    directory, arch, runner_os, message=message, retry_status=retry_status,
                )
                self.assertEqual(result.returncode, retry_status, result.stderr)
                self.assertEqual((directory / "configure-retries").read_text(), "retried\n")
                self.assertIn(message, (directory / "cmake-build-configure-output.log").read_text())

    def test_configure_retries_vcpkg_http_server_errors_once(self):
        for response_code in (502, 503, 504):
            message = f"error: curl operation failed with response code {response_code}."
            with self.subTest(response_code=response_code), tempfile.TemporaryDirectory() as temp:
                directory = Path(temp)
                result = self.run_configure_recovery(directory, message=message)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual((directory / "configure-retries").read_text(), "retried\n")
                self.assertIn(message, (directory / "cmake-build-configure-output.log").read_text())

    def test_configure_does_not_retry_success_or_unrelated_windows_failures(self):
        for initial_status, message in [
            (0, "Download timed out"),
            (0, "error: curl operation failed with error code 35 (SSL connect error)."),
            (23, "CMake Error: invalid configuration"),
            (23, "error: curl operation failed with error code 60 (SSL peer certificate cannot be authenticated)."),
            (23, "error: curl operation failed with response code 404."),
            (23, "error: building arrow:x64-windows failed with: BUILD_FAILED"),
        ]:
            with self.subTest(status=initial_status, message=message), tempfile.TemporaryDirectory() as temp:
                directory = Path(temp)
                result = self.run_configure_recovery(
                    directory, message=message, initial_status=initial_status,
                )
                self.assertEqual(result.returncode, initial_status, result.stderr)
                self.assertFalse((directory / "configure-retries").exists())

    def test_configure_retains_arm64_vcpkg_build_failure_recovery(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            result = self.run_configure_recovery(
                directory, arch="ARM64", runner_os="Linux",
                message="error: building arrow:arm64-linux failed with: BUILD_FAILED",
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual((directory / "configure-retries").read_text(), "retried\n")

    def test_all_release_lanes_stop_on_build_failure_before_packaging(self):
        script = step("release-build-matrix.yml", "prepare-release-matrix", "plan")["run"]
        with tempfile.TemporaryDirectory() as temp:
            result, values = run_bash(
                script, Path(temp), BUILD_MATRIX="all", OS_SELECTOR="all",
                ARCH_SELECTOR="all", EDITION="community", BUILD_TYPE="Release",
                PACKAGING="auto", DISTRIBUTION="all", RUN_NUMBER="123",
            )
        self.assertEqual(result.returncode, 0, result.stderr)
        entries = json.loads(values["matrix"])["include"]
        self.assertEqual(
            {entry["id"] for entry in entries},
            {"linux-amd64", "linux-arm64", "windows-amd64", "macos-arm64"},
        )
        for entry in entries:
            with self.subTest(lane=entry["id"]):
                self.assertFalse(entry.get("build_continue_on_error", False))
                self.assertFalse(entry["validation_continue_on_error"])
                self.assertIn("validate_runtime_payload.py", entry["validation_command"])

        caller = workflow("release-build-matrix.yml")["jobs"]["release-platform-matrix"]
        self.assertEqual(caller["uses"], "./.github/workflows/reusable-cmake-build.yml")
        self.assertEqual(
            caller["with"]["build_continue_on_error"],
            "${{ matrix.build_continue_on_error || false }}",
        )

    def test_reusable_build_preserves_diagnostics_but_not_packages_after_failure(self):
        steps = workflow("reusable-cmake-build.yml")["jobs"]["build"]["steps"]
        diagnostics = next(item for item in steps if item["name"] == "Upload artifacts")
        packages = next(item for item in steps if item["name"] == "Upload secondary artifacts")
        self.assertEqual(
            diagnostics["if"],
            "${{ always() && !cancelled() && inputs.artifact_name != '' && inputs.artifact_paths != '' }}",
        )
        self.assertNotIn("always()", packages["if"])
        self.assertNotIn("!cancelled()", packages["if"])
        self.assertEqual(
            diagnostics["with"]["if-no-files-found"],
            "${{ inputs.ctest_junit_path != '' && 'error' || 'ignore' }}",
        )

    def test_macos_release_uses_matching_llvm_compiler_headers_and_runtime(self):
        script = step("release-build-matrix.yml", "prepare-release-matrix", "plan")["run"]
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            result, values = run_bash(
                script, directory, BUILD_MATRIX="os", OS_SELECTOR="all",
                ARCH_SELECTOR="all", EDITION="community", BUILD_TYPE="Release",
                PACKAGING="auto", DISTRIBUTION="all", RUN_NUMBER="123",
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            entries = json.loads(values["matrix"])["include"]
            macos = next(item for item in entries if item["id"] == "macos-arm64")
            self.assertEqual(macos["runs_on"], "macos-14")
            self.assertEqual(macos["compiler"], "homebrew-llvm")
            setup = macos["setup_command"]
            self.assertIn("\nbrew install llvm\n", setup)
            self.assertIn('LLVM_PREFIX="$(brew --prefix llvm)"', setup)
            for setting in [
                "CC=${LLVM_PREFIX}/bin/clang",
                "CXX=${LLVM_PREFIX}/bin/clang++",
                "CXXFLAGS=-nostdinc++ -isystem ${LLVM_PREFIX}/include/c++/v1",
                "LDFLAGS=-L${LLVM_PREFIX}/lib/c++ -Wl,-rpath,${LLVM_PREFIX}/lib/c++",
            ]:
                self.assertIn(f'echo "{setting}"', setup)
            self.assertIn('} >> "$GITHUB_ENV"', setup)
            self.assertLess(setup.index("brew install llvm"), setup.index('LLVM_PREFIX='))
            for entry in entries:
                if entry["id"] != "macos-arm64":
                    self.assertNotIn("brew install llvm", entry["setup_command"])
                    self.assertNotEqual(entry["compiler"], "homebrew-llvm")

    def make_source(self, directory, release_type="stable"):
        scripts = directory / ".github/scripts"
        scripts.mkdir(parents=True)
        shutil.copy(ROOT / ".github/scripts/version-release-type-manager.sh", scripts)
        (directory / "VERSION").write_text("2.4.0\n")
        (directory / "RELEASE_TYPE").write_text(f"{release_type}\n")
        for arguments in [
            ["init", "-q"], ["config", "user.name", "Test"],
            ["config", "user.email", "test@example.invalid"], ["add", "."],
            ["commit", "-qm", "release"], ["tag", "v2.4.0"],
            ["tag", "minimal-v2.4.0"], ["tag", "v2.4.0-beta1"],
        ]:
            subprocess.run(["git", *arguments], cwd=directory, check=True, capture_output=True)
        sha = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=directory, text=True).strip()
        (directory / "VERSION").write_text("9.9.9\n")
        subprocess.run(["git", "commit", "-qam", "newer branch"], cwd=directory, check=True, capture_output=True)
        return sha

    def test_docker_explicit_tag_resolves_tag_commit_not_workflow_commit(self):
        script = step("release-docker-image.yml", "prepare-release-context", "version")["run"]
        for event in ["workflow_dispatch", "issue_comment"]:
            for tag, edition in [("v2.4.0", "COMMUNITY"), ("2.4.0", "COMMUNITY"),
                                 ("minimal-v2.4.0", "MINIMAL"), ("v2.4.0-beta1", "COMMUNITY")]:
                with self.subTest(event=event, tag=tag), tempfile.TemporaryDirectory() as temp:
                    directory = Path(temp)
                    sha = self.make_source(directory)
                    event_file = directory / "event.json"
                    event_file.write_text("{}")
                    result, values = run_bash(
                        script, directory, EVENT_NAME=event, GIT_REF="refs/heads/develop",
                        TAG_INPUT=tag, PUSH_INPUT="true", GITHUB_EVENT_PATH=str(event_file),
                    )
                    self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
                    self.assertEqual(values["source_sha"], sha)
                    self.assertEqual(values["edition"], edition)
                    self.assertEqual(values["version"], "v2.4.0" if tag == "2.4.0" else tag)
                    self.assertEqual(values["release_type"], "beta" if "beta" in tag else "stable")
                    self.assertEqual((directory / "VERSION").read_text().strip(), "2.4.0")

    def test_docker_tag_checkout_does_not_require_version_manager_in_tag(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            (directory / "VERSION").write_text("2.4.0\n")
            (directory / "RELEASE_TYPE").write_text("stable\n")
            for arguments in [
                ["init", "-q"], ["config", "user.name", "Test"],
                ["config", "user.email", "test@example.invalid"], ["add", "VERSION", "RELEASE_TYPE"],
                ["commit", "-qm", "historical release"], ["tag", "v2.4.0"],
            ]:
                subprocess.run(["git", *arguments], cwd=directory, check=True, capture_output=True)

            scripts = directory / ".github/scripts"
            scripts.mkdir(parents=True)
            shutil.copy(ROOT / ".github/scripts/version-release-type-manager.sh", scripts)
            subprocess.run(["git", "add", "."], cwd=directory, check=True, capture_output=True)
            subprocess.run(
                ["git", "commit", "-qm", "add version manager"], cwd=directory, check=True, capture_output=True
            )

            result, values = run_bash(
                step("release-docker-image.yml", "prepare-release-context", "version")["run"],
                directory, EVENT_NAME="workflow_dispatch", GIT_REF="refs/heads/develop",
                TAG_INPUT="v2.4.0", PUSH_INPUT="true", GITHUB_EVENT_PATH="/dev/null",
            )
            self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
            self.assertEqual(values["version"], "v2.4.0")
            self.assertEqual(values["source_sha"], subprocess.check_output(
                ["git", "rev-parse", "v2.4.0^{commit}"], cwd=directory, text=True
            ).strip())

    def test_docker_publication_rejects_missing_invalid_or_private_tags(self):
        script = step("release-docker-image.yml", "prepare-release-context", "version")["run"]
        for tag in ["", "v2.4.99", "develop", "enterprise-v2.4.0"]:
            with self.subTest(tag=tag), tempfile.TemporaryDirectory() as temp:
                directory = Path(temp)
                self.make_source(directory)
                result, _ = run_bash(
                    script, directory, EVENT_NAME="workflow_dispatch", GIT_REF="refs/heads/develop",
                    TAG_INPUT=tag, PUSH_INPUT="true", GITHUB_EVENT_PATH="/dev/null",
                )
                self.assertNotEqual(result.returncode, 0)

    def test_docker_publication_explains_missing_release_tag(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            self.make_source(directory)
            result, _ = run_bash(
                step("release-docker-image.yml", "prepare-release-context", "version")["run"],
                directory, EVENT_NAME="workflow_dispatch", GIT_REF="refs/heads/develop",
                TAG_INPUT="v2.4.99", PUSH_INPUT="true", GITHUB_EVENT_PATH="/dev/null",
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("Requested release tag 'v2.4.99' does not exist", result.stderr)

    def test_docker_dry_run_can_use_workflow_source_without_release_tag(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            self.make_source(directory)
            sha = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=directory, text=True).strip()
            result, values = run_bash(
                step("release-docker-image.yml", "prepare-release-context", "version")["run"],
                directory, EVENT_NAME="workflow_dispatch", GIT_REF="refs/heads/develop",
                TAG_INPUT="", PUSH_INPUT="true", DRY_RUN="true", GITHUB_EVENT_PATH="/dev/null",
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(values["version"], "v9.9.9")
            self.assertEqual(values["source_sha"], sha)

    def test_docker_dry_run_can_build_workflow_source_for_existing_release_tag(self):
        script = step("release-docker-image.yml", "prepare-release-context", "version")["run"]
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            tagged_sha = self.make_source(directory)
            workflow_sha = subprocess.check_output(
                ["git", "rev-parse", "HEAD"], cwd=directory, text=True
            ).strip()
            result, values = run_bash(
                script, directory, EVENT_NAME="workflow_dispatch", GIT_REF="refs/heads/develop",
                TAG_INPUT="v2.4.0", PUSH_INPUT="true", DRY_RUN="true",
                BUILD_FROM_WORKFLOW_SOURCE="true", GITHUB_EVENT_PATH="/dev/null",
            )
            self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
            self.assertEqual(values["version"], "v2.4.0")
            self.assertEqual(values["source_sha"], workflow_sha)
            self.assertNotEqual(values["source_sha"], tagged_sha)
            self.assertEqual(
                subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=directory, text=True).strip(),
                workflow_sha,
            )
            self.assertIn("dry-run only", result.stdout)

    def test_dockerhub_overwrite_builds_workflow_source_for_existing_release(self):
        script = step("release-docker-image.yml", "prepare-release-context", "version")["run"]
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            tagged_sha = self.make_source(directory)
            workflow_sha = subprocess.check_output(
                ["git", "rev-parse", "HEAD"], cwd=directory, text=True
            ).strip()
            result, values = run_bash(
                script, directory, EVENT_NAME="workflow_dispatch", GIT_REF="refs/heads/develop",
                TAG_INPUT="v2.4.0", PUSH_INPUT="true", DRY_RUN="false",
                BUILD_FROM_WORKFLOW_SOURCE="false", OVERWRITE_DOCKERHUB_IMAGE="true",
                REGISTRY="docker.io", OVERRIDE_SOURCE_PREFLIGHT="false",
                GITHUB_EVENT_PATH="/dev/null",
            )
        self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
        self.assertEqual(values["version"], "v2.4.0")
        self.assertEqual(values["source_sha"], workflow_sha)
        self.assertNotEqual(values["source_sha"], tagged_sha)
        self.assertIn("Docker Hub overwrite enabled", result.stdout)

    def test_dockerhub_overwrite_with_both_registry_is_scoped_to_dockerhub(self):
        script = step("release-docker-image.yml", "prepare-release-context", "version")["run"]
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            self.make_source(directory)
            result, values = run_bash(
                script, directory, EVENT_NAME="workflow_dispatch", GIT_REF="refs/heads/develop",
                TAG_INPUT="v2.4.0", PUSH_INPUT="true", DRY_RUN="false",
                BUILD_FROM_WORKFLOW_SOURCE="false", OVERWRITE_DOCKERHUB_IMAGE="true",
                REGISTRY="both", OVERRIDE_SOURCE_PREFLIGHT="false",
                GITHUB_EVENT_PATH="/dev/null",
            )
        self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
        self.assertEqual(values["registry"], "docker.io")
        self.assertIn("limiting publication to Docker Hub", result.stdout)

    def test_dockerhub_overwrite_requires_explicit_safe_dispatch_settings(self):
        script = step("release-docker-image.yml", "prepare-release-context", "version")["run"]
        cases = [
            {"EVENT_NAME": "workflow_call"},
            {"TAG_INPUT": ""},
            {"PUSH_INPUT": "false"},
            {"REGISTRY": "ghcr.io"},
            {"DRY_RUN": "true"},
            {"BUILD_FROM_WORKFLOW_SOURCE": "true"},
            {"OVERRIDE_SOURCE_PREFLIGHT": "true"},
        ]
        for overrides in cases:
            with self.subTest(overrides=overrides), tempfile.TemporaryDirectory() as temp:
                directory = Path(temp)
                self.make_source(directory)
                environment = {
                    "EVENT_NAME": "workflow_dispatch",
                    "GIT_REF": "refs/heads/develop",
                    "TAG_INPUT": "v2.4.0",
                    "PUSH_INPUT": "true",
                    "DRY_RUN": "false",
                    "BUILD_FROM_WORKFLOW_SOURCE": "false",
                    "OVERWRITE_DOCKERHUB_IMAGE": "true",
                    "REGISTRY": "docker.io",
                    "OVERRIDE_SOURCE_PREFLIGHT": "false",
                    "GITHUB_EVENT_PATH": "/dev/null",
                    **overrides,
                }
                result, _ = run_bash(script, directory, **environment)
                self.assertNotEqual(result.returncode, 0)

    def test_dockerhub_overwrite_policy_preserves_registry_push(self):
        script = step("release-docker-image.yml", "prepare-release-context", "push-policy")["run"]
        with tempfile.TemporaryDirectory() as temp:
            result, values = run_bash(
                script, Path(temp), PUSH_REQUESTED="true", BUILD_FROM_WORKFLOW_SOURCE="false",
                OVERWRITE_DOCKERHUB_IMAGE="true", TAG_NAME="v2.4.0", DRY_RUN="false",
                REGISTRY="docker.io", INCLUDE_TINYLLAMA="true", LLM_MODEL="tinyllama",
                LLM_MODEL_URL="", LLM_MODEL_FILE="", FORCE_DOWNLOAD="false",
            )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(values["push_requested"], "true")
        self.assertEqual(values["dry_run"], "false")
        self.assertIn("will replace those image tags", result.stdout)

    def test_docker_workflow_source_build_automatically_uses_dry_run(self):
        script = step("release-docker-image.yml", "prepare-release-context", "version")["run"]
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            self.make_source(directory, release_type="alpha")
            source_sha = subprocess.check_output(
                ["git", "rev-parse", "HEAD"], cwd=directory, text=True
            ).strip()
            result, values = run_bash(
                script, directory, EVENT_NAME="workflow_dispatch", GIT_REF="refs/heads/develop",
                TAG_INPUT="v2.4.0-alpha", PUSH_INPUT="true",
                BUILD_FROM_WORKFLOW_SOURCE="true",
                GITHUB_EVENT_PATH="/dev/null",
            )
        self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
        self.assertEqual(values["version"], "v2.4.0-alpha")
        self.assertEqual(values["source_sha"], source_sha)
        self.assertIn("automatically enable dry-run mode", result.stdout)

    def test_docker_workflow_source_build_forces_registry_push_off(self):
        script = step("release-docker-image.yml", "prepare-release-context", "push-policy")["run"]
        with tempfile.TemporaryDirectory() as temp:
            result, values = run_bash(
                script, Path(temp), PUSH_REQUESTED="true", BUILD_FROM_WORKFLOW_SOURCE="true",
                DRY_RUN="false", REGISTRY="ghcr.io", INCLUDE_TINYLLAMA="true",
                LLM_MODEL="tinyllama", LLM_MODEL_URL="", LLM_MODEL_FILE="",
                FORCE_DOWNLOAD="false",
            )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(values["push_requested"], "false")
        self.assertEqual(values["dry_run"], "true")
        self.assertIn("registry publication is disabled", result.stdout)

    def test_docker_dry_run_forces_registry_push_off(self):
        script = step("release-docker-image.yml", "prepare-release-context", "push-policy")["run"]
        with tempfile.TemporaryDirectory() as temp:
            result, values = run_bash(
                script, Path(temp), PUSH_REQUESTED="true", DRY_RUN="true",
                REGISTRY="docker.io", INCLUDE_TINYLLAMA="true", LLM_MODEL="tinyllama",
                LLM_MODEL_URL="", LLM_MODEL_FILE="", FORCE_DOWNLOAD="false",
            )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(values["push_requested"], "false")
        self.assertEqual(values["dry_run"], "true")
        self.assertIn("registry publication is disabled", result.stdout)

    def test_docker_without_tag_uses_canonical_version_and_release_type(self):
        script = step("release-docker-image.yml", "prepare-release-context", "version")["run"]
        expected_versions = {
            "stable": "v9.9.9",
            "alpha": "v9.9.9-alpha",
            "beta": "v9.9.9-beta",
            "rc": "v9.9.9-rc",
        }
        for release_type, expected_version in expected_versions.items():
            with self.subTest(release_type=release_type), tempfile.TemporaryDirectory() as temp:
                directory = Path(temp)
                self.make_source(directory, release_type=release_type)
                result, values = run_bash(
                    script, directory, EVENT_NAME="workflow_dispatch", GIT_REF="refs/heads/develop",
                    TAG_INPUT="", PUSH_INPUT="false", GITHUB_EVENT_PATH="/dev/null",
                    GITHUB_RUN_NUMBER="123",
                )
                self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
                self.assertEqual(values["version"], expected_version)
                self.assertEqual(values["release_type"], release_type)

    def test_docker_without_tag_publishing_resolves_canonical_release_tag(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            self.make_source(directory, release_type="alpha")
            source_sha = subprocess.check_output(
                ["git", "rev-parse", "HEAD"], cwd=directory, text=True
            ).strip()
            subprocess.run(
                ["git", "tag", "v9.9.9-alpha"], cwd=directory, check=True, capture_output=True
            )
            result, values = run_bash(
                step("release-docker-image.yml", "prepare-release-context", "version")["run"],
                directory, EVENT_NAME="workflow_dispatch", GIT_REF="refs/heads/develop",
                TAG_INPUT="", PUSH_INPUT="true", GITHUB_EVENT_PATH="/dev/null",
                GITHUB_RUN_NUMBER="123",
            )
            self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
            self.assertEqual(values["version"], "v9.9.9-alpha")
            self.assertEqual(values["release_type"], "alpha")
            self.assertEqual(values["source_sha"], source_sha)

    def test_docker_without_tag_generates_canonical_nightly_version(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            self.make_source(directory, release_type="nightly")
            result, values = run_bash(
                step("release-docker-image.yml", "prepare-release-context", "version")["run"],
                directory, EVENT_NAME="workflow_dispatch", GIT_REF="refs/heads/develop",
                TAG_INPUT="", PUSH_INPUT="false", GITHUB_EVENT_PATH="/dev/null",
                GITHUB_RUN_NUMBER="123",
            )
            self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
            self.assertRegex(values["version"], r"^v9\.9\.9-nightly\.\d{8}\.123$")
            self.assertEqual(values["release_type"], "nightly")

    def test_prerelease_and_minimal_docker_tags_do_not_overwrite_community_latest(self):
        script = step("release-docker-image.yml", "prepare-release-context", "tags")["run"]
        cases = [
            ("v2.4.0-beta1", "COMMUNITY", "beta", "2.4.0-beta1"),
            ("minimal-v2.4.0", "MINIMAL", "stable", "minimal-2.4.0"),
            ("minimal-v2.4.0-nightly", "MINIMAL", "nightly", "minimal-2.4.0-nightly"),
        ]
        for version, edition, channel, expected_tag in cases:
            with self.subTest(version=version), tempfile.TemporaryDirectory() as temp:
                rendered = script
                for key, value in {"version": version, "edition": edition, "release_type": channel}.items():
                    rendered = rendered.replace("${{ steps.version.outputs." + key + " }}", value)
                result, values = run_bash(
                    rendered, Path(temp), REPOSITORY="makr-code/ThemisDB", TARGET_REGISTRY_INPUT="both",
                    GIT_REF="refs/heads/develop", COMMIT_SHA="a" * 40, RUN_ID="1",
                )
                self.assertEqual(result.returncode, 0, result.stderr)
                tags = values["value"].split(",")
                self.assertIn("ghcr.io/makr-code/themisdb:" + expected_tag, tags)
                self.assertNotIn("ghcr.io/makr-code/themisdb:latest", tags)
                self.assertNotIn("ghcr.io/makr-code/themisdb:nightly", tags)

    def test_manifest_requires_both_distinct_platforms_before_docker_invocation(self):
        script = step("release-docker-image.yml", "merge-image-manifest", "manifest")["run"]
        script = script.replace("${{ needs.prepare-release-context.outputs.tag_list }}", "ghcr.io/makr-code/themisdb:2.4.0")
        for platforms, same_digest, should_pass in [
            (["linux-amd64"], False, False), (["linux-amd64", "unexpected"], False, False),
            (["linux-amd64", "linux-arm64"], False, True),
            (["linux-amd64", "linux-arm64"], True, False),
        ]:
            with self.subTest(platforms=platforms), tempfile.TemporaryDirectory() as temp:
                directory = Path(temp)
                digests = directory / "docker-digests"
                digests.mkdir()
                for index, platform in enumerate(platforms):
                    (digests / f"docker-digest-{platform}.txt").write_text(
                        f"ghcr.io/makr-code/themisdb:platform-{index}@sha256:{str(0 if same_digest else index) * 64}\n"
                    )
                binary = directory / "bin"
                binary.mkdir()
                docker = binary / "docker"
                docker.write_text('#!/bin/sh\necho called >> "$CALL_LOG"\necho \'"sha256:abc"\'\n')
                docker.chmod(0o755)
                log = directory / "calls"
                result, _ = run_bash(script, directory, PATH=f"{binary}:{os.environ['PATH']}", CALL_LOG=str(log))
                self.assertEqual(result.returncode == 0, should_pass, result.stderr + result.stdout)
                self.assertEqual(log.exists(), should_pass)

    def test_release_source_and_edition_are_propagated_to_builds(self):
        packaging = workflow("release-mainline.yml")["jobs"]
        self.assertNotIn("docker-image", packaging)
        self.assertIn("preflight-release-policy", packaging["publish-community"]["needs"])
        self.assertIn("github.event_name == 'release'", packaging["publish-community"]["if"])
        self.assertEqual(packaging["build-matrix"]["with"]["source_sha"],
                         "${{ needs.preflight-release-policy.outputs.source_sha }}")
        platforms = workflow("release-docker-image.yml")["jobs"]["build-platforms"]["with"]
        self.assertEqual(platforms["source_sha"], "${{ needs.prepare-release-context.outputs.source_sha }}")
        self.assertEqual(platforms["edition"], "${{ needs.prepare-release-context.outputs.edition }}")

    def test_packaging_resolves_release_tags_and_rejects_edition_mismatch(self):
        script = step("release-mainline.yml", "preflight-release-policy", "identity")["run"]
        for tag, edition, should_pass in [
            ("2.4.0", "community", True), ("minimal-v2.4.0", "minimal", True),
            ("minimal-v2.4.0", "community", False), ("", "community", False),
        ]:
            with self.subTest(tag=tag, edition=edition), tempfile.TemporaryDirectory() as temp:
                directory = Path(temp)
                sha = self.make_source(directory)
                result, values = run_bash(
                    script, directory, TAG_INPUT=tag, EDITION_INPUT=edition,
                    EVENT_NAME="workflow_dispatch", DRY_RUN="false",
                )
                self.assertEqual(result.returncode == 0, should_pass, result.stderr + result.stdout)
                if should_pass:
                    self.assertEqual(values["source_sha"], sha)
                    self.assertEqual(values["edition"], edition)

    def test_packaging_dispatch_without_tag_defaults_to_dry_run(self):
        workflow_text = (ROOT / ".github/workflows/release-mainline.yml").read_text()
        self.assertIn(
            "      dry_run:\n"
            "        description: 'Dry-run for dispatch (build/package/validate only; skip publish/distribution)'\n"
            "        required: false\n"
            "        default: true\n"
            "        type: boolean",
            workflow_text,
        )

        script = step("release-mainline.yml", "preflight-release-policy", "identity")["run"]
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            self.make_source(directory)
            sha = subprocess.check_output(
                ["git", "rev-parse", "HEAD"], cwd=directory, text=True
            ).strip()
            result, values = run_bash(
                script,
                directory,
                TAG_INPUT="",
                EDITION_INPUT="community",
                EVENT_NAME="workflow_dispatch",
                DRY_RUN="true",
            )

        self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
        self.assertEqual(values["tag"], "")
        self.assertEqual(values["source_sha"], sha)
        self.assertEqual(values["edition"], "community")

    def test_packaging_dispatch_can_stage_a_new_tag_from_selected_ref(self):
        script = step("release-mainline.yml", "preflight-release-policy", "identity")["run"]
        for event_name, should_pass in [("workflow_dispatch", True), ("release", False)]:
            with self.subTest(event_name=event_name), tempfile.TemporaryDirectory() as temp:
                directory = Path(temp)
                subprocess.run(["git", "init", "-q"], cwd=directory, check=True)
                subprocess.run(["git", "config", "user.name", "Test"], cwd=directory, check=True)
                subprocess.run(
                    ["git", "config", "user.email", "test@example.invalid"],
                    cwd=directory,
                    check=True,
                )
                (directory / "source.txt").write_text("selected release source\n", encoding="utf-8")
                subprocess.run(["git", "add", "."], cwd=directory, check=True)
                subprocess.run(
                    ["git", "commit", "-qm", "selected release source"],
                    cwd=directory,
                    check=True,
                )
                sha = subprocess.check_output(
                    ["git", "rev-parse", "HEAD"], cwd=directory, text=True
                ).strip()

                result, values = run_bash(
                    script,
                    directory,
                    TAG_INPUT="v2.4.0",
                    EDITION_INPUT="community",
                    EVENT_NAME=event_name,
                    DRY_RUN="false",
                )

                self.assertEqual(result.returncode == 0, should_pass, result.stderr + result.stdout)
                if should_pass:
                    self.assertEqual(values["tag"], "v2.4.0")
                    self.assertEqual(values["source_sha"], sha)
                    self.assertEqual(values["edition"], "community")

    def test_packaging_tracker_reports_staging_failures_as_failures(self):
        script = step("release-mainline.yml", "finalize-release-status", "status")["run"]
        for result in ["success", "failure", "cancelled"]:
            with self.subTest(result=result), tempfile.TemporaryDirectory() as temp:
                process, values = run_bash(
                    script, Path(temp), BUILD_RESULT="success", PACKAGE_RESULT="success",
                    VALIDATE_RESULT="success", COMMUNITY_RESULT=result, PRIVATE_RESULT="skipped",
                    CHANGELOG_RESULT="skipped", WIDGET_RESULT="skipped", LLM_RESULT="skipped",
                )
                self.assertEqual(process.returncode, 0, process.stderr)
                self.assertEqual(values["status_label"], "ci/build-ok" if result == "success" else "ci/build-failed")
                if result == "success":
                    self.assertIn("awaiting", values["status_text"])

    def test_release_approval_preserves_prerelease_suffix_and_checks_assets_before_publication(self):
        steps = workflow("release-mainline-approval.yml")["jobs"]["release_publish_from_issue"]["steps"]
        version_script = next(item["with"]["script"] for item in steps if item.get("id") == "extract_version")
        match_line = next(line for line in version_script.splitlines() if "issueBody.match" in line)
        result = subprocess.run(
            ["node", "-e", "const issueBody = '## Release Version: v2.4.0-beta1\\n';\n" +
             match_line + "\nif (versionMatch[1] !== '2.4.0-beta1') process.exit(1);"],
            capture_output=True, text=True,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        script = next(item["run"] for item in steps if item.get("id") == "publish_release")
        script = script.replace("${{ github.repository }}", "test/test")
        self.assertLess(next(index for index, item in enumerate(steps) if item.get("id") == "publish_release"),
                        next(index for index, item in enumerate(steps) if item.get("id") == "create_tracking"))
        for valid in [True, False]:
            with self.subTest(valid=valid), tempfile.TemporaryDirectory() as temp:
                directory = Path(temp)
                assets = directory / "assets"
                assets.mkdir()
                (assets / "package.zip").write_bytes(b"release bytes")
                (assets / "RELEASE_MANIFEST.txt").write_text("artifacts:\n  - package.zip\n")
                checksum = hashlib.sha256(b"release bytes").hexdigest() if valid else "0" * 64
                (assets / "SHA256SUMS.txt").write_text(f"{checksum}  package.zip\n")
                binary = directory / "bin"
                binary.mkdir()
                gh = binary / "gh"
                gh.write_text(
                    '#!/bin/sh\nif [ "$2" = "download" ]; then\n'
                    '  while [ "$1" != "--dir" ]; do shift; done\n'
                    '  cp "$ASSETS/"* "$2/"\n'
                    'elif [ "$2" = "edit" ]; then touch "$PUBLISHED"; fi\n'
                )
                gh.chmod(0o755)
                published = directory / "published"
                process, _ = run_bash(
                    script, directory, PATH=f"{binary}:{os.environ['PATH']}", VERSION="2.4.0-beta1",
                    RUNNER_TEMP=str(directory), ASSETS=str(assets), PUBLISHED=str(published),
                )
                self.assertEqual(process.returncode == 0, valid, process.stderr)
                self.assertEqual(published.exists(), valid)

    def test_winget_artifact_transfers_restore_files_in_expected_directory(self):
        jobs = workflow("release-winget.yml")["jobs"]
        uploaded = next(s for s in jobs["download-artifacts"]["steps"] if s.get("uses", "").startswith("actions/upload-artifact"))
        for job in ["generate-checksums", "generate-manifests"]:
            downloads = [s["with"] for s in jobs[job]["steps"] if s.get("uses", "").startswith("actions/download-artifact")]
            self.assertIn({"name": uploaded["with"]["name"], "path": "release-artifacts/"}, downloads)
        for job in ["validate-manifests", "create-fork-pr"]:
            download = next(s for s in jobs[job]["steps"] if s.get("uses", "").startswith("actions/download-artifact"))
            self.assertEqual(download["with"]["path"], "${{ needs.generate-manifests.outputs.manifest-dir }}")
        self.assertIn("new-winget-manifest.ps1", step("release-winget.yml", "generate-manifests", "generate")["run"])

    @unittest.skipUnless(shutil.which("pwsh"), "PowerShell is required")
    def test_winget_checksums_and_generator_handle_actual_zip_layout(self):
        for executable in ["bin/themis_server.exe", "unexpected-root/bin/themis_server.exe",
                           "missing.exe", "../bin/themis_server.exe"]:
            with self.subTest(executable=executable), tempfile.TemporaryDirectory() as temp:
                directory = Path(temp)
                archive = directory / "ThemisDB-2.4.0-Windows-x64.zip"
                with zipfile.ZipFile(archive, "w") as package:
                    package.writestr(executable, b"test payload")
                output = directory / "output"
                output.touch()
                subprocess.run(
                    ["pwsh", "-NoProfile", "-File", str(ROOT / ".github/scripts/generate-winget-checksums.ps1"),
                     "-ArtifactDir", str(directory), "-OutputDir", str(directory / "checksums")],
                    env={**os.environ, "GITHUB_OUTPUT": str(output)}, check=True, capture_output=True,
                )
                checksum = json.loads((directory / "checksums/checksums.json").read_text(encoding="utf-8-sig"))["artifacts"][0]
                self.assertEqual(checksum["sha256"].lower(), hashlib.sha256(archive.read_bytes()).hexdigest())
                self.assertIn("checksums-json=", output.read_text(encoding="utf-8-sig"))
                result = subprocess.run(
                    ["pwsh", "-NoProfile", "-File", str(ROOT / "scripts/release/new-winget-manifest.ps1"),
                     "-Version", "2.4.0", "-InstallerUrl", "https://example.invalid/" + archive.name,
                     "-InstallerSha256", checksum["sha256"], "-InstallerArchivePath", str(archive),
                     "-OutputRoot", str(directory / "manifests"), "-IncludeGermanLocale"],
                    capture_output=True, text=True,
                )
                if executable in ["missing.exe", "../bin/themis_server.exe"]:
                    self.assertNotEqual(result.returncode, 0)
                    continue
                self.assertEqual(result.returncode, 0, result.stderr)
                manifests = directory / "manifests/t/ThemisDB/ThemisDB/2.4.0"
                for filename, manifest_type in [
                    ("ThemisDB.ThemisDB.yaml", "version"),
                    ("ThemisDB.ThemisDB.installer.yaml", "installer"),
                    ("ThemisDB.ThemisDB.locale.en-US.yaml", "defaultLocale"),
                ]:
                    manifest = yaml.safe_load((manifests / filename).read_text())
                    self.assertEqual(manifest["ManifestType"], manifest_type)
                    self.assertEqual(manifest["ManifestVersion"], "1.6.0")
                    if manifest_type == "installer":
                        installer = manifest["Installers"][0]
                        self.assertEqual(installer["NestedInstallerType"], "portable")
                        self.assertEqual(installer["NestedInstallerFiles"][0]["RelativeFilePath"],
                                         executable.replace("/", "\\"))

    @unittest.skipUnless(shutil.which("pwsh"), "PowerShell is required")
    def test_winget_submission_propagates_native_pr_creation_failure(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            scripts = directory / "scripts/release"
            scripts.mkdir(parents=True)
            shutil.copy(ROOT / "scripts/release/submit-winget-pkgs.ps1", scripts)
            manifests = directory / "packaging/winget/manifests/t/ThemisDB/ThemisDB/2.4.0"
            manifests.mkdir(parents=True)
            for filename in ["ThemisDB.ThemisDB.yaml", "ThemisDB.ThemisDB.installer.yaml",
                             "ThemisDB.ThemisDB.locale.en-US.yaml"]:
                (manifests / filename).write_text("PackageVersion: 2.4.0\n")
            binary = directory / "bin"
            binary.mkdir()
            commands = {
                "winget": "#!/bin/sh\nexit 0\n",
                "gh": '#!/bin/sh\nif [ "$1 $2" = "pr create" ]; then exit 1; fi\nexit 0\n',
                "git": '#!/bin/sh\nif [ "$1" = "clone" ]; then mkdir -p "$3"; fi\nexit 0\n',
            }
            for name, command in commands.items():
                path = binary / name
                path.write_text(command)
                path.chmod(0o755)
            result = subprocess.run(
                ["pwsh", "-NoProfile", "-File", str(scripts / "submit-winget-pkgs.ps1"),
                 "-Version", "2.4.0", "-ForkOwner", "test"],
                env={**os.environ, "PATH": f"{binary}:{os.environ['PATH']}", "TEMP": str(directory),
                     "GH_TOKEN": "test-only"},
                capture_output=True, text=True,
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("WinGet pull request creation failed", result.stderr)


@unittest.skipUnless(
    shutil.which("cmake") and (
        (shutil.which("gcc") and shutil.which("g++")) or
        (shutil.which("clang") and shutil.which("clang++"))
    ),
    "CMake and GNU or Clang C/C++ compilers required",
)
class GgmlErrnoWorkaroundTests(unittest.TestCase):
    def configure(self, directory, apple, target=True, explicit_cerrno=False):
        dependencies = (ROOT / "cmake/Dependencies.cmake").read_text()
        after_fetch = dependencies.split("FetchContent_MakeAvailable(llama_cpp)", 1)[1]
        start = "if(APPLE AND TARGET ggml-base)"
        self.assertIn(start, after_fetch)
        block = start + after_fetch.split(start, 1)[1].split("endif()", 1)[0] + "endif()"
        header = "#include <cerrno>\n" if explicit_cerrno else ""
        (directory / "gguf.cpp").write_text(
            header + "#include <cstring>\nint gguf_errno() { return errno; }\n"
        )
        (directory / "helper.c").write_text("int helper(void) { return 0; }\n")
        (directory / "consumer.cpp").write_text(
            "#include <cerrno>\nint consumer_errno() { return errno; }\n"
        )
        targets = (
            "add_library(ggml-base STATIC gguf.cpp helper.c)\n"
            "add_library(consumer STATIC consumer.cpp)\n"
            "target_link_libraries(consumer PRIVATE ggml-base)\n"
        ) if target else ""
        (directory / "CMakeLists.txt").write_text(
            "cmake_minimum_required(VERSION 3.16)\nproject(errno_regression LANGUAGES C CXX)\n"
            "set(CMAKE_EXPORT_COMPILE_COMMANDS ON)\n"
            f"set(APPLE {'TRUE' if apple else 'FALSE'})\n" + targets + block + "\n"
        )
        cc, cxx = ("gcc", "g++") if shutil.which("gcc") and shutil.which("g++") else (
            "clang", "clang++"
        )
        result = subprocess.run(
            ["cmake", "-S", str(directory), "-B", str(directory / "build"),
             "-G", "Unix Makefiles", f"-DCMAKE_C_COMPILER={shutil.which(cc)}",
             f"-DCMAKE_CXX_COMPILER={shutil.which(cxx)}"],
            capture_output=True, text=True,
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def build(self, directory):
        return subprocess.run(
            ["cmake", "--build", str(directory / "build")],
            capture_output=True, text=True,
        )

    def test_apple_force_include_is_cxx_only_and_private(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            self.configure(directory, apple=True)
            result = self.build(directory)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            commands = json.loads((directory / "build/compile_commands.json").read_text())
            self.assertEqual({Path(item["file"]).name for item in commands},
                             {"gguf.cpp", "helper.c", "consumer.cpp"})
            for item in commands:
                with self.subTest(source=Path(item["file"]).name):
                    words = item["command"].split()
                    if Path(item["file"]).name == "gguf.cpp":
                        self.assertIn("-include", words)
                        self.assertEqual(words[words.index("-include") + 1], "cerrno")
                    else:
                        self.assertNotIn("-include", words)
                        self.assertNotIn("cerrno", words)

    def test_nonapple_has_no_force_include(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            self.configure(directory, apple=False, explicit_cerrno=True)
            result = self.build(directory)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            commands = json.loads((directory / "build/compile_commands.json").read_text())
            for item in commands:
                self.assertNotIn("-include", item["command"].split())
                self.assertNotIn("cerrno", item["command"].split())

    def test_apple_without_ggml_base_configures_safely(self):
        with tempfile.TemporaryDirectory() as temp:
            self.configure(Path(temp), apple=True, target=False)

    def test_missing_cerrno_fails_without_workaround(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            self.configure(directory, apple=False)
            result = self.build(directory)
            self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn("errno", result.stdout + result.stderr)


@unittest.skipUnless(
    shutil.which("cmake") and shutil.which("clang") and shutil.which("clang++"),
    "CMake and Clang C/C++ compilers required",
)
class GgmlAppleBackendWorkaroundTests(unittest.TestCase):
    def configure(self, directory, apple=True, targets=True, compiler_id="Clang",
                  supported=True, explicit_memory=False, sdk_enum=False):
        dependencies = (ROOT / "cmake/Dependencies.cmake").read_text()
        block = dependencies.split(
            "    # The pinned ggml-metal.cpp uses std::unique_ptr without including <memory>.\n", 1
        )[1].split("    # =========================================================================", 1)[0]
        header = "#include <memory>\n" if explicit_memory else ""
        (directory / "metal.cpp").write_text(
            header + "int metal() { std::unique_ptr<int> p(new int(7)); return *p; }\n"
        )
        sdk_header = (
            "enum class SDKEnum : unsigned;\n"
            "typedef enum SDKEnum : unsigned SDKEnum;\n"
        ) if sdk_enum else ""
        (directory / "cpu.cpp").write_text(sdk_header + "int cpu() { return 0; }\n")
        (directory / "blas.cpp").write_text("int blas() { return 0; }\n")
        (directory / "helper.c").write_text("int helper(void) { return 0; }\n")
        (directory / "consumer.cpp").write_text("int consumer() { return 0; }\n")
        target_definitions = (
            "add_library(ggml-metal STATIC metal.cpp helper.c)\n"
            "add_library(ggml-cpu STATIC cpu.cpp helper.c)\n"
            "add_library(ggml-blas STATIC blas.cpp helper.c)\n"
            "add_library(consumer STATIC consumer.cpp)\n"
            "target_link_libraries(consumer PRIVATE ggml-metal ggml-cpu ggml-blas)\n"
        ) if targets else ""
        (directory / "CMakeLists.txt").write_text(
            "cmake_minimum_required(VERSION 3.16)\n"
            "project(apple_backends LANGUAGES C CXX)\n"
            "set(CMAKE_EXPORT_COMPILE_COMMANDS ON)\n"
            f"set(APPLE {'TRUE' if apple else 'FALSE'})\n"
            f'set(CMAKE_CXX_COMPILER_ID "{compiler_id}")\n'
            + (f"set(_THEMIS_HAVE_ELABORATED_ENUM_BASE {'TRUE' if supported else 'FALSE'})\n"
               if supported is not None else "")
            + target_definitions + block
        )
        result = subprocess.run(
            ["cmake", "-S", str(directory), "-B", str(directory / "build"),
             "-G", "Unix Makefiles", f"-DCMAKE_C_COMPILER={shutil.which('clang')}",
             f"-DCMAKE_CXX_COMPILER={shutil.which('clang++')}"],
            capture_output=True, text=True,
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def build(self, directory):
        return subprocess.run(
            ["cmake", "--build", str(directory / "build")],
            capture_output=True, text=True,
        )

    def test_apple_backend_flags_are_private_and_cxx_only(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            self.configure(directory)
            result = self.build(directory)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            commands = json.loads((directory / "build/compile_commands.json").read_text())
            for item in commands:
                source = Path(item["file"]).name
                words = item["command"].split()
                with self.subTest(source=source):
                    self.assertEqual("-include" in words, source == "metal.cpp")
                    if source == "metal.cpp":
                        self.assertEqual(words[words.index("-include") + 1], "memory")
                    self.assertEqual(
                        "-Wno-error=elaborated-enum-base" in words,
                        source == "cpu.cpp",
                    )

    def test_nonapple_build_has_no_workaround_flags(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            self.configure(directory, apple=False, explicit_memory=True)
            result = self.build(directory)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            commands = json.loads((directory / "build/compile_commands.json").read_text())
            for item in commands:
                self.assertNotIn("-include", item["command"].split())
                self.assertNotIn("-Wno-error=elaborated-enum-base", item["command"].split())

    def test_missing_memory_fails_without_workaround(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            self.configure(directory, apple=False)
            result = self.build(directory)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("unique_ptr", result.stdout + result.stderr)

    def test_sdk_enum_error_becomes_warning_only_with_workaround(self):
        for supported in (False, True):
            with self.subTest(workaround=supported):
                with tempfile.TemporaryDirectory() as temp:
                    directory = Path(temp)
                    self.configure(directory, supported=supported, sdk_enum=True)
                    result = self.build(directory)
                    output = result.stdout + result.stderr
                    if supported:
                        self.assertEqual(result.returncode, 0, output)
                        self.assertIn("warning: non-defining declaration of enumeration", output)
                        self.assertNotIn("error:", output)
                    else:
                        self.assertNotEqual(result.returncode, 0, output)
                        self.assertIn("error: non-defining declaration of enumeration", output)
                    self.assertIn("-Welaborated-enum-base", output)

    def test_missing_backend_targets_configure_safely(self):
        with tempfile.TemporaryDirectory() as temp:
            self.configure(Path(temp), targets=False)

    def test_sdk_flag_is_detected_by_compiler_probe(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            self.configure(directory, supported=None, sdk_enum=True)
            result = self.build(directory)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            commands = json.loads((directory / "build/compile_commands.json").read_text())
            cpu = next(item for item in commands if Path(item["file"]).name == "cpu.cpp")
            self.assertIn("-Wno-error=elaborated-enum-base", cpu["command"].split())

    def test_sdk_flag_requires_supported_homebrew_clang(self):
        for compiler_id, supported in (("AppleClang", True), ("GNU", True), ("Clang", False)):
            with self.subTest(compiler=compiler_id, supported=supported):
                with tempfile.TemporaryDirectory() as temp:
                    directory = Path(temp)
                    self.configure(directory, compiler_id=compiler_id, supported=supported)
                    commands = json.loads((directory / "build/compile_commands.json").read_text())
                    for item in commands:
                        self.assertNotIn(
                            "-Wno-error=elaborated-enum-base", item["command"].split()
                        )


if __name__ == "__main__":
    unittest.main()
