"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            chimera_regression_detector.py                     ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-06 04:04:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     430                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8dbef46dde  2026-03-14  fix: move shebang to line 1 and fix from __future__ impor... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 0829d54f32  2026-02-28  feat(chimera): automated benchmark CI pipeline with regre... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
from __future__ import annotations

"""
Chimera Benchmark Regression Detector

Detects performance regressions in CHIMERA suite benchmark results by
comparing current harness output against a stored baseline.  Designed
to be called from the chimera-benchmark-ci.yml GitHub Actions workflow.

Baseline / current file format (produced by run_ci_benchmarks.py)::

    {
        "version":   "<semver>",
        "branch":    "<git-branch>",
        "commit":    "<git-sha>",
        "timestamp": "<ISO-8601>",
        "workloads": {
            "<workload_id>": {
                "throughput_ops_per_sec": <float>,
                "mean_latency_ms":        <float>,
                "p95_latency_ms":         <float>,
                "p99_latency_ms":         <float>
            },
            ...
        }
    }

Regression semantics:
    - ``throughput_ops_per_sec`` : HIGHER_IS_BETTER  → decrease is a regression
    - ``mean_latency_ms``        : LOWER_IS_BETTER   → increase is a regression
    - ``p95_latency_ms``         : LOWER_IS_BETTER   → increase is a regression
    - ``p99_latency_ms``         : LOWER_IS_BETTER   → increase is a regression

Exit codes:
    0  No blocking regressions (or no baseline to compare against).
    1  Blocking regressions detected (at or above --fail-on threshold).
    2  Fatal error (bad arguments, missing files, JSON parse error).
"""

import argparse
import json
import sys
from dataclasses import dataclass
from enum import Enum
from pathlib import Path
from typing import Dict, List, Optional, Any


# ---------------------------------------------------------------------------
# Data model
# ---------------------------------------------------------------------------

# Metrics tracked per workload and whether a *higher* value is better.
_TRACKED_METRICS: Dict[str, bool] = {
    "throughput_ops_per_sec": True,   # higher is better
    "mean_latency_ms":        False,  # lower is better
    "p95_latency_ms":         False,  # lower is better
    "p99_latency_ms":         False,  # lower is better
}


class RegressionSeverity(Enum):
    NONE     = "none"
    MINOR    = "minor"     # change >= threshold_minor
    MAJOR    = "major"     # change >= threshold_major
    CRITICAL = "critical"  # change >= threshold_critical


@dataclass
class WorkloadComparison:
    """Comparison result for a single workload/metric pair."""

    workload_id:     str
    metric:          str
    higher_is_better: bool
    baseline_value:  float
    current_value:   float
    percent_change:  float   # positive = current higher than baseline
    severity:        RegressionSeverity

    def is_regression(self) -> bool:
        """True when the change degrades performance."""
        if self.higher_is_better:
            return self.percent_change < 0   # throughput went down
        return self.percent_change > 0       # latency went up

    def is_improvement(self) -> bool:
        """True when the change improves performance significantly."""
        if self.higher_is_better:
            return self.percent_change > 0 and self.severity != RegressionSeverity.NONE
        return self.percent_change < 0 and self.severity != RegressionSeverity.NONE


# ---------------------------------------------------------------------------
# Detector
# ---------------------------------------------------------------------------

class ChimeraRegressionDetector:
    """Compares chimera benchmark results against a stored baseline."""

    DEFAULT_THRESHOLDS = {
        "minor":    5.0,
        "major":   10.0,
        "critical": 20.0,
    }

    def __init__(self, thresholds: Optional[Dict[str, float]] = None) -> None:
        self.thresholds = thresholds or dict(self.DEFAULT_THRESHOLDS)

    # ------------------------------------------------------------------
    # Public API
    # ------------------------------------------------------------------

    def compare(
        self,
        baseline: Dict[str, Any],
        current: Dict[str, Any],
    ) -> List[WorkloadComparison]:
        """Compare *current* workload results against *baseline*.

        Only workloads present in both are compared; new workloads in
        *current* are silently skipped (not a regression).

        Returns:
            List of :class:`WorkloadComparison` objects, one per
            (workload, metric) pair that could be compared.
        """
        baseline_workloads = baseline.get("workloads", {})
        current_workloads  = current.get("workloads", {})

        comparisons: List[WorkloadComparison] = []

        for wid, current_metrics in current_workloads.items():
            if wid not in baseline_workloads:
                continue  # new workload — not a regression

            baseline_metrics = baseline_workloads[wid]

            for metric, higher_is_better in _TRACKED_METRICS.items():
                b_val = baseline_metrics.get(metric)
                c_val = current_metrics.get(metric)

                if b_val is None or c_val is None:
                    continue
                if b_val == 0:
                    continue

                pct_change = (c_val - b_val) / abs(b_val) * 100.0
                severity   = self._classify(abs(pct_change))

                comparisons.append(WorkloadComparison(
                    workload_id=wid,
                    metric=metric,
                    higher_is_better=higher_is_better,
                    baseline_value=b_val,
                    current_value=c_val,
                    percent_change=pct_change,
                    severity=severity,
                ))

        return comparisons

    def has_blocking_regressions(
        self,
        comparisons: List[WorkloadComparison],
        block_threshold: str = "major",
    ) -> bool:
        """Return True if any regression is at or above *block_threshold*."""
        _order = ["minor", "major", "critical"]
        block_idx = _order.index(block_threshold)

        for c in comparisons:
            if not c.is_regression():
                continue
            if c.severity == RegressionSeverity.NONE:
                continue
            if _order.index(c.severity.value) >= block_idx:
                return True
        return False

    def generate_report(
        self,
        comparisons: List[WorkloadComparison],
        baseline_meta: Dict[str, Any],
        current_meta: Dict[str, Any],
    ) -> str:
        """Return a formatted plain-text regression report."""
        regressions = {
            "critical": [c for c in comparisons
                         if c.severity == RegressionSeverity.CRITICAL and c.is_regression()],
            "major":    [c for c in comparisons
                         if c.severity == RegressionSeverity.MAJOR    and c.is_regression()],
            "minor":    [c for c in comparisons
                         if c.severity == RegressionSeverity.MINOR    and c.is_regression()],
        }
        improvements = [c for c in comparisons if c.is_improvement()]

        lines: List[str] = []
        sep = "=" * 80

        lines += [sep, "CHIMERA BENCHMARK REGRESSION REPORT", sep, ""]

        # Metadata
        lines.append("COMPARISON INFO:")
        lines.append(f"  Baseline : {baseline_meta.get('version', 'unknown')}"
                     f" @ {str(baseline_meta.get('commit', 'unknown'))[:8]}"
                     f"  ({baseline_meta.get('branch', 'unknown')})")
        lines.append(f"  Current  : {current_meta.get('version', 'unknown')}"
                     f" @ {str(current_meta.get('commit', 'unknown'))[:8]}"
                     f"  ({current_meta.get('branch', 'unknown')})")
        lines.append(f"  Timestamp: {current_meta.get('timestamp', 'unknown')}")

        # Summary
        lines += [
            "",
            "SUMMARY:",
            f"  Critical regressions : {len(regressions['critical'])}",
            f"  Major regressions    : {len(regressions['major'])}",
            f"  Minor regressions    : {len(regressions['minor'])}",
            f"  Improvements         : {len(improvements)}",
            f"  Total compared       : {len(comparisons)}",
            "",
            "THRESHOLDS:",
            f"  Minor    : {self.thresholds['minor']}%",
            f"  Major    : {self.thresholds['major']}%",
            f"  Critical : {self.thresholds['critical']}%",
        ]

        for label, items in [
            ("CRITICAL REGRESSIONS (>= {:.0f}%)".format(self.thresholds["critical"]),
             regressions["critical"]),
            ("MAJOR REGRESSIONS    (>= {:.0f}%)".format(self.thresholds["major"]),
             regressions["major"]),
            ("MINOR REGRESSIONS    (>= {:.0f}%)".format(self.thresholds["minor"]),
             regressions["minor"]),
        ]:
            if not items:
                continue
            lines += ["", sep, label, sep]
            for c in sorted(items, key=lambda x: abs(x.percent_change), reverse=True):
                self._format_comparison(c, lines)

        if improvements:
            lines += ["", sep, "TOP IMPROVEMENTS", sep]
            for c in sorted(improvements,
                            key=lambda x: abs(x.percent_change), reverse=True)[:10]:
                self._format_comparison(c, lines)

        # Verdict
        lines += ["", sep, "VERDICT:"]
        if regressions["critical"] or regressions["major"]:
            lines.append("  FAILED – Significant performance regressions detected.")
            lines.append(
                f"  {len(regressions['critical'])} critical, "
                f"{len(regressions['major'])} major regression(s) found."
            )
        elif regressions["minor"]:
            lines.append("  WARNING – Minor performance regressions detected.")
            lines.append(f"  {len(regressions['minor'])} minor regression(s) found.")
        else:
            lines.append("  PASSED – No significant regressions detected.")
        lines.append(sep)

        return "\n".join(lines)

    # ------------------------------------------------------------------
    # Internal helpers
    # ------------------------------------------------------------------

    def _classify(self, abs_pct: float) -> RegressionSeverity:
        if abs_pct >= self.thresholds["critical"]:
            return RegressionSeverity.CRITICAL
        if abs_pct >= self.thresholds["major"]:
            return RegressionSeverity.MAJOR
        if abs_pct >= self.thresholds["minor"]:
            return RegressionSeverity.MINOR
        return RegressionSeverity.NONE

    @staticmethod
    def _format_comparison(c: WorkloadComparison, lines: List[str]) -> None:
        sign = "+" if c.percent_change > 0 else ""
        direction = "higher-is-better" if c.higher_is_better else "lower-is-better"
        lines.append(
            f"\n  Workload : {c.workload_id}\n"
            f"  Metric   : {c.metric}  ({direction})\n"
            f"  Baseline : {c.baseline_value:,.4f}\n"
            f"  Current  : {c.current_value:,.4f}\n"
            f"  Change   : {sign}{c.percent_change:.2f}%"
        )


# ---------------------------------------------------------------------------
# CLI entry-point
# ---------------------------------------------------------------------------

def _build_arg_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="Detect performance regressions in CHIMERA suite benchmarks."
    )
    p.add_argument("--baseline", required=True,
                   help="Path to baseline JSON file.")
    p.add_argument("--current",  required=True,
                   help="Path to current benchmark results JSON file.")
    p.add_argument("--output",   default="chimera_regression_report.txt",
                   help="Path for the plain-text report (default: chimera_regression_report.txt).")
    p.add_argument("--fail-on",  default="major",
                   choices=["minor", "major", "critical"],
                   help="Exit 1 if regressions at or above this severity are found.")
    p.add_argument("--threshold-minor",    type=float, default=5.0,
                   help="Threshold for minor regressions (%%); default 5.")
    p.add_argument("--threshold-major",    type=float, default=10.0,
                   help="Threshold for major regressions (%%); default 10.")
    p.add_argument("--threshold-critical", type=float, default=20.0,
                   help="Threshold for critical regressions (%%); default 20.")
    return p


def main(argv: Optional[List[str]] = None) -> int:  # noqa: D401
    """CLI entry-point; returns an exit code."""
    parser = _build_arg_parser()
    args = parser.parse_args(argv)

    thresholds = {
        "minor":    args.threshold_minor,
        "major":    args.threshold_major,
        "critical": args.threshold_critical,
    }
    detector = ChimeraRegressionDetector(thresholds=thresholds)

    # Load files
    try:
        with open(args.baseline, encoding="utf-8") as fh:
            baseline = json.load(fh)
        print(f"Loaded baseline : {args.baseline}")
    except (FileNotFoundError, json.JSONDecodeError) as exc:
        print(f"ERROR loading baseline: {exc}", file=sys.stderr)
        return 2

    try:
        with open(args.current, encoding="utf-8") as fh:
            current = json.load(fh)
        print(f"Loaded current  : {args.current}")
    except (FileNotFoundError, json.JSONDecodeError) as exc:
        print(f"ERROR loading current results: {exc}", file=sys.stderr)
        return 2

    # If baseline has no workloads yet (seed/empty), skip comparison.
    if not baseline.get("workloads"):
        print("Baseline has no workloads — skipping regression comparison.")
        report_text = (
            "CHIMERA BENCHMARK REGRESSION REPORT\n"
            "No baseline workloads available; skipping regression comparison.\n"
            "VERDICT: SKIPPED"
        )
        Path(args.output).write_text(report_text, encoding="utf-8")
        return 0

    comparisons = detector.compare(baseline, current)
    report_text = detector.generate_report(comparisons, baseline, current)

    # Save report
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(report_text, encoding="utf-8")
    print(f"Report saved    : {output_path}")

    # Save JSON summary alongside the text report
    json_path = output_path.with_suffix(".json")
    json_summary = {
        "summary": {
            "critical":     len([c for c in comparisons
                                 if c.severity == RegressionSeverity.CRITICAL and c.is_regression()]),
            "major":        len([c for c in comparisons
                                 if c.severity == RegressionSeverity.MAJOR    and c.is_regression()]),
            "minor":        len([c for c in comparisons
                                 if c.severity == RegressionSeverity.MINOR    and c.is_regression()]),
            "improvements": len([c for c in comparisons if c.is_improvement()]),
            "total":        len(comparisons),
        },
        "comparisons": [
            {
                "workload_id":     c.workload_id,
                "metric":          c.metric,
                "higher_is_better": c.higher_is_better,
                "baseline_value":  c.baseline_value,
                "current_value":   c.current_value,
                "percent_change":  c.percent_change,
                "severity":        c.severity.value,
                "is_regression":   c.is_regression(),
            }
            for c in comparisons
        ],
    }
    json_path.write_text(json.dumps(json_summary, indent=2), encoding="utf-8")
    print(f"JSON summary    : {json_path}")

    print(report_text)

    if detector.has_blocking_regressions(comparisons, args.fail_on):
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
