"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            changelog_updater.py                               ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-15 05:58:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     428                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03973ea1c5  2026-04-14  feat: shared CHANGELOG workflow + milestone-based histori... ║
    • 86745ceec2  2026-04-14  feat: shared CHANGELOG workflow + milestone-based histori... ║
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

Inserts a new structured entry into a section of ``CHANGELOG.md`` after a CI run.

By default the entry goes into **[Unreleased] → ### <section>**.
Pass ``--target-version`` to write into a specific versioned release block
(``## [x.y.z] - YYYY-MM-DD``) instead — useful for historical backfills.

If the target sub-section (``### <section>``) does not yet exist it is created
automatically.  If ``--target-version`` names a release block that does not
exist yet, the block is created and inserted in chronological order.

Idempotency guard
-----------------
The script embeds a hidden HTML comment ``<!-- changelog-updater: <key> -->``
at the end of each inserted entry.  Before inserting, it checks whether this
key already exists in the file.  If so, the run is a no-op.

Usage
-----
    # Write to [Unreleased] (default):
    python3 tools/ci/changelog_updater.py \\
        --entry-title "Module-Docs Sync" \\
        --entry-body  "- 47 modules processed" \\
        --entry-body  "- 277 files indexed" \\
        [--key    my-unique-run-key]  \\
        [--section Documentation]    \\
        [--changelog CHANGELOG.md]   \\
        [--dry-run]

    # Write to a specific versioned section (historical / backfill):
    python3 tools/ci/changelog_updater.py \\
        --target-version 1.8.0 \\
        --release-date   2026-03-22 \\
        --entry-title    "Build pipeline added" \\
        --entry-body     "- CI matrix for Linux / Windows" \\
        --section        Added

Options
-------
    --changelog PATH        CHANGELOG.md path (default: <repo-root>/CHANGELOG.md)
    --target-version VER    Write to ## [VER] instead of ## [Unreleased].
                            Creates the block if absent (date from --release-date).
    --release-date DATE     ISO date (YYYY-MM-DD) used when creating a new versioned
                            block.  Ignored when the block already exists.
                            Defaults to today when omitted.
    --section NAME          Sub-section name (default: Documentation)
    --entry-title TEXT      Bold title for the bullet point (required)
    --entry-body TEXT       Body line(s); may be repeated for multiple lines
    --key TEXT              Idempotency key (default: title)
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
# Matches any versioned heading like "## [1.8.0] - 2026-03-22" or "## [1.8.0]"
_VERSIONED_HEADING_RE = re.compile(
    r"^## \[(?P<ver>[^\]]+)\](?:[ \t]*-[ \t]*(?P<date>\d{4}-\d{2}-\d{2}))?[ \t]*$",
    re.MULTILINE,
)
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


def _versioned_range(content: str, version: str) -> Optional[tuple]:
    """Return (heading_start, body_start, body_end) for a versioned block.

    Searches for a heading matching ``## [<version>]`` (date suffix optional).
    Returns ``None`` when the version block does not exist.
    """
    for m in _VERSIONED_HEADING_RE.finditer(content):
        if m.group("ver") == version:
            body_start = m.end()
            next_heading = _VERSION_RE.search(content, body_start)
            body_end = next_heading.start() if next_heading else len(content)
            return m.start(), body_start, body_end
    return None


def _insert_versioned_block(content: str, version: str, release_date: str) -> str:
    """Insert a new ``## [version] - date`` heading block at the correct position.

    New blocks are inserted:
    - After the [Unreleased] block when present (most common case).
    - Before the first existing versioned block whose date is older than
      *release_date* (chronological ordering).
    - At the end of the file when no better position is found.

    Returns the modified content string.
    """
    heading = f"## [{version}] - {release_date}\n\n---\n\n"

    # Prefer inserting right after the [Unreleased] block.
    ur = _unreleased_range(content)
    if ur is not None:
        _, ur_end = ur
        return content[:ur_end] + heading + content[ur_end:]

    # No [Unreleased] section — insert before the first versioned block that is
    # chronologically older (or at the top if none found).
    for m in _VERSIONED_HEADING_RE.finditer(content):
        existing_date = m.group("date") or ""
        if existing_date and existing_date < release_date:
            return content[: m.start()] + heading + content[m.start() :]

    # Fall back: insert before the first ## [ heading.
    first = _VERSION_RE.search(content)
    insert_at = first.start() if first else len(content)
    return content[:insert_at] + heading + content[insert_at:]


def _find_section(block: str, section_name: str) -> Optional[re.Match]:
    """Search for ``### <section_name>`` inside *block*."""
    pat = re.compile(_SECTION_RE_TPL.format(name=re.escape(section_name)), re.MULTILINE)
    return pat.search(block)


def _insert_into_block(
    content: str,
    body_start: int,
    body_end: int,
    section_name: str,
    entry: str,
) -> str:
    """Insert *entry* into the correct sub-section within a block range."""
    block = content[body_start:body_end]
    section_m = _find_section(block, section_name)

    if section_m:
        heading_end_in_block = section_m.end()
        if (
            heading_end_in_block < len(block)
            and block[heading_end_in_block] == "\n"
        ):
            heading_end_in_block += 1
        abs_pos = body_start + heading_end_in_block
        return content[:abs_pos] + "\n" + entry + "\n" + content[abs_pos:]
    else:
        # Section missing — create it before the ``---`` separator or body_end.
        sep_m = re.search(r"^---[ \t]*$", block, re.MULTILINE)
        abs_pos = body_start + (sep_m.start() if sep_m else len(block))
        new_section = f"### {section_name}\n\n{entry}\n\n"
        return content[:abs_pos] + new_section + content[abs_pos:]


def insert_entry(
    content: str,
    section_name: str,
    entry: str,
    key: str,
    target_version: str = "",
    release_date: str = "",
) -> tuple:
    """Insert *entry* into the correct position in *content*.

    When *target_version* is empty (default), the entry goes into
    ``## [Unreleased] → ### section_name``.

    When *target_version* is set (e.g. ``"1.8.0"``), the entry goes into
    ``## [1.8.0] → ### section_name``.  If that versioned block does not yet
    exist, it is created using *release_date* (defaults to today).

    Returns ``(new_content, changed)`` where ``changed`` is ``False`` when
    the idempotency marker was already present.
    """
    marker = _idempotency_marker(key)

    # Idempotency check
    if marker in content:
        return content, False

    if target_version:
        # ── Versioned block path ──────────────────────────────────────────
        vr = _versioned_range(content, target_version)
        if vr is None:
            # Block does not exist yet — create it.
            date = release_date or datetime.date.today().strftime("%Y-%m-%d")
            content = _insert_versioned_block(content, target_version, date)
            vr = _versioned_range(content, target_version)
            if vr is None:  # should never happen
                raise RuntimeError(f"Failed to create block for version {target_version}")
        _heading_start, body_start, body_end = vr
        return _insert_into_block(content, body_start, body_end, section_name, entry), True

    # ── [Unreleased] path (original behaviour) ────────────────────────────
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
    return _insert_into_block(content, body_start, body_end, section_name, entry), True


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def build_arg_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description=(
            "Insert a CI-generated entry into CHANGELOG.md.\n"
            "Writes to [Unreleased] by default, or to a specific versioned\n"
            "block when --target-version is supplied."
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    p.add_argument("--changelog", metavar="PATH", help="Path to CHANGELOG.md")
    p.add_argument(
        "--target-version",
        default="",
        metavar="VER",
        dest="target_version",
        help=(
            "Write to ## [VER] instead of ## [Unreleased]. "
            "Creates the block if absent (date from --release-date)."
        ),
    )
    p.add_argument(
        "--release-date",
        default="",
        metavar="YYYY-MM-DD",
        dest="release_date",
        help=(
            "ISO date used when creating a new versioned block. "
            "Defaults to today when omitted."
        ),
    )
    p.add_argument(
        "--section",
        default="Documentation",
        metavar="NAME",
        help="Sub-section name (default: Documentation)",
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
        help="Idempotency key (default: entry-title)",
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

    key = args.key or args.entry_title

    entry = _build_entry(args.entry_title, args.entry_body, key)
    content = changelog_path.read_text(encoding="utf-8")

    new_content, changed = insert_entry(
        content,
        args.section,
        entry,
        key,
        target_version=args.target_version,
        release_date=args.release_date,
    )

    if not changed:
        if not args.quiet:
            print(f"CHANGELOG.md already contains entry for key '{key}' — skipped.")
        return 0

    if args.dry_run:
        if not args.quiet:
            target = f"## [{args.target_version}]" if args.target_version else "## [Unreleased]"
            print(f"--- dry-run: would insert into {target} → ### {args.section} ---")
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
        target = f"## [{args.target_version}]" if args.target_version else "## [Unreleased]"
        print(
            f"CHANGELOG.md updated: added entry '{args.entry_title}' "
            f"in {target} → ### {args.section}."
        )

    return 0


if __name__ == "__main__":
    sys.exit(main())
