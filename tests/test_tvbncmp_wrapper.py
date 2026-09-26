from __future__ import annotations

import subprocess
import sys
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]


class TvbncmpWrapperTest(unittest.TestCase):
    def test_tvbncmp_wrapper_uses_canonical_benchmark_mapping_verifier(self):
        wrapper_result = subprocess.run(
            [sys.executable, str(REPO_ROOT / "tools" / "python_tools" / "audit" / "tvbncmp.py")],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            check=False,
        )
        canonical_result = subprocess.run(
            [sys.executable, str(REPO_ROOT / "tools" / "verify_benchmark_mapping.py")],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            check=False,
        )

        wrapper_output = f"{wrapper_result.stdout}\n{wrapper_result.stderr}"
        canonical_output = f"{canonical_result.stdout}\n{canonical_result.stderr}"
        self.assertEqual(wrapper_result.returncode, canonical_result.returncode, wrapper_output)
        self.assertEqual(wrapper_result.returncode, 0, wrapper_output)
        self.assertIn("perf_audit check 7a: PASS", wrapper_result.stdout)
        self.assertEqual(
            self._summary_line(wrapper_result.stdout),
            self._summary_line(canonical_result.stdout),
            f"wrapper output:\n{wrapper_output}\ncanonical output:\n{canonical_output}",
        )

    @staticmethod
    def _summary_line(output: str) -> str:
        for line in output.splitlines():
            if line.startswith("perf_audit check 7a: "):
                return line
        return ""


if __name__ == "__main__":
    unittest.main()
