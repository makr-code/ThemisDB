"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            check_bench_targets.py                             ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-15 07:24:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     858                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 774f817af3  2026-04-14  feat(ci-guard): bench-source guard – allowlist, Grundkate... ║
    • 99ffe76e1d  2026-04-14  feat(ci-guard): bench-source guard – allowlist, Grundkate... ║
    • acbec398e5  2026-04-13  feat(ci): implement bench-source CI guard (Issue #4, Maßn... ║
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

3. The source stem appears in the allowlist file (``--allowlist FILE``),
   meaning it is explicitly permitted to be absent from the build profile
   (e.g. GPU-only or platform-specific benchmarks).

In binary-check mode (``--build-dir DIR``) the guard additionally compares
bench_*.cpp sources against the bench_* executables that were actually produced
in the given build directory, reporting any source whose binary is absent and
not covered by the allowlist.

Exit codes
----------
0  All benchmark sources are covered (or allowlisted).
1  One or more orphaned benchmark sources detected.
2  Internal error / bad arguments.

Usage
-----
    python3 tools/check_bench_targets.py [--benchmarks-dir DIR]
                                         [--cmake-file FILE]
                                         [--allowlist FILE]
                                         [--build-dir DIR]
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
    --allowlist FILE      Path to the allowlist file listing bench stems that
                          are permitted to be absent from the build profile
                          (default: tools/bench_source_allowlist.toml if
                          present, otherwise no allowlist is applied)
    --build-dir DIR       When provided, also check that each covered bench
                          source has a corresponding executable (bench_* or
                          bench_*.exe) inside DIR.  Missing binaries that are
                          not allowlisted are reported as binary-orphans.
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

# tomllib is available in Python 3.11+; fall back to the regex-based parser
# for older runtimes (pre-commit hooks, dev machines with Python 3.10).
try:
    import tomllib as _tomllib  # type: ignore[import]
except ImportError:
    _tomllib = None  # type: ignore[assignment]

# ---------------------------------------------------------------------------
# Category helpers
# ---------------------------------------------------------------------------

# Maps first-segment prefixes to human-readable categories used in output.
# When no prefix matches, the raw first segment is used as the category.
_CATEGORY_MAP: dict[str, str] = {
    "acceleration": "acceleration",
    "active": "memory",
    "adaptive": "query",
    "advanced": "misc",
    "api": "api",
    "approximate": "geo",
    "aql": "aql",
    "arm": "platform",
    "async": "io",
    "auth": "security",
    "auto": "storage",
    "backend": "acceleration",
    "batch": "storage",
    "binary": "storage",
    "blob": "storage",
    "branch": "version",
    "cdc": "cdc",
    "changefeed": "cdc",
    "chaos": "resilience",
    "compliance": "security",
    "comprehensive": "misc",
    "compression": "storage",
    "config": "config",
    "content": "content",
    "core": "core",
    "cross": "misc",
    "crud": "storage",
    "cuda": "gpu",
    "cycle": "misc",
    "data": "storage",
    "di": "misc",
    "diff": "query",
    "distributed": "distributed",
    "docker": "platform",
    "edge": "misc",
    "embedded": "misc",
    "embedding": "ml",
    "encryption": "security",
    "ethics": "ml",
    "exporters": "observability",
    "extended": "misc",
    "flash": "storage",
    "fused": "query",
    "geo": "geo",
    "gnn": "ml",
    "gorilla": "timeseries",
    "gossip": "distributed",
    "governance": "security",
    "gpu": "gpu",
    "graph": "graph",
    "hnsw": "vector",
    "hot": "storage",
    "hotspots": "storage",
    "hsm": "security",
    "hybrid": "query",
    "image": "ml",
    "importer": "storage",
    "index": "index",
    "ingestion": "storage",
    "insert": "storage",
    "knowledge": "ml",
    "latency": "misc",
    "learned": "ml",
    "legal": "security",
    "llama": "ml",
    "llm": "ml",
    "locality": "storage",
    "lock": "concurrency",
    "lora": "ml",
    "lossy": "ml",
    "metadata": "storage",
    "metrics": "observability",
    "mixed": "misc",
    "mmdb": "geo",
    "module": "misc",
    "multi": "misc",
    "multithreading": "concurrency",
    "mvcc": "concurrency",
    "olap": "query",
    "pagerank": "graph",
    "phase1": "misc",
    "pii": "security",
    "plugin": "misc",
    "policy": "security",
    "postgres": "storage",
    "process": "misc",
    "product": "misc",
    "prompt": "ml",
    "qlora": "ml",
    "query": "query",
    "rag": "ml",
    "raid": "storage",
    "random": "misc",
    "replication": "distributed",
    "residual": "ml",
    "rotary": "ml",
    "saga": "distributed",
    "sanity": "misc",
    "scalability": "misc",
    "security": "security",
    "shard": "distributed",
    "sharding": "distributed",
    "simd": "platform",
    "simple": "misc",
    "snapshot": "storage",
    "spatial": "geo",
    "storage": "storage",
    "stream": "streaming",
    "task": "misc",
    "temporal": "timeseries",
    "text": "content",
    "thread": "concurrency",
    "timeseries": "timeseries",
    "tpcc": "benchmark",
    "tpch": "benchmark",
    "transaction": "concurrency",
    "update": "storage",
    "user": "misc",
    "v1": "misc",
    "vector": "vector",
    "video": "ml",
    "voice": "ml",
    "vulkan": "gpu",
    "wal": "storage",
    "whisper": "ml",
    "ycsb": "benchmark",
}


def get_category(stem: str) -> str:
    """
    Return the Grundkategorie (base category) for a bench_*.cpp stem.

    The category is derived from the first ``_``-separated segment after
    the ``bench_`` prefix.  Known prefixes are mapped to canonical category
    names via ``_CATEGORY_MAP``; unknown prefixes are returned as-is.
    """
    without_prefix = stem[len("bench_"):] if stem.startswith("bench_") else stem
    first_segment = without_prefix.split("_")[0]
    return _CATEGORY_MAP.get(first_segment, first_segment)

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


# ---------------------------------------------------------------------------
# Allowlist helpers
# ---------------------------------------------------------------------------

_DEFAULT_ALLOWLIST_RELPATH = "tools/bench_source_allowlist.toml"


def load_allowlist(allowlist_file: Path) -> dict[str, str]:
    """
    Parse an allowlist TOML file and return a mapping of ``stem → reason``.

    Expected TOML structure (only top-level string key-value pairs are used):

    .. code-block:: toml

        bench_cuda_vs_cpu = "GPU_ONLY: requires CUDA device"
        bench_arm_simd = "PLATFORM: ARM SIMD intrinsics"

    TOML section headers (``[section]``) are ignored; all ``bench_*`` keys
    found at any level are collected.

    When ``tomllib`` is not available (Python < 3.11) a line-by-line fallback
    parser is used that handles the same subset of the format.  Malformed
    lines in fallback mode are printed as warnings rather than silently
    skipped.

    Returns
    -------
    dict mapping stem (e.g. ``bench_cuda_vs_cpu``) to reason string.
    """
    result: dict[str, str] = {}
    if not allowlist_file.is_file():
        return result

    try:
        raw = allowlist_file.read_bytes()
    except OSError as exc:
        print(
            _c(f"WARNING: cannot read allowlist {allowlist_file}: {exc}", _YELLOW),
            file=sys.stderr,
        )
        return result

    # ── tomllib path (Python 3.11+) ─────────────────────────────────────────
    if _tomllib is not None:
        try:
            data = _tomllib.loads(raw.decode("utf-8", errors="replace"))
        except Exception as exc:  # noqa: BLE001
            print(
                _c(f"WARNING: TOML parse error in {allowlist_file}: {exc}", _YELLOW),
                file=sys.stderr,
            )
            data = {}
        # Collect all bench_* keys from any level (flatten nested tables)
        def _collect(node: object) -> None:
            if isinstance(node, dict):
                for k, v in node.items():
                    if k.startswith("bench_") and isinstance(v, str):
                        result[k] = v.strip()
                    else:
                        _collect(v)
            elif isinstance(node, list):
                for item in node:
                    _collect(item)

        _collect(data)
        return result

    # ── Fallback line-by-line parser (Python 3.10 and earlier) ─────────────
    _kv_re = re.compile(
        r"""^\s*(?P<stem>bench_[A-Za-z0-9_]+)\s*=\s*['"](?P<reason>[^'"]*)['"]\s*$"""
    )
    _bare_re = re.compile(
        r"""^\s*(?P<stem>bench_[A-Za-z0-9_]+)\s*(?:#\s*(?P<reason>.*))?$"""
    )

    for lineno, line in enumerate(
        raw.decode("utf-8", errors="replace").splitlines(), start=1
    ):
        stripped = line.strip()
        if not stripped or stripped.startswith("#") or stripped.startswith("["):
            continue
        m = _kv_re.match(stripped)
        if m:
            result[m.group("stem")] = m.group("reason").strip()
            continue
        m = _bare_re.match(stripped)
        if m:
            result[m.group("stem")] = (m.group("reason") or "").strip()
            continue
        print(
            _c(
                f"WARNING: {allowlist_file.name}:{lineno}: unrecognised line: {stripped!r}",
                _YELLOW,
            ),
            file=sys.stderr,
        )

    return result


def find_built_binaries(build_dir: Path) -> set[str]:
    """
    Return the set of bench_* executable stems found under ``build_dir``.

    Searches recursively for files whose name starts with ``bench_`` and
    is a recognisable executable:

    * ``bench_*.exe`` – Windows PE binary (checked by extension only).
    * ``bench_*`` (no extension) – Unix executable; verified with
      ``os.access(path, os.X_OK)`` to exclude non-executable files such as
      backup copies, lock files, or directory traversal artefacts.
    """
    found: set[str] = set()
    if not build_dir.is_dir():
        return found

    for path in build_dir.rglob("bench_*"):
        if not path.is_file():
            continue
        name = path.name
        if name.endswith(".exe"):
            found.add(name[:-4])
        elif "." not in name and os.access(path, os.X_OK):
            # Unix executable without extension
            found.add(name)
    return found


def check_bench_targets(
    benchmarks_dir: Path,
    cmake_file: Path,
    *,
    strict: bool = False,
    allowlist: Optional[dict[str, str]] = None,
    build_dir: Optional[Path] = None,
) -> tuple[list[str], list[str], bool, list[str]]:
    """
    Check benchmark sources against CMake targets and (optionally) built binaries.

    Parameters
    ----------
    benchmarks_dir : directory containing bench_*.cpp files
    cmake_file     : benchmarks/CMakeLists.txt
    strict         : require explicit add_executable(); auto-reg is not enough
    allowlist      : mapping of stem → reason for explicitly permitted absences
    build_dir      : when set, also verify actual built binaries exist here

    Returns
    -------
    (orphaned, auto_reg_covered, auto_reg_present, binary_missing)
        orphaned         – source stems with no CMake coverage (after allowlist filter)
        auto_reg_covered – source stems covered only by auto-registration
        auto_reg_present – whether the auto-registration block was found
        binary_missing   – source stems with CMake coverage but no built binary
                           (only populated when build_dir is provided; after
                           allowlist filter)
    """
    if allowlist is None:
        allowlist = {}

    sources = find_bench_sources(benchmarks_dir)
    explicit_targets = parse_explicit_targets(cmake_file)
    auto_reg = auto_registration_present(cmake_file)

    orphaned: list[str] = []
    auto_reg_covered: list[str] = []

    for stem in sources:
        if stem in allowlist:
            continue  # explicitly permitted absence
        has_explicit = stem in explicit_targets
        if has_explicit:
            continue
        # No explicit target
        if auto_reg and not strict:
            auto_reg_covered.append(stem)
        else:
            orphaned.append(stem)

    # Optional binary-level check
    binary_missing: list[str] = []
    if build_dir is not None:
        built = find_built_binaries(build_dir)
        for stem in sources:
            if stem in allowlist:
                continue
            if stem not in built:
                binary_missing.append(stem)

    return orphaned, auto_reg_covered, auto_reg, binary_missing


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------


def _repo_root() -> Path:
    """Best-effort attempt to locate the repo root from this script's location."""
    return Path(__file__).resolve().parent.parent


def _resolve_defaults(args: argparse.Namespace) -> tuple[Path, Path, Optional[Path], Optional[Path]]:
    root = _repo_root()
    bench_dir = Path(args.benchmarks_dir) if args.benchmarks_dir else root / "benchmarks"
    cmake_file = Path(args.cmake_file) if args.cmake_file else bench_dir / "CMakeLists.txt"

    # Allowlist: explicit path > default location > None
    allowlist_path: Optional[Path] = None
    if args.allowlist:
        allowlist_path = Path(args.allowlist).resolve()
    else:
        default_al = root / _DEFAULT_ALLOWLIST_RELPATH
        if default_al.is_file():
            allowlist_path = default_al

    # Build-dir for binary check
    build_dir: Optional[Path] = None
    if args.build_dir:
        build_dir = Path(args.build_dir).resolve()

    return bench_dir.resolve(), cmake_file.resolve(), allowlist_path, build_dir


def format_text(
    orphaned: list[str],
    auto_reg_covered: list[str],
    auto_reg_present: bool,
    *,
    quiet: bool,
    strict: bool,
    allowlist: Optional[dict[str, str]] = None,
    binary_missing: Optional[list[str]] = None,
    allowlist_file: Optional[Path] = None,
) -> str:
    if allowlist is None:
        allowlist = {}
    if binary_missing is None:
        binary_missing = []

    lines: list[str] = []

    if not quiet:
        lines.append(
            _c("ThemisDB Bench-Source CI Guard", _BOLD)
            + " – benchmarks/CMakeLists.txt coverage check"
        )
        if allowlist_file:
            lines.append(
                _c("ℹ️  Allowlist", _CYAN)
                + f"  {len(allowlist)} entries loaded from {allowlist_file.name}"
            )
        lines.append("")

    n_orphaned = len(orphaned)
    n_auto = len(auto_reg_covered)
    n_binary = len(binary_missing)

    # ── CMake-target check ──────────────────────────────────────────────────
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
                cat = get_category(name)
                lines.append(
                    f"    {_c('✗', _RED)}  benchmarks/{name}.cpp"
                    f"  {_c(f'[{cat}]', _YELLOW)}"
                )

    if not quiet and strict and auto_reg_covered:
        lines.append("")
        lines.append(
            _c("⚠️  STRICT MODE", _YELLOW, _BOLD)
            + f"  {n_auto} source(s) covered only by auto-registration (no explicit target):"
        )
        for name in auto_reg_covered:
            cat = get_category(name)
            lines.append(
                f"    {_c('~', _YELLOW)}  benchmarks/{name}.cpp"
                f"  {_c(f'[{cat}]', _YELLOW)}"
            )

    # ── Binary check (only when --build-dir was supplied) ──────────────────
    if binary_missing:
        lines.append("")
        lines.append(
            _c("❌ FAIL", _RED, _BOLD)
            + f"  {n_binary} bench_*.cpp source(s) have no built binary."
        )
        if not quiet:
            for name in binary_missing:
                cat = get_category(name)
                lines.append(
                    f"    {_c('✗', _RED)}  {name}  →  no binary found"
                    f"  {_c(f'[{cat}]', _YELLOW)}"
                )

    # ── Allowlist summary ───────────────────────────────────────────────────
    if not quiet and allowlist:
        lines.append("")
        lines.append(
            _c("ℹ️  ALLOWLIST", _CYAN, _BOLD)
            + f"  {len(allowlist)} source(s) are explicitly allowed to be absent:"
        )
        for stem, reason in sorted(allowlist.items()):
            cat = get_category(stem)
            reason_str = f"  # {reason}" if reason else ""
            lines.append(
                f"    {_c('○', _CYAN)}  benchmarks/{stem}.cpp"
                f"  {_c(f'[{cat}]', _CYAN)}{_c(reason_str, _CYAN)}"
            )

    return "\n".join(lines)


def format_json(
    orphaned: list[str],
    auto_reg_covered: list[str],
    auto_reg_present: bool,
    *,
    strict: bool,
    allowlist: Optional[dict[str, str]] = None,
    binary_missing: Optional[list[str]] = None,
) -> str:
    if allowlist is None:
        allowlist = {}
    if binary_missing is None:
        binary_missing = []

    def _with_category(stems: list[str]) -> list[dict[str, str]]:
        return [{"stem": s, "category": get_category(s)} for s in stems]

    return json.dumps(
        {
            "pass": len(orphaned) == 0 and len(binary_missing) == 0,
            "auto_registration_present": auto_reg_present,
            "orphaned_sources": _with_category(orphaned),
            "auto_reg_covered_sources": _with_category(auto_reg_covered),
            "binary_missing_sources": _with_category(binary_missing),
            "allowlisted_sources": [
                {"stem": s, "reason": r, "category": get_category(s)}
                for s, r in sorted(allowlist.items())
            ],
            "strict": strict,
            "counts": {
                "orphaned": len(orphaned),
                "auto_reg_covered": len(auto_reg_covered),
                "binary_missing": len(binary_missing),
                "allowlisted": len(allowlist),
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
        "--allowlist",
        metavar="FILE",
        default="",
        help=(
            "Path to the allowlist file (default: tools/bench_source_allowlist.toml "
            "if present, otherwise no allowlist is applied). "
            "Allowlisted sources are excluded from failure reporting."
        ),
    )
    p.add_argument(
        "--build-dir",
        metavar="DIR",
        default="",
        help=(
            "When provided, also verify that each bench source has an actual "
            "executable (bench_* or bench_*.exe) in this directory.  "
            "Sources with no binary and not on the allowlist are reported as "
            "binary-orphans."
        ),
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

    bench_dir, cmake_file, allowlist_path, build_dir = _resolve_defaults(args)

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

    allowlist = load_allowlist(allowlist_path) if allowlist_path else {}

    if build_dir and not build_dir.is_dir():
        print(
            _c(f"ERROR: build directory not found: {build_dir}", _RED, _BOLD),
            file=sys.stderr,
        )
        return 2

    orphaned, auto_reg_covered, auto_reg_present, binary_missing = check_bench_targets(
        bench_dir,
        cmake_file,
        strict=args.strict,
        allowlist=allowlist,
        build_dir=build_dir,
    )

    if args.format == "json":
        print(
            format_json(
                orphaned,
                auto_reg_covered,
                auto_reg_present,
                strict=args.strict,
                allowlist=allowlist,
                binary_missing=binary_missing,
            )
        )
    else:
        print(
            format_text(
                orphaned,
                auto_reg_covered,
                auto_reg_present,
                quiet=args.quiet,
                strict=args.strict,
                allowlist=allowlist,
                binary_missing=binary_missing,
                allowlist_file=allowlist_path,
            )
        )

    return 1 if (orphaned or binary_missing) else 0


if __name__ == "__main__":
    sys.exit(main())
