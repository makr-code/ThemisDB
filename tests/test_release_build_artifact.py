#!/usr/bin/env python3

import json
import hashlib
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPO_ROOT))
from scripts.ci.release_build_artifact import (
    MANIFEST_NAME,
    create_manifest,
    validate_manifest,
    validate_set,
)


def make_identity(
    source_sha: str,
    platform_id: str = "linux-amd64",
    architecture: str = "x64",
    triplet: str = "x64-linux",
) -> dict[str, object]:
    configuration: dict[str, object] = {
        "source_sha": source_sha,
        "edition": "community",
        "build_type": "Release",
        "platform": platform_id,
        "architecture": architecture,
        "compiler": "gcc",
        "triplet": triplet,
        "profile": "release-packages-v1",
        "setup_command": "setup",
        "configure_command": "configure",
        "build_command": "build",
        "package_generators": "TGZ",
    }
    config_id = hashlib.sha256(
        json.dumps(configuration, sort_keys=True, separators=(",", ":")).encode()
    ).hexdigest()
    return {
        **{field: configuration[field] for field in (
            "source_sha", "edition", "build_type", "platform", "architecture",
            "compiler", "triplet", "profile",
        )},
        "config_id": config_id,
        "configuration": configuration,
    }


class ReleaseBuildArtifactTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary_directory.cleanup)
        self.root = Path(self.temporary_directory.name)
        self.source_sha = subprocess.run(
            ["git", "rev-parse", "HEAD"], check=True, capture_output=True, text=True
        ).stdout.strip()
        self.identity = make_identity(self.source_sha)
        self.expected_identity = {
            key: value for key, value in self.identity.items() if key != "configuration"
        }

    def create_package(self, directory: Path, payload: bytes = b"package") -> None:
        directory.mkdir(parents=True, exist_ok=True)
        (directory / "themisdb-linux-amd64.tar.gz").write_bytes(payload)
        create_manifest(directory, self.identity)

    def test_manifest_binds_packages_to_build_identity_and_toolchain(self) -> None:
        package_dir = self.root / "packages"
        self.create_package(package_dir)

        manifest = validate_manifest(package_dir, self.expected_identity)

        self.assertEqual(manifest["identity"], self.expected_identity)
        self.assertTrue(manifest["producer"]["compiler_version"])
        self.assertEqual((package_dir / MANIFEST_NAME).is_file(), True)

    def test_manifest_rejects_mismatched_identity(self) -> None:
        package_dir = self.root / "packages"
        self.create_package(package_dir)

        with self.assertRaisesRegex(ValueError, "identity mismatch for source_sha"):
            validate_manifest(
                package_dir, {**self.expected_identity, "source_sha": "b" * 40}
            )

    def test_manifest_rejects_changed_package_bytes(self) -> None:
        package_dir = self.root / "packages"
        self.create_package(package_dir)
        (package_dir / "themisdb-linux-amd64.tar.gz").write_bytes(b"tampered")

        with self.assertRaisesRegex(ValueError, "checksum mismatch"):
            validate_manifest(package_dir, self.expected_identity)

    def test_validate_set_requires_exact_platforms_and_flattens_packages(self) -> None:
        artifact_root = self.root / "artifacts"
        for platform_id, architecture, triplet in (
            ("linux-amd64", "x64", "x64-linux"),
            ("linux-arm64", "arm64", "arm64-linux"),
        ):
            identity = make_identity(
                self.source_sha, platform_id, architecture, triplet
            )
            package_dir = artifact_root / f"packages-{platform_id}"
            package_dir.mkdir(parents=True)
            (package_dir / f"themisdb-{platform_id}.tar.gz").write_bytes(platform_id.encode())
            create_manifest(package_dir, identity)

        output_dir = self.root / "release"
        validate_set(
            artifact_root,
            {
                "common": {
                    "source_sha": self.source_sha,
                    "edition": "community",
                    "build_type": "Release",
                    "profile": "release-packages-v1",
                },
                "platform_identities": {
                    "linux-amd64": {
                        "architecture": "x64",
                        "compiler": "gcc",
                        "triplet": "x64-linux",
                    },
                    "linux-arm64": {
                        "architecture": "arm64",
                        "compiler": "gcc",
                        "triplet": "arm64-linux",
                    },
                },
            },
            {"linux-amd64", "linux-arm64"},
            output_dir,
        )

        self.assertEqual(
            sorted(path.name for path in output_dir.iterdir()),
            ["themisdb-linux-amd64.tar.gz", "themisdb-linux-arm64.tar.gz"],
        )
        with self.assertRaisesRegex(ValueError, "Platform artifact set mismatch"):
            validate_set(
                artifact_root,
                {
                    "common": {
                        "source_sha": self.source_sha,
                        "edition": "community",
                        "build_type": "Release",
                        "profile": "release-packages-v1",
                    },
                    "platform_identities": {
                        "linux-amd64": {
                            "architecture": "x64",
                            "compiler": "gcc",
                            "triplet": "x64-linux",
                        },
                        "linux-arm64": {
                            "architecture": "arm64",
                            "compiler": "gcc",
                            "triplet": "arm64-linux",
                        },
                    },
                },
                {"linux-amd64"},
                self.root / "wrong-platforms",
            )

    def test_manifest_rejects_unexpected_or_duplicate_package_entries(self) -> None:
        package_dir = self.root / "packages"
        self.create_package(package_dir)
        manifest_path = package_dir / MANIFEST_NAME
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        manifest["packages"].append(manifest["packages"][0])
        manifest_path.write_text(json.dumps(manifest), encoding="utf-8")

        with self.assertRaisesRegex(ValueError, "Invalid package checksum entry"):
            validate_manifest(package_dir, self.expected_identity)


if __name__ == "__main__":
    unittest.main()
