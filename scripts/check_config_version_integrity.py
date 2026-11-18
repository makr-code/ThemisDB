#!/usr/bin/env python3
"""Fail-closed lint for shipped network configuration and package versions."""

from __future__ import annotations

import argparse
import ipaddress
import json
import re
import sys
import tomllib
from pathlib import Path
from urllib.parse import urlsplit

import yaml


VERSION_PATTERN = re.compile(r"^\d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?(?:\+[0-9A-Za-z.-]+)?$")
RELEASE_TYPES = {"nightly", "alpha", "beta", "rc", "stable"}
EXAMPLE_CONFIGS = (
    "config/timestamp_authority.yaml",
    "config/raid_entity_config.example.yaml",
    "config/replication.example.yaml",
)
PRODUCTION_PROFILES = (
    "helm/themisdb/values-production.yaml",
)
SDK_VERSION_SOURCES = (
    "clients/javascript/package.json",
    "clients/javascript/package-lock.json",
    "clients/javascript/src/index.ts",
    "clients/python/pyproject.toml",
    "clients/python/themis/__init__.py",
    "clients/rust/Cargo.toml",
    "clients/rust/Cargo.lock",
    "clients/ruby/themisdb.gemspec",
    "sdks/python/themis_llm/__init__.py",
)


def _read_yaml(path: Path, errors: list[str]) -> dict | None:
    try:
        value = yaml.safe_load(path.read_text(encoding="utf-8"))
    except (OSError, yaml.YAMLError) as exc:
        errors.append(f"{path}: cannot parse YAML: {exc}")
        return None
    if not isinstance(value, dict):
        errors.append(f"{path}: expected a YAML mapping at the document root")
        return None
    return value


def _loopback_host(host: str) -> bool:
    normalized = host.strip().strip("[]").rstrip(".").lower()
    if normalized == "localhost" or normalized.endswith(".localhost"):
        return True
    try:
        return ipaddress.ip_address(normalized).is_loopback
    except ValueError:
        return False


def _endpoint(value: str) -> tuple[str, str]:
    parsed = urlsplit(value if "://" in value else f"//{value}")
    return parsed.scheme.lower(), parsed.hostname or ""


def _path_is_configured(value: object) -> bool:
    return isinstance(value, str) and bool(value.strip())


def _check_tsa_config(config: dict, path: Path, errors: list[str]) -> None:
    tsa = config.get("timestamp_authority")
    if not isinstance(tsa, dict):
        errors.append(f"{path}: missing timestamp_authority mapping")
        return
    if tsa.get("verify_tsa_cert") is not True:
        errors.append(f"{path}: timestamp_authority.verify_tsa_cert must be true")
    if tsa.get("enabled") is True:
        scheme, host = _endpoint(str(tsa.get("url", "")))
        if scheme != "https" or not host:
            errors.append(f"{path}: enabled timestamp authority must use a valid HTTPS URL")


def _check_raid_config(config: dict, path: Path, errors: list[str]) -> None:
    raid = config.get("raid", {})
    if isinstance(raid, dict) and raid.get("enabled") is True:
        default = raid.get("default", {})
        if isinstance(default, dict) and default.get("enable_quorum_enforcement") is not True:
            errors.append(f"{path}: enabled RAID requires default.enable_quorum_enforcement=true")

    grpc = config.get("grpc", {})
    if isinstance(grpc, dict) and grpc.get("enabled") is True:
        service = grpc.get("wal_apply_service", {})
        security = grpc.get("security", {})
        if isinstance(service, dict) and isinstance(security, dict):
            host = str(service.get("host", ""))
            if host and not _loopback_host(host):
                if security.get("enable_tls") is not True:
                    errors.append(f"{path}: external gRPC bind requires TLS")
                if (
                    security.get("require_client_cert") is not True
                    or not _path_is_configured(security.get("ca_cert_path"))
                    or not _path_is_configured(security.get("server_cert_path"))
                    or not _path_is_configured(security.get("server_key_path"))
                ):
                    errors.append(f"{path}: external gRPC bind requires configured mTLS certificates")

    http_server = config.get("http_server", {})
    if isinstance(http_server, dict):
        host = str(http_server.get("host", ""))
        if host and not _loopback_host(host) and http_server.get("enable_tls") is not True:
            errors.append(f"{path}: external HTTP bind requires TLS")
        if http_server.get("enable_tls") is True and (
            not _path_is_configured(http_server.get("tls_cert_path"))
            or not _path_is_configured(http_server.get("tls_key_path"))
        ):
            errors.append(f"{path}: TLS-enabled HTTP bind requires certificate and key paths")


def _check_replication_config(config: dict, path: Path, errors: list[str]) -> None:
    server = config.get("server", {})
    if isinstance(server, dict):
        host = str(server.get("host", ""))
        if host and not _loopback_host(host):
            errors.append(f"{path}: replication HTTP server must bind to loopback; no TLS server is configured")

    replication = config.get("replication", {})
    if not isinstance(replication, dict):
        errors.append(f"{path}: missing replication mapping")
        return

    cert_paths = (
        replication.get("cert_path"),
        replication.get("key_path"),
        replication.get("ca_cert_path"),
    )
    mtls_configured = all(_path_is_configured(value) for value in cert_paths)
    replicas = replication.get("replicas", [])
    if not isinstance(replicas, list):
        errors.append(f"{path}: replication.replicas must be a list")
        return
    if replication.get("shipper_enabled") is True and (
        not replicas or not mtls_configured
    ):
        errors.append(f"{path}: enabled WAL shipping requires replicas and all mTLS certificate paths")

    for replica in replicas:
        if not isinstance(replica, dict):
            errors.append(f"{path}: each replication replica must be a mapping")
            continue
        endpoint = str(replica.get("endpoint", "")).strip()
        scheme, host = _endpoint(endpoint)
        if not host:
            errors.append(f"{path}: replica endpoint must include a host")
        elif not _loopback_host(host):
            if scheme == "http":
                errors.append(f"{path}: external replica endpoint must not use plaintext HTTP")
            if scheme not in ("", "https"):
                errors.append(f"{path}: external replica endpoint has unsupported scheme {scheme!r}")
            if not mtls_configured:
                errors.append(f"{path}: external replica endpoint requires mTLS certificate paths")


def _walk_production_profile(value: object, path: Path, errors: list[str], location: str = "") -> None:
    if isinstance(value, dict):
        for key, child in value.items():
            child_location = f"{location}.{key}" if location else str(key)
            if isinstance(child, str):
                _, host = _endpoint(child)
                if key.lower() in {"bind_address", "listen_address", "bind_host"} and host:
                    if not _loopback_host(host):
                        errors.append(
                            f"{path}:{child_location}: production profile has an external bind without an explicit TLS contract"
                        )
            _walk_production_profile(child, path, errors, child_location)
    elif isinstance(value, list):
        for index, child in enumerate(value):
            _walk_production_profile(child, path, errors, f"{location}[{index}]")
    elif isinstance(value, str) and value.lower().startswith("http://"):
        scheme, host = _endpoint(value)
        if host and not _loopback_host(host):
            errors.append(f"{path}:{location}: production profile contains an external plaintext HTTP URL")


def validate_configs(root: Path) -> tuple[list[str], list[str]]:
    errors: list[str] = []
    checked: list[str] = []
    loaded: dict[str, dict] = {}
    for relative in EXAMPLE_CONFIGS:
        path = root / relative
        config = _read_yaml(path, errors)
        if config is not None:
            loaded[relative] = config
            checked.append(relative)

    tsa_path = root / EXAMPLE_CONFIGS[0]
    if EXAMPLE_CONFIGS[0] in loaded:
        _check_tsa_config(loaded[EXAMPLE_CONFIGS[0]], tsa_path, errors)
    raid_path = root / EXAMPLE_CONFIGS[1]
    if EXAMPLE_CONFIGS[1] in loaded:
        _check_raid_config(loaded[EXAMPLE_CONFIGS[1]], raid_path, errors)
    replication_path = root / EXAMPLE_CONFIGS[2]
    if EXAMPLE_CONFIGS[2] in loaded:
        _check_replication_config(loaded[EXAMPLE_CONFIGS[2]], replication_path, errors)

    profiles = sorted(
        path for path in (root / "config").rglob("*")
        if path.is_file() and "production" in path.name.lower() and path.suffix.lower() in {".yaml", ".yml"}
    )
    profiles.extend(root / relative for relative in PRODUCTION_PROFILES)
    for path in profiles:
        if not path.is_file():
            errors.append(f"{path}: required production profile is missing")
            continue
        config = _read_yaml(path, errors)
        if config is not None:
            checked.append(path.relative_to(root).as_posix())
            _walk_production_profile(config, path, errors)
    return errors, checked


def _version_entry(
    component: str,
    source: str,
    actual: str,
    expected: str,
    errors: list[str],
) -> dict[str, str]:
    status = "pass" if actual == expected else "fail"
    if status == "fail":
        errors.append(f"{source}: {component} version {actual!r} does not match expected {expected!r}")
    return {
        "component": component,
        "source": source,
        "version": actual,
        "expected": expected,
        "status": status,
    }


def _read_text(root: Path, relative: str, errors: list[str]) -> str:
    try:
        return (root / relative).read_text(encoding="utf-8")
    except OSError as exc:
        errors.append(f"{relative}: cannot read version source: {exc}")
        return ""


def validate_versions(root: Path) -> tuple[list[str], list[dict[str, str]], str, str]:
    errors: list[str] = []
    entries: list[dict[str, str]] = []
    version_text = _read_text(root, "VERSION", errors).strip()
    if not VERSION_PATTERN.fullmatch(version_text):
        errors.append(f"VERSION: invalid canonical semantic version {version_text!r}")
        return errors, entries, version_text, ""

    canonical = version_text
    numeric = ".".join(canonical.split("+", 1)[0].split("-", 1)[0].split(".")[:3])
    release_type = _read_text(root, "RELEASE_TYPE", errors).strip().lower() or "stable"
    if release_type not in RELEASE_TYPES:
        errors.append(f"RELEASE_TYPE: unsupported release type {release_type!r}")
    package_version = canonical if release_type == "stable" else f"{canonical}-{release_type}"

    versions_cmake = _read_text(root, "cmake/Versions.cmake", errors)
    root_cmake = _read_text(root, "CMakeLists.txt", errors)
    if not re.search(
        r'file\s*\(\s*READ\s+"\$\{CMAKE_SOURCE_DIR\}/VERSION"\s+THEMIS_VERSION_STRING\s*\)',
        versions_cmake,
    ):
        errors.append("cmake/Versions.cmake: CMake must read the canonical VERSION file")
    if not re.search(r"include\s*\(\s*cmake/Versions\.cmake\s*\)", root_cmake):
        errors.append("CMakeLists.txt: root build must include cmake/Versions.cmake")
    if not re.search(r"project\s*\(\s*Themis\s+VERSION\s+\$\{THEMIS_VERSION_NUMERIC\}", root_cmake):
        errors.append("CMakeLists.txt: project version must derive from THEMIS_VERSION_NUMERIC")
    entries.append(_version_entry("CMake project", "VERSION via cmake/Versions.cmake", numeric, numeric, errors))

    debian_assignment = re.search(
        r'set\s*\(\s*CPACK_DEBIAN_PACKAGE_VERSION\s+"(\$\{THEMIS_VERSION_STRING\})(\$\{_themis_package_channel_suffix\})?"\s*\)',
        root_cmake,
    )
    if not debian_assignment:
        errors.append(
            "CMakeLists.txt: Debian package version must derive from THEMIS_VERSION_STRING and the release channel"
        )
    entries.append(
        _version_entry(
            "Debian CPack",
            "CMakeLists.txt: CPACK_DEBIAN_PACKAGE_VERSION",
            package_version,
            package_version,
            errors,
        )
    )
    changelog = root / "debian" / "changelog"
    if changelog.exists():
        changelog_text = _read_text(root, "debian/changelog", errors)
        match = re.search(r"^\S+\s+\(([^)]+)\)", changelog_text, re.MULTILINE)
        if not match:
            errors.append("debian/changelog: missing a parseable first package version")
        else:
            entries.append(
                _version_entry(
                    "Debian changelog",
                    "debian/changelog",
                    match.group(1),
                    package_version,
                    errors,
                )
            )

    vcpkg_path = root / "vcpkg.json"
    try:
        vcpkg = json.loads(vcpkg_path.read_text(encoding="utf-8"))
        entries.append(_version_entry("vcpkg", "vcpkg.json", str(vcpkg["version"]), canonical, errors))
    except (OSError, json.JSONDecodeError, KeyError, TypeError) as exc:
        errors.append(f"vcpkg.json: cannot read package version: {exc}")

    chart_path = root / "helm/themisdb/Chart.yaml"
    chart = _read_yaml(chart_path, errors)
    if chart is not None:
        entries.append(
            _version_entry("Helm chart", "helm/themisdb/Chart.yaml: version",
                           str(chart.get("version", "")), canonical, errors)
        )
        entries.append(
            _version_entry("Helm application", "helm/themisdb/Chart.yaml: appVersion",
                           str(chart.get("appVersion", "")).strip('"'), canonical, errors)
        )

    for relative in SDK_VERSION_SOURCES:
        path = root / relative
        try:
            text = path.read_text(encoding="utf-8")
            if relative.endswith("package.json"):
                value = json.loads(text)["version"]
            elif relative.endswith("package-lock.json"):
                lock = json.loads(text)
                value = lock["packages"][""]["version"]
                if lock.get("version") != value:
                    errors.append(f"{relative}: top-level lock version does not match root package version")
            elif relative.endswith("src/index.ts"):
                match = re.search(r'export\s+const\s+version\s*=\s*"([^"]+)"', text)
                value = match.group(1) if match else ""
            elif relative.endswith("pyproject.toml"):
                value = tomllib.loads(text)["project"]["version"]
            elif relative.endswith("Cargo.toml"):
                value = tomllib.loads(text)["package"]["version"]
            elif relative.endswith("Cargo.lock"):
                lock = tomllib.loads(text)
                value = next(
                    package["version"] for package in lock["package"]
                    if package["name"] == "themisdb-client"
                )
            elif relative.endswith(".gemspec"):
                match = re.search(r"spec\.version\s*=\s*['\"]([^'\"]+)['\"]", text)
                value = match.group(1) if match else ""
            else:
                matches = re.findall(r"__version__\s*=\s*['\"]([^'\"]+)['\"]", text)
                value = matches[0] if matches else ""
                banner = re.search(r"Version:\s*([0-9A-Za-z.+-]+)", text)
                if banner and banner.group(1) != value:
                    errors.append(f"{relative}: module banner version {banner.group(1)!r} disagrees with __version__")
            entries.append(_version_entry(relative, relative, str(value), canonical, errors))
        except (OSError, json.JSONDecodeError, KeyError, TypeError, tomllib.TOMLDecodeError, StopIteration) as exc:
            errors.append(f"{relative}: cannot read package version: {exc}")

    return errors, entries, canonical, release_type


def check_repository(root: Path) -> dict:
    config_errors, checked_configs = validate_configs(root)
    version_errors, version_entries, canonical, release_type = validate_versions(root)
    errors = config_errors + version_errors
    return {
        "canonical_version": canonical,
        "release_type": release_type,
        "config_lint": {
            "status": "pass" if not config_errors else "fail",
            "checked_files": checked_configs,
            "errors": config_errors,
        },
        "version_matrix": {
            "status": "pass" if not version_errors else "fail",
            "entries": version_entries,
            "errors": version_errors,
        },
        "errors": errors,
        "status": "pass" if not errors else "fail",
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--matrix-output", type=Path, help="write the machine-readable integrity report to this path")
    args = parser.parse_args()

    report = check_repository(args.root.resolve())
    serialized = json.dumps(report, indent=2, sort_keys=True) + "\n"
    if args.matrix_output:
        args.matrix_output.parent.mkdir(parents=True, exist_ok=True)
        args.matrix_output.write_text(serialized, encoding="utf-8")
    if report["status"] == "pass":
        print(
            f"Config lint: PASS ({len(report['config_lint']['checked_files'])} files); "
            f"version matrix: PASS ({len(report['version_matrix']['entries'])} entries; "
            f"canonical VERSION {report['canonical_version']})"
        )
        return 0
    for error in report["errors"]:
        print(f"ERROR: {error}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
