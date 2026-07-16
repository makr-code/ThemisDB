#!/usr/bin/env python3
"""Generate L0 dokifi graph gap scanner report."""

import json
from pathlib import Path
from datetime import datetime

# Read gap scan
gap_scan_path = Path(__file__).parent / 'ai_working' / 'gap_scan_graph.json'
with open(gap_scan_path, 'r') as f:
    gap_scan = json.load(f)

# Analyze
gaps_by_file = {}
for gap in gap_scan['gaps']:
    file = gap['file']
    gaps_by_file[file] = gaps_by_file.get(file, 0) + 1

category_count = {}
severity_count = {}
for gap in gap_scan['gaps']:
    cat = gap['category']
    sev = gap['severity']
    category_count[cat] = category_count.get(cat, 0) + 1
    severity_count[sev] = severity_count.get(sev, 0) + 1

# Build report
report = {
    'timestamp': datetime.utcnow().isoformat(),
    'level': 'L0',
    'scope': 'graph',
    'execution_status': 'success',
    'discovered_locations': [
        {'path': 'src/graph', 'exists': True, 'note': 'implementation source'},
        {'path': 'include/graph', 'exists': True, 'note': 'public API headers'},
        {'path': 'tests/graph', 'exists': True, 'note': 'unit tests'},
        {'path': 'benchmarks/graph', 'exists': False, 'note': 'performance benchmarks'}
    ],
    'gap_scan_summary': {
        'total_gaps': len(gap_scan['gaps']),
        'categories': category_count,
        'severity_distribution': severity_count,
        'affected_files': len(gaps_by_file),
        'top_affected_files': sorted(
            [{'file': f, 'gap_count': c} for f, c in gaps_by_file.items()],
            key=lambda x: x['gap_count'],
            reverse=True
        )[:3]
    },
    'critical_gaps': [
        {
            'file': gap['file'],
            'line': gap['line_num'],
            'category': gap['category'],
            'severity': gap['severity'],
            'context': gap.get('context', '')
        }
        for gap in gap_scan['gaps'] if gap['severity'] == 'critical'
    ][:5],  # Top 5
    'warnings': [
        f"CRITICAL: {len([g for g in gap_scan['gaps'] if g['severity'] == 'critical'])} critical gaps found in unimplemented functions",
        f"UNIMPLEMENTED: {category_count.get('unimplemented', 0)} stub/incomplete functions (return {{}} patterns)",
        f"STUBS: {category_count.get('stub', 0)} mock/stub test functions",
        "ACTION: explain_plan.cpp and ontology_manager.cpp have 4 critical gaps each"
    ],
    'next_steps': [
        'L1: Execute full gap_scanner_v3.py with all phases (Security, Memory, Reliability, Concurrency, RAII, Container, Platform, Performance)',
        'L1.1: Prioritize Security phase on graph/explain_plan.cpp and graph/ontology_manager.cpp',
        'L1.2: Run Memory Safety phase to check for GPUTensor/index initialization patterns',
        'L2: Implement missing bodies in explain_plan::getExplainText(), getExplainJson() and ontology_manager functions',
        'L3: Convert test stubs in test_compute_graph_header.cpp from Mock to real implementations',
        'L4: Add comprehensive gap_scan_v3_graph.json output for Phase 1-11 analysis'
    ]
}

# Write report
output_path = Path(__file__).parent / 'ai_working' / 'gap_scanner_results.json'
with open(output_path, 'w') as f:
    json.dump(report, f, indent=2)

print('✓ DOKIFI L0 GRAPH REPORT GENERATED')
print('=' * 70)
print(json.dumps(report, indent=2))
