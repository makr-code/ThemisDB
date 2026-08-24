#!/usr/bin/env python3
"""Assess L0 data structure and readiness for L0.5 → L3 orchestration."""

import json
import glob
from pathlib import Path

def inspect_l0_file(filepath):
    """Inspect structure of an L0 gap scan file."""
    try:
        with open(filepath) as f:
            data = json.load(f)
            info = {
                'file': Path(filepath).name,
                'keys': list(data.keys()),
                'findings_count': 0,
                'metadata': {}
            }
            
            # Look for findings
            if 'findings' in data:
                info['findings_count'] = len(data['findings'])
            
            # Extract metadata
            if 'metadata' in data:
                meta = data['metadata']
                info['metadata'] = {
                    'scanner': meta.get('scanner', 'unknown'),
                    'total_gaps': meta.get('total_gaps', len(data.get('findings', []))),
                }
            
            return info
    except Exception as e:
        return {'file': Path(filepath).name, 'error': str(e)}

# Assess key modules
modules_to_check = ['graph', 'cache', 'query', 'network', 'server', 'llm']
print("=" * 80)
print("L0 DATA ASSESSMENT FOR L0.5 → L3 ORCHESTRATION")
print("=" * 80)
print()

# Priority 1: Validated Phase 3-4 results
print("✅ PRIORITY 1: Validated Phase 3-4 Results")
print("-" * 80)
phase34_files = glob.glob('ai_working/gap_scan_results_*_phase34.json')
for f in sorted(phase34_files):
    info = inspect_l0_file(f)
    if 'error' not in info:
        print(f"{info['file']:45s} | Findings: {info['findings_count']:6d} | Scanner: {info['metadata'].get('scanner', 'N/A')}")
print()

# Priority 2: V3 full-scanner results
print("✅ PRIORITY 2: Gap Scanner V3 Results")
print("-" * 80)
v3_files = glob.glob('ai_working/gap_scan_v3_*.json')
for mod in modules_to_check:
    matching = [f for f in v3_files if f'gap_scan_v3_{mod}' in f]
    if matching:
        info = inspect_l0_file(matching[0])
        if 'error' not in info:
            print(f"{mod:12s}: {info['file']:45s} | Findings: {info['findings_count']:6d}")

print()
print("=" * 80)
print("L0 READINESS STATUS")
print("=" * 80)
print(f"Phase 3-4 (Validated):        {len(phase34_files)} files")
print(f"V3 Full-Scanner:              {len(v3_files)} files")
print(f"Recommended for L0.5:         Use Phase 3-4 validated (highest confidence)")
print()
