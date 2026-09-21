#!/usr/bin/env python3
"""
remove_gap_summary_headers.py
─────────────────────────────────────────────────────────────────────────────
One-time cleanup: removes auto-generated `@note Gap Summary` lines from all
C/C++ source files under src/.

Background
----------
Every file in src/ contains a Doxygen file-header block that was automatically
stamped by the `add_doc_metadata.py` tool.  One of those lines is:

    * @note Gap Summary: total=N; TODO=N, Stub=N, ...

These lines cause every standard TODO/STUB/MOCK grep to produce ~1 500 false
positives, drowning the 48 real actionable TODOs in noise (see
audit/ACTIONABLE_TODOS_2026-09-21.md).

This script removes ONLY that one line (exact pattern match) — it does not
touch any other content.

Usage
-----
    # Dry-run (preview affected files, no writes):
    python3 scripts/remove_gap_summary_headers.py --dry-run

    # Apply in-place:
    python3 scripts/remove_gap_summary_headers.py

    # Restrict to a sub-directory:
    python3 scripts/remove_gap_summary_headers.py --root src/chimera

Flags
-----
    --dry-run   Preview mode: list affected files, print diff stats, no writes.
    --root DIR  Root directory to scan (default: src/).
    --verbose   Print each modified file path.

Exit codes
----------
    0  Success (or nothing to do in dry-run).
    1  Unexpected error.
"""

import argparse
import re
import sys
from pathlib import Path

# Pattern that identifies the auto-generated Gap Summary annotation.
# Must match the full line content (leading whitespace + trailing optional CR).
_PATTERN = re.compile(r'[ \t]*\*[ \t]+@note Gap Summary:.*')

_EXTENSIONS = {'.cpp', '.cc', '.c', '.h', '.hpp', '.hxx', '.cxx', '.ipp', '.tpp'}


def _process_file(path: Path, dry_run: bool, verbose: bool) -> int:
    """
    Remove all @note Gap Summary lines from *path*.

    Returns the number of lines removed.
    """
    try:
        original = path.read_text(encoding='utf-8', errors='replace')
    except OSError as exc:
        print(f'ERROR reading {path}: {exc}', file=sys.stderr)
        return 0

    lines = original.splitlines(keepends=True)
    cleaned = [line for line in lines if not _PATTERN.fullmatch(line.rstrip('\r\n'))]
    removed = len(lines) - len(cleaned)

    if removed == 0:
        return 0

    if verbose or dry_run:
        print(f'{"[DRY-RUN] " if dry_run else ""}  {path}  (-{removed} line{"s" if removed != 1 else ""})')

    if not dry_run:
        try:
            path.write_text(''.join(cleaned), encoding='utf-8')
        except OSError as exc:
            print(f'ERROR writing {path}: {exc}', file=sys.stderr)

    return removed


def main() -> int:
    parser = argparse.ArgumentParser(
        description='Remove auto-generated @note Gap Summary lines from C/C++ sources.'
    )
    parser.add_argument(
        '--dry-run', action='store_true',
        help='Preview mode: list affected files but do not write changes.'
    )
    parser.add_argument(
        '--root', default='src',
        help='Root directory to scan (default: src).'
    )
    parser.add_argument(
        '--verbose', action='store_true',
        help='Print each modified file path even when not in dry-run.'
    )
    args = parser.parse_args()

    root = Path(args.root)
    if not root.is_dir():
        print(f'ERROR: --root "{root}" is not a directory.', file=sys.stderr)
        return 1

    total_files = 0
    total_lines = 0

    for path in sorted(root.rglob('*')):
        if path.suffix in _EXTENSIONS and path.is_file():
            removed = _process_file(path, dry_run=args.dry_run, verbose=args.verbose)
            if removed:
                total_files += 1
                total_lines += removed

    mode = 'DRY-RUN' if args.dry_run else 'DONE'
    print(
        f'\n[{mode}] Processed {total_files} file(s), '
        f'removed {total_lines} @note Gap Summary line(s).'
    )
    if args.dry_run and total_lines > 0:
        print('Re-run without --dry-run to apply changes.')

    return 0


if __name__ == '__main__':
    sys.exit(main())
