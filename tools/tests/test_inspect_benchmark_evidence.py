"""Synthetic unit fixtures for the opt-in evidence preflight."""

import copy
import hashlib
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "ci"))
from inspect_benchmark_evidence import inspect

WORKLOAD = "themisdb:component/example@1"


def encoded(value):
    return json.dumps(value, sort_keys=True).encode("utf-8")


def digest(value):
    return hashlib.sha256(encoded(value)).hexdigest()


def fixture():
    policy = {
        "schema_version": 1, "workload_id": WORKLOAD, "repetitions": 2,
        "required_cases": {"Fixture/A/real_time": ["items_per_second"],
                           "Fixture/B/real_time": ["items_per_second"]},
    }
    catalog = {"schema_version": 1, "profiles": [
        {"workload_id": WORKLOAD, "status": "source-described"},
    ]}
    rows = [
        {"name": name, "run_name": name, "run_type": "iteration",
         "repetitions": 2, "repetition_index": index, "iterations": 10,
         "real_time": 10.0, "cpu_time": 8.0, "time_unit": "ns",
         "items_per_second": 100.0}
        for name in policy["required_cases"] for index in range(2)
    ]
    result = {"benchmarks": rows}
    sidecar = {
        "schema_version": 1,
        "baseline_freeze": {"compiler": "test compiler", "compiler_flags": "test flags",
                            "preset": "test preset", "hardware_profile": "test profile",
                            "os_image": "test OS"},
        "hardware": {"cpu_model": "test CPU", "architecture": "test architecture",
                     "logical_processors": 2, "memory_bytes": 100, "storage_identity": "none",
                     "accelerator_identity": "none"},
        "build": {"build_type": "test", "configuration_sha256": "a" * 64,
                  "dependencies_sha256": "b" * 64},
        "workload": {"workload_id": WORKLOAD, "catalog_sha256": digest(catalog),
                     "dataset_sha256": "c" * 64, "configuration_sha256": "d" * 64,
                     "seed": 1, "warmup": "test warmup", "minimum_measurement_time": "1s",
                     "repetitions": 2},
        "execution": {"status": "completed", "exit_code": 0, "run_id": "baseline-test",
                      "source_revision": "a" * 40, "worktree_dirty": False,
                      "binary_sha256": "e" * 64, "runner_sha256": "f" * 64,
                      "result_sha256": digest(result), "policy_sha256": digest(policy)},
    }
    current_sidecar = copy.deepcopy(sidecar)
    current_sidecar["execution"].update(run_id="current-test", source_revision="b" * 40,
                                       binary_sha256="b" * 64)
    return [policy, digest(policy), catalog, digest(catalog), sidecar, result, digest(result),
            current_sidecar, copy.deepcopy(result), digest(result)]


def observation(args, current=False):
    sidecar = args[7 if current else 4]
    return {
        "schema_version": 1,
        "identity_scope": "local-observation-not-attestation",
        "captured_at_utc": "2026-01-01T00:00:00+00:00",
        "host_fingerprint_scope": (
            "OS, CPU model, architecture, logical processor count, and reported memory; "
            "storage and accelerators are not observed"),
        "workload": {"workload_id": sidecar["workload"]["workload_id"],
                     "catalog_sha256": sidecar["workload"]["catalog_sha256"]},
        "artifacts": {
            "executable_sha256": sidecar["execution"]["binary_sha256"],
            "runner_sha256": sidecar["execution"]["runner_sha256"],
            "raw_result_sha256": sidecar["execution"]["result_sha256"],
            "configuration_sha256": "9" * 64,
        },
        "host": {"os": {"system": "Windows", "release": "11", "distribution": None,
                         "distribution_version": "10.0.26100"},
                 "hardware": {key: sidecar["hardware"][key] for key in
                              ("cpu_model", "architecture", "logical_processors", "memory_bytes")}},
    }


class EvidencePreflightTests(unittest.TestCase):
    def test_complete_evidence_is_consistent_not_a_gate(self):
        inspect(*fixture())

    def test_identity_mismatch_or_missing_fails(self):
        for section, key, value in (
            ("baseline_freeze", "compiler", "different"),
            ("baseline_freeze", "compiler_flags", "different"),
            ("baseline_freeze", "os_image", "different"),
            ("baseline_freeze", "preset", "different"),
            ("baseline_freeze", "hardware_profile", "unknown"),
            ("hardware", "cpu_model", "different"),
            ("hardware", "architecture", "different"),
            ("hardware", "memory_bytes", 101),
            ("hardware", "logical_processors", 3),
            ("hardware", "storage_identity", "different"),
            ("hardware", "accelerator_identity", "different"),
            ("build", "configuration_sha256", "9" * 64),
            ("build", "dependencies_sha256", "9" * 64),
            ("workload", "dataset_sha256", "9" * 64),
            ("workload", "configuration_sha256", "9" * 64),
            ("workload", "seed", 3), ("workload", "warmup", "different"),
            ("workload", "minimum_measurement_time", "2s"),
            ("workload", "workload_id", "themisdb:component/other@1"),
        ):
            args = fixture()
            args[7][section][key] = value
            with self.subTest(section=section, key=key), self.assertRaises(ValueError):
                inspect(*args)
        args = fixture()
        del args[7]["hardware"]["cpu_model"]
        with self.assertRaises(ValueError):
            inspect(*args)

    def test_status_hash_and_provenance_fail_closed(self):
        for key, value in (
            ("status", "skipped"), ("status", "failed"), ("exit_code", 1),
            ("exit_code", False), ("worktree_dirty", True), ("source_revision", "unknown"),
            ("run_id", "baseline-test"), ("binary_sha256", ""), ("runner_sha256", "1" * 64),
            ("result_sha256", "2" * 64), ("policy_sha256", "2" * 64),
        ):
            args = fixture()
            args[7]["execution"][key] = value
            with self.subTest(key=key, value=value), self.assertRaises(ValueError):
                inspect(*args)
        args = fixture()
        args[4]["execution"]["status"] = "skipped"
        with self.assertRaises(ValueError):
            inspect(*args)

    def test_existing_wave4_baseline_is_not_an_identity_sidecar(self):
        args = fixture()
        path = ROOT / "benchmarks/wave4/baselines/baseline_v1.5.0.json"
        args[4] = json.loads(path.read_text(encoding="utf-8"))
        with self.assertRaises(ValueError):
            inspect(*args)

    def test_case_repetition_and_metric_gaps_fail(self):
        for mutation in (
            lambda rows: rows.pop(),
            lambda rows: rows.append(copy.deepcopy(rows[0])),
            lambda rows: rows[0].update(repetition_index=2),
            lambda rows: rows[0].pop("repetition_index"),
            lambda rows: rows[0].update(repetitions=1),
            lambda rows: rows[0].update(items_per_second=float("nan")),
            lambda rows: rows[0].pop("items_per_second"),
            lambda rows: rows[0].update(skipped=True),
            lambda rows: rows[0].update(error_occurred=True),
            lambda rows: rows[0].update(name="Fixture/A/real_time_extra"),
            lambda rows: rows[0].update(run_type="aggregate"),
            lambda rows: rows[0].update(repetition_index=True),
            lambda rows: rows[0].update(real_time=float("inf")),
        ):
            args = fixture()
            mutation(args[8]["benchmarks"])
            args[9] = digest(args[8])
            args[7]["execution"]["result_sha256"] = args[9]
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                inspect(*args)

    def test_policy_and_catalog_fail_closed(self):
        for mutation in (
            lambda args: args[0].update(required_cases={}),
            lambda args: args[0]["required_cases"].update({"Fixture/A/real_time": []}),
            lambda args: args[0].update(repetitions=0),
            lambda args: args[2]["profiles"][0].update(status="simulation-only"),
            lambda args: args[2]["profiles"].append(copy.deepcopy(args[2]["profiles"][0])),
            lambda args: args[7]["workload"].update(catalog_sha256="0" * 64),
        ):
            args = fixture()
            mutation(args)
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                inspect(*args)

    def test_cli_binds_actual_files_and_rejects_tampering(self):
        args = fixture()
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            root = Path(tmp)
            files = [("policy", args[0]), ("catalog", args[2]), ("baseline-sidecar", args[4]),
                     ("baseline-result", args[5]), ("current-sidecar", args[7]), ("current-result", args[8])]
            command = [sys.executable, str(ROOT / "tools/ci/inspect_benchmark_evidence.py")]
            for name, value in files:
                path = root / (name + ".json")
                path.write_bytes(encoded(value))
                command.extend(["--" + name, str(path)])
            result = subprocess.run(command, capture_output=True, text=True, check=False)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn("NOT a performance", result.stdout)
            (root / "current-result.json").write_bytes(encoded(args[8]) + b" ")
            result = subprocess.run(command, capture_output=True, text=True, check=False)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("hash mismatch", result.stderr)
            (root / "policy.json").write_text('{"schema_version": 1, "schema_version": 1}', encoding="utf-8")
            result = subprocess.run(command, capture_output=True, text=True, check=False)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("duplicate JSON key", result.stderr)

    def test_paired_observations_and_each_comparable_mismatch(self):
        args = fixture()
        baseline, current = observation(args), observation(args, True)
        inspect(*args, baseline, current)
        for side in (0, 1):
            for section, key, value in (
                ("workload", "workload_id", "themisdb:component/other@1"),
                ("workload", "catalog_sha256", "0" * 64),
                ("artifacts", "executable_sha256", "0" * 64),
                ("artifacts", "runner_sha256", "0" * 64),
                ("artifacts", "raw_result_sha256", "0" * 64),
                ("hardware", "cpu_model", "other CPU"),
                ("hardware", "architecture", "other architecture"),
                ("hardware", "logical_processors", 3),
                ("hardware", "memory_bytes", 101),
            ):
                pair = [copy.deepcopy(baseline), copy.deepcopy(current)]
                target = pair[side]["host"]["hardware"] if section == "hardware" else pair[side][section]
                target[key] = value
                with self.subTest(side=side, key=key), self.assertRaisesRegex(ValueError, "mismatch"):
                    inspect(*args, *pair)
        for pair in ((baseline,), (baseline, None)):
            with self.subTest(pair=pair), self.assertRaises(ValueError):
                inspect(*args, *pair)
        with self.assertRaises(ValueError):
            inspect(*args, current_observation=current)
        for key, value in (("release", "10"), ("distribution_version", "10.0.1")):
            changed = copy.deepcopy(current)
            changed["host"]["os"][key] = value
            with self.subTest(os_key=key), self.assertRaisesRegex(ValueError, "OS.*mismatch"):
                inspect(*args, baseline, changed)
        linux = copy.deepcopy(current)
        linux["host"]["os"] = {"system": "Linux", "release": "6.1", "distribution": "debian",
                                "distribution_version": "12"}
        with self.assertRaisesRegex(ValueError, "OS.*mismatch"):
            inspect(*args, baseline, linux)
        linux_baseline = copy.deepcopy(baseline)
        linux_baseline["host"]["os"] = copy.deepcopy(linux["host"]["os"])
        inspect(*args, linux_baseline, linux)
        for key in ("distribution", "distribution_version"):
            changed = copy.deepcopy(linux)
            changed["host"]["os"][key] = "different"
            with self.subTest(os_key=key), self.assertRaisesRegex(ValueError, "OS.*mismatch"):
                inspect(*args, linux_baseline, changed)

    def test_observation_schema_rejects_missing_extra_and_wrong_types(self):
        args = fixture()
        original = observation(args)
        paths = (
            (), ("workload",), ("artifacts",), ("host",),
            ("host", "os"), ("host", "hardware"),
        )
        for path in paths:
            target = original
            for key in path:
                target = target[key]
            for key in target:
                record = copy.deepcopy(original)
                nested = record
                for part in path:
                    nested = nested[part]
                del nested[key]
                with self.subTest(path=path, missing=key), self.assertRaises(ValueError):
                    inspect(*args, record, observation(args, True))
            record = copy.deepcopy(original)
            nested = record
            for part in path:
                nested = nested[part]
            nested["unexpected"] = "value"
            with self.subTest(path=path, extra=True), self.assertRaises(ValueError):
                inspect(*args, record, observation(args, True))
        for path, value in (
            (("schema_version",), True), (("identity_scope",), "unknown"),
            (("host_fingerprint_scope",), "other"), (("captured_at_utc",), "unknown"),
            (("captured_at_utc",), "2026-01-01T00:00:00+01:00"),
            (("captured_at_utc",), 42), (("workload", "workload_id"), None),
            (("workload", "catalog_sha256"), "F" * 64),
            (("artifacts", "configuration_sha256"), "unknown"),
            (("artifacts", "runner_sha256"), 123), (("host",), []),
            (("host", "os", "system"), "unknown"), (("host", "os", "release"), None),
            (("host", "os", "distribution"), "none"),
            (("host", "hardware", "cpu_model"), "unknown"),
            (("host", "hardware", "architecture"), "none known"),
            (("host", "hardware", "logical_processors"), True),
            (("host", "hardware", "memory_bytes"), 0),
        ):
            record = copy.deepcopy(original)
            nested = record
            for part in path[:-1]:
                nested = nested[part]
            nested[path[-1]] = value
            with self.subTest(path=path, value=value), self.assertRaises(ValueError):
                inspect(*args, record, observation(args, True))

    def test_capture_cli_observations_are_bound_to_actual_results(self):
        args = fixture()
        name = "MMDBFixture/IndexedProductLookup/1000/real_time"
        args[0]["workload_id"] = "themisdb:component/mmdb-secondary-index@1"
        args[0]["required_cases"] = {name: ["items_per_second"]}
        args[1] = digest(args[0])
        catalog_path = ROOT / "benchmarks" / "workload_profiles.json"
        args[2] = json.loads(catalog_path.read_text(encoding="utf-8-sig"))
        args[3] = hashlib.sha256(catalog_path.read_bytes()).hexdigest()
        for sidecar_index, result_index, hash_index in ((4, 5, 6), (7, 8, 9)):
            args[result_index]["benchmarks"] = args[result_index]["benchmarks"][:2]
            for row in args[result_index]["benchmarks"]:
                row["name"] = row["run_name"] = name
            if sidecar_index == 7:
                args[result_index]["benchmarks"][0]["real_time"] = 11.0
            args[hash_index] = digest(args[result_index])
            args[sidecar_index]["workload"].update(workload_id=args[0]["workload_id"],
                                                  catalog_sha256=args[3])
            args[sidecar_index]["execution"].update(policy_sha256=args[1],
                                                   result_sha256=args[hash_index])
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            root = Path(tmp)
            runner, config = root / "runner.py", root / "configuration.json"
            runner.write_bytes(b"synthetic runner")
            config.write_bytes(b"synthetic configuration")
            command = [sys.executable, str(ROOT / "tools" / "ci" / "inspect_benchmark_evidence.py")]
            for option, content in (("policy", args[0]), ("baseline-result", args[5]),
                                    ("current-result", args[8])):
                path = root / (option + ".json")
                path.write_bytes(encoded(content))
                command.extend(["--" + option, str(path)])
            command.extend(["--catalog", str(catalog_path)])
            for label, sidecar_index in (("baseline", 4), ("current", 7)):
                folder = root / label
                folder.mkdir()
                executable = folder / "bench_mmdb.exe"
                executable.write_bytes(label.encode())
                output = folder / "observation.json"
                capture = subprocess.run(
                    [sys.executable, str(ROOT / "tools" / "ci" / "capture_benchmark_identity.py"),
                     "--executable", str(executable), "--runner", str(runner),
                     "--result", str(root / (label + "-result.json")),
                     "--configuration", str(config), "--target", "bench_mmdb",
                     "--output", str(output)], capture_output=True, text=True, check=False)
                self.assertEqual(capture.returncode, 0, capture.stderr)
                record = json.loads(output.read_text(encoding="utf-8"))
                args[sidecar_index]["hardware"].update(record["host"]["hardware"])
                args[sidecar_index]["execution"].update(
                    binary_sha256=record["artifacts"]["executable_sha256"],
                    runner_sha256=record["artifacts"]["runner_sha256"])
                sidecar_path = folder / "sidecar.json"
                sidecar_path.write_bytes(encoded(args[sidecar_index]))
                command.extend(["--" + label + "-sidecar", str(sidecar_path),
                                "--" + label + "-observation", str(output)])
            result = subprocess.run(command, capture_output=True, text=True, check=False)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn("NOT attestation", result.stdout)
            for label in ("baseline", "current"):
                missing = command[:]
                index = missing.index("--" + label + "-observation")
                del missing[index:index + 2]
                result = subprocess.run(missing, capture_output=True, text=True, check=False)
                self.assertNotIn("Traceback", result.stderr)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("provided together", result.stderr)
            (root / "current-result.json").write_bytes(encoded(args[8]) + b" ")
            result = subprocess.run(command, capture_output=True, text=True, check=False)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("hash mismatch", result.stderr)
            (root / "current-result.json").write_bytes(encoded(args[8]))
            output.write_text('{"schema_version":1,"schema_version":1}', encoding="utf-8")
            result = subprocess.run(command, capture_output=True, text=True, check=False)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("duplicate JSON key", result.stderr)
            output.write_text("null", encoding="utf-8")
            result = subprocess.run(command, capture_output=True, text=True, check=False)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("current observation", result.stderr)


if __name__ == "__main__":
    unittest.main()
