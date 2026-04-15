"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            check_disabled_stubs.py                            ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-15 18:19:51                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 RELEASE-CANDIDATE                            ║
    • Quality Score:   63.0/100                                       ║
    • Total Lines:     200                                            ║
    • Open Issues:     TODOs: 1, Stubs: 8                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • cf3e31ffa9  2026-04-13  feat(governance): Disabled-Stub-Policy für Benchmarks ein... ║
    • 1071f1d20f  2026-04-13  feat(governance): Disabled-Stub-Policy für Benchmarks ein... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ⚠️  Needs Work                                              ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
ThemisDB – Disabled-Stub-Policy CI Guard
=========================================

Scans all ``benchmarks/**/*.cpp`` files for ``BENCHMARK(BM_*_Disabled)``
registrations and verifies that each one carries both a **deadline** comment
and an **issue reference** in the same file.

Exit codes
----------
0  All disabled stubs are compliant (or none found).
1  At least one disabled stub violates the policy.
2  Internal error (bad arguments, unreadable directory, …).

See ``docs/governance/DISABLED_STUB_POLICY.md`` for the full policy.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Regex patterns
# ---------------------------------------------------------------------------

# Matches any BENCHMARK(BM_*_Disabled) or BENCHMARK(BM_*_disabled) call.
_RE_DISABLED_REGISTRATION = re.compile(
    r"BENCHMARK\s*\(\s*(\w+[Dd]isabled\w*)\s*\)"
)

# An issue reference: #123, Issue: #123, issue #123, gh-123, Ticket: #123, etc.
_RE_ISSUE = re.compile(
    r"(?:issue[:\s#]|ticket[:\s#]|jira[:\s#]|gh-\d+|github\.com.*/issues/\d+"
    r"|#\d{3,}|TODO\s*\(#\d+\)|FIXME\s*\(#\d+\))",
    re.IGNORECASE,
)

# A deadline reference: Deadline: v1.9.0, Deadline: 2026-Q3, due: vX.Y.Z, etc.
_RE_DEADLINE = re.compile(
    r"(?:Deadline\s*:|deadline\s*:|DEADLINE\s*:|due[_\s]*date\s*:|due\s*:|"
    r"fälligkeit\s*:|sunset\s*:)",
    re.IGNORECASE,
)

# Context window around each BENCHMARK() call (characters) to search for annotations.
_CONTEXT_CHARS = 300


# ---------------------------------------------------------------------------
# Core logic
# ---------------------------------------------------------------------------


def _read_file(path: Path) -> str | None:
    try:
        return path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return None


def _check_file(cpp: Path, repo_root: Path) -> list[str]:
    """Return a list of violation messages for *cpp* (empty = compliant)."""
    content = _read_file(cpp)
    if content is None:
        return [f"  WARN  Could not read {cpp.relative_to(repo_root)}"]

    violations: list[str] = []

    for match in _RE_DISABLED_REGISTRATION.finditer(content):
        func_name = match.group(1)
        start = max(0, match.start() - _CONTEXT_CHARS)
        end = min(len(content), match.end() + _CONTEXT_CHARS)
        context = content[start:end]

        has_deadline = bool(_RE_DEADLINE.search(context))
        has_issue = bool(_RE_ISSUE.search(context))

        if not (has_deadline and has_issue):
            rel = cpp.relative_to(repo_root)
            missing = []
            if not has_issue:
                missing.append("Issue reference (e.g. Issue: #1234)")
            if not has_deadline:
                missing.append("Deadline (e.g. Deadline: v2.1.0)")
            violations.append(
                f"  FAIL  {rel}::{func_name} — missing: {', '.join(missing)}"
            )

    return violations


def run(repo_root: Path) -> int:
    """Run the check; return 0 (pass), 1 (fail), or 2 (error)."""
    bench_dir = repo_root / "benchmarks"
    if not bench_dir.is_dir():
        print(
            f"ERROR: benchmarks directory not found: {bench_dir}",
            file=sys.stderr,
        )
        return 2

    all_violations: list[str] = []
    checked_files: int = 0
    disabled_count: int = 0

    for cpp in sorted(bench_dir.rglob("*.cpp")):
        content = _read_file(cpp)
        if content is None:
            continue
        if not _RE_DISABLED_REGISTRATION.search(content):
            continue
        checked_files += 1
        disabled_count += len(_RE_DISABLED_REGISTRATION.findall(content))
        all_violations.extend(_check_file(cpp, repo_root))

    # ------------------------------------------------------------------
    # Report
    # ------------------------------------------------------------------
    print("ThemisDB Disabled-Stub-Policy Check")
    print("=" * 60)

    if disabled_count == 0:
        print("  OK    No *_Disabled BENCHMARK stubs found.")
        return 0

    compliant = disabled_count - len(all_violations)
    print(
        f"  INFO  Found {disabled_count} disabled stub(s) in "
        f"{checked_files} file(s): "
        f"{compliant} compliant, {len(all_violations)} violation(s)."
    )

    if all_violations:
        print()
        print(
            "Policy: every BENCHMARK(BM_*_Disabled) must carry a deadline comment\n"
            "        AND an issue reference in the same file.\n"
            "        See docs/governance/DISABLED_STUB_POLICY.md for details.\n"
        )
        for v in all_violations:
            print(v)
        print()
        print(f"RESULT: FAIL — {len(all_violations)} violation(s) found.")
        return 1

    print("RESULT: PASS — all disabled stubs are policy-compliant.")
    return 0


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Check that every *_Disabled benchmark stub has a "
                    "deadline comment and issue reference.",
    )
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path("."),
        help="Root directory of the ThemisDB repository (default: CWD).",
    )
    args = parser.parse_args()

    repo_root = args.repo_root.resolve()
    sys.exit(run(repo_root))


if __name__ == "__main__":
    main()
