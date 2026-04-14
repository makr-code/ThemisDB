"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            sync-milestones-from-roadmap.py                    ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-14 11:31:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     646                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • cf9e50bfc5  2026-02-24  fix: correct 403 rate-limit detection; add CI workflows f... ║
    • 9d86e07e25  2026-02-24  feat: add milestone sync script, audit report, and tests ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
sync-milestones-from-roadmap.py
================================
ThemisDB – Milestone Sync from Roadmap Files

Parses every ``src/**/ROADMAP.md`` file, extracts ``(Target: <milestone>)``
and ``(Issue: #NNN)`` annotations, then (optionally) creates the corresponding
GitHub milestones and assigns the referenced open issues to them.

Usage
-----
::

    # Preview what would be done (no writes)
    python scripts/sync-milestones-from-roadmap.py --dry-run

    # Apply changes (requires GITHUB_TOKEN with issues:write)
    GITHUB_TOKEN=ghp_... python scripts/sync-milestones-from-roadmap.py

    # Only report – generate audit file and exit
    python scripts/sync-milestones-from-roadmap.py --audit-only

    # Specify a different repository
    python scripts/sync-milestones-from-roadmap.py --repo owner/name

Environment variables
---------------------
GITHUB_TOKEN
    Personal Access Token (or GITHUB_TOKEN in Actions) with at minimum
    ``repo`` scope (issues:write + milestones:write).

GITHUB_REPOSITORY
    Fallback repository in ``owner/repo`` format when ``--repo`` is not given.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
import time
import urllib.error
import urllib.request
from collections import defaultdict
from pathlib import Path

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

REPO_DEFAULT = "makr-code/ThemisDB"
GITHUB_API = "https://api.github.com"

# Map of quarter labels to ISO due-dates (last day of quarter).
# Used as a fast-path lookup; unknown quarters are resolved dynamically by
# ``_quarter_due_date()``.
QUARTER_DUE_DATES: dict[str, str] = {
    "Q1 2026": "2026-03-31T23:59:59Z",
    "Q2 2026": "2026-06-30T23:59:59Z",
    "Q3 2026": "2026-09-30T23:59:59Z",
    "Q4 2026": "2026-12-31T23:59:59Z",
    "Q1 2027": "2027-03-31T23:59:59Z",
    "Q2 2027": "2027-06-30T23:59:59Z",
    "Q3 2027": "2027-09-30T23:59:59Z",
    "Q4 2027": "2027-12-31T23:59:59Z",
}

# Last day of each calendar quarter (month, day)
_QUARTER_ENDS = {1: (3, 31), 2: (6, 30), 3: (9, 30), 4: (12, 31)}

_QUARTER_LABEL_RE = re.compile(r"^Q([1-4])\s+(\d{4})$")


def _quarter_due_date(label: str) -> str | None:
    """Return the ISO due-date for a quarter label like ``Q3 2026``.

    Falls back to dynamic computation for quarters not in the pre-built table.
    Returns ``None`` for labels that do not match the ``QN YYYY`` pattern.
    """
    if label in QUARTER_DUE_DATES:
        return QUARTER_DUE_DATES[label]
    m = _QUARTER_LABEL_RE.match(label.strip())
    if not m:
        return None
    q, year = int(m.group(1)), int(m.group(2))
    month, day = _QUARTER_ENDS[q]
    return f"{year:04d}-{month:02d}-{day:02d}T23:59:59Z"

# ---------------------------------------------------------------------------
# Regex patterns
# ---------------------------------------------------------------------------

ISSUE_RE = re.compile(r"\(Issue:\s*#(\d+)\)")
TARGET_RE = re.compile(r"\(Target:\s*([^)]+)\)")

# ---------------------------------------------------------------------------
# GitHub API helpers
# ---------------------------------------------------------------------------


class GitHubAPI:
    """Thin wrapper around the GitHub REST API."""

    def __init__(self, token: str, repo: str, dry_run: bool = False) -> None:
        self.token = token
        self.repo = repo
        self.dry_run = dry_run
        self._rate_limit_remaining = 5000
        self._rate_limit_reset = 0

    def _headers(self) -> dict[str, str]:
        return {
            "Authorization": f"Bearer {self.token}",
            "Accept": "application/vnd.github+json",
            "X-GitHub-Api-Version": "2022-11-28",
            "Content-Type": "application/json",
        }

    def _request(
        self,
        method: str,
        path: str,
        body: dict | None = None,
        *,
        retry: int = 3,
    ) -> dict | list | None:
        url = f"{GITHUB_API}{path}"
        data = json.dumps(body).encode() if body else None
        req = urllib.request.Request(url, data=data, headers=self._headers(), method=method)
        for attempt in range(retry):
            try:
                with urllib.request.urlopen(req, timeout=30) as resp:
                    remaining = resp.headers.get("X-RateLimit-Remaining", "5000")
                    reset = resp.headers.get("X-RateLimit-Reset", "0")
                    self._rate_limit_remaining = int(remaining)
                    self._rate_limit_reset = int(reset)
                    if resp.status == 204:
                        return None
                    return json.loads(resp.read())
            except urllib.error.HTTPError as exc:
                # Read the body once so we can inspect it below.
                try:
                    body_text = exc.read().decode(errors="replace")
                except Exception:
                    body_text = ""

                if exc.code == 403:
                    # GitHub sends rate-limit 403s with X-RateLimit-Remaining: 0
                    # and a JSON body containing "rate limit" or "secondary rate".
                    remaining_hdr = int(exc.headers.get("X-RateLimit-Remaining", "1") if exc.headers else "1")
                    is_rate_limit = (
                        remaining_hdr == 0
                        or "rate limit" in body_text.lower()
                        or "secondary rate" in body_text.lower()
                    )
                    if is_rate_limit and attempt < retry - 1:
                        reset_hdr = exc.headers.get("X-RateLimit-Reset", "0") if exc.headers else "0"
                        wait = max(0, int(reset_hdr) - int(time.time())) + 5
                        print(f"  ⏳ Rate-limited. Waiting {wait}s …", file=sys.stderr)
                        time.sleep(wait)
                    else:
                        print(
                            f"  ❌ HTTP 403 {method} {path}: {body_text[:300]}",
                            file=sys.stderr,
                        )
                        return None
                elif exc.code == 422:
                    # Unprocessable – e.g. milestone already exists
                    print(f"  ⚠️  422 Unprocessable for {method} {path}: {body_text[:200]}")
                    return None
                elif exc.code in (502, 503, 504) and attempt < retry - 1:
                    time.sleep(2 ** attempt)
                else:
                    print(
                        f"  ❌ HTTP {exc.code} {method} {path}: {body_text[:300]}",
                        file=sys.stderr,
                    )
                    return None
            except OSError as exc:
                if attempt < retry - 1:
                    time.sleep(2 ** attempt)
                else:
                    print(f"  ❌ Network error {method} {path}: {exc}", file=sys.stderr)
                    return None
        return None

    # ---- Milestones --------------------------------------------------------

    def list_milestones(self, state: str = "all") -> list[dict]:
        """Return all milestones (paginates automatically)."""
        milestones: list[dict] = []
        page = 1
        while True:
            batch = self._request(
                "GET",
                f"/repos/{self.repo}/milestones?state={state}&per_page=100&page={page}",
            )
            if not batch:
                break
            milestones.extend(batch)  # type: ignore[arg-type]
            if len(batch) < 100:  # type: ignore[arg-type]
                break
            page += 1
        return milestones

    def create_milestone(self, title: str, due_on: str | None = None) -> dict | None:
        """Create a milestone and return the API object, or None on failure."""
        if self.dry_run:
            print(f"  [dry-run] Would create milestone: '{title}'" + (f" (due {due_on})" if due_on else ""))
            return {"number": -1, "title": title, "html_url": "(dry-run)"}
        payload: dict = {"title": title, "state": "open"}
        if due_on:
            payload["due_on"] = due_on
        result = self._request("POST", f"/repos/{self.repo}/milestones", payload)
        return result  # type: ignore[return-value]

    # ---- Issues ------------------------------------------------------------

    def get_issue(self, number: int) -> dict | None:
        return self._request("GET", f"/repos/{self.repo}/issues/{number}")  # type: ignore[return-value]

    def set_issue_milestone(self, issue_number: int, milestone_number: int) -> bool:
        """Assign a milestone to an issue.  Returns True on success."""
        if self.dry_run:
            print(f"  [dry-run] Would assign issue #{issue_number} → milestone #{milestone_number}")
            return True
        result = self._request(
            "PATCH",
            f"/repos/{self.repo}/issues/{issue_number}",
            {"milestone": milestone_number},
        )
        return result is not None


# ---------------------------------------------------------------------------
# Roadmap parsing
# ---------------------------------------------------------------------------


def parse_roadmaps(src_root: Path) -> dict[int, dict]:
    """
    Walk ``src_root`` for ROADMAP.md files and return a dict mapping
    issue_number → { 'milestone': str, 'roadmap': str, 'line': str }.

    Only lines that contain **both** an ``(Issue: #NNN)`` ref **and**
    a ``(Target: …)`` annotation are included.
    """
    repo_root = src_root.parent
    mapping: dict[int, dict] = {}
    for roadmap_path in sorted(src_root.rglob("ROADMAP.md")):
        rel = roadmap_path.relative_to(repo_root)
        with roadmap_path.open(encoding="utf-8", errors="replace") as fh:
            for line in fh:
                issue_hits = ISSUE_RE.findall(line)
                target_hit = TARGET_RE.search(line)
                if issue_hits and target_hit:
                    milestone_name = target_hit.group(1).strip()
                    for iss_str in issue_hits:
                        iss_num = int(iss_str)
                        # First occurrence wins
                        if iss_num not in mapping:
                            mapping[iss_num] = {
                                "milestone": milestone_name,
                                "roadmap": str(rel),
                                "line": line.strip(),
                            }
    return mapping


def parse_all_issue_refs(src_root: Path) -> dict[int, dict]:
    """
    Return ALL ``(Issue: #NNN)`` references found in roadmaps, including those
    without a ``(Target: …)`` annotation.  Used for the audit report.
    """
    repo_root = src_root.parent
    refs: dict[int, dict] = {}
    for roadmap_path in sorted(src_root.rglob("ROADMAP.md")):
        rel = roadmap_path.relative_to(repo_root)
        with roadmap_path.open(encoding="utf-8", errors="replace") as fh:
            for line in fh:
                issue_hits = ISSUE_RE.findall(line)
                target_hit = TARGET_RE.search(line)
                if issue_hits:
                    for iss_str in issue_hits:
                        iss_num = int(iss_str)
                        refs.setdefault(
                            iss_num,
                            {
                                "milestone": target_hit.group(1).strip() if target_hit else None,
                                "roadmap": str(rel),
                                "line": line.strip(),
                            },
                        )
    return refs


# ---------------------------------------------------------------------------
# Audit report generation
# ---------------------------------------------------------------------------


def generate_audit_report(
    src_root: Path,
    all_refs: dict[int, dict],
    mapped: dict[int, dict],
    existing_milestones: list[dict],
    output_path: Path,
) -> None:
    """Write ``docs/issue-milestone-audit.md``."""
    existing_by_title = {m["title"]: m for m in existing_milestones}
    milestones_in_roadmap = sorted({v["milestone"] for v in mapped.values()})

    unmapped = {k: v for k, v in all_refs.items() if k not in mapped}
    by_module_unmapped: dict[str, list[int]] = defaultdict(list)
    for iss, info in sorted(unmapped.items()):
        mod = _module_from_path(info["roadmap"])
        by_module_unmapped[mod].append(iss)

    lines: list[str] = [
        "# Issue–Milestone Audit Report",
        "",
        "> **Auto-generated** by `scripts/sync-milestones-from-roadmap.py`.",
        "> Re-run the script to refresh this file.",
        "",
        "---",
        "",
        "## Summary",
        "",
        f"| Metric | Count |",
        f"|--------|-------|",
        f"| ROADMAP.md files scanned | {len(list(src_root.rglob('ROADMAP.md')))} |",
        f"| Total `(Issue: #NNN)` references | {len(all_refs)} |",
        f"| Issues with explicit `(Target: …)` milestone | {len(mapped)} |",
        f"| Issues **without** explicit milestone | {len(unmapped)} |",
        f"| Milestone labels found in roadmaps | {', '.join(milestones_in_roadmap)} |",
        "",
        "---",
        "",
        "## Milestones Status",
        "",
        "| Milestone | Due Date | GitHub Status |",
        "|-----------|----------|---------------|",
    ]
    for ms_name in milestones_in_roadmap:
        due = _quarter_due_date(ms_name) or "—"
        status = "✅ exists" if ms_name in existing_by_title else "❌ missing – will be created"
        lines.append(f"| {ms_name} | {due[:10] if due != '—' else '—'} | {status} |")

    lines += [
        "",
        "---",
        "",
        "## Issues with Explicit Milestone Mapping",
        "",
        "These issues appear in a roadmap line that contains both `(Issue: #NNN)` and",
        "`(Target: …)`.  The script will assign them to the named milestone.",
        "",
        "| Issue | Milestone | Roadmap |",
        "|-------|-----------|---------|",
    ]
    for iss, info in sorted(mapped.items()):
        rp = info["roadmap"].replace("\\", "/")
        lines.append(f"| #{iss} | {info['milestone']} | `{rp}` |")

    lines += [
        "",
        "---",
        "",
        "## Issues Without Explicit Milestone (Needs Manual Review)",
        "",
        "These issues are referenced in roadmaps but their roadmap lines do **not**",
        "contain a `(Target: …)` annotation.  A maintainer should either add a Target",
        "annotation to the roadmap or assign the milestone manually on GitHub.",
        "",
        "Suggested heuristic: assign to the **earliest open milestone** of the module.",
        "",
    ]
    for mod in sorted(by_module_unmapped.keys()):
        issues = sorted(by_module_unmapped[mod])
        lines.append(f"### Module: `{mod}` ({len(issues)} issues)")
        lines.append("")
        lines.append("| Issue | Suggested Milestone | Roadmap line (excerpt) |")
        lines.append("|-------|---------------------|------------------------|")
        for iss in issues:
            info = unmapped[iss]
            rp = info["roadmap"].replace("\\", "/")
            excerpt = info["line"][:80].replace("|", "\\|")
            # Heuristic: suggest the first milestone that has issues for this module
            mod_milestones = [
                v["milestone"]
                for k, v in mapped.items()
                if _module_from_path(v["roadmap"]) == mod
            ]
            suggestion = mod_milestones[0] if mod_milestones else "—"
            lines.append(f"| #{iss} | {suggestion} | `{excerpt}` |")
        lines.append("")

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"📝 Audit report written to: {output_path}")


def _module_from_path(path: str) -> str:
    """Extract the src sub-module name from a roadmap path (relative or absolute)."""
    parts = Path(path.replace("\\", "/")).parts
    for i, p in enumerate(parts):
        if p == "src" and i + 1 < len(parts):
            return parts[i + 1]
    return "unknown"


# ---------------------------------------------------------------------------
# Main sync logic
# ---------------------------------------------------------------------------


def run(
    *,
    repo: str,
    token: str,
    src_root: Path,
    dry_run: bool,
    audit_only: bool,
    audit_path: Path,
    verbose: bool,
) -> int:
    print("=" * 60)
    print("  ThemisDB – Milestone Sync from Roadmap")
    print("=" * 60)
    print(f"  Repo    : {repo}")
    print(f"  Dry-run : {dry_run}")
    print(f"  Src root: {src_root}")
    print()

    # 1. Parse roadmaps --------------------------------------------------
    print("📖 Parsing roadmap files …")
    mapped = parse_roadmaps(src_root)
    all_refs = parse_all_issue_refs(src_root)

    milestones_needed = sorted({v["milestone"] for v in mapped.values()})
    print(f"  {len(all_refs)} issue references found in roadmaps.")
    print(f"  {len(mapped)} issues have explicit Target milestone.")
    print(f"  Milestones referenced: {milestones_needed}")
    print()

    api = GitHubAPI(token=token, repo=repo, dry_run=dry_run)

    # 2. Fetch existing milestones --------------------------------------
    print("🔍 Fetching existing milestones …")
    existing = api.list_milestones(state="all")
    existing_by_title: dict[str, dict] = {m["title"]: m for m in existing}
    print(f"  {len(existing)} milestones found on GitHub.")
    print()

    if audit_only:
        generate_audit_report(src_root, all_refs, mapped, existing, audit_path)
        return 0

    # 3. Create missing milestones -------------------------------------
    print("🏷️  Ensuring milestones exist …")
    milestone_number_by_name: dict[str, int] = {
        title: m["number"] for title, m in existing_by_title.items()
    }
    created_count = 0
    for ms_name in milestones_needed:
        if ms_name in existing_by_title:
            print(f"  ✅ '{ms_name}' already exists (#{existing_by_title[ms_name]['number']})")
        else:
            due = _quarter_due_date(ms_name)
            new_ms = api.create_milestone(ms_name, due_on=due)
            if new_ms and new_ms.get("number", -1) != -1:
                milestone_number_by_name[ms_name] = new_ms["number"]
                print(f"  ✅ Created '{ms_name}' (#{new_ms['number']})")
                created_count += 1
            elif new_ms and new_ms.get("number") == -1:
                # Dry-run placeholder
                milestone_number_by_name[ms_name] = -1
            else:
                print(f"  ❌ Failed to create '{ms_name}'")
    print()

    # 4. Assign issues to milestones -----------------------------------
    print("🔗 Assigning issues to milestones …")
    assigned = 0
    skipped_already = 0
    skipped_closed = 0
    skipped_error = 0

    for iss_num, info in sorted(mapped.items()):
        ms_name = info["milestone"]
        ms_number = milestone_number_by_name.get(ms_name)
        if ms_number is None:
            print(f"  ⚠️  Skipping #{iss_num}: no milestone number for '{ms_name}'")
            skipped_error += 1
            continue

        # Check current issue state / milestone (skip writes in dry-run)
        if not dry_run:
            issue_data = api.get_issue(iss_num)
            if issue_data is None:
                if verbose:
                    print(f"  ⚠️  #{iss_num}: not found or inaccessible – skipped")
                skipped_error += 1
                continue
            if issue_data.get("state") == "closed":
                if verbose:
                    print(f"  ⏭️  #{iss_num}: already closed – skipped")
                skipped_closed += 1
                continue
            current_ms = issue_data.get("milestone")
            if current_ms and current_ms.get("number") == ms_number:
                if verbose:
                    print(f"  ⏭️  #{iss_num}: already assigned to '{ms_name}' – skipped")
                skipped_already += 1
                continue

        ok = api.set_issue_milestone(iss_num, ms_number)
        if ok:
            if verbose or dry_run:
                print(f"  ✅ #{iss_num} → '{ms_name}'")
            assigned += 1
        else:
            skipped_error += 1

    print()

    # 5. Generate audit report -----------------------------------------
    generate_audit_report(src_root, all_refs, mapped, existing, audit_path)
    print()

    # 6. Summary -------------------------------------------------------
    print("=" * 60)
    print("  Summary")
    print("=" * 60)
    print(f"  Milestones created  : {created_count}")
    print(f"  Issues assigned     : {assigned}")
    print(f"  Already assigned    : {skipped_already}")
    print(f"  Closed (skipped)    : {skipped_closed}")
    print(f"  Errors / not found  : {skipped_error}")
    print()
    return 0


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Sync GitHub milestones from src/**/ROADMAP.md files.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--repo",
        default=os.environ.get("GITHUB_REPOSITORY", REPO_DEFAULT),
        help="GitHub repository in owner/name format (default: %(default)s)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        default=False,
        help="Preview all actions without writing to GitHub",
    )
    parser.add_argument(
        "--audit-only",
        action="store_true",
        default=False,
        help="Only generate the audit report; skip all GitHub API writes",
    )
    parser.add_argument(
        "--audit-output",
        default="docs/issue-milestone-audit.md",
        help="Path for the audit report (default: %(default)s)",
    )
    parser.add_argument(
        "--src-root",
        default="src",
        help="Root directory to scan for ROADMAP.md files (default: %(default)s)",
    )
    parser.add_argument(
        "--verbose",
        "-v",
        action="store_true",
        default=False,
        help="Print one line per issue even when skipped/already assigned",
    )
    args = parser.parse_args(argv)

    token = os.environ.get("GITHUB_TOKEN", "")
    if not token and not args.dry_run and not args.audit_only:
        print(
            "❌ GITHUB_TOKEN is not set.  Export it or use --dry-run / --audit-only.",
            file=sys.stderr,
        )
        return 1

    repo_root = Path(__file__).parent.parent
    src_root = (repo_root / args.src_root).resolve()
    if not src_root.is_dir():
        print(f"❌ src-root not found: {src_root}", file=sys.stderr)
        return 1

    audit_path = (repo_root / args.audit_output).resolve()

    return run(
        repo=args.repo,
        token=token,
        src_root=src_root,
        dry_run=args.dry_run,
        audit_only=args.audit_only,
        audit_path=audit_path,
        verbose=args.verbose,
    )


if __name__ == "__main__":
    sys.exit(main())
