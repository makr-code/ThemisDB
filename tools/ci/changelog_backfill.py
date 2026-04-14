"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            changelog_backfill.py                              ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-04-14 19:10:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     466                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03973ea1c5  2026-04-14  feat: shared CHANGELOG workflow + milestone-based histori... ║
    • 86745ceec2  2026-04-14  feat: shared CHANGELOG workflow + milestone-based histori... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
ThemisDB Changelog Backfill
============================

Reconstructs CHANGELOG.md history by reading GitHub milestones and their
associated pull requests via the GitHub REST API.

Milestones are treated as version numbers (e.g. milestone "v1.8.0" or
"1.8.0" → CHANGELOG section ``## [1.8.0]``).  For each milestone the script
groups closed PRs by their label category and calls ``changelog_updater.py``
to insert one entry per category into the correct versioned block.

Label → CHANGELOG section mapping (customisable via --label-map JSON):
  feat / enhancement / feature  → Added
  fix / bug / bugfix            → Fixed
  security                      → Security
  docs / documentation          → Documentation
  perf / performance            → Performance
  refactor / chore              → Changed
  deprecat*                     → Deprecated
  remov* / break*               → Removed
  infra / infrastructure / ci   → Infrastructure
  (unrecognised)                → Changed

Usage
-----
    python3 tools/ci/changelog_backfill.py \\
        --repo    makr-code/ThemisDB \\
        [--milestone 1.8.0]          \\  # process only this milestone (omit = all)
        [--since  2025-12-01]        \\  # skip milestones closed before this date
        [--until  2026-04-14]        \\  # skip milestones closed after this date
        [--label-map '{"hotfix":"Fixed"}']  # extend default map
        [--dry-run]
        [--quiet]

Requirements
------------
  * ``gh`` CLI authenticated (GITHUB_TOKEN env var or ``gh auth login``)
  * Python 3.9+

Exit codes
----------
  0  All milestones processed successfully.
  1  Fatal error (auth failure, invalid CHANGELOG.md, …).
  2  Partial failure — some milestones had no matching PRs.
"""

from __future__ import annotations

import argparse
import datetime
import json
import re
import subprocess
import sys
from pathlib import Path
from typing import Dict, List, Optional, Tuple

# ---------------------------------------------------------------------------
# Default label → CHANGELOG section mapping
# ---------------------------------------------------------------------------

DEFAULT_LABEL_MAP: Dict[str, str] = {
    # Added
    "feat":           "Added",
    "feature":        "Added",
    "enhancement":    "Added",
    "new":            "Added",
    # Fixed
    "fix":            "Fixed",
    "bug":            "Fixed",
    "bugfix":         "Fixed",
    "hotfix":         "Fixed",
    # Security
    "security":       "Security",
    "cve":            "Security",
    "vulnerability":  "Security",
    # Documentation
    "docs":           "Documentation",
    "documentation":  "Documentation",
    # Performance
    "perf":           "Performance",
    "performance":    "Performance",
    "benchmark":      "Performance",
    # Changed / Refactor
    "refactor":       "Changed",
    "chore":          "Changed",
    "maintenance":    "Changed",
    "improvement":    "Changed",
    # Deprecated
    "deprecate":      "Deprecated",
    "deprecated":     "Deprecated",
    "deprecation":    "Deprecated",
    # Removed
    "remove":         "Removed",
    "removed":        "Removed",
    "breaking":       "Removed",
    "breaking-change":"Removed",
    # Infrastructure / CI
    "infra":          "Infrastructure",
    "infrastructure": "Infrastructure",
    "ci":             "Infrastructure",
    "cd":             "Infrastructure",
    "devops":         "Infrastructure",
    "build":          "Infrastructure",
}

_FALLBACK_SECTION = "Changed"

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _find_repo_root() -> Path:
    here = Path(__file__).resolve()
    return here.parent.parent.parent


def _run_gh(args: List[str], *, quiet: bool = False) -> dict | list:
    """Run ``gh api`` and return parsed JSON.  Raises on non-zero exit."""
    cmd = ["gh", "api", "--paginate"] + args
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        if not quiet:
            print(f"  ERROR: gh command failed: {' '.join(cmd)}", file=sys.stderr)
            print(f"  stderr: {result.stderr.strip()}", file=sys.stderr)
        raise RuntimeError(result.stderr.strip())
    # gh --paginate may output multiple JSON arrays concatenated — parse robustly.
    raw = result.stdout.strip()
    if not raw:
        return []
    try:
        return json.loads(raw)
    except json.JSONDecodeError:
        # Concatenated arrays: "[…][…]" → merge into one list.
        items: list = []
        decoder = json.JSONDecoder()
        pos = 0
        while pos < len(raw):
            obj, pos = decoder.raw_decode(raw, pos)
            if isinstance(obj, list):
                items.extend(obj)
            else:
                items.append(obj)
            pos = max(pos, raw.find("[", pos))
            if pos == -1:
                break
        return items


def _normalize_version(title: str) -> str:
    """Strip leading 'v' from a milestone title to get a bare version number."""
    return title.lstrip("v").strip()


def _milestone_date(ms: dict) -> str:
    """Return ISO date for a milestone: closed_at → due_on → today."""
    for field in ("closed_at", "due_on"):
        val = ms.get(field)
        if val:
            return val[:10]  # YYYY-MM-DD
    return datetime.date.today().strftime("%Y-%m-%d")


def _label_to_section(labels: List[str], label_map: Dict[str, str]) -> str:
    """Map a list of PR label names to the best matching CHANGELOG section."""
    for label in labels:
        lower = label.lower()
        if lower in label_map:
            return label_map[lower]
        # prefix match (e.g. "feat: …" or "feat/…")
        for key, section in label_map.items():
            if lower.startswith(key):
                return section
    return _FALLBACK_SECTION


def _group_prs_by_section(
    prs: List[dict],
    label_map: Dict[str, str],
) -> Dict[str, List[dict]]:
    """Group a list of PR dicts by CHANGELOG section."""
    groups: Dict[str, List[dict]] = {}
    for pr in prs:
        labels = [lbl["name"] for lbl in pr.get("labels", [])]
        section = _label_to_section(labels, label_map)
        groups.setdefault(section, []).append(pr)
    return groups


def _pr_summary_line(pr: dict) -> str:
    """Format a single PR as a changelog body line."""
    number = pr.get("number", "?")
    title = pr.get("title", "").strip()
    user = pr.get("user", {}).get("login", "")
    user_part = f" (@{user})" if user else ""
    return f"- #{number}: {title}{user_part}"

# ---------------------------------------------------------------------------
# Core processing
# ---------------------------------------------------------------------------

def process_milestone(
    ms: dict,
    repo: str,
    label_map: Dict[str, str],
    changelog_path: Path,
    dry_run: bool,
    quiet: bool,
) -> Tuple[int, int]:
    """Process a single milestone. Returns (sections_written, prs_processed)."""
    version = _normalize_version(ms["title"])
    release_date = _milestone_date(ms)
    ms_number = ms["number"]

    if not quiet:
        state = ms.get("state", "?")
        print(f"\n── Milestone {ms['title']!r} (#{ms_number}, {state}) → v{version}  [{release_date}]")

    # Fetch all closed PRs in this milestone.
    try:
        prs = _run_gh([
            f"repos/{repo}/issues",
            "--field", "state=closed",
            "--field", "milestone=" + str(ms_number),
            "--field", f"per_page=100",
        ], quiet=quiet)
    except RuntimeError:
        if not quiet:
            print(f"  WARN: could not fetch PRs for milestone #{ms_number} — skipping.")
        return 0, 0

    # GitHub issues endpoint returns both issues and PRs; keep only PRs.
    prs = [p for p in prs if p.get("pull_request")]

    if not prs:
        if not quiet:
            print("  No merged PRs found — skipping.")
        return 0, 0

    if not quiet:
        print(f"  {len(prs)} PR(s) found.")

    groups = _group_prs_by_section(prs, label_map)
    sections_written = 0

    for section, section_prs in sorted(groups.items()):
        # Build a title and body for this section group.
        entry_title = f"v{version} — {section} ({len(section_prs)} PR{'s' if len(section_prs) != 1 else ''})"
        body_lines = [_pr_summary_line(pr) for pr in section_prs]
        idempotency_key = f"backfill-{version}-{section.lower()}"

        updater_args = [
            sys.executable,
            str(Path(__file__).parent / "changelog_updater.py"),
            "--changelog",       str(changelog_path),
            "--target-version",  version,
            "--release-date",    release_date,
            "--section",         section,
            "--entry-title",     entry_title,
            "--key",             idempotency_key,
        ]
        for line in body_lines:
            updater_args += ["--entry-body", line]
        if dry_run:
            updater_args.append("--dry-run")

        result = subprocess.run(updater_args, capture_output=True, text=True)
        output = (result.stdout + result.stderr).strip()

        if not quiet:
            prefix = "  [dry-run] " if dry_run else "  "
            if "already contains entry" in output:
                print(f"{prefix}Section '{section}': already up-to-date (skipped).")
            elif result.returncode == 0:
                print(f"{prefix}Section '{section}': {len(section_prs)} PR(s) written.")
                sections_written += 1
            else:
                print(f"{prefix}Section '{section}': ERROR — {output}")

        if result.returncode != 0 and not dry_run:
            print(f"  ERROR in changelog_updater.py: {output}", file=sys.stderr)

    return sections_written, len(prs)

# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def build_arg_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description=(
            "Reconstruct CHANGELOG.md history from GitHub milestones + PRs.\n"
            "Each milestone title is treated as a version number."
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    p.add_argument(
        "--repo",
        required=True,
        metavar="OWNER/REPO",
        help='GitHub repository (e.g. "makr-code/ThemisDB")',
    )
    p.add_argument(
        "--milestone",
        default="",
        metavar="VERSION",
        help=(
            "Process only this milestone/version (e.g. '1.8.0' or 'v1.8.0'). "
            "Omit to process all milestones."
        ),
    )
    p.add_argument(
        "--since",
        default="",
        metavar="YYYY-MM-DD",
        help="Skip milestones whose release date is before this date.",
    )
    p.add_argument(
        "--until",
        default="",
        metavar="YYYY-MM-DD",
        help="Skip milestones whose release date is after this date.",
    )
    p.add_argument(
        "--state",
        default="all",
        choices=["open", "closed", "all"],
        help="Milestone state filter (default: all).",
    )
    p.add_argument(
        "--label-map",
        default="{}",
        metavar="JSON",
        help=(
            "JSON object extending the default label→section mapping. "
            'Example: \'{"hotfix":"Fixed","perf-fix":"Performance"}\''
        ),
    )
    p.add_argument(
        "--changelog",
        default="",
        metavar="PATH",
        help="Path to CHANGELOG.md (default: repo-root/CHANGELOG.md).",
    )
    p.add_argument(
        "--dry-run",
        action="store_true",
        help="Show what would be written without modifying CHANGELOG.md.",
    )
    p.add_argument("--quiet", action="store_true", help="Suppress progress output.")
    return p


def main(argv=None) -> int:
    parser = build_arg_parser()
    args = parser.parse_args(argv)

    # Resolve CHANGELOG.md.
    if args.changelog:
        changelog_path = Path(args.changelog).resolve()
    else:
        changelog_path = _find_repo_root() / "CHANGELOG.md"

    if not changelog_path.exists():
        print(f"ERROR: CHANGELOG.md not found: {changelog_path}", file=sys.stderr)
        return 1

    # Parse extra label map.
    try:
        extra_map: Dict[str, str] = json.loads(args.label_map)
    except json.JSONDecodeError as exc:
        print(f"ERROR: --label-map is not valid JSON: {exc}", file=sys.stderr)
        return 1
    label_map = {**DEFAULT_LABEL_MAP, **{k.lower(): v for k, v in extra_map.items()}}

    # Fetch milestones from GitHub.
    if not args.quiet:
        print(f"Fetching milestones from {args.repo} (state={args.state}) …")
    try:
        milestones = _run_gh([
            f"repos/{args.repo}/milestones",
            "--field", f"state={args.state}",
            "--field", "per_page=100",
            "--field", "sort=due_on",
            "--field", "direction=asc",
        ], quiet=args.quiet)
    except RuntimeError as exc:
        print(f"ERROR: Failed to fetch milestones — {exc}", file=sys.stderr)
        print("Ensure GH_TOKEN / GITHUB_TOKEN is set and `gh` CLI is available.", file=sys.stderr)
        return 1

    if not milestones:
        if not args.quiet:
            print("No milestones found.")
        return 2

    # Apply filters.
    target_ver = _normalize_version(args.milestone) if args.milestone else ""
    filtered: List[dict] = []
    for ms in milestones:
        ver = _normalize_version(ms["title"])
        if target_ver and ver != target_ver:
            continue
        date = _milestone_date(ms)
        if args.since and date < args.since:
            continue
        if args.until and date > args.until:
            continue
        filtered.append(ms)

    if not filtered:
        if not args.quiet:
            print("No milestones matched the given filters.")
        return 2

    if not args.quiet:
        print(f"{len(filtered)} milestone(s) to process.")

    total_sections = 0
    total_prs = 0

    for ms in filtered:
        s, p = process_milestone(
            ms,
            repo=args.repo,
            label_map=label_map,
            changelog_path=changelog_path,
            dry_run=args.dry_run,
            quiet=args.quiet,
        )
        total_sections += s
        total_prs += p

    if not args.quiet:
        mode = " (dry-run)" if args.dry_run else ""
        print(f"\n✅ Done{mode}: {total_sections} section(s) written, {total_prs} PR(s) processed across {len(filtered)} milestone(s).")

    return 0


if __name__ == "__main__":
    sys.exit(main())
