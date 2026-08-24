#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock


REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPTS_DIR = REPO_ROOT / "scripts"

if str(SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPTS_DIR))

import check_vcpkg_licenses as audit  # noqa: E402


class VcpkgLicenseAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.policy = audit.load_policy(REPO_ROOT / ".license-policy.json")

    def test_dual_license_expression_uses_allowed_branch(self) -> None:
        action, reason = audit.evaluate_license_expression("GPL-2.0-only OR Apache-2.0", self.policy)
        self.assertEqual(action, "allow")
        self.assertIn("allowed", reason.lower())

    def test_unknown_license_follows_block_policy(self) -> None:
        action, reason = audit.evaluate_license_expression("LicenseRef-proprietary-internal", self.policy)
        self.assertEqual(action, "warn")
        self.assertIn("manual review required", reason.lower())

    def test_platform_expression_matches_linux_target(self) -> None:
        self.assertTrue(audit.evaluate_platform_expression("linux | osx", {"linux", "x64"}))
        self.assertFalse(audit.evaluate_platform_expression("windows & !uwp", {"linux", "x64"}))

    def test_repo_manifest_requires_builtin_baseline(self) -> None:
        manifest = json.loads((REPO_ROOT / "vcpkg.json").read_text(encoding="utf-8"))
        self.assertIn("builtin-baseline", manifest)
        self.assertRegex(manifest["builtin-baseline"], r"^[0-9a-f]{40}$")

    def test_dependency_walk_includes_default_and_feature_dependencies(self) -> None:
        manifest = {
            "builtin-baseline": "test-baseline",
            "dependencies": [
                {
                    "name": "alpha",
                    "features": ["feature-x"],
                }
            ],
            "features": {
                "optional-y": {
                    "dependencies": ["beta"]
                }
            },
        }

        ports = {
            "alpha": {
                "name": "alpha",
                "version": "1.0.0",
                "license": "MIT",
                "default-features": ["default-z"],
                "dependencies": [],
                "features": {
                    "default-z": {"dependencies": ["gamma"]},
                    "feature-x": {"dependencies": ["delta"]},
                },
            },
            "beta": {
                "name": "beta",
                "version": "1.0.0",
                "license": "Apache-2.0",
                "dependencies": [],
            },
            "gamma": {
                "name": "gamma",
                "version": "1.0.0",
                "license": "BSD-3-Clause",
                "dependencies": [],
            },
            "delta": {
                "name": "delta",
                "version": "1.0.0",
                "license": "MIT",
                "dependencies": [],
            },
        }

        def fake_fetch(port_name: str, *, baseline: str, ports_dir: Path | None, cache: dict[str, dict]):
            self.assertEqual(baseline, "test-baseline")
            return ports[port_name]

        with mock.patch.object(audit, "fetch_port_manifest", side_effect=fake_fetch):
            packages = audit.build_package_records(
                manifest,
                self.policy,
                baseline="test-baseline",
                ports_dir=REPO_ROOT,
                platform_tags={"linux", "x64"},
                include_host_deps=False,
            )

        names = {pkg["name"] for pkg in packages}
        self.assertEqual(names, {"alpha", "beta", "gamma", "delta"})

        alpha = next(pkg for pkg in packages if pkg["name"] == "alpha")
        self.assertEqual(alpha["selected_features"], ["feature-x"])
        self.assertTrue(alpha["default_features_enabled"])
        self.assertTrue(alpha["direct"])

    def test_report_writer_emits_expected_artifacts(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            tmp_path = Path(temp_dir)
            manifest_path = tmp_path / "vcpkg.json"
            manifest_path.write_text(json.dumps({"builtin-baseline": "abc123"}), encoding="utf-8")

            report_paths = audit.write_reports(
                output_dir=tmp_path / "reports",
                manifest_path=manifest_path,
                baseline="abc123",
                policy=self.policy,
                packages=[
                    {
                        "name": "openssl",
                        "version": "3.0.0",
                        "port_version": None,
                        "license_expression": "Apache-2.0",
                        "action": "allow",
                        "reason": "Apache-2.0 is on the allowed SPDX list",
                        "direct": True,
                        "requested_by": ["root dependency"],
                        "selected_features": [],
                        "default_features_enabled": True,
                        "supports": None,
                        "homepage": "https://example.invalid",
                    }
                ],
            )

            self.assertTrue(report_paths["json"].exists())
            self.assertTrue(report_paths["markdown"].exists())
            self.assertIn(
                "Pull requests must not merge while this workflow is failing.",
                report_paths["markdown"].read_text(encoding="utf-8"),
            )

    def test_policy_exception_downgrades_blocked_package_to_warning(self) -> None:
        manifest = {
            "builtin-baseline": "test-baseline",
            "dependencies": ["glslang"],
        }

        ports = {
            "glslang": {
                "name": "glslang",
                "version": "16.1.0",
                "license": "Apache-2.0 AND BSD-3-Clause AND MIT AND GPL-3.0-or-later",
                "dependencies": [],
            }
        }

        def fake_fetch(port_name: str, *, baseline: str, ports_dir: Path | None, cache: dict[str, dict]):
            self.assertEqual(baseline, "test-baseline")
            return ports[port_name]

        with mock.patch.object(audit, "fetch_port_manifest", side_effect=fake_fetch):
            packages = audit.build_package_records(
                manifest,
                self.policy,
                baseline="test-baseline",
                ports_dir=REPO_ROOT,
                platform_tags={"linux", "x64"},
                include_host_deps=False,
            )

        self.assertEqual(len(packages), 1)
        self.assertEqual(packages[0]["action"], "warn")
        self.assertIn("policy exception applied", packages[0]["reason"].lower())
        self.assertEqual(
            packages[0]["policy_exception"]["package"],
            "glslang",
        )


if __name__ == "__main__":
    unittest.main()
