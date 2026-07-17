#!/usr/bin/env python3
"""Wave 7 variance reporting tool.

Reads Google Benchmark JSON output and computes per-benchmark:
  - mean, stddev, CV (%), p50, p95, p99 (from Repetitions output)
  - gate_passed flag based on release_gate_manifest_w7.json thresholds

Usage:
  python3 report_variance_w7.py --input <bench_output.json> [--manifest release_gate_manifest_w7.json]

Exit code:
  0  All hard gates passed.
  1  One or more hard gates failed.
"""

from __future__ import annotations

import argparse
import json
import math
import statistics
import sys
from pathlib import Path
from typing import Any, Dict, List, Optional


# ---------------------------------------------------------------------------
# Data loading
# ---------------------------------------------------------------------------

def load_benchmark_json(path: Path) -> Dict[str, Any]:
    with path.open(encoding="utf-8") as f:
        return json.load(f)


def load_manifest(path: Path) -> Dict[str, Any]:
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
    idx = int(len(sorted_samples) * pct / 100.0)
    idx = min(idx, len(sorted_samples) - 1)
    return sorted_samples[idx]


# ---------------------------------------------------------------------------
# Benchmark result aggregation
# ---------------------------------------------------------------------------

def aggregate_benchmarks(raw: Dict[str, Any]) -> Dict[str, Dict[str, Any]]:
    """Group repetition results by base benchmark name and compute stats."""
    groups: Dict[str, List[float]] = {}
    for bench in raw.get("benchmarks", []):
        name: str = bench.get("name", "")
        # Strip repetition suffix e.g. "/repeats:5_mean" → base name
        base = name.split("/repeats:")[0]
        real_time = bench.get("real_time", None)
        if real_time is not None:
            groups.setdefault(base, []).append(float(real_time))

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
    return results


# ---------------------------------------------------------------------------
# Gate evaluation
# ---------------------------------------------------------------------------

GateResult = Dict[str, Any]


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

            # Map metric name → aggregated field
            metric_map = {
                "latency_p99_us": "p99",
                "latency_p99_ms": "p99",
                "items_per_second": "mean",
                "throughput_drift_pct": "cv_pct",
                "cv_percent": "cv_pct",
                "p99_us": "p99",
                "ops_per_s": "mean",
                "mismatches": "mean",
            }
            field = metric_map.get(metric, "mean")
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
) -> None:
    print("\n" + "=" * 72)
    print("Wave 7 Variance & Gate Report")
    print("=" * 72)

    print("\n--- Per-Benchmark Statistics ---")
    header = f"{'Benchmark':<55} {'n':>3} {'mean':>8} {'cv%':>6} {'p99':>8}"
    print(header)
    print("-" * len(header))
    for name, stats in sorted(aggregated.items()):
        print(
            f"{name:<55} {stats['n']:>3} "
            f"{stats['mean']:>8.1f} "
            f"{stats['cv_pct']:>6.1f} "
            f"{stats['p99']:>8.1f}"
        )

    print("\n--- Gate Evaluation ---")
    hard_fail = [r for r in gate_results if not r["passed"] and r["severity"] == "blocking"]
    soft_fail = [r for r in gate_results if not r["passed"] and r["severity"] == "warning"]

    for r in gate_results:
        icon = "✅" if r["passed"] else ("❌" if r["severity"] == "blocking" else "⚠️ ")
        measured_str = f"{r['measured']:.2f}" if r["measured"] is not None else "N/A"
        print(
            f"  {icon} [{r['id']}] {r['benchmark']}"
            f"\n      metric={r['metric']} measured={measured_str} threshold={r['threshold']}"
            f" ({r['severity']})"
        )

    print(f"\nSummary: {len(gate_results)} gates evaluated — "
          f"{len(hard_fail)} HARD FAIL, {len(soft_fail)} SOFT WARN")
    print("=" * 72)


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(description="Wave 7 variance and gate reporter.")
    parser.add_argument("--input", required=True, help="Google Benchmark JSON output file.")
    parser.add_argument(
        "--manifest",
        default=str(Path(__file__).parent / "release_gate_manifest_w7.json"),
        help="Gate manifest JSON (default: release_gate_manifest_w7.json).",
    )
    parser.add_argument("--output", default=None, help="Write JSON report to this file.")
    args = parser.parse_args()

    raw = load_benchmark_json(Path(args.input))
    manifest = load_manifest(Path(args.manifest))
    aggregated = aggregate_benchmarks(raw)
    gate_results = evaluate_gates(aggregated, manifest)
    print_report(aggregated, gate_results)

    if args.output:
        report = {
            "aggregated": aggregated,
            "gate_results": gate_results,
        }
        Path(args.output).write_text(json.dumps(report, indent=2), encoding="utf-8")
        print(f"\nReport written to: {args.output}")

    hard_failures = [r for r in gate_results if not r["passed"] and r["severity"] == "blocking"]
    return 1 if hard_failures else 0


if __name__ == "__main__":
    sys.exit(main())
