from __future__ import annotations

import subprocess
import sys
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]


class TvbncmpWrapperTest(unittest.TestCase):
    def test_tvbncmp_wrapper_uses_canonical_benchmark_mapping_verifier(self):
        result = subprocess.run(
            [sys.executable, str(REPO_ROOT / "tools" / "python_tools" / "audit" / "tvbncmp.py")],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            check=False,
        )

        combined_output = f"{result.stdout}\n{result.stderr}"
        self.assertEqual(result.returncode, 0, combined_output)
        self.assertIn("perf_audit check 7a: PASS", result.stdout)
        self.assertIn("[5a] Check coverage_summary.total vs PERFORMANCE_EXPECTATIONS.md", result.stdout)


if __name__ == "__main__":
    unittest.main()
