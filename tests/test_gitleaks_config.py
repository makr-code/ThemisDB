#!/usr/bin/env python3

import tomllib
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
GITLEAKS_CONFIG = REPO_ROOT / ".gitleaks.toml"


class GitleaksConfigTests(unittest.TestCase):
    @staticmethod
    def _load_config() -> dict:
        return tomllib.loads(GITLEAKS_CONFIG.read_text(encoding="utf-8"))

    def test_workflow_policy_files_are_allowlisted_for_gitleaks(self) -> None:
        config = self._load_config()
        allowlist_paths = set(config.get("allowlist", {}).get("paths", []))

        self.assertIn(r"\.github/workflows/ci-pr-gates\.yml$", allowlist_paths)
        self.assertIn(r"\.github/workflows/gate-pr-core\.yml$", allowlist_paths)

    def test_config_keeps_expected_global_allowlist_shape(self) -> None:
        config = self._load_config()
        allowlist = config.get("allowlist")
        self.assertIsInstance(allowlist, dict)
        self.assertIsInstance(allowlist.get("paths"), list)
        self.assertGreater(len(allowlist["paths"]), 0)

    def test_private_key_detection_rule_is_present(self) -> None:
        config = self._load_config()
        rules = config.get("rules")
        self.assertIsInstance(rules, list)
        private_key_rule = next((rule for rule in rules if rule.get("id") == "private-key-file"), None)
        self.assertIsNotNone(private_key_rule)
        self.assertIn("BEGIN", private_key_rule.get("regex", ""))


if __name__ == "__main__":
    unittest.main()
