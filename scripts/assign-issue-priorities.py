"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            assign-issue-priorities.py                         ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-04-15 18:07:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     431                                            ║
    • Open Issues:     TODOs: 1, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
assign-issue-priorities.py
===========================
ThemisDB – Retroactive Priority Assignment for GitHub Issues

Reads every open issue that has **no** ``priority:*`` label and assigns one
based on signals extracted from the issue title, body, and existing labels:

Priority rules (first match wins):
  critical  – Phase 1 marker, P0 prefix, ``### Severity: critical``, or
               existing ``priority:critical`` / ``priority:P0`` label
  high      – Short-term (Next 3-6 months) roadmap section, Phase 2–4 marker,
               P1 prefix, ``### Severity: high``, ``type:bug`` label, or
               ``[BUG]`` in title
  medium    – Long-term (6-12 months) roadmap section, Phase 5 marker, or
               default fallback
  low       – ``[Docs-Audit]`` prefix, ``### Severity: low``, or Phase 6 marker

Usage
-----
::

    # Preview (no changes written)
    python scripts/assign-issue-priorities.py --dry-run

    # Apply changes (requires GITHUB_TOKEN with issues:write)
    GITHUB_TOKEN=ghp_... python scripts/assign-issue-priorities.py

    # Also create the priority labels if they are missing
    GITHUB_TOKEN=ghp_... python scripts/assign-issue-priorities.py --create-labels

Environment variables
---------------------
GITHUB_TOKEN
    Personal Access Token with ``repo`` scope (issues:write + labels:write).

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
from typing import Optional

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

REPO_DEFAULT = os.environ.get("GITHUB_REPOSITORY", "makr-code/ThemisDB")
GITHUB_API = "https://api.github.com"

PRIORITY_LABELS: dict[str, dict[str, str]] = {
    "priority:critical": {"color": "dc2626", "description": "Critical priority"},
    "priority:high":     {"color": "f97316", "description": "High priority"},
    "priority:medium":   {"color": "eab308", "description": "Medium priority"},
    "priority:low":      {"color": "6b7280", "description": "Low priority"},
}

# Regex helpers
_SEVERITY_RE      = re.compile(r"###\s*Severity\s*\n+\s*(\w+)", re.IGNORECASE)
_PHASE_RE         = re.compile(r"\bphase\s*([1-6])\b", re.IGNORECASE)
_P_PREFIX_RE      = re.compile(r"\b(P0|P1|P2|P3)\b")
_SHORT_TERM_RE    = re.compile(r"\bshort[- ]term\b|\bnext\s+3[- ]6\s+months?\b", re.IGNORECASE)
_LONG_TERM_RE     = re.compile(r"\blong[- ]term\b|\b6[- ]12\s+months?\b", re.IGNORECASE)


# ---------------------------------------------------------------------------
# GitHub API helper
# ---------------------------------------------------------------------------

class GitHubAPI:
    def __init__(self, token: str, repo: str, dry_run: bool = False) -> None:
        self.token = token
        self.repo = repo
        self.dry_run = dry_run
        self._remaining = 5000

    def _headers(self) -> dict[str, str]:
        return {
            "Authorization": f"Bearer {self.token}",
            "Accept": "application/vnd.github+json",
            "X-GitHub-Api-Version": "2022-11-28",
            "Content-Type": "application/json",
        }

    def _request(self, method: str, path: str, body: Optional[dict] = None) -> object:
        url = f"{GITHUB_API}{path}"
        data = json.dumps(body).encode() if body else None
        req = urllib.request.Request(url, data=data, headers=self._headers(), method=method)
        for attempt in range(3):
            try:
                with urllib.request.urlopen(req, timeout=30) as resp:
                    self._remaining = int(resp.headers.get("X-RateLimit-Remaining", "5000"))
                    if self._remaining < 50:
                        reset = int(resp.headers.get("X-RateLimit-Reset", "0"))
                        wait = max(0, reset - int(time.time())) + 5
                        print(f"⏳ Rate limit low ({self._remaining} left) – sleeping {wait}s")
                        time.sleep(wait)
                    if resp.status == 204:
                        return None
                    return json.loads(resp.read())
            except urllib.error.HTTPError as exc:
                body_text = exc.read().decode(errors="replace")
                if exc.code == 403 and "rate limit" in body_text.lower():
                    time.sleep(60)
                    continue
                raise RuntimeError(f"HTTP {exc.code} on {method} {path}: {body_text}") from exc
        raise RuntimeError(f"Failed after 3 retries: {method} {path}")

    def get(self, path: str) -> object:
        return self._request("GET", path)

    def post(self, path: str, body: dict) -> object:
        return self._request("POST", path, body)

    def patch(self, path: str, body: dict) -> object:
        return self._request("PATCH", path, body)

    # ------------------------------------------------------------------
    def list_open_issues(self) -> list[dict]:
        """Return all open issues (not PRs) regardless of priority label."""
        issues: list[dict] = []
        page = 1
        while True:
            batch = self.get(
                f"/repos/{self.repo}/issues?state=open&per_page=100&page={page}"
            )
            if not batch:
                break
            issues.extend(i for i in batch if "pull_request" not in i)
            if len(batch) < 100:
                break
            page += 1
        return issues

    def add_label(self, issue_number: int, label: str) -> None:
        if self.dry_run:
            return
        self.post(f"/repos/{self.repo}/issues/{issue_number}/labels", {"labels": [label]})

    def ensure_label_exists(self, name: str, color: str, description: str) -> None:
        try:
            self.get(f"/repos/{self.repo}/labels/{urllib.parse.quote(name, safe='')}")
        except RuntimeError:
            # Label likely does not exist; create it
            try:
                self.post(
                    f"/repos/{self.repo}/labels",
                    {"name": name, "color": color, "description": description},
                )
                print(f"  ✅ Created label: {name}")
            except RuntimeError as exc:
                if "already_exists" in str(exc) or "422" in str(exc):
                    pass  # Race or already present
                else:
                    raise


# ---------------------------------------------------------------------------
# Priority inference
# ---------------------------------------------------------------------------

def _label_names(issue: dict) -> set[str]:
    return {lbl["name"].lower() for lbl in issue.get("labels", [])}


def _has_priority(issue: dict) -> bool:
    return any(n.startswith("priority:") for n in _label_names(issue))


def _severity_from_body(body: str) -> Optional[str]:
    """Extract severity keyword from ``### Severity\\n<value>`` block."""
    m = _SEVERITY_RE.search(body or "")
    if m:
        return m.group(1).strip().lower()
    # Also handle inline variant: "### Severity\nmedium" or "Severity: medium"
    inline = re.search(r"\bseverity[:\s]+(\w+)", body or "", re.IGNORECASE)
    if inline:
        return inline.group(1).strip().lower()
    return None


def _phase_from_text(text: str) -> Optional[int]:
    m = _PHASE_RE.search(text or "")
    if m:
        return int(m.group(1))
    return None


def _p_prefix(text: str) -> Optional[str]:
    m = _P_PREFIX_RE.search(text or "")
    if m:
        return m.group(1).upper()
    return None


def _is_short_term(text: str) -> bool:
    """Return True if *text* contains a Short-term (3-6 months) marker."""
    return bool(_SHORT_TERM_RE.search(text or ""))


def _is_long_term(text: str) -> bool:
    """Return True if *text* contains a Long-term (6-12 months) marker."""
    return bool(_LONG_TERM_RE.search(text or ""))


def infer_priority(issue: dict) -> str:
    """Return the ``priority:*`` label name that best fits this issue.

    Decision order (first match wins):
    1. Existing non-standard priority labels → normalise to canonical form.
    2. ``### Severity:`` field in body (critical/blocker → critical, etc.).
    3. P0/P1/P2/P3 prefix in title.
    4. Roadmap phase marker (Phase 1 → critical, 2-4 → high, 5 → medium, 6 → low).
    5. Time-horizon section in body:
       - Short-term (Next 3-6 months) → high
       - Long-term (6-12 months)      → medium
    6. Type label / title heuristics (type:bug → high, [Docs-Audit] → low).
    7. Deferred severity "medium" from step 2.
    8. Default → medium.
    """
    title  = issue.get("title", "") or ""
    body   = issue.get("body",  "") or ""
    labels = _label_names(issue)

    # 1. Existing priority labels (non-standard naming) → normalise
    if "priority:critical" in labels or "priority:p0" in labels:
        return "priority:critical"
    if "priority:high" in labels or "priority:p1" in labels:
        return "priority:high"
    if "priority:low" in labels or "priority:p3" in labels:
        return "priority:low"
    if "priority:medium" in labels or "priority:p2" in labels:
        return "priority:medium"

    # 2. Severity field in body
    severity = _severity_from_body(body)
    if severity in ("critical", "blocker"):
        return "priority:critical"
    if severity == "high":
        return "priority:high"
    if severity == "low":
        return "priority:low"
    # "medium" falls through to additional heuristics below

    # 3. P0 / P1 / P2 / P3 prefix in title
    p_prefix = _p_prefix(title)
    if p_prefix == "P0":
        return "priority:critical"
    if p_prefix == "P1":
        return "priority:high"
    if p_prefix == "P2":
        return "priority:medium"
    if p_prefix == "P3":
        return "priority:low"

    # 4. Roadmap phase marker in title or body (first 500 chars)
    phase_title = _phase_from_text(title)
    phase_body  = _phase_from_text(body[:500])
    phase = phase_title or phase_body
    if phase == 1:
        return "priority:critical"
    if phase in (2, 3, 4):
        return "priority:high"
    if phase == 5:
        return "priority:medium"
    if phase == 6:
        return "priority:low"

    # 5. Time-horizon section marker in body (checked after phase so that
    #    "Phase 1 … Short-term" correctly yields critical, not high)
    body_lower = body.lower()
    if _is_short_term(body_lower):
        return "priority:high"
    if _is_long_term(body_lower):
        return "priority:medium"

    # 6. Type label / title heuristics
    if "type:bug" in labels or "[bug]" in title.lower():
        return "priority:high"
    if "[docs-audit]" in title.lower():
        return "priority:low"

    # 7. Severity "medium" (deferred from step 2)
    if severity == "medium":
        return "priority:medium"

    # 8. Default
    return "priority:medium"


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Assign priority labels to GitHub issues that have none."
    )
    parser.add_argument(
        "--repo",
        default=REPO_DEFAULT,
        help="Repository in owner/repo format (default: %(default)s)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print what would be done without making any changes",
    )
    parser.add_argument(
        "--create-labels",
        action="store_true",
        help="Create missing priority:* labels before processing issues",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Print one line per issue (including already-prioritised ones)",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    token = os.environ.get("GITHUB_TOKEN", "")
    if not token:
        print("❌ GITHUB_TOKEN is not set.", file=sys.stderr)
        return 1

    api = GitHubAPI(token=token, repo=args.repo, dry_run=args.dry_run)

    if args.dry_run:
        print("🛑 Dry-run mode – no changes will be written.\n")

    # Optionally ensure labels exist
    if args.create_labels:
        print("🏷️  Ensuring priority labels exist…")
        import urllib.parse  # noqa: F401 – used in ensure_label_exists
        for name, meta in PRIORITY_LABELS.items():
            api.ensure_label_exists(name, meta["color"], meta["description"])
        print()

    print("🔍 Fetching open issues…")
    all_issues = api.list_open_issues()
    print(f"   Found {len(all_issues)} open issue(s).\n")

    needs_priority = [i for i in all_issues if not _has_priority(i)]
    already_set    = len(all_issues) - len(needs_priority)

    print(f"📊 {already_set} issue(s) already have a priority label.")
    print(f"🎯 {len(needs_priority)} issue(s) need a priority label.\n")

    if not needs_priority:
        print("✅ Nothing to do.")
        return 0

    counts: dict[str, int] = {k: 0 for k in PRIORITY_LABELS}
    skipped = 0

    for issue in needs_priority:
        num   = issue["number"]
        title = issue.get("title", "")
        prio  = infer_priority(issue)
        counts[prio] += 1

        if args.dry_run or args.verbose:
            mode = "would assign" if args.dry_run else "assigning"
            print(f"  #{num:5d}  {prio:<20}  {title[:70]}")

        if not args.dry_run:
            try:
                api.add_label(num, prio)
            except RuntimeError as exc:
                print(f"  ⚠️  #{num}: {exc}", file=sys.stderr)
                skipped += 1
            # Small pause to be kind to the rate limit
            time.sleep(0.05)

    print()
    print("── Summary ────────────────────────────────────────────────")
    for prio, count in counts.items():
        if count:
            prefix = "would assign" if args.dry_run else "assigned"
            print(f"  {prio:<22}  {prefix} {count} issue(s)")
    if skipped:
        print(f"  ⚠️  skipped (errors): {skipped}")
    if args.dry_run:
        print("\n🛑 Dry-run complete – no changes were written.")
    else:
        total = sum(counts.values()) - skipped
        print(f"\n✅ Done – {total} issue(s) updated.")
    return 0


if __name__ == "__main__":
    # urllib.parse is required by ensure_label_exists; import here so the
    # module is available when called without --create-labels as well.
    import urllib.parse  # noqa: F401
    sys.exit(main())
