#!/usr/bin/env python
"""L1 - Module Documentation Orchestrator: Update module docs from L0.5 verified gaps."""

import json
import os
from collections import defaultdict
from datetime import datetime

# Load verified gaps
with open('gap_scan_results_verified_L0.5_full.json', 'r', encoding='utf-8') as f:
    l0_5_data = json.load(f)

TIMESTAMP = datetime.now().isoformat()
PRIORITY_MODULES = ['llm', 'server', 'query', 'network', 'graph', 'cache']

# Aggregate gaps by module
module_gaps = defaultdict(lambda: {
    'total': 0, 'CRITICAL': 0, 'HIGH': 0, 'MEDIUM': 0, 'LOW': 0,
    'files': set(), 'findings': []
})

for finding in l0_5_data.get('findings', []):
    file_path = (finding.get('file') or finding.get('file_path', 'unknown')).replace('\\', '/')
    parts = file_path.split('/')
    module = parts[1] if len(parts) > 1 and parts[0] == 'src' else parts[0]
    
    severity = finding.get('severity', 'UNKNOWN')
    module_gaps[module]['total'] += 1
    module_gaps[module][severity] += 1
    module_gaps[module]['files'].add(file_path)
    module_gaps[module]['findings'].append({
        'file': file_path,
        'line': finding.get('line') or finding.get('line_number'),
        'severity': severity,
        'issue': finding.get('issue') or finding.get('description'),
    })

# Generate L1 module update summary
print("=== L1 - Module Documentation Updates ===")
print(f"Execution: {TIMESTAMP}")
print(f"Verified gaps: {l0_5_data['summary']['verified_gaps']}")
print()

l1_report = {
    'orchestration_level': 'L1',
    'timestamp': TIMESTAMP,
    'source': 'gap_scan_results_verified_L0.5_full.json',
    'priority_modules': PRIORITY_MODULES,
    'module_updates': {}
}

# Process priority modules
for module in PRIORITY_MODULES:
    if module in module_gaps:
        gaps = module_gaps[module]
        update_info = {
            'total_gaps': gaps['total'],
            'severity_breakdown': {
                'CRITICAL': gaps['CRITICAL'],
                'HIGH': gaps['HIGH'],
                'MEDIUM': gaps['MEDIUM'],
                'LOW': gaps['LOW']
            },
            'affected_files': len(gaps['files']),
            'files_list': sorted(list(gaps['files']))[:10],  # Top 10
            'actions': [
                f"Update or create src/{module}/README.md",
                f"Update or create src/{module}/ROADMAP.md with gap context",
                f"Add CRITICAL ({gaps['CRITICAL']}) and HIGH ({gaps['HIGH']}) gap items",
                f"Map findings to relevant roadmap sections",
                f"Link to L0.5 verification evidence"
            ]
        }
        l1_report['module_updates'][module] = update_info
        
        print(f"Module: {module}")
        print(f"  Total gaps: {gaps['total']}")
        print(f"  Severity: CRITICAL={gaps['CRITICAL']}, HIGH={gaps['HIGH']}, MEDIUM={gaps['MEDIUM']}, LOW={gaps['LOW']}")
        print(f"  Affected files: {len(gaps['files'])}")
        print(f"  Top files: {', '.join(sorted(list(gaps['files']))[:3])}")
        print()

# Save L1 report
with open('L1_MODULE_ORCHESTRATION_REPORT.json', 'w', encoding='utf-8') as f:
    json.dump(l1_report, f, indent=2)

print("\nL1 orchestration report saved to: L1_MODULE_ORCHESTRATION_REPORT.json")
print("\nNext steps:")
print("1. Update module README.md files with overview and current status")
print("2. Update module ROADMAP.md with gap remediation priorities")
print("3. Add L0.5 verification reference in module docs")
print("4. Ensure UPPER_SNAKE naming in all module documentation")
print("5. Run L2 aggregation: python L2_aggregates_orchestrator.py")
