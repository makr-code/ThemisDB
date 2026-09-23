#!/usr/bin/env python3
"""
compare_expectations.py — ThemisDB Benchmark Expectation Comparator

Parses every src/<module>/PERFORMANCE_EXPECTATIONS.md file, extracts the
"Specific Expectations" table (Target ID | Expectation | Benchmark case),
then looks up matching benchmark results in the provided results directory
(Google Benchmark JSON output) and emits a Markdown comparison report.

Usage:
    python3 benchmarks/compare_expectations.py \
        --src-dir src \
        --results-dir benchmarks/results \
        --output benchmark_comparison_report.md \
        [--regression-threshold 0.20]

Exit codes:
    0 — all covered targets PASS or WARN
    1 — at least one covered target FAIL
"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional


# ---------------------------------------------------------------------------
# Data model
# ---------------------------------------------------------------------------

@dataclass
class Expectation:
    """One row from the Specific Expectations table."""
    module: str
    target_id: str
    expectation_text: str
    benchmark_cases: list[str]       # comma/space-separated names from the table
    # Parsed numeric constraint (if any)
    constraint_op: Optional[str] = None   # "<=", ">=", "<", ">", "=="
    constraint_value: Optional[float] = None
    constraint_unit: Optional[str] = None # "us", "ms", "s", "ops/s", "k/s", "%"


@dataclass
class BenchmarkResult:
    """One benchmark case from a Google Benchmark JSON result file."""
    name: str
    real_time_ns: Optional[float] = None   # real_time in nanoseconds
    cpu_time_ns: Optional[float] = None
    iterations: Optional[int] = None
    items_per_second: Optional[float] = None
    time_unit: str = "ns"


@dataclass
class TargetVerdict:
    module: str
    target_id: str
    expectation_text: str
    benchmark_cases: list[str]
    status: str = "NOT_RUN"   # PASS | WARN | FAIL | NOT_RUN | UNCHECKED
    detail: str = ""
    matched_results: list[BenchmarkResult] = field(default_factory=list)


# ---------------------------------------------------------------------------
# Parsing helpers
# ---------------------------------------------------------------------------

# Matches Markdown table rows: | col1 | col2 | col3 |
_TABLE_ROW_RE = re.compile(r"^\s*\|(.+)\|\s*$")

# Heuristics for extracting numeric thresholds from expectation text
# Examples: "<= 100 us", "> 6 k/s", ">= 1 000 ops/sec", "<= 10 ms",
#           "<= 10%", "< 5 %", ">= 1000", "<= 1 µs"
_CONSTRAINT_RE = re.compile(
    r"(<=|>=|<|>|==)\s*([\d][,\d\s]*[\d]|[\d]+)"
    r"(?:\s*(us|µs|ms|s|ops/s|ops/sec|k/s|%))?",
    re.IGNORECASE,
)

_UNIT_TO_NS = {
    "us": 1_000.0,
    "µs": 1_000.0,
    "ms": 1_000_000.0,
    "s":  1_000_000_000.0,
}


def _parse_number(text: str) -> float:
    """Remove thousand-separator spaces/commas and parse as float."""
    cleaned = text.replace(",", "").replace(" ", "")
    return float(cleaned)


def _parse_constraint(text: str) -> tuple[Optional[str], Optional[float], Optional[str]]:
    """Extract (operator, value, unit) from expectation text, or (None,None,None)."""
    m = _CONSTRAINT_RE.search(text)
    if not m:
        return None, None, None
    op = m.group(1)
    val = _parse_number(m.group(2))
    unit = (m.group(3) or "").lower().replace("ops/sec", "ops/s")
    return op, val, unit or None


def _split_benchmark_cases(raw: str) -> list[str]:
    """Split a comma/space-separated list of benchmark case names."""
    # Each case typically looks like BM_Foo_Bar or Fixture/CaseName
    parts = [p.strip().strip("`") for p in re.split(r"[,\n]+", raw) if p.strip()]
    result: list[str] = []
    for p in parts:
        # Further split on spaces only if they don't look like a single compound name
        for tok in p.split():
            cleaned = tok.strip("`")
            if cleaned:
                result.append(cleaned)
    return result


def _parse_performance_expectations(path: Path, module: str) -> list[Expectation]:
    """Parse the 'Specific Expectations' table from one PERFORMANCE_EXPECTATIONS.md."""
    expectations: list[Expectation] = []
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()

    in_specific_table = False
    header_seen = False

    for line in lines:
        # Detect the "Specific Expectations" section (English and German variants)
        if re.search(
            r"##\s*(?:Specific\s+Expectations|Spezifische\s+Erwartungswerte)",
            line, re.IGNORECASE
        ):
            in_specific_table = True
            header_seen = False
            continue

        # Exit section when a new ## heading starts
        if in_specific_table and re.match(r"^##\s", line):
            in_specific_table = False
            continue

        if not in_specific_table:
            continue

        m = _TABLE_ROW_RE.match(line)
        if not m:
            continue

        cols = [c.strip() for c in m.group(1).split("|")]
        if not cols:
            continue

        # Skip separator rows (---)
        if all(re.match(r"^[-:]+$", c) for c in cols if c):
            continue

        # First content row after separator is the header
        if not header_seen:
            header_seen = True
            continue

        # We need at least 3 columns: Target ID | Expectation | Benchmark case
        if len(cols) < 3:
            continue

        target_id = cols[0]
        expectation_text = cols[1]
        cases_raw = cols[2]

        # Skip empty / header-looking rows
        if not target_id or target_id.lower() in (
            "target id", "ziel-id", "gate id", "target-id", "ziel id"
        ):
            continue

        cases = _split_benchmark_cases(cases_raw)
        op, val, unit = _parse_constraint(expectation_text)

        expectations.append(Expectation(
            module=module,
            target_id=target_id,
            expectation_text=expectation_text,
            benchmark_cases=cases,
            constraint_op=op,
            constraint_value=val,
            constraint_unit=unit,
        ))

    return expectations


def _discover_expectations(src_dir: Path) -> list[Expectation]:
    """Walk src/<module>/PERFORMANCE_EXPECTATIONS.md and collect all expectations."""
    all_exp: list[Expectation] = []
    for perf_file in sorted(src_dir.rglob("PERFORMANCE_EXPECTATIONS.md")):
        module = perf_file.parent.name
        all_exp.extend(_parse_performance_expectations(perf_file, module))
    return all_exp


# ---------------------------------------------------------------------------
# Result loading
# ---------------------------------------------------------------------------

def _time_to_ns(value: float, unit: str) -> float:
    """Convert benchmark time value to nanoseconds."""
    unit = unit.lower()
    factors = {"ns": 1.0, "us": 1e3, "µs": 1e3, "ms": 1e6, "s": 1e9}
    return value * factors.get(unit, 1.0)


def _load_benchmark_json(path: Path) -> dict[str, BenchmarkResult]:
    """Load a Google Benchmark JSON file, return {case_name: BenchmarkResult}."""
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError) as exc:
        print(f"[WARN] Could not parse {path}: {exc}", file=sys.stderr)
        return {}

    results: dict[str, BenchmarkResult] = {}
    for bm in data.get("benchmarks", []):
        name: str = bm.get("name", "")
        time_unit: str = bm.get("time_unit", "ns")
        raw_real = bm.get("real_time", None)
        raw_cpu = bm.get("cpu_time", None)

        real_ns = _time_to_ns(float(raw_real), time_unit) if raw_real is not None else None
        cpu_ns = _time_to_ns(float(raw_cpu), time_unit) if raw_cpu is not None else None

        results[name] = BenchmarkResult(
            name=name,
            real_time_ns=real_ns,
            cpu_time_ns=cpu_ns,
            iterations=bm.get("iterations"),
            items_per_second=bm.get("items_per_second"),
            time_unit=time_unit,
        )
    return results


def _load_all_results(results_dir: Path) -> dict[str, BenchmarkResult]:
    """Merge all benchmark JSON files in results_dir into a single name→result map."""
    combined: dict[str, BenchmarkResult] = {}
    for json_file in results_dir.rglob("*.json"):
        combined.update(_load_benchmark_json(json_file))
    return combined


# ---------------------------------------------------------------------------
# Evaluation
# ---------------------------------------------------------------------------

_OP_FUNCS = {
    "<=": lambda a, b: a <= b,
    ">=": lambda a, b: a >= b,
    "<":  lambda a, b: a < b,
    ">":  lambda a, b: a > b,
    "==": lambda a, b: abs(a - b) < 1e-9,
}

# Tolerance before flipping PASS→WARN (5 % below threshold still PASS, etc.)
_WARN_MARGIN = 0.05


def _evaluate_expectation(
    exp: Expectation,
    all_results: dict[str, BenchmarkResult],
) -> TargetVerdict:
    verdict = TargetVerdict(
        module=exp.module,
        target_id=exp.target_id,
        expectation_text=exp.expectation_text,
        benchmark_cases=exp.benchmark_cases,
    )

    # Match benchmark cases (exact name or prefix match for parametric cases)
    matched: list[BenchmarkResult] = []
    for case_name in exp.benchmark_cases:
        if case_name in all_results:
            matched.append(all_results[case_name])
        else:
            # Prefix match for parametric benchmarks like BM_Foo/100
            prefix_matches = [r for n, r in all_results.items()
                              if n == case_name or n.startswith(case_name + "/")]
            matched.extend(prefix_matches)

    verdict.matched_results = matched

    if not matched:
        verdict.status = "NOT_RUN"
        verdict.detail = f"No results found for any of: {', '.join(exp.benchmark_cases)}"
        return verdict

    # If no parseable numeric constraint → UNCHECKED (run confirmed but no threshold)
    if exp.constraint_op is None or exp.constraint_value is None:
        verdict.status = "UNCHECKED"
        verdict.detail = (
            f"Benchmark(s) executed ({len(matched)} case(s)); "
            "no numeric threshold extracted from expectation text."
        )
        return verdict

    # Evaluate each matched result
    failures: list[str] = []
    warnings: list[str] = []

    for r in matched:
        # Determine measured value in the same unit as the constraint
        measured: Optional[float] = None
        unit = exp.constraint_unit or ""

        if unit in ("us", "µs", "ms", "s", ""):
            # Time-based constraint — use real_time
            ns = r.real_time_ns
            if ns is not None:
                if unit in ("us", "µs"):
                    measured = ns / 1_000.0
                elif unit == "ms":
                    measured = ns / 1_000_000.0
                elif unit == "s":
                    measured = ns / 1_000_000_000.0
                else:
                    # No unit — assume the constraint value is in the same unit
                    # as the benchmark report (ns). Use real_time_ns directly.
                    measured = ns

        elif unit == "%":
            # Percentage overhead requires a baseline for comparison.
            # Without baseline data we cannot evaluate; mark as UNCHECKED.
            warnings.append(
                f"{r.name}: percentage overhead constraint requires baseline "
                "data — cannot evaluate without a reference measurement."
            )
            continue

        elif unit in ("ops/s", "k/s"):
            # Throughput constraint
            if r.items_per_second is not None:
                measured = r.items_per_second
                if unit == "k/s":
                    measured = measured / 1_000.0
            elif r.real_time_ns and r.iterations:
                # Derive ops/s from time and iterations
                measured = r.iterations / (r.real_time_ns * 1e-9)
                if unit == "k/s":
                    measured = measured / 1_000.0

        if measured is None:
            warnings.append(
                f"{r.name}: measured value unavailable for unit '{unit}'"
            )
            continue

        op_fn = _OP_FUNCS.get(exp.constraint_op)
        if op_fn is None:
            warnings.append(f"{r.name}: unknown operator '{exp.constraint_op}'")
            continue

        passed = op_fn(measured, exp.constraint_value)
        if not passed:
            # Check if within warning margin of the threshold.
            # Upper-bound constraints (<=, <): warn when measured ≤ threshold * (1 + margin).
            # Lower-bound constraints (>=, >): warn when measured ≥ threshold * (1 - margin).
            near_op = {
                "<=": lambda a, b: a <= b * (1 + _WARN_MARGIN),
                "<":  lambda a, b: a < b * (1 + _WARN_MARGIN),
                ">=": lambda a, b: a >= b * (1 - _WARN_MARGIN),
                ">":  lambda a, b: a > b * (1 - _WARN_MARGIN),
            }.get(exp.constraint_op)
            in_margin = near_op is not None and near_op(measured, exp.constraint_value)
            msg = (
                f"{r.name}: measured={measured:.3f}{unit} "
                f"(expected {exp.constraint_op} {exp.constraint_value}{unit})"
            )
            if in_margin:
                warnings.append(msg)
            else:
                failures.append(msg)

    if failures:
        verdict.status = "FAIL"
        verdict.detail = "; ".join(failures)
        if warnings:
            verdict.detail += " | WARN: " + "; ".join(warnings)
    elif warnings:
        verdict.status = "WARN"
        verdict.detail = "; ".join(warnings)
    else:
        verdict.status = "PASS"
        verdict.detail = f"{len(matched)} case(s) checked — all within threshold."

    return verdict


# ---------------------------------------------------------------------------
# Report generation
# ---------------------------------------------------------------------------

_STATUS_ICON = {
    "PASS":      "✅",
    "WARN":      "⚠️",
    "FAIL":      "❌",
    "NOT_RUN":   "⬜",
    "UNCHECKED": "🔵",
}


def _build_report(
    verdicts: list[TargetVerdict],
    run_id: str,
    sha: str,
    repo: str,
) -> str:
    lines: list[str] = []

    total = len(verdicts)
    counts: dict[str, int] = {}
    for v in verdicts:
        counts[v.status] = counts.get(v.status, 0) + 1

    lines.append("<!-- ci-benchmark:type:perf-results -->")
    lines.append("## 📊 ThemisDB Benchmark Performance Gate")
    lines.append("")
    lines.append(
        f"**Run:** [{run_id}](https://github.com/{repo}/actions/runs/{run_id})  "
        f"**SHA:** `{sha}`"
    )
    lines.append("")
    lines.append("### Summary")
    lines.append("")
    lines.append("| Status | Count |")
    lines.append("|--------|-------|")
    for st in ("PASS", "FAIL", "WARN", "NOT_RUN", "UNCHECKED"):
        if counts.get(st, 0):
            lines.append(f"| {_STATUS_ICON[st]} {st} | {counts[st]} |")
    lines.append(f"| **Total targets** | **{total}** |")
    lines.append("")

    # Failures first
    failures = [v for v in verdicts if v.status == "FAIL"]
    if failures:
        lines.append("### ❌ Failing Targets (action required)")
        lines.append("")
        lines.append("| Module | Target ID | Expectation | Detail |")
        lines.append("|--------|-----------|-------------|--------|")
        for v in failures:
            lines.append(
                f"| `{v.module}` | `{v.target_id}` | {v.expectation_text} | {v.detail} |"
            )
        lines.append("")

    # Warnings
    warnings = [v for v in verdicts if v.status == "WARN"]
    if warnings:
        lines.append("### ⚠️ Warning Targets (within margin, monitor)")
        lines.append("")
        lines.append("| Module | Target ID | Expectation | Detail |")
        lines.append("|--------|-----------|-------------|--------|")
        for v in warnings:
            lines.append(
                f"| `{v.module}` | `{v.target_id}` | {v.expectation_text} | {v.detail} |"
            )
        lines.append("")

    # Not-run targets
    not_run = [v for v in verdicts if v.status == "NOT_RUN"]
    if not_run:
        lines.append("### ⬜ Targets Without Results (benchmark not executed / binary missing)")
        lines.append("")
        lines.append("| Module | Target ID | Expected Cases |")
        lines.append("|--------|-----------|----------------|")
        for v in not_run:
            cases = ", ".join(v.benchmark_cases[:5])
            if len(v.benchmark_cases) > 5:
                cases += f", …+{len(v.benchmark_cases) - 5}"
            lines.append(f"| `{v.module}` | `{v.target_id}` | {cases} |")
        lines.append("")

    # Full table
    lines.append("### Full Results per Module")
    lines.append("")

    by_module: dict[str, list[TargetVerdict]] = {}
    for v in verdicts:
        by_module.setdefault(v.module, []).append(v)

    for module in sorted(by_module):
        module_verdicts = by_module[module]
        lines.append(f"<details><summary><b>{module}</b> ({len(module_verdicts)} targets)</summary>")
        lines.append("")
        lines.append("| Target ID | Status | Expectation | Detail |")
        lines.append("|-----------|--------|-------------|--------|")
        for v in module_verdicts:
            icon = _STATUS_ICON.get(v.status, "❓")
            lines.append(
                f"| `{v.target_id}` | {icon} {v.status} | {v.expectation_text} | {v.detail} |"
            )
        lines.append("")
        lines.append("</details>")
        lines.append("")

    # Recommendation section
    lines.append("### 🔍 Recommendation")
    lines.append("")
    if failures:
        lines.append(
            "> **Performance regressions detected.** "
            "The failing targets listed above exceeded their defined thresholds. "
            "Review the benchmark results and determine whether a performance "
            "improvement task must be created before the next release gate."
        )
    elif warnings:
        lines.append(
            "> **Marginal results.** One or more targets are within the 5 % warning margin. "
            "Monitor trend over the next weekly run and act if the pattern persists."
        )
    elif not_run:
        lines.append(
            "> **Incomplete coverage.** Some benchmark targets could not be matched to results. "
            "Verify that all bench_* binaries were built and executed successfully."
        )
    else:
        lines.append(
            "> **All covered targets pass.** "
            "No performance action required at this time."
        )
    lines.append("")

    lines.append("---")
    lines.append(
        "_Generated by `benchmarks/compare_expectations.py` | "
        "Expectations source: `src/*/PERFORMANCE_EXPECTATIONS.md`_"
    )

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Compare benchmark results against PERFORMANCE_EXPECTATIONS.md files."
    )
    parser.add_argument(
        "--src-dir",
        default="src",
        help="Root directory containing src/<module>/ subdirectories.",
    )
    parser.add_argument(
        "--results-dir",
        required=True,
        help="Directory containing Google Benchmark JSON output files.",
    )
    parser.add_argument(
        "--output",
        default="benchmark_comparison_report.md",
        help="Output path for the Markdown report.",
    )
    parser.add_argument(
        "--run-id",
        default=os.environ.get("GITHUB_RUN_ID", "local"),
        help="GitHub Actions run ID (used in report links).",
    )
    parser.add_argument(
        "--sha",
        default=os.environ.get("GITHUB_SHA", "HEAD"),
        help="Git SHA of the measured commit.",
    )
    parser.add_argument(
        "--repo",
        default=os.environ.get("GITHUB_REPOSITORY", "makr-code/ThemisDB"),
        help="GitHub repository slug (owner/repo).",
    )
    parser.add_argument(
        "--regression-threshold",
        type=float,
        default=0.20,
        help="Regression tolerance for unconstrained baselines (default: 0.20 = 20%%).",
    )
    args = parser.parse_args()

    src_dir = Path(args.src_dir)
    results_dir = Path(args.results_dir)

    if not src_dir.is_dir():
        print(f"[ERROR] src-dir not found: {src_dir}", file=sys.stderr)
        return 1
    if not results_dir.is_dir():
        print(f"[ERROR] results-dir not found: {results_dir}", file=sys.stderr)
        return 1

    print(f"[INFO] Discovering PERFORMANCE_EXPECTATIONS.md in {src_dir} …", file=sys.stderr)
    expectations = _discover_expectations(src_dir)
    print(f"[INFO] Found {len(expectations)} expectation targets.", file=sys.stderr)

    print(f"[INFO] Loading benchmark results from {results_dir} …", file=sys.stderr)
    all_results = _load_all_results(results_dir)
    print(f"[INFO] Loaded {len(all_results)} benchmark result entries.", file=sys.stderr)

    verdicts: list[TargetVerdict] = []
    for exp in expectations:
        verdicts.append(_evaluate_expectation(exp, all_results))

    fail_count = sum(1 for v in verdicts if v.status == "FAIL")
    warn_count = sum(1 for v in verdicts if v.status == "WARN")
    not_run_count = sum(1 for v in verdicts if v.status == "NOT_RUN")
    print(
        f"[INFO] Results: PASS={sum(1 for v in verdicts if v.status == 'PASS')}, "
        f"FAIL={fail_count}, WARN={warn_count}, NOT_RUN={not_run_count}, "
        f"UNCHECKED={sum(1 for v in verdicts if v.status == 'UNCHECKED')}",
        file=sys.stderr,
    )

    report = _build_report(verdicts, args.run_id, args.sha, args.repo)
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(report, encoding="utf-8")
    print(f"[INFO] Report written to {output_path}", file=sys.stderr)

    return 1 if fail_count > 0 else 0


if __name__ == "__main__":
    sys.exit(main())
