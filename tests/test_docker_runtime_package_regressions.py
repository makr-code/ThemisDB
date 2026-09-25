#!/usr/bin/env python3

import re
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
DOCKERFILE = REPO_ROOT / "docker" / "Dockerfile.unified"

EXPECTED_UBUNTU_2404_RUNTIME_PACKAGES = (
    "librocksdb8.9",
    "libgrpc++1.51t64",
    "libprotobuf32t64",
    "libcurl4t64",
)
LEGACY_RUNTIME_PACKAGES = (
    "librocksdb7",
    "libgrpc++1",
    "libprotobuf32",
    "libcurl4",
)


def extract_stage(text: str, stage_name: str) -> str:
    match = re.search(
        rf"^FROM ubuntu:24\.04 AS {re.escape(stage_name)}\n(?P<body>.*?)(?=^FROM |\Z)",
        text,
        re.MULTILINE | re.DOTALL,
    )
    if match is None:
        raise AssertionError(f"Could not find Docker stage {stage_name!r}")
    return match.group("body")


def extract_installed_packages(stage_text: str, stage_name: str) -> set[str]:
    match = re.search(
        r"apt-get install -y --no-install-recommends \\\n(?P<body>.*?)(?=\s*&& \\\n\s*rm -rf /var/lib/apt/lists/\*)",
        stage_text,
        re.DOTALL,
    )
    if match is None:
        raise AssertionError(f"Could not find apt-get install package block in Docker stage {stage_name!r}")

    packages: set[str] = set()
    for line in match.group("body").splitlines():
        for token in line.strip().rstrip("\\").split():
            packages.add(token)
    return packages


class DockerRuntimePackageRegressionTests(unittest.TestCase):
    @staticmethod
    def _dockerfile_text() -> str:
        return DOCKERFILE.read_text(encoding="utf-8")

    def test_runtime_stage_uses_ubuntu_2404_runtime_packages(self) -> None:
        runtime_stage = extract_stage(self._dockerfile_text(), "runtime")
        runtime_packages = extract_installed_packages(runtime_stage, "runtime")

        for package_name in EXPECTED_UBUNTU_2404_RUNTIME_PACKAGES:
            self.assertIn(package_name, runtime_packages)

    def test_debug_stage_uses_ubuntu_2404_runtime_packages(self) -> None:
        debug_stage = extract_stage(self._dockerfile_text(), "debug")
        debug_packages = extract_installed_packages(debug_stage, "debug")

        for package_name in EXPECTED_UBUNTU_2404_RUNTIME_PACKAGES:
            self.assertIn(package_name, debug_packages)

    def test_runtime_and_debug_stages_do_not_reintroduce_legacy_runtime_packages(self) -> None:
        dockerfile_text = self._dockerfile_text()
        runtime_and_debug_packages = (
            extract_installed_packages(extract_stage(dockerfile_text, "runtime"), "runtime")
            | extract_installed_packages(extract_stage(dockerfile_text, "debug"), "debug")
        )

        for package_name in LEGACY_RUNTIME_PACKAGES:
            self.assertNotIn(package_name, runtime_and_debug_packages)


if __name__ == "__main__":
    unittest.main()
