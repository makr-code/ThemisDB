"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            create_module_epics.py                             ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-13 04:23:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     620                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 507fb1e555  2026-02-25  fix: remove dead variable epic_issue_id and fix dry-run o... ║
    • 2c0bbaa4c4  2026-02-25  feat: add scripts/create_module_epics.py for module epic ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
create_module_epics.py
======================
ThemisDB – Create Module Epic Issues and Link Child Issues

Creates one parent "epic" issue per ThemisDB module (44 total), populates the
issue body with the module's ``ROADMAP.md`` content, and links every existing
open issue that contains ``[<module>]`` in its title as a sub-issue (child)
of the corresponding epic.

Usage
-----
::

    # Preview what would be done (no GitHub writes)
    python scripts/create_module_epics.py --dry-run

    # Create epics and link children (requires GITHUB_TOKEN with issues:write)
    GITHUB_TOKEN=ghp_... python scripts/create_module_epics.py

    # Target a different repository
    GITHUB_TOKEN=ghp_... python scripts/create_module_epics.py --repo owner/name

    # Only create epics; skip linking child issues
    GITHUB_TOKEN=ghp_... python scripts/create_module_epics.py --no-link-children

    # Specify a different source root (default: src)
    GITHUB_TOKEN=ghp_... python scripts/create_module_epics.py --src-root src

Environment variables
---------------------
GITHUB_TOKEN
    Personal Access Token (or GITHUB_TOKEN in Actions) with ``repo`` scope
    (issues:write + labels:write).

GITHUB_REPOSITORY
    Fallback repository in ``owner/repo`` format when ``--repo`` is not given.

What it does, step by step
--------------------------
1. For each module in ``MODULES`` list:
   a. Search for an existing open issue titled
      ``[<module>] Module Roadmap & Tracking`` to ensure idempotency.
   b. If not found, read ``src/<module>/ROADMAP.md`` (falls back to a standard
      template if the file does not exist).
   c. Create the epic issue with labels ``type:epic`` and ``<module>``.
2. Fetch all open issues whose title contains ``[<module>]`` (case-insensitive).
3. For each matching child issue, call the GitHub Sub-Issues API to establish
   the parent→child relationship.
4. Print a summary report.

Notes
-----
* The GitHub Sub-Issues API is in public beta; the header
  ``GraphQL-Features: sub_issues`` is required.
* The script is idempotent: if an epic issue already exists for a module it
  will not be recreated; if a sub-issue relationship already exists the API
  will return a 422 which is silently ignored.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path
from typing import Optional

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

REPO_DEFAULT = os.environ.get("GITHUB_REPOSITORY", "makr-code/ThemisDB")
GITHUB_API = "https://api.github.com"

MODULES: list[str] = [
    "acceleration",
    "analytics",
    "api",
    "aql",
    "auth",
    "base",
    "cache",
    "cdc",
    "chimera",
    "config",
    "content",
    "core",
    "exporters",
    "geo",
    "governance",
    "gpu",
    "graph",
    "importers",
    "index",
    "ingestion",
    "llm",
    "metadata",
    "network",
    "observability",
    "performance",
    "plugins",
    "prompt_engineering",
    "query",
    "rag",
    "replication",
    "scheduler",
    "search",
    "security",
    "server",
    "sharding",
    "storage",
    "temporal",
    "themis",
    "timeseries",
    "training",
    "transaction",
    "updates",
    "utils",
    "voice",
]

EPIC_LABEL = "type:epic"
EPIC_LABEL_COLOR = "6e40c9"
EPIC_LABEL_DESC = "Epic – parent tracking issue for a module roadmap"

# Standard template used when a module has no ROADMAP.md
_ROADMAP_TEMPLATE = """\
## Module: `{module}`

> **Note:** No `ROADMAP.md` found for this module. This is the auto-generated
> tracking issue for all `[{module}]`-tagged tasks.

### Overview
This epic tracks all open issues, enhancements, and tasks related to the
**{module}** module in ThemisDB.

### References
- Source directory: `src/{module}/`
- Repository-wide roadmap: `src/ROADMAP.md`
"""


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

    def _headers(self, extra: Optional[dict] = None) -> dict[str, str]:
        h = {
            "Authorization": f"Bearer {self.token}",
            "Accept": "application/vnd.github+json",
            "X-GitHub-Api-Version": "2022-11-28",
            "Content-Type": "application/json",
        }
        if extra:
            h.update(extra)
        return h

    def _request(
        self,
        method: str,
        path: str,
        body: Optional[dict] = None,
        extra_headers: Optional[dict] = None,
    ) -> object:
        url = f"{GITHUB_API}{path}"
        data = json.dumps(body).encode() if body else None
        req = urllib.request.Request(
            url, data=data, headers=self._headers(extra_headers), method=method
        )
        for attempt in range(3):
            try:
                with urllib.request.urlopen(req, timeout=30) as resp:
                    self._rate_limit_remaining = int(
                        resp.headers.get("X-RateLimit-Remaining", "5000")
                    )
                    if self._rate_limit_remaining < 50:
                        reset = int(resp.headers.get("X-RateLimit-Reset", "0"))
                        wait = max(0, reset - int(time.time())) + 5
                        print(
                            f"⏳ Rate limit low ({self._rate_limit_remaining} left)"
                            f" – sleeping {wait}s"
                        )
                        time.sleep(wait)
                    if resp.status == 204:
                        return None
                    return json.loads(resp.read())
            except urllib.error.HTTPError as exc:
                body_text = exc.read().decode(errors="replace")
                if exc.code == 422:
                    # Sub-issue relationship already exists, or duplicate issue
                    raise _AlreadyExistsError(body_text) from exc
                if exc.code == 403 and "rate limit" in body_text.lower():
                    time.sleep(60 * (attempt + 1))
                    continue
                if exc.code in (502, 503, 504) and attempt < 2:
                    time.sleep(10 * (attempt + 1))
                    continue
                raise RuntimeError(
                    f"HTTP {exc.code} on {method} {path}: {body_text}"
                ) from exc
        raise RuntimeError(f"Failed after 3 retries: {method} {path}")

    def get(self, path: str) -> object:
        return self._request("GET", path)

    def post(
        self,
        path: str,
        body: dict,
        extra_headers: Optional[dict] = None,
    ) -> object:
        return self._request("POST", path, body, extra_headers)

    # ------------------------------------------------------------------
    # Issues
    # ------------------------------------------------------------------

    def list_open_issues(self, label: Optional[str] = None) -> list[dict]:
        """Return all open issues (not PRs).  Optionally filter by label."""
        issues: list[dict] = []
        page = 1
        label_param = f"&labels={urllib.parse.quote(label)}" if label else ""
        while True:
            batch = self.get(
                f"/repos/{self.repo}/issues?state=open&per_page=100&page={page}{label_param}"
            )
            if not batch:
                break
            issues.extend(i for i in batch if "pull_request" not in i)  # type: ignore[union-attr]
            if len(batch) < 100:  # type: ignore[arg-type]
                break
            page += 1
        return issues

    def search_issues(self, query: str) -> list[dict]:
        """Return issues matching a search query (excludes PRs)."""
        issues: list[dict] = []
        page = 1
        encoded = urllib.parse.quote(query)
        while True:
            result = self.get(
                f"/search/issues?q={encoded}&per_page=100&page={page}"
            )
            items = result.get("items", [])  # type: ignore[union-attr]
            issues.extend(i for i in items if "pull_request" not in i)
            if len(items) < 100:
                break
            page += 1
            time.sleep(0.5)  # respect secondary rate limits on search
        return issues

    def create_issue(
        self, title: str, body: str, labels: list[str]
    ) -> dict:
        """Create an issue and return the response dict."""
        return self._request(  # type: ignore[return-value]
            "POST",
            f"/repos/{self.repo}/issues",
            {"title": title, "body": body, "labels": labels},
        )

    def add_sub_issue(self, parent_number: int, child_id: int) -> None:
        """Link *child_id* (node id → integer issue id) as a sub-issue of *parent_number*.

        Uses the GitHub Sub-Issues REST API (public beta).
        Silently ignores 422 (relationship already exists).
        """
        try:
            self.post(
                f"/repos/{self.repo}/issues/{parent_number}/sub_issues",
                {"sub_issue_id": child_id},
                extra_headers={"GraphQL-Features": "sub_issues"},
            )
        except _AlreadyExistsError:
            pass  # already linked – idempotent

    # ------------------------------------------------------------------
    # Labels
    # ------------------------------------------------------------------

    def ensure_label(
        self, name: str, color: str = "0075ca", description: str = ""
    ) -> None:
        """Create *name* label if it does not already exist."""
        try:
            self.get(f"/repos/{self.repo}/labels/{urllib.parse.quote(name, safe='')}")
            return  # already exists
        except RuntimeError as exc:
            if "404" not in str(exc):
                raise
        if self.dry_run:
            print(f"  [dry-run] Would create label: {name}")
            return
        try:
            self.post(
                f"/repos/{self.repo}/labels",
                {"name": name, "color": color, "description": description},
            )
            print(f"  🏷️  Created label: {name}")
        except (_AlreadyExistsError, RuntimeError) as exc:
            if "already_exists" in str(exc) or "422" in str(exc):
                pass
            else:
                raise


class _AlreadyExistsError(RuntimeError):
    """Raised when a 422 response is received (duplicate / already exists)."""


# ---------------------------------------------------------------------------
# Roadmap loading
# ---------------------------------------------------------------------------


def load_roadmap(src_root: Path, module: str) -> str:
    """Return the content of ``src/<module>/ROADMAP.md``, or a standard template."""
    roadmap_path = src_root / module / "ROADMAP.md"
    if roadmap_path.is_file():
        return roadmap_path.read_text(encoding="utf-8")
    return _ROADMAP_TEMPLATE.format(module=module)


# ---------------------------------------------------------------------------
# Epic issue helpers
# ---------------------------------------------------------------------------

_EPIC_TITLE_RE = re.compile(
    r"^\[(?P<module>[^\]]+)\]\s+Module Roadmap & Tracking$", re.IGNORECASE
)


def epic_title(module: str) -> str:
    return f"[{module}] Module Roadmap & Tracking"


def epic_body(module: str, roadmap_content: str) -> str:
    return f"""\
## 📦 Module Epic: `{module}`

This is the **parent tracking issue** for all tasks, enhancements, and bugs
related to the `{module}` module.  All open `[{module}]` issues are linked
below as sub-issues.

---

{roadmap_content.strip()}

---

*This issue was auto-generated by `scripts/create_module_epics.py`.*
"""


def find_existing_epic(
    api: GitHubAPI, module: str, all_open_issues: list[dict]
) -> Optional[dict]:
    """Return the existing epic issue for *module*, or ``None``."""
    target_title = epic_title(module).lower()
    for issue in all_open_issues:
        if issue.get("title", "").lower() == target_title:
            return issue
    return None


# ---------------------------------------------------------------------------
# Main logic
# ---------------------------------------------------------------------------


def build_module_child_map(
    open_issues: list[dict], modules: list[str]
) -> dict[str, list[dict]]:
    """Group *open_issues* by module based on ``[module]`` prefix in title.

    An issue is assigned to a module if its title matches ``[<module>]``
    (case-insensitive) at any position.  Issues that match the epic title
    pattern are excluded (they are the parents, not children).
    """
    # Build a lookup set for fast membership tests
    module_set = {m.lower() for m in modules}
    result: dict[str, list[dict]] = {m: [] for m in modules}

    for issue in open_issues:
        title = issue.get("title", "")
        # Skip epic issues themselves
        if _EPIC_TITLE_RE.match(title):
            continue
        # Find bracketed tokens that look like module names
        for token in re.findall(r"\[([^\]]+)\]", title):
            key = token.lower()
            if key in module_set:
                result[key].append(issue)
    return result


def run(args: argparse.Namespace) -> int:
    token = os.environ.get("GITHUB_TOKEN", "")
    if not token:
        print("❌ GITHUB_TOKEN environment variable is not set.", file=sys.stderr)
        return 1

    api = GitHubAPI(token=token, repo=args.repo, dry_run=args.dry_run)
    src_root = Path(args.src_root)

    print("=" * 70)
    print("ThemisDB – Module Epic Creator")
    print("=" * 70)
    print(f"Repository  : {args.repo}")
    print(f"Source root : {src_root}")
    print(f"Modules     : {len(MODULES)}")
    print(f"Mode        : {'DRY-RUN – no GitHub writes' if args.dry_run else 'LIVE'}")
    print("=" * 70)
    print()

    # ------------------------------------------------------------------
    # 1. Ensure the type:epic label exists
    # ------------------------------------------------------------------
    print("🏷️  Ensuring labels …")
    if not args.dry_run:
        api.ensure_label(EPIC_LABEL, EPIC_LABEL_COLOR, EPIC_LABEL_DESC)
    else:
        print(f"  [dry-run] Would ensure label: {EPIC_LABEL}")
    print()

    # ------------------------------------------------------------------
    # 2. Fetch ALL open issues once (to avoid many API calls)
    # ------------------------------------------------------------------
    print("🔍 Fetching all open issues …")
    all_open = api.list_open_issues()
    print(f"   Found {len(all_open)} open issue(s).\n")

    # ------------------------------------------------------------------
    # 3. Group child issues by module
    # ------------------------------------------------------------------
    child_map = build_module_child_map(all_open, MODULES)

    # ------------------------------------------------------------------
    # 4. Process each module
    # ------------------------------------------------------------------
    created = 0
    skipped = 0
    linked = 0
    errors = 0

    for idx, module in enumerate(MODULES, start=1):
        print(f"[{idx:02d}/{len(MODULES)}] {module}")

        # ---- Find or create epic issue ----
        existing_epic = find_existing_epic(api, module, all_open)

        if existing_epic:
            epic_number = existing_epic["number"]
            print(f"  ℹ️  Epic already exists: #{epic_number} – skipping creation")
            skipped += 1
        else:
            roadmap = load_roadmap(src_root, module)
            title = epic_title(module)
            body = epic_body(module, roadmap)
            labels = [EPIC_LABEL, module]

            if args.dry_run:
                print(f"  [dry-run] Would create: «{title}»")
                print(f"            Labels: {labels}")
                created += 1
                # Fall through so children are listed consistently with the
                # existing-epic dry-run path; use a sentinel epic_number.
                epic_number = 0
            else:
                try:
                    result = api.create_issue(title, body, labels)
                    epic_number = result["number"]
                    print(f"  ✅ Created epic #{epic_number}: {result['html_url']}")
                    created += 1
                    # Brief pause to avoid secondary rate limits
                    time.sleep(0.3)
                except RuntimeError as exc:
                    print(f"  ❌ Failed to create epic: {exc}", file=sys.stderr)
                    errors += 1
                    continue

        # ---- Link child issues ----
        if args.no_link_children:
            continue

        children = child_map[module]
        if not children:
            print(f"  ↳  No child issues to link.")
            continue

        print(f"  ↳  Linking {len(children)} child issue(s) …")
        for child in children:
            child_num = child["number"]
            child_id = child["id"]
            child_title_short = child.get("title", "")[:60]

            if args.dry_run:
                print(f"     [dry-run] Would link #{child_num}: {child_title_short}")
                linked += 1
            else:
                try:
                    api.add_sub_issue(epic_number, child_id)
                    print(f"     🔗 Linked #{child_num}: {child_title_short}")
                    linked += 1
                    time.sleep(0.1)
                except RuntimeError as exc:
                    print(
                        f"     ⚠️  Could not link #{child_num}: {exc}",
                        file=sys.stderr,
                    )
                    errors += 1

    # ------------------------------------------------------------------
    # 5. Summary
    # ------------------------------------------------------------------
    print()
    print("=" * 70)
    print("Summary")
    print("=" * 70)
    if args.dry_run:
        print(f"  Would create  : {created} epic issue(s)")
        print(f"  Would skip    : {skipped} (already exist)")
        print(f"  Would link    : {linked} child issue(s)")
    else:
        print(f"  Created       : {created} epic issue(s)")
        print(f"  Skipped       : {skipped} (already existed)")
        print(f"  Linked        : {linked} child issue(s)")
        print(f"  Errors        : {errors}")
    print("=" * 70)

    return 0 if errors == 0 else 1


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Create module epic issues and link child issues in ThemisDB.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--repo",
        default=os.environ.get("GITHUB_REPOSITORY", REPO_DEFAULT),
        help="GitHub repository in owner/name format (default: %(default)s)",
    )
    parser.add_argument(
        "--src-root",
        default="src",
        metavar="PATH",
        help="Path to the source root containing module directories (default: src)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        default=False,
        help="Preview all actions without writing to GitHub",
    )
    parser.add_argument(
        "--no-link-children",
        action="store_true",
        default=False,
        help="Create epic issues only; do not link child issues",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    return run(args)


if __name__ == "__main__":
    sys.exit(main())
