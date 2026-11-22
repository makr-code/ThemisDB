#!/usr/bin/env python3

import sys
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPTS_DIR = REPO_ROOT / "scripts"
if str(SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPTS_DIR))

import validate_primary_doc_structure as gate  # noqa: E402


class PrimaryDocStructureScopeTests(unittest.TestCase):
    def test_module_root_readme_is_in_scope(self) -> None:
        config = {"include": ["src/*/README.md"], "exclude": []}

        self.assertTrue(gate.is_in_scope("src/tensor/README.md", config))

    def test_nested_readme_is_not_in_module_root_scope(self) -> None:
        config = {"include": ["src/*/README.md"], "exclude": []}

        self.assertFalse(
            gate.is_in_scope("src/distributed_tensor/src/README.md", config)
        )


if __name__ == "__main__":
    unittest.main()
