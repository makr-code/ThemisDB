#!/usr/bin/env python
"""Inspect finding structure in L0.5 data."""

import json

data = json.load(open('gap_scan_results_verified_L0.5_full.json', encoding='utf-8'))

# Check first few findings
print("First 5 findings structure:")
for i, finding in enumerate(data.get('findings', [])[:5]):
    print(f"\nFinding {i}:")
    print(f"  Keys: {list(finding.keys())}")
    file_path = finding.get('file_path') or finding.get('file')
    print(f"  File: {file_path}")
    print(f"  Severity: {finding.get('severity')}")
    
# Also check for alternative field names
print("\n\nAll unique keys in findings:")
all_keys = set()
for f in data.get('findings', []):
    all_keys.update(f.keys())
print(sorted(all_keys))

# Check file distribution by looking at 'file' vs 'file_path'
print("\n\nChecking file field usage:")
file_path_count = sum(1 for f in data.get('findings', []) if 'file_path' in f)
file_count = sum(1 for f in data.get('findings', []) if 'file' in f)
print(f"  file_path: {file_path_count}")
print(f"  file: {file_count}")

# Show some actual files
print("\nSample files from 'file' field:")
files_seen = set()
for f in data.get('findings', []):
    if 'file' in f and len(files_seen) < 10:
        files_seen.add(f['file'])
        print(f"  {f['file']}")
