"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            perf_expectations_audit.py                         ║
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
ThemisDB Performance-Expectations Audit
==========================================

Orchestrates the checks defined in ``PERFORMANCE_EXPECTATIONS.md`` §1.4
(Top-10 Maßnahmen zur Vollabdeckung).  Each check maps to a numbered
*Maßnahme* and returns one of:

    STATUS_PASS  – criterion satisfied; no action required
    STATUS_WARN  – non-blocking advisory; action recommended
    STATUS_FAIL  – hard failure; CI must not pass

Current checks
--------------
    Check 8a  (Maßnahme #8)
        Verify that every ``bench_*.cpp`` in ``benchmarks/`` is covered by a
        CMake target.  A source is covered when it has an explicit
        ``add_executable()`` entry OR when the auto-registration block
        (``THEMIS_AUTO_REGISTER_ELIGIBLE_BENCHMARKS``) is present in
        ``benchmarks/CMakeLists.txt``.

        **Status: STATUS_FAIL** – CI hard-fails when orphaned_sources != [].

    Check 8b  (Maßnahme #8)
        Verify that the external standalone guard
        (``tools/check_bench_targets.py``) exists in the repository so that
        developers can run the check locally and as a pre-commit hook.

        **Status: STATUS_FAIL** – CI hard-fails when the guard script is absent.

Exit codes
----------
    0  All STATUS_FAIL checks passed.
    1  At least one STATUS_FAIL check failed.
    2  Internal error / bad arguments.

Usage
-----
    python3 tools/perf_expectations_audit.py [--benchmarks-dir DIR]
                                              [--cmake-file FILE]
                                              [--format {text,json}]
                                              [--no-color]
                                              [-q]
"""

import argparse
import json
import sys
from dataclasses import dataclass, field
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
_MAGENTA = "\033[35m"

_use_color = True


def _c(text: str, *codes: str) -> str:
    if not _use_color:
        return text
    return "".join(codes) + text + _RESET


# ---------------------------------------------------------------------------
# Check result model
# ---------------------------------------------------------------------------

STATUS_PASS = "PASS"
STATUS_WARN = "WARN"
STATUS_FAIL = "FAIL"


@dataclass
class CheckResult:
    check_id: str        # e.g. "8a"
    massnahme: int       # e.g. 8
    title: str
    status: str          # STATUS_PASS / STATUS_WARN / STATUS_FAIL
    message: str
    details: list[str] = field(default_factory=list)
    metadata: dict = field(default_factory=dict)


# ---------------------------------------------------------------------------
# Check 8a – orphaned bench sources
# ---------------------------------------------------------------------------

def _repo_root() -> Path:
    return Path(__file__).resolve().parent.parent


def run_check_8a(
    bench_dir: Path,
    cmake_file: Path,
) -> CheckResult:
    """
    Check 8a: every bench_*.cpp must be covered by a CMake target.

    A source is covered when it has an explicit add_executable() entry OR when
    THEMIS_AUTO_REGISTER_ELIGIBLE_BENCHMARKS is present in CMakeLists.txt.
    """
    # Import the standalone guard module inline to avoid circular dependencies.
    import importlib.util
    guard_path = _repo_root() / "tools" / "check_bench_targets.py"
    spec = importlib.util.spec_from_file_location("check_bench_targets", guard_path)
    mod = importlib.util.module_from_spec(spec)  # type: ignore[arg-type]
    spec.loader.exec_module(mod)  # type: ignore[union-attr]

    orphaned, auto_reg_covered, auto_reg_present = mod.check_bench_targets(
        bench_dir, cmake_file, strict=False
    )

    n_orphaned = len(orphaned)
    n_auto = len(auto_reg_covered)
    n_explicit = len(mod.find_bench_sources(bench_dir)) - n_orphaned - n_auto

    metadata = {
        "orphaned_sources": orphaned,
        "auto_reg_covered_count": n_auto,
        "explicit_target_count": n_explicit,
        "auto_registration_present": auto_reg_present,
    }

    if n_orphaned > 0:
        return CheckResult(
            check_id="8a",
            massnahme=8,
            title="bench_*.cpp ohne CMake-Target (orphaned sources)",
            status=STATUS_FAIL,
            message=(
                f"{n_orphaned} orphaned bench_*.cpp source(s) found with no CMake target coverage."
                f" Remove THEMIS_AUTO_REGISTER_ELIGIBLE_BENCHMARKS from CMakeLists.txt"
                f" is no longer safe."
            ),
            details=[f"benchmarks/{s}.cpp" for s in orphaned],
            metadata=metadata,
        )

    if n_auto > 0 and auto_reg_present:
        return CheckResult(
            check_id="8a",
            massnahme=8,
            title="bench_*.cpp ohne CMake-Target (orphaned sources)",
            status=STATUS_PASS,
            message=(
                f"All bench_*.cpp sources are covered. "
                f"{n_explicit} explicit target(s), "
                f"{n_auto} covered by auto-registration (THEMIS_AUTO_REGISTER_ELIGIBLE_BENCHMARKS)."
            ),
            metadata=metadata,
        )

    return CheckResult(
        check_id="8a",
        massnahme=8,
        title="bench_*.cpp ohne CMake-Target (orphaned sources)",
        status=STATUS_PASS,
        message=(
            f"All {n_explicit} bench_*.cpp sources have explicit CMake targets."
        ),
        metadata=metadata,
    )


# ---------------------------------------------------------------------------
# Check 8b – external guard script exists
# ---------------------------------------------------------------------------

def run_check_8b() -> CheckResult:
    """
    Check 8b: tools/check_bench_targets.py must exist.

    The standalone guard enables pre-commit and local developer usage
    independent of the full audit orchestrator.
    """
    guard_path = _repo_root() / "tools" / "check_bench_targets.py"

    if guard_path.is_file():
        return CheckResult(
            check_id="8b",
            massnahme=8,
            title="Externer Guard (check_bench_targets.py) vorhanden",
            status=STATUS_PASS,
            message=f"Guard script found: {guard_path.relative_to(_repo_root())}",
            metadata={"guard_path": str(guard_path)},
        )

    return CheckResult(
        check_id="8b",
        massnahme=8,
        title="Externer Guard (check_bench_targets.py) vorhanden",
        status=STATUS_FAIL,
        message=(
            f"Guard script not found: tools/check_bench_targets.py. "
            "Create the script to enable pre-commit and local guard usage."
        ),
        metadata={"guard_path": str(guard_path)},
    )


# ---------------------------------------------------------------------------
# Audit runner
# ---------------------------------------------------------------------------

def run_all_checks(
    bench_dir: Path,
    cmake_file: Path,
) -> list[CheckResult]:
    return [
        run_check_8a(bench_dir, cmake_file),
        run_check_8b(),
    ]


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

_STATUS_ICON = {
    STATUS_PASS: "✅",
    STATUS_WARN: "⚠️ ",
    STATUS_FAIL: "❌",
}

_STATUS_COLOR = {
    STATUS_PASS: (_GREEN, _BOLD),
    STATUS_WARN: (_YELLOW, _BOLD),
    STATUS_FAIL: (_RED, _BOLD),
}


def format_text_report(results: list[CheckResult], *, quiet: bool) -> str:
    lines: list[str] = []
    if not quiet:
        lines.append(_c("ThemisDB Performance-Expectations Audit", _BOLD, _MAGENTA))
        lines.append(_c("PERFORMANCE_EXPECTATIONS.md §1.4 – Maßnahme #8 CI-Guard", _CYAN))
        lines.append("")

    for r in results:
        icon = _STATUS_ICON.get(r.status, "?")
        color = _STATUS_COLOR.get(r.status, ())
        status_str = _c(f"{r.status:4}", *color)
        lines.append(f"  {icon}  Check {r.check_id}  [{status_str}]  {r.title}")
        if not quiet:
            lines.append(f"          {r.message}")
            for detail in r.details:
                lines.append(f"            {_c('•', _RED)}  {detail}")
            lines.append("")

    fails = [r for r in results if r.status == STATUS_FAIL]
    warns = [r for r in results if r.status == STATUS_WARN]
    passes = [r for r in results if r.status == STATUS_PASS]

    summary_color = _GREEN if not fails else _RED
    lines.append(
        _c(
            f"Summary: {len(passes)} PASS  {len(warns)} WARN  {len(fails)} FAIL"
            f"  ({'all checks passed' if not fails else f'{len(fails)} hard failure(s)'})",
            summary_color,
            _BOLD,
        )
    )
    return "\n".join(lines)


def format_json_report(results: list[CheckResult]) -> str:
    fails = [r for r in results if r.status == STATUS_FAIL]
    return json.dumps(
        {
            "pass": len(fails) == 0,
            "summary": {
                STATUS_PASS: len([r for r in results if r.status == STATUS_PASS]),
                STATUS_WARN: len([r for r in results if r.status == STATUS_WARN]),
                STATUS_FAIL: len(fails),
            },
            "checks": [
                {
                    "id": r.check_id,
                    "massnahme": r.massnahme,
                    "title": r.title,
                    "status": r.status,
                    "message": r.message,
                    "details": r.details,
                    "metadata": r.metadata,
                }
                for r in results
            ],
        },
        indent=2,
    )


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="perf_expectations_audit.py",
        description=(
            "ThemisDB performance-expectations audit. "
            "Implements PERFORMANCE_EXPECTATIONS.md §1.4 CI checks."
        ),
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
        help="Suppress per-check detail; only print the summary",
    )
    return p


def _resolve_paths(args: argparse.Namespace) -> tuple[Path, Path]:
    root = _repo_root()
    bench_dir = Path(args.benchmarks_dir) if args.benchmarks_dir else root / "benchmarks"
    cmake_file = Path(args.cmake_file) if args.cmake_file else bench_dir / "CMakeLists.txt"
    return bench_dir.resolve(), cmake_file.resolve()


def main(argv: Optional[list[str]] = None) -> int:
    global _use_color

    parser = build_parser()
    args = parser.parse_args(argv)

    if args.no_color or not sys.stdout.isatty():
        _use_color = False

    bench_dir, cmake_file = _resolve_paths(args)

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

    results = run_all_checks(bench_dir, cmake_file)

    if args.format == "json":
        print(format_json_report(results))
    else:
        print(format_text_report(results, quiet=args.quiet))

    fails = [r for r in results if r.status == STATUS_FAIL]
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
