#!/usr/bin/env python3
"""Wave 8 variance and drift reporting tool.

Reads Google Benchmark JSON output and computes per-benchmark:
  - mean, stddev, CV (%), p50, p95, p99 (from Repetitions output)
  - gate_passed flag based on release_gate_manifest_w8.json thresholds
  - drift signal: rolling-segment slope and CV for THD-03/04/05

Usage:
  python3 report_variance_w8.py --input <bench_output.json> \\
      [--manifest release_gate_manifest_w8.json] \\
      [--baseline <prior_baseline.json>] \\
      [--output <report.json>]

Exit code:
  0  All hard gates passed.
  1  One or more hard gates failed.
  2  Usage / file error.
"""

from __future__ import annotations

import argparse
import json
import math
import statistics
import sys
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple


# ---------------------------------------------------------------------------
# Data loading
# ---------------------------------------------------------------------------

def load_json(path: Path) -> Dict[str, Any]:
    with path.open(encoding="utf-8") as f:
        return json.load(f)


# ---------------------------------------------------------------------------
# Statistics helpers
# ---------------------------------------------------------------------------

def coefficient_of_variation(samples: List[float]) -> float:
    """Return CV as percentage.  Returns 0.0 for empty or zero-mean inputs."""
    if len(samples) < 2:
        return 0.0
    mean = statistics.mean(samples)
    if abs(mean) < 1e-12:
        return 0.0
    return (statistics.stdev(samples) / mean) * 100.0


def percentile(sorted_samples: List[float], pct: float) -> float:
    """Return the @pct percentile from a sorted list (0–100)."""
    if not sorted_samples:
        return float("nan")
    idx = min(int(len(sorted_samples) * pct / 100.0), len(sorted_samples) - 1)
    return sorted_samples[idx]


def trend_slope(values: List[float]) -> float:
    """Return the linear least-squares slope of values[i] vs i.
    A positive slope means values are rising.
    """
    n = len(values)
    if n < 2:
        return 0.0
    sx = sum(range(n))
    sy = sum(values)
    sxy = sum(i * v for i, v in enumerate(values))
    sxx = sum(i * i for i in range(n))
    denom = n * sxx - sx * sx
    if abs(denom) < 1e-12:
        return 0.0
    return (n * sxy - sx * sy) / denom


def delta_pct(baseline: float, current: float, higher_is_better: bool) -> float:
    """Return % change from baseline to current, sign-corrected for direction."""
    if abs(baseline) < 1e-12:
        return 0.0
    raw = (current - baseline) / baseline * 100.0
    return raw if not higher_is_better else -raw


# ---------------------------------------------------------------------------
# Benchmark result aggregation
# ---------------------------------------------------------------------------

def aggregate_benchmarks(raw: Dict[str, Any]) -> Dict[str, Dict[str, Any]]:
    """Group repetition results by base benchmark name and compute stats.

    Also aggregates structured counters emitted by drift-detection benchmarks.
    """
    groups: Dict[str, List[float]] = {}
    counters: Dict[str, Dict[str, List[float]]] = {}

    for bench in raw.get("benchmarks", []):
        name: str = bench.get("name", "")
        # Strip repetition suffix
        base = name.split("/repeats:")[0]

        real_time = bench.get("real_time")
        if real_time is not None:
            groups.setdefault(base, []).append(float(real_time))

        # Collect custom counters (drift signals, gate flags, throughput fields, etc.)
        for key, value in bench.items():
            if key in ("name", "run_name", "run_type", "repetitions",
                       "repetition_index", "threads", "iterations",
                       "real_time", "cpu_time", "time_unit"):
                continue
            if isinstance(value, (int, float)):
                counters.setdefault(base, {}).setdefault(key, []).append(float(value))

    results: Dict[str, Dict[str, Any]] = {}
    for name, samples in groups.items():
        sorted_s = sorted(samples)
        results[name] = {
            "n": len(samples),
            "mean": statistics.mean(samples) if samples else float("nan"),
            "stddev": statistics.stdev(samples) if len(samples) > 1 else 0.0,
            "cv_pct": coefficient_of_variation(samples),
            "p50": percentile(sorted_s, 50),
            "p95": percentile(sorted_s, 95),
            "p99": percentile(sorted_s, 99),
        }
        # Attach aggregated counter means
        for ctr_name, ctr_values in counters.get(name, {}).items():
            results[name][f"ctr_{ctr_name}"] = statistics.mean(ctr_values) if ctr_values else 0.0

    return results


# ---------------------------------------------------------------------------
# Drift analysis
# ---------------------------------------------------------------------------

def analyse_drift(
    aggregated: Dict[str, Dict[str, Any]],
    manifest: Dict[str, Any],
) -> List[Dict[str, Any]]:
    """Analyse drift signals from THD-03/04/05 benchmarks."""
    drift_results = []
    policy = manifest.get("regression_policy", {})
    alert_pct = float(policy.get("drift_alert_threshold_pct", 8.0))

    drift_benchmarks = {
        "W8B/THD03_ReadThroughput_drift_detection": {
            "cv_field": "ctr_read_drift_cv_pct",
            "slope_field": "ctr_read_drift_slope_ops_s_per_seg",
        },
        "W8B/THD04_WriteThroughput_drift_detection": {
            "cv_field": "ctr_write_drift_cv_pct",
            "slope_field": "ctr_write_drift_slope_ops_s_per_seg",
        },
        "W8B/THD05_Latency_trend_slope": {
            "cv_field": None,
            "slope_field": "ctr_latency_trend_slope_us_per_seg",
        },
    }

    for bench_name, fields in drift_benchmarks.items():
        stats = aggregated.get(bench_name)
        if not stats:
            continue

        cv   = stats.get(fields["cv_field"], 0.0) if fields["cv_field"] else None
        slope = stats.get(fields["slope_field"], 0.0)

        cv_alert   = (cv is not None) and (cv > alert_pct)
        slope_alert = (slope > 0.0) if "Latency" in bench_name else (abs(slope) > alert_pct)

        drift_results.append({
            "benchmark": bench_name,
            "cv_pct": cv,
            "slope": slope,
            "cv_alert": cv_alert,
            "slope_alert": slope_alert,
            "alert": cv_alert or slope_alert,
        })

    return drift_results


# ---------------------------------------------------------------------------
# Baseline comparison
# ---------------------------------------------------------------------------

def compare_baseline(
    aggregated: Dict[str, Dict[str, Any]],
    baseline: Dict[str, Any],
    tolerance_pct: float,
) -> List[Dict[str, Any]]:
    """Compare current results to a frozen baseline and report regressions."""
    comparisons = []
    for bench_name, stats in aggregated.items():
        prior = baseline.get("aggregated", {}).get(bench_name)
        if not prior:
            continue
        prior_mean = prior.get("mean", 0.0)
        current_mean = stats.get("mean", 0.0)
        if abs(prior_mean) < 1e-12:
            continue

        pct_change = (current_mean - prior_mean) / prior_mean * 100.0
        regression = pct_change > tolerance_pct
        comparisons.append({
            "benchmark": bench_name,
            "prior_mean": prior_mean,
            "current_mean": current_mean,
            "delta_pct": pct_change,
            "tolerance_pct": tolerance_pct,
            "regression": regression,
        })
    return comparisons


# ---------------------------------------------------------------------------
# Gate evaluation
# ---------------------------------------------------------------------------

GateResult = Dict[str, Any]

METRIC_MAP: Dict[str, str] = {
    "latency_p99_us":         "p99",
    "latency_p99_ms":         "p99",
    "latency_mean_us":        "mean",
    "items_per_second":       "ctr_items_per_second",
    "throughput_drift_pct":   "cv_pct",
    "cv_percent":             "cv_pct",
    "read_drift_cv_pct":      "ctr_read_drift_cv_pct",
    "write_drift_cv_pct":     "ctr_write_drift_cv_pct",
    "latency_trend_rising":   "ctr_latency_trend_rising",
    "flake_cv_pct":           "ctr_flake_cv_pct",
    "p99_us":                 "p99",
    "ops_per_s":              "mean",
    "mismatches":             "mean",
    "seed_mismatch_count":    "ctr_seed_mismatch_count",
    "triage_completeness_score": "ctr_triage_completeness_score",
    "guardrail_coverage_score":  "ctr_guardrail_coverage_score",
    "baseline_fresh":         "ctr_baseline_fresh",
    "policy_compliance_score":"ctr_policy_compliance_score",
}


def evaluate_gates(
    aggregated: Dict[str, Dict[str, Any]],
    manifest: Dict[str, Any],
) -> List[GateResult]:
    results: List[GateResult] = []

    def check(gates: List[Dict[str, Any]], severity: str) -> None:
        for gate in gates:
            bench_name: str = gate["benchmark"]
            metric: str = gate["metric"]
            threshold: float = float(gate["threshold"])
            direction: str = gate["direction"]

            stats = aggregated.get(bench_name)
            if stats is None:
                results.append({
                    "id": gate["id"],
                    "benchmark": bench_name,
                    "metric": metric,
                    "threshold": threshold,
                    "measured": None,
                    "passed": False,
                    "severity": severity,
                    "reason": "benchmark not found in input",
                })
                continue

            field = METRIC_MAP.get(metric, "mean")
            measured: float = stats.get(field, float("nan"))

            if math.isnan(measured):
                passed = False
            elif direction == "lower_is_better":
                passed = measured <= threshold
            else:
                passed = measured >= threshold

            results.append({
                "id": gate["id"],
                "benchmark": bench_name,
                "metric": metric,
                "threshold": threshold,
                "measured": measured,
                "passed": passed,
                "severity": severity,
                "description": gate.get("description", ""),
            })

    check(manifest.get("hard_gates", []), "blocking")
    check(manifest.get("soft_gates", []), "warning")
    return results


# ---------------------------------------------------------------------------
# Report formatting
# ---------------------------------------------------------------------------

def print_report(
    aggregated: Dict[str, Dict[str, Any]],
    gate_results: List[GateResult],
    drift_results: List[Dict[str, Any]],
    baseline_comparisons: Optional[List[Dict[str, Any]]] = None,
) -> None:
    sep = "=" * 76
    print(f"\n{sep}")
    print("Wave 8 Variance, Drift & Gate Report")
    print(sep)

    # Per-benchmark statistics
    print("\n--- Per-Benchmark Statistics ---")
    header = f"{'Benchmark':<55} {'n':>3} {'mean':>9} {'cv%':>6} {'p99':>9}"
    print(header)
    print("-" * len(header))
    for name, stats in sorted(aggregated.items()):
        print(
            f"{name:<55} {stats['n']:>3} "
            f"{stats['mean']:>9.2f} "
            f"{stats['cv_pct']:>6.1f} "
            f"{stats['p99']:>9.2f}"
        )

    # Drift analysis
    if drift_results:
        print("\n--- Drift Analysis ---")
        for dr in drift_results:
            icon = "⚠️ " if dr["alert"] else "✅"
            cv_str = f"cv={dr['cv_pct']:.1f}%" if dr["cv_pct"] is not None else ""
            print(f"  {icon} {dr['benchmark']}")
            print(f"      slope={dr['slope']:.4f}  {cv_str}")

    # Baseline comparison
    if baseline_comparisons:
        print("\n--- Baseline Comparison ---")
        regressions = [c for c in baseline_comparisons if c["regression"]]
        for c in sorted(baseline_comparisons, key=lambda x: abs(x["delta_pct"]), reverse=True)[:10]:
            icon = "🔴" if c["regression"] else "🟢"
            print(
                f"  {icon} {c['benchmark']}"
                f"\n      prior={c['prior_mean']:.2f}  current={c['current_mean']:.2f}"
                f"  Δ={c['delta_pct']:+.1f}% (tol={c['tolerance_pct']}%)"
            )
        if len(baseline_comparisons) > 10:
            print(f"  ... and {len(baseline_comparisons) - 10} more benchmarks compared.")
        if regressions:
            print(f"\n  ⚠️  {len(regressions)} regression(s) detected vs baseline.")

    # Gate evaluation
    print("\n--- Gate Evaluation ---")
    hard_fail = [r for r in gate_results if not r["passed"] and r["severity"] == "blocking"]
    soft_fail = [r for r in gate_results if not r["passed"] and r["severity"] == "warning"]

    for r in gate_results:
        icon = "✅" if r["passed"] else ("❌" if r["severity"] == "blocking" else "⚠️ ")
        measured_str = f"{r['measured']:.4f}" if r["measured"] is not None else "N/A"
        print(
            f"  {icon} [{r['id']}] {r['benchmark']}"
            f"\n      metric={r['metric']} measured={measured_str}"
            f" threshold={r['threshold']} ({r['severity']})"
        )

    print(
        f"\nSummary: {len(gate_results)} gates — "
        f"{len(hard_fail)} HARD FAIL, {len(soft_fail)} SOFT WARN"
    )
    print(sep)


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Wave 8 variance, drift, and gate reporter."
    )
    parser.add_argument("--input",    required=True,
                        help="Google Benchmark JSON output file.")
    parser.add_argument(
        "--manifest",
        default=str(Path(__file__).parent / "release_gate_manifest_w8.json"),
        help="Gate manifest JSON (default: release_gate_manifest_w8.json).",
    )
    parser.add_argument("--baseline", default=None,
                        help="Prior baseline JSON for regression comparison.")
    parser.add_argument("--output",   default=None,
                        help="Write JSON report to this file.")
    args = parser.parse_args()

    try:
        raw      = load_json(Path(args.input))
        manifest = load_json(Path(args.manifest))
    except (FileNotFoundError, json.JSONDecodeError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2

    aggregated    = aggregate_benchmarks(raw)
    gate_results  = evaluate_gates(aggregated, manifest)
    drift_results = analyse_drift(aggregated, manifest)

    baseline_comparisons: Optional[List[Dict[str, Any]]] = None
    if args.baseline:
        try:
            baseline_json = load_json(Path(args.baseline))
            tol = float(
                manifest.get("regression_policy", {}).get("tolerance_percent", 5.0)
            )
            baseline_comparisons = compare_baseline(aggregated, baseline_json, tol)
        except (FileNotFoundError, json.JSONDecodeError) as exc:
            print(f"WARNING: baseline load failed: {exc}", file=sys.stderr)

    print_report(aggregated, gate_results, drift_results, baseline_comparisons)

    if args.output:
        report: Dict[str, Any] = {
            "aggregated": aggregated,
            "gate_results": gate_results,
            "drift_results": drift_results,
        }
        if baseline_comparisons is not None:
            report["baseline_comparisons"] = baseline_comparisons
        Path(args.output).write_text(json.dumps(report, indent=2), encoding="utf-8")
        print(f"\nReport written to: {args.output}")

    hard_failures = [r for r in gate_results if not r["passed"] and r["severity"] == "blocking"]
    return 1 if hard_failures else 0


if __name__ == "__main__":
    sys.exit(main())
