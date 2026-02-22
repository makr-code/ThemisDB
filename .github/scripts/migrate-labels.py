#!/usr/bin/env python3
import os
import json
from urllib.request import Request, urlopen

GITHUB_API = "https://api.github.com"
GITHUB_TOKEN = os.environ.get("GITHUB_TOKEN")
GITHUB_REPOSITORY = os.environ.get("GITHUB_REPOSITORY")

MIGRATION_MAP = {
    "enhancement": "type:enhancement",
    "AQL": "area:aql",
    "core": "area:core",
    "query": "area:query",
    "acceleration": "area:acceleration",
}

def get_issues():
    url = f"{GITHUB_API}/repos/{GITHUB_REPOSITORY}/issues?state=open&per_page=100"
    headers = {"Authorization": f"Bearer {GITHUB_TOKEN}"}
    try:
        request = Request(url, headers=headers)
        response = urlopen(request)
        return json.loads(response.read().decode())
    except:
        return []

def update_labels(issue_num, labels):
    url = f"{GITHUB_API}/repos/{GITHUB_REPOSITORY}/issues/{issue_num}/labels"
    payload = json.dumps({"labels": list(labels)}).encode("utf-8")
    headers = {
        "Authorization": f"Bearer {GITHUB_TOKEN}",
        "Accept": "application/vnd.github+json",
        "Content-Type": "application/json",
    }
    try:
        request = Request(url, data=payload, headers=headers, method="PUT")
        urlopen(request)
        return True
    except:
        return False

issues = get_issues()
print(f"Migrating {len(issues)} issues...")

for issue in issues:
    labels = set([l["name"] for l in issue.get("labels", [])])
    
    for old, new in MIGRATION_MAP.items():
        if old in labels:
            labels.remove(old)
            labels.add(new)
    
    # Add defaults
    if not any(l.startswith("type:") for l in labels):
        labels.add("type:feature")
    if not any(l.startswith("status:") for l in labels):
        labels.add("status:open")
    if not any(l.startswith("priority:") for l in labels):
        labels.add("priority:medium")
    
    if update_labels(issue["number"], labels):
        print(f"✅ Issue #{issue['number']}")

print("Done!")