#!/usr/bin/env python
"""L1: Update module documentation with L0.5 verified gaps (LLM, Server, Query, Network, Graph, Cache)."""

import json
from collections import defaultdict
from datetime import datetime

# Load L0.5 verified gaps
with open('gap_scan_results_verified_L0.5_full.json', 'r', encoding='utf-8') as f:
    l0_5_data = json.load(f)

TIMESTAMP = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
PRIORITY_MODULES = {
    'llm': {'gap_count': 3821, 'critical': 1029, 'high': 1937},
    'server': {'gap_count': 2172, 'critical': 186, 'high': 468},
    'query': {'gap_count': 933, 'critical': 131, 'high': 296},
    'network': {'gap_count': 368, 'critical': 22, 'high': 221},
    'graph': {'gap_count': 248, 'critical': 18, 'high': 45},
    'cache': {'gap_count': 127, 'critical': 10, 'high': 74},
}

# Aggregate gaps by module for detailed stats
module_gaps = defaultdict(lambda: {
    'total': 0, 'CRITICAL': 0, 'HIGH': 0, 'MEDIUM': 0, 'LOW': 0,
    'files': set(), 'findings_by_category': defaultdict(int)
})

for finding in l0_5_data.get('findings', []):
    file_path = (finding.get('file') or finding.get('file_path', 'unknown')).replace('\\', '/')
    parts = file_path.split('/')
    module = parts[1] if len(parts) > 1 and parts[0] == 'src' else parts[0]
    
    severity = finding.get('severity', 'UNKNOWN')
    module_gaps[module]['total'] += 1
    module_gaps[module][severity] += 1
    module_gaps[module]['files'].add(file_path)
    
    # Track by category
    category = finding.get('category') or finding.get('type') or finding.get('gap_type', 'unknown')
    module_gaps[module]['findings_by_category'][category] += 1

# Generate L1 update plan
print("=" * 80)
print("L1 - Module Documentation Update Plan")
print(f"Execution: {TIMESTAMP}")
print("=" * 80)
print()

# Create MODULE_GAPS.md updates
for module in ['llm', 'server', 'query', 'network', 'graph', 'cache']:
    if module not in module_gaps:
        continue
    
    gaps = module_gaps[module]
    expected = PRIORITY_MODULES.get(module, {})
    
    print(f"Module: {module.upper()}")
    print(f"  Path: src/{module}/")
    print(f"  L0.5 Verified Gaps: {gaps['total']}")
    print(f"  Severity: CRITICAL={gaps['CRITICAL']}, HIGH={gaps['HIGH']}, MEDIUM={gaps['MEDIUM']}, LOW={gaps['LOW']}")
    print(f"  Affected Files: {len(gaps['files'])}")
    print(f"  Top Categories:")
    
    # Sort categories by count
    sorted_cats = sorted(gaps['findings_by_category'].items(), key=lambda x: x[1], reverse=True)[:5]
    for cat, count in sorted_cats:
        print(f"    - {cat}: {count}")
    
    print(f"  Documentation Updates Required:")
    print(f"    ✓ src/{module}/MODULE_GAPS.md (refresh with L0.5 data)")
    print(f"    ✓ src/{module}/README.md (add Gap Status section)")
    print(f"    ✓ src/{module}/ROADMAP.md (add gap remediation items)")
    if gaps['CRITICAL'] > 10 or 'security' in str(gaps['findings_by_category']).lower():
        print(f"    ✓ src/{module}/SECURITY.md (add security-related gaps)")
    print()

print("\n" + "=" * 80)
print("Action Items for L1 Phase:")
print("=" * 80)
print("""
1. Update MODULE_GAPS.md for each priority module
   - Replace old scan data with L0.5 verified findings
   - Update severity summary table
   - Update category summary with actual L0.5 data
   - Add L0.5 verification timestamp and method

2. Update README.md for each priority module
   - Add "Gap Status" section after "Known Limitations"
   - Link to MODULE_GAPS.md for detailed breakdown
   - Summarize top issues and remediation status

3. Update ROADMAP.md for each priority module
   - Add "Gap Remediation" phase with top CRITICAL items
   - Link to GitHub issues for tracking
   - Set remediation targets by severity level

4. Update SECURITY.md if needed
   - Highlight security-related gaps
   - Add immediate action items for CRITICAL gaps
   - Link to issue tracker

5. Verification steps
   - Confirm UPPER_SNAKE naming in all module docs
   - Verify cross-links between README, ROADMAP, MODULE_GAPS
   - Check timestamps are current (2026-06-25)
   - Ensure SOT references to L0.5 verification are present

Source: gap_scan_results_verified_L0.5_full.json
Level: L1 (Module-level Documentation)
Next: L2 Aggregation → L3 Root Docs
""")

# Save execution summary
summary = {
    'timestamp': TIMESTAMP,
    'level': 'L1',
    'source': 'gap_scan_results_verified_L0.5_full.json',
    'operation': 'Module Documentation Update',
    'priority_modules': list(PRIORITY_MODULES.keys()),
    'total_verified_gaps': l0_5_data['summary']['verified_gaps'],
    'modules_updated': {
        mod: {
            'path': f'src/{mod}/',
            'verified_gaps': module_gaps[mod]['total'],
            'severity_breakdown': {
                'CRITICAL': module_gaps[mod]['CRITICAL'],
                'HIGH': module_gaps[mod]['HIGH'],
                'MEDIUM': module_gaps[mod]['MEDIUM'],
                'LOW': module_gaps[mod]['LOW']
            },
            'affected_files': len(module_gaps[mod]['files']),
            'docs_to_update': [
                f'src/{mod}/MODULE_GAPS.md',
                f'src/{mod}/README.md',
                f'src/{mod}/ROADMAP.md',
                f'src/{mod}/SECURITY.md'
            ]
        }
        for mod in PRIORITY_MODULES.keys()
    }
}

with open('L1_MODULE_UPDATE_PLAN.json', 'w', encoding='utf-8') as f:
    json.dump(summary, f, indent=2)

print("\nPlan saved to: L1_MODULE_UPDATE_PLAN.json")
