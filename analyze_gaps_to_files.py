#!/usr/bin/env python3
"""
Analyze Phase 2 gaps to understand target files and locations.
"""

import json
from pathlib import Path
from collections import defaultdict

# Load Phase 2 results
with open('ai_working/phase2_batch_results.json') as f:
    phase2 = json.load(f)

# Get INDEX gaps
index_gaps = phase2.get('modules', {}).get('index', {}).get('gaps', [])

print("INDEX Module - Gap Analysis")
print("=" * 80)
print(f"Total gaps: {len(index_gaps)}\n")

# Group by affected file
files_by_gap = defaultdict(list)
for gap in index_gaps[:20]:  # First 20
    affected_file = gap.get('affected_file', 'unknown')
    files_by_gap[affected_file].append(gap)

print("Target Files:")
for file, gaps in sorted(files_by_gap.items()):
    print(f"\n  {file} ({len(gaps)} gaps)")
    for gap in gaps[:3]:  # Show first 3 gaps per file
        print(f"    - {gap.get('description', 'N/A')[:60]}")
        print(f"      Priority: {gap.get('priority', 'N/A')}")
        print(f"      Location: {gap.get('location', 'N/A')}")

print("\n" + "=" * 80)
print("Key Insight:")
print("Phase 3 must map generated code snippets back to these target files")
print("Each snippet should have: target_file, insert_location, context_before, context_after")
