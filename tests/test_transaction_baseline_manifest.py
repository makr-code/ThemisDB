#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_PATH = REPO_ROOT / "tools" / "ci" / "generate_transaction_baseline_manifest.py"
WORKFLOW_PATH = REPO_ROOT / ".github" / "workflows" / "build-wave-b-transaction.yml"
SIGNOFF_PATH = REPO_ROOT / "docs" / "governance" / "GA_PROMOTION_SIGN_OFF.md"


def load_module():
    spec = importlib.util.spec_from_file_location("generate_transaction_baseline_manifest", SCRIPT_PATH)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


generator = load_module()


class TransactionBaselineManifestTests(unittest.TestCase):
    def test_script_generates_manifest_with_rollup_metrics(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            benchmark_json = root / "phase4_baseline.json"
            chaos_log = root / "chaos.txt"
            lscpu = root / "lscpu.txt"
            memory = root / "memory.txt"
            chaos_hw = root / "chaos_hw.md"
            output_json = root / "benchmarks" / "wave8" / "TRANSACTION_BASELINES_LATEST.json"
            output_md = root / "benchmarks" / "wave8" / "TRANSACTION_BASELINES_LATEST.md"

            benchmark_json.write_text(
                json.dumps(
                    {
                        "benchmarks": [
                            {
                                "name": "ThroughputBaseline_ReadCommitted",
                                "iterations": 100,
                                "time_unit": "ms",
                                "txns_per_sec": 12500.0,
                                "p50_ms": 0.04,
                                "p95_ms": 0.09,
                                "p99_ms": 0.11,
                                "gate_pass": 1.0,
                            },
                            {
                                "name": "ThroughputBaseline_Rollback",
                                "iterations": 100,
                                "time_unit": "ms",
                                "txns_per_sec": 9800.0,
                                "p50_ms": 0.05,
                                "p95_ms": 0.12,
                                "p99_ms": 0.16,
                                "gate_pass": 0.0,
                            },
                            {
                                "name": "ThroughputBaseline_Rollback_mean",
                                "txns_per_sec": 11150.0,
                            },
                        ]
                    }
                ),
                encoding="utf-8",
            )
            chaos_log.write_text("100% tests passed, 0 tests failed out of 4\n", encoding="utf-8")
            lscpu.write_text(
                "Architecture: x86_64\nCPU(s): 16\nModel name: Example CPU\n",
                encoding="utf-8",
            )
            memory.write_text("Mem:           62Gi       10Gi\n", encoding="utf-8")
            chaos_hw.write_text("# hw\n", encoding="utf-8")

            old_argv = sys.argv[:]
            try:
                sys.argv = [
                    "generate_transaction_baseline_manifest.py",
                    "--benchmark-json",
                    str(benchmark_json),
                    "--chaos-log",
                    str(chaos_log),
                    "--lscpu",
                    str(lscpu),
                    "--memory",
                    str(memory),
                    "--chaos-hardware",
                    str(chaos_hw),
                    "--workflow",
                    "build-wave-b-transaction.yml",
                    "--run-id",
                    "12345",
                    "--run-url",
                    "https://github.com/makr-code/ThemisDB/actions/runs/12345",
                    "--commit-sha",
                    "abc123",
                    "--output-json",
                    str(output_json),
                    "--output-md",
                    str(output_md),
                ]
                code = generator.main()
            finally:
                sys.argv = old_argv

            self.assertEqual(code, 0)
            manifest = json.loads(output_json.read_text(encoding="utf-8"))
            self.assertEqual(manifest["workflow"], "build-wave-b-transaction.yml")
            self.assertEqual(manifest["run_id"], "12345")
            self.assertEqual(manifest["performance_summary"]["best_ops_per_sec"], 12500.0)
            self.assertEqual(manifest["performance_summary"]["lowest_ops_per_sec"], 9800.0)
            self.assertEqual(manifest["performance_summary"]["worst_p99_ms"], 0.16)
            self.assertEqual(manifest["chaos_recovery"]["status"], "success")
            self.assertEqual(manifest["chaos_recovery"]["pass_rate_percent"], 100)
            self.assertEqual(manifest["hardware_profile"]["cpu_model"], "Example CPU")
            self.assertEqual(manifest["signoff"]["required_approver"], "platform-release@themisdb")
            self.assertEqual(manifest["signoff"]["status"], "pending")
            self.assertTrue(output_md.exists())

    def test_script_rejects_missing_benchmark_rows(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            benchmark_json = root / "phase4_baseline.json"
            benchmark_json.write_text(json.dumps({"benchmarks": []}), encoding="utf-8")
            output_json = root / "out.json"

            completed = subprocess.run(
                [
                    sys.executable,
                    str(SCRIPT_PATH),
                    "--benchmark-json",
                    str(benchmark_json),
                    "--workflow",
                    "build-wave-b-transaction.yml",
                    "--run-id",
                    "12345",
                    "--run-url",
                    "https://github.com/makr-code/ThemisDB/actions/runs/12345",
                    "--commit-sha",
                    "abc123",
                    "--output-json",
                    str(output_json),
                ],
                text=True,
                capture_output=True,
            )
            self.assertNotEqual(completed.returncode, 0)
            self.assertIn("did not contain any non-aggregate benchmark rows", completed.stderr)

    def test_workflow_and_signoff_reference_canonical_wave_b_evidence(self) -> None:
        workflow = WORKFLOW_PATH.read_text(encoding="utf-8")
        signoff = SIGNOFF_PATH.read_text(encoding="utf-8")

        self.assertIn('uses: actions/checkout@11bd71901bbe5b1630ceea73d27597364c9af683', workflow)
        self.assertIn("generate_transaction_baseline_manifest.py", workflow)
        self.assertIn("TRANSACTION_BASELINES_LATEST.json", workflow)
        self.assertIn("transaction-baseline-evidence", workflow)

        self.assertIn("TRANSACTION_BASELINES_LATEST.json", signoff)
        self.assertIn("transaction-baseline-evidence", signoff)


if __name__ == "__main__":
    unittest.main()
