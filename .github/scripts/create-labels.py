#!/usr/bin/env python3
import os
import json
from urllib.request import Request, urlopen
from urllib.error import HTTPError

GITHUB_API = "https://api.github.com"
GITHUB_TOKEN = os.environ.get("GITHUB_TOKEN")
GITHUB_REPOSITORY = os.environ.get("GITHUB_REPOSITORY")

LABELS_TO_CREATE = {
    "type:feature": {"color": "1f6feb", "description": "New feature or enhancement"},
    "type:bug": {"color": "d73a49", "description": "Bug fix or defect"},
    "type:test": {"color": "ffc837", "description": "Test coverage"},
    "type:documentation": {"color": "0075ca", "description": "Documentation"},
    "type:refactor": {"color": "a371f7", "description": "Code refactoring"},
    "type:chore": {"color": "cfcfcf", "description": "Chores"},
    "priority:critical": {"color": "dc2626", "description": "Critical priority"},
    "priority:high": {"color": "f97316", "description": "High priority"},
    "priority:medium": {"color": "eab308", "description": "Medium priority"},
    "priority:low": {"color": "6b7280", "description": "Low priority"},
    "status:open": {"color": "0075ca", "description": "Status: open"},
    "status:in_progress": {"color": "1f6feb", "description": "Status: in progress"},
    "status:blocked": {"color": "d73a49", "description": "Status: blocked"},
    "status:review": {"color": "6366f1", "description": "Status: review"},
    "status:ready": {"color": "10b981", "description": "Status: ready"},
    "area:storage": {"color": "ea580c", "description": "Area: storage"},
    "area:vector": {"color": "1f6feb", "description": "Area: vector"},
    "area:graph": {"color": "a371f7", "description": "Area: graph"},
    "area:rag": {"color": "d4a100", "description": "Area: RAG"},
    "area:infrastructure": {"color": "cfcfcf", "description": "Area: infrastructure"},
    "help-wanted": {"color": "008672", "description": "Help wanted"},
    "wip": {"color": "ffc837", "description": "Work in progress"},
    "duplicate": {"color": "d73a49", "description": "Duplicate"},
    "invalid": {"color": "e4e4e7", "description": "Invalid"},
    # ── Copilot Issue Dispatcher labels ────────────────────────────────────
    # Issue labels
    "queue/copilot": {"color": "7057ff", "description": "Issue is eligible for automatic Copilot processing"},
    "copilot/delegated": {"color": "1f6feb", "description": "Delegation comment posted; Copilot Coding Agent has been tasked"},
    # ── EPIC labels ────────────────────────────────────────────────────────
    # EPIC 5518: Hybrid Retrieval Execution Boundaries
    "epic/5518": {"color": "6e40c9", "description": "Part of EPIC #5518: Hybrid Retrieval Execution Boundaries"},
}

def create_label(name, data):
    url = f"{GITHUB_API}/repos/{GITHUB_REPOSITORY}/labels"
    payload = json.dumps({
        "name": name,
        "color": data["color"],
        "description": data["description"]
    }).encode("utf-8")
    
    headers = {
        "Authorization": f"Bearer {GITHUB_TOKEN}",
        "Accept": "application/vnd.github+json",
    }
    
    try:
        request = Request(url, data=payload, headers=headers, method="POST")
        response = urlopen(request)
        print(f"✅ Created: {name}")
        return True
    except HTTPError as e:
        if e.code == 422:
            print(f"⚠️  Exists: {name}")
            return True
        print(f"❌ Failed: {name}")
        return False

print("Creating labels...")
for name, data in LABELS_TO_CREATE.items():
    create_label(name, data)
print("Done!")