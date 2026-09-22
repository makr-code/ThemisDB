#!/usr/bin/env python3
"""
scripts/compendium-drift-scan.py
---------------------------------
Detects stale compendium chapters relative to the ThemisDB v2.4.0-alpha
ROADMAP.md and FUTURE_ENHANCEMENTS.md sources, and proposes new chapters
for roadmap topics that have no existing coverage.

Operating model (human-in-the-loop):
  1. This script DETECTS and CLASSIFIES drift; it never rewrites content.
  2. It also evaluates `gap_topics` from CHAPTER_ROADMAP_MAPPING.yml and
     proposes new chapters when roadmap topics have no existing coverage.
  3. Results are written to a JSON report and a human-readable Markdown summary.
  4. The GitHub Actions workflow reads the report and opens/updates tracker issues.
  5. Humans review, assign, and approve chapter update or creation PRs.

Exit codes:
  0  No stale chapters and no new-chapter proposals.
  1  One or more stale chapters detected, or new chapters proposed.
  2  Fatal error (missing files, parse failure).

Usage:
  python3 scripts/compendium-drift-scan.py [OPTIONS]

Options:
  --mapping FILE      Path to CHAPTER_ROADMAP_MAPPING.yml
                      (default: docs/compendium/CHAPTER_ROADMAP_MAPPING.yml)
  --compendium-dir DIR
                      Path to docs/compendium/docs/
                      (default: docs/compendium/docs)
  --roadmap FILE      Path to ROADMAP.md (default: ROADMAP.md)
  --future FILE       Path to FUTURE_ENHANCEMENTS.md
                      (default: FUTURE_ENHANCEMENTS.md)
  --out-json FILE     Write JSON report to FILE (default: /tmp/compendium-drift.json)
  --out-md FILE       Write Markdown summary to FILE
                      (default: /tmp/compendium-drift-summary.md)
  --priority LEVEL    Only report chapters at or above this priority
                      (high|medium|low, default: medium)
  --target-version VER
                      Version string to check against chapter headers
                      (default: 2.4.0-alpha)
  --github-output     Write GITHUB_OUTPUT compatible key=value lines to stdout
  --verbose           Print per-chapter detail to stderr
"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

try:
    import yaml
except ImportError:
    print("ERROR: PyYAML not installed. Run: pip install pyyaml", file=sys.stderr)
    sys.exit(2)

# ---------------------------------------------------------------------------
# Priority ordering
# ---------------------------------------------------------------------------
PRIORITY_ORDER = {"high": 0, "medium": 1, "low": 2}

# ---------------------------------------------------------------------------
# Staleness heuristics
# ---------------------------------------------------------------------------

# Patterns that indicate a chapter was written for an older release
_OLD_VERSION_RE = re.compile(
    r"(?:Version|Stand|version)\s*[:=]\s*"
    r"(?P<ver>[0-9]+\.[0-9]+[\w.\-]*)"
)

# Markers inside chapter text that hint at missing v2.4.0 content
_V24_KEYWORDS: list[str] = [
    "2.4.0",
    "Wave A",
    "Wave B",
    "Wave C",
    "Wave D",
    "v2.4.0",
    "2.4.0-alpha",
]

# Version strings that are clearly older than 2.4.0-alpha
_STALE_VERSION_PATTERN = re.compile(
    r"^(?:1\.|2\.[0-3]\.|2\.4\.0-?(?:dev|rc(?:[0-2])|beta|draft|old))",
    re.IGNORECASE,
)


def _is_stale_version(ver: str) -> bool:
    """Return True if ver is clearly older than 2.4.0-alpha."""
    return bool(_STALE_VERSION_PATTERN.match(ver.strip()))


def _chapter_has_v24_signals(text: str) -> bool:
    """Return True if the chapter already references v2.4.0 content."""
    return any(kw.lower() in text.lower() for kw in _V24_KEYWORDS)


def _extract_versions(text: str) -> list[str]:
    return _OLD_VERSION_RE.findall(text)


# ---------------------------------------------------------------------------
# Load helpers
# ---------------------------------------------------------------------------

def _load_yaml(path: Path) -> Any:
    with path.open("r", encoding="utf-8") as fh:
        return yaml.safe_load(fh)


def _load_text(path: Path) -> str:
    with path.open("r", encoding="utf-8") as fh:
        return fh.read()


# ---------------------------------------------------------------------------
# SOT freshness cache
# ---------------------------------------------------------------------------

def _build_sot_cache(
    repo_root: Path,
    sot_paths_all: list[str],
) -> dict[str, str]:
    """Load SOT file texts once for keyword presence checks."""
    cache: dict[str, str] = {}
    for rel in sot_paths_all:
        p = repo_root / rel
        if p.exists():
            try:
                cache[rel] = _load_text(p)
            except OSError:
                cache[rel] = ""
    return cache


def _sot_mentions_topics(
    sot_cache: dict[str, str],
    sot_paths: list[str],
    topics: list[str],
) -> list[str]:
    """Return topics that appear in at least one SOT file."""
    found: list[str] = []
    for topic in topics:
        for rel in sot_paths:
            text = sot_cache.get(rel, "")
            if topic.lower() in text.lower():
                found.append(topic)
                break
    return found


# ---------------------------------------------------------------------------
# Scan logic — existing chapters
# ---------------------------------------------------------------------------

def scan_chapter(
    entry: dict,
    compendium_dir: Path,
    sot_cache: dict[str, str],
    target_version: str,
    verbose: bool = False,
) -> dict:
    """Evaluate a single chapter entry; return a result dict."""
    fname = entry["file"]
    file_path = compendium_dir / fname
    result: dict = {
        "file": fname,
        "title": entry.get("title", fname),
        "chapter_number": entry.get("chapter_number", ""),
        "priority": entry.get("priority", "medium"),
        "sot_paths": entry.get("sot_paths", []),
        "roadmap_topics": entry.get("roadmap_topics", []),
        "exists": False,
        "stale_versions_found": [],
        "has_target_version_signal": False,
        "active_sot_topics": [],
        "reasons": [],
        "verdict": "ok",  # ok | stale | missing
    }

    if not file_path.exists():
        result["exists"] = False
        result["verdict"] = "missing"
        result["reasons"].append(f"File not found: {file_path}")
        return result

    result["exists"] = True
    text = _load_text(file_path)

    # Check for stale version strings
    versions = _extract_versions(text)
    stale = [v for v in versions if _is_stale_version(v)]
    result["stale_versions_found"] = stale

    # Check for target-version signals
    result["has_target_version_signal"] = _chapter_has_v24_signals(text)

    # Check SOT topic presence
    result["active_sot_topics"] = _sot_mentions_topics(
        sot_cache,
        entry.get("sot_paths", []),
        entry.get("roadmap_topics", []),
    )

    # Derive verdict
    reasons: list[str] = []
    if stale:
        reasons.append(
            f"Outdated version string(s) found: {', '.join(stale)}"
        )
    if not result["has_target_version_signal"]:
        reasons.append(
            f"No {target_version} signals found in chapter text"
        )
    if result["active_sot_topics"]:
        reasons.append(
            f"SOT references these topics not yet integrated: "
            f"{', '.join(result['active_sot_topics'][:5])}"
            + (" ..." if len(result["active_sot_topics"]) > 5 else "")
        )

    if reasons:
        result["verdict"] = "stale"
        result["reasons"] = reasons
    else:
        result["verdict"] = "ok"

    if verbose:
        print(
            f"  [{result['verdict'].upper():7s}] {fname}  "
            f"versions={stale or '-'}  "
            f"v24signal={result['has_target_version_signal']}",
            file=sys.stderr,
        )
    return result


# ---------------------------------------------------------------------------
# Gap topic evaluation — new chapter proposals
# ---------------------------------------------------------------------------

def evaluate_gap_topic(
    gap_entry: dict,
    sot_cache: dict[str, str],
    prio_threshold: int,
    verbose: bool = False,
) -> dict | None:
    """
    Evaluate one gap_topics entry.  Return a proposal dict if the topic is
    active in the SOTs (i.e., it appears in at least one SOT file) and its
    priority passes the threshold — or None if it should be skipped.
    """
    prio = PRIORITY_ORDER.get(gap_entry.get("priority", "medium"), 1)
    if prio > prio_threshold:
        return None

    topic = gap_entry.get("topic", "")
    keywords: list[str] = gap_entry.get("keywords", [])
    sot_paths: list[str] = gap_entry.get("sot_paths", [])

    # Find which keywords appear in the SOT files
    active_keywords = _sot_mentions_topics(sot_cache, sot_paths, keywords)

    if not active_keywords:
        # Topic not yet mentioned in any SOT — no proposal needed
        if verbose:
            print(
                f"  [GAP_SKIP] {topic!r} — no keywords found in SOTs",
                file=sys.stderr,
            )
        return None

    proposal: dict = {
        "topic": topic,
        "priority": gap_entry.get("priority", "medium"),
        "suggested_file": gap_entry.get("suggested_file", ""),
        "rationale": gap_entry.get("rationale", ""),
        "active_keywords": active_keywords,
        "sot_paths": sot_paths,
    }
    if verbose:
        print(
            f"  [PROPOSE ] {topic!r}  → {gap_entry.get('suggested_file', '?')}  "
            f"keywords={active_keywords[:3]}",
            file=sys.stderr,
        )
    return proposal


# ---------------------------------------------------------------------------
# Report generation
# ---------------------------------------------------------------------------

def _write_markdown_summary(
    results: list[dict],
    proposals: list[dict],
    out_path: Path,
    target_version: str,
    run_ts: str,
) -> None:
    stale = [r for r in results if r["verdict"] == "stale"]
    missing = [r for r in results if r["verdict"] == "missing"]
    ok = [r for r in results if r["verdict"] == "ok"]

    lines: list[str] = [
        "# Compendium Drift Scan — Summary",
        "",
        f"**Generated:** {run_ts}  ",
        f"**Target version:** `{target_version}`  ",
        f"**Total chapters scanned:** {len(results)}  ",
        f"**Stale:** {len(stale)}  **Missing:** {len(missing)}  **OK:** {len(ok)}  "
        f"**New chapter proposals:** {len(proposals)}",
        "",
        "---",
        "",
    ]

    if not stale and not missing and not proposals:
        lines += [
            "✅ **No drift detected and no new chapters needed.** "
            "All scanned chapters appear current.",
            "",
        ]
    else:
        if stale:
            lines += [
                "## 🟡 Stale Chapters (need update)",
                "",
                "| Priority | File | Reasons |",
                "|----------|------|---------|",
            ]
            for r in sorted(stale, key=lambda x: PRIORITY_ORDER.get(x["priority"], 9)):
                reasons_short = "; ".join(r["reasons"])[:120]
                lines.append(
                    f"| {r['priority']} | `{r['file']}` | {reasons_short} |"
                )
            lines.append("")

        if missing:
            lines += [
                "## 🔴 Missing Chapter Files",
                "",
            ]
            for r in missing:
                lines.append(f"- `{r['file']}` — {r['title']}")
            lines.append("")

        if proposals:
            lines += [
                "## 🟢 New Chapter Proposals",
                "",
                "These roadmap topics appear in the SOT files but have no dedicated",
                "compendium chapter yet. A maintainer should decide whether to create",
                "a new chapter or extend an existing one.",
                "",
                "| Priority | Suggested File | Topic | Active Keywords |",
                "|----------|----------------|-------|-----------------|",
            ]
            for p in sorted(proposals, key=lambda x: PRIORITY_ORDER.get(x["priority"], 9)):
                kw_short = ", ".join(p["active_keywords"][:4])
                lines.append(
                    f"| {p['priority']} | `{p['suggested_file']}` "
                    f"| {p['topic']} | {kw_short} |"
                )
            lines.append("")
            lines += [
                "### How to action a proposal",
                "",
                "1. Confirm the topic warrants a standalone chapter (vs. extending an existing one).",
                "2. Agree on the filename with the team and update `CHAPTER_ROADMAP_MAPPING.yml`:",
                "   - Move the entry from `gap_topics` into `chapters` with the confirmed `file` name.",
                "3. Create the new chapter file under `docs/compendium/docs/`.",
                "4. Follow the update procedure in `docs/compendium/COMPENDIUM_SYNC_PROCESS.md`.",
                "",
            ]

    lines += [
        "---",
        "",
        "## Next Steps (Human-in-the-Loop)",
        "",
        "1. Review the stale chapters listed above.",
        "2. For each stale chapter, open or update the tracker issue created by the",
        "   `maintenance-compendium-sync` workflow.",
        "3. Assign a reviewer / author and create a draft PR for the chapter update.",
        "4. Review new chapter proposals — confirm, assign, and create draft PRs.",
        "5. Apply changes, request human review, and merge on approval.",
        "",
        "See `docs/compendium/COMPENDIUM_SYNC_PROCESS.md` for the full operating model.",
        "",
    ]

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text("\n".join(lines), encoding="utf-8")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def _parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(
        description=(
            "Detect stale compendium chapters vs ROADMAP/FUTURE_ENHANCEMENTS "
            "and propose new chapters for uncovered roadmap topics"
        )
    )
    p.add_argument(
        "--mapping",
        default="docs/compendium/CHAPTER_ROADMAP_MAPPING.yml",
        help="Path to CHAPTER_ROADMAP_MAPPING.yml",
    )
    p.add_argument(
        "--compendium-dir",
        default="docs/compendium/docs",
        help="Path to compendium docs directory",
    )
    p.add_argument("--roadmap", default="ROADMAP.md")
    p.add_argument("--future", default="FUTURE_ENHANCEMENTS.md")
    p.add_argument("--out-json", default="/tmp/compendium-drift.json")
    p.add_argument("--out-md", default="/tmp/compendium-drift-summary.md")
    p.add_argument(
        "--priority",
        default="medium",
        choices=["high", "medium", "low"],
        help="Minimum priority level to report (chapters and gap topics)",
    )
    p.add_argument(
        "--target-version",
        default="2.4.0-alpha",
        help="Version string to check against chapter headers",
    )
    p.add_argument(
        "--github-output",
        action="store_true",
        help="Emit GITHUB_OUTPUT compatible lines to stdout",
    )
    p.add_argument("--verbose", action="store_true")
    return p.parse_args()


def main() -> int:
    args = _parse_args()
    repo_root = Path.cwd()

    mapping_path = repo_root / args.mapping
    compendium_dir = repo_root / args.compendium_dir
    roadmap_path = repo_root / args.roadmap
    future_path = repo_root / args.future

    # Validate required inputs
    for label, path in [
        ("mapping", mapping_path),
        ("compendium-dir", compendium_dir),
        ("roadmap", roadmap_path),
        ("future", future_path),
    ]:
        if not path.exists():
            print(f"ERROR: {label} not found: {path}", file=sys.stderr)
            return 2

    mapping = _load_yaml(mapping_path)
    target_version = args.target_version

    # Gather all SOT paths referenced in mapping (chapters + gap_topics)
    all_entries: list[dict] = list(mapping.get("chapters", [])) + list(
        mapping.get("appendices", [])
    )
    gap_entries: list[dict] = list(mapping.get("gap_topics", []))
    all_sot_sources = all_entries + gap_entries

    sot_paths_all: list[str] = list(
        {p for entry in all_sot_sources for p in entry.get("sot_paths", [])}
    )
    # Always include root SOT files
    for root_sot in [args.roadmap, args.future]:
        if root_sot not in sot_paths_all:
            sot_paths_all.append(root_sot)

    sot_cache = _build_sot_cache(repo_root, sot_paths_all)

    # Priority filter.
    # PRIORITY_ORDER maps high=0, medium=1, low=2, so a higher numeric value
    # means lower priority. prio_threshold is the maximum numeric value still
    # considered "at or above" the requested minimum priority level.
    prio_threshold = PRIORITY_ORDER.get(args.priority, 1)

    run_ts = datetime.now(tz=timezone.utc).strftime("%Y-%m-%d %H:%M UTC")
    results: list[dict] = []
    proposals: list[dict] = []

    if args.verbose:
        print(
            f"Scanning {len(all_entries)} chapter entries, "
            f"{len(gap_entries)} gap-topic entries ...",
            file=sys.stderr,
        )

    # Scan existing chapters
    for entry in all_entries:
        prio = PRIORITY_ORDER.get(entry.get("priority", "medium"), 1)
        if prio > prio_threshold:
            continue
        r = scan_chapter(
            entry,
            compendium_dir,
            sot_cache,
            target_version,
            verbose=args.verbose,
        )
        results.append(r)

    # Evaluate gap topics → new-chapter proposals
    if args.verbose:
        print("Evaluating gap topics ...", file=sys.stderr)
    for gap_entry in gap_entries:
        proposal = evaluate_gap_topic(
            gap_entry,
            sot_cache,
            prio_threshold,
            verbose=args.verbose,
        )
        if proposal is not None:
            proposals.append(proposal)

    stale_count = sum(1 for r in results if r["verdict"] == "stale")
    missing_count = sum(1 for r in results if r["verdict"] == "missing")
    proposals_count = len(proposals)
    total_issues = stale_count + missing_count + proposals_count

    # Write JSON report
    report = {
        "generated_at": run_ts,
        "target_version": target_version,
        "total_scanned": len(results),
        "stale_count": stale_count,
        "missing_count": missing_count,
        "ok_count": len(results) - stale_count - missing_count,
        "proposals_count": proposals_count,
        "results": results,
        "proposals": proposals,
    }
    out_json = Path(args.out_json)
    out_json.parent.mkdir(parents=True, exist_ok=True)
    out_json.write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")
    print(f"JSON report written to: {out_json}", file=sys.stderr)

    # Write Markdown summary
    out_md = Path(args.out_md)
    _write_markdown_summary(results, proposals, out_md, target_version, run_ts)
    print(f"Markdown summary written to: {out_md}", file=sys.stderr)

    # GitHub Actions outputs
    if args.github_output:
        github_output = os.environ.get("GITHUB_OUTPUT", "")
        output_lines = [
            f"stale_count={stale_count}",
            f"missing_count={missing_count}",
            f"proposals_count={proposals_count}",
            f"total_issues={total_issues}",
            f"report_json={out_json}",
            f"report_md={out_md}",
        ]
        if github_output:
            with open(github_output, "a", encoding="utf-8") as fh:
                for line in output_lines:
                    fh.write(line + "\n")
        else:
            for line in output_lines:
                print(line)

    # Print summary to stderr
    print(
        f"\nDrift scan complete: {stale_count} stale, "
        f"{missing_count} missing, "
        f"{proposals_count} new-chapter proposals, "
        f"{len(results) - stale_count - missing_count} ok "
        f"(out of {len(results)} chapters scanned, "
        f"priority >= {args.priority})",
        file=sys.stderr,
    )

    return 1 if total_issues > 0 else 0


if __name__ == "__main__":
    sys.exit(main())
