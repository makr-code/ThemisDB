#!/usr/bin/env python3
"""
Quick review of clustered issues
"""

import json
from pathlib import Path

clustered = json.load(open("ai_working/clustered_issues/clustered_issues.json"))

print("=" * 70)
print("THEMISDB IMPLEMENTATION GAP AUDIT — ISSUE SUMMARY")
print("=" * 70)

print(f"\nTotal Issues: {clustered['total_issues']}")
print(f"Total Gaps: {sum(i['total_gaps'] for i in clustered['issues'])}\n")

print("ISSUE BREAKDOWN\n")

# Group by issue type
meta_issues = [i for i in clustered['issues'] if i['id'].startswith('META')]
mod_issues = [i for i in clustered['issues'] if i['id'].startswith('MOD')]
group_issues = [i for i in clustered['issues'] if i['id'].startswith('GROUP')]

print("META-ISSUES (System-wide Patterns)")
print("-" * 70)
for issue in sorted(meta_issues, key=lambda x: x['total_gaps'], reverse=True):
    print(f"  {issue['id']:12} | {issue['severity']:8} | {issue['total_gaps']:5} gaps | {issue['title']}")

print("\nCRITICAL MODULE ISSUES (>50 gaps each)")
print("-" * 70)
for issue in sorted(mod_issues, key=lambda x: x['total_gaps'], reverse=True):
    print(f"  {issue['id']:12} | {issue['severity']:8} | {issue['total_gaps']:5} gaps | {issue['title'][:50]}")

print("\nGROUPED MODULE ISSUES (Related modules)")
print("-" * 70)
for issue in sorted(group_issues, key=lambda x: x['total_gaps'], reverse=True):
    mods = ", ".join(issue['affected_modules'][:3])
    if len(issue['affected_modules']) > 3:
        mods += f" +{len(issue['affected_modules'])-3}"
    print(f"  {issue['id']:12} | {issue['severity']:8} | {issue['total_gaps']:5} gaps | {mods}")

print("\n" + "=" * 70)
print("RECOMMENDED PRIORITY ORDER")
print("=" * 70)
print("""
1. META-001 — Fix 1,620 unimplemented paths (CRITICAL)
   └─ These are production blockers; required before anything ships
   
2. MOD-acceleration, MOD-security, MOD-storage (CRITICAL)
   └─ High-impact modules; unblock dependent features
   
3. META-002 — Standardize STUB documentation (HIGH)
   └─ Required by COPILOT_INSTRUCTIONS.md
   
4. MOD-ingestion, MOD-llm, MOD-index (CRITICAL)
   └─ Data flow pipeline completeness
   
5. GROUP-001, GROUP-003 (Data Layer & ML/AI)
   └─ Enable advanced features

6. META-003 — TODO resolution (MEDIUM)
   └─ Ongoing maintenance
""")

print("=" * 70)
print(f"Generated: {clustered['generated']}")
print("=" * 70)
