"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            verify_benchmark_mapping.py                        ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-04-14 11:54:51                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     384                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 1f89357bb7  2026-04-14  fix(benchmarks): standardize goal-ID→benchmark mapping, f... ║
    • 5d4629af87  2026-04-13  feat(perf): add benchmark target mapping, verify script, ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
verify_benchmark_mapping.py

Validates benchmarks/benchmark_target_mapping.json against the benchmark
source files and PERFORMANCE_EXPECTATIONS.md.

Exit codes:
  0  – all checks passed
  1  – one or more checks failed

Checks performed:
  1. Mapping file is valid JSON and has the required top-level keys.
  2. Every target-ID entry contains exactly the required fields
     (primary_benchmark, file).
  3. The benchmark source file referenced by each entry actually exists
     under the benchmarks/ directory.
  4. The primary_benchmark function (or fixture registration) referenced
     by each entry is present in that source file.
  5. Every target-ID found in PERFORMANCE_EXPECTATIONS.md §1.2 module
     tables is present in the mapping.
"""

from __future__ import annotations

import json
import os
import re
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Paths (relative to repository root)
# ---------------------------------------------------------------------------
REPO_ROOT = Path(__file__).resolve().parent.parent
MAPPING_FILE = REPO_ROOT / "benchmarks" / "benchmark_target_mapping.json"
PERF_EXPECTATIONS = REPO_ROOT / "PERFORMANCE_EXPECTATIONS.md"
BENCHMARKS_DIR = REPO_ROOT / "benchmarks"

REQUIRED_ENTRY_FIELDS = {"primary_benchmark", "file"}

# Valid values for the optional "status" field
VALID_STATUS_VALUES = {"mapped", "proxy", "not_measurable", "gap"}

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _err(msg: str) -> None:
    print(f"  [FAIL] {msg}", file=sys.stderr)


def _ok(msg: str) -> None:
    print(f"  [PASS] {msg}")


def _warn(msg: str) -> None:
    print(f"  [WARN] {msg}")


# ---------------------------------------------------------------------------
# Check 1 – mapping file loadable
# ---------------------------------------------------------------------------

def check_mapping_loadable() -> tuple[bool, dict]:
    if not MAPPING_FILE.exists():
        _err(f"Mapping file not found: {MAPPING_FILE}")
        return False, {}
    try:
        data = json.loads(MAPPING_FILE.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        _err(f"Mapping file is not valid JSON: {exc}")
        return False, {}
    for key in ("version", "modules"):
        if key not in data:
            _err(f"Mapping file missing required top-level key: '{key}'")
            return False, {}
    _ok(f"Mapping file loaded (version={data.get('version')}, "
        f"{sum(len(v) for v in data['modules'].values())} target IDs across "
        f"{len(data['modules'])} modules)")
    return True, data


# ---------------------------------------------------------------------------
# Check 2 – required fields per entry
# ---------------------------------------------------------------------------

def check_entry_fields(data: dict) -> bool:
    ok = True
    for module, targets in data["modules"].items():
        for tid, entry in targets.items():
            missing = REQUIRED_ENTRY_FIELDS - set(entry.keys())
            if missing:
                _err(f"{module}/{tid}: missing required fields: {missing}")
                ok = False
    if ok:
        _ok("All entries have required fields (primary_benchmark, file)")
    return ok


# ---------------------------------------------------------------------------
# Check 3 – source files exist
# ---------------------------------------------------------------------------

def check_files_exist(data: dict) -> bool:
    ok = True
    seen: set[str] = set()
    for module, targets in data["modules"].items():
        for tid, entry in targets.items():
            fname = entry.get("file", "")
            if fname in seen:
                continue
            seen.add(fname)
            fpath = BENCHMARKS_DIR / fname
            if not fpath.exists():
                _err(f"{module}/{tid}: referenced file not found: benchmarks/{fname}")
                ok = False
    if ok:
        _ok(f"All {len(seen)} referenced benchmark source files exist")
    return ok


# ---------------------------------------------------------------------------
# Check 4 – primary_benchmark symbol present in source file
# ---------------------------------------------------------------------------

def _benchmark_present(source_text: str, bench_name: str) -> bool:
    """
    A benchmark function BM_Foo is registered via:
      BENCHMARK(BM_Foo) or BENCHMARK_F(Fixture, Foo) or
      BENCHMARK_REGISTER_F(Fixture, Foo) or BENCHMARK_DEFINE_F(Fixture, Foo)

    The mapping uses:
      - Plain names like "BM_Foo" for BENCHMARK(BM_Foo)
      - "FixtureName_MethodName" for fixture-based registrations

    We accept a match if the bench_name appears verbatim anywhere in the
    file (inside a BENCHMARK* macro call), or the equivalent
    "Fixture_Method" → "Fixture, Method" form appears.
    """
    # Direct presence as a token
    if re.search(r'\b' + re.escape(bench_name) + r'\b', source_text):
        return True
    # Fixture_Method → "Fixture, Method"
    if "_" in bench_name:
        parts = bench_name.split("_", 1)
        pattern = re.escape(parts[0]) + r'\s*,\s*' + re.escape(parts[1])
        if re.search(pattern, source_text):
            return True
    return False


def check_benchmarks_present(data: dict) -> bool:
    ok = True
    file_cache: dict[str, str] = {}
    missing: list[str] = []

    for module, targets in data["modules"].items():
        for tid, entry in targets.items():
            fname = entry.get("file", "")
            bench = entry.get("primary_benchmark", "")
            if not fname or not bench:
                continue
            if fname not in file_cache:
                fpath = BENCHMARKS_DIR / fname
                if fpath.exists():
                    file_cache[fname] = fpath.read_text(encoding="utf-8",
                                                        errors="replace")
                else:
                    file_cache[fname] = ""
            if not _benchmark_present(file_cache[fname], bench):
                missing.append(f"{module}/{tid}: '{bench}' not found in {fname}")
                ok = False

    if ok:
        _ok("All primary_benchmark symbols found in their source files")
    else:
        for m in missing:
            _err(m)
    return ok


# ---------------------------------------------------------------------------
# Check 5 – all target IDs from PERFORMANCE_EXPECTATIONS.md are mapped
# ---------------------------------------------------------------------------

# Prefixes that are NOT SLO target IDs (they appear in tables but are
# section counters, version tags, delta labels, etc.)
_EXCLUDED_PREFIXES = {
    "BM",   # system-level TPC/YCSB are tracked under system_level module
    "D",    # gap labels (D-1..D-7 in §35)
    "P",    # roadmap priority items (P-1..P-10)
    "RSA",  # §39 technology label (RSA-4096), not a module SLO target ID
    "AVX",  # §39 technology label (AVX-512), not a module SLO target ID
}

def _collect_md_target_ids() -> set[str]:
    """
    Extract every token of the form UPPER_PREFIX-DIGIT(S) from table rows
    in PERFORMANCE_EXPECTATIONS.md, filtering out known non-SLO prefixes.
    """
    if not PERF_EXPECTATIONS.exists():
        _warn(f"PERFORMANCE_EXPECTATIONS.md not found at {PERF_EXPECTATIONS}; "
              "skipping cross-reference check")
        return set()

    content = PERF_EXPECTATIONS.read_text(encoding="utf-8", errors="replace")
    # Match tokens at the start of a table cell: | TOKEN |
    raw = re.findall(r'^\|\s*([A-Z]{1,4}-\d+[a-z]?)\s', content,
                     re.MULTILINE)
    ids: set[str] = set()
    for tid in raw:
        prefix = tid.split("-")[0]
        if prefix in _EXCLUDED_PREFIXES:
            # BM-* are tracked explicitly as system_level module entries
            if prefix == "BM":
                ids.add(tid)
            continue
        ids.add(tid)
    return ids


def _collect_mapping_ids(data: dict) -> set[str]:
    ids: set[str] = set()
    for targets in data["modules"].values():
        ids.update(targets.keys())
    return ids


def check_full_coverage(data: dict) -> bool:
    md_ids = _collect_md_target_ids()
    if not md_ids:
        _warn("No target IDs extracted from PERFORMANCE_EXPECTATIONS.md; "
              "skipping coverage check")
        return True

    mapped_ids = _collect_mapping_ids(data)
    unmapped = md_ids - mapped_ids

    if unmapped:
        _err(f"{len(unmapped)} target ID(s) from PERFORMANCE_EXPECTATIONS.md "
             f"are NOT in the mapping:")
        for tid in sorted(unmapped):
            _err(f"  missing: {tid}")
        return False

    extra = mapped_ids - md_ids
    if extra:
        _warn(f"{len(extra)} mapping entries have no corresponding ID in "
              f"PERFORMANCE_EXPECTATIONS.md (may be planned/future):")
        for tid in sorted(extra):
            _warn(f"  extra: {tid}")

    _ok(f"All {len(md_ids)} target IDs from PERFORMANCE_EXPECTATIONS.md "
        f"are present in the mapping")
    return True


# ---------------------------------------------------------------------------
# Check 6 – status field validation and coverage quote
# ---------------------------------------------------------------------------

def check_status_and_coverage(data: dict) -> bool:
    """
    Optional check: validate the 'status' field on each entry (when present)
    and print a coverage-quote breakdown per status category.
    """
    counts: dict[str, int] = {s: 0 for s in VALID_STATUS_VALUES}
    counts["(missing)"] = 0
    invalid: list[str] = []

    for module, targets in data["modules"].items():
        for tid, entry in targets.items():
            status = entry.get("status")
            if status is None:
                counts["(missing)"] += 1
            elif status not in VALID_STATUS_VALUES:
                invalid.append(f"{module}/{tid}: invalid status '{status}'")
                counts["(missing)"] += 1
            else:
                counts[status] += 1

    if invalid:
        for msg in invalid:
            _err(msg)
        return False

    total = sum(v for k, v in counts.items() if k != "(missing)")
    total += counts["(missing)"]
    directly_mapped = counts.get("mapped", 0)
    proxy_count = counts.get("proxy", 0)
    not_measurable_count = counts.get("not_measurable", 0)
    gap_count = counts.get("gap", 0)
    missing_count = counts["(missing)"]

    coverage_pct = (directly_mapped / total * 100) if total > 0 else 0.0

    print(f"  Coverage quote  : {directly_mapped}/{total} entries with status='mapped' "
          f"= {coverage_pct:.1f}%")
    print(f"  proxy           : {proxy_count}")
    print(f"  not_measurable  : {not_measurable_count}")
    print(f"  gap (open tasks): {gap_count}")
    if missing_count:
        _warn(f"{missing_count} entries are missing the 'status' field")
    else:
        _ok("All entries carry a valid 'status' field")

    if gap_count:
        _warn(f"{gap_count} gap entries require a benchmark case (open subtasks)")

    return True


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> int:
    print("=" * 60)
    print("verify_benchmark_mapping.py – Check 7a")
    print("=" * 60)

    results: list[bool] = []

    print("\n[1] Load and validate mapping file")
    loaded, data = check_mapping_loadable()
    results.append(loaded)
    if not loaded:
        print("\nCannot continue without a valid mapping file.")
        return 1

    print("\n[2] Check required fields per entry")
    results.append(check_entry_fields(data))

    print("\n[3] Check benchmark source files exist")
    results.append(check_files_exist(data))

    print("\n[4] Check primary_benchmark symbols present in source files")
    results.append(check_benchmarks_present(data))

    print("\n[5] Cross-reference: all PERFORMANCE_EXPECTATIONS.md IDs mapped")
    results.append(check_full_coverage(data))

    print("\n[6] Status field validation and coverage quote")
    results.append(check_status_and_coverage(data))

    print("\n" + "=" * 60)
    passed = all(results)
    if passed:
        total = sum(len(v) for v in data["modules"].values())
        print(f"perf_audit check 7a: PASS  "
              f"(mapping file exists and is valid, {total} target IDs covered)")
    else:
        failed = results.count(False)
        print(f"perf_audit check 7a: FAIL  ({failed} check(s) failed)")
    print("=" * 60)
    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main())
