#!/usr/bin/env python3

import tomllib
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
GITLEAKS_CONFIG = REPO_ROOT / ".gitleaks.toml"


class GitleaksConfigTests(unittest.TestCase):
    def test_workflow_policy_files_are_allowlisted_for_gitleaks(self) -> None:
        config = tomllib.loads(GITLEAKS_CONFIG.read_text(encoding="utf-8"))
        allowlist_paths = set(config.get("allowlist", {}).get("paths", []))

        self.assertIn(r"\.github/workflows/ci-pr-gates\.yml$", allowlist_paths)
        self.assertIn(r"\.github/workflows/gate-pr-core\.yml$", allowlist_paths)


if __name__ == "__main__":
    unittest.main()
