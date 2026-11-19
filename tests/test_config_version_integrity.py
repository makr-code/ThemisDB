from __future__ import annotations

import json
import shutil
import sys
import tempfile
import unittest
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from scripts.check_config_version_integrity import (
    EXAMPLE_CONFIGS,
    PRODUCTION_PROFILES,
    SDK_VERSION_SOURCES,
    check_repository,
)


VERSION_FILES = (
    "VERSION",
    "RELEASE_TYPE",
    "CMakeLists.txt",
    "cmake/Versions.cmake",
    "vcpkg.json",
    "helm/themisdb/Chart.yaml",
    *SDK_VERSION_SOURCES,
)


class ConfigVersionIntegrityTests(unittest.TestCase):
    def make_repository_copy(self) -> tuple[tempfile.TemporaryDirectory, Path]:
        temp_dir = tempfile.TemporaryDirectory()
        root = Path(temp_dir.name)
        profiles = [
            path.relative_to(ROOT).as_posix()
            for path in (ROOT / "config").rglob("*")
            if path.is_file() and "production" in path.name.lower() and path.suffix.lower() in {".yaml", ".yml"}
        ]
        for relative in (*VERSION_FILES, *EXAMPLE_CONFIGS, *PRODUCTION_PROFILES, *profiles):
            source = ROOT / relative
            target = root / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, target)
        return temp_dir, root

    def test_shipped_examples_have_safe_defaults_and_versions_match(self) -> None:
        report = check_repository(ROOT)
        self.assertEqual(report["status"], "pass", report["errors"])
        self.assertEqual(report["config_lint"]["status"], "pass")
        self.assertEqual(report["version_matrix"]["status"], "pass")
        self.assertTrue(any(entry["component"] == "Debian CPack" for entry in report["version_matrix"]["entries"]))

        tsa = yaml.safe_load((ROOT / EXAMPLE_CONFIGS[0]).read_text(encoding="utf-8"))
        raid = yaml.safe_load((ROOT / EXAMPLE_CONFIGS[1]).read_text(encoding="utf-8"))
        replication = yaml.safe_load((ROOT / EXAMPLE_CONFIGS[2]).read_text(encoding="utf-8"))
        self.assertIs(tsa["timestamp_authority"]["verify_tsa_cert"], True)
        self.assertEqual(raid["http_server"]["host"], "127.0.0.1")
        self.assertTrue(raid["grpc"]["security"]["enable_tls"])
        self.assertTrue(raid["grpc"]["security"]["require_client_cert"])
        self.assertFalse(replication["replication"]["shipper_enabled"])

    def test_rejects_external_plaintext_replication_and_unsecured_raid(self) -> None:
        temp_dir, root = self.make_repository_copy()
        with temp_dir:
            raid_path = root / EXAMPLE_CONFIGS[1]
            raid = yaml.safe_load(raid_path.read_text(encoding="utf-8"))
            raid["grpc"]["security"]["enable_tls"] = False
            raid["grpc"]["security"]["require_client_cert"] = False
            raid_path.write_text(yaml.safe_dump(raid), encoding="utf-8")

            replication_path = root / EXAMPLE_CONFIGS[2]
            replication = yaml.safe_load(replication_path.read_text(encoding="utf-8"))
            replication["replication"]["replicas"][0]["endpoint"] = "http://replica.example.com:8766"
            replication["replication"]["ca_cert_path"] = ""
            replication_path.write_text(yaml.safe_dump(replication), encoding="utf-8")

            profile_path = root / PRODUCTION_PROFILES[0]
            profile = yaml.safe_load(profile_path.read_text(encoding="utf-8"))
            profile["replicaEndpoint"] = "http://replica.example.com:8766"
            profile_path.write_text(yaml.safe_dump(profile), encoding="utf-8")

            report = check_repository(root)
            self.assertEqual(report["status"], "fail")
            self.assertTrue(any("external gRPC bind requires TLS" in error for error in report["errors"]))
            self.assertTrue(any("external gRPC bind requires configured mTLS certificates" in error for error in report["errors"]))
            self.assertTrue(any("external replica endpoint must not use plaintext HTTP" in error for error in report["errors"]))
            self.assertTrue(any("external replica endpoint requires mTLS certificate paths" in error for error in report["errors"]))
            self.assertTrue(any("production profile contains an external plaintext HTTP URL" in error for error in report["errors"]))

    def test_rejects_mismatched_package_versions(self) -> None:
        temp_dir, root = self.make_repository_copy()
        with temp_dir:
            manifest_path = root / "vcpkg.json"
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            manifest["version"] = "9.9.9"
            manifest_path.write_text(json.dumps(manifest), encoding="utf-8")

            report = check_repository(root)
            self.assertEqual(report["status"], "fail")
            self.assertTrue(any("vcpkg version '9.9.9'" in error for error in report["errors"]))


if __name__ == "__main__":
    unittest.main()
