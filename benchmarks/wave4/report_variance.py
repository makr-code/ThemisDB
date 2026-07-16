#!/usr/bin/env python3
"""
report_variance.py — Wave 4 Variance Analysis and Regression Reporting Tool

Consumes Google Benchmark JSON output (--benchmark_out_format=json) and
emits:
  1. Per-benchmark variance statistics (p50/p95/p99, CV, sample count).
  2. Regression hints vs a provided baseline JSON.
  3. Structured repro steps for any failing gate.
  4. A human-readable summary table.

Usage
-----
  # Analyse a single result file (no baseline comparison):
  python3 report_variance.py --input results.json

  # Compare against a baseline and check Wave 4 gates:
  python3 report_variance.py \\
    --input current.json \\
    --baseline baseline.json \\
    --manifest benchmarks/wave4/release_gate_manifest.json \\
    --output report.json

Exit codes
----------
  0  All gates passed (or no gates defined).
  1  One or more RELEASE-BLOCKING gates failed.
  2  Input file not found or invalid format.
"""

from __future__ import annotations

import argparse
import json
import math
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Tuple


# ---------------------------------------------------------------------------
# Data models
# ---------------------------------------------------------------------------

@dataclass
class BenchmarkEntry:
    """A single benchmark result extracted from the JSON output."""
    name: str
    real_time: float        # ns
    cpu_time: float         # ns
    items_per_second: float
    bytes_per_second: float
    iterations: int
    counters: Dict[str, float] = field(default_factory=dict)

    @property
    def p50_ns(self) -> Optional[float]:
        return self.counters.get("p50_ns")

    @property
    def p95_ns(self) -> Optional[float]:
        return self.counters.get("p95_ns")

    @property
    def p99_ns(self) -> Optional[float]:
        return self.counters.get("p99_ns")

    @property
    def cv(self) -> Optional[float]:
        return self.counters.get("cv")

    @property
    def gate_id(self) -> Optional[int]:
        v = self.counters.get("gate_id")
        return int(v) if v is not None else None


@dataclass
class GateResult:
    """Evaluation result for a single release gate."""
    gate_id: str
    name: str
    criticality: str
    metric: str
    direction: str
    threshold_pct: float
    baseline_value: Optional[float]
    current_value: Optional[float]
    pct_change: Optional[float]
    passed: bool
    message: str
    repro_cmd: str


# ---------------------------------------------------------------------------
# Parsing
# ---------------------------------------------------------------------------

def load_benchmark_json(path: Path) -> List[BenchmarkEntry]:
    """Load a Google Benchmark JSON output file and return a list of entries."""
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (FileNotFoundError, json.JSONDecodeError) as exc:
        print(f"ERROR: Cannot load {path}: {exc}", file=sys.stderr)
        sys.exit(2)

    entries: List[BenchmarkEntry] = []
    for bm in data.get("benchmarks", []):
        name = bm.get("name", "")
        if bm.get("run_type") == "aggregate":
            continue  # skip mean/median/stddev aggregates

        counters: Dict[str, float] = {}
        for k, v in bm.items():
            if k not in {"name", "run_name", "run_type", "repetitions",
                         "repetition_index", "threads", "iterations",
                         "real_time", "cpu_time", "time_unit",
                         "items_per_second", "bytes_per_second",
                         "error_occurred", "error_message"}:
                try:
                    counters[k] = float(v)
                except (TypeError, ValueError):
                    pass

        entries.append(BenchmarkEntry(
            name=name,
            real_time=float(bm.get("real_time", 0.0)),
            cpu_time=float(bm.get("cpu_time", 0.0)),
            items_per_second=float(bm.get("items_per_second", 0.0)),
            bytes_per_second=float(bm.get("bytes_per_second", 0.0)),
            iterations=int(bm.get("iterations", 0)),
            counters=counters,
        ))

    return entries


def build_lookup(entries: List[BenchmarkEntry]) -> Dict[str, BenchmarkEntry]:
    return {e.name: e for e in entries}


# ---------------------------------------------------------------------------
# Variance statistics (computed from VarianceTracker counters)
# ---------------------------------------------------------------------------

def format_ns(ns: Optional[float]) -> str:
    if ns is None:
        return "n/a"
    if ns >= 1_000_000_000:
        return f"{ns / 1_000_000_000:.3f} s"
    if ns >= 1_000_000:
        return f"{ns / 1_000_000:.3f} ms"
    if ns >= 1_000:
        return f"{ns / 1_000:.3f} µs"
    return f"{ns:.1f} ns"


def print_variance_table(entries: List[BenchmarkEntry]) -> None:
    """Print a human-readable variance summary table."""
    header = f"{'Benchmark':<60} {'P50':>12} {'P95':>12} {'P99':>12} {'CV':>8} {'Samples':>8}"
    print("\n" + "=" * len(header))
    print("VARIANCE SUMMARY")
    print("=" * len(header))
    print(header)
    print("-" * len(header))

    for e in sorted(entries, key=lambda x: x.name):
        has_variance = e.p50_ns is not None or e.p99_ns is not None
        if not has_variance:
            continue
        cv_str = f"{e.cv:.3f}" if e.cv is not None else "n/a"
        samples_str = str(int(e.counters.get("samples", 0))) if "samples" in e.counters else "n/a"
        print(
            f"{e.name:<60} "
            f"{format_ns(e.p50_ns):>12} "
            f"{format_ns(e.p95_ns):>12} "
            f"{format_ns(e.p99_ns):>12} "
            f"{cv_str:>8} "
            f"{samples_str:>8}"
        )
    print()


# ---------------------------------------------------------------------------
# Gate evaluation
# ---------------------------------------------------------------------------

def get_metric_value(entry: BenchmarkEntry, metric: str) -> Optional[float]:
    """Extract a named metric from a benchmark entry."""
    if metric == "items_per_second":
        return entry.items_per_second if entry.items_per_second > 0 else None
    if metric == "bytes_per_second":
        return entry.bytes_per_second if entry.bytes_per_second > 0 else None
    if metric == "real_time":
        return entry.real_time
    if metric == "cpu_time":
        return entry.cpu_time
    return entry.counters.get(metric)


def evaluate_gate(
    gate: dict,
    current_lookup: Dict[str, BenchmarkEntry],
    baseline_lookup: Optional[Dict[str, BenchmarkEntry]],
) -> GateResult:
    """Evaluate one gate definition against current (and optional baseline) results."""
    gate_id   = gate["id"]
    name      = gate["name"]
    filt      = gate["benchmark_filter"]
    metric    = gate["metric"]
    direction = gate["direction"]
    threshold = float(gate.get("regression_threshold_pct",
                               gate.get("max_degradation_pct_vs_baseline", 10)))
    criticality = gate.get("criticality", "advisory")

    # Build the repro command.
    bench_bin = gate.get("benchmark", "bench_w4a_release_gates")
    repro_cmd = (
        f"./{bench_bin} "
        f"--benchmark_filter='{filt}' "
        f"--benchmark_out=results_{gate_id}.json "
        f"--benchmark_out_format=json "
        f"--benchmark_repetitions=5"
    )

    # Look up current result.
    current_entry = current_lookup.get(filt)
    if current_entry is None:
        # Try prefix match.
        for k, v in current_lookup.items():
            if k.startswith(filt):
                current_entry = v
                break

    current_val: Optional[float] = None
    if current_entry is not None:
        current_val = get_metric_value(current_entry, metric)

    if current_val is None:
        return GateResult(
            gate_id=gate_id, name=name, criticality=criticality,
            metric=metric, direction=direction, threshold_pct=threshold,
            baseline_value=None, current_value=None,
            pct_change=None, passed=False,
            message=f"MISSING: benchmark '{filt}' not found in results.",
            repro_cmd=repro_cmd,
        )

    # Look up baseline.
    baseline_val: Optional[float] = None
    if baseline_lookup is not None:
        baseline_entry = baseline_lookup.get(filt)
        if baseline_entry is None:
            for k, v in baseline_lookup.items():
                if k.startswith(filt):
                    baseline_entry = v
                    break
        if baseline_entry is not None:
            baseline_val = get_metric_value(baseline_entry, metric)

    if baseline_val is None or baseline_val == 0.0:
        # No baseline — emit informational result only.
        return GateResult(
            gate_id=gate_id, name=name, criticality=criticality,
            metric=metric, direction=direction, threshold_pct=threshold,
            baseline_value=None, current_value=current_val,
            pct_change=None, passed=True,
            message="INFO: No baseline available — result recorded for future comparison.",
            repro_cmd=repro_cmd,
        )

    pct_change = ((current_val - baseline_val) / abs(baseline_val)) * 100.0

    # Determine if this is a regression.
    is_regression: bool
    if direction == "lower_is_better":
        # Positive pct_change means metric went up (worse).
        is_regression = pct_change > threshold
    else:
        # higher_is_better: negative pct_change means metric went down (worse).
        is_regression = pct_change < -threshold

    passed = not is_regression
    if passed:
        msg = (
            f"PASS: {metric} = {current_val:.2f} "
            f"(Δ {pct_change:+.1f}%, threshold {threshold}%)"
        )
    else:
        msg = (
            f"FAIL [{criticality.upper()}]: {metric} regressed by {abs(pct_change):.1f}% "
            f"(threshold {threshold}%). "
            f"Baseline: {baseline_val:.2f}, Current: {current_val:.2f}."
        )

    return GateResult(
        gate_id=gate_id, name=name, criticality=criticality,
        metric=metric, direction=direction, threshold_pct=threshold,
        baseline_value=baseline_val, current_value=current_val,
        pct_change=pct_change, passed=passed,
        message=msg, repro_cmd=repro_cmd,
    )


def evaluate_all_gates(
    manifest: dict,
    current_lookup: Dict[str, BenchmarkEntry],
    baseline_lookup: Optional[Dict[str, BenchmarkEntry]],
) -> Tuple[List[GateResult], List[GateResult]]:
    """Return (all_results, failing_release_blocking_gates)."""
    results: List[GateResult] = []

    for gate in manifest.get("gates", []):
        results.append(evaluate_gate(gate, current_lookup, baseline_lookup))

    for gate in manifest.get("resilience_reference_gates", []):
        results.append(evaluate_gate(gate, current_lookup, baseline_lookup))

    failing_blocking = [
        r for r in results
        if not r.passed and r.criticality == "release-blocking"
    ]
    return results, failing_blocking


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

def print_gate_report(results: List[GateResult]) -> None:
    print("\n" + "=" * 80)
    print("RELEASE GATE EVALUATION")
    print("=" * 80)

    for r in results:
        status = "✅ PASS" if r.passed else "❌ FAIL"
        print(f"\n[{r.gate_id}] {r.name}")
        print(f"  Status    : {status}")
        print(f"  Metric    : {r.metric} ({r.direction})")
        print(f"  Threshold : {r.threshold_pct:.1f}%")
        if r.baseline_value is not None:
            print(f"  Baseline  : {r.baseline_value:.4f}")
        if r.current_value is not None:
            print(f"  Current   : {r.current_value:.4f}")
        if r.pct_change is not None:
            print(f"  Change    : {r.pct_change:+.2f}%")
        print(f"  Message   : {r.message}")
        if not r.passed:
            print(f"  Repro     : {r.repro_cmd}")


def print_summary(results: List[GateResult], blocking_failures: List[GateResult]) -> None:
    total = len(results)
    passed = sum(1 for r in results if r.passed)
    advisory_failures = [r for r in results if not r.passed and r.criticality != "release-blocking"]

    print("\n" + "=" * 80)
    print("SUMMARY")
    print("=" * 80)
    print(f"Total gates evaluated : {total}")
    print(f"Passed                : {passed}")
    print(f"Failed (blocking)     : {len(blocking_failures)}")
    print(f"Failed (advisory)     : {len(advisory_failures)}")

    if blocking_failures:
        print("\n⛔ RELEASE BLOCKED — the following gates failed:")
        for r in blocking_failures:
            print(f"   [{r.gate_id}] {r.name}: {r.message}")
    else:
        print("\n✅ All release-blocking gates passed.")

    if advisory_failures:
        print("\n⚠️  Advisory failures (non-blocking):")
        for r in advisory_failures:
            print(f"   [{r.gate_id}] {r.name}: {r.message}")


def write_json_report(
    output_path: Path,
    entries: List[BenchmarkEntry],
    gate_results: List[GateResult],
    blocking_failures: List[GateResult],
) -> None:
    report = {
        "summary": {
            "total_gates": len(gate_results),
            "passed": sum(1 for r in gate_results if r.passed),
            "blocking_failures": len(blocking_failures),
            "release_blocked": len(blocking_failures) > 0,
        },
        "gates": [
            {
                "gate_id": r.gate_id,
                "name": r.name,
                "criticality": r.criticality,
                "passed": r.passed,
                "metric": r.metric,
                "baseline_value": r.baseline_value,
                "current_value": r.current_value,
                "pct_change": r.pct_change,
                "message": r.message,
                "repro_cmd": r.repro_cmd,
            }
            for r in gate_results
        ],
        "variance_stats": [
            {
                "name": e.name,
                "p50_ns": e.p50_ns,
                "p95_ns": e.p95_ns,
                "p99_ns": e.p99_ns,
                "cv": e.cv,
                "samples": e.counters.get("samples"),
            }
            for e in entries
            if e.p50_ns is not None or e.p99_ns is not None
        ],
    }
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(f"\nJSON report written to: {output_path}")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Wave 4 variance analysis and regression reporting tool.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--input", required=True, type=Path,
        help="Google Benchmark JSON result file (--benchmark_out_format=json).",
    )
    parser.add_argument(
        "--baseline", type=Path, default=None,
        help="Baseline JSON result file for regression comparison.",
    )
    parser.add_argument(
        "--manifest", type=Path,
        default=Path(__file__).parent / "release_gate_manifest.json",
        help="Release gate manifest JSON (default: release_gate_manifest.json).",
    )
    parser.add_argument(
        "--output", type=Path, default=None,
        help="Write structured JSON report to this path.",
    )
    parser.add_argument(
        "--no-gates", action="store_true",
        help="Skip gate evaluation (variance analysis only).",
    )
    args = parser.parse_args()

    # Load inputs.
    entries = load_benchmark_json(args.input)
    current_lookup = build_lookup(entries)

    baseline_lookup: Optional[Dict[str, BenchmarkEntry]] = None
    if args.baseline is not None:
        baseline_entries = load_benchmark_json(args.baseline)
        baseline_lookup = build_lookup(baseline_entries)

    # Variance table.
    print_variance_table(entries)

    gate_results: List[GateResult] = []
    blocking_failures: List[GateResult] = []

    if not args.no_gates and args.manifest.exists():
        try:
            manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
        except json.JSONDecodeError as exc:
            print(f"ERROR: Invalid manifest JSON: {exc}", file=sys.stderr)
            return 2

        gate_results, blocking_failures = evaluate_all_gates(
            manifest, current_lookup, baseline_lookup
        )
        print_gate_report(gate_results)
        print_summary(gate_results, blocking_failures)
    elif not args.no_gates:
        print(f"INFO: Manifest not found at {args.manifest} — skipping gate evaluation.")

    if args.output is not None:
        write_json_report(args.output, entries, gate_results, blocking_failures)

    return 1 if blocking_failures else 0


if __name__ == "__main__":
    sys.exit(main())
