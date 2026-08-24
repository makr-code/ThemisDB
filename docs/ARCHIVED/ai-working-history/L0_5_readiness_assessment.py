#!/usr/bin/env python3
"""
L0.5 → L3 Orchestration Cycle Status Report

Assesses current L0 data, prepares for L0.5 verification, L1-L3 updates.
"""

import json
import glob
from pathlib import Path
from collections import defaultdict

def read_v3_format(filepath):
    """Read V3 format gap scan file (module-keyed structure)."""
    try:
        with open(filepath) as f:
            data = json.load(f)
            results = {}
            for module, module_data in data.items():
                if isinstance(module_data, dict) and 'total' in module_data:
                    results[module] = {
                        'total': module_data.get('total', 0),
                        'critical': module_data.get('severity_critical', 0),
                        'high': module_data.get('severity_high', 0),
                        'categories': module_data.get('by_category', {}),
                        'affected_files': len(module_data.get('by_file', {})) if module_data.get('by_file') else 0,
                    }
            return results
    except Exception as e:
        return {}

# Gather L0 data for primary modules
modules = ['graph', 'cache', 'query', 'network', 'server', 'llm']
l0_data = {}

print("=" * 100)
print("L0 → L0.5 → L1 → L2 → L3 DOCUMENTATION ORCHESTRATION CYCLE")
print("STATUS REPORT & READINESS ASSESSMENT")
print("=" * 100)
print()

# Phase 3-4 validated results
print("📊 L0 DATA SOURCES (Prioritized)")
print("-" * 100)

# Check Phase 3-4 (highest quality)
phase34 = 'ai_working/gap_scan_results_graph_phase34.json'
if Path(phase34).exists():
    try:
        with open(phase34) as f:
            d = json.load(f)
            # This appears to be a different format
            print(f"✅ Phase 3-4 Validated Graph: AVAILABLE ({Path(phase34).stat().st_size / 1024:.0f} KB)")
    except:
        print(f"⚠️  Phase 3-4 Graph: File exists but unreadable")

# Check V3 results for each module
print()
print("📊 V3 FULL-SCANNER RESULTS (by module)")
print("-" * 100)

v3_files = glob.glob('ai_working/gap_scan_v3_*.json')
total_gaps = 0
total_critical = 0
total_high = 0
modules_with_data = []

for mod in modules:
    matching = [f for f in v3_files if f'gap_scan_v3_{mod}' in f]
    if matching:
        data = read_v3_format(matching[0])
        if data and mod in data:
            stat = data[mod]
            total = stat['total']
            crit = stat['critical']
            high = stat['high']
            files = stat['affected_files']
            
            total_gaps += total
            total_critical += crit
            total_high += high
            modules_with_data.append(mod)
            
            status = "✅" if total > 0 else "ℹ️ "
            print(f"{status} {mod:12s} | Total: {total:5d} | CRITICAL: {crit:3d} | HIGH: {high:3d} | Files: {files:3d}")
            l0_data[mod] = stat
        else:
            print(f"⏳ {mod:12s} | (No data or not found)")
    else:
        print(f"❌ {mod:12s} | (File not found)")

print()
print("=" * 100)
print("L0.5 VERIFICATION READINESS")
print("=" * 100)
print()
print(f"✅ Modules Ready for L0.5:     {len(modules_with_data)}/{len(modules)}")
print(f"✅ Total Findings to Verify:   {total_gaps:,}")
print(f"✅ CRITICAL Findings:          {total_critical}")
print(f"✅ HIGH Findings:              {total_high}")
print()

if total_gaps > 0:
    false_positive_target = int(total_gaps * 0.75)  # 75% target removal
    print(f"📈 Verification Target (L0.5 → L0.5v)")
    print(f"   - False-Positive Removal Goal: {false_positive_target:,} findings ({false_positive_target/total_gaps*100:.0f}%)")
    print(f"   - Expected Verified Gaps:      {total_gaps - false_positive_target:,} (25% remaining)")
    print()

print("=" * 100)
print("ORCHESTRATION CYCLE PLAN")
print("=" * 100)
print()
print("✅ L0.5 (Gap Verification)")
print("   - Input:    {}, gaps to verify".format(total_gaps) if total_gaps > 0 else "   - Input:    Aggregate L0 results from V3")
print("   - Process:  AI code review for semantic validation, false-positive elimination")
print("   - Output:   ai_working/gap_scan_results_verified_L0.5.json")
print("   - Target:   70-80% false-positive removal")
print()
print("✅ L1 (Module Documentation)")
print("   - Scope:    src/graph, src/cache, src/query, src/network, src/server, src/llm")
print("   - Update:   README.md, ROADMAP.md, ARCHITECTURE.md per module")
print("   - Source:   Verified L0.5 findings + existing module docs")
print("   - Output:   Updated module-level docs (127+ files in scope)")
print()
print("✅ L2 (Aggregates)")
print("   - Process:  module_doc_generator.py")
print("   - Output:   ai_working/MODULE_SNAPSHOT_AGGREGATE.md")
print("   - Output:   Cross-module views & developer snapshots")
print()
print("✅ L3 (Root Documentation)")
print("   - Update:   CHANGELOG.md, README.md, ARCHITECTURE.md, SECURITY.md")
print("   - Source:   L2 aggregates + verified L0.5 findings")
print("   - Output:   Updated root documentation")
print()
print("=" * 100)
print()
