"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_bench_coverage_report.py                      ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-04-15 18:52:34                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     487                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a3ee0f257e  2026-04-15  test(bench): add unit tests for nightly all-module benchm... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Unit tests for tools/bench_coverage_report.py

Covers:
  BCR-01  _bench_name_to_module: known module patterns are mapped correctly
  BCR-02  _bench_name_to_module: unrecognised names return None
  BCR-03  _summarise_bench_file: empty benchmark list handled gracefully
  BCR-04  _summarise_bench_file: normal run computes mean CPU time
  BCR-05  _summarise_bench_file: error_occurred flag is counted
  BCR-06  _summarise_bench_file: aggregate entries are excluded from mean
  BCR-07  _delta_str: positive and negative deltas are formatted correctly
  BCR-08  _delta_str: None inputs yield 'n/a'
  BCR-09  _delta_str: zero previous value yields 'n/a' (division guard)
  BCR-10  _traffic_light: uncovered module returns GREY
  BCR-11  _traffic_light: clean run returns GREEN
  BCR-12  _traffic_light: error in output returns RED
  BCR-13  _traffic_light: delta > 10 % returns RED
  BCR-14  _traffic_light: delta 5–10 % returns YELLOW
  BCR-15  collect_module_data: JSON files are parsed and assigned to modules
  BCR-16  collect_module_data: previous run files are used for delta
  BCR-17  collect_module_data: missing previous dir is handled gracefully
  BCR-18  collect_module_data: unrecognised bench file names are ignored
  BCR-19  build_markdown_report: report contains header and all module rows
  BCR-20  build_json_summary: JSON has correct structure and covered count
  BCR-21  main(): end-to-end run writes both output files
  BCR-22  main(): run without previous dir produces 'n/a' delta column

Run with:  python3 -m pytest tests/test_bench_coverage_report.py -v
"""

from __future__ import annotations

import json
import sys
import os
from pathlib import Path

# Make the tools directory importable
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "tools"))

import bench_coverage_report as bcr

# ---------------------------------------------------------------------------
# Fixtures / helpers
# ---------------------------------------------------------------------------

def _make_bench_json(
    tmp_path: Path,
    name: str,
    benchmarks: list[dict],
) -> Path:
    """Write a minimal Google Benchmark JSON file."""
    p = tmp_path / name
    p.write_text(json.dumps({"benchmarks": benchmarks}), encoding="utf-8")
    return p


def _bench_entry(
    cpu_time: float = 100.0,
    error_occurred: bool = False,
    run_type: str = "iteration",
) -> dict:
    entry: dict = {"cpu_time": cpu_time, "run_type": run_type}
    if error_occurred:
        entry["error_occurred"] = True
    return entry


# ---------------------------------------------------------------------------
# BCR-01 / BCR-02  _bench_name_to_module
# ---------------------------------------------------------------------------

class TestBenchNameToModule:

    def test_bcr_01_storage_pattern(self):
        assert bcr._bench_name_to_module("bench_storage_latency") == 2

    def test_bcr_01_rocksdb_pattern(self):
        assert bcr._bench_name_to_module("bench_rocksdb_compaction") == 2

    def test_bcr_01_index_pattern(self):
        assert bcr._bench_name_to_module("bench_inverted_index") == 3

    def test_bcr_01_query_pattern(self):
        assert bcr._bench_name_to_module("bench_aql_parser") == 4

    def test_bcr_01_graph_traversal(self):
        assert bcr._bench_name_to_module("bench_graph_bfs") == 5

    def test_bcr_01_transaction(self):
        assert bcr._bench_name_to_module("bench_tx_commit") == 6

    def test_bcr_01_llm(self):
        assert bcr._bench_name_to_module("bench_llm_inference") == 17

    def test_bcr_01_chimera(self):
        assert bcr._bench_name_to_module("bench_chimera_suite") == 27

    def test_bcr_01_tpcc(self):
        assert bcr._bench_name_to_module("bench_tpcc") == 33

    def test_bcr_02_unknown_returns_none(self):
        assert bcr._bench_name_to_module("bench_completely_unknown_xyz") is None

    def test_bcr_02_empty_string_returns_none(self):
        assert bcr._bench_name_to_module("") is None


# ---------------------------------------------------------------------------
# BCR-03 / BCR-04 / BCR-05 / BCR-06  _summarise_bench_file
# ---------------------------------------------------------------------------

class TestSummariseBenchFile:

    def test_bcr_03_empty_benchmarks_list(self):
        result = bcr._summarise_bench_file({"benchmarks": []})
        assert result == {"count": 0, "error_count": 0, "mean_cpu_ns": None}

    def test_bcr_03_missing_benchmarks_key(self):
        result = bcr._summarise_bench_file({})
        assert result["count"] == 0
        assert result["mean_cpu_ns"] is None

    def test_bcr_04_single_benchmark(self):
        data = {"benchmarks": [_bench_entry(cpu_time=200.0)]}
        result = bcr._summarise_bench_file(data)
        assert result["count"] == 1
        assert result["error_count"] == 0
        assert abs(result["mean_cpu_ns"] - 200.0) < 1e-9

    def test_bcr_04_multiple_benchmarks_mean(self):
        data = {
            "benchmarks": [
                _bench_entry(cpu_time=100.0),
                _bench_entry(cpu_time=300.0),
            ]
        }
        result = bcr._summarise_bench_file(data)
        assert result["count"] == 2
        assert abs(result["mean_cpu_ns"] - 200.0) < 1e-9

    def test_bcr_05_error_occurred_counted(self):
        data = {
            "benchmarks": [
                _bench_entry(cpu_time=100.0, error_occurred=True),
                _bench_entry(cpu_time=200.0),
            ]
        }
        result = bcr._summarise_bench_file(data)
        assert result["error_count"] == 1

    def test_bcr_06_aggregate_entries_excluded_from_mean(self):
        # Aggregate entries (e.g. mean, stddev) must not distort the mean
        data = {
            "benchmarks": [
                _bench_entry(cpu_time=100.0, run_type="iteration"),
                {"cpu_time": 9999.0, "run_type": "aggregate"},
            ]
        }
        result = bcr._summarise_bench_file(data)
        # Only the iteration entry should contribute to the mean
        assert abs(result["mean_cpu_ns"] - 100.0) < 1e-9


# ---------------------------------------------------------------------------
# BCR-07 / BCR-08 / BCR-09  _delta_str
# ---------------------------------------------------------------------------

class TestDeltaStr:

    def test_bcr_07_positive_delta(self):
        s = bcr._delta_str(110.0, 100.0)
        assert "+10.0 %" in s

    def test_bcr_07_negative_delta(self):
        s = bcr._delta_str(90.0, 100.0)
        assert "-10.0 %" in s

    def test_bcr_07_zero_delta(self):
        s = bcr._delta_str(100.0, 100.0)
        assert "+0.0 %" in s

    def test_bcr_08_current_none(self):
        assert bcr._delta_str(None, 100.0) == "n/a"

    def test_bcr_08_previous_none(self):
        assert bcr._delta_str(100.0, None) == "n/a"

    def test_bcr_09_previous_zero(self):
        assert bcr._delta_str(100.0, 0.0) == "n/a"


# ---------------------------------------------------------------------------
# BCR-10 / BCR-11 / BCR-12 / BCR-13 / BCR-14  _traffic_light
# ---------------------------------------------------------------------------

class TestTrafficLight:

    def _module_summary(
        self,
        bench_count: int = 1,
        error_count: int = 0,
        delta_pct: float | None = None,
    ) -> dict:
        return {
            "bench_count": bench_count,
            "error_count": error_count,
            "mean_cpu_ns_delta_pct": delta_pct,
        }

    def test_bcr_10_no_benchmarks_returns_grey(self):
        assert bcr._traffic_light(self._module_summary(bench_count=0)) == bcr.GREY

    def test_bcr_11_clean_run_returns_green(self):
        assert bcr._traffic_light(self._module_summary()) == bcr.GREEN

    def test_bcr_11_clean_run_with_small_delta_returns_green(self):
        assert bcr._traffic_light(self._module_summary(delta_pct=3.0)) == bcr.GREEN

    def test_bcr_12_errors_return_red(self):
        assert bcr._traffic_light(self._module_summary(error_count=1)) == bcr.RED

    def test_bcr_13_large_regression_returns_red(self):
        assert bcr._traffic_light(self._module_summary(delta_pct=10.1)) == bcr.RED

    def test_bcr_13_exactly_ten_percent_returns_red(self):
        # > 10.0 is RED; exactly 10.0 is also > 10.0 (no), should be YELLOW
        assert bcr._traffic_light(self._module_summary(delta_pct=10.0)) == bcr.YELLOW

    def test_bcr_14_minor_regression_returns_yellow(self):
        assert bcr._traffic_light(self._module_summary(delta_pct=7.5)) == bcr.YELLOW

    def test_bcr_14_boundary_5_percent_returns_yellow(self):
        assert bcr._traffic_light(self._module_summary(delta_pct=5.1)) == bcr.YELLOW


# ---------------------------------------------------------------------------
# BCR-15 / BCR-16 / BCR-17 / BCR-18  collect_module_data
# ---------------------------------------------------------------------------

class TestCollectModuleData:

    def test_bcr_15_json_files_are_parsed_and_assigned(self, tmp_path):
        _make_bench_json(
            tmp_path,
            "bench_storage_latency.json",
            [_bench_entry(cpu_time=500.0)],
        )
        modules = bcr.collect_module_data(tmp_path, prev_dir=None)
        storage = modules[2]
        assert storage["bench_count"] == 1
        assert abs(storage["mean_cpu_ns"] - 500.0) < 1e-6
        assert storage["status"] == bcr.GREEN

    def test_bcr_16_previous_run_triggers_delta(self, tmp_path):
        curr_dir = tmp_path / "curr"
        prev_dir = tmp_path / "prev"
        curr_dir.mkdir()
        prev_dir.mkdir()
        _make_bench_json(curr_dir, "bench_storage_latency.json",
                         [_bench_entry(cpu_time=600.0)])
        _make_bench_json(prev_dir, "bench_storage_latency.json",
                         [_bench_entry(cpu_time=500.0)])

        modules = bcr.collect_module_data(curr_dir, prev_dir=prev_dir)
        storage = modules[2]
        # Delta = (600-500)/500 * 100 = 20 % → RED
        assert storage["mean_cpu_ns_delta_pct"] is not None
        assert abs(storage["mean_cpu_ns_delta_pct"] - 20.0) < 0.1
        assert storage["status"] == bcr.RED

    def test_bcr_17_missing_previous_dir_handled(self, tmp_path):
        _make_bench_json(tmp_path, "bench_storage_latency.json",
                         [_bench_entry(cpu_time=300.0)])
        nonexistent = tmp_path / "no_such_dir"
        modules = bcr.collect_module_data(tmp_path, prev_dir=nonexistent)
        storage = modules[2]
        assert storage["mean_cpu_ns_delta_pct"] is None
        assert storage["status"] == bcr.GREEN

    def test_bcr_18_unrecognised_bench_names_are_ignored(self, tmp_path):
        _make_bench_json(tmp_path, "bench_xyz_unknown_module.json",
                         [_bench_entry(cpu_time=100.0)])
        modules = bcr.collect_module_data(tmp_path, prev_dir=None)
        # All modules should remain at 0 bench_count
        assert all(m["bench_count"] == 0 for m in modules.values())

    def test_bcr_15_multiple_files_same_module_accumulate(self, tmp_path):
        _make_bench_json(tmp_path, "bench_rocksdb_compaction.json",
                         [_bench_entry(cpu_time=200.0)])
        _make_bench_json(tmp_path, "bench_storage_latency.json",
                         [_bench_entry(cpu_time=400.0)])
        modules = bcr.collect_module_data(tmp_path, prev_dir=None)
        storage = modules[2]
        assert storage["bench_count"] == 2

    def test_bcr_15_all_modules_initialised(self, tmp_path):
        modules = bcr.collect_module_data(tmp_path, prev_dir=None)
        assert set(modules.keys()) == set(bcr.MODULE_MAP.keys())
        for m in modules.values():
            assert m["status"] == bcr.GREY  # nothing in dir → all uncovered


# ---------------------------------------------------------------------------
# BCR-19  build_markdown_report
# ---------------------------------------------------------------------------

class TestBuildMarkdownReport:

    def _minimal_modules(self) -> dict:
        """Create a module dict with one covered module."""
        modules = {
            mid: {
                "id": mid,
                "name": info["name"],
                "bench_count": 0,
                "error_count": 0,
                "bench_names": [],
                "mean_cpu_ns": None,
                "mean_cpu_ns_prev": None,
                "mean_cpu_ns_delta_pct": None,
                "status": bcr.GREY,
            }
            for mid, info in bcr.MODULE_MAP.items()
        }
        # Mark module 2 (Storage) as covered and healthy
        modules[2]["bench_count"] = 3
        modules[2]["mean_cpu_ns"] = 250.0
        modules[2]["status"] = bcr.GREEN
        return modules

    def test_bcr_19_report_contains_header(self):
        modules = self._minimal_modules()
        report = bcr.build_markdown_report(modules, "2026-01-01T02:00:00Z",
                                           Path("artifacts/nightly"), None)
        assert "Nightly Benchmark Sweep" in report
        assert "Coverage Report" in report

    def test_bcr_19_report_contains_all_module_rows(self):
        modules = self._minimal_modules()
        report = bcr.build_markdown_report(modules, "2026-01-01T02:00:00Z",
                                           Path("artifacts/nightly"), None)
        for mid in bcr.MODULE_MAP:
            assert str(mid) in report

    def test_bcr_19_report_contains_legend(self):
        modules = self._minimal_modules()
        report = bcr.build_markdown_report(modules, "2026-01-01T02:00:00Z",
                                           Path("artifacts/nightly"), None)
        assert "Legend" in report
        assert bcr.GREEN in report
        assert bcr.GREY in report

    def test_bcr_19_report_contains_coverage_summary(self):
        modules = self._minimal_modules()
        report = bcr.build_markdown_report(modules, "2026-01-01T02:00:00Z",
                                           Path("artifacts/nightly"), None)
        # At least 1/32 module covered
        assert "1/" in report


# ---------------------------------------------------------------------------
# BCR-20  build_json_summary
# ---------------------------------------------------------------------------

class TestBuildJsonSummary:

    def test_bcr_20_json_has_correct_top_level_keys(self):
        modules: dict = {
            mid: {
                "id": mid,
                "name": bcr.MODULE_MAP[mid]["name"],
                "bench_count": 0,
                "error_count": 0,
                "bench_names": [],
                "mean_cpu_ns": None,
                "mean_cpu_ns_prev": None,
                "mean_cpu_ns_delta_pct": None,
                "status": bcr.GREY,
            }
            for mid in bcr.MODULE_MAP
        }
        summary = bcr.build_json_summary(modules, "2026-01-01T02:00:00Z")
        assert "generated" in summary
        assert "total_modules" in summary
        assert "covered_modules" in summary
        assert "modules" in summary

    def test_bcr_20_covered_modules_count_correct(self):
        modules: dict = {}
        for mid, info in bcr.MODULE_MAP.items():
            modules[mid] = {
                "id": mid,
                "name": info["name"],
                "bench_count": 2 if mid in (2, 3) else 0,
                "error_count": 0,
                "bench_names": [],
                "mean_cpu_ns": None,
                "mean_cpu_ns_prev": None,
                "mean_cpu_ns_delta_pct": None,
                "status": bcr.GREEN if mid in (2, 3) else bcr.GREY,
            }
        summary = bcr.build_json_summary(modules, "2026-01-01T02:00:00Z")
        assert summary["covered_modules"] == 2
        assert summary["total_modules"] == len(bcr.MODULE_MAP)

    def test_bcr_20_module_ids_are_string_keys(self):
        modules: dict = {
            mid: {
                "id": mid,
                "name": bcr.MODULE_MAP[mid]["name"],
                "bench_count": 0,
                "error_count": 0,
                "bench_names": [],
                "mean_cpu_ns": None,
                "mean_cpu_ns_prev": None,
                "mean_cpu_ns_delta_pct": None,
                "status": bcr.GREY,
            }
            for mid in bcr.MODULE_MAP
        }
        summary = bcr.build_json_summary(modules, "2026-01-01T02:00:00Z")
        # Keys in the JSON modules dict must be string representations of the IDs
        for key in summary["modules"]:
            assert isinstance(key, str)


# ---------------------------------------------------------------------------
# BCR-21 / BCR-22  main() – end-to-end
# ---------------------------------------------------------------------------

class TestMain:

    def _write_bench_json(self, directory: Path, stem: str, cpu_time: float) -> None:
        p = directory / f"{stem}.json"
        p.write_text(
            json.dumps({"benchmarks": [_bench_entry(cpu_time=cpu_time)]}),
            encoding="utf-8",
        )

    def test_bcr_21_end_to_end_writes_output_files(self, tmp_path):
        bench_dir = tmp_path / "nightly"
        bench_dir.mkdir()
        out_dir = tmp_path / "audit"
        self._write_bench_json(bench_dir, "bench_storage_latency", 300.0)
        self._write_bench_json(bench_dir, "bench_llm_inference", 5000.0)

        rc = bcr.main([
            "--bench-dir", str(bench_dir),
            "--output-dir", str(out_dir),
        ])
        assert rc == 0
        assert (out_dir / "coverage_report.md").exists()
        assert (out_dir / "coverage_report.json").exists()

    def test_bcr_21_output_json_is_valid(self, tmp_path):
        bench_dir = tmp_path / "nightly"
        bench_dir.mkdir()
        out_dir = tmp_path / "audit"
        self._write_bench_json(bench_dir, "bench_graph_bfs", 120.0)

        bcr.main(["--bench-dir", str(bench_dir), "--output-dir", str(out_dir)])
        data = json.loads((out_dir / "coverage_report.json").read_text())
        assert data["total_modules"] == len(bcr.MODULE_MAP)
        assert data["covered_modules"] >= 1

    def test_bcr_22_no_previous_dir_produces_na_delta(self, tmp_path):
        bench_dir = tmp_path / "nightly"
        bench_dir.mkdir()
        out_dir = tmp_path / "audit"
        self._write_bench_json(bench_dir, "bench_storage_latency", 400.0)

        bcr.main(["--bench-dir", str(bench_dir), "--output-dir", str(out_dir)])
        report = (out_dir / "coverage_report.md").read_text()
        assert "n/a" in report

    def test_bcr_21_output_dir_created_automatically(self, tmp_path):
        bench_dir = tmp_path / "nightly"
        bench_dir.mkdir()
        nested_out = tmp_path / "deep" / "nested" / "audit"
        # Must not exist beforehand
        assert not nested_out.exists()

        bcr.main(["--bench-dir", str(bench_dir), "--output-dir", str(nested_out)])
        assert nested_out.exists()
        assert (nested_out / "coverage_report.md").exists()
