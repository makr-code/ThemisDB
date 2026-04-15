"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            check_disabled_bench_policy.py                     ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-15 04:33:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   83.0/100                                       ║
    • Total Lines:     210                                            ║
    • Open Issues:     TODOs: 0, Stubs: 4                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3224c48c81  2026-04-13  [Governance] Introduce disabled benchmark policy with lin... ║
    • bd21a7cd4b  2026-04-13  [Governance] Introduce disabled benchmark policy with lin... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
check_disabled_bench_policy.py
──────────────────────────────
Lint tool that enforces the Disabled Benchmark Policy defined in
benchmarks/docs/DISABLED_BENCHMARK_POLICY.md.

Every BENCHMARK(BM_*_Disabled) or BENCHMARK_REGISTER_F(…, *_Disabled)
registration in benchmarks/**/*.cpp must have – within 300 characters
of the macro – BOTH:

  • an issue reference  (#NNN or .../issues/NNN)
  • a sunset deadline   (Deadline: vX.Y.Z / Deadline: YYYY-QN / Deadline: YYYY-MM-DD)

Exit codes
──────────
  0  All stubs compliant (or no stubs found).
  1  One or more violations detected.

Usage
─────
  python3 tools/check_disabled_bench_policy.py [--repo-root <path>] [--verbose]
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path
from typing import NamedTuple

# ---------------------------------------------------------------------------
# Patterns
# ---------------------------------------------------------------------------

# Matches BENCHMARK(BM_*_Disabled) or BENCHMARK_REGISTER_F(fixture, *_Disabled)
# where the benchmark name ENDS with "Disabled" (the conventional disabled-stub suffix).
# This deliberately excludes names like "AutoLoadDisabledVsEnabled" that contain
# "Disabled" in the middle — those are real benchmarks testing disabled/enabled states.
_RE_REGISTRATION = re.compile(
    r"BENCHMARK(?:_REGISTER_F)?\s*\(\s*(?:[A-Za-z_]\w*\s*,\s*)?(\w*[Dd]isabled)\s*\)"
)

# Issue reference: #NNN (3+ digits) or a GitHub URL fragment /issues/NNN
_RE_ISSUE = re.compile(
    r"#\d{3,}|/issues/\d+|issue[:\s#]+\d+|ticket[:\s#]+\d+",
    re.IGNORECASE,
)

# Deadline: vX.Y.Z  |  Deadline: YYYY-QN  |  Deadline: YYYY-MM-DD
_RE_DEADLINE = re.compile(
    r"[Dd]eadline\s*:\s*(?:v\d+[\d.]+|\d{4}-Q\d|\d{4}-\d{2}-\d{2})"
)

# Context window (chars) to search on each side of the registration macro
_CONTEXT = 300


# ---------------------------------------------------------------------------
# Data types
# ---------------------------------------------------------------------------

class Violation(NamedTuple):
    path: str
    line: int
    func: str
    missing: list[str]


# ---------------------------------------------------------------------------
# Core checker
# ---------------------------------------------------------------------------

def check_file(src: Path, repo_root: Path) -> list[Violation]:
    violations: list[Violation] = []
    try:
        text = src.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return violations

    for match in _RE_REGISTRATION.finditer(text):
        func_name = match.group(1)
        start = max(0, match.start() - _CONTEXT)
        end = min(len(text), match.end() + _CONTEXT)
        context = text[start:end]

        missing: list[str] = []
        if not _RE_ISSUE.search(context):
            missing.append("issue reference (#NNN)")
        if not _RE_DEADLINE.search(context):
            missing.append("deadline (Deadline: vX.Y.Z)")

        if missing:
            line_no = text[: match.start()].count("\n") + 1
            rel = str(src.relative_to(repo_root))
            violations.append(Violation(rel, line_no, func_name, missing))

    return violations


def run(repo_root: Path, verbose: bool) -> int:
    bench_dir = repo_root / "benchmarks"
    if not bench_dir.is_dir():
        print(f"ERROR: benchmarks directory not found: {bench_dir}", file=sys.stderr)
        return 1

    all_violations: list[Violation] = []
    files_checked = 0
    files_with_disabled = 0

    for cpp in sorted(bench_dir.rglob("*.cpp")):
        text = cpp.read_text(encoding="utf-8", errors="replace") if cpp.exists() else ""
        if not _RE_REGISTRATION.search(text):
            files_checked += 1
            continue
        files_with_disabled += 1
        files_checked += 1
        violations = check_file(cpp, repo_root)
        all_violations.extend(violations)
        if verbose and not violations:
            print(f"  OK   {cpp.relative_to(repo_root)}")

    if all_violations:
        print(
            f"FAIL  Disabled-benchmark policy violations found "
            f"({len(all_violations)} registration(s) in "
            f"{len({v.path for v in all_violations})} file(s)):\n"
        )
        for v in all_violations:
            print(f"  {v.path}:{v.line}  {v.func}")
            for m in v.missing:
                print(f"      missing: {m}")
        print(
            "\nFix: add a comment immediately above each BENCHMARK() line, e.g.:\n"
            "  // Disabled: <reason> | Deadline: vX.Y.Z | Issue: #NNN\n"
            "See benchmarks/docs/DISABLED_BENCHMARK_POLICY.md for details."
        )
        return 1

    total_stubs = sum(
        len(list(_RE_REGISTRATION.finditer(
            cpp.read_text(encoding="utf-8", errors="replace")
        )))
        for cpp in bench_dir.rglob("*.cpp")
        if cpp.exists()
    )
    print(
        f"OK    Disabled-benchmark policy check passed "
        f"({total_stubs} stub(s) in {files_with_disabled} file(s), "
        f"all compliant).  {files_checked} file(s) scanned."
    )
    return 0


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Lint disabled benchmark stubs for policy compliance."
    )
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=None,
        help="Repository root (default: parent of this script's directory).",
    )
    parser.add_argument(
        "--verbose", "-v",
        action="store_true",
        help="Print each compliant file as well.",
    )
    args = parser.parse_args()

    repo_root: Path
    if args.repo_root:
        repo_root = args.repo_root.resolve()
    else:
        repo_root = Path(__file__).resolve().parent.parent

    return run(repo_root, args.verbose)


if __name__ == "__main__":
    sys.exit(main())
