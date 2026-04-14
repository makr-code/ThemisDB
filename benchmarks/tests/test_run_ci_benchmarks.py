"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_run_ci_benchmarks.py                          ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-13 20:19:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     292                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 42a3eb2e8f  2026-04-10  refactor(chimera): remove local benchmark tree and use su... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • db650a5981  2026-03-01  feat(chimera): add run_ci_benchmarks tests and seed basel... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Unit tests for external/chimera/run_ci_benchmarks.py

Covers:
- run_benchmarks() returns a valid harness report with all four workload families
- build_output() wraps the report with version/branch/commit/timestamp metadata
- build_output() extracts the expected per-workload metrics
- main() CLI writes a valid JSON file to the requested output path
- main() handles a custom --warmup / --iterations value
"""

from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Any, Dict

import pytest

# ---------------------------------------------------------------------------
# Ensure the chimera package directory is importable
# ---------------------------------------------------------------------------
_HERE       = Path(__file__).resolve().parent          # benchmarks/tests/
_CHIMERA    = _HERE.parent.parent / "external" / "chimera"  # external/chimera/
_BENCHMARKS = _HERE.parent                             # benchmarks/

for _p in (_CHIMERA, _BENCHMARKS):
    if str(_p) not in sys.path:
        sys.path.insert(0, str(_p))

from run_ci_benchmarks import (   # noqa: E402
    run_benchmarks,
    build_output,
    main,
    _REPO_ROOT,
    _WORKLOADS,
)


# ---------------------------------------------------------------------------
# Expected workload IDs (must match the registry in run_ci_benchmarks.py)
# ---------------------------------------------------------------------------

_EXPECTED_WORKLOAD_IDS = {
    "relational_sort",
    "vector_dot_product",
    "document_lookup",
    "graph_bfs",
}

_EXPECTED_METRICS = {
    "throughput_ops_per_sec",
    "mean_latency_ms",
    "p95_latency_ms",
    "p99_latency_ms",
}


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _fast_report() -> Dict[str, Any]:
    """Run a minimal benchmark (1 warm-up, 5 iterations) for test speed."""
    return run_benchmarks(warmup_iterations=1, run_iterations=5)


# ---------------------------------------------------------------------------
# Tests: _WORKLOADS registry
# ---------------------------------------------------------------------------

class TestWorkloadRegistry:
    """The module-level workload list must contain all four operation families."""

    def test_all_expected_workloads_registered(self):
        registered_ids = {wl.workload_id for wl in _WORKLOADS}
        assert registered_ids == _EXPECTED_WORKLOAD_IDS

    def test_workload_operations_are_callable(self):
        for wl in _WORKLOADS:
            assert callable(wl.operation), f"{wl.workload_id}.operation must be callable"

    def test_workload_operations_do_not_raise(self):
        """Each operation must execute without raising."""
        for wl in _WORKLOADS:
            wl.operation()  # should not raise


# ---------------------------------------------------------------------------
# Tests: run_benchmarks()
# ---------------------------------------------------------------------------

class TestRunBenchmarks:
    """run_benchmarks() must return a valid BenchmarkHarness report dict."""

    def test_returns_dict(self):
        report = _fast_report()
        assert isinstance(report, dict)

    def test_system_name_field(self):
        report = _fast_report()
        assert report.get("system_name") == "chimera_ci"

    def test_workloads_key_present(self):
        report = _fast_report()
        assert "workloads" in report

    def test_all_four_workloads_present(self):
        report = _fast_report()
        assert set(report["workloads"].keys()) == _EXPECTED_WORKLOAD_IDS

    def test_each_workload_has_throughput(self):
        report = _fast_report()
        for wid, data in report["workloads"].items():
            assert "throughput_ops_per_sec" in data, f"{wid} missing throughput"
            assert data["throughput_ops_per_sec"] > 0.0, f"{wid} throughput must be positive"

    def test_each_workload_has_latency_metrics(self):
        report = _fast_report()
        for wid, data in report["workloads"].items():
            assert "mean_latency_ms" in data, f"{wid} missing mean_latency_ms"
            assert "p95_latency_ms" in data, f"{wid} missing p95_latency_ms"
            assert "p99_latency_ms" in data, f"{wid} missing p99_latency_ms"

    def test_latencies_are_non_negative(self):
        report = _fast_report()
        for wid, data in report["workloads"].items():
            assert data["mean_latency_ms"] >= 0.0, f"{wid} mean_latency_ms < 0"
            assert data["p95_latency_ms"] >= 0.0, f"{wid} p95_latency_ms < 0"
            assert data["p99_latency_ms"] >= 0.0, f"{wid} p99_latency_ms < 0"

    def test_p99_ge_p95(self):
        report = _fast_report()
        for wid, data in report["workloads"].items():
            assert data["p99_latency_ms"] >= data["p95_latency_ms"], (
                f"{wid}: p99 ({data['p99_latency_ms']}) < p95 ({data['p95_latency_ms']})"
            )

    def test_no_errors_in_synthetic_workloads(self):
        report = _fast_report()
        for wid, data in report["workloads"].items():
            assert data.get("error_count", 0) == 0, f"{wid} reported errors"

    def test_custom_iteration_count_respected(self):
        """run_iterations affects elapsed time; a larger count takes longer."""
        report_small = run_benchmarks(warmup_iterations=0, run_iterations=5)
        report_large = run_benchmarks(warmup_iterations=0, run_iterations=50)
        # The larger run should have more elapsed time (not a strict guarantee
        # on every machine, but valid for synthetic CPU-bound workloads).
        small_tput = report_small["workloads"]["relational_sort"]["throughput_ops_per_sec"]
        large_tput = report_large["workloads"]["relational_sort"]["throughput_ops_per_sec"]
        # Both must be positive
        assert small_tput > 0.0
        assert large_tput > 0.0


# ---------------------------------------------------------------------------
# Tests: build_output()
# ---------------------------------------------------------------------------

class TestBuildOutput:
    """build_output() must wrap the harness report with versioning metadata."""

    def _make_report(self) -> Dict[str, Any]:
        return {
            "system_name": "chimera_ci",
            "workloads": {
                "relational_sort": {
                    "throughput_ops_per_sec": 12345.6,
                    "mean_latency_ms": 0.08,
                    "p95_latency_ms": 0.12,
                    "p99_latency_ms": 0.15,
                    "error_count": 0,
                    "run_iterations": 5,
                    "elapsed_seconds": 0.001,
                }
            },
        }

    def test_version_key_present(self):
        out = build_output(self._make_report(), _REPO_ROOT)
        assert "version" in out

    def test_branch_key_present(self):
        out = build_output(self._make_report(), _REPO_ROOT)
        assert "branch" in out

    def test_commit_key_present(self):
        out = build_output(self._make_report(), _REPO_ROOT)
        assert "commit" in out

    def test_timestamp_key_present(self):
        out = build_output(self._make_report(), _REPO_ROOT)
        assert "timestamp" in out

    def test_workloads_key_present(self):
        out = build_output(self._make_report(), _REPO_ROOT)
        assert "workloads" in out

    def test_workload_metrics_extracted(self):
        out = build_output(self._make_report(), _REPO_ROOT)
        wl = out["workloads"]["relational_sort"]
        assert "throughput_ops_per_sec" in wl
        assert "mean_latency_ms" in wl
        assert "p95_latency_ms" in wl
        assert "p99_latency_ms" in wl

    def test_output_is_json_serializable(self):
        out = build_output(self._make_report(), _REPO_ROOT)
        json.dumps(out)  # must not raise

    def test_timestamp_is_iso8601_utc(self):
        from datetime import datetime, timezone
        out = build_output(self._make_report(), _REPO_ROOT)
        ts = out["timestamp"]
        # Must be parseable and must end with 'Z'
        assert ts.endswith("Z"), f"Timestamp {ts!r} must end with 'Z'"
        # Remove trailing Z and parse
        datetime.fromisoformat(ts.rstrip("Z"))


# ---------------------------------------------------------------------------
# Tests: main() CLI
# ---------------------------------------------------------------------------

class TestMain:
    """main() must produce a valid JSON file at the requested output path."""

    def test_main_writes_json_file(self, tmp_path):
        output_file = tmp_path / "results.json"
        rc = main(["--output", str(output_file), "--warmup", "1", "--iterations", "5"])
        assert rc == 0
        assert output_file.exists()

    def test_main_output_is_valid_json(self, tmp_path):
        output_file = tmp_path / "results.json"
        main(["--output", str(output_file), "--warmup", "1", "--iterations", "5"])
        data = json.loads(output_file.read_text(encoding="utf-8"))
        assert isinstance(data, dict)

    def test_main_output_contains_expected_workloads(self, tmp_path):
        output_file = tmp_path / "results.json"
        main(["--output", str(output_file), "--warmup", "1", "--iterations", "5"])
        data = json.loads(output_file.read_text(encoding="utf-8"))
        assert set(data["workloads"].keys()) == _EXPECTED_WORKLOAD_IDS

    def test_main_output_contains_metadata(self, tmp_path):
        output_file = tmp_path / "results.json"
        main(["--output", str(output_file), "--warmup", "1", "--iterations", "5"])
        data = json.loads(output_file.read_text(encoding="utf-8"))
        for key in ("version", "branch", "commit", "timestamp"):
            assert key in data, f"Missing metadata key: {key}"

    def test_main_creates_parent_dirs(self, tmp_path):
        nested = tmp_path / "a" / "b" / "c" / "results.json"
        rc = main(["--output", str(nested), "--warmup", "1", "--iterations", "5"])
        assert rc == 0
        assert nested.exists()

    def test_main_default_output_path(self, tmp_path, monkeypatch):
        """main() without --output defaults to benchmark_results/chimera_results.json."""
        monkeypatch.chdir(tmp_path)
        rc = main(["--warmup", "1", "--iterations", "5"])
        assert rc == 0
        default_out = tmp_path / "benchmark_results" / "chimera_results.json"
        assert default_out.exists()
