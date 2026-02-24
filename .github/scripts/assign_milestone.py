#!/usr/bin/env python3
"""Assign all open issues that have no milestone to a specific milestone.

Environment variables:
  GITHUB_TOKEN       – GitHub token with issues:write scope
  GITHUB_REPOSITORY  – "owner/repo" string (set automatically in Actions)
  MILESTONE_INPUT    – release version (e.g. "v1.5.0") or exact milestone title
                       (e.g. "Q2 2026") or milestone number (int).
                       A YAML mapping file (.github/scripts/milestone_mapping.yml)
                       can translate version tags to milestone titles.
  DRY_RUN            – "true" to only print what would be done (no writes)
"""

import json
import os
import sys
from pathlib import Path
from urllib.error import HTTPError
from urllib.request import Request, urlopen

try:
    import yaml  # PyYAML – listed in requirements.txt
    _YAML_AVAILABLE = True
except ImportError:  # pragma: no cover
    _YAML_AVAILABLE = False

GITHUB_API = "https://api.github.com"
TOKEN = os.environ.get("GITHUB_TOKEN", "")
REPO = os.environ.get("GITHUB_REPOSITORY", "")
MILESTONE_INPUT = os.environ.get("MILESTONE_INPUT", "").strip()
DRY_RUN = os.environ.get("DRY_RUN", "false").lower() == "true"

_MAPPING_FILE = Path(__file__).with_name("milestone_mapping.yml")


def _headers():
    return {
        "Authorization": f"Bearer {TOKEN}",
        "Accept": "application/vnd.github+json",
        "X-GitHub-Api-Version": "2022-11-28",
    }


def api_get(path):
    url = f"{GITHUB_API}{path}"
    req = Request(url, headers=_headers())
    try:
        with urlopen(req) as resp:
            return json.loads(resp.read().decode())
    except HTTPError as exc:
        body = exc.read().decode()
        print(f"❌ GET {path} → HTTP {exc.code}: {body}", file=sys.stderr)
        sys.exit(1)


def api_patch(path, payload):
    url = f"{GITHUB_API}{path}"
    data = json.dumps(payload).encode()
    req = Request(url, data=data, headers=_headers(), method="PATCH")
    try:
        with urlopen(req) as resp:
            return json.loads(resp.read().decode())
    except HTTPError as exc:
        body = exc.read().decode()
        print(f"❌ PATCH {path} → HTTP {exc.code}: {body}", file=sys.stderr)
        sys.exit(1)


def api_post(path, payload):
    url = f"{GITHUB_API}{path}"
    data = json.dumps(payload).encode()
    req = Request(url, data=data, headers=_headers(), method="POST")
    try:
        with urlopen(req) as resp:
            return json.loads(resp.read().decode())
    except HTTPError as exc:
        body = exc.read().decode()
        print(f"❌ POST {path} → HTTP {exc.code}: {body}", file=sys.stderr)
        sys.exit(1)


def load_mapping():
    """Return the version→milestone-title mapping dict, or {} on any error."""
    if not _YAML_AVAILABLE:
        print("⚠️  PyYAML not installed – milestone mapping skipped.", file=sys.stderr)
        return {}
    try:
        with open(_MAPPING_FILE, encoding="utf-8") as fh:
            data = yaml.safe_load(fh)
        if isinstance(data, dict):
            # Normalise keys to strings (YAML may parse e.g. numeric keys)
            return {str(k): str(v) for k, v in data.items()}
        print(
            f"⚠️  {_MAPPING_FILE} is not a YAML mapping – skipped.",
            file=sys.stderr,
        )
        return {}
    except FileNotFoundError:
        return {}  # Mapping file is optional
    except (yaml.YAMLError, OSError) as exc:
        print(f"⚠️  Could not read {_MAPPING_FILE}: {exc} – skipped.", file=sys.stderr)
        return {}


def get_all_milestones():
    """Return all milestones (open + closed) with pagination."""
    milestones = []
    for state in ("open", "closed"):
        page = 1
        while True:
            batch = api_get(
                f"/repos/{REPO}/milestones?state={state}&per_page=100&page={page}"
            )
            if not batch:
                break
            milestones.extend(batch)
            if len(batch) < 100:
                break
            page += 1
    return milestones


def resolve_milestone():
    """Return (number, title) for the requested milestone.

    Resolution order:
    1. Apply the version→title mapping from milestone_mapping.yml (if present).
    2. If the resulting title matches an existing milestone, return it.
    3. If the resulting title does NOT match any milestone, create it via the API.
    4. Numeric inputs skip the mapping and look up by milestone number directly.
    """
    if not MILESTONE_INPUT:
        print("❌ MILESTONE_INPUT is empty. Provide a milestone number or title.", file=sys.stderr)
        sys.exit(1)

    all_milestones = get_all_milestones()

    # Numeric lookup – no mapping applied (numbers are unambiguous)
    if MILESTONE_INPUT.isdigit():
        num = int(MILESTONE_INPUT)
        for m in all_milestones:
            if m["number"] == num:
                return m["number"], m["title"]
        print(f"❌ No milestone with number {num} found.", file=sys.stderr)
        sys.exit(1)

    # Apply version→title mapping
    mapping = load_mapping()
    milestone_title = mapping.get(MILESTONE_INPUT, MILESTONE_INPUT)
    if milestone_title != MILESTONE_INPUT:
        print(f"🗺️  Mapped '{MILESTONE_INPUT}' → '{milestone_title}'")

    # Title lookup (exact match)
    matches = [m for m in all_milestones if m["title"] == milestone_title]
    if len(matches) == 1:
        return matches[0]["number"], matches[0]["title"]
    if len(matches) > 1:
        nums = ", ".join(str(m["number"]) for m in matches)
        print(
            f"❌ Multiple milestones with title '{milestone_title}' found (#{nums}). "
            "Use the milestone number instead.",
            file=sys.stderr,
        )
        sys.exit(1)

    # Milestone not found – create it automatically
    print(f"➕ Milestone '{milestone_title}' not found – creating it via GitHub API.")
    if not DRY_RUN:
        created = api_post(
            f"/repos/{REPO}/milestones",
            {"title": milestone_title},
        )
        print(f"  ✅ Created milestone #{created['number']}: '{created['title']}'")
        return created["number"], created["title"]
    else:
        print(f"  🛑 Dry-run mode – milestone would be created: '{milestone_title}'")
        sys.exit(0)


def get_issues_without_milestone():
    """Return all open issues (not PRs) that have no milestone."""
    issues = []
    page = 1
    while True:
        batch = api_get(
            f"/repos/{REPO}/issues?state=open&milestone=none&per_page=100&page={page}"
        )
        if not batch:
            break
        # Filter out pull requests
        issues.extend(i for i in batch if "pull_request" not in i)
        if len(batch) < 100:
            break
        page += 1
    return issues


def main():
    if not TOKEN:
        print("❌ GITHUB_TOKEN is not set.", file=sys.stderr)
        sys.exit(1)
    if not REPO:
        print("❌ GITHUB_REPOSITORY is not set.", file=sys.stderr)
        sys.exit(1)

    milestone_num, milestone_title = resolve_milestone()
    print(f"🎯 Target milestone: #{milestone_num} – {milestone_title!r}")

    issues = get_issues_without_milestone()
    total = len(issues)
    print(f"🔍 Found {total} open issue(s) without a milestone.")

    if total == 0:
        print("✅ Nothing to do.")
        return

    if DRY_RUN:
        print("🛑 Dry-run mode – no changes will be made.")
        for issue in issues:
            print(f"   Would assign #{issue['number']}: {issue['title']!r}")
        return

    updated = 0
    skipped = 0
    for issue in issues:
        # Double-check: skip if milestone already set (guard for race conditions)
        if issue.get("milestone") is not None:
            skipped += 1
            continue
        api_patch(
            f"/repos/{REPO}/issues/{issue['number']}",
            {"milestone": milestone_num},
        )
        print(f"  ✅ Assigned #{issue['number']}: {issue['title']!r}")
        updated += 1

    summary = f"\n📊 Summary: {updated} issue(s) assigned to milestone #{milestone_num}."
    if skipped:
        summary += f" {skipped} issue(s) skipped (already had a milestone)."
    print(summary)


if __name__ == "__main__":
    main()
