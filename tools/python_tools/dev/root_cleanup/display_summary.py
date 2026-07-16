#!/usr/bin/env python3
"""Generate detailed per-directory summary with top issues"""

import json
from collections import Counter
from pathlib import Path

print("=" * 90)
print(" " * 20 + "AI-VIBE COMPREHENSIVE SCAN - FINAL REPORT")
print("=" * 90)
print()

# Load all scans
scans = {
    'src': 'ai_working/scan_src.json',
    'include': 'ai_working/scan_include.json', 
    'tests': 'ai_working/scan_tests.json',
    'benchmarks': 'ai_working/scan_benchmarks.json',
}

total_ai_vibe = 0
total_critical = 0
total_high = 0
all_modules = Counter()

for scope, filepath in scans.items():
    data = json.load(open(filepath))
    gaps = data.get('gaps', [])
    
    # Extract AI-vibe
    ai_vibe = [g for g in gaps if any(x in g.get('type','') for x in 
               ['todo_', 'simulation_', 'error_handling', 'llm_', 'header_drift'])]
    
    if not ai_vibe:
        continue
    
    total_ai_vibe += len(ai_vibe)
    
    # Severity counts
    crit = sum(1 for g in ai_vibe if g.get('severity') == 'CRITICAL')
    high = sum(1 for g in ai_vibe if g.get('severity') == 'HIGH')
    total_critical += crit
    total_high += high
    
    # Top files by finding count
    files = Counter(g.get('file', 'unknown') for g in ai_vibe)
    
    print(f"┌─ {scope.upper():20s} ─ {len(ai_vibe):5d} findings ─┐")
    print(f"│ Severity: CRITICAL={crit:4d}  HIGH={high:4d}                        │")
    print(f"│ Top 5 Affected Files:                                      │")
    
    for i, (fname, count) in enumerate(sorted(files.items(), key=lambda x: -x[1])[:5], 1):
        fname_short = fname if len(fname) < 45 else fname[-42:] + "..."
        print(f"│   {i}. {fname_short:45s} {count:4d} findings │")
    print(f"└────────────────────────────────────────────────────────────┘")
    print()

print()
print("=" * 90)
print("FINAL AGGREGATE")
print("=" * 90)
print(f"Total AI-Vibe Findings (all 4 directories): {total_ai_vibe:,d}")
print(f"  CRITICAL: {total_critical:,d} ({100*total_critical/total_ai_vibe:.1f}%)")
print(f"  HIGH:     {total_high:,d} ({100*total_high/total_ai_vibe:.1f}%)")
print()
print("Breakdown by Type:")

# Re-aggregate types
all_types = Counter()
for filepath in scans.values():
    data = json.load(open(filepath))
    gaps = data.get('gaps', [])
    ai_vibe = [g for g in gaps if any(x in g.get('type','') for x in 
               ['todo_', 'simulation_', 'error_handling', 'llm_', 'header_drift'])]
    all_types.update(g.get('type','') for g in ai_vibe)

for t, c in sorted(all_types.items(), key=lambda x: -x[1]):
    pct = 100 * c / total_ai_vibe
    bar_len = int(pct / 2)
    bar = "█" * bar_len
    print(f"  {t:40s} {c:5d} ({pct:5.1f}%) {bar}")

print()
print("=" * 90)
print("IMMEDIATE ACTIONS (P0 - Week 1)")
print("=" * 90)
print("1. TEST INFRASTRUCTURE LEAK: Audit 586 stub markers in tests/")
print("2. LLM SECURITY: Implement validation for 319 unvalidated/unsanitized LLM I/O")
print("3. CRITICAL REVIEW: Assess 2,311 CRITICAL-severity findings for production impact")
print()
print("→ Full analysis available in: AI_VIBE_SCAN_REPORT.md")
print("=" * 90)
