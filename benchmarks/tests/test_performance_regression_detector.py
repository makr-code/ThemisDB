/*
 * ThemisDB | File: test_performance_regression_detector.py | Version: 0.0.15
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_performance_regression_detector.py            ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-04-15 18:43:45                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     556                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Unit tests for performance_regression_detector.py and baseline_manager.py.

Covers:
- Regression detection accuracy at each severity level
- Direction logic for throughput vs. time metrics
- Threshold boundary behaviour
- Report generation
- Baseline manager save/load round-trip
- Edge cases: empty benchmark sets, null values, partial overlap
"""

import json
import pytest
import tempfile
from pathlib import Path
import sys

# Allow direct import from the benchmarks/ directory.
sys.path.insert(0, str(Path(__file__).parent.parent))

from performance_regression_detector import (
    PerformanceRegressionDetector,
    BenchmarkComparison,
    RegressionSeverity,
)
from baseline_manager import BaselineManager


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _make_baseline(benchmarks: dict) -> dict:
    return {
        "version": "1.0.0",
        "branch": "main",
        "commit": "abc123",
        "timestamp": "2026-01-01T00:00:00Z",
        "benchmarks": benchmarks,
    }


def _make_current(benchmarks: dict) -> dict:
    return {
        "version": "1.0.1",
        "branch": "feature/test",
        "commit": "def456",
        "timestamp": "2026-02-01T00:00:00Z",
        "benchmarks": benchmarks,
    }


def _single(baseline_val: float, current_val: float, metric: str = "real_time") -> list:
    """Return comparisons for a single benchmark with one metric."""
    baseline = _make_baseline({"BM_Test": {metric: baseline_val}})
    current = _make_current({"BM_Test": {metric: current_val}})
    detector = PerformanceRegressionDetector(metrics=[metric])
    return detector.compare_benchmarks(baseline, current)


# ===========================================================================
# PerformanceRegressionDetector — severity classification
# ===========================================================================

class TestSeverityClassification:
    """Verify that changes are mapped to the correct severity bucket."""

    def test_no_change_is_none(self):
        comparisons = _single(1000.0, 1000.0)
        assert len(comparisons) == 1
        assert comparisons[0].severity == RegressionSeverity.NONE

    def test_tiny_change_is_none(self):
        # 2 % change — below the 5 % minor threshold
        comparisons = _single(1000.0, 1020.0)
        assert comparisons[0].severity == RegressionSeverity.NONE

    def test_minor_regression_boundary(self):
        # Exactly 5 % increase in real_time -> minor
        comparisons = _single(1000.0, 1050.0)
        assert comparisons[0].severity == RegressionSeverity.MINOR

    def test_major_regression_boundary(self):
        # Exactly 10 % increase in real_time -> major
        comparisons = _single(1000.0, 1100.0)
        assert comparisons[0].severity == RegressionSeverity.MAJOR

    def test_critical_regression_boundary(self):
        # Exactly 20 % increase in real_time -> critical
        comparisons = _single(1000.0, 1200.0)
        assert comparisons[0].severity == RegressionSeverity.CRITICAL

    def test_below_critical_is_major(self):
        # 15 % increase -> major (>=10 %, <20 %)
        comparisons = _single(1000.0, 1150.0)
        assert comparisons[0].severity == RegressionSeverity.MAJOR

    def test_custom_thresholds(self):
        thresholds = {"minor": 2.0, "major": 5.0, "critical": 10.0}
        detector = PerformanceRegressionDetector(thresholds=thresholds, metrics=["real_time"])
        baseline = _make_baseline({"BM_Test": {"real_time": 100.0}})
        current = _make_current({"BM_Test": {"real_time": 108.0}})  # +8 %
        comparisons = detector.compare_benchmarks(baseline, current)
        assert comparisons[0].severity == RegressionSeverity.MAJOR


# ===========================================================================
# PerformanceRegressionDetector — regression / improvement direction
# ===========================================================================

class TestRegressionDirection:
    """Higher time = regression; higher throughput = improvement."""

    def test_higher_real_time_is_regression(self):
        comparisons = _single(100.0, 150.0, metric="real_time")
        assert comparisons[0].is_regression()
        assert not comparisons[0].is_improvement()

    def test_lower_real_time_is_improvement(self):
        comparisons = _single(100.0, 80.0, metric="real_time")
        assert comparisons[0].is_improvement()
        assert not comparisons[0].is_regression()

    def test_lower_items_per_second_is_regression(self):
        comparisons = _single(1_000_000.0, 800_000.0, metric="items_per_second")
        assert comparisons[0].is_regression()
        assert not comparisons[0].is_improvement()

    def test_higher_items_per_second_is_improvement(self):
        comparisons = _single(1_000_000.0, 1_200_000.0, metric="items_per_second")
        assert comparisons[0].is_improvement()
        assert not comparisons[0].is_regression()

    def test_lower_bytes_per_second_is_regression(self):
        comparisons = _single(50_000_000.0, 30_000_000.0, metric="bytes_per_second")
        assert comparisons[0].is_regression()

    def test_higher_bytes_per_second_is_improvement(self):
        comparisons = _single(50_000_000.0, 70_000_000.0, metric="bytes_per_second")
        assert comparisons[0].is_improvement()


# ===========================================================================
# PerformanceRegressionDetector — has_blocking_regressions
# ===========================================================================

class TestBlockingRegressions:

    def _comparisons_with(self, severity: RegressionSeverity, is_regression: bool):
        """Build a single comparison manually."""
        comp = BenchmarkComparison(
            name="BM_Test",
            baseline_value=100.0,
            current_value=130.0 if is_regression else 70.0,
            percent_change=30.0 if is_regression else -30.0,
            severity=severity,
            metric_name="real_time",
        )
        return [comp]

    def test_critical_blocks_at_major_threshold(self):
        detector = PerformanceRegressionDetector()
        comparisons = self._comparisons_with(RegressionSeverity.CRITICAL, True)
        assert detector.has_blocking_regressions(comparisons, "major")

    def test_major_blocks_at_major_threshold(self):
        detector = PerformanceRegressionDetector()
        comparisons = self._comparisons_with(RegressionSeverity.MAJOR, True)
        assert detector.has_blocking_regressions(comparisons, "major")

    def test_minor_does_not_block_at_major_threshold(self):
        detector = PerformanceRegressionDetector()
        comparisons = self._comparisons_with(RegressionSeverity.MINOR, True)
        assert not detector.has_blocking_regressions(comparisons, "major")

    def test_major_does_not_block_at_critical_threshold(self):
        detector = PerformanceRegressionDetector()
        comparisons = self._comparisons_with(RegressionSeverity.MAJOR, True)
        assert not detector.has_blocking_regressions(comparisons, "critical")

    def test_improvement_never_blocks(self):
        detector = PerformanceRegressionDetector()
        comparisons = self._comparisons_with(RegressionSeverity.CRITICAL, False)
        assert not detector.has_blocking_regressions(comparisons, "minor")


# ===========================================================================
# PerformanceRegressionDetector — compare_benchmarks edge cases
# ===========================================================================

class TestCompareBenchmarksEdgeCases:

    def test_missing_benchmark_in_current_skipped(self):
        """Benchmarks in baseline but not current are silently skipped."""
        baseline = _make_baseline({
            "BM_A": {"real_time": 100.0},
            "BM_B": {"real_time": 200.0},
        })
        current = _make_current({"BM_A": {"real_time": 105.0}})
        detector = PerformanceRegressionDetector(metrics=["real_time"])
        comparisons = detector.compare_benchmarks(baseline, current)
        names = [c.name for c in comparisons]
        assert "BM_A" in names
        assert "BM_B" not in names

    def test_new_benchmark_in_current_skipped(self):
        """Benchmarks present only in current (not baseline) are skipped."""
        baseline = _make_baseline({"BM_A": {"real_time": 100.0}})
        current = _make_current({
            "BM_A": {"real_time": 105.0},
            "BM_New": {"real_time": 50.0},
        })
        detector = PerformanceRegressionDetector(metrics=["real_time"])
        comparisons = detector.compare_benchmarks(baseline, current)
        names = [c.name for c in comparisons]
        assert "BM_New" not in names

    def test_null_metric_value_skipped(self):
        """A null/None metric value in baseline or current should be skipped."""
        baseline = _make_baseline({"BM_A": {"real_time": None, "cpu_time": 100.0}})
        current = _make_current({"BM_A": {"real_time": 110.0, "cpu_time": 105.0}})
        detector = PerformanceRegressionDetector(metrics=["real_time", "cpu_time"])
        comparisons = detector.compare_benchmarks(baseline, current)
        metrics_compared = {c.metric_name for c in comparisons}
        assert "real_time" not in metrics_compared
        assert "cpu_time" in metrics_compared

    def test_zero_baseline_value_skipped(self):
        """A zero baseline causes division by zero — must be skipped."""
        baseline = _make_baseline({"BM_A": {"real_time": 0.0}})
        current = _make_current({"BM_A": {"real_time": 100.0}})
        detector = PerformanceRegressionDetector(metrics=["real_time"])
        comparisons = detector.compare_benchmarks(baseline, current)
        assert len(comparisons) == 0

    def test_empty_current_benchmarks(self):
        baseline = _make_baseline({"BM_A": {"real_time": 100.0}})
        current = _make_current({})
        detector = PerformanceRegressionDetector(metrics=["real_time"])
        comparisons = detector.compare_benchmarks(baseline, current)
        assert comparisons == []

    def test_multiple_metrics_per_benchmark(self):
        baseline = _make_baseline({"BM_A": {
            "real_time": 100.0,
            "cpu_time": 95.0,
            "items_per_second": 10_000.0,
        }})
        current = _make_current({"BM_A": {
            "real_time": 110.0,
            "cpu_time": 104.0,
            "items_per_second": 9_000.0,
        }})
        detector = PerformanceRegressionDetector(
            metrics=["real_time", "cpu_time", "items_per_second"]
        )
        comparisons = detector.compare_benchmarks(baseline, current)
        assert len(comparisons) == 3
        metric_names = {c.metric_name for c in comparisons}
        assert metric_names == {"real_time", "cpu_time", "items_per_second"}


# ===========================================================================
# PerformanceRegressionDetector — generate_report and save_report
# ===========================================================================

class TestReportGeneration:

    def test_report_contains_summary(self):
        baseline = _make_baseline({"BM_Test": {"real_time": 100.0}})
        current = _make_current({"BM_Test": {"real_time": 125.0}})  # +25 % -> critical
        detector = PerformanceRegressionDetector(metrics=["real_time"])
        comparisons = detector.compare_benchmarks(baseline, current)
        report = detector.generate_report(comparisons, baseline, current)
        assert "PERFORMANCE REGRESSION DETECTION REPORT" in report
        assert "SUMMARY" in report
        assert "VERDICT" in report
        assert "FAILED" in report

    def test_report_passed_when_no_regression(self):
        baseline = _make_baseline({"BM_Test": {"real_time": 100.0}})
        current = _make_current({"BM_Test": {"real_time": 101.0}})  # +1 % -> none
        detector = PerformanceRegressionDetector(metrics=["real_time"])
        comparisons = detector.compare_benchmarks(baseline, current)
        report = detector.generate_report(comparisons, baseline, current)
        assert "PASSED" in report

    def test_save_report_writes_txt_and_json(self):
        baseline = _make_baseline({"BM_Test": {"real_time": 100.0}})
        current = _make_current({"BM_Test": {"real_time": 130.0}})
        detector = PerformanceRegressionDetector(metrics=["real_time"])
        comparisons = detector.compare_benchmarks(baseline, current)
        report = detector.generate_report(comparisons, baseline, current)

        with tempfile.TemporaryDirectory() as tmpdir:
            output_path = Path(tmpdir) / "report.txt"
            detector.save_report(report, output_path, comparisons)

            assert output_path.exists()
            json_path = output_path.with_suffix(".json")
            assert json_path.exists()

            with open(json_path) as f:
                data = json.load(f)

            assert "summary" in data
            assert "comparisons" in data
            assert data["summary"]["critical"] == 1


# ===========================================================================
# PerformanceRegressionDetector — raw Google Benchmark JSON wrapping
# ===========================================================================

class TestRawGoogleBenchmarkInput:
    """
    The CLI accepts raw Google Benchmark JSON output (list of benchmarks,
    no top-level 'version' field).  Verify the wrapping logic works.
    """

    def _raw_gb_json(self) -> dict:
        """Minimal Google Benchmark JSON output format."""
        return {
            "context": {"host_name": "ci-runner", "num_cpus": 2},
            "benchmarks": [
                {
                    "name": "BM_CPU_ANN_L2Distance/1000/64",
                    "real_time": 510.0,
                    "cpu_time": 510.0,
                    "iterations": 1000,
                    "items_per_second": 1960000.0,
                }
            ],
        }

    def test_raw_json_parsed_correctly(self):
        raw = self._raw_gb_json()
        # The raw format has no 'version' key, so the detector wraps it.
        assert "version" not in raw
        assert "benchmarks" in raw

        wrapped = {
            "version": "unknown",
            "branch": "unknown",
            "commit": "unknown",
            "timestamp": "unknown",
            "benchmarks": {},
        }
        for bench in raw["benchmarks"]:
            name = bench.get("name", "unknown")
            wrapped["benchmarks"][name] = {
                "real_time": bench.get("real_time"),
                "cpu_time": bench.get("cpu_time"),
                "iterations": bench.get("iterations"),
                "items_per_second": bench.get("items_per_second"),
                "bytes_per_second": bench.get("bytes_per_second"),
            }

        baseline_benchmarks = {
            "BM_CPU_ANN_L2Distance/1000/64": {
                "real_time": 500.0,
                "cpu_time": 500.0,
                "iterations": 1000,
                "items_per_second": 2000000.0,
                "bytes_per_second": None,
            }
        }
        baseline = _make_baseline(baseline_benchmarks)
        detector = PerformanceRegressionDetector(
            metrics=["real_time", "cpu_time", "items_per_second"]
        )
        comparisons = detector.compare_benchmarks(baseline, wrapped)
        assert len(comparisons) == 3
        names = {c.name for c in comparisons}
        assert "BM_CPU_ANN_L2Distance/1000/64" in names


# ===========================================================================
# BaselineManager — save / load round-trip
# ===========================================================================

class TestBaselineManager:

    def test_save_and_load_main_branch(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            manager = BaselineManager(baselines_dir=tmpdir)
            data = {
                "BM_Test": {"real_time": 100.0, "cpu_time": 99.0}
            }
            manager.save_baseline(data, branch="main", version="1.0.0", commit="abc")
            loaded = manager.load_baseline(branch="main")

            assert loaded is not None
            assert loaded["version"] == "1.0.0"
            assert loaded["branch"] == "main"
            assert loaded["commit"] == "abc"
            assert "BM_Test" in loaded["benchmarks"]
            assert loaded["benchmarks"]["BM_Test"]["real_time"] == 100.0

    def test_save_and_load_develop_branch(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            manager = BaselineManager(baselines_dir=tmpdir)
            data = {"BM_X": {"real_time": 200.0}}
            manager.save_baseline(data, branch="develop", version="1.1.0", commit="xyz")
            loaded = manager.load_baseline(branch="develop")

            assert loaded is not None
            assert loaded["branch"] == "develop"

    def test_save_and_load_release(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            manager = BaselineManager(baselines_dir=tmpdir)
            data = {"BM_Prod": {"real_time": 50.0}}
            manager.save_baseline(
                data, branch="main", version="2.0.0", commit="tag", is_release=True
            )
            loaded = manager.load_baseline(version="2.0.0")

            assert loaded is not None
            assert loaded["version"] == "2.0.0"

    def test_load_nonexistent_returns_none(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            manager = BaselineManager(baselines_dir=tmpdir)
            loaded = manager.load_baseline(branch="main")
            assert loaded is None

    def test_list_baselines(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            manager = BaselineManager(baselines_dir=tmpdir)
            data = {"BM_A": {"real_time": 10.0}}
            manager.save_baseline(data, branch="main", version="1.0.0", commit="a")
            manager.save_baseline(
                data, branch="main", version="1.0.1", commit="b", is_release=True
            )

            listing = manager.list_baselines()
            assert len(listing["main"]) == 1
            assert len(listing["releases"]) == 1

    def test_parse_benchmark_json(self):
        """Round-trip: write a Google Benchmark JSON file, parse it."""
        with tempfile.TemporaryDirectory() as tmpdir:
            manager = BaselineManager(baselines_dir=tmpdir)
            bench_json = {
                "context": {"host_name": "test"},
                "benchmarks": [
                    {"name": "BM_A", "real_time": 1.0, "cpu_time": 1.0,
                     "iterations": 1000, "items_per_second": 1_000_000.0},
                    {"name": "BM_B", "real_time": 2.0, "cpu_time": 2.0,
                     "iterations": 500},
                ],
            }
            p = Path(tmpdir) / "results.json"
            with open(p, "w") as f:
                json.dump(bench_json, f)

            result = manager.load_benchmark_results(str(p))
            assert "BM_A" in result
            assert "BM_B" in result
            assert result["BM_A"]["real_time"] == 1.0
            assert result["BM_A"]["items_per_second"] == 1_000_000.0
            assert result["BM_B"]["items_per_second"] is None


# ===========================================================================
# Acceleration baseline file — structural validation
# ===========================================================================

class TestAccelerationBaseline:
    """
    Sanity-check the committed baseline file used by the CI workflow to ensure
    it is well-formed and compatible with the regression detector.
    """

    BASELINE_PATH = (
        Path(__file__).parent.parent / "baselines" / "acceleration" / "baseline.json"
    )

    def test_baseline_file_exists(self):
        assert self.BASELINE_PATH.exists(), (
            f"Baseline file not found: {self.BASELINE_PATH}"
        )

    def test_baseline_has_required_fields(self):
        with open(self.BASELINE_PATH) as f:
            data = json.load(f)
        for field in ("version", "branch", "commit", "timestamp", "benchmarks"):
            assert field in data, f"Missing field: {field}"

    def test_baseline_has_cpu_benchmarks(self):
        with open(self.BASELINE_PATH) as f:
            data = json.load(f)
        benchmarks = data["benchmarks"]
        assert len(benchmarks) > 0
        for name in benchmarks:
            assert name.startswith("BM_CPU_"), (
                f"Non-CPU benchmark in acceleration baseline: {name}"
            )

    def test_baseline_metric_values_are_positive(self):
        with open(self.BASELINE_PATH) as f:
            data = json.load(f)
        for name, metrics in data["benchmarks"].items():
            for metric, value in metrics.items():
                if value is not None:
                    assert value > 0, (
                        f"Non-positive value for {name}[{metric}]: {value}"
                    )

    def test_baseline_compatible_with_regression_detector(self):
        """Ensure the detector can load and process the baseline without error."""
        with open(self.BASELINE_PATH) as f:
            baseline = json.load(f)

        current = {
            "version": "1.0.1",
            "branch": "test",
            "commit": "fff",
            "timestamp": "2026-02-23T00:00:00Z",
            "benchmarks": {
                k: dict(v) for k, v in baseline["benchmarks"].items()
            },
        }

        detector = PerformanceRegressionDetector()
        comparisons = detector.compare_benchmarks(baseline, current)

        regressions = [c for c in comparisons if c.is_regression()]
        assert regressions == [], (
            f"Unexpected regressions when comparing baseline to itself: {regressions}"
        )
