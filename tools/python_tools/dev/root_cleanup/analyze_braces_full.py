#!/usr/bin/env python3
import json
import sys
from collections import Counter

# Load the full braces test results
with open('ai_working/gap_scan_braces_test.json', 'r') as f:
    data = json.load(f)

print("=== Braces Check Analysis ===\n")
print(f"Top-level keys: {list(data.keys())}")
print(f"Total gaps reported: {data.get('total_gaps')}")

# Check for gaps
if 'gaps' in data:
    gaps = data['gaps']
    print(f"\nGaps structure:")
    print(f"  Type: {type(gaps)}")
    print(f"  Count: {len(gaps) if isinstance(gaps, list) else 'N/A'}")
    
    if isinstance(gaps, list) and len(gaps) > 0:
        # Analyze gap types
        types = Counter(g.get('type') for g in gaps if isinstance(g, dict))
        print(f"\nFinding types distribution:")
        for type_name, count in sorted(types.items(), key=lambda x: -x[1]):
            print(f"  {type_name}: {count}")
        
        # Analyze by file
        files = Counter(g.get('file') for g in gaps if isinstance(g, dict))
        print(f"\nFindings per file (top 20):")
        for file, count in files.most_common(20):
            print(f"  {file}: {count}")
        
        # Show severity distribution
        severities = Counter(g.get('severity') for g in gaps if isinstance(g, dict))
        print(f"\nSeverity distribution:")
        for sev, count in sorted(severities.items(), key=lambda x: -x[1]):
            print(f"  {sev}: {count}")
        
        # Show confidence scores
        confidences = [g.get('confidence', 0) for g in gaps if isinstance(g, dict)]
        print(f"\nConfidence statistics:")
        print(f"  Min: {min(confidences) if confidences else 'N/A'}")
        print(f"  Max: {max(confidences) if confidences else 'N/A'}")
        print(f"  Avg: {sum(confidences)/len(confidences) if confidences else 'N/A':.2f}")
        
        # Show first 5 findings
        print(f"\nSample findings (first 5):")
        for i, finding in enumerate(gaps[:5], 1):
            if isinstance(finding, dict):
                print(f"\n  Finding {i}:")
                print(f"    File: {finding.get('file')}")
                print(f"    Type: {finding.get('type')}")
                print(f"    Line: {finding.get('line')}")
                print(f"    Severity: {finding.get('severity')}")
                print(f"    Description: {finding.get('description', '')[:80]}...")

elif 'scope_breakdown' in data:
    print("\nStructure contains scope_breakdown, not gaps list")
    print(f"Scope keys: {list(data.get('scope_breakdown', {}).keys())}")
