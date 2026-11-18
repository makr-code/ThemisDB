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
        --output benchmark_comparison_report.md

Exit codes:
    0 — all covered targets PASS or WARN
    1 — at least one covered target FAIL
    2 — incomplete coverage when --strict-completeness is enabled
"""

from __future__ import annotations

import argparse
import json
import math
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
    constraint_unit: Optional[str] = None # "us", "ms", "s", "ops/s", "k/s", "mops/s", "%"


@dataclass
class BenchmarkResult:
    """One benchmark case from a Google Benchmark JSON result file."""
    name: str
    real_time_ns: Optional[float] = None   # real_time in nanoseconds
    cpu_time_ns: Optional[float] = None
    iterations: Optional[int] = None
    items_per_second: Optional[float] = None
    time_unit: str = "ns"
    percentile_time_ns: dict[str, float] = field(default_factory=dict)
    ambiguous_percentiles: set[str] = field(default_factory=set)
    repeated_sample: bool = False
    invalid_measurement: bool = False


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
# Examples: "<= 100 us", "> 6 k/s", "> 50 M ops/s", ">= 1 000 ops/sec", "<= 10 ms",
#           "<= 10%", "< 5 %", ">= 1000", "<= 1 µs"
_CONSTRAINT_RE = re.compile(
    r"(?<![<>=!])(?:<=|>=|==|≤|≥|<|>)(?![=<>])\s*"
    r"([+-]?(?:(?:\d{1,3}(?:[ ,]\d{3})+|\d+)(?:\.\d+)?|\.\d+)"
    r"(?:[eE][+-]?\d+)?)(?![\d.,])"
    r"(?:\s*(M\s*ops/(?:sec|s)\b|[a-zA-Zµμ%×]+(?:/[a-zA-Z]+)?))?",
    re.IGNORECASE,
)


def _parse_number(text: str) -> float:
    """Remove thousand-separator spaces/commas and parse as float."""
    cleaned = text.replace(",", "").replace(" ", "")
    return float(cleaned)


def _parse_constraint(text: str) -> tuple[Optional[str], Optional[float], Optional[str]]:
    """Extract (operator, value, unit) from expectation text, or (None,None,None)."""
    m = _CONSTRAINT_RE.search(text)
    if not m:
        return None, None, None
    op = re.match(r"<=|>=|==|≤|≥|<|>", m.group()).group()
    op = {"≤": "<=", "≥": ">="}.get(op, op)
    val = _parse_number(m.group(1))
    unit = (m.group(2) or "").lower().replace(" ", "").replace("ops/sec", "ops/s")
    unit = unit.replace("μ", "µ")
    if unit == "mops/s":
        val *= 1_000_000.0
        unit = "ops/s"
    return op, val, unit or None


def _split_benchmark_cases(raw: str) -> list[str]:
    """Split a comma/space-separated list of benchmark case names.

    Handles both plain identifiers (``BM_Foo_Bar``, ``Fixture/CaseName``) and
    Google Benchmark macro invocations such as ``BENCHMARK_F(Fixture, Case)``
    or ``BENCHMARK(FuncName)``.  The latter are normalised to ``Fixture/Case``
    or ``FuncName`` respectively so that they match Google Benchmark JSON
    output names.  Trailing parametric annotations (e.g. ``/100``, ``/BM_*``)
    are preserved as part of the identifier.
    """
    # Normalise BENCHMARK_F(Fixture, Case) → Fixture/Case
    def _normalise_macro(token: str) -> str:
        # BENCHMARK_F(Fixture, CaseName), BENCHMARK_DEFINE_F(...), or REGISTER_F(...)
        m = re.match(
            r"BENCHMARK(?:_DEFINE|_REGISTER)?_F\(\s*(\w+)\s*,\s*(\w+)\s*\)",
            token,
        )
        if m:
            return f"{m.group(1)}/{m.group(2)}"
        # BENCHMARK(FuncName)
        m = re.match(r"BENCHMARK\(\s*(\w+)\s*\)", token)
        if m:
            return m.group(1)
        return token

    parts: list[str] = []
    start = 0
    depth = 0
    for index, character in enumerate(raw):
        if character == "(":
            depth += 1
        elif character == ")" and depth > 0:
            depth -= 1
        elif character in ",\n" and depth == 0:
            part = raw[start:index].strip().strip("`")
            if part:
                parts.append(part)
            start = index + 1

    trailing = raw[start:].strip().strip("`")
    if trailing:
        parts.append(trailing)

    result: list[str] = []
    for p in parts:
        # Normalise macro notation before any further splitting
        p = _normalise_macro(p.strip())
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
    # Column index map: populated from the header row
    col_target_id: int = 0
    col_expectation: Optional[int] = None
    col_benchmark: int = 2
    threshold_cols: list[tuple[int, str]] = []

    for line in lines:
        # Detect the "Specific Expectations" section (English and German variants)
        if re.search(
            r"##\s*(?:Specific\s+Expectations|Spezifische\s+Erwartungswerte)",
            line, re.IGNORECASE
        ):
            in_specific_table = True
            header_seen = False
            col_target_id, col_expectation, col_benchmark = 0, None, 2
            threshold_cols = []
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

        # First content row after separator is the header — detect column positions
        if not header_seen:
            header_seen = True
            # Map header names to indices (case-insensitive, flexible naming)
            _id_keywords = {"target id", "ziel-id", "gate id", "target-id", "ziel id", "id"}
            _exp_keywords = {"expectation", "erwartung", "erwartungswert"}
            _bench_keywords = {"benchmark", "bench", "case", "test case"}
            for idx, header in enumerate(cols):
                h = header.lower().strip()
                if h in _id_keywords:
                    col_target_id = idx
                elif any(k in h for k in _bench_keywords):
                    col_benchmark = idx
                elif any(k in h for k in _exp_keywords):
                    col_expectation = idx
                elif any(k in h for k in ("threshold", "target", "schwellenwert",
                                           "measurement", "metric", "messwert")):
                    threshold_cols.append((idx, header))
            continue

        # We need at least enough columns to satisfy all three mapped indices
        needed = max([col_target_id, col_benchmark]
                     + ([col_expectation] if col_expectation is not None else [])
                     + [idx for idx, _ in threshold_cols]) + 1
        if len(cols) < needed:
            continue

        target_id = cols[col_target_id]
        expectation_text = cols[col_expectation] if col_expectation is not None else ""
        for idx, header in threshold_cols:
            if idx != col_expectation and cols[idx]:
                expectation_text += f"{'; ' if expectation_text else ''}{header}: {cols[idx]}"
        cases_raw = cols[col_benchmark]

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

def _merge_result(results: dict[str, BenchmarkResult], result: BenchmarkResult) -> None:
    """Keep the most sampled regular result, but never silently choose percentiles."""
    existing = results.get(result.name)
    if existing is None:
        results[result.name] = result
        return
    selected = result if (result.iterations or 0) > (existing.iterations or 0) else existing
    selected.ambiguous_percentiles = (
        existing.ambiguous_percentiles | result.ambiguous_percentiles
        | set(existing.percentile_time_ns) | set(result.percentile_time_ns)
    )
    selected.repeated_sample = existing.repeated_sample or result.repeated_sample
    selected.invalid_measurement = existing.invalid_measurement or result.invalid_measurement
    results[result.name] = selected


def _time_to_ns(value: float, unit: str) -> Optional[float]:
    """Convert benchmark time value to nanoseconds."""
    unit = unit.lower()
    factors = {"ns": 1.0, "us": 1e3, "µs": 1e3, "ms": 1e6, "s": 1e9}
    factor = factors.get(unit.replace("μ", "µ"))
    return value * factor if factor is not None else None


def _load_benchmark_json(
    path: Path, diagnostics: Optional[list[str]] = None,
    expected_cases: Optional[list[str]] = None,
) -> dict[str, BenchmarkResult]:
    """Load a Google Benchmark JSON file, return {case_name: BenchmarkResult}."""
    def diagnose(message: str) -> None:
        detail = f"{path}: {message}"
        if diagnostics is not None:
            diagnostics.append(detail)
        print(f"[WARN] {detail}", file=sys.stderr)

    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError, UnicodeError) as exc:
        diagnose(f"Could not parse JSON: {exc}")
        return {}

    if not isinstance(data, dict) or not isinstance(data.get("benchmarks"), list):
        diagnose("Invalid Google Benchmark JSON: expected a benchmarks array")
        return {}
    if not data["benchmarks"]:
        diagnose("Empty benchmarks array")
        return {}
    if expected_cases is not None:
        names = [bm.get("name") for bm in data["benchmarks"] if isinstance(bm, dict)]
        if sorted(names, key=str) != sorted(expected_cases):
            diagnose("Result cases do not match the finished manifest; file discarded")
            return {}
    errors = [bm for bm in data["benchmarks"]
              if isinstance(bm, dict) and bm.get("error_occurred")]
    if data.get("error_occurred") or data.get("error") or errors:
        for bm in errors:
            diagnose(f"{bm.get('name', '<unnamed>')}: benchmark error: "
                     f"{bm.get('error_message', 'unspecified error')}")
        diagnose("Failed benchmark run; all results from this file discarded")
        return {}

    def number(value: object) -> Optional[float]:
        if isinstance(value, bool) or not isinstance(value, (int, float)):
            return None
        try:
            value = float(value)
        except OverflowError:
            return None
        return value if math.isfinite(value) and value >= 0 else None

    results: dict[str, BenchmarkResult] = {}
    for bm in data.get("benchmarks", []):
        if not isinstance(bm, dict) or not isinstance(bm.get("name"), str) or not bm["name"]:
            diagnose("Invalid benchmark entry: missing case name")
            continue
        if str(bm.get("label", "")).strip().casefold() == "synthetic":
            continue
        if bm.get("error_occurred", False):
            diagnose(f"{bm['name']}: benchmark error: {bm.get('error_message', 'unspecified error')}")
            continue
        if bm.get("run_type") == "aggregate":
            continue
        name: str = bm.get("name", "")
        time_unit = bm.get("time_unit")
        valid_unit = isinstance(time_unit, str) and _time_to_ns(1, time_unit) is not None
        if not valid_unit:
            diagnose(f"{name}: unsupported time unit {time_unit!r}")
            time_unit = ""
        raw_real = number(bm.get("real_time"))
        raw_cpu = number(bm.get("cpu_time"))
        invalid_numeric = False
        for key in ("real_time", "cpu_time", "items_per_second",
                    "p95_real_time", "p99_real_time", "iterations"):
            if key in bm and number(bm[key]) is None:
                diagnose(f"{name}: invalid {key} value")
                invalid_numeric = True
        if bm.get("iterations") == 0:
            diagnose(f"{name}: no benchmark iterations")
            invalid_numeric = True
        if not valid_unit or invalid_numeric:
            _merge_result(results, BenchmarkResult(
                name=name, time_unit=time_unit, invalid_measurement=True))
            continue
        real_ns = number(_time_to_ns(raw_real, time_unit)) if raw_real is not None else None
        cpu_ns = number(_time_to_ns(raw_cpu, time_unit)) if raw_cpu is not None else None
        if (raw_real is not None and real_ns is None) or (raw_cpu is not None and cpu_ns is None):
            diagnose(f"{name}: non-finite converted time")
            _merge_result(results, BenchmarkResult(
                name=name, time_unit=time_unit, invalid_measurement=True))
            continue
        percentiles = {}
        for percentile in ("p95", "p99"):
            raw = number(bm.get(f"{percentile}_real_time"))
            converted = number(_time_to_ns(raw, time_unit)) if raw is not None else None
            if converted is not None:
                percentiles[percentile] = converted
            elif raw is not None:
                diagnose(f"{name}: non-finite converted {percentile} time")
                invalid_numeric = True
        if invalid_numeric:
            _merge_result(results, BenchmarkResult(
                name=name, time_unit=time_unit, invalid_measurement=True))
            continue
        iterations = number(bm.get("iterations"))
        result = BenchmarkResult(
            name=name,
            real_time_ns=real_ns,
            cpu_time_ns=cpu_ns,
            iterations=int(iterations) if iterations is not None else None,
            items_per_second=number(bm.get("items_per_second")),
            time_unit=time_unit,
            percentile_time_ns=percentiles,
            repeated_sample=("repetition_index" in bm or
                             (number(bm.get("repetitions")) or 0) > 1),
        )
        if real_ns is None and cpu_ns is None and result.items_per_second is None and not percentiles:
            diagnose(f"{name}: no usable measurements")
        _merge_result(results, result)
    return results


def _load_all_results(
    results_dir: Path, diagnostics: Optional[list[str]] = None,
    manifest_info: Optional[dict] = None,
) -> dict[str, BenchmarkResult]:
    """Load finished manifest-approved files, or all JSON files in standalone mode.

    Optional diagnostics and manifest_info are populated for report integration.
    A present but invalid or unfinished manifest fails closed.
    """
    combined: dict[str, BenchmarkResult] = {}
    manifest_path = results_dir / "manifest"
    approved: Optional[dict[Path, list[str]]] = None
    if manifest_path.exists():
        approved = {}
        try:
            data = json.loads(manifest_path.read_text(encoding="utf-8"))
            if not isinstance(data, dict) or not isinstance(data.get("tests"), list):
                raise ValueError("expected an object with a tests array")
            if manifest_info is not None:
                manifest_info.update(data)
            if data.get("status") != "finished":
                raise ValueError("manifest is not finished")
            rejected: set[Path] = set()
            for test in data["tests"]:
                if not isinstance(test, dict):
                    raise ValueError("invalid test entry")
                raw_path = test.get("result")
                if (not isinstance(raw_path, str) or not raw_path
                        or Path(raw_path).is_absolute() or ".." in Path(raw_path).parts):
                    if diagnostics is not None:
                        diagnostics.append(f"Manifest test {test.get('test', '?')}: invalid result path")
                    continue
                path = (results_dir / raw_path).resolve()
                if not path.is_relative_to(results_dir.resolve()) or path.suffix != ".json":
                    if diagnostics is not None:
                        diagnostics.append(f"Manifest result outside results directory or not JSON: {raw_path}")
                    continue
                cases = test.get("cases")
                usable = (test.get("execution_status") == "completed"
                          and test.get("json_status") == "valid"
                          and isinstance(cases, list)
                          and all(isinstance(case, dict)
                                  and isinstance(case.get("name"), str)
                                  and case["name"]
                                  and case.get("status") == "observed" for case in cases))
                if not usable or path in approved:
                    rejected.add(path)
                    approved.pop(path, None)
                    if diagnostics is not None:
                        diagnostics.append(
                            f"Manifest test {test.get('test', '?')}: unusable or duplicate result "
                            f"(execution={test.get('execution_status')}, JSON={test.get('json_status')})"
                        )
                elif path not in rejected:
                    approved[path] = [case["name"] for case in cases]
        except (OSError, UnicodeError, json.JSONDecodeError, ValueError, RuntimeError) as exc:
            approved = {}
            if diagnostics is not None:
                diagnostics.append(f"{manifest_path}: unusable manifest: {exc}")

        for path, cases in sorted(approved.items()):
            if not path.is_file() and diagnostics is not None:
                diagnostics.append(f"{path}: manifest-approved result file missing")

    json_files = sorted(results_dir.rglob("*.json"))
    if not json_files and diagnostics is not None:
        diagnostics.append(f"{results_dir}: no JSON result files found")
    for json_file in json_files:
        resolved = json_file.resolve()
        if approved is not None and resolved not in approved:
            continue
        expected_cases = approved[resolved] if approved is not None else None
        for name, result in _load_benchmark_json(json_file, diagnostics, expected_cases).items():
            _merge_result(combined, result)
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
    seen_names: set[str] = set()
    matched: list[BenchmarkResult] = []
    missing: list[str] = []
    for case_name in exp.benchmark_cases:
        case_results = ([all_results[case_name]] if case_name in all_results else
                        [r for n, r in all_results.items()
                         if n.startswith(case_name + "/")])
        if not case_results:
            missing.append(case_name)
        for r in case_results:
            if r.name not in seen_names:
                seen_names.add(r.name)
                matched.append(r)

    verdict.matched_results = matched

    if not matched:
        verdict.status = "NOT_RUN"
        verdict.detail = f"No results found for any of: {', '.join(exp.benchmark_cases)}"
        return verdict

    missing_detail = f"Missing required cases: {', '.join(missing)}" if missing else ""
    text = exp.expectation_text
    percentiles = {p.lower().replace(" ", "") for p in re.findall(
        r"\bp\s*\d+(?:[._]\d+)?(?:st|nd|rd|th)?\b", text, re.IGNORECASE
    )}
    percentile = next(iter(percentiles)) if len(percentiles) == 1 else None
    reason = ""
    if re.search(r"\b(?:baseline|relative|versus|vs\.?|compared|regression|speedup)\b", text, re.IGNORECASE):
        reason = "Baseline-relative expectation requires reference measurements."
    elif re.search(r"!=|~=|=<|=>|≠|≈|≃", text):
        reason = "Unsupported comparison operator."
    elif (len(list(_CONSTRAINT_RE.finditer(text))) != 1
          or len(re.findall(r"<=|>=|==|≤|≥|<|>", text)) != 1):
        reason = "Expected exactly one supported numeric threshold."
    elif exp.constraint_op is None or exp.constraint_value is None:
        reason = "No numeric threshold extracted from expectation text."
    elif not math.isfinite(exp.constraint_value):
        reason = "Threshold is not finite."
    elif (len(percentiles) > 1 or any(p not in ("p95", "p99") for p in percentiles)
          or re.search(r"\b(?:\d+(?:\.\d+)?(?:st|nd|rd|th)[ -]+percentile|"
                       r"percentile\s*[-:]?\s*\d+(?:\.\d+)?)\b", text, re.IGNORECASE)
          or (not percentiles and re.search(r"\bpercentile\b", text, re.IGNORECASE))):
        reason = "Unsupported or ambiguous percentile; only p95 and p99 are supported."
    elif exp.constraint_unit not in ("ns", "us", "µs", "ms", "s", "ops/s", "k/s", "mops/s"):
        reason = f"Unsupported or unspecified constraint unit: {exp.constraint_unit!r}."
    elif percentile and exp.constraint_unit not in ("ns", "us", "µs", "ms", "s"):
        reason = f"{percentile} requires a percentile time measurement."
    elif exp.constraint_unit in ("ops/s", "k/s", "mops/s") and not re.search(
        r"\bitems_per_second\s*=\s*ops/s\b", text, re.IGNORECASE
    ):
        reason = "Throughput requires explicit mapping: items_per_second = ops/s."

    if reason:
        verdict.status = "UNCHECKED"
        verdict.detail = "; ".join(filter(None, (reason, missing_detail)))
        return verdict

    # Evaluate each matched result
    failures: list[str] = []
    warnings: list[str] = []
    unavailable: list[str] = [missing_detail] if missing_detail else []

    for r in matched:
        # Determine measured value in the same unit as the constraint
        measured: Optional[float] = None
        unit = exp.constraint_unit or ""

        if r.invalid_measurement:
            unavailable.append(f"{r.name}: invalid measurement or time unit")
            continue
        if r.repeated_sample or (percentile and percentile in r.ambiguous_percentiles):
            unavailable.append(f"{r.name}: ambiguous repeated or duplicate percentile measurements")
            continue

        if unit in ("ns", "us", "µs", "ms", "s"):
            ns = r.percentile_time_ns.get(percentile) if percentile else r.real_time_ns
            if ns is not None:
                if unit in ("us", "µs"):
                    measured = ns / 1_000.0
                elif unit == "ms":
                    measured = ns / 1_000_000.0
                elif unit == "s":
                    measured = ns / 1_000_000_000.0
                else:
                    measured = ns

        elif unit in ("ops/s", "k/s", "mops/s"):
            # Throughput constraint
            if r.items_per_second is not None:
                measured = r.items_per_second
                if unit == "k/s":
                    measured = measured / 1_000.0
                elif unit == "mops/s":
                    measured = measured / 1_000_000.0
        if measured is None or not math.isfinite(measured) or measured < 0:
            unavailable.append(
                f"{r.name}: {percentile or 'measured value'} unavailable for unit '{unit}'"
            )
            continue

        op_fn = _OP_FUNCS.get(exp.constraint_op)
        if op_fn is None:
            unavailable.append(f"{r.name}: unknown operator '{exp.constraint_op}'")
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
        if unavailable:
            verdict.detail += " | UNCHECKED: " + "; ".join(unavailable)
    elif unavailable:
        verdict.status = "UNCHECKED"
        verdict.detail = "; ".join(unavailable + warnings)
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
    diagnostics: Optional[list[str]] = None,
    manifest_info: Optional[dict] = None,
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
    lines.append(
        "Scope: only tables under `Specific Expectations` / "
        "`Spezifische Erwartungswerte` are discovered; other tables are not evaluated."
    )
    lines.append("")
    lines.append(
        "Measurements: time thresholds use Google Benchmark `real_time`; throughput "
        "requires `items_per_second` and an explicit expectation mapping "
        "`items_per_second = ops/s` (one item must mean one target operation). "
        "Percentiles require explicit `p95_real_time`/`p99_real_time` "
        "time fields in the same `time_unit`; aggregate rows and iteration counts "
        "do not establish percentiles or throughput. Duplicate percentile records "
        "and individual repetition samples are not evaluated."
    )
    lines.append("")
    if diagnostics:
        lines.append("### ⚠️ Result Loading Diagnostics")
        lines.append("")
        for diagnostic in diagnostics:
            escaped = diagnostic.replace("\n", " ").replace("|", "\\|")
            lines.append(f"- {escaped}")
        lines.append("")

    def safe(value: object) -> str:
        return str(value if value is not None else "unknown").replace("\n", " ").replace("|", "\\|")

    if manifest_info:
        lines.append("### Manifest Test and Case Outcomes")
        lines.append("")
        lines.append(f"Manifest status: **{safe(manifest_info.get('status'))}**.")
        lines.append("")
        lines.append("| Test | Execution | JSON | Case | Case status | Error |")
        lines.append("|---|---|---|---|---|---|")
        tests = manifest_info.get("tests")
        for test in tests if isinstance(tests, list) else []:
            if not isinstance(test, dict):
                continue
            cases = test.get("cases")
            for case in cases if isinstance(cases, list) and cases else [{}]:
                if not isinstance(case, dict):
                    continue
                lines.append(
                    f"| {safe(test.get('test', test.get('id')))} "
                    f"| {safe(test.get('execution_status'))} | {safe(test.get('json_status'))} "
                    f"| {safe(case.get('name'))} | {safe(case.get('status'))} "
                    f"| {safe(case.get('error_message'))} |"
                )
        lines.append("")
        lines.append("#### Manifest expectation mappings (recorded observations)")
        lines.append("")
        lines.append("| Module | Target ID | Expected case | Mapping | Observations |")
        lines.append("|---|---|---|---|---|")
        mappings = manifest_info.get("expectations")
        for mapping in mappings if isinstance(mappings, list) else []:
            if not isinstance(mapping, dict):
                continue
            observations = mapping.get("observations")
            summaries = []
            for observation in observations if isinstance(observations, list) else []:
                if isinstance(observation, dict):
                    summaries.append(
                        f"{safe(observation.get('test'))}: {safe(observation.get('case_status'))}"
                    )
            lines.append(
                f"| {safe(mapping.get('module'))} | {safe(mapping.get('target_id'))} "
                f"| {safe(mapping.get('case'))} | {safe(mapping.get('mapping_status'))} "
                f"| {', '.join(summaries) or 'none'} |"
            )
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
        lines.append("### ⬜ Targets Without Results (no matching usable result)")
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
    elif not total or counts.get("UNCHECKED", 0) or diagnostics:
        lines.append(
            "> **Incomplete verification.** Some targets or result files could not be "
            "evaluated; review the details and loading diagnostics."
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
        "--strict-completeness",
        action="store_true",
        help="Exit 2 on incomplete coverage or result-loading diagnostics (FAIL remains 1).",
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
    diagnostics: list[str] = []
    manifest_info: dict = {}
    all_results = _load_all_results(results_dir, diagnostics, manifest_info)
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

    report = _build_report(verdicts, args.run_id, args.sha, args.repo, diagnostics, manifest_info)
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(report, encoding="utf-8")
    print(f"[INFO] Report written to {output_path}", file=sys.stderr)

    if fail_count:
        return 1
    if args.strict_completeness and (
        not expectations or not all_results or diagnostics
        or any(v.status in ("NOT_RUN", "UNCHECKED") for v in verdicts)
    ):
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
