#!/usr/bin/env python3
"""AdaLoRA↔TT Bridge — Result Collection and Statistical Reporting Tool.

Reads Google Benchmark JSON output files for AdaLoRA↔TT bridge benchmark runs,
computes required statistics (mean, stddev, CV, p50, p95, p99, Cohen's d,
bootstrap CI), validates against protocol minimums, and writes a summary.json
that conforms to result_schema.json.

Usage:
    python3 collect_results.py \\
        --input bt1_cold.json [bt1_warm.json ...] \\
        --track bt1 \\
        --variant cold_load_rank8 \\
        --env env.json \\
        --git-sha <40-char SHA> \\
        --output results/2026-09-15_bt1_cold_load_rank8_a3f2c1b0/summary.json

Exit codes:
    0  All validity checks passed.
    1  One or more validity checks failed (run is invalid for publication).
    2  Input/argument error.

Notes:
    - Requires Python >= 3.10 and numpy (pip install numpy).
    - Optionally validates output against result_schema.json if jsonschema is
      installed (pip install jsonschema).
    - Does NOT fabricate or interpolate results. All statistics are derived
      exclusively from the raw benchmark JSON input files.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import random
import statistics
import sys
from pathlib import Path
from typing import Any

# ---------------------------------------------------------------------------
# Constants (aligned with ADALORA_TT_BRIDGE_BENCHMARK_PROTOCOL.md §4)
# ---------------------------------------------------------------------------

SCHEMA_VERSION = "1.0"
MIN_RUNS = 30
CV_CEILING_PCT = 15.0
CV_INVALIDATION_PCT = 25.0
BOOTSTRAP_ITERATIONS = 10_000
BOOTSTRAP_SEED = 42


# ---------------------------------------------------------------------------
# Data loading
# ---------------------------------------------------------------------------

def load_benchmark_json(path: Path) -> dict[str, Any]:
    """Load a Google Benchmark JSON output file."""
    try:
        with path.open(encoding="utf-8") as f:
            return json.load(f)
    except (OSError, json.JSONDecodeError) as e:
        print(f"ERROR: Cannot load {path}: {e}", file=sys.stderr)
        sys.exit(2)


def extract_samples(bench_data: dict[str, Any], filter_prefix: str = "") -> dict[str, list[float]]:
    """Extract per-benchmark latency samples (real_time in nanoseconds → µs).

    Returns a dict mapping benchmark name -> list of real_time_us samples.
    Aggregate rows ('_mean', '_stddev', '_cv', '_median', '_iqr', '_max', '_min')
    are excluded so only raw repetition rows are kept.
    """
    samples: dict[str, list[float]] = {}
    aggregate_suffixes = ("_mean", "_stddev", "_cv", "_median", "_iqr", "_max", "_min")

    for bm in bench_data.get("benchmarks", []):
        name: str = bm.get("name", "")
        if any(name.endswith(s) for s in aggregate_suffixes):
            continue
        if filter_prefix and not name.startswith(filter_prefix):
            continue
        # Google Benchmark reports real_time in nanoseconds by default.
        rt_ns: float = bm.get("real_time", float("nan"))
        rt_us = rt_ns / 1_000.0
        samples.setdefault(name, []).append(rt_us)

    return samples


# ---------------------------------------------------------------------------
# Statistics
# ---------------------------------------------------------------------------

def percentile(data: list[float], pct: float) -> float:
    """Return the p-th percentile of data (nearest-rank method)."""
    if not data:
        return float("nan")
    sorted_data = sorted(data)
    n = len(sorted_data)
    idx = max(0, min(n - 1, int(math.ceil(pct / 100.0 * n)) - 1))
    return sorted_data[idx]


def coefficient_of_variation(data: list[float]) -> float:
    """Return CV as a percentage. Returns 0.0 for empty or zero-mean inputs."""
    if len(data) < 2:
        return 0.0
    mean = statistics.mean(data)
    if abs(mean) < 1e-12:
        return 0.0
    return (statistics.stdev(data) / mean) * 100.0


def cohens_d(treatment: list[float], baseline: list[float]) -> float | None:
    """Compute Cohen's d effect size between treatment and baseline samples."""
    if len(treatment) < 2 or len(baseline) < 2:
        return None
    mean_t = statistics.mean(treatment)
    mean_b = statistics.mean(baseline)
    sd_t = statistics.stdev(treatment)
    sd_b = statistics.stdev(baseline)
    pooled_sd = math.sqrt((sd_t ** 2 + sd_b ** 2) / 2.0)
    if pooled_sd < 1e-12:
        return 0.0
    return (mean_t - mean_b) / pooled_sd


def bootstrap_ci(data: list[float], stat_fn=statistics.median,
                  n_iter: int = BOOTSTRAP_ITERATIONS,
                  seed: int = BOOTSTRAP_SEED,
                  confidence: float = 0.95) -> tuple[float, float]:
    """Compute bootstrap confidence interval for a statistic.

    Returns (lower, upper) for the given confidence level.
    """
    rng = random.Random(seed)
    n = len(data)
    if n < 2:
        v = stat_fn(data) if data else float("nan")
        return v, v

    bootstrap_stats: list[float] = []
    for _ in range(n_iter):
        sample = [data[rng.randint(0, n - 1)] for _ in range(n)]
        bootstrap_stats.append(stat_fn(sample))

    bootstrap_stats.sort()
    alpha = 1.0 - confidence
    lo_idx = max(0, int(math.floor(alpha / 2 * n_iter)))
    hi_idx = min(n_iter - 1, int(math.ceil((1.0 - alpha / 2) * n_iter)))
    return bootstrap_stats[lo_idx], bootstrap_stats[hi_idx]


def compute_latency_stats(samples: list[float]) -> dict[str, float]:
    """Compute the full latency statistics object for result_schema.json."""
    if not samples:
        return {"mean": float("nan"), "stddev": float("nan"), "cv_pct": float("nan"),
                "p50": float("nan"), "p95": float("nan"), "p99": float("nan")}
    return {
        "mean": statistics.mean(samples),
        "stddev": statistics.stdev(samples) if len(samples) > 1 else 0.0,
        "cv_pct": coefficient_of_variation(samples),
        "p50": percentile(samples, 50),
        "p95": percentile(samples, 95),
        "p99": percentile(samples, 99),
    }


# ---------------------------------------------------------------------------
# Validity checks
# ---------------------------------------------------------------------------

def check_validity(measurements: list[dict[str, Any]],
                   track_id: str,
                   warmup_protocol_followed: bool,
                   bt4_gate_cleared: bool,
                   corpus_type: str,
                   env_file: Path | None) -> tuple[dict[str, Any], list[str]]:
    """Evaluate all validity flags and return (flags_dict, list_of_violations)."""
    violations: list[str] = []

    n_runs_ok = all(m["n_runs"] >= MIN_RUNS for m in measurements)
    if not n_runs_ok:
        bad = [m["experiment_id"] for m in measurements if m["n_runs"] < MIN_RUNS]
        violations.append(f"n_runs < {MIN_RUNS} for: {bad}")

    cv_values: list[float] = []
    for m in measurements:
        lat = m.get("latency_us")
        if lat and lat.get("cv_pct") is not None and not math.isnan(lat["cv_pct"]):
            cv_values.append(lat["cv_pct"])

    cv_max = max(cv_values, default=0.0)
    cv_ok = cv_max <= CV_CEILING_PCT
    if not cv_ok:
        if cv_max > CV_INVALIDATION_PCT:
            violations.append(
                f"CV {cv_max:.1f}% exceeds invalidation threshold {CV_INVALIDATION_PCT}% "
                "— run is INVALID for publication."
            )
        else:
            violations.append(
                f"CV {cv_max:.1f}% exceeds ceiling {CV_CEILING_PCT}% — review required."
            )

    if track_id == "bt4" and not bt4_gate_cleared:
        violations.append(
            "BT-4 gate not cleared (GGML bridge wiring Stub #271 outstanding). "
            "BT-4 results must not be published."
        )

    env_present = env_file is not None and env_file.exists()
    if not env_present:
        violations.append("env.json not found — environment descriptor missing.")

    flags = {
        "n_runs_sufficient": n_runs_ok,
        "cv_within_ceiling": cv_ok,
        "cv_max_pct": round(cv_max, 2),
        "warmup_protocol_followed": warmup_protocol_followed,
        "bt4_gate_cleared": bt4_gate_cleared,
        "corpus_type_declared": corpus_type in ("synthetic", "real"),
        "env_descriptor_present": env_present,
    }
    return flags, violations


# ---------------------------------------------------------------------------
# Schema validation (optional)
# ---------------------------------------------------------------------------

def validate_against_schema(summary: dict[str, Any], schema_path: Path) -> list[str]:
    """Validate summary dict against result_schema.json if jsonschema is installed."""
    try:
        import jsonschema  # noqa: PLC0415
        with schema_path.open(encoding="utf-8") as f:
            schema = json.load(f)
        errors = list(jsonschema.Draft7Validator(schema).iter_errors(summary))
        return [str(e.message) for e in errors]
    except ImportError:
        return []  # jsonschema not installed — skip silently
    except Exception as e:  # noqa: BLE001
        return [f"Schema validation error: {e}"]


# ---------------------------------------------------------------------------
# CLI argument parsing
# ---------------------------------------------------------------------------

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Collect and validate AdaLoRA↔TT bridge benchmark results.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--input", nargs="+", required=True, metavar="FILE",
        help="One or more Google Benchmark JSON output files.",
    )
    parser.add_argument(
        "--track", required=True, choices=["bt1", "bt2", "bt3", "bt4"],
        help="Benchmark track identifier.",
    )
    parser.add_argument(
        "--variant", default="unnamed",
        help="Experiment variant descriptor (e.g., 'cold_load_rank8').",
    )
    parser.add_argument(
        "--env", metavar="ENV_JSON", default=None,
        help="Path to env.json environment descriptor.",
    )
    parser.add_argument(
        "--git-sha", default="0" * 40, metavar="SHA",
        help="Full 40-character ThemisDB git SHA (use: git rev-parse HEAD).",
    )
    parser.add_argument(
        "--output", required=True, metavar="SUMMARY_JSON",
        help="Output path for summary.json.",
    )
    parser.add_argument(
        "--corpus-type", choices=["synthetic", "real"], default="synthetic",
        help="Adapter corpus type (default: synthetic).",
    )
    parser.add_argument(
        "--warmup-protocol-followed", action="store_true", default=False,
        help="Assert that 3-phase warmup protocol was applied.",
    )
    parser.add_argument(
        "--bt4-gate-cleared", action="store_true", default=False,
        help="Assert BT-4 GGML bridge gate is cleared (use only after Stub #271 is resolved).",
    )
    parser.add_argument(
        "--baseline-input", nargs="+", default=None, metavar="FILE",
        help="Google Benchmark JSON files for baseline (for Cohen's d calculation).",
    )
    parser.add_argument(
        "--filter", default="", metavar="PREFIX",
        help="Filter benchmark names by prefix when extracting samples.",
    )
    parser.add_argument(
        "--date", default=None,
        help="Run date (YYYY-MM-DD). Defaults to today.",
    )
    return parser.parse_args()


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> int:
    import datetime

    args = parse_args()

    run_date = args.date or datetime.date.today().isoformat()

    # Load input files
    all_samples: dict[str, list[float]] = {}
    for p in args.input:
        data = load_benchmark_json(Path(p))
        for name, vals in extract_samples(data, filter_prefix=args.filter).items():
            all_samples.setdefault(name, []).extend(vals)

    # Load baseline samples (for Cohen's d)
    baseline_samples: dict[str, list[float]] = {}
    if args.baseline_input:
        for p in args.baseline_input:
            data = load_benchmark_json(Path(p))
            for name, vals in extract_samples(data, filter_prefix=args.filter).items():
                baseline_samples.setdefault(name, []).extend(vals)

    # Build measurement records from extracted samples
    measurements: list[dict[str, Any]] = []
    for bm_name, samples in sorted(all_samples.items()):
        latency_stats = compute_latency_stats(samples)
        ci_low, ci_high = bootstrap_ci(samples, stat_fn=statistics.median)

        cd: float | None = None
        if bm_name in baseline_samples:
            cd = cohens_d(samples, baseline_samples[bm_name])

        # Infer experiment_id from benchmark name (strip repetition suffix if present)
        # Format: BT<N>-<variant-parts>
        exp_id = bm_name.split("/")[0]  # strip Google Benchmark parameterisation
        if not exp_id.startswith("BT"):
            exp_id = f"BT{args.track[2]}-{exp_id}"

        rec: dict[str, Any] = {
            "experiment_id": exp_id,
            "configuration": {
                "method": "bridge",  # placeholder — override in post-processing if needed
            },
            "n_runs": len(samples),
            "latency_us": latency_stats,
            "bytes_before": None,
            "bytes_after": None,
            "dedup_ratio": None,
            "dedup_hits": None,
            "fp_count": None,
            "recon_error_mean": None,
            "recon_error_max": None,
            "rank_before": None,
            "rank_after": None,
            "downstream_accuracy_delta": None,
            "downstream_task": None,
            "cohens_d": round(cd, 4) if cd is not None else None,
            "ci_95_low": round(ci_low, 2),
            "ci_95_high": round(ci_high, 2),
        }
        measurements.append(rec)

    # Compute env_sha256
    env_path = Path(args.env) if args.env else None
    if env_path and env_path.exists():
        env_sha = hashlib.sha256(env_path.read_bytes()).hexdigest()[:8]
    else:
        env_sha = "00000000"

    # Validate
    validity_flags, violations = check_validity(
        measurements=measurements,
        track_id=args.track,
        warmup_protocol_followed=args.warmup_protocol_followed,
        bt4_gate_cleared=args.bt4_gate_cleared,
        corpus_type=args.corpus_type,
        env_file=env_path,
    )

    # Assemble summary
    summary: dict[str, Any] = {
        "schema_version": SCHEMA_VERSION,
        "track_id": args.track,
        "variant": args.variant,
        "date": run_date,
        "env_sha256": env_sha,
        "themisdb_git_sha": args.git_sha,
        "gate_status": "cleared" if (args.track == "bt4" and args.bt4_gate_cleared)
                        else ("not_applicable" if args.track != "bt4" else "blocked"),
        "corpus_type": args.corpus_type,
        "measurements": measurements,
        "validity_flags": validity_flags,
        "notes": "",
    }

    # Optional JSON Schema validation
    schema_path = Path(__file__).parent / "result_schema.json"
    schema_errors = validate_against_schema(summary, schema_path)
    if schema_errors:
        print("Schema validation warnings:", file=sys.stderr)
        for e in schema_errors:
            print(f"  - {e}", file=sys.stderr)

    # Write output
    out_path = Path(args.output)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("w", encoding="utf-8") as f:
        json.dump(summary, f, indent=2, ensure_ascii=False)
    print(f"Written: {out_path}")

    # Report validity
    if violations:
        print("\nVALIDITY VIOLATIONS (run is invalid for publication):", file=sys.stderr)
        for v in violations:
            print(f"  ✗ {v}", file=sys.stderr)
        return 1

    print("\nValidity: PASS — all protocol minimums met.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
