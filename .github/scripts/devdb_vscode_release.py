#!/usr/bin/env python3
"""Fail-closed request and manifest checks for the DevDB VSIX workflow."""

import json
import os
from pathlib import Path
import re
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[2]
EXTENSION = ROOT / "tools" / "themisdb-devdb-vscode"
TAG = re.compile(r"refs/tags/devdb-v(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\Z")


def validate_request(env):
    if env.get("EVENT_NAME") != "workflow_dispatch" or env.get("REPOSITORY") != "makr-code/ThemisDB":
        raise ValueError("Only manual runs in makr-code/ThemisDB are allowed")
    if env.get("REQUEST_PUBLISH") not in ("true", "false"):
        raise ValueError("Explicit publish choice required")
    if env["REQUEST_PUBLISH"] == "true":
        if env.get("REF_PROTECTED") != "true" or not TAG.fullmatch(env.get("REF", "")):
            raise ValueError("Publishing requires a protected devdb-vMAJOR.MINOR.PATCH tag")


def validate_manifest(manifest, env):
    if manifest.get("publisher") != "makr-code":
        raise ValueError("Extension publisher must be makr-code; verify Marketplace ownership")
    if not isinstance(manifest.get("name"), str) or not re.fullmatch(r"[a-z0-9][a-z0-9-]*", manifest["name"]):
        raise ValueError("Invalid extension name")
    version = manifest.get("version")
    if not isinstance(version, str) or not re.fullmatch(r"(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)", version):
        raise ValueError("Extension version must be a stable MAJOR.MINOR.PATCH")
    if env.get("REQUEST_PUBLISH") == "true" and env.get("REF") != f"refs/tags/devdb-v{version}":
        raise ValueError("Extension version does not match the release tag")


def read_source(env):
    manifest_path = EXTENSION / "package.json"
    if not manifest_path.is_file() or not (EXTENSION / "package-lock.json").is_file():
        raise ValueError("DevDB extension prerequisite missing: integrate package.json and package-lock.json into develop first")
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    validate_manifest(manifest, env)
    if not all(manifest.get("scripts", {}).get(script) for script in ("build", "test")):
        raise ValueError("DevDB extension must provide build and focused test scripts")
    return manifest


def validate_artifact(path, env):
    source = read_source(env)
    with zipfile.ZipFile(path) as archive:
        if archive.namelist().count("extension/package.json") != 1:
            raise ValueError("VSIX must contain exactly one extension/package.json")
        packaged = json.loads(archive.read("extension/package.json"))
    validate_manifest(packaged, env)
    if any(packaged.get(key) != source.get(key) for key in ("publisher", "name", "version")):
        raise ValueError("VSIX identity differs from checked source")


def main():
    command = sys.argv[1]
    if command == "request":
        validate_request(os.environ)
    elif command == "source":
        read_source(os.environ)
    elif command == "artifact":
        validate_artifact(sys.argv[2], os.environ)
    else:
        raise ValueError("Unknown validation command")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError, KeyError, IndexError, zipfile.BadZipFile) as exc:
        print(f"DevDB release validation failed: {exc}", file=sys.stderr)
        sys.exit(1)
