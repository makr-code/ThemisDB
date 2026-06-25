#!/usr/bin/env python3
"""Quick analysis of all AI-vibe scan results WITH IMPACT CLASSIFICATION"""

import json
import glob
from collections import Counter
from pathlib import Path

results_dir = Path('ai_working')

# Find all scan results
scan_files = {
    'benchmarks': 'scan_benchmarks.json',
    'tests': 'scan_tests.json',
    'include': 'scan_include.json',
    'src': 'scan_src.json',
}

print("=" * 100)
print("AI-VIBE SCAN ANALYSIS WITH THEMISDB IMPACT CLASSIFICATION")
print("=" * 100)
print()

all_ai_vibe = {}

for scope, filename in scan_files.items():
    filepath = results_dir / filename
    
    if not filepath.exists():
        print(f"[{scope:15s}] - SCANNING...")
        continue
    
    try:
        data = json.load(open(filepath))
        gaps = data.get('gaps', [])
        
        # Extract AI-vibe findings
        ai_vibe = [g for g in gaps if any(x in g.get('type','') for x in 
                   ['todo_', 'simulation_', 'error_handling', 'llm_', 'header_drift'])]
        
        all_ai_vibe[scope] = ai_vibe
        
        # Count by type
        types = Counter(g.get('type','') for g in ai_vibe)
        sev = Counter(g.get('severity','') for g in ai_vibe)
        
        # NEW: Count by impact level
        impact = Counter(g.get('impact_level','') for g in ai_vibe)
        
        print(f"[{scope:15s}] {len(ai_vibe):5d} AI-Vibe findings")
        print(f"  Severity:  C:{sev.get('CRITICAL',0):4d}  H:{sev.get('HIGH',0):4d}  M:{sev.get('MEDIUM',0):4d}  L:{sev.get('LOW',0):4d}")
        print(f"  Impact:    CRIT:{impact.get('CRITICAL',0):4d}  HIGH:{impact.get('HIGH',0):4d}  MED:{impact.get('MEDIUM',0):4d}  LOW:{impact.get('LOW',0):4d}  3P:{impact.get('THIRD_PARTY',0):4d}")
        
        if types:
            print(f"  Top Types:")
            for t, c in sorted(types.items(), key=lambda x: -x[1])[:3]:
                print(f"    - {t}: {c}")
        print()
        
    except Exception as e:
        print(f"[{scope:15s}] ERROR: {e}")
        print()

# Summary
print("=" * 100)
print("AGGREGATE WITH IMPACT TIERS")
print("=" * 100)

total_ai_vibe = sum(len(v) for v in all_ai_vibe.values())
print(f"\nTotal AI-Vibe Findings Across All Scopes: {total_ai_vibe}")

# Aggregate by type and impact
all_types = Counter()
all_sev = Counter()
all_impact = Counter()

# NEW: Critical path analysis (CRITICAL severity + CRITICAL/HIGH impact)
critical_path = []

for scope, findings in all_ai_vibe.items():
    all_types.update(g.get('type','') for g in findings)
    all_sev.update(g.get('severity','') for g in findings)
    all_impact.update(g.get('impact_level','') for g in findings)
    
    # Find critical path findings (worst combo)
    for g in findings:
        if g.get('severity') == 'CRITICAL' and g.get('impact_level') in ['CRITICAL', 'HIGH']:
            critical_path.append(g)

print(f"\nBy Severity (aggregated):")
for s in ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW', 'INFO']:
    if all_sev.get(s, 0) > 0:
        print(f"  {s:10s}: {all_sev[s]:5d}")

print(f"\nBy ThemisDB Impact Level (aggregated):")
for imp in ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW', 'THIRD_PARTY']:
    if all_impact.get(imp, 0) > 0:
        print(f"  {imp:12s}: {all_impact[imp]:5d}")

print(f"\n⚠️  CRITICAL PATH (Sev=CRITICAL + Impact=CRITICAL/HIGH):")
print(f"  {len(critical_path)} findings requiring immediate review")

print(f"\nBy Type (aggregated):")
for t, c in sorted(all_types.items(), key=lambda x: -x[1])[:5]:
    print(f"  {t:45s}: {c:5d}")

print("\n" + "=" * 100)
