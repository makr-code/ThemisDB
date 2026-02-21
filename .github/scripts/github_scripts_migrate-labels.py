#!/usr/bin/env python3
"""
Migrate existing issues to new label schema
"""
import os
import json
from urllib.request import Request, urlopen
from urllib.error import HTTPError

GITHUB_API = "https://api.github.com"
GITHUB_TOKEN = os.environ.get("GITHUB_TOKEN")
GITHUB_REPOSITORY = os.environ.get("GITHUB_REPOSITORY")

MIGRATION_MAP = {
    "enhancement": "type:feature",
    "AQL": "area:aql",
}

DEFAULT_LABELS = {
    "type:feature",      # Default type if missing
    "status:open",       # Default status if missing
    "priority:medium",   # Default priority if missing
}

def get_all_issues():
    """Fetch all open issues/PRs"""
    url = f"{GITHUB_API}/repos/{GITHUB_REPOSITORY}/issues?state=open&per_page=100"
    headers = {
        "Accept": "application/vnd.github+json",
        "Authorization": f"Bearer {GITHUB_TOKEN}",
        "X-GitHub-Api-Version": "2022-11-28"
    }
    
    try:
        request = Request(url, headers=headers)
        response = urlopen(request)
        return json.loads(response.read().decode())
    except Exception as e:
        print(f"❌ Error fetching issues: {e}")
        return []

def update_issue_labels(issue_number, new_labels):
    """Update labels for an issue"""
    url = f"{GITHUB_API}/repos/{GITHUB_REPOSITORY}/issues/{issue_number}/labels"
    
    payload = json.dumps({"labels": list(new_labels)}).encode("utf-8")
    headers = {
        "Accept": "application/vnd.github+json",
        "Authorization": f"Bearer {GITHUB_TOKEN}",
        "X-GitHub-Api-Version": "2022-11-28"
    }
    
    try:
        request = Request(url, data=payload, headers=headers, method="PUT")
        response = urlopen(request)
        return True
    except Exception as e:
        print(f"⚠️  Could not update issue #{issue_number}: {e}")
        return False

def migrate_issue(issue):
    """Migrate a single issue"""
    current_labels = set([label["name"] for label in issue.get("labels", [])])
    new_labels = set()
    
    # Migrate old labels to new
    for old_label, new_label in MIGRATION_MAP.items():
        if old_label in current_labels:
            current_labels.remove(old_label)
            new_labels.add(new_label)
    
    # Keep non-migratable labels
    new_labels.update(current_labels)
    
    # Add defaults if missing
    has_type = any(l.startswith("type:") for l in new_labels)
    has_status = any(l.startswith("status:") for l in new_labels)
    has_priority = any(l.startswith("priority:") for l in new_labels)
    
    if not has_type:
        new_labels.add("type:feature")
    if not has_status:
        new_labels.add("status:open")
    if not has_priority:
        new_labels.add("priority:medium")
    
    return new_labels

def main():
    print("\n" + "="*70)
    print("MIGRATING LABELS FOR ALL ISSUES")
    print("="*70 + "\n")
    
    issues = get_all_issues()
    print(f"Found {len(issues)} open issues/PRs\n")
    
    migrated = 0
    for issue in issues:
        new_labels = migrate_issue(issue)
        if update_issue_labels(issue["number"], new_labels):
            print(f"✅ Issue #{issue['number']}: {', '.join(sorted(new_labels))}")
            migrated += 1
    
    print(f"\n✅ Successfully migrated: {migrated}/{len(issues)} issues")
    print("="*70 + "\n")

if __name__ == "__main__":
    main()