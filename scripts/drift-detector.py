"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            drift-detector.py                                  ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-04-15 07:11:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     448                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 72315421ac  2026-03-12  docs: fix audit gaps — validate-docs.sh, Wiki/Archive str... ║
    • 1f04e03bd3  2026-03-11  docs: implement documentation system infrastructure (Phas... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
ThemisDB Documentation Drift Detector

Compares primary documentation (src/, include/, examples/) with secondary docs
(docs/de/, docs/en/, …) to detect potential staleness.

A secondary doc is considered "potentially drifting" when its last-modified
timestamp is significantly older than the corresponding primary source listed
in the file's **Primary (Quelle der Wahrheit):** header field.

Usage:
  python3 scripts/drift-detector.py [options]

Options:
  --primary-index PATH    Primary index file
                          (default: docs/_generated/primary_index.json)
  --docs-dirs DIRS        Comma-separated secondary docs directories
                          (default: docs/de,docs/en)
  --repo-root PATH        Repository root (auto-detected if omitted)
  --drifting-days N       Days before a doc is flagged as 'drifting' (default: 90)
  --stale-days N          Days before a doc is flagged as 'stale' (default: 180)
  --format text|json      Output format (default: text)
  --fail-on-drift         Exit 1 when any drift is detected
  --output PATH           Write report to this file instead of stdout

Exit codes:
  0 = no actionable drift (or --fail-on-drift not set)
  1 = drift detected when --fail-on-drift is set
"""

import argparse
import json
import os
import re
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, List, Optional, Tuple

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

DEFAULT_PRIMARY_INDEX = "docs/_generated/primary_index.json"
DEFAULT_DOCS_DIRS = ["docs/de", "docs/en"]
DEFAULT_DRIFTING_DAYS = 90
DEFAULT_STALE_DAYS = 180

# Regex to extract primary source references from the header block.
# Matches backtick-wrapped paths like `src/foo/README.md`
PRIMARY_REF_RE = re.compile(r"`((?:src|include|examples)/[^`]+)`")

# Regex to detect **Primary …:** header field
PRIMARY_FIELD_RE = re.compile(r"\*\*Primary[^:]*:\*\*", re.IGNORECASE)

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def find_repo_root() -> Path:
    """Locate repository root via git, falling back to script's parent."""
    try:
        result = subprocess.run(
            ["git", "rev-parse", "--show-toplevel"],
            capture_output=True,
            text=True,
            check=True,
        )
        return Path(result.stdout.strip())
    except Exception:
        return Path(__file__).resolve().parent.parent


def _parse_iso(ts: str) -> Optional[datetime]:
    """Parse an ISO-8601 timestamp to an aware datetime (UTC)."""
    if not ts:
        return None
    ts = ts.rstrip("Z")
    for fmt in ("%Y-%m-%dT%H:%M:%S", "%Y-%m-%dT%H:%M", "%Y-%m-%d"):
        try:
            return datetime.strptime(ts, fmt).replace(tzinfo=timezone.utc)
        except ValueError:
            continue
    return None


def get_file_mtime(file_path: Path, repo_root: Path) -> Optional[datetime]:
    """Return the last-modified datetime for a file (git-based, then filesystem)."""
    try:
        result = subprocess.run(
            ["git", "log", "-1", "--format=%cI", "--", str(file_path)],
            capture_output=True,
            text=True,
            cwd=str(repo_root),
        )
        if result.returncode == 0 and result.stdout.strip():
            return _parse_iso(result.stdout.strip())
    except Exception:
        pass

    # Fallback: filesystem mtime
    try:
        mtime = file_path.stat().st_mtime
        return datetime.fromtimestamp(mtime, tz=timezone.utc)
    except Exception:
        return None


# ---------------------------------------------------------------------------
# Primary index
# ---------------------------------------------------------------------------


def load_primary_index(index_path: Path) -> Dict[str, dict]:
    """
    Load primary_index.json and return a dict keyed by relative file path.
    Returns an empty dict when the file does not exist or cannot be parsed.
    """
    if not index_path.exists():
        return {}
    try:
        with open(index_path, "r", encoding="utf-8") as fh:
            data = json.load(fh)
        entries = data.get("entries", data) if isinstance(data, dict) else data
        if isinstance(entries, list):
            return {e["path"]: e for e in entries if "path" in e}
        return {}
    except Exception:
        return {}


# ---------------------------------------------------------------------------
# Secondary-doc scanning
# ---------------------------------------------------------------------------


def extract_primary_refs(file_path: Path, max_lines: int = 40) -> List[str]:
    """Extract primary source references from the doc header."""
    try:
        with open(file_path, "r", encoding="utf-8") as fh:
            content = "".join(fh.readline() for _ in range(max_lines))
    except Exception:
        return []

    if not PRIMARY_FIELD_RE.search(content):
        return []

    return PRIMARY_REF_RE.findall(content)


def scan_secondary_docs(docs_dirs: List[Path]) -> List[Path]:
    """Return all .md files under the given secondary docs directories."""
    files: List[Path] = []
    for d in docs_dirs:
        if d.exists():
            files.extend(sorted(d.rglob("*.md")))
    return files


# ---------------------------------------------------------------------------
# Drift analysis
# ---------------------------------------------------------------------------

DriftEntry = Dict  # keys: file, primary_refs, secondary_mtime, max_primary_mtime, age_days, status


def analyse_drift(
    secondary_files: List[Path],
    primary_index: Dict[str, dict],
    repo_root: Path,
    drifting_days: int,
    stale_days: int,
) -> List[DriftEntry]:
    """
    For each secondary doc that references at least one primary source,
    compare timestamps and determine drift status.

    Returns list of DriftEntry dicts for files with detected drift.
    """
    results: List[DriftEntry] = []

    for sec_path in secondary_files:
        primary_refs = extract_primary_refs(sec_path)
        if not primary_refs:
            continue  # No primary reference → skip

        # Resolve primary mtimes
        primary_mtimes: List[datetime] = []
        for ref in primary_refs:
            entry = primary_index.get(ref)
            if entry and entry.get("last_modified"):
                dt = _parse_iso(entry["last_modified"])
                if dt:
                    primary_mtimes.append(dt)
            else:
                # Try direct filesystem lookup
                p = repo_root / ref
                if p.exists():
                    dt = get_file_mtime(p, repo_root)
                    if dt:
                        primary_mtimes.append(dt)

        if not primary_mtimes:
            continue  # Cannot determine primary mtime → skip

        max_primary_mtime = max(primary_mtimes)
        sec_mtime = get_file_mtime(sec_path, repo_root)
        if sec_mtime is None:
            continue

        # Use total_seconds for sub-day precision, then floor to whole days.
        delta_seconds = (max_primary_mtime - sec_mtime).total_seconds()
        if delta_seconds <= 0:
            continue  # Secondary is at least as recent as primary → no drift

        age_days = int(delta_seconds / 86400)

        if age_days >= stale_days:
            status = "stale"
        elif age_days >= drifting_days:
            status = "drifting"
        else:
            continue  # Within acceptable range

        try:
            rel = sec_path.relative_to(repo_root).as_posix()
        except ValueError:
            rel = str(sec_path)

        results.append(
            {
                "file": rel,
                "primary_refs": primary_refs,
                "secondary_mtime": sec_mtime.strftime("%Y-%m-%dT%H:%M:%SZ"),
                "max_primary_mtime": max_primary_mtime.strftime("%Y-%m-%dT%H:%M:%SZ"),
                "age_days": age_days,
                "status": status,
            }
        )

    return sorted(results, key=lambda e: (-e["age_days"], e["file"]))


# ---------------------------------------------------------------------------
# Output helpers
# ---------------------------------------------------------------------------


def _status_icon(status: str) -> str:
    return {"stale": "🔴", "drifting": "🟡"}.get(status, "⚪")


def build_text_report(
    drift_entries: List[DriftEntry],
    secondary_files_count: int,
    drifting_days: int,
    stale_days: int,
) -> str:
    lines: List[str] = []
    lines.append("ThemisDB Documentation Drift Report")
    lines.append("=" * 60)
    lines.append(f"Secondary docs scanned : {secondary_files_count}")
    lines.append(f"Drifting threshold     : {drifting_days} days")
    lines.append(f"Stale threshold        : {stale_days} days")
    lines.append(f"Drift detected         : {len(drift_entries)} file(s)")
    lines.append("")

    if not drift_entries:
        lines.append("✅ No drift detected.")
        return "\n".join(lines)

    stale_count = sum(1 for e in drift_entries if e["status"] == "stale")
    drifting_count = sum(1 for e in drift_entries if e["status"] == "drifting")
    lines.append(f"  🔴 stale    : {stale_count}")
    lines.append(f"  🟡 drifting : {drifting_count}")
    lines.append("")

    for entry in drift_entries:
        icon = _status_icon(entry["status"])
        lines.append(f"{icon} [{entry['status'].upper():8s}] {entry['file']}")
        lines.append(f"   Age      : {entry['age_days']} days behind primary source")
        lines.append(f"   Primary  : {', '.join(entry['primary_refs'])}")
        lines.append(f"   Sec mtime: {entry['secondary_mtime']}")
        lines.append(f"   Pri mtime: {entry['max_primary_mtime']}")
        lines.append("")

    lines.append("Recommendation:")
    lines.append("  • Review and update the secondary docs listed above.")
    lines.append("  • Set '**Status:** drifting' or '**Status:** stale' in the file header")
    lines.append("    to make the drift visible to readers.")
    return "\n".join(lines)


def build_json_report(
    drift_entries: List[DriftEntry],
    secondary_files_count: int,
    drifting_days: int,
    stale_days: int,
) -> str:
    stale_count = sum(1 for e in drift_entries if e["status"] == "stale")
    drifting_count = sum(1 for e in drift_entries if e["status"] == "drifting")
    payload = {
        "generated_at": datetime.now(tz=timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "secondary_files_scanned": secondary_files_count,
        "drifting_threshold_days": drifting_days,
        "stale_threshold_days": stale_days,
        "drift_count": len(drift_entries),
        "stale_count": stale_count,
        "drifting_count": drifting_count,
        "entries": drift_entries,
    }
    return json.dumps(payload, indent=2)


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------


def main() -> int:
    parser = argparse.ArgumentParser(
        description="ThemisDB Documentation Drift Detector",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--primary-index",
        default=DEFAULT_PRIMARY_INDEX,
        help=f"Path to primary index JSON (default: {DEFAULT_PRIMARY_INDEX}).",
    )
    parser.add_argument(
        "--docs-dirs",
        default=",".join(DEFAULT_DOCS_DIRS),
        help=f"Comma-separated secondary docs directories (default: {','.join(DEFAULT_DOCS_DIRS)}).",
    )
    parser.add_argument(
        "--repo-root",
        default=None,
        help="Repository root directory (auto-detected if omitted).",
    )
    parser.add_argument(
        "--drifting-days",
        type=int,
        default=DEFAULT_DRIFTING_DAYS,
        metavar="N",
        help=f"Days before flagging as 'drifting' (default: {DEFAULT_DRIFTING_DAYS}).",
    )
    parser.add_argument(
        "--stale-days",
        type=int,
        default=DEFAULT_STALE_DAYS,
        metavar="N",
        help=f"Days before flagging as 'stale' (default: {DEFAULT_STALE_DAYS}).",
    )
    parser.add_argument(
        "--format",
        choices=["text", "json"],
        default="text",
        help="Output format (default: text).",
    )
    parser.add_argument(
        "--fail-on-drift",
        action="store_true",
        help="Exit 1 when drift is detected.",
    )
    parser.add_argument(
        "--output",
        default=None,
        metavar="PATH",
        help="Write report to this file instead of stdout.",
    )
    args = parser.parse_args()

    if args.drifting_days >= args.stale_days:
        parser.error("--drifting-days must be less than --stale-days.")

    repo_root = Path(args.repo_root) if args.repo_root else find_repo_root()
    index_path = repo_root / args.primary_index
    docs_dirs = [repo_root / d.strip() for d in args.docs_dirs.split(",") if d.strip()]

    primary_index = load_primary_index(index_path)
    secondary_files = scan_secondary_docs(docs_dirs)

    drift_entries = analyse_drift(
        secondary_files,
        primary_index,
        repo_root,
        args.drifting_days,
        args.stale_days,
    )

    if args.format == "json":
        report = build_json_report(
            drift_entries,
            len(secondary_files),
            args.drifting_days,
            args.stale_days,
        )
    else:
        report = build_text_report(
            drift_entries,
            len(secondary_files),
            args.drifting_days,
            args.stale_days,
        )

    if args.output:
        output_path = Path(args.output)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        with open(output_path, "w", encoding="utf-8") as fh:
            fh.write(report)
        print(f"Drift report written to: {output_path}")
    else:
        print(report)

    if args.fail_on_drift and drift_entries:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
