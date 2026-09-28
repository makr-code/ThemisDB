#!/usr/bin/env python3

import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT = REPO_ROOT / "scripts" / "check_module_direct_doxygen.py"


class CheckModuleDirectDoxygenTests(unittest.TestCase):
    def test_module_without_cpp_files_is_reported_as_skip(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            repo_root = Path(temp_dir)
            (repo_root / "src" / "ai_working").mkdir(parents=True)
            (repo_root / "out").mkdir()

            report_json = repo_root / "out" / "report.json"
            summary_md = repo_root / "out" / "summary.md"
            missing_json = repo_root / "out" / "missing.json"
            coverage_md = repo_root / "out" / "coverage.md"

            proc = subprocess.run(
                [
                    sys.executable,
                    str(SCRIPT),
                    "--repo-root",
                    str(repo_root),
                    "--module",
                    "ai_working",
                    "--doxygen-artifacts-root",
                    "out/artifacts",
                    "--report-json",
                    "out/report.json",
                    "--summary-md",
                    "out/summary.md",
                    "--missing-report-json",
                    "out/missing.json",
                    "--coverage-summary-md",
                    "out/coverage.md",
                ],
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(proc.returncode, 0, msg=proc.stderr)
            self.assertIn("DIRECT_DOXYGEN_CHECK_SKIPPED", proc.stdout)

            report = json.loads(report_json.read_text(encoding="utf-8"))
            self.assertEqual(report["status"], "SKIP")
            self.assertEqual(report["source_mode"], "no_cpp_files")
            self.assertEqual(report["source_files_scanned"], 0)
            self.assertEqual(report["coverage"]["missing_symbol_count"], 0)

            missing = json.loads(missing_json.read_text(encoding="utf-8"))
            self.assertEqual(missing["findings"], [])

            self.assertIn("Status: SKIP", summary_md.read_text(encoding="utf-8"))
            self.assertIn("Status: SKIP", coverage_md.read_text(encoding="utf-8"))


if __name__ == "__main__":
    unittest.main()
