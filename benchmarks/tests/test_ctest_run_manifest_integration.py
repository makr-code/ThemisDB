"""End-to-end CTest coverage for benchmark run manifests."""

import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import unittest


ROOT = Path(__file__).resolve().parents[2]
POLICY = ROOT / "benchmarks" / "cmake" / "BenchmarkPolicy.cmake"
RUNNER = POLICY.with_name("benchmark_run_manifest.py")


@unittest.skipIf(os.name == "nt", "The integration fixture uses a POSIX executable script and process groups")
class CTestManifestIntegration(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary_directory = tempfile.TemporaryDirectory(prefix="themis-benchmark-manifest-")
        cls.addClassCleanup(cls.temporary_directory.cleanup)
        cls.work = Path(cls.temporary_directory.name)
        cls.addClassCleanup(shutil.rmtree, cls.work, ignore_errors=True)
        cls.build = cls.work / "build"
        cls.src = cls.work / "src"
        (cls.src / "module").mkdir(parents=True)
        (cls.src / "module" / "PERFORMANCE_EXPECTATIONS.md").write_text(
            "## Specific Expectations\n\n| Target ID | Expectation | Benchmark case |\n"
            "|---|---|---|\n| E1 | > 1 ops/s | BM_Ok |\n"
            "| E2 | > 1 ops/s | BM_Unseen |\n"
            "| E3 | > 1 ops/s | BM_Error |\n", encoding="utf-8")
        benchmark = cls.work / "fake_benchmark.py"
        benchmark.write_text('''#!/usr/bin/env python3
import json, pathlib, subprocess, sys, time
out = next(a.split("=", 1)[1] for a in sys.argv if a.startswith("--benchmark_out="))
mode = next((a.split("=", 1)[1] for a in sys.argv if a.startswith("--mode=")), "good")
print("benchmark stderr " + mode, file=sys.stderr)
if mode == "timeout":
    marker = pathlib.Path(__file__).parent / "descendant"
    subprocess.Popen([sys.executable, "-c", "import time; from pathlib import Path; time.sleep(3); Path(" + repr(str(marker)) + ").write_text('alive')"])
    time.sleep(10)
elif mode == "missing":
    pass
elif mode == "invalid":
    pathlib.Path(out).write_text("{")
elif mode == "empty":
    pathlib.Path(out).write_text('{"benchmarks":[]}')
else:
    row = {"name": "BM_Ok/42"}
    if mode == "error":
        row = {"name": "BM_Error", "error_occurred": True, "error_message": "unavailable"}
    pathlib.Path(out).write_text(json.dumps({"benchmarks": [row]}))
    if mode == "fail":
        sys.exit(7)
''', encoding="utf-8")
        benchmark.chmod(0o755)
        comparator = cls.work / "postprocess.py"
        comparator.write_text('''import pathlib, sys
root = pathlib.Path(sys.argv[sys.argv.index("--results-dir") + 1])
assert all(p.name not in ("manifest", "owner") for p in root.rglob("*.json"))
pathlib.Path(sys.argv[sys.argv.index("--output") + 1]).write_text("postprocessed")
''', encoding="utf-8")
        cmake = f'''cmake_minimum_required(VERSION 3.20)
project(ManifestFixture NONE)
enable_testing()
set(THEMIS_BENCHMARK_ROOT "{cls.work}")
include("{POLICY}")
add_executable(fake IMPORTED GLOBAL)
set_target_properties(fake PROPERTIES IMPORTED_LOCATION "{benchmark}")
add_executable(absent IMPORTED GLOBAL)
set_target_properties(absent PROPERTIES IMPORTED_LOCATION "{cls.work / 'absent'}")
'''
        for name, mode in [("good", "good"), ("error", "error"), ("fail", "fail"),
                           ("missing", "missing"), ("invalid", "invalid"), ("empty", "empty"),
                           ("timeout", "timeout"), ("unsafe/../name", "good")]:
            cmake += (f'themis_register_benchmark_ctest(NAME "{name}" TARGET fake MODULE m '
                      f'TIMEOUT 1 EXTRA_ARGS "--mode={mode}")\n')
        cmake += '''themis_register_benchmark_ctest(NAME missing_binary TARGET absent MODULE m)
themis_register_benchmark_ctest(NAME unavailable TARGET no_such_target MODULE m)
'''
        cmake += f'''themis_register_benchmark_postprocess_ctest(
 NAME post SCRIPT "{comparator}" RESULTS_DIR "${{CMAKE_BINARY_DIR}}/bench_results/current"
 ARCHIVE_DIR "${{CMAKE_BINARY_DIR}}/bench_results/archive"
 REPORT_PATH "${{CMAKE_BINARY_DIR}}/bench_results/current/report.md")
'''
        (cls.work / "CMakeLists.txt").write_text(cmake, encoding="utf-8")
        subprocess.run(["cmake", "-S", str(cls.work), "-B", str(cls.build)],
                       check=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        cls.results = cls.build / "bench_results" / "current"

    def ctest(self, *args):
        return subprocess.run(["ctest", "--test-dir", str(self.build), "--output-on-failure", *args],
                              text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    def test_ctest_lifecycle_mapping_and_archive(self):
        first = self.ctest("-j", "8")
        self.assertNotEqual(first.returncode, 0, first.stdout)
        self.assertIn("benchmark stderr fail", first.stdout)
        manifest = json.loads((self.results / "manifest").read_text())
        self.assertEqual(manifest["status"], "finished")
        self.assertGreater(manifest["elapsed_seconds"], 0)
        tests = {t["test"]: t for t in manifest["tests"]}
        self.assertEqual(tests["good"]["execution_status"], "completed")
        self.assertEqual(tests["good"]["json_status"], "valid")
        self.assertEqual(Path(tests["good"]["result"]).name, "good.json")
        self.assertIsNone(tests["good"]["cases"][0]["elapsed_seconds"])
        self.assertEqual(tests["error"]["cases"][0]["error_message"], "unavailable")
        self.assertEqual(tests["fail"]["exit_code"], 7)
        self.assertEqual(tests["fail"]["execution_status"], "process_failure")
        self.assertFalse((self.results / tests["fail"]["result"]).exists())
        self.assertEqual(tests["missing"]["json_status"], "missing")
        self.assertEqual(tests["invalid"]["json_status"], "invalid")
        self.assertEqual(tests["empty"]["json_status"], "invalid")
        self.assertEqual(tests["timeout"]["execution_status"], "timeout")
        self.assertEqual(tests["missing_binary"]["execution_status"], "not_built")
        self.assertEqual(tests["unavailable"]["registration_status"], "not_available")
        self.assertEqual(tests["unavailable"]["execution_status"], "not_executed")
        self.assertEqual(Path(tests["unsafe/../name"]["result"]).parent.name, ".unsafe")
        observed, unseen, error = manifest["expectations"]
        self.assertEqual(observed["mapping_status"], "observed")
        self.assertEqual(unseen["mapping_status"], "unknown")
        self.assertTrue(any(x["case"] == "BM_Ok/42" and x["execution_status"] == "process_failure"
                            for x in observed["observations"]))
        self.assertEqual(error["observations"][0]["case_status"], "error")
        self.assertTrue((self.results / "report.md").exists())
        time.sleep(3)
        self.assertFalse((self.work / "descendant").exists())

        second = self.ctest("-R", "^good$")
        self.assertEqual(second.returncode, 0, second.stdout)
        next_manifest = json.loads((self.results / "manifest").read_text())
        self.assertNotEqual(manifest["run_id"], next_manifest["run_id"])
        next_tests = {t["test"]: t for t in next_manifest["tests"]}
        self.assertEqual(next_tests["missing_binary"]["build_status"], "missing")
        self.assertEqual(next_tests["missing_binary"]["execution_status"], "unknown")
        self.assertFalse((self.results / "error.json").exists())
        archives = list((self.build / "bench_results" / "archive").glob("ctest_*"))
        self.assertEqual(len(archives), 1)
        self.assertEqual(json.loads((archives[0] / "manifest").read_text())["run_id"], manifest["run_id"])
        self.assertTrue((archives[0] / tests["good"]["result_relative"]).exists())
        self.assertTrue((archives[0] / tests["fail"]["observation_relative"]).exists())

        lease = self.results.with_name("current.lease")
        lease.mkdir()
        (lease / "owner").write_text(json.dumps({"ctest_pid": -1, "run_id": "other"}))
        blocked = self.ctest("-R", "^good$")
        self.assertNotEqual(blocked.returncode, 0, blocked.stdout)
        self.assertEqual(json.loads((self.results / "manifest").read_text())["run_id"], next_manifest["run_id"])
        self.assertTrue(lease.exists())
        shutil.rmtree(lease)

    def test_interrupted_wrapper_leaves_unknown_execution(self):
        registry = self.build / "bench_results" / "registry-.json"
        prepare = ["prepare", "--results-dir", str(self.results), "--archive-dir",
                   str(self.build / "bench_results" / "archive"), "--registry", str(registry),
                   "--cmake", shutil.which("cmake"), "--prepare-script",
                   str(POLICY.with_name("PrepareBenchmarkResults.cmake"))]
        subprocess.run([sys.executable, str(RUNNER), *prepare], check=True,
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        manifest = json.loads((self.results / "manifest").read_text())
        entry = next(item for item in manifest["tests"] if item["test"] == "timeout")
        command = [sys.executable, str(RUNNER), "run", "--results-dir", str(self.results),
                   "--id", entry["id"], "--timeout", "20", "--", entry["executable"],
                   "--benchmark_out=ignored", "--mode=timeout"]
        runner = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        fragment = self.results / ".manifest" / entry["id"]
        try:
            for _ in range(100):
                if fragment.exists():
                    break
                time.sleep(0.02)
            self.assertTrue(fragment.exists())
            time.sleep(0.2)
            runner.terminate()
            runner.communicate(timeout=10)
            self.assertNotEqual(runner.returncode, 0)
        finally:
            if runner.poll() is None:
                runner.kill()
                runner.communicate()
        cleanup = subprocess.run([sys.executable, str(RUNNER), "finish", "--results-dir",
                                  str(self.results), "--src-dir", str(self.src), "--",
                                  sys.executable, "-c", "pass"], capture_output=True, text=True)
        self.assertEqual(cleanup.returncode, 0, cleanup.stderr)
        manifest = json.loads((self.results / "manifest").read_text())
        entry = next(item for item in manifest["tests"] if item["test"] == "timeout")
        self.assertEqual(entry["execution_status"], "unknown")
        self.assertEqual(entry["json_status"], "unknown")
        self.assertFalse((self.results / entry["result"]).exists())

    def test_overlapping_ctest_invocations_fail_closed(self):
        command = ["ctest", "--test-dir", str(self.build), "--output-on-failure", "-R", "^timeout$"]
        first = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        lease = self.results.with_name("current.lease")
        try:
            for _ in range(100):
                if lease.exists():
                    break
                if first.poll() is not None:
                    self.fail("First CTest exited before taking the lease")
                time.sleep(0.01)
            self.assertTrue(lease.exists())
            second = self.ctest("-R", "^good$")
            self.assertNotEqual(second.returncode, 0, second.stdout)
            self.assertIn("leased", second.stdout)
            output, _ = first.communicate(timeout=20)
            self.assertNotEqual(first.returncode, 0, output)
            manifest = json.loads((self.results / "manifest").read_text())
            entry = next(item for item in manifest["tests"] if item["test"] == "timeout")
            self.assertEqual(entry["execution_status"], "timeout")
            self.assertFalse(lease.exists())
        finally:
            if first.poll() is None:
                first.terminate()
                first.communicate(timeout=20)

    def test_ctest_repeat_keeps_attempts(self):
        result = self.ctest("-R", "^good$", "--repeat", "until-fail:2")
        self.assertEqual(result.returncode, 0, result.stdout)
        manifest = json.loads((self.results / "manifest").read_text())
        entry = next(item for item in manifest["tests"] if item["test"] == "good")
        self.assertEqual(entry["execution_status"], "completed")
        self.assertEqual(len(entry["attempts"]), 1)
        self.assertEqual(entry["attempts"][0]["execution_status"], "completed")
        self.assertTrue((self.results / entry["attempts"][0]["observation_relative"]).exists())

    def test_explicit_case_error_fails_ctest(self):
        result = self.ctest("-R", "^error$")
        self.assertNotEqual(result.returncode, 0, result.stdout)
        manifest = json.loads((self.results / "manifest").read_text())
        entry = next(item for item in manifest["tests"] if item["test"] == "error")
        self.assertEqual(entry["exit_code"], 0)
        self.assertEqual(entry["cases"][0]["status"], "error")


class BenchmarkRunCaseErrorTests(unittest.TestCase):
    def test_case_error_returns_failure_and_preserves_json(self):
        with tempfile.TemporaryDirectory(prefix="themis-benchmark-case-error-") as directory:
            root = Path(directory)
            results = root / "current"
            (results / ".manifest").mkdir(parents=True)
            lease = root / "current.lease"
            lease.mkdir()
            run_id = "case-error-run"
            (lease / "owner").write_text(
                json.dumps({"ctest_pid": os.getpid(), "run_id": run_id}), encoding="utf-8")
            (results / "manifest").write_text(json.dumps({
                "run_id": run_id,
                "tests": [{"id": "test-id", "test": "case-error",
                           "executable": sys.executable, "result": "case-error.json"}],
            }), encoding="utf-8")

            benchmark = root / "fake_benchmark.py"
            benchmark.write_text(
                "import json, pathlib, sys\n"
                "output = next(arg.split('=', 1)[1] for arg in sys.argv "
                "if arg.startswith('--benchmark_out='))\n"
                "pathlib.Path(output).write_text(json.dumps({'benchmarks': [{"
                "'name': 'BM_Failed', 'error_occurred': True, "
                "'error_message': 'expected failure'}]}))\n",
                encoding="utf-8",
            )
            result = subprocess.run(
                [sys.executable, str(RUNNER), "run", "--results-dir", str(results),
                 "--id", "test-id", "--timeout", "5", "--", sys.executable,
                 str(benchmark), "--benchmark_out=ignored"],
                capture_output=True, text=True,
            )

            self.assertNotEqual(result.returncode, 0, result.stdout)
            self.assertIn("BM_Failed: expected failure", result.stderr)
            fragment = json.loads((results / ".manifest" / "test-id").read_text())
            self.assertEqual(fragment["execution_status"], "completed")
            self.assertEqual(fragment["cases"][0]["status"], "error")
            self.assertTrue((results / "case-error.json").exists())

    def test_skip_with_message_returns_success_and_preserves_skip_status(self):
        with tempfile.TemporaryDirectory(prefix="themis-benchmark-case-skip-") as directory:
            root = Path(directory)
            results = root / "current"
            (results / ".manifest").mkdir(parents=True)
            lease = root / "current.lease"
            lease.mkdir()
            run_id = "case-skip-run"
            (lease / "owner").write_text(
                json.dumps({"ctest_pid": os.getpid(), "run_id": run_id}), encoding="utf-8")
            (results / "manifest").write_text(json.dumps({
                "run_id": run_id,
                "tests": [{"id": "test-id", "test": "case-skip",
                           "executable": sys.executable, "result": "case-skip.json"}],
            }), encoding="utf-8")

            benchmark = root / "fake_benchmark.py"
            benchmark.write_text(
                "import json, pathlib, sys\n"
                "output = next(arg.split('=', 1)[1] for arg in sys.argv "
                "if arg.startswith('--benchmark_out='))\n"
                "pathlib.Path(output).write_text(json.dumps({'benchmarks': [{"
                "'name': 'BM_Skipped', 'skipped': True, "
                "'skip_message': 'optional backend unavailable'}]}))\n",
                encoding="utf-8",
            )
            result = subprocess.run(
                [sys.executable, str(RUNNER), "run", "--results-dir", str(results),
                 "--id", "test-id", "--timeout", "5", "--", sys.executable,
                 str(benchmark), "--benchmark_out=ignored"],
                capture_output=True, text=True,
            )

            self.assertEqual(result.returncode, 0, result.stderr)
            fragment = json.loads((results / ".manifest" / "test-id").read_text())
            self.assertEqual(fragment["execution_status"], "completed")
            self.assertEqual(fragment["cases"][0]["status"], "skipped")
            self.assertEqual(fragment["cases"][0]["skip_message"],
                             "optional backend unavailable")
            self.assertTrue((results / "case-skip.json").exists())

class BenchmarkJsonStrictnessTests(unittest.TestCase):
    def test_nonstandard_infinity_is_invalid_json(self):
        spec = importlib.util.spec_from_file_location("benchmark_run_manifest", RUNNER)
        runner = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(runner)

        with tempfile.TemporaryDirectory(prefix="themis-benchmark-json-") as directory:
            result = Path(directory) / "benchmark.json"
            result.write_text(
                '{"benchmarks":[{"name":"BM_ZeroCpu","cpu_time":0,'
                '"items_per_second":Infinity}]}',
                encoding="utf-8")
            status, cases = runner.read_result(result)

        self.assertEqual(status, "invalid")
        self.assertEqual(cases, [])


if __name__ == "__main__":
    unittest.main()
