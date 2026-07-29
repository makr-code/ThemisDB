"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_chimera_regression_detector.py                ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-06 04:04:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     520                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 2877ac90cc  2026-02-28  test(chimera): add TestChimeraBaseline structural tests f... ║
    • 0829d54f32  2026-02-28  feat(chimera): automated benchmark CI pipeline with regre... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
# Unit tests for benchmarks/chimera/chimera_regression_detector.py
#
# Covers:
# - Regression detection for throughput (higher-is-better) and latency (lower-is-better)
# - Severity classification at each threshold boundary
# - Improvement detection
# - Blocking-regression gate
# - Report generation smoke test
# - Empty / seed baseline handling (no workloads -> skip)
# - Partial overlap (workloads only in current, only in baseline)
# - Zero baseline values (division guard)
# - CLI exit codes via main()

from __future__ import annotations

import json
import sys
import tempfile
from pathlib import Path
from typing import Any, Dict

import pytest

# ---------------------------------------------------------------------------
# Ensure the chimera package directory is importable
# ---------------------------------------------------------------------------
_HERE       = Path(__file__).resolve().parent          # benchmarks/tests/
_CHIMERA    = _HERE.parent / "chimera"                 # benchmarks/chimera/
_BENCHMARKS = _HERE.parent                             # benchmarks/

for _p in (_CHIMERA, _BENCHMARKS):
    if str(_p) not in sys.path:
        sys.path.insert(0, str(_p))

from chimera_regression_detector import (   # noqa: E402
    ChimeraRegressionDetector,
    RegressionSeverity,
    WorkloadComparison,
    main as detector_main,
)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _baseline(workloads: Dict[str, Any]) -> Dict[str, Any]:
    return {
        "version":   "1.0.0",
        "branch":    "main",
        "commit":    "abc1234",
        "timestamp": "2026-01-01T00:00:00Z",
        "workloads": workloads,
    }


def _current(workloads: Dict[str, Any]) -> Dict[str, Any]:
    return {
        "version":   "1.0.1",
        "branch":    "feature/test",
        "commit":    "def5678",
        "timestamp": "2026-02-01T00:00:00Z",
        "workloads": workloads,
    }


def _single_throughput(
    baseline_val: float,
    current_val: float,
) -> list[WorkloadComparison]:
    """Compare one workload with only the throughput metric."""
    det = ChimeraRegressionDetector()
    return det.compare(
        _baseline({"wl_a": {"throughput_ops_per_sec": baseline_val}}),
        _current({"wl_a": {"throughput_ops_per_sec": current_val}}),
    )


def _single_latency(
    baseline_val: float,
    current_val: float,
    metric: str = "mean_latency_ms",
) -> list[WorkloadComparison]:
    det = ChimeraRegressionDetector()
    return det.compare(
        _baseline({"wl_a": {metric: baseline_val}}),
        _current({"wl_a": {metric: current_val}}),
    )


# ---------------------------------------------------------------------------
# Throughput (higher-is-better) tests
# ---------------------------------------------------------------------------

class TestThroughputRegression:

    def test_no_change_is_not_regression(self):
        comps = _single_throughput(1000.0, 1000.0)
        assert len(comps) == 1
        assert comps[0].severity == RegressionSeverity.NONE
        assert not comps[0].is_regression()

    def test_decrease_is_regression(self):
        # Throughput dropped → regression
        comps = _single_throughput(1000.0, 850.0)  # -15%
        assert comps[0].is_regression()

    def test_increase_is_improvement(self):
        comps = _single_throughput(1000.0, 1200.0)  # +20%
        assert not comps[0].is_regression()
        assert comps[0].is_improvement()

    def test_minor_threshold_lower_boundary(self):
        # Exactly at minor threshold (5%) for a decrease → MINOR
        comps = _single_throughput(1000.0, 950.0)  # -5%
        assert comps[0].severity == RegressionSeverity.MINOR
        assert comps[0].is_regression()

    def test_below_minor_threshold(self):
        # 3% drop → no severity
        comps = _single_throughput(1000.0, 970.0)
        assert comps[0].severity == RegressionSeverity.NONE

    def test_major_threshold(self):
        comps = _single_throughput(1000.0, 880.0)  # -12%
        assert comps[0].severity == RegressionSeverity.MAJOR

    def test_critical_threshold(self):
        comps = _single_throughput(1000.0, 750.0)  # -25%
        assert comps[0].severity == RegressionSeverity.CRITICAL


# ---------------------------------------------------------------------------
# Latency (lower-is-better) tests
# ---------------------------------------------------------------------------

class TestLatencyRegression:

    def test_increase_is_regression(self):
        comps = _single_latency(10.0, 13.0)  # +30% → critical
        assert comps[0].is_regression()
        assert comps[0].severity == RegressionSeverity.CRITICAL

    def test_decrease_is_improvement(self):
        comps = _single_latency(10.0, 8.5)   # -15%
        assert not comps[0].is_regression()
        assert comps[0].is_improvement()

    def test_p95_latency_regression(self):
        comps = _single_latency(20.0, 24.0, metric="p95_latency_ms")  # +20%
        assert comps[0].is_regression()
        assert comps[0].severity == RegressionSeverity.CRITICAL

    def test_p99_latency_minor_regression(self):
        comps = _single_latency(50.0, 52.6, metric="p99_latency_ms")  # +5.2%
        assert comps[0].is_regression()
        assert comps[0].severity == RegressionSeverity.MINOR

    def test_no_change(self):
        comps = _single_latency(10.0, 10.0)
        assert comps[0].severity == RegressionSeverity.NONE
        assert not comps[0].is_regression()


# ---------------------------------------------------------------------------
# Custom thresholds
# ---------------------------------------------------------------------------

class TestCustomThresholds:

    def test_stricter_thresholds(self):
        det = ChimeraRegressionDetector(thresholds={"minor": 2.0, "major": 5.0, "critical": 10.0})
        comps = det.compare(
            _baseline({"w": {"throughput_ops_per_sec": 1000.0}}),
            _current({"w": {"throughput_ops_per_sec": 930.0}}),  # -7%
        )
        assert comps[0].severity == RegressionSeverity.MAJOR

    def test_lenient_thresholds(self):
        det = ChimeraRegressionDetector(thresholds={"minor": 15.0, "major": 25.0, "critical": 40.0})
        comps = det.compare(
            _baseline({"w": {"throughput_ops_per_sec": 1000.0}}),
            _current({"w": {"throughput_ops_per_sec": 900.0}}),  # -10%
        )
        assert comps[0].severity == RegressionSeverity.NONE


# ---------------------------------------------------------------------------
# Blocking regressions gate
# ---------------------------------------------------------------------------

class TestBlockingRegressions:

    def test_critical_blocks_at_major(self):
        det = ChimeraRegressionDetector()
        comps = _single_throughput(1000.0, 700.0)   # -30% → CRITICAL
        assert det.has_blocking_regressions(comps, block_threshold="major")

    def test_major_blocks_at_major(self):
        det = ChimeraRegressionDetector()
        comps = _single_throughput(1000.0, 880.0)   # -12% → MAJOR
        assert det.has_blocking_regressions(comps, block_threshold="major")

    def test_minor_does_not_block_at_major(self):
        det = ChimeraRegressionDetector()
        comps = _single_throughput(1000.0, 950.0)   # -5% → MINOR
        assert not det.has_blocking_regressions(comps, block_threshold="major")

    def test_minor_blocks_at_minor(self):
        det = ChimeraRegressionDetector()
        comps = _single_throughput(1000.0, 950.0)
        assert det.has_blocking_regressions(comps, block_threshold="minor")

    def test_improvement_does_not_block(self):
        det = ChimeraRegressionDetector()
        comps = _single_throughput(1000.0, 1300.0)
        assert not det.has_blocking_regressions(comps)


# ---------------------------------------------------------------------------
# Multiple metrics per workload
# ---------------------------------------------------------------------------

class TestMultipleMetrics:

    def test_all_four_metrics_compared(self):
        det = ChimeraRegressionDetector()
        comps = det.compare(
            _baseline({"w": {
                "throughput_ops_per_sec": 1000.0,
                "mean_latency_ms": 5.0,
                "p95_latency_ms": 10.0,
                "p99_latency_ms": 15.0,
            }}),
            _current({"w": {
                "throughput_ops_per_sec": 1000.0,
                "mean_latency_ms": 5.0,
                "p95_latency_ms": 10.0,
                "p99_latency_ms": 15.0,
            }}),
        )
        assert len(comps) == 4

    def test_mixed_regression_and_improvement(self):
        det = ChimeraRegressionDetector()
        comps = det.compare(
            _baseline({"w": {
                "throughput_ops_per_sec": 1000.0,
                "mean_latency_ms": 10.0,
            }}),
            _current({"w": {
                "throughput_ops_per_sec": 1200.0,   # +20% improvement
                "mean_latency_ms": 12.0,            # +20% regression
            }}),
        )
        regressions = [c for c in comps if c.is_regression()]
        improvements = [c for c in comps if c.is_improvement()]
        assert len(regressions) == 1
        assert len(improvements) == 1


# ---------------------------------------------------------------------------
# Edge cases
# ---------------------------------------------------------------------------

class TestEdgeCases:

    def test_empty_baseline_workloads_skips(self):
        det = ChimeraRegressionDetector()
        comps = det.compare(
            _baseline({}),
            _current({"w": {"throughput_ops_per_sec": 1000.0}}),
        )
        assert comps == []

    def test_new_workload_in_current_skipped(self):
        det = ChimeraRegressionDetector()
        comps = det.compare(
            _baseline({"old_wl": {"throughput_ops_per_sec": 1000.0}}),
            _current({"new_wl": {"throughput_ops_per_sec": 500.0}}),
        )
        assert comps == []

    def test_zero_baseline_value_skipped(self):
        det = ChimeraRegressionDetector()
        comps = det.compare(
            _baseline({"w": {"throughput_ops_per_sec": 0.0}}),
            _current({"w": {"throughput_ops_per_sec": 1000.0}}),
        )
        assert comps == []

    def test_null_metric_value_skipped(self):
        det = ChimeraRegressionDetector()
        comps = det.compare(
            _baseline({"w": {"throughput_ops_per_sec": 1000.0, "mean_latency_ms": None}}),
            _current({"w": {"throughput_ops_per_sec": 900.0, "mean_latency_ms": 5.0}}),
        )
        # Only throughput compared; mean_latency_ms skipped (baseline None)
        assert len(comps) == 1
        assert comps[0].metric == "throughput_ops_per_sec"

    def test_workload_only_in_baseline_skipped(self):
        det = ChimeraRegressionDetector()
        comps = det.compare(
            _baseline({"wl_removed": {"throughput_ops_per_sec": 1000.0}}),
            _current({}),
        )
        assert comps == []


# ---------------------------------------------------------------------------
# Report generation smoke tests
# ---------------------------------------------------------------------------

class TestReportGeneration:

    def test_report_contains_verdict_passed(self):
        det = ChimeraRegressionDetector()
        comps = det.compare(
            _baseline({"w": {"throughput_ops_per_sec": 1000.0}}),
            _current({"w": {"throughput_ops_per_sec": 1000.0}}),
        )
        report = det.generate_report(comps, _baseline({}), _current({}))
        assert "PASSED" in report

    def test_report_contains_verdict_failed(self):
        det = ChimeraRegressionDetector()
        comps = det.compare(
            _baseline({"w": {"throughput_ops_per_sec": 1000.0}}),
            _current({"w": {"throughput_ops_per_sec": 700.0}}),
        )
        report = det.generate_report(comps, _baseline({}), _current({}))
        assert "FAILED" in report

    def test_report_is_nonempty_string(self):
        det = ChimeraRegressionDetector()
        comps: list = []
        report = det.generate_report(comps, _baseline({}), _current({}))
        assert isinstance(report, str)
        assert len(report) > 0


# ---------------------------------------------------------------------------
# CLI tests
# ---------------------------------------------------------------------------

class TestCLI:

    def _write_json(self, tmp: Path, name: str, data: dict) -> Path:
        p = tmp / name
        p.write_text(json.dumps(data), encoding="utf-8")
        return p

    def test_exit_0_no_regressions(self, tmp_path):
        baseline_path = self._write_json(tmp_path, "baseline.json",
                                         _baseline({"w": {"throughput_ops_per_sec": 1000.0}}))
        current_path  = self._write_json(tmp_path, "current.json",
                                         _current({"w": {"throughput_ops_per_sec": 1000.0}}))
        rc = detector_main([
            "--baseline", str(baseline_path),
            "--current",  str(current_path),
            "--output",   str(tmp_path / "report.txt"),
        ])
        assert rc == 0

    def test_exit_1_major_regression(self, tmp_path):
        baseline_path = self._write_json(tmp_path, "baseline.json",
                                         _baseline({"w": {"throughput_ops_per_sec": 1000.0}}))
        current_path  = self._write_json(tmp_path, "current.json",
                                         _current({"w": {"throughput_ops_per_sec": 800.0}}))  # -20%
        rc = detector_main([
            "--baseline", str(baseline_path),
            "--current",  str(current_path),
            "--output",   str(tmp_path / "report.txt"),
            "--fail-on",  "major",
        ])
        assert rc == 1

    def test_exit_0_minor_regression_with_major_gate(self, tmp_path):
        baseline_path = self._write_json(tmp_path, "baseline.json",
                                         _baseline({"w": {"throughput_ops_per_sec": 1000.0}}))
        current_path  = self._write_json(tmp_path, "current.json",
                                         _current({"w": {"throughput_ops_per_sec": 950.0}}))  # -5%
        rc = detector_main([
            "--baseline", str(baseline_path),
            "--current",  str(current_path),
            "--output",   str(tmp_path / "report.txt"),
            "--fail-on",  "major",
        ])
        assert rc == 0

    def test_exit_0_empty_baseline_workloads(self, tmp_path):
        """Seed/empty baseline → skip comparison, exit 0."""
        baseline_path = self._write_json(tmp_path, "baseline.json",
                                         _baseline({}))
        current_path  = self._write_json(tmp_path, "current.json",
                                         _current({"w": {"throughput_ops_per_sec": 1000.0}}))
        rc = detector_main([
            "--baseline", str(baseline_path),
            "--current",  str(current_path),
            "--output",   str(tmp_path / "report.txt"),
        ])
        assert rc == 0

    def test_exit_2_missing_baseline(self, tmp_path):
        current_path = self._write_json(tmp_path, "current.json",
                                        _current({"w": {"throughput_ops_per_sec": 1000.0}}))
        rc = detector_main([
            "--baseline", str(tmp_path / "nonexistent.json"),
            "--current",  str(current_path),
            "--output",   str(tmp_path / "report.txt"),
        ])
        assert rc == 2

    def test_report_file_created(self, tmp_path):
        baseline_path = self._write_json(tmp_path, "baseline.json",
                                         _baseline({"w": {"throughput_ops_per_sec": 1000.0}}))
        current_path  = self._write_json(tmp_path, "current.json",
                                         _current({"w": {"throughput_ops_per_sec": 1000.0}}))
        report_path = tmp_path / "report.txt"
        detector_main([
            "--baseline", str(baseline_path),
            "--current",  str(current_path),
            "--output",   str(report_path),
        ])
        assert report_path.exists()
        assert report_path.stat().st_size > 0

    def test_json_summary_file_created(self, tmp_path):
        baseline_path = self._write_json(tmp_path, "baseline.json",
                                         _baseline({"w": {"throughput_ops_per_sec": 1000.0}}))
        current_path  = self._write_json(tmp_path, "current.json",
                                         _current({"w": {"throughput_ops_per_sec": 1000.0}}))
        report_path = tmp_path / "report.txt"
        detector_main([
            "--baseline", str(baseline_path),
            "--current",  str(current_path),
            "--output",   str(report_path),
        ])
        json_path = tmp_path / "report.json"
        assert json_path.exists()
        data = json.loads(json_path.read_text())
        assert "summary" in data
        assert "comparisons" in data


# ---------------------------------------------------------------------------
# Baseline file sanity-check (mirrors TestAccelerationBaseline pattern)
# ---------------------------------------------------------------------------

class TestChimeraBaseline:
    """
    Sanity-check the committed chimera baseline file used by the CI workflow
    to ensure it is well-formed and compatible with the regression detector.
    """

    BASELINE_PATH = (
        Path(__file__).parent.parent / "baselines" / "chimera" / "baseline.json"
    )

    def test_baseline_file_exists(self):
        assert self.BASELINE_PATH.exists(), (
            f"Chimera baseline file not found: {self.BASELINE_PATH}"
        )

    def test_baseline_has_required_fields(self):
        with open(self.BASELINE_PATH, encoding="utf-8") as fh:
            data = json.load(fh)
        for field in ("version", "branch", "commit", "timestamp", "workloads"):
            assert field in data, f"Missing required field in chimera baseline: {field!r}"

    def test_baseline_workloads_is_dict(self):
        with open(self.BASELINE_PATH, encoding="utf-8") as fh:
            data = json.load(fh)
        assert isinstance(data["workloads"], dict), (
            "'workloads' must be a dict (may be empty for a seed baseline)"
        )

    def test_baseline_compatible_with_regression_detector(self):
        """The detector must be able to load and use the baseline without errors."""
        with open(self.BASELINE_PATH, encoding="utf-8") as fh:
            baseline = json.load(fh)
        # A seed baseline has no workloads; compare() must return an empty list.
        det = ChimeraRegressionDetector()
        current = {
            "version": "test",
            "branch": "test",
            "commit": "test",
            "timestamp": "2026-01-01T00:00:00Z",
            "workloads": {},
        }
        comps = det.compare(baseline, current)
        assert isinstance(comps, list)
