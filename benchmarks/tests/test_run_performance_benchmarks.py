"""Regression tests for the Google Benchmark shell runner's result gates."""

import json
import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


_SCRIPT = Path(__file__).resolve().parents[1] / "run_performance_benchmarks.sh"
_BASH = shutil.which("bash")


def _run_runner(build_dir: Path, output_dir: Path) -> subprocess.CompletedProcess:
    env = os.environ.copy()
    env["BUILD_DIR"] = str(build_dir)
    env["OUTPUT_DIR"] = str(output_dir)
    return subprocess.run(
        [_BASH, str(_SCRIPT)],
        capture_output=True,
        text=True,
        env=env,
        timeout=30,
        check=False,
    )


@unittest.skipUnless(_BASH is not None and os.name != "nt", "POSIX bash is required")
class RunPerformanceBenchmarksTests(unittest.TestCase):
    def test_missing_benchmark_binaries_do_not_report_success(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            result = _run_runner(root / "build", root / "results")

        self.assertEqual(result.returncode, 2)
        self.assertIn("Successful: \x1b[0;32m0", result.stdout)
        self.assertIn("No benchmark produced a validated measurement.", result.stdout)

    def test_only_validated_measurements_are_combined(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            build_dir = root / "build"
            binary_dir = build_dir / "benchmarks"
            binary_dir.mkdir(parents=True)
            binary = binary_dir / "bench_storage_performance"
            binary.write_text(
                "#!/bin/sh\n"
                "for arg in \"$@\"; do\n"
                "  case \"$arg\" in --benchmark_out=*) output=${arg#*=} ;; esac\n"
                "done\n"
                "printf '%s\\n' '{\"benchmarks\":[{\"name\":\"sample\",\"real_time\":1}]}' > \"$output\"\n",
                encoding="utf-8",
            )
            binary.chmod(0o755)
            output_dir = root / "results"

            result = _run_runner(build_dir, output_dir)

            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn("Successful: \x1b[0;32m1", result.stdout)
            combined_file = next(output_dir.glob("all_benchmarks_*.json"))
            combined = json.loads(combined_file.read_text(encoding="utf-8"))
            self.assertEqual(
                [entry["name"] for entry in combined["benchmark_files"]],
                ["bench_storage_performance"],
            )

    def test_invalid_measurement_json_fails_the_runner(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            build_dir = root / "build"
            binary_dir = build_dir / "benchmarks"
            binary_dir.mkdir(parents=True)
            binary = binary_dir / "bench_storage_performance"
            binary.write_text(
                "#!/bin/sh\n"
                "for arg in \"$@\"; do\n"
                "  case \"$arg\" in --benchmark_out=*) output=${arg#*=} ;; esac\n"
                "done\n"
                "printf '%s\\n' '{\"benchmarks\":[{\"name\":\"sample\",\"real_time\":true}]}' > \"$output\"\n",
                encoding="utf-8",
            )
            binary.chmod(0o755)

            result = _run_runner(build_dir, root / "results")

        self.assertEqual(result.returncode, 1)
        self.assertIn("missing or invalid measurement JSON", result.stdout)
        self.assertIn("Successful: \x1b[0;32m0", result.stdout)
