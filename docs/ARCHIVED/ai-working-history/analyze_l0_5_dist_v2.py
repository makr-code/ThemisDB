#!/usr/bin/env python
"""Analyze L0.5 verified gaps distribution by module (corrected)."""

import json
from collections import defaultdict

# Load verified gaps
with open('gap_scan_results_verified_L0.5_full.json', 'r', encoding='utf-8') as f:
    data = json.load(f)

print("=== L0.5 Verified Gaps Summary ===")
print(f"Total verified gaps: {data['summary']['verified_gaps']}")
print(f"Severity: {data['summary']['severity_distribution']}")
print()

# Count gaps by module (use 'file' field, not 'file_path')
module_gaps = defaultdict(lambda: {'total': 0, 'HIGH': 0, 'CRITICAL': 0, 'MEDIUM': 0, 'LOW': 0, 'files': set()})
for finding in data.get('findings', []):
    # Prefer 'file' over 'file_path'
    file_path = finding.get('file') or finding.get('file_path', 'unknown')
    file_path = file_path.replace('\\', '/')
    parts = file_path.split('/')
    
    # Better module extraction for src/ structure
    if len(parts) > 1 and parts[0] == 'src':
        module = parts[1] if len(parts) > 1 else 'root'
    else:
        module = parts[0] if parts[0] else 'root'
    
    severity = finding.get('severity', 'UNKNOWN')
    module_gaps[module]['total'] += 1
    module_gaps[module]['files'].add(file_path.replace(f'src/{module}/', ''))
    if severity in module_gaps[module]:
        module_gaps[module][severity] += 1

# Sort by total gaps
top_modules = sorted(module_gaps.items(), key=lambda x: x[1]['total'], reverse=True)

print(f"All {len(top_modules)} modules with gaps:")
print()
for mod, counts in top_modules:
    file_count = len(counts['files'])
    print(f"  {mod:20s}: {counts['total']:5d} gaps in {file_count:3d} files (CRITICAL: {counts['CRITICAL']:4d}, HIGH: {counts['HIGH']:4d}, MEDIUM: {counts['MEDIUM']:5d})")

# Identify priority modules (top 6 from user request: graph, cache, query, network, server, llm)
priority_modules = ['graph', 'cache', 'query', 'network', 'server', 'llm']
print("\n\nPriority modules for L1 phase:")
for pmod in priority_modules:
    if pmod in dict(module_gaps):
        counts = module_gaps[pmod]
        print(f"  {pmod:20s}: {counts['total']:5d} gaps (CRITICAL: {counts['CRITICAL']:4d}, HIGH: {counts['HIGH']:4d})")
    else:
        print(f"  {pmod:20s}: No gaps found")
