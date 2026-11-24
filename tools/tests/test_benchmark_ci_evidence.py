"""Focused tests for benchmark smoke and disabled Wave 4 gate behavior."""

import copy
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "ci"))
from validate_benchmark_smoke import validate, wave4_required_cases

ROW = {
    "name": "Fixture/Case/1",
    "iterations": 10,
    "real_time": 1.0,
    "cpu_time": 1.0,
    "time_unit": "ns",
}
MANIFEST_PATH = ROOT / "benchmarks/wave4/release_gate_manifest.json"


def wave4_manifest():
    return json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))


def wave4_rows():
    return [
        {**ROW, "name": name + "/real_time", "run_name": name + "/real_time"}
        for name in sorted(wave4_required_cases(wave4_manifest()))
    ]


class SmokeArtifactTests(unittest.TestCase):
    def test_valid_iterations_and_aggregates(self):
        validate({"benchmarks": [ROW, {"name": "Fixture/Case_mean", "run_type": "aggregate"}]})

    def test_incomplete_or_failed_evidence_is_rejected(self):
        examples = [
            {}, {"benchmarks": []}, {"benchmarks": [None]},
            {"error_occurred": True, "benchmarks": [ROW]},
            {"benchmarks": [{"name": "mean", "run_type": "aggregate"}]},
        ]
        for update in (
            {"error_occurred": True}, {"error_message": "failed"},
            {"skipped": True}, {"skip_message": "unavailable"},
            {"iterations": 0}, {"iterations": True}, {"real_time": float("nan")},
            {"cpu_time": None}, {"time_unit": "unknown"}, {"run_type": "unknown"},
        ):
            row = copy.copy(ROW)
            row.update(update)
            examples.append({"benchmarks": [ROW, row]})
        for example in examples:
            with self.subTest(example=example):
                with self.assertRaises(ValueError):
                    validate(example)

    def test_wave4_all_manifest_cases_required_including_advisory(self):
        required = wave4_required_cases(wave4_manifest())
        self.assertEqual(len(required), 6)
        rows = wave4_rows()
        validate({"benchmarks": rows}, required)
        for index in range(len(rows)):
            with self.subTest(missing=rows[index]["name"]):
                remaining = rows[:index] + rows[index + 1:]
                with self.assertRaisesRegex(ValueError, "missing required cases"):
                    validate({"benchmarks": remaining}, required)

    def test_aggregate_and_prefix_cannot_satisfy_missing_case(self):
        required = wave4_required_cases(wave4_manifest())
        rows = wave4_rows()
        missing = rows.pop()
        for substitute in (
            {"name": missing["name"] + "_mean", "run_type": "aggregate"},
            {**missing, "name": missing["name"].replace("/real_time", "0/real_time"),
             "run_name": missing["run_name"].replace("/real_time", "0/real_time")},
        ):
            with self.subTest(substitute=substitute):
                with self.assertRaisesRegex(ValueError, "missing required cases"):
                    validate({"benchmarks": rows + [substitute]}, required)

    def test_inconsistent_run_name_and_failed_repetition_rejected(self):
        required = wave4_required_cases(wave4_manifest())
        rows = wave4_rows()
        for update in ({"run_name": "other"}, {"error_occurred": True}, {"skipped": True}):
            with self.subTest(update=update):
                with self.assertRaises(ValueError):
                    validate({"benchmarks": rows + [{**rows[0], **update}]}, required)

    def test_missing_duplicate_or_malformed_manifest_gate_rejected(self):
        manifest = wave4_manifest()
        examples = [
            {}, {"gates": []}, {**manifest, "gates": manifest["gates"][:-1]},
            {**manifest, "gates": manifest["gates"] + [manifest["gates"][0]]},
            {**manifest, "gates": [*manifest["gates"][:-1], {**manifest["gates"][-1], "benchmark_filter": ""}]},
        ]
        for example in examples:
            with self.subTest(example=example):
                with self.assertRaises(ValueError):
                    wave4_required_cases(example)

    def test_cli_requires_manifest_cases_and_manifest_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            result_file = Path(tmp) / "results.json"
            command = [sys.executable, str(ROOT / "tools/ci/validate_benchmark_smoke.py"),
                       str(result_file), "--wave4-manifest", str(MANIFEST_PATH)]
            for rows, expected in ((wave4_rows(), 0), (wave4_rows()[:-1], 1)):
                result_file.write_text(json.dumps({"benchmarks": rows}), encoding="utf-8")
                result = subprocess.run(command, capture_output=True, text=True, check=False)
                self.assertEqual(result.returncode, expected, result.stderr)
            command[-1] = str(Path(tmp) / "missing.json")
            self.assertNotEqual(subprocess.run(command, capture_output=True).returncode, 0)


class Wave4ReporterTests(unittest.TestCase):
    def invoke(self, *args):
        return subprocess.run(
            [sys.executable, str(ROOT / "benchmarks/wave4/report_variance.py"), *args],
            text=True, capture_output=True, check=False,
        )

    def test_gate_requests_fail_closed_even_with_missing_or_matching_inputs(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "results.json"
            path.write_text(json.dumps({"benchmarks": [ROW]}), encoding="utf-8")
            baseline = Path(tmp) / "baseline.json"
            baseline.write_text(json.dumps({
                "metadata": {"hardware": "other", "compiler": "other"},
                "benchmarks": [ROW],
            }), encoding="utf-8")
            for arguments in (
                ("--input", str(path)),
                ("--input", str(path), "--baseline", str(path)),
                ("--input", str(path), "--baseline", str(baseline)),
                ("--input", str(path), "--manifest", str(Path(tmp) / "missing.json")),
                ("--input", str(path), "--no-gates", "--baseline", str(path)),
            ):
                with self.subTest(arguments=arguments):
                    self.assertNotEqual(self.invoke(*arguments).returncode, 0)

    def test_diagnostics_do_not_report_release_pass(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "results.json"
            report = Path(tmp) / "report.json"
            path.write_text(json.dumps({"benchmarks": [ROW]}), encoding="utf-8")
            result = self.invoke("--input", str(path), "--no-gates", "--output", str(report))
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIsNone(json.loads(report.read_text(encoding="utf-8"))["summary"]["release_blocked"])

    def test_runbook_reporter_examples_are_diagnostic_only(self):
        runbook = (ROOT / "benchmarks/wave4/RUNBOOK.md").read_text(encoding="utf-8")
        for block in runbook.split("```")[1::2]:
            if "python3 benchmarks/wave4/report_variance.py" in block:
                self.assertIn("--no-gates", block)
                self.assertNotIn("--baseline", block)
                self.assertNotIn("--manifest", block)


if __name__ == "__main__":
    unittest.main()
