"""Retired RAID benchmark entry points must have no execution side effects."""

import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


_BENCHMARKS = Path(__file__).resolve().parents[1]
_PYTHON_ENTRY_POINTS = (
    "run_multi_shard_raid_benchmark.py",
    "quickstart.py",
)
_POWERSHELL = shutil.which("pwsh")


class RaidBenchmarkRetirementTests(unittest.TestCase):
    def test_python_entry_points_fail_closed_without_side_effects(self):
        for entry_point in _PYTHON_ENTRY_POINTS:
            with self.subTest(entry_point=entry_point), tempfile.TemporaryDirectory() as directory:
                result = subprocess.run(
                    [sys.executable, str(_BENCHMARKS / entry_point)],
                    cwd=directory,
                    capture_output=True,
                    text=True,
                    timeout=30,
                    check=False,
                )

                self.assertEqual(result.returncode, 2)
                self.assertIn("retired:", result.stderr.lower())
                self.assertIn("no benchmark", result.stderr.lower())
                self.assertEqual(list(Path(directory).iterdir()), [])

    @unittest.skipUnless(_POWERSHELL, "pwsh is required")
    def test_powershell_entry_point_fails_closed_without_side_effects(self):
        with tempfile.TemporaryDirectory() as directory:
            result = subprocess.run(
                [
                    _POWERSHELL,
                    "-NoLogo",
                    "-NoProfile",
                    "-File",
                    str(_BENCHMARKS / "run_benchmark_simple.ps1"),
                    "-Scenario",
                    "S1",
                    "-RaidLevel",
                    "RAID10",
                    "-NumShards",
                    "3",
                ],
                cwd=directory,
                capture_output=True,
                text=True,
                timeout=30,
                check=False,
            )

            self.assertEqual(result.returncode, 2)
            self.assertIn("Retired:", result.stderr)
            self.assertIn("No benchmark was run", result.stderr)
            self.assertEqual(list(Path(directory).iterdir()), [])


if __name__ == "__main__":
    unittest.main()
