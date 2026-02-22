#!/usr/bin/env python3
import os
import sys

EXISTING = ["enhancement", "priority:medium", "priority:high", "core", "query", "acceleration", "AQL"]
REQUIRED = {
    "Type": ["type:feature", "type:bug", "type:enhancement", "type:test", "type:documentation", "type:refactor", "type:chore"],
    "Priority": ["priority:critical", "priority:high", "priority:medium", "priority:low"],
    "Status": ["status:open", "status:in_progress", "status:blocked", "status:review", "status:ready"],
    "Area": ["area:core", "area:aql", "area:query", "area:acceleration", "area:storage", "area:vector", "area:graph", "area:rag", "area:infrastructure"]
}

print("\n" + "="*70)
print("THEMISDB LABEL AUDIT REPORT")
print("="*70)
print(f"\n📊 Existing labels: {len(EXISTING)}")
print(f"📊 Required labels: {sum(len(v) for v in REQUIRED.values())}")

all_required = []
for labels in REQUIRED.values():
    all_required.extend(labels)

missing = [l for l in all_required if l not in EXISTING]
print(f"\n➕ Labels to create: {len(missing)}")
for label in sorted(missing):
    print(f"  • {label}")

print("\n" + "="*70 + "\n")