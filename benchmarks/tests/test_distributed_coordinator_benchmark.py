"""Run with THEMIS_DC04_BENCHMARK set to the built coordinator benchmark."""

import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest


@unittest.skipUnless(os.environ.get("THEMIS_DC04_BENCHMARK"),
                     "Set THEMIS_DC04_BENCHMARK to the built benchmark executable")
class DegradedThroughputBenchmarkTest(unittest.TestCase):
    def test_completed_query_rates_and_gate_are_reported(self):
        executable = Path(os.environ["THEMIS_DC04_BENCHMARK"])
        with tempfile.TemporaryDirectory(prefix="themis-dc04-") as directory:
            output = Path(directory) / "result.json"
            process = subprocess.run(
                [str(executable), "--benchmark_filter=^BenchDegradedModeThroughput",
                 f"--benchmark_out={output}", "--benchmark_out_format=json"],
                capture_output=True, encoding="utf-8", errors="replace", timeout=60)
            self.assertEqual(process.returncode, 0, process.stdout + process.stderr)
            rows = json.loads(output.read_text(encoding="utf-8"))["benchmarks"]

        self.assertEqual(len(rows), 1)
        row = rows[0]
        message = row.get("error_message", row.get("label", ""))
        match = re.search(
            r"throughput(?: ratio =)?\s+(\d+)%(?: < 80% threshold)?; "
            r"normal=(\d+)/(\d+\.\d+)s, degraded=(\d+)/(\d+\.\d+)s",
            message, re.IGNORECASE)
        self.assertIsNotNone(match, message)
        percent, normal, normal_seconds, degraded, degraded_seconds = match.groups()
        normal, degraded = int(normal), int(degraded)
        normal_seconds, degraded_seconds = float(normal_seconds), float(degraded_seconds)
        self.assertGreater(normal, 0)
        self.assertGreater(degraded, 0)
        self.assertGreaterEqual(normal_seconds, 1)
        self.assertGreaterEqual(degraded_seconds, 1)
        actual = 100 * (degraded / degraded_seconds) / (normal / normal_seconds)
        self.assertAlmostEqual(int(percent), actual, delta=1.01)
        if abs(actual - 80) > 0.001:
            self.assertEqual(row.get("error_occurred", False), actual < 80)
        self.assertNotIn("aborting merge", process.stdout + process.stderr)


if __name__ == "__main__":
    unittest.main()
