#!/usr/bin/env python3
"""
Label Audit Script for ThemisDB
Identifies missing labels and issues that need remediation
"""

EXISTING_LABELS = [
    "enhancement",
    "priority:medium",
    "priority:high",
    "core",
    "query",
    "acceleration",
    "AQL"
]

REQUIRED_LABELS = {
    "Type": ["type:feature", "type:bug", "type:test", "type:documentation", "type:refactor", "type:chore"],
    "Priority": ["priority:critical", "priority:high", "priority:medium", "priority:low"],
    "Status": ["status:open", "status:in_progress", "status:blocked", "status:review", "status:ready"],
    "Area": ["area:core", "area:aql", "area:query", "area:acceleration", "area:storage", "area:vector", "area:graph", "area:rag", "area:infrastructure"]
}

MIGRATION_MAP = {
    "enhancement": "type:feature",
    "priority:medium": "priority:medium",
    "priority:high": "priority:high",
    "core": "area:core",
    "query": "area:query",
    "acceleration": "area:acceleration",
    "AQL": "area:aql",
}

def get_missing_labels():
    all_required = []
    for category, labels in REQUIRED_LABELS.items():
        all_required.extend(labels)
    missing = [l for l in all_required if l not in EXISTING_LABELS and l not in MIGRATION_MAP]
    return missing

def print_audit():
    print("\n" + "="*70)
    print("THEMISDB LABEL AUDIT REPORT")
    print("="*70)
    print(f"\n📊 Current: {len(EXISTING_LABELS)} labels")
    print(f"📊 Required: {sum(len(v) for v in REQUIRED_LABELS.values())} labels")
    
    missing = get_missing_labels()
    print(f"\n➕ Labels to Create: {len(missing)}")
    for label in sorted(missing):
        print(f"  • {label}")
    
    print(f"\n✅ Mandatory Rules:")
    print("  • Exactly ONE type:* label")
    print("  • Exactly ONE priority:* label")
    print("  • Exactly ONE status:* label")
    print("  • At least ONE area:* label")
    print("\n" + "="*70 + "\n")

if __name__ == "__main__":
    print_audit()