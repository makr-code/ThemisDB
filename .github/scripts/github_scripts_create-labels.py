#!/usr/bin/env python3
"""
Create missing labels in GitHub repository
"""
import os
import sys
import json
from urllib.request import Request, urlopen
from urllib.error import URLError, HTTPError

GITHUB_API = "https://api.github.com"
GITHUB_TOKEN = os.environ.get("GITHUB_TOKEN")
GITHUB_REPOSITORY = os.environ.get("GITHUB_REPOSITORY")

LABELS_TO_CREATE = {
    # Type Labels
    "type:feature": {"color": "1f6feb", "description": "New feature or enhancement"},
    "type:bug": {"color": "d73a49", "description": "Bug fix or defect"},
    "type:test": {"color": "ffc837", "description": "Test coverage, test improvement"},
    "type:documentation": {"color": "0075ca", "description": "Documentation, guides"},
    "type:refactor": {"color": "a371f7", "description": "Code quality, refactoring"},
    "type:chore": {"color": "cfcfcf", "description": "Dependency updates, tooling"},
    
    # Priority Labels
    "priority:critical": {"color": "dc2626", "description": "Critical: blocks releases"},
    "priority:low": {"color": "6b7280", "description": "Low: future consideration"},
    
    # Status Labels
    "status:open": {"color": "0075ca", "description": "New issue, not started"},
    "status:in_progress": {"color": "1f6feb", "description": "Actively being worked on"},
    "status:blocked": {"color": "d73a49", "description": "Blocked by dependency"},
    "status:review": {"color": "6366f1", "description": "PR in review"},
    "status:ready": {"color": "10b981", "description": "Ready to merge/close"},
    
    # Area Labels
    "area:storage": {"color": "ea580c", "description": "Storage engine"},
    "area:vector": {"color": "1f6feb", "description": "Vector DB, ANN"},
    "area:graph": {"color": "a371f7", "description": "Graph processing"},
    "area:rag": {"color": "d4a100", "description": "RAG pipeline"},
    "area:infrastructure": {"color": "cfcfcf", "description": "CI/CD, deployment"},
    
    # Special Labels
    "help-wanted": {"color": "008672", "description": "Good first issue"},
    "wip": {"color": "ffc837", "description": "Work in progress"},
    "duplicate": {"color": "d73a49", "description": "Duplicate issue"},
    "invalid": {"color": "e4e4e7", "description": "Invalid/cannot reproduce"},
}

def create_label(label_name, label_data):
    """Create a single label via GitHub API"""
    url = f"{GITHUB_API}/repos/{GITHUB_REPOSITORY}/labels"
    
    payload = json.dumps({
        "name": label_name,
        "color": label_data["color"],
        "description": label_data["description"]
    }).encode("utf-8")
    
    headers = {
        "Accept": "application/vnd.github+json",
        "Authorization": f"Bearer {GITHUB_TOKEN}",
        "X-GitHub-Api-Version": "2022-11-28"
    }
    
    try:
        request = Request(url, data=payload, headers=headers, method="POST")
        response = urlopen(request)
        result = json.loads(response.read().decode())
        print(f"✅ Created label: {label_name}")
        return True
    except HTTPError as e:
        if e.code == 422:  # Already exists
            print(f"⚠️  Label already exists: {label_name}")
            return True
        else:
            print(f"❌ Failed to create label {label_name}: {e}")
            return False
    except Exception as e:
        print(f"❌ Error creating label {label_name}: {e}")
        return False

def main():
    print("\n" + "="*70)
    print("CREATING MISSING LABELS")
    print("="*70 + "\n")
    
    if not GITHUB_TOKEN or not GITHUB_REPOSITORY:
        print("❌ Missing GITHUB_TOKEN or GITHUB_REPOSITORY environment variables")
        sys.exit(1)
    
    success_count = 0
    for label_name, label_data in LABELS_TO_CREATE.items():
        if create_label(label_name, label_data):
            success_count += 1
    
    print(f"\n✅ Successfully processed: {success_count}/{len(LABELS_TO_CREATE)} labels")
    print("="*70 + "\n")

if __name__ == "__main__":
    main()