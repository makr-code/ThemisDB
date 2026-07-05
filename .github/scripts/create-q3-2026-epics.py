#!/usr/bin/env python3
"""
create-q3-2026-epics.py
-----------------------
Batch-creates GitHub Issues for Q3 2026 Epic (EPIC-001 through EPIC-005)
with all sub-issues (001-A/B/C, 002-A/B/C/D/E, 003-A/B/C/D/E/F, 004-A/B/C/D, 005-A/B/C/D).

This script reads pre-defined epic markdown files and creates corresponding GitHub issues.

Usage:
    GITHUB_TOKEN=<token> GITHUB_REPOSITORY=makr-code/ThemisDB \\
        python3 .github/scripts/create-q3-2026-epics.py

    # Preview only:
    DRY_RUN=1 python3 .github/scripts/create-q3-2026-epics.py

    # Single epic:
    EPIC=001 python3 .github/scripts/create-q3-2026-epics.py

Environment variables:
    GITHUB_TOKEN       – Personal access token with `repo` scope
    GITHUB_REPOSITORY  – Owner/repo (default: makr-code/ThemisDB)
    DRY_RUN            – "1" to print without creating (default: "0")
    EPIC               – "001" through "005" for single epic, "" = all (default)

Source: ai_working/EPIC_*.md
"""

import json
import os
import sys
import time
import re
from urllib.request import Request, urlopen
from urllib.error import HTTPError
from pathlib import Path

GITHUB_API = "https://api.github.com"
GITHUB_TOKEN = os.environ.get("GITHUB_TOKEN", "")
GITHUB_REPOSITORY = os.environ.get("GITHUB_REPOSITORY", "makr-code/ThemisDB")
DRY_RUN = os.environ.get("DRY_RUN", "0") == "1"
EPIC_FILTER = os.environ.get("EPIC", "").upper()

REPO_ROOT = Path(__file__).parent.parent.parent
AI_WORKING = REPO_ROOT / "ai_working"

# ---------------------------------------------------------------------------
# Helper Functions
# ---------------------------------------------------------------------------

def _headers() -> dict:
    return {
        "Authorization": f"******",
        "Accept": "application/vnd.github+json",
        "X-GitHub-Api-Version": "2022-11-28",
    }

def create_issue(title: str, body: str, labels: list) -> dict | None:
    """Create a GitHub issue. Returns the created issue dict or None on failure."""
    url = f"{GITHUB_API}/repos/{GITHUB_REPOSITORY}/issues"
    payload = json.dumps({"title": title, "body": body, "labels": labels}).encode()
    req = Request(url, data=payload, headers=_headers(), method="POST")
    try:
        with urlopen(req) as resp:
            return json.loads(resp.read())
    except HTTPError as exc:
        print(f"  ❌  HTTP {exc.code}: {exc.read().decode()}", file=sys.stderr)
        return None

def issue_exists(title: str) -> bool:
    """Return True if an open or closed issue with this exact title already exists."""
    for state in ("open", "closed"):
        url = (
            f"{GITHUB_API}/repos/{GITHUB_REPOSITORY}/issues"
            f"?state={state}&per_page=100"
        )
        req = Request(url, headers=_headers())
        try:
            with urlopen(req) as resp:
                issues = json.loads(resp.read())
                if any(i["title"] == title for i in issues):
                    return True
        except HTTPError:
            pass
    return False

# ---------------------------------------------------------------------------
# Epic Parsing
# ---------------------------------------------------------------------------

def parse_epic_markdown(epic_file: Path) -> dict:
    """Parse an epic markdown file and extract all issue definitions."""
    if not epic_file.exists():
        print(f"❌  Epic file not found: {epic_file}", file=sys.stderr)
        return {}
    
    content = epic_file.read_text(encoding='utf-8')
    
    # Extract main epic title and metadata
    match = re.search(r'^# EPIC-(\d+):\s*(.+)$', content, re.MULTILINE)
    if not match:
        print(f"❌  No EPIC header found in {epic_file}", file=sys.stderr)
        return {}
    
    epic_num = match.group(1)
    epic_title = match.group(2).strip()
    
    issues = []
    
    # Extract sub-issue sections (###)
    sub_issue_pattern = r'^### (.+?)$'
    body_pattern = r'^- \*\*Description:\*\*(.+?)(?=^###|^##|$)'
    labels_pattern = r'^- \*\*Labels:\*\*\s*(.+?)$'
    
    # Split by sub-issue headers
    parts = re.split(sub_issue_pattern, content, flags=re.MULTILINE)
    
    for i in range(1, len(parts), 2):
        if i + 1 >= len(parts):
            break
        
        sub_title = parts[i].strip()
        sub_body = parts[i + 1].strip()
        
        # Extract labels from body
        labels_match = re.search(labels_pattern, sub_body, re.MULTILINE)
        labels = []
        if labels_match:
            label_str = labels_match.group(1).strip()
            labels = [l.strip().strip('`') for l in label_str.split(',')]
        
        # Fallback labels
        if not labels:
            labels = [f"epic-{epic_num}", "q3-2026"]
        
        issues.append({
            "epic_num": epic_num,
            "epic_title": epic_title,
            "sub_title": sub_title,
            "body": sub_body,
            "labels": labels,
        })
    
    return {
        "epic_num": epic_num,
        "epic_title": epic_title,
        "issues": issues,
    }

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> int:
    if not GITHUB_TOKEN and not DRY_RUN:
        print(
            "❌  GITHUB_TOKEN not set.\n"
            "    Export it or set DRY_RUN=1 to preview.\n"
            "    Usage: GITHUB_TOKEN=ghp_… GITHUB_REPOSITORY=makr-code/ThemisDB "
            "python3 .github/scripts/create-q3-2026-epics.py",
            file=sys.stderr,
        )
        return 1
    
    if not AI_WORKING.exists():
        print(f"❌  ai_working directory not found: {AI_WORKING}", file=sys.stderr)
        return 1
    
    # Load all epics
    all_epics = []
    for i in range(1, 6):
        epic_file = AI_WORKING / f"EPIC_{i:03d}_*.md"
        matches = list(AI_WORKING.glob(f"EPIC_{i:03d}_*.md"))
        if matches:
            data = parse_epic_markdown(matches[0])
            if data:
                all_epics.append(data)
    
    if not all_epics:
        print("❌  No epic files found in ai_working/", file=sys.stderr)
        return 1
    
    # Filter by EPIC env var
    if EPIC_FILTER:
        all_epics = [e for e in all_epics if e["epic_num"] == EPIC_FILTER]
    
    print(f"Repository : {GITHUB_REPOSITORY}")
    print(f"Dry-run    : {DRY_RUN}")
    print(f"Epic Filter: {EPIC_FILTER or 'None (all)'}")
    print(f"Epics Found: {len(all_epics)}")
    total_issues = sum(len(e.get("issues", [])) for e in all_epics)
    print(f"Sub-Issues : {total_issues}")
    print()
    
    created = 0
    skipped = 0
    failed = 0
    
    for epic in all_epics:
        epic_num = epic["epic_num"]
        print(f"\n=== EPIC-{epic_num}: {epic['epic_title']} ===")
        
        for issue in epic.get("issues", []):
            title = issue["sub_title"]
            body = issue["body"]
            labels = issue["labels"]
            
            print(f"→ {title}")
            
            if DRY_RUN:
                print(f"  [DRY-RUN] Would create with labels: {labels}")
                created += 1
                continue
            
            if issue_exists(title):
                print("  ⚠️  Already exists — skipping")
                skipped += 1
                continue
            
            result = create_issue(title, body, labels)
            if result:
                print(f"  ✅  Created: #{result['number']} → {result['html_url']}")
                created += 1
            else:
                print("  ❌  Failed to create issue")
                failed += 1
            
            # Respect GitHub secondary rate limit (1 req/s is safe)
            time.sleep(1)
    
    print()
    print("=" * 60)
    print(f"Created : {created}")
    print(f"Skipped : {skipped}")
    print(f"Failed  : {failed}")
    return 0 if failed == 0 else 1

if __name__ == "__main__":
    sys.exit(main())
