"""Exercise the actual CTest and PowerShell workload identity resolvers."""

import importlib.util
import json
from contextlib import contextmanager
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import unittest


BENCHMARKS = Path(__file__).resolve().parents[1]
RUNNER = BENCHMARKS / "cmake" / "benchmark_run_manifest.py"
RESOLVER = BENCHMARKS / "workload_link_resolver.ps1"
spec = importlib.util.spec_from_file_location("benchmark_run_manifest_links", RUNNER)
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)

@contextmanager
def workspace_directory(prefix):
    path = Path(tempfile.mkdtemp(prefix=prefix, dir=BENCHMARKS))
    try:
        yield path
    finally:
        for attempt in range(100):
            try:
                shutil.rmtree(path)
                break
            except PermissionError:
                if attempt == 99:
                    raise
                time.sleep(0.1)


class WorkloadResolverTests(unittest.TestCase):
    def test_ctest_runner_records_only_exact_cases(self):
        if not shutil.which("ctest"):
            self.skipTest("ctest is unavailable")
        mapping = json.loads((BENCHMARKS / "benchmark_suite_map.json").read_text(encoding="utf-8"))
        link = mapping["observed_case_workload_links"]["bench_mmdb"]
        case = link["exact_observed_cases"][0]
        with workspace_directory("link-ctest-") as root:
            results = root / "results"
            (results / ".manifest").mkdir(parents=True)
            lease = root / "results.lease"
            lease.mkdir()
            (lease / "owner").write_text('{"run_id":"test"}', encoding="utf-8")
            (results / "manifest").write_text(json.dumps({
                "run_id": "test", "tests": [{"id": "test", "result": "result.json",
                    "target": "bench_mmdb", "executable": sys.executable}]}), encoding="utf-8")
            fake = root / "fake.py"
            fake.write_text(
                "import json,sys\n"
                "out=next(a.split('=',1)[1] for a in sys.argv if a.startswith('--benchmark_out='))\n"
                f"names={repr([case, case.lower(), 'IndexedProductLookup', 'MMDBFixture/IndexedProductLookup/1000'])}\n"
                "open(out,'w',encoding='utf-8').write(json.dumps({'benchmarks':[{'name':n} for n in names]}))\n",
                encoding="utf-8")
            command = [sys.executable, str(RUNNER), "run", "--results-dir", str(results),
                       "--id", "test", "--timeout", "30", "--", sys.executable,
                       str(fake), "--benchmark_out=ignored"]
            (root / "CTestTestfile.cmake").write_text(
                "add_test(link " + " ".join(json.dumps(arg) for arg in command) + ")\n",
                encoding="utf-8")
            completed = subprocess.run(["ctest", "--test-dir", str(root), "--output-on-failure"],
                                       capture_output=True, text=True)
            self.assertEqual(completed.returncode, 0, completed.stdout + completed.stderr)
            fragment = json.loads((results / ".manifest" / "test").read_text(encoding="utf-8"))
            self.assertEqual([row["workload_id"] for row in fragment["cases"]],
                             [link["workload_id"], None, None, None])

    def test_powershell_and_python_use_same_validated_catalog(self):
        shells = [p for name in ("pwsh", "powershell") if (p := shutil.which(name))]
        if not shells:
            self.skipTest("PowerShell is unavailable")
        mapping = json.loads((BENCHMARKS / "benchmark_suite_map.json").read_text(encoding="utf-8"))
        catalog = json.loads((BENCHMARKS / "workload_profiles.json").read_text(encoding="utf-8"))
        queries = []
        for target, entry in mapping["observed_case_workload_links"].items():
            for case in entry["exact_observed_cases"]:
                queries.append([target + ".exe", case, entry["workload_id"]])
                queries.append([target.upper() + ".exe", case, None])
                queries.append([target + ".exe", case.lower(), None])
        queries.extend([
            ["bench_mmdb.exe", "IndexedProductLookup", None],
            ["bench_mmdb.exe", "IndexedProductLookup/1000", None],
            ["bench_mmdb.exe", "MMDBFixture/IndexedProductLookup/1000", None],
            ["bench_p0_crud_baseline.exe", "BM_P0_InsertHeavy", None],
            ["unknown.exe", "unknown", None],
        ])
        with workspace_directory("link-resolver-") as root:
            driver = root / "test.ps1"
            driver.write_text(
                "param($resolver, $directory, $queries)\n"
                ". $resolver\n"
                "$rows = (Get-Content -Raw -LiteralPath $queries | ConvertFrom-Json).queries\n"
                "$results = @($rows | ForEach-Object { "
                "$value = Resolve-BenchmarkWorkloadId -ExecutableName $_[0] -ObservedName $_[1] -CatalogDirectory $directory; "
                "if ($null -eq $value) { '<null>' } else { $value } })\n"
                "ConvertTo-Json -InputObject $results -Compress\n",
                encoding="utf-8")
            def check(directory, rows):
                (root / "queries.json").write_text(json.dumps({"queries": rows}), encoding="utf-8")
                links = runner.load_workload_links(directory)
                self.assertEqual([runner.resolve_workload_id(a, b, links) for a, b, _ in rows],
                                 [expected for _, _, expected in rows])
                for shell in shells:
                    with self.subTest(shell=shell, directory=directory.name):
                        result = subprocess.run([shell, "-NoProfile", "-File", str(driver),
                                                 str(RESOLVER), str(directory), str(root / "queries.json")],
                                                capture_output=True, text=True)
                        self.assertEqual(result.returncode, 0, result.stderr)
                        self.assertEqual(json.loads(result.stdout),
                                         [expected if expected is not None else "<null>" for _, _, expected in rows])

            check(BENCHMARKS, queries)
            (root / "workload_profiles.json").write_text(json.dumps(catalog), encoding="utf-8")
            cases = ["SameCase"]
            first, second = [p["workload_id"] for p in catalog["profiles"][:2]]
            mapping["observed_case_workload_links"] = {
                "fixture": [{"workload_id": first, "exact_observed_cases": cases},
                            {"workload_id": second, "exact_observed_cases": cases}]}
            map_path = root / "benchmark_suite_map.json"
            map_path.write_text(json.dumps(mapping), encoding="utf-8")
            check(root, [["fixture.exe", "SameCase", None]])
            mapping["observed_case_workload_links"]["fixture"].pop()
            map_path.write_text(json.dumps(mapping), encoding="utf-8")
            check(root, [["fixture.exe", "SameCase", first], ["fixture.exe", "samecase", None]])
            mapping["observed_case_workload_links"]["fixture"][0]["workload_id"] = "missing"
            map_path.write_text(json.dumps(mapping), encoding="utf-8")
            check(root, [["fixture.exe", "SameCase", None]])
            mapping["observed_case_workload_links"]["fixture"][0]["workload_id"] = first
            mapping["observed_case_workload_links"]["fixture"][0]["exact_observed_cases"] = ["Changed"]
            map_path.write_text(json.dumps(mapping), encoding="utf-8")
            check(root, [["fixture.exe", "SameCase", None], ["fixture.exe", "Changed", first]])
            next_mapping = json.loads(json.dumps(mapping))
            next_mapping["observed_case_workload_links"]["fixture"][0]["exact_observed_cases"] = ["Next"]
            (root / "next.json").write_text(json.dumps(next_mapping), encoding="utf-8")
            driver.write_text(
                "param($resolver, $directory)\n. $resolver\n"
                "$a = Resolve-BenchmarkWorkloadId fixture.exe Changed -CatalogDirectory $directory\n"
                "Copy-Item -LiteralPath (Join-Path $directory 'next.json') -Destination (Join-Path $directory 'benchmark_suite_map.json') -Force\n"
                "$changed = Resolve-BenchmarkWorkloadId fixture.exe Changed -CatalogDirectory $directory\n"
                "$next = Resolve-BenchmarkWorkloadId fixture.exe Next -CatalogDirectory $directory\n"
                "Remove-Item -LiteralPath (Join-Path $directory 'benchmark_suite_map.json')\n"
                "$b = Resolve-BenchmarkWorkloadId fixture.exe Next -CatalogDirectory $directory\n"
                "ConvertTo-Json -InputObject @($a, $changed, $next, $b) -Compress\n", encoding="utf-8")
            for shell in shells:
                map_path.write_text(json.dumps(mapping), encoding="utf-8")
                result = subprocess.run([shell, "-NoProfile", "-File", str(driver),
                                         str(RESOLVER), str(root)], capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(json.loads(result.stdout), [first, None, first, None])
            driver.write_text(
                "param($resolver, $directory, $queries)\n. $resolver\n"
                "$rows = (Get-Content -Raw -LiteralPath $queries | ConvertFrom-Json).queries\n"
                "$results = @($rows | ForEach-Object { "
                "$value = Resolve-BenchmarkWorkloadId -ExecutableName $_[0] -ObservedName $_[1] -CatalogDirectory $directory; "
                "if ($null -eq $value) { '<null>' } else { $value } })\n"
                "ConvertTo-Json -InputObject $results -Compress\n", encoding="utf-8")
            text = json.dumps(mapping)
            map_path.write_text(text.replace('"workload_profile_catalog":',
                                             '"workload_profile_catalog": "invalid", "workload_profile_catalog":', 1),
                                encoding="utf-8")
            check(root, [["fixture.exe", "Changed", None]])
            map_path.unlink()
            check(root, [["fixture.exe", "SameCase", None]])


if __name__ == "__main__":
    unittest.main()
