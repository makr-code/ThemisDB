#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 ThemisDB Contributors
"""
report_variance_w5.py — Wave 5 Benchmark Variance & Regression Reporter

Usage
-----
# Compare a fresh run against a stored baseline:
    python benchmarks/wave5/report_variance_w5.py \\
        --input    bench_w5d.json \\
        --baseline benchmarks/baselines/wave5/bench_w5d_baseline.json

# Update a baseline (store the current run as the new reference):
    python benchmarks/wave5/report_variance_w5.py \\
        --input    bench_w5d.json \\
        --baseline benchmarks/baselines/wave5/bench_w5d_baseline.json \\
        --update-baseline

# Validate all gate counters only (no regression comparison):
    python benchmarks/wave5/report_variance_w5.py \\
        --input bench_w5d.json \\
        --gates-only

# Summarise variance from multiple runs (multi-run mode):
    python benchmarks/wave5/report_variance_w5.py \\
        --multi-run run1.json run2.json run3.json

Exit codes
----------
    0  All gates pass, regression within budget
    1  One or more gates failed or regression detected
    2  Invalid input (file not found, parse error)

Gate thresholds (aligned with release_gate_manifest_w5.json):
    gate_pass  counter == 1.0  → PASS; 0.0 → FAIL
    Regression: real_time increase > 10% over baseline → WARN
                real_time increase > 20% over baseline → FAIL
"""

from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path
from typing import Any

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

REGRESSION_WARN_PCT  = 10.0   # % increase triggers WARNING
REGRESSION_FAIL_PCT  = 20.0   # % increase triggers FAIL
CV_MAX_PCT           = 5.0    # maximum acceptable coefficient of variation

# ---------------------------------------------------------------------------
# Data loading
# ---------------------------------------------------------------------------


def load_json(path: str) -> dict[str, Any]:
    """Load and parse a Google Benchmark JSON output file."""
    try:
        with open(path, encoding="utf-8") as f:
            return json.load(f)
    except FileNotFoundError:
        print(f"ERROR: file not found: {path}", file=sys.stderr)
        sys.exit(2)
    except json.JSONDecodeError as exc:
        print(f"ERROR: JSON parse error in {path}: {exc}", file=sys.stderr)
        sys.exit(2)


def extract_benchmarks(data: dict[str, Any]) -> dict[str, dict[str, Any]]:
    """Return a flat {name: benchmark_entry} mapping from a benchmark JSON.

    Supports two formats:
    1. Google Benchmark native: ``{"benchmarks": [{"name": "...", ...}, ...]}``
    2. ThemisDB baseline dict: ``{"benchmarks": {"BM_Foo": {"real_time": ...}}}``
    """
    result: dict[str, dict[str, Any]] = {}
    raw = data.get("benchmarks", [])
    if isinstance(raw, list):
        for bm in raw:
            if isinstance(bm, dict) and "name" in bm:
                result[bm["name"]] = bm
    elif isinstance(raw, dict):
        for name, entry in raw.items():
            if isinstance(entry, dict) and not name.startswith("_"):
                result[name] = dict(entry, name=name)
    return result

# ---------------------------------------------------------------------------
# Gate validation
# ---------------------------------------------------------------------------


def check_gates(benchmarks: dict[str, dict[str, Any]]) -> tuple[int, int, list[str]]:
    """
    Inspect all benchmarks for gate_pass counter.

    Returns
    -------
    (pass_count, fail_count, messages)
    """
    passed, failed = 0, 0
    messages: list[str] = []

    for name, bm in benchmarks.items():
        counters: dict[str, Any] = bm.get("counters", {})
        if "gate_pass" not in counters:
            continue
        gate_val = counters["gate_pass"]
        if gate_val == 1.0:
            passed += 1
            messages.append(f"  ✓  {name} — gate PASS")
        else:
            failed += 1
            label = bm.get("label", "")
            messages.append(f"  ✗  {name} — gate FAIL  [{label}]")

    return passed, failed, messages

# ---------------------------------------------------------------------------
# Regression comparison
# ---------------------------------------------------------------------------


def compare_regression(
    current:  dict[str, dict[str, Any]],
    baseline: dict[str, dict[str, Any]],
) -> tuple[int, int, int, list[str]]:
    """
    Compare current run against baseline.

    Returns
    -------
    (ok_count, warn_count, fail_count, messages)
    """
    ok, warn, fail = 0, 0, 0
    messages: list[str] = []

    for name, cur in current.items():
        if name not in baseline:
            messages.append(f"  ?  {name} — not in baseline (new benchmark)")
            continue
        base = baseline[name]
        cur_rt  = cur.get("real_time",  0.0)
        base_rt = base.get("real_time", 0.0)

        if base_rt <= 0:
            messages.append(f"  ?  {name} — baseline real_time=0, skipping")
            continue

        delta_pct = (cur_rt - base_rt) / base_rt * 100.0

        if delta_pct >= REGRESSION_FAIL_PCT:
            fail += 1
            messages.append(
                f"  ✗  {name} — REGRESSION {delta_pct:+.1f}%"
                f"  (cur={cur_rt:.2f}  base={base_rt:.2f})"
            )
        elif delta_pct >= REGRESSION_WARN_PCT:
            warn += 1
            messages.append(
                f"  ⚠  {name} — WARNING    {delta_pct:+.1f}%"
                f"  (cur={cur_rt:.2f}  base={base_rt:.2f})"
            )
        else:
            ok += 1
            messages.append(
                f"  ✓  {name} — OK         {delta_pct:+.1f}%"
                f"  (cur={cur_rt:.2f}  base={base_rt:.2f})"
            )

    return ok, warn, fail, messages

# ---------------------------------------------------------------------------
# Multi-run variance summary
# ---------------------------------------------------------------------------


def _stats(values: list[float]) -> dict[str, float]:
    n = len(values)
    if n == 0:
        return {"mean": 0, "stddev": 0, "cv_pct": 0, "min": 0, "max": 0}
    mean = sum(values) / n
    var  = sum((x - mean) ** 2 for x in values) / n
    sd   = math.sqrt(var)
    cv   = (sd / mean * 100.0) if mean > 0 else 0.0
    return {
        "mean":    round(mean, 4),
        "stddev":  round(sd, 4),
        "cv_pct":  round(cv, 2),
        "min":     round(min(values), 4),
        "max":     round(max(values), 4),
    }


def multi_run_summary(paths: list[str]) -> int:
    """
    Summarise variance across multiple runs.

    Returns
    -------
    exit code (0 = all CV within budget, 1 = CV violations)
    """
    all_runs: list[dict[str, dict[str, Any]]] = [
        extract_benchmarks(load_json(p)) for p in paths
    ]

    # Collect per-benchmark real_time samples
    names: set[str] = set()
    for run in all_runs:
        names.update(run.keys())

    violations = 0
    rows: list[tuple[str, dict[str, float]]] = []
    for name in sorted(names):
        samples = [
            run[name]["real_time"]
            for run in all_runs
            if name in run and "real_time" in run[name]
        ]
        if len(samples) < 2:
            continue
        s = _stats(samples)
        rows.append((name, s))
        if s["cv_pct"] > CV_MAX_PCT:
            violations += 1

    print("\n=== Wave 5 Multi-Run Variance Summary ===")
    print(f"{'Benchmark':<60}  {'Mean':>8}  {'SD':>8}  {'CV%':>6}  {'Min':>8}  {'Max':>8}")
    print("-" * 100)
    for name, s in rows:
        cv_flag = " ⚠" if s["cv_pct"] > CV_MAX_PCT else "  "
        print(
            f"{name:<60}  {s['mean']:>8.2f}  {s['stddev']:>8.2f}"
            f"  {s['cv_pct']:>5.1f}%{cv_flag}"
            f"  {s['min']:>8.2f}  {s['max']:>8.2f}"
        )
    print("-" * 100)
    if violations:
        print(f"\nCV violations ({CV_MAX_PCT}% threshold): {violations}")
    else:
        print(f"\nAll CV within {CV_MAX_PCT}% threshold ✓")

    return 1 if violations > 0 else 0

# ---------------------------------------------------------------------------
# Baseline update
# ---------------------------------------------------------------------------


def update_baseline(
    input_data: dict[str, Any],
    baseline_path: str,
) -> None:
    """Overwrite the baseline file with the current run's results."""
    Path(baseline_path).parent.mkdir(parents=True, exist_ok=True)
    with open(baseline_path, "w", encoding="utf-8") as f:
        json.dump(input_data, f, indent=2)
    print(f"Baseline updated: {baseline_path}")

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Wave 5 benchmark variance & regression reporter",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--input", metavar="FILE",
        help="Google Benchmark JSON output file from the current run",
    )
    parser.add_argument(
        "--baseline", metavar="FILE",
        help="Baseline JSON file for regression comparison",
    )
    parser.add_argument(
        "--update-baseline", action="store_true",
        help="Replace the baseline file with the current run",
    )
    parser.add_argument(
        "--gates-only", action="store_true",
        help="Check gate_pass counters only; skip regression comparison",
    )
    parser.add_argument(
        "--multi-run", metavar="FILE", nargs="+",
        help="Compute variance across multiple run JSON files",
    )
    parser.add_argument(
        "--fail-on-warn", action="store_true",
        help="Treat regression WARNINGs as failures (strict mode)",
    )

    args = parser.parse_args()

    # Multi-run mode
    if args.multi_run:
        return multi_run_summary(args.multi_run)

    if not args.input:
        parser.error("--input is required (unless --multi-run is used)")

    current_data   = load_json(args.input)
    current_bms    = extract_benchmarks(current_data)
    exit_code      = 0

    # --- Gate validation ---
    g_pass, g_fail, g_msgs = check_gates(current_bms)
    print(f"\n=== Wave 5 Gate Validation: {args.input} ===")
    for m in g_msgs:
        print(m)
    print(f"\n  Gates: {g_pass} PASS  /  {g_fail} FAIL")
    if g_fail > 0:
        exit_code = 1

    if args.gates_only:
        return exit_code

    # --- Regression comparison ---
    if args.baseline and Path(args.baseline).exists():
        baseline_data = load_json(args.baseline)
        baseline_bms  = extract_benchmarks(baseline_data)
        r_ok, r_warn, r_fail, r_msgs = compare_regression(current_bms, baseline_bms)

        print(f"\n=== Wave 5 Regression Comparison (vs. {args.baseline}) ===")
        for m in r_msgs:
            print(m)
        print(
            f"\n  Regression: {r_ok} OK  /  {r_warn} WARN  /  {r_fail} FAIL"
        )
        if r_fail > 0 or (args.fail_on_warn and r_warn > 0):
            exit_code = 1
    elif args.baseline:
        print(f"\nBaseline not found: {args.baseline} — skipping regression check")
        print("Run with --update-baseline to create the initial baseline.")

    # --- Baseline update ---
    if args.update_baseline and args.baseline:
        update_baseline(current_data, args.baseline)

    if exit_code == 0:
        print("\n✓  All Wave 5 checks passed")
    else:
        print("\n✗  Wave 5 checks FAILED — see details above", file=sys.stderr)

    return exit_code


if __name__ == "__main__":
    sys.exit(main())
