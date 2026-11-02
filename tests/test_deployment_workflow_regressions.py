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

    def test_docker_unpublished_dry_run_can_use_workflow_source(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            self.make_source(directory)
            sha = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=directory, text=True).strip()
            result, values = run_bash(
                step("release-docker-image.yml", "prepare-release-context", "version")["run"],
                directory, EVENT_NAME="workflow_dispatch", GIT_REF="refs/heads/develop",
                TAG_INPUT="", PUSH_INPUT="false", GITHUB_EVENT_PATH="/dev/null",
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(values["version"], "v9.9.9")
            self.assertEqual(values["source_sha"], sha)

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


if __name__ == "__main__":
    unittest.main()
