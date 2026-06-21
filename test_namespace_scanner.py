#!/usr/bin/env python3
"""Test the Namespace & Unity Build Scanner"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent / "tools"))

from scanners.gs3_step01_namespace_unity_check import NamespaceUnityCheckScanner

# Test files
test_file = Path("src/graph/ontology_manager.cpp")

if not test_file.exists():
    print(f"❌ Test file not found: {test_file}")
    sys.exit(1)

print("=" * 80)
print("Testing Namespace & Unity Build Scanner")
print("=" * 80)
print()

scanner = NamespaceUnityCheckScanner()
gaps = scanner.scan(test_file)

print(f"Scanner: {scanner.name}")
print(f"Version: {scanner.version}")
print(f"Test File: {test_file}")
print(f"Findings: {len(gaps)}")
print()

if gaps:
    print("Findings:")
    for gap in gaps:
        print(f"  [{gap.severity:8s}] {gap.type:40s} Line {gap.line}: {gap.description}")
        print(f"             Remediation: {gap.remediation}")
else:
    print("✅ No gaps detected")

print()
print("=" * 80)
