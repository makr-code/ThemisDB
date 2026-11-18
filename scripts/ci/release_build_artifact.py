#!/usr/bin/env python3
"""Create and verify provenance manifests for reusable release package artifacts."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import re
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any


MANIFEST_NAME = "release-build-manifest.json"
REQUIRED_IDENTITY_FIELDS = {
    "source_sha",
    "edition",
    "build_type",
    "platform",
    "architecture",
    "compiler",
    "triplet",
    "profile",
    "config_id",
}
PACKAGE_SUFFIXES = (".tar.gz", ".tgz", ".zip", ".deb", ".rpm", ".msi", ".dmg")


def _read_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ValueError(f"Could not read JSON file {path}: {error}") from error
    if not isinstance(value, dict):
        raise ValueError(f"Expected a JSON object in {path}")
    return value


def _identity(value: Any) -> dict[str, str]:
    if not isinstance(value, dict):
        raise ValueError("Artifact identity must be a JSON object")
    missing = REQUIRED_IDENTITY_FIELDS - value.keys()
    if missing:
        raise ValueError(f"Artifact identity is missing fields: {', '.join(sorted(missing))}")
    identity = {field: str(value[field]) for field in REQUIRED_IDENTITY_FIELDS}
    if not re.fullmatch(r"[0-9a-f]{40}", identity["source_sha"]):
        raise ValueError("source_sha must be a full 40-character lowercase Git SHA")
    if not re.fullmatch(r"[0-9a-f]{64}", identity["config_id"]):
        raise ValueError("config_id must be a 64-character lowercase SHA-256 digest")
    if any(not identity[field].strip() for field in REQUIRED_IDENTITY_FIELDS):
        raise ValueError("Artifact identity fields cannot be empty")
    return identity


def _package_files(directory: Path) -> list[Path]:
    if not directory.is_dir():
        raise ValueError(f"Package directory does not exist: {directory}")
    files = sorted(path for path in directory.iterdir() if path.name != MANIFEST_NAME)
    if any(path.is_symlink() or not path.is_file() for path in files):
        raise ValueError(f"Package directory contains a non-regular file: {directory}")
    packages = [path for path in files if path.name.endswith(PACKAGE_SUFFIXES)]
    if not packages:
        raise ValueError(f"No supported package files found in {directory}")
    return packages


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _command_output(*command: str) -> str:
    try:
        result = subprocess.run(command, check=True, capture_output=True, text=True)
    except (OSError, subprocess.CalledProcessError):
        return "unknown"
    return result.stdout.strip().splitlines()[0] if result.stdout.strip() else "unknown"


def _vcpkg_revision() -> str:
    try:
        result = subprocess.run(
            ("git", "ls-tree", "HEAD", "--", "vcpkg"),
            check=True,
            capture_output=True,
            text=True,
        )
    except (OSError, subprocess.CalledProcessError):
        return "not-used"
    fields = result.stdout.split()
    return fields[2] if len(fields) >= 3 and fields[0] == "160000" else "not-used"


def create_manifest(directory: Path, raw_identity: dict[str, Any]) -> None:
    identity = _identity(raw_identity)
    configuration = raw_identity.get("configuration")
    if not isinstance(configuration, dict):
        raise ValueError("Artifact identity must include its build configuration")
    configuration_digest = hashlib.sha256(
        json.dumps(configuration, sort_keys=True, separators=(",", ":")).encode()
    ).hexdigest()
    if configuration_digest != identity["config_id"]:
        raise ValueError("Build configuration does not match config_id")
    if any(str(configuration.get(field, "")) != identity[field] for field in REQUIRED_IDENTITY_FIELDS - {"config_id"}):
        raise ValueError("Build configuration does not match artifact identity")
    actual_sha = _command_output("git", "rev-parse", "HEAD")
    if actual_sha != identity["source_sha"]:
        raise ValueError(
            f"Checked-out source SHA {actual_sha} does not match artifact identity "
            f"{identity['source_sha']}"
        )
    packages = _package_files(directory)
    manifest = {
        "format_version": 1,
        "identity": identity,
        "configuration": configuration,
        "producer": {
            "runner": platform.platform(),
            "compiler_version": _command_output("c++", "--version"),
            "cmake_version": _command_output("cmake", "--version"),
            "vcpkg_revision": _vcpkg_revision(),
        },
        "packages": [
            {"name": package.name, "sha256": _sha256(package)} for package in packages
        ],
    }
    (directory / MANIFEST_NAME).write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )


def validate_manifest(directory: Path, expected: dict[str, Any]) -> dict[str, Any]:
    manifest_path = directory / MANIFEST_NAME
    manifest = _read_json(manifest_path)
    if manifest.get("format_version") != 1:
        raise ValueError(f"Unsupported manifest format in {manifest_path}")
    identity = _identity(manifest.get("identity"))
    configuration = manifest.get("configuration")
    if not isinstance(configuration, dict):
        raise ValueError(f"Missing build configuration in {manifest_path}")
    configuration_digest = hashlib.sha256(
        json.dumps(configuration, sort_keys=True, separators=(",", ":")).encode()
    ).hexdigest()
    if configuration_digest != identity["config_id"]:
        raise ValueError(f"Build configuration digest mismatch in {manifest_path}")
    if any(str(configuration.get(field, "")) != identity[field] for field in REQUIRED_IDENTITY_FIELDS - {"config_id"}):
        raise ValueError(f"Build configuration identity mismatch in {manifest_path}")
    for field, value in expected.items():
        if field not in REQUIRED_IDENTITY_FIELDS:
            raise ValueError(f"Unsupported expected identity field: {field}")
        if identity[field] != str(value):
            raise ValueError(
                f"Artifact identity mismatch for {field}: expected {value}, got {identity[field]}"
            )
    producer = manifest.get("producer")
    if not isinstance(producer, dict) or any(
        not producer.get(field)
        for field in ("runner", "compiler_version", "cmake_version", "vcpkg_revision")
    ):
        raise ValueError(f"Incomplete toolchain provenance in {manifest_path}")

    records = manifest.get("packages")
    if not isinstance(records, list) or not records:
        raise ValueError(f"No package checksums recorded in {manifest_path}")
    expected_hashes: dict[str, str] = {}
    for record in records:
        if not isinstance(record, dict):
            raise ValueError(f"Invalid package record in {manifest_path}")
        name, digest = record.get("name"), record.get("sha256")
        if (
            not isinstance(name, str)
            or Path(name).name != name
            or not isinstance(digest, str)
            or not re.fullmatch(r"[0-9a-f]{64}", digest)
            or name in expected_hashes
        ):
            raise ValueError(f"Invalid package checksum entry in {manifest_path}")
        expected_hashes[name] = digest

    actual_packages = {path.name: _sha256(path) for path in _package_files(directory)}
    if actual_packages != expected_hashes:
        raise ValueError(f"Package set or checksum mismatch in {directory}")
    return manifest


def validate_set(
    root: Path,
    expected: dict[str, Any],
    platforms: set[str],
    output_directory: Path,
) -> None:
    if not root.is_dir():
        raise ValueError(f"Artifact root does not exist: {root}")
    directories = [path for path in root.iterdir() if path.is_dir()]
    common_expected = expected.get("common", {})
    platform_expected = expected.get("platform_identities", {})
    if not isinstance(common_expected, dict) or not isinstance(platform_expected, dict):
        raise ValueError("Expected identities must contain common and platform_identities objects")
    manifests = []
    for directory in directories:
        if not (directory / MANIFEST_NAME).is_file():
            continue
        manifest_identity = _identity(_read_json(directory / MANIFEST_NAME).get("identity"))
        platform_id = manifest_identity["platform"]
        lane_expected = platform_expected.get(platform_id)
        if not isinstance(lane_expected, dict):
            raise ValueError(f"Unexpected platform artifact: {platform_id}")
        manifests.append(
            (directory, validate_manifest(directory, {**common_expected, **lane_expected}))
        )
    actual_platforms: set[str] = set()
    output_directory.mkdir(parents=True, exist_ok=True)
    copied_names: set[str] = set()
    for directory, manifest in manifests:
        platform_id = manifest["identity"]["platform"]
        if platform_id in actual_platforms:
            raise ValueError(f"Duplicate platform artifact: {platform_id}")
        actual_platforms.add(platform_id)
        for package in _package_files(directory):
            if package.name in copied_names:
                raise ValueError(f"Package filename collision: {package.name}")
            copied_names.add(package.name)
            shutil.copy2(package, output_directory / package.name)
    if actual_platforms != platforms:
        raise ValueError(
            f"Platform artifact set mismatch: expected {sorted(platforms)}, "
            f"got {sorted(actual_platforms)}"
        )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    create_parser = subparsers.add_parser("create")
    create_parser.add_argument("--packages-dir", type=Path, required=True)
    create_parser.add_argument("--identity-json", type=Path, required=True)
    validate_parser = subparsers.add_parser("validate-set")
    validate_parser.add_argument("--artifact-root", type=Path, required=True)
    validate_parser.add_argument("--output-dir", type=Path, required=True)
    validate_parser.add_argument("--expected-json", type=Path, required=True)
    validate_parser.add_argument("--platforms", required=True)
    single_parser = subparsers.add_parser("validate")
    single_parser.add_argument("--packages-dir", type=Path, required=True)
    single_parser.add_argument("--expected-json", type=Path, required=True)
    args = parser.parse_args()

    try:
        if args.command == "create":
            create_manifest(args.packages_dir, _read_json(args.identity_json))
        elif args.command == "validate-set":
            expected = _read_json(args.expected_json)
            validate_set(
                args.artifact_root,
                expected,
                {platform_id for platform_id in args.platforms.split(",") if platform_id},
                args.output_dir,
            )
        else:
            validate_manifest(args.packages_dir, _read_json(args.expected_json))
        return 0
    except (OSError, ValueError) as error:
        print(f"release artifact validation failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
