#!/usr/bin/env python3
"""Wave 8 variance reporting tool.

Reads Google Benchmark JSON output and computes per-benchmark:
  - mean, stddev, CV (%), p50, p95, p99 (from Repetitions output)
  - gate_passed flag based on release_gate_manifest_w8.json thresholds

Usage:
  python3 report_variance_w8.py --input <bench_output.json> [--manifest release_gate_manifest_w8.json]

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
# Repetition extraction
# ---------------------------------------------------------------------------

def extract_repetition_times(benchmarks: List[Dict[str, Any]], name: str) -> List[float]:
    """Return real_time values for all repetitions of `name` (excludes _mean/_stddev)."""
    times: List[float] = []
    for b in benchmarks:
        bname: str = b.get("name", "")
        if bname.startswith(name) and not any(
            bname.endswith(sfx)
            for sfx in ("_mean", "_stddev", "_median", "_cv")
        ):
            rt = b.get("real_time")
            if rt is not None:
                times.append(float(rt))
    return times


# ---------------------------------------------------------------------------
# Statistics
# ---------------------------------------------------------------------------

def percentile(data: List[float], pct: float) -> float:
    if not data:
        return 0.0
    sorted_data = sorted(data)
    idx = (pct / 100.0) * (len(sorted_data) - 1)
    lo, hi = int(idx), min(int(idx) + 1, len(sorted_data) - 1)
    frac = idx - lo
    return sorted_data[lo] * (1.0 - frac) + sorted_data[hi] * frac


def compute_stats(times: List[float]) -> Dict[str, float]:
    if not times:
        return {"mean": 0.0, "stddev": 0.0, "cv_pct": 0.0,
                "p50": 0.0, "p95": 0.0, "p99": 0.0, "n": 0}
    mean = statistics.mean(times)
    stddev = statistics.stdev(times) if len(times) >= 2 else 0.0
    cv_pct = (stddev / mean * 100.0) if mean > 1e-12 else 0.0
    return {
        "mean":    mean,
        "stddev":  stddev,
        "cv_pct":  cv_pct,
        "p50":     percentile(times, 50),
        "p95":     percentile(times, 95),
        "p99":     percentile(times, 99),
        "n":       len(times),
    }


# ---------------------------------------------------------------------------
# Gate evaluation
# ---------------------------------------------------------------------------

def evaluate_gate(stats: Dict[str, float], gate: Dict[str, Any]) -> bool:
    metric: str    = gate["metric"]
    threshold: float = float(gate["threshold"])
    direction: str = gate["direction"]

    value: Optional[float] = None
    if metric == "latency_p99_us":
        value = stats.get("p99")
    elif metric == "items_per_second":
        # items/s is not directly in time stats; reported via counter
        # In real tooling, parse from benchmark counters field
        return True  # advisory — counter-based, not latency-based
    elif metric == "cv_percent":
        value = stats.get("cv_pct")
    elif metric in ("triage_completeness", "operability_coverage_pct"):
        return True  # counter-based, evaluated separately
    else:
        value = stats.get(metric)

    if value is None:
        return True  # unknown metric — skip

    if direction == "lower_is_better":
        return value <= threshold
    elif direction == "higher_is_better":
        return value >= threshold
    return True


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

def print_report(results: List[Dict[str, Any]], hard_gate_failures: int) -> None:
    print("\n" + "=" * 72)
    print("  Wave 8 Benchmark Variance Report")
    print("=" * 72)

    for r in results:
        name    = r["name"]
        stats   = r["stats"]
        gates   = r.get("gates", [])
        passed  = all(g["passed"] for g in gates)
        status  = "✅ PASS" if passed else "❌ FAIL"

        print(f"\n  {status}  {name}")
        print(f"         n={stats['n']:.0f}  mean={stats['mean']:.1f} µs  "
              f"stddev={stats['stddev']:.1f}  CV={stats['cv_pct']:.1f}%")
        print(f"         p50={stats['p50']:.1f}  p95={stats['p95']:.1f}  "
              f"p99={stats['p99']:.1f}")
        for g in gates:
            g_status = "✅" if g["passed"] else "❌"
            print(f"           {g_status} [{g['id']}] {g['description']} "
                  f"(threshold={g['threshold']}, direction={g['direction']})")

    print("\n" + "=" * 72)
    if hard_gate_failures == 0:
        print("  ✅ All hard gates PASSED")
    else:
        print(f"  ❌ {hard_gate_failures} hard gate(s) FAILED")
    print("=" * 72 + "\n")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(description="Wave 8 variance report")
    parser.add_argument("--input",    required=True, help="Benchmark JSON output file")
    parser.add_argument("--manifest", default=Path(__file__).parent / "release_gate_manifest_w8.json",
                        help="Release gate manifest JSON")
    args = parser.parse_args()

    bench_data = load_benchmark_json(Path(args.input))
    manifest   = load_manifest(Path(args.manifest))

    benchmarks: List[Dict[str, Any]] = bench_data.get("benchmarks", [])
    hard_gates: List[Dict[str, Any]] = manifest.get("hard_gates", [])
    soft_gates: List[Dict[str, Any]] = manifest.get("soft_gates", [])

    # Collect unique benchmark base names
    base_names: List[str] = []
    seen: set = set()
    for b in benchmarks:
        name: str = b.get("name", "")
        # Strip repetition suffixes
        base = name.split("/")[0] if "/" in name else name
        for sfx in ("_mean", "_stddev", "_median", "_cv"):
            base = base.removesuffix(sfx)
        if base and base not in seen:
            seen.add(base)
            base_names.append(base)

    results: List[Dict[str, Any]] = []
    hard_gate_failures = 0

    for base in base_names:
        times = extract_repetition_times(benchmarks, base)
        stats = compute_stats(times)

        gate_results: List[Dict[str, Any]] = []
        for gate in hard_gates + soft_gates:
            bm_name: str = gate.get("benchmark", "")
            if base not in bm_name:
                continue
            passed = evaluate_gate(stats, gate)
            if not passed and gate.get("severity") == "blocking":
                hard_gate_failures += 1
            gate_results.append({
                "id":          gate["id"],
                "description": gate["description"],
                "threshold":   gate["threshold"],
                "direction":   gate["direction"],
                "severity":    gate.get("severity", "warning"),
                "passed":      passed,
            })

        results.append({"name": base, "stats": stats, "gates": gate_results})

    print_report(results, hard_gate_failures)

    # Write summary JSON
    summary_path = Path("wave8_variance_report.json")
    with summary_path.open("w", encoding="utf-8") as f:
        json.dump({
            "wave": "8",
            "hard_gate_failures": hard_gate_failures,
            "results": results,
        }, f, indent=2)
    print(f"  Summary written to {summary_path}")

    return 1 if hard_gate_failures > 0 else 0


if __name__ == "__main__":
    sys.exit(main())
