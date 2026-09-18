#!/usr/bin/env python3
"""Remove auto-generated Doxygen lines that contain '@note Gap Summary:' from source files.

The match is intentionally prefix-based to tolerate changing metric values.
"""

from __future__ import annotations

import argparse
from pathlib import Path
from typing import Iterable, Tuple

TARGET_EXTENSIONS = {
    ".c",
    ".cc",
    ".cpp",
    ".cxx",
    ".h",
    ".hh",
    ".hpp",
    ".hxx",
    ".ipp",
    ".tpp",
}

EXCLUDED_DIR_NAMES = {
    ".git",
    ".github",
    ".venv",
    ".vscode",
    "build",
    "build-msvc-windows-debug",
    "build-msvc-windows-release",
    "dist",
    "node_modules",
    "third_party",
    "external",
    "vendor",
    "artifacts",
}

GAP_SUMMARY_MARKER = "@note Gap Summary:"


def should_skip(path: Path) -> bool:
    return any(part in EXCLUDED_DIR_NAMES for part in path.parts)


def iter_source_files(root: Path) -> Iterable[Path]:
    for file_path in root.rglob("*"):
        if not file_path.is_file():
            continue
        if file_path.suffix.lower() not in TARGET_EXTENSIONS:
            continue
        if should_skip(file_path):
            continue
        yield file_path


def strip_gap_summary_lines(text: str) -> Tuple[str, int]:
    lines = text.splitlines(keepends=True)
    kept_lines = []
    removed = 0

    for line in lines:
        if GAP_SUMMARY_MARKER in line:
            removed += 1
            continue
        kept_lines.append(line)

    return "".join(kept_lines), removed


def load_text_with_fallback(path: Path) -> Tuple[str, str]:
    try:
        return path.read_text(encoding="utf-8"), "utf-8"
    except UnicodeDecodeError:
        return path.read_text(encoding="latin-1"), "latin-1"


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Remove all lines that contain '@note Gap Summary:' from C/C++ source files."
    )
    parser.add_argument(
        "--root",
        default=".",
        help="Repository root to scan (default: current directory)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Show files that would change without writing them",
    )
    args = parser.parse_args()

    root = Path(args.root).resolve()
    scanned_files = 0
    changed_files = 0
    removed_lines_total = 0

    for file_path in iter_source_files(root):
        scanned_files += 1
        content, encoding = load_text_with_fallback(file_path)
        updated, removed = strip_gap_summary_lines(content)

        if removed == 0:
            continue

        changed_files += 1
        removed_lines_total += removed
        rel = file_path.relative_to(root).as_posix()
        print(f"{rel}: removed {removed} line(s)")

        if not args.dry_run:
            file_path.write_text(updated, encoding=encoding)

    mode = "DRY-RUN" if args.dry_run else "WRITE"
    print(
        f"[{mode}] scanned={scanned_files} changed={changed_files} removed_lines={removed_lines_total}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
