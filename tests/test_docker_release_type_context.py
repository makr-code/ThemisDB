import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

import yaml


ROOT = Path(__file__).resolve().parents[1]
RELEASE = ROOT / ".github/workflows/release-docker-image.yml"
PLATFORM = ROOT / ".github/workflows/reusable-docker-image-platform.yml"


class DockerReleaseTypeContextTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.release = yaml.safe_load(RELEASE.read_text())
        cls.platform = yaml.safe_load(PLATFORM.read_text())
        cls.steps = cls.platform["jobs"]["build"]["steps"]
        cls.context_step = next(
            step for step in cls.steps
            if step.get("name") == "Apply resolved release type to Docker context"
        )
        cls.contract_step = next(
            step for step in cls.steps
            if step.get("name") == "Validate release type source contract"
        )

    def test_release_channel_is_passed_to_both_platforms_and_dry_runs(self):
        jobs = self.release["jobs"]
        caller = jobs["build-platforms"]
        self.assertEqual(caller["strategy"]["matrix"]["platform"], ["linux/amd64", "linux/arm64"])
        self.assertEqual(
            caller["with"]["release_type"],
            "${{ needs.prepare-release-context.outputs.release_type }}",
        )
        self.assertEqual(
            caller["with"]["source_sha"],
            "${{ needs.prepare-release-context.outputs.source_sha }}",
        )
        self.assertEqual(
            caller["with"]["dry_run"],
            "${{ needs.prepare-release-context.outputs.dry_run == 'true' }}",
        )
        self.assertEqual(
            jobs["prepare-release-context"]["outputs"]["release_type"],
            "${{ steps.version.outputs.release_type }}",
        )
        # PyYAML parses the YAML 1.1 key "on" as True.
        input_spec = self.platform[True]["workflow_call"]["inputs"]["release_type"]
        self.assertFalse(input_spec["required"])
        self.assertEqual(input_spec["default"], "")
        self.assertNotIn("if", self.context_step)
        self.assertNotIn("if", self.contract_step)
        self.assertLess(self.steps.index(self.contract_step), self.steps.index(self.context_step))
        self.assertEqual(self.context_step["env"]["RELEASE_TYPE"], "${{ inputs.release_type }}")
        build = next(step for step in self.steps if step.get("id") == "build-candidate")
        self.assertEqual(build["with"]["context"], ".")
        self.assertEqual(self.steps.index(self.context_step) + 1, self.steps.index(build))

    def test_selected_source_contract_fails_closed_even_with_preflight_override(self):
        files = (".dockerignore", "docker/Dockerfile.unified", "CMakeLists.txt",
                 "cmake/Versions.cmake")
        for changed in ("", *files, "docker/Dockerfile.unified.dockerignore"):
            with self.subTest(changed=changed), tempfile.TemporaryDirectory(dir=ROOT) as temp:
                root = Path(temp)
                for name in files:
                    target = root / name
                    target.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copyfile(ROOT / name, target)
                if changed == "docker/Dockerfile.unified.dockerignore":
                    (root / changed).write_text("*\n")
                elif changed:
                    with (root / changed).open("a") as source:
                        source.write("\n# historical change\n")
                (root / "shell-functions").write_text(
                    "git() {\n"
                    '  case "$1" in\n'
                    '    fetch) return 0 ;;\n'
                    '    show) cat "$TRUSTED_ROOT/${2#*:}" ;;\n'
                    '    *) return 1 ;;\n'
                    "  esac\n"
                    "}\n"
                )
                result = subprocess.run(
                    ["bash", "-euo", "pipefail", "-c", self.contract_step["run"]],
                    cwd=root, env={
                        **os.environ, "BASH_ENV": str(root / "shell-functions"),
                        "TRUSTED_ROOT": str(ROOT), "WORKFLOW_SHA": "trusted",
                        "RELEASE_TYPE": "nightly", "OVERRIDE_SOURCE_PREFLIGHT": "true",
                    },
                    text=True, capture_output=True, check=False,
                )
                self.assertEqual(result.returncode == 0, not changed, result.stderr)

    def test_strict_source_contract_applies_only_to_nightly(self):
        for channel in ("", "stable", "alpha", "beta", "rc"):
            with self.subTest(channel=channel), tempfile.TemporaryDirectory(dir=ROOT) as temp:
                result = subprocess.run(
                    ["bash", "-euo", "pipefail", "-c", self.contract_step["run"]],
                    cwd=temp, env={**os.environ, "RELEASE_TYPE": channel, "WORKFLOW_SHA": "unused"},
                    text=True, capture_output=True, check=False,
                )
                self.assertEqual(result.returncode, 0, result.stderr)

    def test_trusted_but_incomplete_docker_contract_is_rejected(self):
        files = (".dockerignore", "docker/Dockerfile.unified", "CMakeLists.txt",
                 "cmake/Versions.cmake")
        for defect in ("missing-copy", "missing-include", "later-ignore"):
            with self.subTest(defect=defect), tempfile.TemporaryDirectory(dir=ROOT) as temp:
                root = Path(temp)
                for name in files:
                    target = root / name
                    target.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copyfile(ROOT / name, target)
                if defect == "missing-copy":
                    path = root / "docker/Dockerfile.unified"
                    path.write_text(path.read_text().replace("VERSION RELEASE_TYPE LICENSE", "VERSION LICENSE"))
                else:
                    path = root / ".dockerignore"
                    text = path.read_text()
                    path.write_text(text.replace("!RELEASE_TYPE\n", "") if defect == "missing-include"
                                    else text + "\nRELEASE_*\n")
                (root / "shell-functions").write_text(
                    "git() {\n"
                    '  case "$1" in\n'
                    '    fetch) return 0 ;;\n'
                    '    show) cat "${2#*:}" ;;\n'
                    '    *) return 1 ;;\n'
                    "  esac\n"
                    "}\n"
                )
                result = subprocess.run(
                    ["bash", "-euo", "pipefail", "-c", self.contract_step["run"]],
                    cwd=root, env={
                        **os.environ, "BASH_ENV": str(root / "shell-functions"),
                        "WORKFLOW_SHA": "trusted", "RELEASE_TYPE": "nightly",
                    },
                    text=True, capture_output=True, check=False,
                )
                self.assertNotEqual(result.returncode, 0, result.stdout)

    def run_context(self, directory, release_type):
        return subprocess.run(
            ["bash", "-euo", "pipefail", "-c", self.context_step["run"]],
            cwd=directory, env={**os.environ, "RELEASE_TYPE": release_type},
            text=True, capture_output=True, check=False,
        )

    def test_channel_override_and_cmake_resolution(self):
        for channel in ("nightly", "alpha", "beta", "rc", "stable"):
            with self.subTest(channel=channel), tempfile.TemporaryDirectory(dir=ROOT) as temp:
                root = Path(temp)
                (root / "VERSION").write_text("2.4.0\n")
                (root / "RELEASE_TYPE").write_text("alpha\n")
                result = self.run_context(root, channel)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual((root / "RELEASE_TYPE").read_text(), channel + "\n")
                (root / "CMakeLists.txt").write_text(
                    "cmake_minimum_required(VERSION 3.20)\n"
                    "project(Channel NONE)\n"
                    f'include("{(ROOT / "cmake/Versions.cmake").as_posix()}")\n'
                    'file(WRITE "${CMAKE_BINARY_DIR}/channel" "${THEMIS_VERSION_TYPE}")\n'
                )
                configure = subprocess.run(
                    ["cmake", "-S", str(root), "-B", str(root / "build")],
                    capture_output=True, text=True, check=False,
                )
                self.assertEqual(configure.returncode, 0, configure.stderr)
                self.assertEqual((root / "build/channel").read_text(), channel)

    def test_nightly_tag_from_alpha_source_reaches_build_context(self):
        version_step = next(
            step for step in self.release["jobs"]["prepare-release-context"]["steps"]
            if step.get("id") == "version"
        )
        for dry_run in ("true", "false"):
            with self.subTest(dry_run=dry_run), tempfile.TemporaryDirectory(dir=ROOT) as temp:
                root = Path(temp)
                (root / "VERSION").write_text("2.4.0\n")
                (root / "RELEASE_TYPE").write_text("alpha\n")
                (root / "shell-functions").write_text(
                    "git() {\n"
                    '  case "$1" in\n'
                    '    show-ref) [[ "$4" == refs/tags/v2.4.0-nightly.20261128.1 ]] ;;\n'
                    '    rev-parse) echo 0123456789abcdef0123456789abcdef01234567 ;;\n'
                    '    checkout) [[ "$3" == 0123456789abcdef0123456789abcdef01234567 ]] ;;\n'
                    '    *) return 1 ;;\n'
                    "  esac\n"
                    "}\n"
                )
                output = root / "output"
                result = subprocess.run(
                    ["bash", "-euo", "pipefail", "-c", version_step["run"]],
                    cwd=root, env={
                        **os.environ,
                        "BASH_ENV": str(root / "shell-functions"),
                        "GITHUB_OUTPUT": str(output), "GITHUB_EVENT_PATH": "/dev/null",
                        "EVENT_NAME": "workflow_call", "GIT_REF": "refs/heads/develop",
                        "TAG_INPUT": "v2.4.0-nightly.20261128.1", "PUSH_INPUT": "true",
                        "DRY_RUN": dry_run, "BUILD_FROM_WORKFLOW_SOURCE": "false",
                        "OVERWRITE_DOCKERHUB_IMAGE": "false", "REGISTRY": "ghcr.io",
                        "OVERRIDE_SOURCE_PREFLIGHT": "false",
                    },
                    text=True, capture_output=True, check=False,
                )
                self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
                values = dict(line.split("=", 1) for line in output.read_text().splitlines())
                self.assertEqual(values["version"], "v2.4.0-nightly.20261128.1")
                self.assertEqual(values["source_sha"], "0123456789abcdef0123456789abcdef01234567")
                self.assertEqual(values["release_type"], "nightly")
                self.assertEqual((root / "RELEASE_TYPE").read_text(), "alpha\n")
                context = self.run_context(root, values["release_type"])
                self.assertEqual(context.returncode, 0, context.stderr)
                self.assertEqual((root / "RELEASE_TYPE").read_text(), "nightly\n")

    def test_omitted_override_preserves_source_and_invalid_values_fail_closed(self):
        for channel, succeeds in (("", True), ("NIGHTLY", False), ("alpha\nnightly", False),
                                  ("nightly; echo unsafe", False), ("unknown", False)):
            with self.subTest(channel=channel), tempfile.TemporaryDirectory(dir=ROOT) as temp:
                root = Path(temp)
                (root / "VERSION").write_text("2.4.0\n")
                (root / "RELEASE_TYPE").write_text("alpha\n")
                result = self.run_context(root, channel)
                self.assertEqual(result.returncode == 0, succeeds, result.stderr)
                self.assertEqual((root / "RELEASE_TYPE").read_text(), "alpha\n")

    def test_version_suffix_conflicts_fail_before_build(self):
        for version, channel, succeeds in (
            ("2.4.0-alpha", "nightly", False),
            ("2.4.0-beta", "alpha", False),
            ("2.4.0-alpha", "alpha", True),
            ("2.4.0-alpha.1", "alpha", False),
            ("2.4.0+build123", "nightly", True),
            ("bad-version", "nightly", False),
        ):
            with self.subTest(version=version, channel=channel), tempfile.TemporaryDirectory(dir=ROOT) as temp:
                root = Path(temp)
                (root / "VERSION").write_text(version + "\n")
                (root / "RELEASE_TYPE").write_text("beta\n")
                result = self.run_context(root, channel)
                self.assertEqual(result.returncode == 0, succeeds, result.stderr)
                self.assertEqual(
                    (root / "RELEASE_TYPE").read_text(),
                    (channel if succeeds else "beta") + "\n",
                )


if __name__ == "__main__":
    unittest.main()
