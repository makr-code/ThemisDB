"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            check_bench_targets.py                             ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-13                                         ║
  Author:          ThemisDB CI                                        ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

from __future__ import annotations

#!/usr/bin/env python3
"""
ThemisDB Bench-Source CI Guard
================================

Verifies that every ``bench_*.cpp`` file in ``benchmarks/`` is covered by a
CMake target so that no benchmark source can silently vanish from the build.

A source file is considered **covered** when at least one of the following
conditions is true:

1. An explicit ``add_executable(<name>`` call exists in
   ``benchmarks/CMakeLists.txt`` where ``<name>`` matches the stem of the
   source file (e.g. ``bench_foo`` for ``bench_foo.cpp``).

2. The ``benchmarks/CMakeLists.txt`` contains the
   ``THEMIS_AUTO_REGISTER_ELIGIBLE_BENCHMARKS`` auto-registration block,
   which dynamically creates ``EXCLUDE_FROM_ALL`` targets for all remaining
   sources at configure time.  When this block is present every bench source
   that is not explicitly wired is still guaranteed to receive a CMake target
   and therefore is **not** considered orphaned by this guard.

Exit codes
----------
0  All benchmark sources are covered.
1  One or more orphaned benchmark sources detected.
2  Internal error / bad arguments.

Usage
-----
    python3 tools/check_bench_targets.py [--benchmarks-dir DIR]
                                         [--cmake-file FILE]
                                         [--strict]
                                         [--format {text,json}]
                                         [--no-color]
                                         [-q]

Options
-------
    --benchmarks-dir DIR  Root directory of benchmark sources
                          (default: benchmarks/ relative to repo root)
    --cmake-file FILE     Path to the CMakeLists.txt to parse
                          (default: benchmarks/CMakeLists.txt)
    --strict              Treat auto-registration as insufficient; every
                          bench_*.cpp MUST have an explicit add_executable()
                          entry.  With --strict the guard will currently
                          report the ~81 files that rely on auto-registration.
    --format {text,json}  Output format (default: text)
    --no-color            Disable ANSI colour output
    -q, --quiet           Suppress per-file detail; only print the summary line
"""

import argparse
import json
import os
import re
import sys
from pathlib import Path
from typing import Optional

# ---------------------------------------------------------------------------
# ANSI helpers
# ---------------------------------------------------------------------------

_RESET = "\033[0m"
_BOLD = "\033[1m"
_RED = "\033[31m"
_GREEN = "\033[32m"
_YELLOW = "\033[33m"
_CYAN = "\033[36m"

_use_color = True


def _c(text: str, *codes: str) -> str:
    if not _use_color:
        return text
    return "".join(codes) + text + _RESET


# ---------------------------------------------------------------------------
# Core logic
# ---------------------------------------------------------------------------

_AUTO_REG_MARKER = "THEMIS_AUTO_REGISTER_ELIGIBLE_BENCHMARKS"


def find_bench_sources(benchmarks_dir: Path) -> list[str]:
    """Return sorted list of stem names for bench_*.cpp files."""
    sources = sorted(
        p.stem for p in benchmarks_dir.glob("bench_*.cpp") if p.is_file()
    )
    return sources


def parse_explicit_targets(cmake_file: Path) -> set[str]:
    """
    Return the set of target names declared via ``add_executable(...)`` in the
    given CMakeLists.txt.  Only the first token after ``add_executable(`` is
    captured; variable expansions (``${...}``) are excluded because they
    correspond to the auto-registration loop, not hand-wired entries.
    """
    pattern = re.compile(r"\badd_executable\(\s*([A-Za-z0-9_][A-Za-z0-9_.+-]*)")
    targets: set[str] = set()
    try:
        text = cmake_file.read_text(encoding="utf-8", errors="replace")
    except OSError as exc:
        print(
            _c(f"ERROR: cannot read {cmake_file}: {exc}", _RED, _BOLD),
            file=sys.stderr,
        )
        sys.exit(2)
    for match in pattern.finditer(text):
        name = match.group(1)
        if not name.startswith("${"):
            targets.add(name)
    return targets


def auto_registration_present(cmake_file: Path) -> bool:
    """Return True if the auto-registration block marker is found in cmake_file."""
    try:
        text = cmake_file.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return False
    return _AUTO_REG_MARKER in text


def check_bench_targets(
    benchmarks_dir: Path,
    cmake_file: Path,
    *,
    strict: bool = False,
) -> tuple[list[str], list[str], bool]:
    """
    Check benchmark sources against CMake targets.

    Returns
    -------
    (orphaned, explicit_only, auto_reg_present)
        orphaned         – source stems with no coverage (always empty when
                           auto-registration is present and strict=False)
        auto_reg_covered – source stems covered only by auto-registration
        auto_reg_present – whether the auto-registration block was found
    """
    sources = find_bench_sources(benchmarks_dir)
    explicit_targets = parse_explicit_targets(cmake_file)
    auto_reg = auto_registration_present(cmake_file)

    orphaned: list[str] = []
    auto_reg_covered: list[str] = []

    for stem in sources:
        has_explicit = stem in explicit_targets
        if has_explicit:
            continue
        # No explicit target
        if auto_reg and not strict:
            auto_reg_covered.append(stem)
        else:
            orphaned.append(stem)

    return orphaned, auto_reg_covered, auto_reg


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------


def _repo_root() -> Path:
    """Best-effort attempt to locate the repo root from this script's location."""
    return Path(__file__).resolve().parent.parent


def _resolve_defaults(args: argparse.Namespace) -> tuple[Path, Path]:
    root = _repo_root()
    bench_dir = Path(args.benchmarks_dir) if args.benchmarks_dir else root / "benchmarks"
    cmake_file = Path(args.cmake_file) if args.cmake_file else bench_dir / "CMakeLists.txt"
    return bench_dir.resolve(), cmake_file.resolve()


def format_text(
    orphaned: list[str],
    auto_reg_covered: list[str],
    auto_reg_present: bool,
    *,
    quiet: bool,
    strict: bool,
) -> str:
    lines: list[str] = []

    if not quiet:
        lines.append(
            _c("ThemisDB Bench-Source CI Guard", _BOLD)
            + f" – benchmarks/CMakeLists.txt coverage check"
        )
        lines.append("")

    n_orphaned = len(orphaned)
    n_auto = len(auto_reg_covered)

    if n_orphaned == 0 and n_auto == 0:
        lines.append(_c("✅ PASS", _GREEN, _BOLD) + "  All bench_*.cpp sources have explicit CMake targets.")
    elif n_orphaned == 0 and not strict:
        lines.append(
            _c("✅ PASS", _GREEN, _BOLD)
            + f"  All bench_*.cpp sources are covered."
            + f"  ({n_auto} via auto-registration, 0 orphaned)"
        )
        if not quiet:
            lines.append(
                _c("ℹ️  INFO", _CYAN)
                + f"  {n_auto} source(s) rely on THEMIS_AUTO_REGISTER_ELIGIBLE_BENCHMARKS"
                + " and have no explicit add_executable() entry."
            )
            lines.append(
                "      To add explicit targets run:"
                " python3 tools/check_bench_targets.py --strict"
            )
    else:
        lines.append(
            _c("❌ FAIL", _RED, _BOLD)
            + f"  {n_orphaned} orphaned bench_*.cpp source(s) detected."
        )
        if not auto_reg_present:
            lines.append(
                _c("⚠️  NOTE", _YELLOW)
                + "  Auto-registration block (THEMIS_AUTO_REGISTER_ELIGIBLE_BENCHMARKS)"
                " not found in CMakeLists.txt."
            )
        if not quiet:
            for name in orphaned:
                lines.append(f"    {_c('✗', _RED)}  benchmarks/{name}.cpp")

    if not quiet and strict and auto_reg_covered:
        lines.append("")
        lines.append(
            _c("⚠️  STRICT MODE", _YELLOW, _BOLD)
            + f"  {n_auto} source(s) covered only by auto-registration (no explicit target):"
        )
        for name in auto_reg_covered:
            lines.append(f"    {_c('~', _YELLOW)}  benchmarks/{name}.cpp")

    return "\n".join(lines)


def format_json(
    orphaned: list[str],
    auto_reg_covered: list[str],
    auto_reg_present: bool,
    *,
    strict: bool,
) -> str:
    return json.dumps(
        {
            "pass": len(orphaned) == 0,
            "auto_registration_present": auto_reg_present,
            "orphaned_sources": orphaned,
            "auto_reg_covered_sources": auto_reg_covered,
            "strict": strict,
            "counts": {
                "orphaned": len(orphaned),
                "auto_reg_covered": len(auto_reg_covered),
            },
        },
        indent=2,
    )


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="check_bench_targets.py",
        description="CI guard: ensure every bench_*.cpp has a CMake target.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    p.add_argument(
        "--benchmarks-dir",
        metavar="DIR",
        default="",
        help="Path to benchmarks directory (default: <repo-root>/benchmarks)",
    )
    p.add_argument(
        "--cmake-file",
        metavar="FILE",
        default="",
        help="Path to benchmarks/CMakeLists.txt (default: <benchmarks-dir>/CMakeLists.txt)",
    )
    p.add_argument(
        "--strict",
        action="store_true",
        default=False,
        help=(
            "Require explicit add_executable() for every bench_*.cpp; "
            "auto-registration alone is not sufficient."
        ),
    )
    p.add_argument(
        "--format",
        choices=["text", "json"],
        default="text",
        help="Output format (default: text)",
    )
    p.add_argument(
        "--no-color",
        action="store_true",
        default=False,
        help="Disable ANSI colour output",
    )
    p.add_argument(
        "-q",
        "--quiet",
        action="store_true",
        default=False,
        help="Suppress per-file detail",
    )
    return p


def main(argv: Optional[list[str]] = None) -> int:
    global _use_color

    parser = build_parser()
    args = parser.parse_args(argv)

    if args.no_color or not sys.stdout.isatty():
        _use_color = False

    bench_dir, cmake_file = _resolve_defaults(args)

    if not bench_dir.is_dir():
        print(
            _c(f"ERROR: benchmarks directory not found: {bench_dir}", _RED, _BOLD),
            file=sys.stderr,
        )
        return 2

    if not cmake_file.is_file():
        print(
            _c(f"ERROR: CMakeLists.txt not found: {cmake_file}", _RED, _BOLD),
            file=sys.stderr,
        )
        return 2

    orphaned, auto_reg_covered, auto_reg_present = check_bench_targets(
        bench_dir,
        cmake_file,
        strict=args.strict,
    )

    if args.format == "json":
        print(format_json(orphaned, auto_reg_covered, auto_reg_present, strict=args.strict))
    else:
        print(
            format_text(
                orphaned,
                auto_reg_covered,
                auto_reg_present,
                quiet=args.quiet,
                strict=args.strict,
            )
        )

    return 1 if orphaned else 0


if __name__ == "__main__":
    sys.exit(main())
