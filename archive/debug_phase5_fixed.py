#!/usr/bin/env python3
import json
from pathlib import Path

# Load the test file
with open('ai_working/gap_scan_phase5_fixed.json', 'r', encoding='utf-8') as f:
    data = json.load(f)

# Show first 10 file paths
if 'gaps' in data:
    print(f"Total gaps in JSON: {len(data['gaps'])}")
    print("\nFirst 10 gaps (if any):")
    for i, gap in enumerate(data['gaps'][:10]):
        print(f"  {i+1}. {gap.get('file')}:{gap.get('line')}")
else:
    print("No 'gaps' key in JSON")

# Try to understand what happened
print("\nMetadata:")
if 'metadata' in data:
    md = data['metadata']
    if 'scope_breakdown' in md:
        print(f"  Scope breakdown: {md['scope_breakdown']['counts']}")
