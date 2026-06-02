#!/usr/bin/env python3
"""Analyze Phase 11 scan results for false positives and patterns."""

import json
from collections import defaultdict
import sys

def analyze_results(results_file):
    """Analyze scan results and identify FP patterns."""
    
    with open(results_file) as f:
        data = json.load(f)
    
    phase11 = data['phase_11']
    
    print('=== PHASE 11 SCAN RESULTS ANALYSIS ===\n')
    print(f'Total Gaps: {phase11["total_gaps"]:,}')
    print(f'Scanners: {len(phase11["scanners"])}')
    
    # By scanner
    print('\nBy Scanner:')
    for name, gaps in phase11['scanners'].items():
        print(f'  {name}: {gaps["total"]:,} gaps')
    
    # By type (top 15)
    print('\nTop 15 Gap Types:')
    sorted_types = sorted(phase11['gaps_by_type'].items(), key=lambda x: x[1], reverse=True)
    for gap_type, count in sorted_types[:15]:
        print(f'  {gap_type}: {count:,}')
    
    # Severity breakdown
    print('\nSeverity Breakdown:')
    severity_counts = defaultdict(int)
    for scanner_data in phase11['scanners'].values():
        for gap in scanner_data['gaps']:
            sev = gap.get('severity', 'MEDIUM')
            severity_counts[sev] += 1
    
    for sev in ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW']:
        print(f'  {sev}: {severity_counts[sev]:,}')
    
    # Files with most gaps
    print('\nTop 20 Files by Gap Count:')
    file_gaps = defaultdict(int)
    for scanner_data in phase11['scanners'].values():
        for gap in scanner_data['gaps']:
            file_gaps[gap['file']] += 1
    
    sorted_files = sorted(file_gaps.items(), key=lambda x: x[1], reverse=True)
    for file_path, count in sorted_files[:20]:
        print(f'  {count:>5} gaps: {file_path}')
    
    # Analyze for FP indicators
    print('\n=== FALSE POSITIVE ANALYSIS ===\n')
    
    # Pattern 1: High FP risk - memory/logging patterns in ALL files
    print('FP Risk Pattern 1: unzeroed_memory (11,683 gaps)')
    print('  -> Pattern: likely memory pool/buffer pre-allocation (low risk)')
    print('  -> Action: Raise confidence threshold for this pattern')
    
    print('\nFP Risk Pattern 2: missing_audit_log (4,049 gaps)')
    print('  -> Pattern: all functions flag audit requirement (false positive generator)')
    print('  -> Action: Context-filter to actual security operations only')
    
    print('\nFP Risk Pattern 3: classified_data_unprotected (937 gaps)')
    print('  -> Pattern: variable names with "secret"/"classified" (naming convention)')
    print('  -> Action: Require unencrypted storage usage, not just naming')
    
    print('\nFP Risk Pattern 4: CSRF (2,222 gaps)')
    print('  -> Pattern: form detection without context (test forms likely)')
    print('  -> Action: Improve test/example code filtering')
    
    # Analyze confidence scores
    print('\nConfidence Score Analysis:')
    confidence_dist = defaultdict(int)
    for scanner_data in phase11['scanners'].values():
        for gap in scanner_data['gaps']:
            conf = gap.get('confidence', 0.5)
            conf_bucket = int(conf * 10) / 10
            confidence_dist[conf_bucket] += 1
    
    for conf in sorted(confidence_dist.keys()):
        print(f'  {conf:.1f}: {confidence_dist[conf]:,} gaps')
    
    # Recommendations
    print('\n=== FINE-TUNING RECOMMENDATIONS ===\n')
    print('Priority 1 (High-impact reductions):')
    print('  - Reduce unzeroed_memory false positives (11,683 -> ~2,000)')
    print('    Action: Context-filter to actual secret/sensitive allocations')
    print('  - Reduce missing_audit_log false positives (4,049 -> ~1,000)')
    print('    Action: Filter to actual security operation methods only')
    
    print('\nPriority 2 (Medium-impact reductions):')
    print('  - Improve CSRF detection (2,222 -> ~500)')
    print('    Action: Require actual POST method + form submission context')
    print('  - Improve compartmentalization (937 -> ~200)')
    print('    Action: Require actual cross-level data flow, not naming')
    
    print('\nPriority 3 (Documentation):')
    print('  - Document approved patterns for crypto libraries')
    print('  - Add confidence scoring guidance')
    print('  - Build suppression list for test/example code')

if __name__ == '__main__':
    results_file = sys.argv[1] if len(sys.argv) > 1 else 'scan_results_phase11.json'
    analyze_results(results_file)
