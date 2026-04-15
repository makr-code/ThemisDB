"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            module_docs_builder.py                             ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 18:58:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     605                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • dbc9bfed9f  2026-04-13  Add CI/CD workflows and scripts for release management ║
    • dd319b9918  2026-04-13  Add CI/CD workflows and scripts for release management ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
ThemisDB Module-Docs Builder
=============================

Scans ``src/**`` and ``include/**`` for ``*.md`` files, groups them by
module (the top-level directory directly beneath ``src/<module>/`` or
``include/<module>/``), and generates one aggregated secondary-documentation
index page per module:

    docs/de/<module>/PRIMARY_SOURCES.md   (German labels)
    docs/en/<module>/PRIMARY_SOURCES.md   (English labels)

Each generated page strictly follows the ThemisDB documentation standard:
    • Clickable breadcrumb chain
    • Doc header: Datum/Date · Status · Primary source list · Bezug/Reference
    • Structured table of every primary Markdown file for the module
    • Deterministic output (modules and files sorted lexicographically)

Usage
-----
    python3 tools/module_docs_builder.py [OPTIONS]

Options
-------
    --repo-root DIR   Repository root (default: auto-detect from script location)
    --dry-run         Scan and report without writing any files
    --quiet           Suppress informational output to stdout

Exit codes
----------
    0   Success
    1   Unrecoverable error (e.g. repo-root not found)
"""

import argparse
import datetime
import json
import os
import sys
import tempfile
from pathlib import Path
from typing import Any, Dict, List, Optional

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

TOOL_NAME: str = "module_docs_builder"
TOOL_VERSION: str = "1.0.0"

#: Source directory roots to scan recursively together with the relative index
#: of the module name inside the repo-root-relative path.
SCAN_ROOTS: List[tuple[str, int]] = [
    ("src", 1),
    ("include", 1),
    ("external/chimera/src", 3),
    ("external/chimera/include", 3),
]


# ---------------------------------------------------------------------------
# File discovery
# ---------------------------------------------------------------------------


def get_repo_root(script_path: Path) -> Path:
    """Return the repository root (parent of the ``tools/`` directory)."""
    return script_path.resolve().parent.parent


def collect_module_files(repo_root: Path) -> Dict[str, List[str]]:
    """Scan configured source roots for ``*.md`` files.

    Files located directly in a scan directory (e.g. ``src/README.md``)
    are **skipped** because they do not belong to a named module.

    Returns
    -------
    dict
        Mapping of ``module_name`` → sorted list of repo-root-relative
        paths using forward slashes.
    """
    module_files: Dict[str, List[str]] = {}

    for scan_dir, module_index in SCAN_ROOTS:
        scan_path = repo_root / scan_dir
        if not scan_path.is_dir():
            continue

        for md_file in scan_path.rglob("*.md"):
            # [R3] Skip symlinks to prevent infinite loops on circular links.
            if md_file.is_symlink():
                continue
            rel_path = md_file.relative_to(repo_root)
            parts = rel_path.parts
            # Files sitting directly in the scan dir or above the configured
            # module index are skipped because they do not belong to a module.
            if len(parts) <= module_index + 1:
                continue

            module = parts[module_index]
            rel_str = "/".join(parts)  # always forward slashes
            module_files.setdefault(module, []).append(rel_str)

    # Deduplicate and sort file lists for deterministic output.
    for module in module_files:
        module_files[module] = sorted(set(module_files[module]))

    return module_files


# ---------------------------------------------------------------------------
# Page generation helpers
# ---------------------------------------------------------------------------


def _primary_source_list(files: List[str]) -> str:
    """Return a Markdown bullet list of backtick-wrapped file paths."""
    return "\n".join(f"- `{f}`" for f in files)


def _file_table_rows(files: List[str]) -> List[str]:
    """Return Markdown table body rows for *files*.

    Links are relative from ``docs/<lang>/<module>/PRIMARY_SOURCES.md``
    to the repository root (three levels up: ``../../..``).
    """
    rows: List[str] = []
    for f in files:
        filename = Path(f).name
        rel_link = f"../../../{f}"
        rows.append(f"| `{filename}` | [`{f}`]({rel_link}) |")
    return rows


def _group_files_by_source_root(module: str, files: List[str]) -> List[tuple[str, List[str]]]:
    """Group module files by their effective source-root directory."""
    grouped: Dict[str, List[str]] = {}
    for path in files:
        root = str(Path(path).parent).replace("\\", "/") + "/"
        grouped.setdefault(root, []).append(path)

    return [(root, grouped[root]) for root in sorted(grouped.keys())]


def _section(
    heading: str,
    files: List[str],
    col_file: str,
    col_path: str,
) -> List[str]:
    """Return Markdown lines for a module sub-section table."""
    if not files:
        return []
    return [
        f"### `{heading}`",
        "",
        f"| {col_file} | {col_path} |",
        f"|{''.join(['-'] * (len(col_file) + 2))}|{''.join(['-'] * (len(col_path) + 2))}|",
        *_file_table_rows(files),
        "",
    ]


# ---------------------------------------------------------------------------
# German page
# ---------------------------------------------------------------------------


def generate_de_page(module: str, files: List[str], today: str) -> str:
    """Generate the German ``PRIMARY_SOURCES.md`` for *module*."""
    primary_list = _primary_source_list(files)
    grouped_files = _group_files_by_source_root(module, files)

    header_lines: List[str] = [
        # Breadcrumb (trailing two spaces = Markdown line break)
        f"[docs](../../index.md) > [de](../index.md) > [{module}](./index.md) > [PRIMARY_SOURCES](./PRIMARY_SOURCES.md)  ",
        f"**Datum:** {today}  ",
        "**Status:** draft  ",
        "**Primary (Quelle der Wahrheit):**",
        primary_list,
        "",
        "**Bezug / Reference:**",
        f"- Tool: `tools/{TOOL_NAME}.py`",
        f"- Kontext: Automatisch generierter Index aller Primary-Markdown-Dateien des Moduls `{module}`",
        "",
        "---",
        "",
    ]

    body_lines: List[str] = [
        f"# Primary Sources — `{module}`",
        "",
        (
            f"Dieser Index listet alle Markdown-Dokumentationsdateien des Moduls "
            f"**`{module}`** aus den erkannten Quellverzeichnissen des Moduls."
        ),
        "",
        "## Primäre Markdown-Dateien",
        "",
        *[
            line
            for root, root_files in grouped_files
            for line in _section(root, root_files, "Datei", "Pfad")
        ],
        "---",
        "",
        f"*Automatisch generiert von `tools/{TOOL_NAME}.py` · {today}*",
        "",
    ]

    return "\n".join(header_lines + body_lines)


# ---------------------------------------------------------------------------
# English page
# ---------------------------------------------------------------------------


def generate_en_page(module: str, files: List[str], today: str) -> str:
    """Generate the English ``PRIMARY_SOURCES.md`` for *module*."""
    primary_list = _primary_source_list(files)
    grouped_files = _group_files_by_source_root(module, files)

    header_lines: List[str] = [
        # Breadcrumb
        f"[docs](../../index.md) > [en](../index.md) > [{module}](./index.md) > [PRIMARY_SOURCES](./PRIMARY_SOURCES.md)  ",
        f"**Date:** {today}  ",
        "**Status:** draft  ",
        "**Primary Source:**",
        primary_list,
        "",
        "**Reference:**",
        f"- Tool: `tools/{TOOL_NAME}.py`",
        f"- Context: Auto-generated index of all primary Markdown files for module `{module}`",
        "",
        "---",
        "",
    ]

    body_lines: List[str] = [
        f"# Primary Sources — `{module}`",
        "",
        (
            f"This index lists all Markdown documentation files for module "
            f"**`{module}`** from the detected source directories for that module."
        ),
        "",
        "## Primary Markdown Files",
        "",
        *[
            line
            for root, root_files in grouped_files
            for line in _section(root, root_files, "File", "Path")
        ],
        "---",
        "",
        f"*Auto-generated by `tools/{TOOL_NAME}.py` · {today}*",
        "",
    ]

    return "\n".join(header_lines + body_lines)


# ---------------------------------------------------------------------------
# File I/O
# ---------------------------------------------------------------------------


def _atomic_write(path: Path, content: str) -> None:
    """Write *content* to *path* atomically via a temp-file + ``os.replace()``.

    Guarantees that *path* is never left in a partially-written state even
    when the process is killed or loses power mid-write.
    The temp file is created in the same directory so the rename is always on
    the same filesystem (no cross-device link error).
    """
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp_fd, tmp_name = tempfile.mkstemp(
        dir=path.parent, prefix="tmp_docbuild_", suffix=path.suffix
    )
    try:
        with os.fdopen(tmp_fd, "w", encoding="utf-8") as fh:
            fh.write(content)
        os.replace(tmp_name, path)  # atomic on POSIX; best-effort on Windows
    except BaseException:  # covers KeyboardInterrupt, SystemExit, Exception
        try:
            os.unlink(tmp_name)
        except OSError:
            pass
        raise


def write_page(path: Path, content: str, dry_run: bool = False) -> bool:
    """Write *content* to *path*, creating parent directories as needed.

    Uses an atomic temp-file rename so the target is never partially written.

    Parameters
    ----------
    path:
        Destination file path.
    content:
        File content to write.
    dry_run:
        When *True*, no files are written; the function always returns *True*
        to indicate the file *would* be written.

    Returns
    -------
    bool
        ``True`` if the file was (or would be) written, ``False`` if the
        existing content is identical (skipped).
    """
    if dry_run:
        return True  # Always report "would write" in dry-run mode.

    # Idempotency check: compare bytes to avoid double-encoding overhead.
    new_bytes = content.encode("utf-8")
    if path.exists() and path.read_bytes() == new_bytes:
        return False  # Nothing changed; skip.

    _atomic_write(path, content)
    return True


# ---------------------------------------------------------------------------
# Secondary-doc coverage analysis (for issue reporter)
# ---------------------------------------------------------------------------


def scan_secondary_coverage(
    repo_root: Path,
    module_files: Dict[str, List[str]],
) -> Dict[str, Dict[str, Any]]:
    """Check each module for human-authored secondary documentation.

    A file is considered "human-authored" when it lives in
    ``docs/de/<module>/`` **and** is not ``PRIMARY_SOURCES.md``.

    Returns a dict mapping module → coverage info dict with keys:
      - ``files``             list of primary source paths
      - ``de_human_authored`` count of human-authored DE files
      - ``en_human_authored`` count of human-authored EN files
    """
    coverage: Dict[str, Dict[str, Any]] = {}

    for module, files in module_files.items():
        de_dir = repo_root / "docs" / "de" / module
        en_dir = repo_root / "docs" / "en" / module

        de_human = (
            [
                f.name
                for f in de_dir.glob("*.md")
                if f.name != "PRIMARY_SOURCES.md"
            ]
            if de_dir.exists()
            else []
        )
        en_human = (
            [
                f.name
                for f in en_dir.glob("*.md")
                if f.name != "PRIMARY_SOURCES.md"
            ]
            if en_dir.exists()
            else []
        )

        coverage[module] = {
            "files": files,
            "de_human_authored": len(de_human),
            "en_human_authored": len(en_human),
            "de_human_files": sorted(de_human),
            "en_human_files": sorted(en_human),
        }

    return coverage


def _build_issues_report(
    module_files: Dict[str, List[str]],
    coverage: Dict[str, Dict[str, Any]],
    new_modules: List[str],
    today: str,
) -> Dict[str, Any]:
    """Build the issues-JSON payload for the issue reporter."""
    underdocumented = sorted(
        module
        for module, info in coverage.items()
        if info["de_human_authored"] == 0
    )
    return {
        "generated_at": today,
        "generator": f"{TOOL_NAME} v{TOOL_VERSION}",
        "modules": {
            module: coverage[module]
            for module in sorted(module_files.keys())
        },
        "new_modules": sorted(new_modules),
        "underdocumented_modules": underdocumented,
    }


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            f"ThemisDB Module-Docs Builder v{TOOL_VERSION}\n\n"
            "Generates docs/de/<module>/PRIMARY_SOURCES.md and\n"
            "docs/en/<module>/PRIMARY_SOURCES.md for every module found\n"
            "in src/** and include/**."
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--repo-root",
        metavar="DIR",
        help="Repository root directory (default: auto-detect from script location)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Scan and report without writing any files",
    )
    parser.add_argument(
        "--quiet",
        action="store_true",
        help="Suppress informational output to stdout",
    )
    parser.add_argument(
        "--issues-json",
        metavar="PATH",
        help=(
            "Write a JSON report of underdocumented/new modules to this path "
            "(consumed by tools/ci/module_docs_issue_reporter.py)"
        ),
    )
    return parser


def _write_step_summary(
    module_files: Dict[str, List[str]],
    total_src_files: int,
    written_de: int,
    written_en: int,
    skipped: int,
    dry_run: bool,
) -> None:
    """Append a Markdown summary to ``$GITHUB_STEP_SUMMARY`` if available."""
    step_summary: Optional[str] = os.environ.get("GITHUB_STEP_SUMMARY")
    if not step_summary:
        return

    with open(step_summary, "a", encoding="utf-8") as fh:
        fh.write("## 📚 Module Docs Sync\n\n")
        fh.write("| Parameter | Value |\n")
        fh.write("|-----------|-------|\n")
        fh.write(f"| **Modules processed** | {len(module_files)} |\n")
        fh.write(f"| **Primary files indexed** | {total_src_files} |\n")
        if dry_run:
            fh.write("| **Mode** | dry-run (no files written) |\n")
        else:
            fh.write(f"| **DE pages written/updated** | {written_de} |\n")
            fh.write(f"| **EN pages written/updated** | {written_en} |\n")
            fh.write(f"| **Unchanged (skipped)** | {skipped} |\n")
        fh.write("\n")
        fh.write("### Modules\n\n")
        fh.write("| Module | Files |\n")
        fh.write("|--------|-------|\n")
        for module in sorted(module_files.keys()):
            fh.write(f"| `{module}` | {len(module_files[module])} |\n")


def main(argv: Optional[List[str]] = None) -> int:
    """Entry point for the module-docs builder CLI.

    Parameters
    ----------
    argv:
        Argument list to parse.  If *None*, ``sys.argv[1:]`` is used.

    Returns
    -------
    int
        0 on success, 1 on unrecoverable error.
    """
    parser = build_arg_parser()
    args = parser.parse_args(argv)

    # Resolve repository root.
    script_path = Path(__file__)
    repo_root = (
        Path(args.repo_root).resolve() if args.repo_root else get_repo_root(script_path)
    )

    if not repo_root.is_dir():
        print(f"ERROR: repo-root is not a directory: {repo_root}", file=sys.stderr)
        return 1

    today = datetime.date.today().strftime("%Y-%m-%d")

    # Discover module files.
    module_files = collect_module_files(repo_root)

    if not module_files:
        if not args.quiet:
            print("No Markdown files found in src/ or include/ module directories.")
        return 0

    total_src_files = sum(len(v) for v in module_files.values())
    written_de = 0
    written_en = 0
    skipped = 0
    new_modules: List[str] = []

    for module in sorted(module_files.keys()):
        files = module_files[module]

        # --- German page ---
        de_content = generate_de_page(module, files, today)
        de_path = repo_root / "docs" / "de" / module / "PRIMARY_SOURCES.md"
        de_is_new = not de_path.exists()
        if write_page(de_path, de_content, dry_run=args.dry_run):
            written_de += 1
            if de_is_new:
                new_modules.append(module)
        else:
            skipped += 1

        # --- English page ---
        en_content = generate_en_page(module, files, today)
        en_path = repo_root / "docs" / "en" / module / "PRIMARY_SOURCES.md"
        if write_page(en_path, en_content, dry_run=args.dry_run):
            written_en += 1
        else:
            skipped += 1

    # --- Issues JSON (optional) ---
    if args.issues_json:
        coverage = scan_secondary_coverage(repo_root, module_files)
        issues_report = _build_issues_report(module_files, coverage, new_modules, today)
        issues_path = Path(args.issues_json)
        _atomic_write(
            issues_path,
            json.dumps(issues_report, indent=2, ensure_ascii=False) + "\n",
        )
        if not args.quiet:
            print(f"Issues report written    : {issues_path}")
            print(f"Underdocumented modules  : {len(issues_report['underdocumented_modules'])}")

    # Console output.
    if not args.quiet:
        print(f"Modules processed        : {len(module_files)}")
        print(f"Primary files indexed    : {total_src_files}")
        if args.dry_run:
            print("[dry-run] No files written.")
        else:
            print(f"DE pages written/updated : {written_de}")
            print(f"EN pages written/updated : {written_en}")
            print(f"Unchanged (skipped)      : {skipped}")

    # GitHub Actions step summary.
    _write_step_summary(
        module_files=module_files,
        total_src_files=total_src_files,
        written_de=written_de,
        written_en=written_en,
        skipped=skipped,
        dry_run=args.dry_run,
    )

    return 0

if __name__ == "__main__":
    sys.exit(main())
