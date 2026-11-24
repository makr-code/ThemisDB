"""Regression tests for the explicit Wave 4A smoke coverage contract."""

import importlib.util
import json
from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[2]
CONTRACT_PATH = ROOT / "benchmarks" / "wave4" / "smoke_contract.json"
VALIDATOR_PATH = ROOT / "benchmarks" / "wave4" / "validate_smoke.py"
GENERIC_VALIDATOR_PATH = ROOT / "tools" / "ci" / "validate_benchmark_smoke.py"
SOURCE_PATH = ROOT / "benchmarks" / "wave4" / "bench_w4a_release_gates.cpp"
WORKFLOW_PATH = ROOT / ".github" / "workflows" / "benchmark-gate.yml"

spec = importlib.util.spec_from_file_location("wave4_smoke_validator", VALIDATOR_PATH)
validator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(validator)
generic_spec = importlib.util.spec_from_file_location("benchmark_smoke_validator", GENERIC_VALIDATOR_PATH)
generic_validator = importlib.util.module_from_spec(generic_spec)
generic_spec.loader.exec_module(generic_validator)


def _raw_rows(name, repetitions):
    return [
        {"name": name, "run_type": "iteration", "repetitions": repetitions,
         "repetition_index": index}
        for index in range(repetitions)
    ]


class Wave4SmokeCompletenessTests(unittest.TestCase):
    def setUp(self):
        self.contract = {
            "schema": "themisdb.wave4_smoke_contract",
            "schema_version": 1,
            "benchmark_filter": "W4A",
            "required_repetitions": 3,
            "expected_cases": ["W4A/100", "W4A/500"],
        }

    def test_raw_repetitions_pass_and_aggregates_do_not_count(self):
        rows = _raw_rows("W4A/100", 3) + _raw_rows("W4A/500", 3)
        rows += [
            {"name": "W4A/100_mean", "run_name": "W4A/100", "run_type": "aggregate",
             "aggregate_name": "mean", "repetitions": 3},
            {"name": "W4A/100_cv", "run_name": "W4A/100", "run_type": "aggregate",
             "aggregate_name": "cv", "repetitions": 3},
        ]
        self.assertEqual(validator.validate_results({"benchmarks": rows}, self.contract), [])

    def test_missing_case_or_repetition_fails(self):
        rows = _raw_rows("W4A/100", 3) + _raw_rows("W4A/500", 3)[:-1]
        errors = validator.validate_results({"benchmarks": rows}, self.contract)
        self.assertTrue(any("missing raw repetitions [2]" in error for error in errors))

    def test_duplicate_and_out_of_range_repetitions_fail(self):
        rows = _raw_rows("W4A/100", 3) + _raw_rows("W4A/500", 3)
        rows.append({"name": "W4A/100", "run_type": "iteration", "repetitions": 3,
                     "repetition_index": 1})
        rows.append({"name": "W4A/500", "run_type": "iteration", "repetitions": 3,
                     "repetition_index": 3})
        errors = validator.validate_results({"benchmarks": rows}, self.contract)
        self.assertTrue(any("duplicate repetition_index 1" in error for error in errors))
        self.assertTrue(any("outside the required range" in error for error in errors))

    def test_aggregate_rows_alone_do_not_satisfy_expected_cases(self):
        rows = [
            {"name": "W4A/100_mean", "run_name": "W4A/100", "run_type": "aggregate",
             "aggregate_name": "mean", "repetitions": 3},
            {"name": "W4A/500_mean", "run_name": "W4A/500", "run_type": "aggregate",
             "aggregate_name": "mean", "repetitions": 3},
        ]
        errors = validator.validate_results({"benchmarks": rows}, self.contract)
        self.assertEqual(sum("missing raw repetitions" in error for error in errors), 2)

    def test_checked_in_contract_accepts_all_declared_raw_repetitions(self):
        contract = json.loads(CONTRACT_PATH.read_text(encoding="utf-8"))
        count = contract["required_repetitions"]
        rows = [
            row
            for name in contract["expected_cases"]
            for row in _raw_rows(name + "/real_time", count)
        ]
        rows.append({"name": contract["expected_cases"][0] + "/real_time_mean",
                     "run_name": contract["expected_cases"][0] + "/real_time",
                     "run_type": "aggregate", "repetitions": count})
        self.assertEqual(validator.validate_results({"benchmarks": rows}, contract), [])

    def test_real_time_suffix_does_not_hide_duplicate_repetition(self):
        rows = _raw_rows("W4A/100", 3) + _raw_rows("W4A/500/real_time", 3)
        rows.append(_raw_rows("W4A/100/real_time", 3)[0])
        errors = validator.validate_results({"benchmarks": rows}, self.contract)
        self.assertTrue(any("duplicate repetition_index 0" in error for error in errors))

    def test_both_smoke_validators_accept_google_benchmark_repetition_output(self):
        contract = json.loads(CONTRACT_PATH.read_text(encoding="utf-8"))
        manifest_path = ROOT / "benchmarks" / "wave4" / "release_gate_manifest.json"
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        rows = []
        for case in contract["expected_cases"]:
            name = case + "/real_time"
            for row in _raw_rows(name, contract["required_repetitions"]):
                row.update(run_name=name, iterations=10, real_time=12.0,
                           cpu_time=11.0, time_unit="ns")
                rows.append(row)
            rows.append({"name": name + "_mean", "run_name": name,
                         "run_type": "aggregate", "repetitions": 5})
        results = {"benchmarks": rows}
        self.assertEqual(validator.validate_results(results, contract), [])
        generic_validator.validate(results, generic_validator.wave4_required_cases(manifest))

    def test_aggregate_metadata_must_match_expected_case_and_repetitions(self):
        rows = _raw_rows("W4A/100", 3) + _raw_rows("W4A/500", 3)
        rows.append({"name": "W4A/extra_mean", "run_name": "W4A/extra",
                     "run_type": "aggregate", "aggregate_name": "mean",
                     "repetitions": 3})
        errors = validator.validate_results({"benchmarks": rows}, self.contract)
        self.assertTrue(any("aggregate references unexpected case" in error
                            for error in errors))

    def test_benchmark_error_and_skip_are_not_measurements(self):
        rows = _raw_rows("W4A/100", 3) + _raw_rows("W4A/500", 3)[:-1]
        rows += [
            {"name": "W4A/500", "run_type": "iteration", "skipped": True,
             "skip_message": "backend unavailable"},
            {"name": "W4A/100", "run_type": "iteration", "error_occurred": True,
             "error_message": "fixture failed"},
        ]
        errors = validator.validate_results({"benchmarks": rows}, self.contract)
        self.assertTrue(any("benchmark error: fixture failed" in error for error in errors))
        self.assertTrue(any("skipped and is not complete" in error for error in errors))
        self.assertTrue(any("missing raw repetitions" in error for error in errors))

    def test_unexpected_cases_and_non_iteration_rows_fail(self):
        rows = _raw_rows("W4A/100", 3) + _raw_rows("W4A/500", 3)
        rows.append({"name": "W4A/new/1", "run_type": "iteration", "repetition_index": 0})
        rows.append({"name": "W4A/other", "run_type": "unexpected"})
        errors = validator.validate_results({"benchmarks": rows}, self.contract)
        self.assertTrue(any("unexpected benchmark case" in error for error in errors))
        self.assertTrue(any("unexpected or missing run_type" in error for error in errors))

    def test_reported_repetition_count_must_match_contract(self):
        rows = _raw_rows("W4A/100", 3) + _raw_rows("W4A/500", 3)
        rows[0]["repetitions"] = 2
        errors = validator.validate_results({"benchmarks": rows}, self.contract)
        self.assertTrue(any("reported repetitions 2" in error for error in errors))

    def test_contract_rejects_duplicate_names_and_nonpositive_repetitions(self):
        duplicate = {**self.contract, "expected_cases": ["W4A/100", "W4A/100"]}
        self.assertTrue(validator.validate_results({"benchmarks": []}, duplicate)[0]
                        .startswith("invalid smoke contract:"))
        zero = {**self.contract, "required_repetitions": 0}
        self.assertTrue(validator.validate_results({"benchmarks": []}, zero)[0]
                        .startswith("invalid smoke contract:"))
        wrong_version = {**self.contract, "schema_version": 2}
        self.assertTrue(validator.validate_results({"benchmarks": []}, wrong_version)[0]
                        .startswith("invalid smoke contract:"))

    def test_explicit_contract_matches_registered_parameter_combinations(self):
        contract = json.loads(CONTRACT_PATH.read_text(encoding="utf-8"))
        source = SOURCE_PATH.read_text(encoding="utf-8")
        registered = []
        fixture_pattern = re.compile(
            r"BENCHMARK_REGISTER_F\(\s*(\w+)\s*,\s*(W4A_\w+)\s*\)(.*?);",
            re.DOTALL,
        )
        for fixture, benchmark_name, body in fixture_pattern.findall(source):
            arguments = re.findall(r"->Arg\((\d+)\)", body)
            registered.extend(
                f"{fixture}/{benchmark_name}/{argument}" for argument in arguments
            )

        standalone_pattern = re.compile(
            r"BENCHMARK\(\s*(BM_W4A_\w+)\s*\)(.*?);",
            re.DOTALL,
        )
        for benchmark_name, body in standalone_pattern.findall(source):
            arguments = re.findall(r"->Arg\((\d+)\)", body)
            registered.extend(
                f"{benchmark_name}/{argument}" for argument in arguments
            )

        self.assertEqual(sorted(contract["expected_cases"]), sorted(registered))

    def test_workflow_uses_contract_filter_and_requested_repetitions(self):
        contract = json.loads(CONTRACT_PATH.read_text(encoding="utf-8"))
        workflow = WORKFLOW_PATH.read_text(encoding="utf-8")
        self.assertIn(f"--benchmark_filter='{contract['benchmark_filter']}'", workflow)
        self.assertIn(f"--benchmark_repetitions={contract['required_repetitions']}", workflow)
        validator_step = workflow.index("validate_smoke.py")
        variance_step = workflow.index("report_variance.py")
        self.assertLess(validator_step, variance_step)


if __name__ == "__main__":
    unittest.main()
