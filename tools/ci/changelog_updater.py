"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            changelog_updater.py                               ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-13 20:54:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     300                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • afcb89febb  2026-03-12  fix: robustness/performance/efficiency improvements for d... ║
    • 212c6d4a65  2026-03-12  feat: add changelog_updater, module_docs_issue_reporter, ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
ThemisDB Changelog Updater
==========================

Inserts a new structured entry into the **[Unreleased] → ### Documentation**
section of ``CHANGELOG.md`` after a CI run.

If the ``### Documentation`` sub-section does not yet exist inside
``[Unreleased]``, it is created automatically (directly before the ``---``
separator or at the end of the Unreleased block).

Idempotency guard
-----------------
The script embeds a hidden HTML comment ``<!-- changelog-updater: <key> -->``
at the end of each inserted entry.  Before inserting, it checks whether this
key already exists in the file.  If so, the run is a no-op.

Usage
-----
    python3 tools/ci/changelog_updater.py \\
        --entry-title "Module-Docs Sync" \\
        --entry-body  "- 47 modules processed" \\
        --entry-body  "- 277 files indexed" \\
        [--key    my-unique-run-key]  \\
        [--section Documentation]    \\
        [--changelog CHANGELOG.md]   \\
        [--dry-run]                  \\
        [--quiet]

Options
-------
    --changelog PATH        CHANGELOG.md path (default: <repo-root>/CHANGELOG.md)
    --section NAME          Sub-section name under [Unreleased] (default: Documentation)
    --entry-title TEXT      Bold title for the bullet point (required)
    --entry-body TEXT       Body line(s); may be repeated for multiple lines
    --key TEXT              Idempotency key (default: title + today's date)
    --dry-run               Print the result without writing
    --quiet                 Suppress output

Exit codes
----------
    0   Success (entry written or already present)
    1   Unrecoverable error
"""

import argparse
import datetime
import os
import re
import sys
import tempfile
from pathlib import Path
from typing import List, Optional

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

_UNRELEASED_RE = re.compile(r"^## \[Unreleased\][ \t]*$", re.MULTILINE)
_VERSION_RE = re.compile(r"^## \[", re.MULTILINE)
_SECTION_RE_TPL = r"^### {name}[ \t]*$"


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def _find_repo_root(script: Path) -> Path:
    """Return repo root (three levels up from tools/ci/<script>)."""
    return script.resolve().parent.parent.parent


def _idempotency_marker(key: str) -> str:
    return f"<!-- changelog-updater: {key} -->"


def _build_entry(title: str, body_lines: List[str], key: str) -> str:
    """Compose the full Markdown bullet to insert."""
    lines = [f"- **{title}**"]
    for line in body_lines:
        lines.append(f"  {line}" if not line.startswith("  ") else line)
    lines.append(f"  {_idempotency_marker(key)}")
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Core insertion logic
# ---------------------------------------------------------------------------


def _unreleased_range(content: str) -> Optional[tuple]:
    """Return (start, end) char offsets of the [Unreleased] body.

    *start* points to the character after the heading newline;
    *end* points to the character where the next ``## [`` heading starts
    (or end-of-file).
    Returns ``None`` when no ``[Unreleased]`` heading exists.
    """
    m = _UNRELEASED_RE.search(content)
    if not m:
        return None
    body_start = m.end()
    next_version = _VERSION_RE.search(content, body_start)
    body_end = next_version.start() if next_version else len(content)
    return body_start, body_end


def _find_section(block: str, section_name: str) -> Optional[re.Match]:
    """Search for ``### <section_name>`` inside *block*."""
    pat = re.compile(_SECTION_RE_TPL.format(name=re.escape(section_name)), re.MULTILINE)
    return pat.search(block)


def insert_entry(
    content: str,
    section_name: str,
    entry: str,
    key: str,
) -> tuple:
    """Insert *entry* into the correct position in *content*.

    Returns ``(new_content, changed)`` where ``changed`` is ``False`` when
    the idempotency marker was already present.
    """
    marker = _idempotency_marker(key)

    # Idempotency check
    if marker in content:
        return content, False

    ur = _unreleased_range(content)

    if ur is None:
        # No [Unreleased] section — prepend one before the first version.
        m = _VERSION_RE.search(content)
        insert_at = m.start() if m else len(content)
        block = (
            f"## [Unreleased]\n\n"
            f"### {section_name}\n\n"
            f"{entry}\n\n"
            f"---\n\n"
        )
        return content[:insert_at] + block + content[insert_at:], True

    body_start, body_end = ur
    unreleased_body = content[body_start:body_end]

    section_m = _find_section(unreleased_body, section_name)

    if section_m:
        # Section already exists — insert entry right after the heading line.
        heading_end_in_block = section_m.end()
        # Skip a single trailing newline that belongs to the heading itself.
        if (
            heading_end_in_block < len(unreleased_body)
            and unreleased_body[heading_end_in_block] == "\n"
        ):
            heading_end_in_block += 1

        abs_pos = body_start + heading_end_in_block
        new_content = content[:abs_pos] + "\n" + entry + "\n" + content[abs_pos:]
    else:
        # Section missing — create it.  Insert before the ``---`` separator
        # if one exists in the Unreleased block, else before body_end.
        sep_m = re.search(r"^---[ \t]*$", unreleased_body, re.MULTILINE)
        if sep_m:
            abs_pos = body_start + sep_m.start()
        else:
            abs_pos = body_end

        new_section = f"### {section_name}\n\n{entry}\n\n"
        new_content = content[:abs_pos] + new_section + content[abs_pos:]

    return new_content, True


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def build_arg_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="Insert a CI-generated entry into CHANGELOG.md [Unreleased].",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    p.add_argument("--changelog", metavar="PATH", help="Path to CHANGELOG.md")
    p.add_argument(
        "--section",
        default="Documentation",
        metavar="NAME",
        help="Sub-section under [Unreleased] (default: Documentation)",
    )
    p.add_argument(
        "--entry-title",
        required=True,
        metavar="TEXT",
        help="Bold title text for the new bullet",
    )
    p.add_argument(
        "--entry-body",
        action="append",
        default=[],
        metavar="LINE",
        dest="entry_body",
        help="Body line (repeatable)",
    )
    p.add_argument(
        "--key",
        metavar="TEXT",
        help="Idempotency key (default: title + today's date)",
    )
    p.add_argument("--dry-run", action="store_true", help="Do not write the file")
    p.add_argument("--quiet", action="store_true", help="Suppress output")
    return p


def main(argv=None) -> int:
    parser = build_arg_parser()
    args = parser.parse_args(argv)

    # Resolve CHANGELOG.md path.
    if args.changelog:
        changelog_path = Path(args.changelog).resolve()
    else:
        repo_root = _find_repo_root(Path(__file__))
        changelog_path = repo_root / "CHANGELOG.md"

    if not changelog_path.exists():
        print(f"ERROR: CHANGELOG.md not found: {changelog_path}", file=sys.stderr)
        return 1

    today = datetime.date.today().strftime("%Y-%m-%d")
    key = args.key or args.entry_title

    entry = _build_entry(args.entry_title, args.entry_body, key)
    content = changelog_path.read_text(encoding="utf-8")

    new_content, changed = insert_entry(content, args.section, entry, key)

    if not changed:
        if not args.quiet:
            print(f"CHANGELOG.md already contains entry for key '{key}' — skipped.")
        return 0

    if args.dry_run:
        if not args.quiet:
            print("--- dry-run: would insert ---")
            print(entry)
            print("--- end ---")
        return 0

    # [R1] Atomic write: temp-file + os.replace() prevents partial-write corruption.
    tmp_fd, tmp_name = tempfile.mkstemp(
        dir=changelog_path.parent, prefix="tmp_changelog_", suffix=".md"
    )
    try:
        with os.fdopen(tmp_fd, "w", encoding="utf-8") as fh:
            fh.write(new_content)
        os.replace(tmp_name, changelog_path)
    except BaseException:
        try:
            os.unlink(tmp_name)
        except OSError:
            pass
        raise

    if not args.quiet:
        print(f"CHANGELOG.md updated: added entry '{args.entry_title}' in ### {args.section}.")

    return 0


if __name__ == "__main__":
    sys.exit(main())
