#!/usr/bin/env python3
"""ai-working-compact.py — ai_working/ cleanup, topic compaction, and archive rotation.

Implements the Karpathy LLM-Wiki pattern for ThemisDB ai_working/:
  https://gist.github.com/karpathy/442a6bf555914893e9891c11519de94f

  Each topic produces one compact wiki page in ai_working/compact/<TOPIC>.md that
  is human-readable AND LLM-context-optimised (key facts, decisions, status,
  not raw execution logs).

Modes
-----
--compact   Group files by topic prefix, produce/update compact wiki pages
            in ai_working/compact/.
--archive   Move files older than --archive-days into ai_working/ARCHIVE_PRE_<YYYY_MM>/
            (default 90 days; pinned/compact/index files are never archived).
--cleanup   Remove empty files, duplicate names and files inside ARCHIVE dirs
            that are themselves older than --cleanup-days (default 365 days).
--all       Run all three modes in order: compact → archive → cleanup.

Usage
-----
    python scripts/ai-working-compact.py --all
    python scripts/ai-working-compact.py --compact --dry-run
    python scripts/ai-working-compact.py --archive --archive-days 60
"""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import sys
from collections import defaultdict
from dataclasses import dataclass, field
from datetime import UTC, datetime, timedelta
from pathlib import Path
from typing import Dict, List, Optional, Sequence

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

TOOL_NAME = "ai-working-compact"
TOOL_VERSION = "1.0.0"

AI_WORKING = Path("ai_working")
COMPACT_DIR = AI_WORKING / "compact"
INDEX_FILE = COMPACT_DIR / "INDEX.md"

# Files that must never be archived or removed
PINNED_FILES = {
    "00_START_HERE.md",
    "00_STREAM_B_START_HERE.md",
    "AI_WIKI_INTEGRATION_PLAYBOOK.md",
}

# Directories inside ai_working/ that should never be touched
PROTECTED_DIRS = {"compact", "ARCHIVE_PRE_2026_08"}

# Regex prefix → canonical topic name mapping (checked in order)
TOPIC_PATTERNS: List[tuple[re.Pattern, str]] = [
    (re.compile(r"ANALYTICS"), "ANALYTICS"),
    (re.compile(r"AQL"), "AQL"),
    (re.compile(r"ACCELERATION"), "ACCELERATION"),
    (re.compile(r"AI_DECISION|AI_WIKI"), "AI_FRAMEWORK"),
    (re.compile(r"BATCH\d|BATCH_"), "BATCH_DELIVERY"),
    (re.compile(r"BLOCK\d|BLOCK_"), "BATCH_DELIVERY"),
    (re.compile(r"PHASE\d|Phase[_\s]*\d"), "PHASE_REPORTS"),
    (re.compile(r"WAVE[_\s]*[A-Z0-9]"), "WAVE_REPORTS"),
    (re.compile(r"GAP|gap_"), "GAP_CLOSURE"),
    (re.compile(r"GA_|GA[_\s]"), "GA_CLOSURE"),
    (re.compile(r"PROCESS_MODULE|PROCESS_PHASE"), "PROCESS_MODULE"),
    (re.compile(r"UPDATES_MODULE|UPDATES_PHASE"), "UPDATES_MODULE"),
    (re.compile(r"UTILS_MODULE|UTILS_PHASE"), "UTILS_MODULE"),
    (re.compile(r"SECURITY|PENTEST|SANITIZER"), "SECURITY"),
    (re.compile(r"CI_|CICD|CI\-"), "CICD"),
    (re.compile(r"STREAM_[A-Z]"), "STREAM_REPORTS"),
    (re.compile(r"AGENT\d|AGENT_"), "AGENT_REPORTS"),
    (re.compile(r"FINAL_|COMPREHENSIVE_"), "FINAL_REPORTS"),
    (re.compile(r"IMPLEMENTATION_|IMPL_"), "IMPLEMENTATION"),
    (re.compile(r"EXECUTION|EXECUTION_"), "EXECUTION"),
]

# Maximum number of source files to inline verbatim per compact page
# (larger topics are summarised by header extraction only)
MAX_INLINE_FILES = 15
MAX_INLINE_SIZE_BYTES = 12 * 1024  # 12 KB per file inline limit


# ---------------------------------------------------------------------------
# Data types
# ---------------------------------------------------------------------------


@dataclass
class FileEntry:
    path: Path
    mtime: datetime
    size: int


@dataclass
class Stats:
    compacted: int = 0
    archived: int = 0
    removed: int = 0
    topics: int = 0


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def _mtime(p: Path) -> datetime:
    return datetime.fromtimestamp(p.stat().st_mtime, tz=UTC)


def _classify_topic(name: str) -> str:
    for pattern, topic in TOPIC_PATTERNS:
        if pattern.search(name):
            return topic
    return "MISC"


def _extract_headers(text: str, max_lines: int = 60) -> str:
    """Return heading lines and first sentence / short lines from text."""
    lines = text.splitlines()
    out: List[str] = []
    for line in lines[:max_lines]:
        stripped = line.strip()
        if stripped.startswith("#") or (stripped and len(stripped) < 120 and not stripped.startswith("```")):
            out.append(stripped)
            if len(out) >= max_lines:
                break
    return "\n".join(out)


def _safe_read(p: Path) -> str:
    try:
        return p.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return ""


def _write_if_changed(p: Path, content: str, dry_run: bool) -> bool:
    if not dry_run:
        p.parent.mkdir(parents=True, exist_ok=True)
        existing = p.read_text(encoding="utf-8") if p.exists() else ""
        if existing == content:
            return False
        p.write_text(content, encoding="utf-8")
    return True


# ---------------------------------------------------------------------------
# Compact
# ---------------------------------------------------------------------------


def compact(dry_run: bool, stats: Stats) -> None:
    """Group ai_working/ root files by topic and produce compact wiki pages."""
    files: List[FileEntry] = []
    for p in sorted(AI_WORKING.iterdir()):
        if p.is_dir():
            continue
        if p.name in PINNED_FILES:
            continue
        if p.suffix.lower() not in {".md", ".txt", ".json", ".yml", ".yaml"}:
            continue
        files.append(FileEntry(path=p, mtime=_mtime(p), size=p.stat().st_size))

    # Group by topic
    by_topic: Dict[str, List[FileEntry]] = defaultdict(list)
    for fe in files:
        topic = _classify_topic(fe.path.stem.upper())
        by_topic[topic].append(fe)

    now_str = datetime.now(UTC).strftime("%Y-%m-%d")
    stats.topics = len(by_topic)

    for topic, entries in sorted(by_topic.items()):
        entries.sort(key=lambda e: e.mtime, reverse=True)
        page = _build_compact_page(topic, entries, now_str)
        dest = COMPACT_DIR / f"{topic}.md"
        if _write_if_changed(dest, page, dry_run):
            stats.compacted += 1
            print(f"  [compact] wrote {dest}  ({len(entries)} sources)")

    # Always regenerate index
    _write_index(by_topic, now_str, dry_run)
    print(f"  [compact] {stats.compacted} pages updated, {stats.topics} topics")


def _build_compact_page(topic: str, entries: List[FileEntry], date: str) -> str:
    """Build a Karpathy-style LLM wiki page for a topic."""
    lines: List[str] = [
        f"# ai_working compact — {topic}",
        "",
        f"**Generated:** {date}  ",
        f"**Sources:** {len(entries)} files  ",
        f"**Last source mtime:** {entries[0].mtime.strftime('%Y-%m-%d') if entries else 'n/a'}  ",
        "",
        "> This page is auto-generated by `scripts/ai-working-compact.py`.",
        "> It is the canonical LLM-wiki summary for this topic group.",
        "> Do not edit manually — edits will be overwritten on next run.",
        "",
        "---",
        "",
        "## Source Files",
        "",
    ]

    for fe in entries:
        lines.append(f"- `{fe.path.name}` ({fe.mtime.strftime('%Y-%m-%d')}, {fe.size:,} B)")

    lines += ["", "---", "", "## Compacted Content", ""]

    inline_count = 0
    for fe in entries:
        text = _safe_read(fe.path)
        if not text.strip():
            continue

        lines.append(f"### {fe.path.name}")
        lines.append("")

        if inline_count < MAX_INLINE_FILES and fe.size <= MAX_INLINE_SIZE_BYTES:
            # Inline full content
            lines.append(text.strip())
            inline_count += 1
        else:
            # Header-extract only (LLM-optimised summary)
            lines.append("*(file too large — key headings extracted)*")
            lines.append("")
            lines.append(_extract_headers(text))

        lines.append("")
        lines.append("---")
        lines.append("")

    return "\n".join(lines)


def _write_index(by_topic: Dict[str, List[FileEntry]], date: str, dry_run: bool) -> None:
    total = sum(len(v) for v in by_topic.values())
    lines = [
        "# ai_working/compact — Index",
        "",
        f"**Generated:** {date}  ",
        f"**Topics:** {len(by_topic)}  ",
        f"**Total source files grouped:** {total}  ",
        "",
        "> Auto-generated LLM-wiki index following the Karpathy LLM Wiki pattern.",
        "> Each page below is a compact knowledge blob for one topic.",
        "",
        "## Topic Pages",
        "",
    ]
    for topic in sorted(by_topic.keys()):
        count = len(by_topic[topic])
        lines.append(f"- [{topic}.md]({topic}.md) — {count} source files")

    lines += [
        "",
        "## Usage for LLM Agents",
        "",
        "1. Load `INDEX.md` to discover available topics.",
        "2. Load the relevant `<TOPIC>.md` page(s) into context.",
        "3. For canonical/normative facts always cross-check the SOT sources",
        "   listed in each page header (ROADMAP.md, FUTURE_ENHANCEMENTS.md, etc.).",
        "",
        "---",
        f"*Generated by {TOOL_NAME} v{TOOL_VERSION}*",
    ]
    _write_if_changed(INDEX_FILE, "\n".join(lines), dry_run)


# ---------------------------------------------------------------------------
# Archive
# ---------------------------------------------------------------------------


def archive(archive_days: int, dry_run: bool, stats: Stats) -> None:
    """Move files older than archive_days into ai_working/ARCHIVE_PRE_<YYYY_MM>/."""
    cutoff = datetime.now(UTC) - timedelta(days=archive_days)
    archive_label = cutoff.strftime("ARCHIVE_PRE_%Y_%m")
    dest_dir = AI_WORKING / archive_label

    moved: List[Path] = []
    for p in sorted(AI_WORKING.iterdir()):
        if p.is_dir():
            continue
        if p.name in PINNED_FILES:
            continue
        if p.parent.name.startswith("ARCHIVE_"):
            continue
        if _mtime(p) < cutoff:
            moved.append(p)

    if not moved:
        print(f"  [archive] nothing to archive (cutoff {cutoff.date()}, {archive_days}d)")
        return

    print(f"  [archive] {len(moved)} files → {dest_dir}/")
    if not dry_run:
        dest_dir.mkdir(parents=True, exist_ok=True)
        # Write/update README if not present
        readme = dest_dir / "README.md"
        if not readme.exists():
            readme.write_text(
                f"# {archive_label} — AI Working Archive\n\n"
                f"**Created:** {datetime.now(UTC).strftime('%Y-%m-%d')}  \n"
                f"**Cutoff:** files older than {archive_days} days ({cutoff.date()})  \n\n"
                "Files here are non-normative historical snapshots.\n"
                "See `ai_working/compact/` for the compacted wiki representation.\n",
                encoding="utf-8",
            )

    for p in moved:
        target = dest_dir / p.name
        if not dry_run:
            if target.exists():
                # Deduplicate with a suffix
                target = dest_dir / f"{p.stem}_dup{p.suffix}"
            p.rename(target)
        stats.archived += 1

    print(f"  [archive] moved {stats.archived} files")


# ---------------------------------------------------------------------------
# Cleanup
# ---------------------------------------------------------------------------


def cleanup(cleanup_days: int, dry_run: bool, stats: Stats) -> None:
    """Remove empty files and archive entries older than cleanup_days."""
    cutoff = datetime.now(UTC) - timedelta(days=cleanup_days)

    # Remove zero-byte files in ai_working/ root (not pinned)
    for p in sorted(AI_WORKING.iterdir()):
        if p.is_dir():
            continue
        if p.name in PINNED_FILES:
            continue
        if p.stat().st_size == 0:
            print(f"  [cleanup] remove empty: {p}")
            if not dry_run:
                p.unlink()
            stats.removed += 1

    # Remove very old files inside ARCHIVE_ dirs
    for archive_dir in sorted(AI_WORKING.iterdir()):
        if not archive_dir.is_dir():
            continue
        if not archive_dir.name.startswith("ARCHIVE_"):
            continue
        for p in sorted(archive_dir.iterdir()):
            if p.name == "README.md":
                continue
            if _mtime(p) < cutoff:
                print(f"  [cleanup] remove old archive entry: {p}")
                if not dry_run:
                    p.unlink()
                stats.removed += 1

    print(f"  [cleanup] removed {stats.removed} files")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def main(argv: Optional[Sequence[str]] = None) -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("--compact", action="store_true", help="Run topic compaction")
    ap.add_argument("--archive", action="store_true", help="Run archive rotation")
    ap.add_argument("--cleanup", action="store_true", help="Run stale-file cleanup")
    ap.add_argument("--all", action="store_true", help="Run compact + archive + cleanup")
    ap.add_argument("--archive-days", type=int, default=90, help="Age threshold for archiving (default: 90)")
    ap.add_argument("--cleanup-days", type=int, default=365, help="Age threshold for archive cleanup (default: 365)")
    ap.add_argument("--dry-run", action="store_true", help="Print actions without modifying files")
    ap.add_argument("--repo-root", default=".", help="Repository root (default: .)")
    args = ap.parse_args(argv)

    # Change to repo root so relative paths work
    os.chdir(args.repo_root)

    if not (args.compact or args.archive or args.cleanup or args.all):
        ap.error("Specify at least one mode: --compact, --archive, --cleanup, or --all")

    if not AI_WORKING.is_dir():
        print(f"ERROR: {AI_WORKING} not found — run from repository root", file=sys.stderr)
        return 1

    run_compact = args.compact or args.all
    run_archive = args.archive or args.all
    run_cleanup = args.cleanup or args.all

    prefix = "[DRY-RUN] " if args.dry_run else ""
    print(f"{TOOL_NAME} v{TOOL_VERSION}  {prefix}(repo: {Path('.').resolve()})")
    print(f"  modes: compact={run_compact} archive={run_archive} cleanup={run_cleanup}")
    print(f"  archive-days={args.archive_days}  cleanup-days={args.cleanup_days}")
    print()

    stats = Stats()

    if run_compact:
        print("=== compact ===")
        compact(args.dry_run, stats)
        print()

    if run_archive:
        print("=== archive ===")
        archive(args.archive_days, args.dry_run, stats)
        print()

    if run_cleanup:
        print("=== cleanup ===")
        cleanup(args.cleanup_days, args.dry_run, stats)
        print()

    print("=== summary ===")
    print(f"  topics compacted : {stats.topics}")
    print(f"  compact pages    : {stats.compacted}")
    print(f"  files archived   : {stats.archived}")
    print(f"  files removed    : {stats.removed}")

    # Write JSON summary for CI consumption
    summary = {
        "tool": TOOL_NAME,
        "version": TOOL_VERSION,
        "date": datetime.now(UTC).isoformat(),
        "dry_run": args.dry_run,
        "stats": {
            "topics": stats.topics,
            "compact_pages_updated": stats.compacted,
            "files_archived": stats.archived,
            "files_removed": stats.removed,
        },
    }
    summary_path = AI_WORKING / "compact" / "COMPACT_RUN_SUMMARY.json"
    if not args.dry_run:
        summary_path.parent.mkdir(parents=True, exist_ok=True)
        summary_path.write_text(json.dumps(summary, indent=2), encoding="utf-8")
        print(f"  summary written  : {summary_path}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
