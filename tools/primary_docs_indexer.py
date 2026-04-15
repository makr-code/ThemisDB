"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            primary_docs_indexer.py                            ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-15 04:33:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     504                                            ║
    • Open Issues:     TODOs: 1, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 071f2f6199  2026-03-09  feat: add primary-docs index/inventory generator ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
ThemisDB Primary-Docs Index / Inventory Generator
==================================================

Scans configured directories (src/, include/, examples/ and optionally the
repository root) for *primary* documentation files – i.e. files whose names
match well-known patterns such as README, ARCHITECTURE, CHANGELOG, etc. –
and writes a structured inventory to ``docs/_generated/primary_index.json``
(JSON) or ``docs/_generated/primary_index.yaml`` (YAML).

Usage
-----
    python3 tools/primary_docs_indexer.py [OPTIONS]

Options
-------
    --repo-root DIR     Root of the repository (default: script's parent dir)
    --output FILE       Output file path
                        (default: docs/_generated/primary_index.json)
    --format {json,yaml}
                        Output format (default: json)
    --include-root      Also scan the repository root directory (non-recursive)
    --scan-dirs DIRS    Comma-separated list of directories to scan recursively
                        (default: src,include,examples)
    --no-git            Skip git-based last_modified lookup (faster, but omits
                        timestamps when the git command is unavailable)
    --pretty            Pretty-print JSON output (default: true for json)
    --quiet             Suppress informational output

Exit codes
----------
    0   Success
    1   Unrecoverable error
"""

import argparse
import datetime
import json
import os
import re
import subprocess
import sys
from dataclasses import asdict, dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Tuple

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

# Filename patterns that identify "primary" documentation files.
# Each tuple is (regex-pattern, canonical-type-name).
# Patterns are matched case-insensitively against the *stem* (filename without
# extension) of each discovered file.
DOC_TYPE_PATTERNS: List[Tuple[str, str]] = [
    (r"^readme$",                    "README"),
    (r"^architecture$",              "ARCHITECTURE"),
    (r"^changelog$",                 "CHANGELOG"),
    (r"^contributing$",              "CONTRIBUTING"),
    (r"^license$",                   "LICENSE"),
    (r"^security$",                  "SECURITY"),
    (r"^code[_\-]?of[_\-]?conduct$","CODE_OF_CONDUCT"),
    (r"^support$",                   "SUPPORT"),
    (r"^setup$",                     "SETUP"),
    (r"^install(?:ation)?$",         "INSTALL"),
    (r"^maintainers?$",              "MAINTAINERS"),
    (r"^authors?$",                  "AUTHORS"),
    (r"^notice$",                    "NOTICE"),
    (r"^todo$",                      "TODO"),
    (r"^roadmap$",                   "ROADMAP"),
    (r"^faq$",                       "FAQ"),
    (r"^hacking$",                   "HACKING"),
    (r"^development$",               "DEVELOPMENT"),
    (r"^design$",                    "DESIGN"),
    (r"^index$",                     "INDEX"),
]

# Compile patterns once.
_COMPILED_PATTERNS: List[Tuple[re.Pattern, str]] = [
    (re.compile(pat, re.IGNORECASE), doc_type)
    for pat, doc_type in DOC_TYPE_PATTERNS
]

# Extensions considered to be plain-text documentation.
DOC_EXTENSIONS: frozenset[str] = frozenset({
    ".md", ".rst", ".txt", ".adoc", ".asciidoc"
})

# Status marker patterns searched in the first ``STATUS_SCAN_LINES`` lines of
# each file.  Matched case-insensitively.
STATUS_MARKERS: List[Tuple[str, str]] = [
    (r"\bDRAFT\b",      "draft"),
    (r"\bWIP\b",        "wip"),
    (r"\bDEPRECATED\b", "deprecated"),
    (r"\bOBSOLETE\b",   "deprecated"),
    (r"\bARCHIVED\b",   "archived"),
    (r"\bTODO\b",       "incomplete"),
    (r"\bSTUB\b",       "incomplete"),
]

_COMPILED_STATUS: List[Tuple[re.Pattern, str]] = [
    (re.compile(pat, re.IGNORECASE), status)
    for pat, status in STATUS_MARKERS
]

STATUS_SCAN_LINES = 10  # Only scan the first N lines for status markers.

# ---------------------------------------------------------------------------
# Data structures
# ---------------------------------------------------------------------------


@dataclass
class DocEntry:
    """Represents a single discovered primary-documentation file."""

    path: str                        # Relative path from repo root
    type: str                        # Canonical doc type (e.g. README)
    extension: str                   # File extension (e.g. .md)
    scan_dir: str                    # Which top-level scan directory was used
    last_modified: Optional[str]     # ISO-8601 datetime string or None
    status: str                      # active | draft | wip | deprecated | archived | incomplete
    size_bytes: int                  # File size in bytes


@dataclass
class PrimaryIndex:
    """Root document written to the output file."""

    generated_at: str                # ISO-8601 UTC timestamp of generation
    generator: str                   # Tool name + version
    repo_root: str                   # Absolute path used as the scan root
    scan_dirs: List[str]             # Directories that were scanned
    total_files: int                 # Total number of entries
    entries: List[DocEntry]          # Discovered files


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def classify_doc_type(stem: str) -> Optional[str]:
    """Return the canonical type for *stem*, or ``None`` if not a primary doc."""
    for pattern, doc_type in _COMPILED_PATTERNS:
        if pattern.match(stem):
            return doc_type
    return None


def detect_status(file_path: Path) -> str:
    """
    Detect the editorial status of a documentation file.

    Reads the first :data:`STATUS_SCAN_LINES` lines and returns the
    most severe status marker found, or ``"active"`` if none is found.
    """
    try:
        with file_path.open(encoding="utf-8", errors="replace") as fh:
            head = "".join(fh.readline() for _ in range(STATUS_SCAN_LINES))
    except OSError:
        return "active"

    for pattern, status in _COMPILED_STATUS:
        if pattern.search(head):
            return status
    return "active"


def git_last_modified(file_path: Path, repo_root: Path) -> Optional[str]:
    """
    Return the ISO-8601 UTC timestamp of the most recent git commit that
    touched *file_path*, or ``None`` if git is unavailable or the file is
    not tracked.
    """
    try:
        result = subprocess.run(
            [
                "git", "log", "-1",
                "--format=%cI",
                "--",
                str(file_path.relative_to(repo_root)),
            ],
            capture_output=True,
            text=True,
            check=False,
            cwd=str(repo_root),
            timeout=10,
        )
        ts = result.stdout.strip()
        return ts if ts else None
    except (FileNotFoundError, subprocess.TimeoutExpired, OSError):
        return None


def fs_last_modified(file_path: Path) -> str:
    """Return the ISO-8601 UTC mtime of *file_path* as a fallback."""
    mtime = file_path.stat().st_mtime
    dt = datetime.datetime.fromtimestamp(mtime, tz=datetime.timezone.utc)
    return dt.strftime("%Y-%m-%dT%H:%M:%S") + "Z"


# ---------------------------------------------------------------------------
# Core scan logic
# ---------------------------------------------------------------------------


def scan_directory(
    directory: Path,
    repo_root: Path,
    scan_label: str,
    recursive: bool,
    use_git: bool,
) -> List[DocEntry]:
    """
    Scan *directory* for primary documentation files.

    Parameters
    ----------
    directory:   Absolute path of the directory to scan.
    repo_root:   Repository root (used to compute relative paths and run git).
    scan_label:  Short label stored in each entry's ``scan_dir`` field.
    recursive:   Whether to descend into subdirectories.
    use_git:     Whether to call git for last_modified timestamps.

    Returns
    -------
    List of :class:`DocEntry` objects for every primary doc found.
    """
    entries: List[DocEntry] = []

    if not directory.is_dir():
        return entries

    glob_pattern = "**/*" if recursive else "*"

    for file_path in sorted(directory.glob(glob_pattern)):
        if not file_path.is_file():
            continue

        ext = file_path.suffix.lower()
        if ext not in DOC_EXTENSIONS:
            continue

        doc_type = classify_doc_type(file_path.stem)
        if doc_type is None:
            continue

        # Relative path from repo root, using forward slashes for portability.
        rel_path = file_path.relative_to(repo_root).as_posix()

        if use_git:
            last_modified = git_last_modified(file_path, repo_root)
        else:
            last_modified = None

        # Always fall back to filesystem mtime when git returns nothing.
        if last_modified is None:
            last_modified = fs_last_modified(file_path)

        status = detect_status(file_path)
        size_bytes = file_path.stat().st_size

        entries.append(DocEntry(
            path=rel_path,
            type=doc_type,
            extension=ext,
            scan_dir=scan_label,
            last_modified=last_modified,
            status=status,
            size_bytes=size_bytes,
        ))

    return entries


# ---------------------------------------------------------------------------
# Output serialisation
# ---------------------------------------------------------------------------


def _index_to_dict(index: PrimaryIndex) -> dict:
    """Convert a :class:`PrimaryIndex` to a plain dict suitable for JSON/YAML."""
    return {
        "generated_at": index.generated_at,
        "generator": index.generator,
        "repo_root": index.repo_root,
        "scan_dirs": index.scan_dirs,
        "total_files": index.total_files,
        "entries": [asdict(e) for e in index.entries],
    }


def write_json(index: PrimaryIndex, output_path: Path, pretty: bool) -> None:
    """Write *index* as JSON to *output_path*."""
    data = _index_to_dict(index)
    indent = 2 if pretty else None
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", encoding="utf-8") as fh:
        json.dump(data, fh, indent=indent, ensure_ascii=False)
        if pretty:
            fh.write("\n")


def write_yaml(index: PrimaryIndex, output_path: Path) -> None:
    """Write *index* as YAML to *output_path*."""
    try:
        import yaml  # type: ignore[import]
    except ImportError:
        print(
            "ERROR: PyYAML is not installed. "
            "Install it with: pip install pyyaml",
            file=sys.stderr,
        )
        sys.exit(1)

    data = _index_to_dict(index)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", encoding="utf-8") as fh:
        yaml.dump(data, fh, default_flow_style=False, allow_unicode=True, sort_keys=False)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="primary_docs_indexer",
        description="Generate a JSON/YAML inventory of primary documentation files.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--repo-root",
        metavar="DIR",
        default=None,
        help="Repository root (default: parent of this script)",
    )
    parser.add_argument(
        "--output",
        metavar="FILE",
        default=None,
        help=(
            "Output file path "
            "(default: docs/_generated/primary_index.json or .yaml)"
        ),
    )
    parser.add_argument(
        "--format",
        choices=["json", "yaml"],
        default="json",
        help="Output format (default: json)",
    )
    parser.add_argument(
        "--scan-dirs",
        metavar="DIRS",
        default="src,include,examples",
        help=(
            "Comma-separated directories to scan recursively "
            "(default: src,include,examples)"
        ),
    )
    parser.add_argument(
        "--include-root",
        action="store_true",
        default=False,
        help="Also scan the repository root (non-recursive)",
    )
    parser.add_argument(
        "--no-git",
        action="store_true",
        default=False,
        help="Skip git-based last_modified lookup",
    )
    parser.add_argument(
        "--no-pretty",
        action="store_true",
        default=False,
        help="Compact JSON output (no indentation)",
    )
    parser.add_argument(
        "--quiet",
        action="store_true",
        default=False,
        help="Suppress informational output",
    )
    return parser


def main(argv: Optional[List[str]] = None) -> int:
    """
    Entry point for the primary-docs indexer CLI.

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
    if args.repo_root:
        repo_root = Path(args.repo_root).resolve()
    else:
        repo_root = Path(__file__).resolve().parent.parent

    if not repo_root.is_dir():
        print(f"ERROR: repo-root is not a directory: {repo_root}", file=sys.stderr)
        return 1

    # Determine output path.
    ext = "yaml" if args.format == "yaml" else "json"
    default_output = repo_root / "docs" / "_generated" / f"primary_index.{ext}"
    output_path = Path(args.output).resolve() if args.output else default_output

    use_git = not args.no_git
    pretty = not args.no_pretty

    # Build list of (directory, label, recursive) tuples to scan.
    scan_targets: List[Tuple[Path, str, bool]] = []
    for raw in args.scan_dirs.split(","):
        label = raw.strip()
        if label:
            scan_targets.append((repo_root / label, label, True))

    if args.include_root:
        scan_targets.append((repo_root, ".", False))

    # Run scans.
    all_entries: List[DocEntry] = []
    scan_labels: List[str] = []

    for directory, label, recursive in scan_targets:
        if not args.quiet:
            print(f"  Scanning {'recursively ' if recursive else ''}in: {directory}")
        found = scan_directory(directory, repo_root, label, recursive, use_git)
        all_entries.extend(found)
        if label not in scan_labels:
            scan_labels.append(label)

    # Build index document.
    now_utc = (
        datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%S") + "Z"
    )
    index = PrimaryIndex(
        generated_at=now_utc,
        generator="primary_docs_indexer v0.0.1",
        repo_root=str(repo_root),
        scan_dirs=scan_labels,
        total_files=len(all_entries),
        entries=all_entries,
    )

    # Write output.
    if args.format == "yaml":
        write_yaml(index, output_path)
    else:
        write_json(index, output_path, pretty=pretty)

    if not args.quiet:
        print(f"\nPrimary docs index written to: {output_path}")
        print(f"Total entries: {index.total_files}")
        if index.total_files:
            by_type: Dict[str, int] = {}
            for entry in index.entries:
                by_type[entry.type] = by_type.get(entry.type, 0) + 1
            for doc_type, count in sorted(by_type.items()):
                print(f"  {doc_type}: {count}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
