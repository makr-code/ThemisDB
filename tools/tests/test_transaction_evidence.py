from __future__ import annotations

import contextlib
import io
import json
import os
import re
import subprocess
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path
from unittest.mock import patch

import yaml

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/ci"))

import run_transaction_tests as transaction
import generate_transaction_baseline_manifest as baseline


class TransactionEvidenceTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.binary = self.root / "transaction-test"
        self.binary.touch()
        self.inventory = [
            {
                "name": name,
                "command": ["cmake", "-DTARGET_NAME=transaction_test",
                            f"-DTARGET_FILE={self.binary}"],
                "properties": [{"name": "LABELS", "value": ["module:transaction"]}],
            }
            for name in sorted(transaction.REQUIRED_TESTS)
        ]

    def invoke(self, *, status=0, cases=None, build_only=False):
        if cases is None:
            cases = {"TransactionWaveAClosureTests": "passed"}

        def execute(command, **kwargs):
            if command[0] == "ctest":
                self.assertEqual(kwargs["env"]["THEMIS_CTEST_DISABLE_ON_DEMAND_BUILD"], "1")
                self.assertEqual(kwargs["env"]["THEMIS_CTEST_STRICT_EVIDENCE"], "1")
                self.assertIn("--no-tests=error", command)
                suite = ET.Element("testsuite")
                for name, result in cases.items():
                    case = ET.SubElement(suite, "testcase", name=name)
                    if result != "passed":
                        ET.SubElement(case, result)
                ET.ElementTree(suite).write(self.root / "evidence/results.xml")
                return subprocess.CompletedProcess(command, status)
            return subprocess.CompletedProcess(command, 0)

        with patch.object(transaction.evidence, "_read_expected_tests", return_value=self.inventory):
            with patch.object(transaction.subprocess, "run", side_effect=execute) as run:
                with contextlib.redirect_stdout(io.StringIO()):
                    result = transaction.run(
                        self.root / "build", "smoke", self.root / "evidence",
                        parallel=2, build_only=build_only, commit_sha="abc", run_url="https://example.invalid/1",
                    )
                return result, run

    def test_requires_each_enabled_suite_not_just_nonempty_inventory(self):
        for inventory in ([], self.inventory[1:]):
            with self.subTest(inventory=inventory):
                with self.assertRaisesRegex(ValueError, "Missing enabled"):
                    transaction.select_tests(inventory, "full")
        self.inventory[0]["properties"].append({"name": "DISABLED", "value": True})
        with self.assertRaisesRegex(ValueError, "Missing enabled"):
            transaction.select_tests(self.inventory, "full")

    def test_full_scope_includes_other_transaction_registrations(self):
        extra = {**self.inventory[0], "name": "AdditionalTransactionTests"}
        selected = transaction.select_tests([*self.inventory, extra], "full")
        self.assertIn(extra, selected)
        self.assertEqual(len(transaction.select_tests(self.inventory, "smoke")), 1)

    def test_missing_target_metadata_and_duplicate_names_fail(self):
        broken = {**self.inventory[0], "command": [str(self.binary)]}
        with self.assertRaisesRegex(ValueError, "metadata"):
            transaction.select_tests([broken, *self.inventory[1:]], "full")
        with self.assertRaisesRegex(ValueError, "duplicate"):
            transaction.select_tests([*self.inventory, self.inventory[0]], "full")

    def test_build_precedes_ctest_and_valid_junit_passes(self):
        result, calls = self.invoke()
        self.assertEqual(result, 0)
        self.assertEqual([call.args[0][0] for call in calls.call_args_list], ["cmake", "ctest"])
        self.assertIn("transaction_test", calls.call_args_list[0].args[0])
        report = (self.root / "evidence/summary.md").read_text()
        self.assertIn("**Status:** PASS", report)
        self.assertIn("`abc`", report)

    def test_build_error_propagates_without_running_ctest(self):
        with patch.object(transaction.evidence, "_read_expected_tests", return_value=self.inventory):
            with patch.object(transaction.subprocess, "run", side_effect=subprocess.CalledProcessError(1, ["cmake"])) as run:
                with self.assertRaises(subprocess.CalledProcessError):
                    transaction.run(
                        self.root, "smoke", self.root / "evidence",
                        parallel=1, build_only=False, commit_sha="abc", run_url="run",
                    )
                self.assertEqual(run.call_count, 1)

    def test_missing_binary_fails_even_after_successful_build(self):
        self.binary.unlink()
        with self.assertRaisesRegex(ValueError, "executable is missing"):
            self.invoke()

    def test_empty_partial_failed_and_skipped_junit_cannot_pass(self):
        for cases in ({}, {"TransactionWaveAClosureTests": "skipped"},
                      {"TransactionWaveAClosureTests": "failure"}, {"WrongSuite": "passed"}):
            with self.subTest(cases=cases):
                self.assertNotEqual(self.invoke(cases=cases)[0], 0)

    def test_stale_junit_is_removed_before_failed_execution(self):
        evidence_dir = self.root / "evidence"
        evidence_dir.mkdir()
        (evidence_dir / "results.xml").write_text("<testsuite/>")
        (evidence_dir / "summary.md").write_text("PASS")
        with patch.object(transaction.evidence, "_read_expected_tests", return_value=[]):
            with self.assertRaises(ValueError):
                transaction.run(self.root, "smoke", evidence_dir, parallel=1, build_only=False,
                                commit_sha="abc", run_url="run")
        self.assertFalse((evidence_dir / "results.xml").exists())
        self.assertFalse((evidence_dir / "summary.md").exists())

    def test_ctest_failure_is_not_overridden_by_successful_junit(self):
        self.assertEqual(self.invoke(status=7)[0], 7)
        self.assertIn("**Status:** FAIL", (self.root / "evidence/summary.md").read_text())

    def test_compile_only_does_not_claim_test_evidence(self):
        result, calls = self.invoke(build_only=True)
        self.assertEqual(result, 0)
        self.assertEqual(calls.call_count, 1)
        self.assertFalse((self.root / "evidence/summary.md").exists())

    def test_real_cmake_ctest_registration_build_and_execution(self):
        source = self.root / "source"
        source.mkdir()
        (source / "test.cpp").write_text("int main() { return 0; }\n")
        (source / "CMakeLists.txt").write_text(
            'cmake_minimum_required(VERSION 3.23)\nproject(TransactionGateFixture CXX)\n'
            f'set(THEMIS_ROOT_DIR "{ROOT.as_posix()}")\n'
            f'include("{ROOT.as_posix()}/tests/cmake/TestPolicy.cmake")\n'
            'enable_testing()\nadd_executable(transaction_test test.cpp)\n'
            'themis_register_test_target(NAME TransactionWaveAClosureTests '
            'TARGET transaction_test LABELS "module:transaction")\n'
        )
        build = self.root / "build"
        subprocess.run(["cmake", "-S", str(source), "-B", str(build)],
                       check=True, capture_output=True, text=True)
        result = subprocess.run(
            [sys.executable, str(ROOT / "tools/ci/run_transaction_tests.py"),
             "--build-dir", str(build), "--suite", "smoke", "--output-dir", str(self.root / "evidence"),
             "--commit-sha", "fixture", "--run-url", "https://example.invalid/fixture"],
            capture_output=True, text=True,
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertTrue((build / "transaction_test").is_file())
        self.assertIn("**Passed:** 1", (self.root / "evidence/summary.md").read_text())
        for output in ("0 tests from 0 test suites ran", "[  SKIPPED ] UnavailableTest"):
            with self.subTest(output=output):
                (source / "test.cpp").write_text(
                    '#include <iostream>\nint main() { std::cout << '
                    + json.dumps(output + "\n") + '; return 0; }\n'
                )
                rejected = subprocess.run(
                    result.args, capture_output=True, text=True,
                )
                self.assertNotEqual(rejected.returncode, 0, rejected.stdout + rejected.stderr)
                self.assertIn("Strict evidence rejects", rejected.stdout)
                self.assertIn("**Status:** FAIL", (self.root / "evidence/summary.md").read_text())
        legacy = subprocess.run(
            ["ctest", "--test-dir", str(build), "--output-on-failure"],
            env={**os.environ, "THEMIS_CTEST_STRICT_EVIDENCE": "0"},
            capture_output=True, text=True,
        )
        self.assertEqual(legacy.returncode, 0, legacy.stdout + legacy.stderr)


class TransactionWorkflowTests(unittest.TestCase):
    def setUp(self):
        self.workflow = yaml.safe_load(
            (ROOT / ".github/workflows/build-wave-b-transaction.yml").read_text()
        )

    def test_embedded_python_syntax(self):
        for job in self.workflow["jobs"].values():
            for step in job.get("steps", []):
                script = step.get("run", "")
                for code in re.findall(r"<<'PY'\n(.*?)\nPY(?:\n|$)", script, re.DOTALL):
                    with self.subTest(step=step["name"]):
                        compile(code, step["name"], "exec")

    def test_baseline_labels_hosted_hardware_and_leaves_signoff_pending(self):
        for scope in ("hosted-runner", "representative-hardware"):
            with self.subTest(scope=scope), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                source = root / "benchmark.json"
                source.write_text(json.dumps({"benchmarks": [{"name": "Txn", "gate_pass": 1.0}]}))
                output = root / "manifest.json"
                markdown = root / "manifest.md"
                arguments = [
                    "generate_transaction_baseline_manifest.py", "--benchmark-json", str(source),
                    "--workflow", "test.yml", "--run-id", "123", "--run-url", "test-url",
                    "--commit-sha", "abc", "--output-json", str(output), "--output-md", str(markdown),
                ]
                if scope == "hosted-runner":
                    arguments += ["--hardware-scope", scope]
                with patch.object(sys, "argv", arguments):
                    self.assertEqual(baseline.main(), 0)
                manifest = json.loads(output.read_text())
                self.assertEqual(manifest["hardware_scope"], scope)
                self.assertEqual(manifest["evidence_scope"], f"wave-b-transaction-{scope}")
                self.assertEqual(manifest["signoff"]["status"], "pending")
                self.assertIn(f"Hardware summary ({scope})", markdown.read_text())
        steps = self.workflow["jobs"]["publish-results"]["steps"]
        generation = next(step for step in steps if step.get("id") == "representative-baseline")
        self.assertIn("--hardware-scope hosted-runner", generation["run"])

    def test_benchmark_gate_rejects_empty_failed_missing_nan_and_error_rows(self):
        steps = self.workflow["jobs"]["phase4-benchmark-baseline"]["steps"]
        script = next(step["run"] for step in steps if step.get("name") == "Render Phase 4 benchmark summary")
        passing = {"name": "TransactionBench", "gate_pass": 1.0}
        for rows, succeeds in (
            ([passing], True), ([], False), ([{"name": "TransactionBench_mean", "gate_pass": 1}], False),
            ([{**passing, "gate_pass": 0.0}], False), ([{"name": "TransactionBench"}], False),
            ([{**passing, "gate_pass": float("nan")}], False),
            ([{**passing, "error_occurred": True}], False),
        ):
            with self.subTest(rows=rows), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                output = root / "transaction-phase4-benchmark"
                output.mkdir()
                (output / "phase4_baseline.json").write_text(json.dumps({"benchmarks": rows}))
                command = script.replace("${{ needs.configure.outputs.build-dir }}", temporary)
                result = subprocess.run(["bash", "-e", "-o", "pipefail", "-c", command],
                                        capture_output=True, text=True)
                self.assertEqual(result.returncode == 0, succeeds, result.stdout + result.stderr)

    def test_successful_jobs_cannot_publish_missing_artifacts(self):
        job = self.workflow["jobs"]["collect-artifacts"]
        script = next(step["run"] for step in job["steps"] if step.get("name") == "Create artifact archive")
        results = {name: {"result": "success"} for name in job["needs"]}
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            env = {**os.environ, "UPSTREAM_RESULTS": json.dumps(results)}
            missing = subprocess.run(["bash", "-e", "-o", "pipefail", "-c", script],
                                     cwd=root, env=env, capture_output=True, text=True)
            self.assertNotEqual(missing.returncode, 0)
            self.assertIn("no evidence artifact", missing.stderr)

            for artifact in ("cmake-configuration-log", "compile-transaction-log",
                             "smoke-test-results", "transaction-test-results",
                             "transaction-chaos-recovery-evidence", "transaction-phase4-benchmark"):
                directory = root / artifact
                directory.mkdir()
                (directory / "execution.log").touch()
                for filename in ("results.xml", "inventory.json", "summary.md"):
                    (directory / filename).touch()
            complete = subprocess.run(["bash", "-e", "-o", "pipefail", "-c", script],
                                      cwd=root, env=env, capture_output=True, text=True)
            self.assertEqual(complete.returncode, 0, complete.stdout + complete.stderr)
            (root / "transaction-test-results/results.xml").unlink()
            incomplete = subprocess.run(["bash", "-e", "-o", "pipefail", "-c", script],
                                        cwd=root, env=env, capture_output=True, text=True)
            self.assertNotEqual(incomplete.returncode, 0)
            self.assertIn("Missing required results.xml", incomplete.stderr)

    def test_closure_requires_all_source_flags_and_collected_artifacts(self):
        steps = self.workflow["jobs"]["publish-results"]["steps"]
        script = next(step["run"] for step in steps if step.get("name") == "Generate Wave-B transaction closure package")
        required = ("configure", "compile-tests", "smoke-tests", "full-transaction-test-suite",
                    "chaos-recovery-evidence", "phase4-benchmark-baseline", "collect-artifacts")
        for unsuccessful in (None, *required):
            with self.subTest(unsuccessful=unsuccessful), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                python = root / "python3"
                python.write_text('#!/bin/sh\nprintf "%s\\n" "$@" > "$CAPTURE_ARGS"\n')
                python.chmod(0o755)
                command = script.replace("${{ github.run_id }}", "123")
                for name in required:
                    command = command.replace("${{ needs." + name + ".result }}",
                                              "skipped" if name == unsuccessful else "success")
                result = subprocess.run(
                    ["bash", "-e", "-o", "pipefail", "-c", command], cwd=root,
                    env={**os.environ, "PATH": f"{root}:{os.environ['PATH']}",
                         "CAPTURE_ARGS": str(root / "args")},
                    capture_output=True, text=True,
                )
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                arguments = (root / "args").read_text().splitlines()
                self.assertEqual(arguments[arguments.index("--overall-status") + 1],
                                 "evidence-captured" if unsuccessful is None else "open")

    def test_required_jobs_are_not_skipped_on_schedule_or_partial_dispatch(self):
        workflow = yaml.safe_load((ROOT / ".github/workflows/build-wave-b-transaction.yml").read_text())
        self.assertEqual(workflow["defaults"]["run"]["shell"], "bash")
        self.assertEqual(workflow["env"]["THEMIS_CTEST_DISABLE_ON_DEMAND_BUILD"], "1")
        jobs = workflow["jobs"]
        for name in ("smoke-tests", "full-transaction-test-suite", "chaos-recovery-evidence"):
            self.assertNotIn("if", jobs[name])
            run = "\n".join(step.get("run", "") for step in jobs[name]["steps"])
            self.assertIn("run_transaction_tests.py", run)
        self.assertNotIn("strategy", jobs["compile-tests"])
        self.assertIn("full-transaction-test-suite", jobs["collect-artifacts"]["needs"])
        self.assertIn("github.event_name == 'schedule'", jobs["phase4-benchmark-baseline"]["if"])

    def test_result_gate_rejects_every_required_job_skip_failure_and_cancellation(self):
        workflow = yaml.safe_load((ROOT / ".github/workflows/build-wave-b-transaction.yml").read_text())
        gate = workflow["jobs"]["check-results"]
        script = gate["steps"][0]["run"]
        results = {name: {"result": "success"} for name in gate["needs"]}

        def check(required_baseline):
            return subprocess.run(
                ["bash", "-e", "-o", "pipefail", "-c", script],
                env={**os.environ, "JOB_RESULTS": json.dumps(results),
                     "REQUIRE_BASELINE": str(required_baseline).lower()},
                capture_output=True, text=True,
            ).returncode

        self.assertEqual(check(True), 0)
        for name in results:
            for status in ("skipped", "failure", "cancelled"):
                with self.subTest(name=name, status=status):
                    results[name]["result"] = status
                    self.assertNotEqual(check(True), 0)
                    results[name]["result"] = "success"
        results["phase4-benchmark-baseline"]["result"] = "skipped"
        self.assertEqual(check(False), 0)

    def test_shell_pipeline_failure_is_not_masked_by_tee(self):
        result = subprocess.run(["bash", "-e", "-o", "pipefail", "-c", "false | tee /dev/null"])
        self.assertNotEqual(result.returncode, 0)


if __name__ == "__main__":
    unittest.main()
