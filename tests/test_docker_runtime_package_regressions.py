#!/usr/bin/env python3

import json
import os
import re
import subprocess
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
DOCKERFILE = REPO_ROOT / "docker" / "Dockerfile.unified"

EXPECTED_UBUNTU_2404_RUNTIME_PACKAGE_TOKENS = frozenset(
    {
        "librocksdb8.9",
        "libgrpc++1.51t64",
        "libprotobuf32t64",
        "libcurl4t64",
    }
)
LEGACY_UBUNTU_2204_RUNTIME_PACKAGE_TOKENS = frozenset(
    {
        # Exact package tokens only: the Ubuntu 24.04 Noble runtime/debug stages
        # intentionally use the distinct t64 package names instead.
        # In particular, libprotobuf32 and libprotobuf32t64 are treated as
        # different exact tokens by this regression guard.
        "librocksdb7",
        "libgrpc++1",
        "libprotobuf32",
        # Exact-token guard: keep the bare libcurl4 package token out of the
        # Noble runtime/debug stages so the Dockerfile stays pinned to the
        # explicit Ubuntu 24.04 t64 package family.
        "libcurl4",
    }
)


def extract_stage(text: str, stage_name: str, base_image: str = "ubuntu:24.04") -> str:
    match = re.search(
        rf"^FROM {re.escape(base_image)} AS {re.escape(stage_name)}\n(?P<body>.*?)(?=^FROM |\Z)",
        text,
        re.MULTILINE | re.DOTALL,
    )
    if match is None:
        raise AssertionError(f"Could not find Docker stage {stage_name!r}")
    return match.group("body")


def extract_installed_packages(stage_text: str, stage_name: str) -> set[str]:
    packages: set[str] = set()
    lines = stage_text.splitlines()

    for index, line in enumerate(lines):
        if "apt-get install" not in line:
            continue

        inline_tail = line.split("apt-get install", 1)[1].strip().rstrip("\\")
        for token in inline_tail.split():
            if not token.startswith("-"):
                packages.add(token)

        continuation_open = line.rstrip().endswith("\\")
        for candidate in lines[index + 1 :]:
            if not continuation_open:
                break

            stripped = candidate.strip()
            if not stripped:
                continuation_open = False
                break

            cleaned = stripped.rstrip("\\").strip()
            before_and = cleaned.split("&&", 1)[0].strip()
            if before_and:
                packages.update(token for token in before_and.split() if not token.startswith("-"))
            if "&&" in cleaned:
                break
            continuation_open = stripped.endswith("\\")

    if not packages:
        raise AssertionError(f"Docker stage {stage_name!r} did not yield any parsed apt package tokens")
    return packages


class DockerRuntimePackageRegressionTests(unittest.TestCase):
    @staticmethod
    def _dockerfile_text() -> str:
        return DOCKERFILE.read_text(encoding="utf-8")

    def test_community_logging_dependencies_use_system_packages(self) -> None:
        deps_packages = extract_installed_packages(
            extract_stage(self._dockerfile_text(), "deps", base_image="base"), "deps"
        )
        manifest = json.loads(
            (REPO_ROOT / "docker" / "vcpkg-community.json").read_text(encoding="utf-8")
        )
        vcpkg_dependencies = {
            dependency if isinstance(dependency, str) else dependency["name"]
            for dependency in manifest["dependencies"]
        }

        # Compiled spdlog and its fmt headers must use the same ABI/provider.
        for library in ("fmt", "spdlog"):
            with self.subTest(library=library):
                self.assertIn(f"lib{library}-dev", deps_packages)
                self.assertNotIn(library, vcpkg_dependencies)

    @unittest.skipUnless(Path("/bin/sh").is_file(), "Requires the Docker build's POSIX shell")
    def test_community_dependency_guard_rejects_mixed_logging_providers(self) -> None:
        deps_stage = extract_stage(self._dockerfile_text(), "deps", base_image="base")
        match = re.search(
            r"^RUN set -eu; \\\n(?P<body>(?:[^\n]*\\\n)*[^\n]*)",
            deps_stage,
            re.MULTILINE,
        )
        self.assertIsNotNone(match, "Missing COMMUNITY logging provider guard")
        command = match.group("body").replace("\\\n", "")

        for triplet in ("x64-linux", "arm64-linux"):
            for edition in ("COMMUNITY", "community", "ENTERPRISE"):
                for library in (None, "fmt", "spdlog"):
                    with self.subTest(triplet=triplet, edition=edition, library=library):
                        with tempfile.TemporaryDirectory() as temporary:
                            root = Path(temporary)
                            (root / "triplet.txt").write_text(triplet, encoding="utf-8")
                            if library:
                                (root / triplet / "include" / library).mkdir(parents=True)
                            result = subprocess.run(
                                [
                                    "/bin/sh", "-eu", "-c",
                                    command.replace("/build/vcpkg_installed", temporary)
                                    .replace("/tmp/triplet.txt", str(root / "triplet.txt")),
                                ],
                                env={**os.environ, "THEMIS_EDITION": edition},
                                capture_output=True,
                                text=True,
                            )
                            rejected = edition.lower() == "community" and library is not None
                            self.assertEqual(result.returncode, int(rejected), result.stderr)
                            if rejected:
                                self.assertIn(f"found vcpkg {library} for {triplet}", result.stdout)

    def test_runtime_stage_uses_ubuntu_2404_runtime_packages(self) -> None:
        runtime_stage = extract_stage(self._dockerfile_text(), "runtime")
        runtime_packages = extract_installed_packages(runtime_stage, "runtime")

        for package_name in EXPECTED_UBUNTU_2404_RUNTIME_PACKAGE_TOKENS:
            self.assertIn(package_name, runtime_packages)

    def test_debug_stage_uses_ubuntu_2404_runtime_packages(self) -> None:
        debug_stage = extract_stage(self._dockerfile_text(), "debug")
        debug_packages = extract_installed_packages(debug_stage, "debug")

        for package_name in EXPECTED_UBUNTU_2404_RUNTIME_PACKAGE_TOKENS:
            self.assertIn(package_name, debug_packages)

    def test_runtime_and_debug_stages_do_not_reintroduce_legacy_runtime_packages(self) -> None:
        dockerfile_text = self._dockerfile_text()
        runtime_packages = extract_installed_packages(extract_stage(dockerfile_text, "runtime"), "runtime")
        debug_packages = extract_installed_packages(extract_stage(dockerfile_text, "debug"), "debug")
        legacy_runtime_overlap = runtime_packages & LEGACY_UBUNTU_2204_RUNTIME_PACKAGE_TOKENS
        legacy_debug_overlap = debug_packages & LEGACY_UBUNTU_2204_RUNTIME_PACKAGE_TOKENS

        self.assertFalse(
            legacy_runtime_overlap,
            msg=(
                "Runtime stage must not reinstall Ubuntu 22.04 runtime package tokens: "
                f"{sorted(legacy_runtime_overlap)}"
            ),
        )
        self.assertFalse(
            legacy_debug_overlap,
            msg=(
                "Debug stage must not reinstall Ubuntu 22.04 runtime package tokens: "
                f"{sorted(legacy_debug_overlap)}"
            ),
        )


if __name__ == "__main__":
    unittest.main()
