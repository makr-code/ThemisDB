#!/usr/bin/env python
"""Analyze L0.5 verified gaps distribution by module."""

import json
from collections import defaultdict

# Load verified gaps
with open('gap_scan_results_verified_L0.5_full.json', 'r', encoding='utf-8') as f:
    data = json.load(f)

print("=== L0.5 Verified Gaps Summary ===")
print(f"Total verified gaps: {data['summary']['verified_gaps']}")
print(f"Severity: {data['summary']['severity_distribution']}")
print()

# Count gaps by module (improved parsing)
module_gaps = defaultdict(lambda: {'total': 0, 'HIGH': 0, 'CRITICAL': 0, 'MEDIUM': 0, 'LOW': 0, 'files': set()})
for finding in data.get('findings', []):
    file_path = finding.get('file_path', 'unknown').replace('\\', '/')
    parts = file_path.split('/')
    
    # Better module extraction
    if len(parts) > 1 and parts[0] == 'src':
        module = parts[1] if len(parts) > 1 else 'root'
    else:
        module = 'root'
    
    severity = finding.get('severity', 'UNKNOWN')
    module_gaps[module]['total'] += 1
    module_gaps[module]['files'].add(file_path)
    if severity in module_gaps[module]:
        module_gaps[module][severity] += 1

# Sort by total gaps
top_modules = sorted(module_gaps.items(), key=lambda x: x[1]['total'], reverse=True)[:20]

print("\nTop 20 modules by gap count:")
for mod, counts in top_modules:
    file_count = len(counts['files'])
    print(f"  {mod:20s}: {counts['total']:5d} gaps in {file_count:3d} files (CRITICAL: {counts['CRITICAL']:4d}, HIGH: {counts['HIGH']:4d}, MEDIUM: {counts['MEDIUM']:5d})")

print(f"\nTotal modules with gaps: {len(module_gaps)}")

# Export module breakdown for L1 orchestration
module_list = sorted([m for m, c in top_modules if c['total'] > 0])
print(f"\nPriority modules for L1 (top by gap count): {module_list}")
