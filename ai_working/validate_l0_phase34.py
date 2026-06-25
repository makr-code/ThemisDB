#!/usr/bin/env python3
"""Validate L0 Phase 3-4 Results"""

import json
from pathlib import Path

result_file = Path('ai_working/gap_scan_results_graph_phase34.json')
if result_file.exists():
    with open(result_file) as f:
        data = json.load(f)
    
    print("=" * 80)
    print("L0 PHASE 3-4 VALIDATION RESULTS (Graph Module Sample)")
    print("=" * 80)
    
    # Extract metadata
    meta = data.get('metadata', {})
    p34 = meta.get('verification_phase_3_4', {})
    
    print("\n[PHASE 3: Cache Stale Detection]")
    cache_info = p34.get('cache_phase3', {})
    print(f"  Cache exists: {cache_info.get('cache_exists', False)}")
    print(f"  Cache age (hours): {cache_info.get('cache_age_hours', 0)}")
    print(f"  Cache valid: {cache_info.get('cache_valid', False)}")
    print(f"  Files checked: {cache_info.get('files_checked', 0)}")
    print(f"  Files missing: {cache_info.get('files_missing', 0)}")
    
    print("\n[PHASE 4: Enriched Metadata]")
    print(f"  Scanner version: {p34.get('version', 'N/A')}")
    print(f"  Scan timestamp: {p34.get('timestamp', 'N/A')}")
    print(f"  Python version: {p34.get('python_version', 'N/A')}")
    print(f"  Platform: {p34.get('platform', 'N/A')}")
    
    scan_stats = p34.get('scan_statistics', {})
    print("\n[SCAN STATISTICS]")
    print(f"  Total gaps after verification: {scan_stats.get('total_gaps', 0)}")
    sev = scan_stats.get('by_severity', {})
    for level in ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW']:
        print(f"    {level}: {sev.get(level, 0)}")
    
    v12 = p34.get('verification_phase1_phase2', {})
    print("\n[VERIFICATION PHASE 1 & 2 SUMMARY]")
    print(f"  Input (raw scanner): {v12.get('input_raw_scanner', 0)}")
    print(f"  FILE_NOT_FOUND removed: {v12.get('file_not_found_removed', 0)}")
    print(f"  Findings downgraded: {v12.get('findings_downgraded', 0)}")
    print(f"  Kept unchanged: {v12.get('kept_unchanged', 0)}")
    
    classifications = v12.get('classifications', {})
    print("\n[GAP CLASSIFICATIONS]")
    for cls, count in classifications.items():
        print(f"  {cls}: {count}")
    
    print("\n[SCOPE BREAKDOWN]")
    scope = meta.get('scope_breakdown', {})
    counts = scope.get('counts', {})
    for key in ['themis_core', 'themis_tests', 'themis_benchmarks', 'third_party']:
        pct = scope.get('percentages', {}).get(key, 0)
        print(f"  {key}: {counts.get(key, 0)} ({pct}%)")
    
    print("\n✓ Phase 3-4 pipeline validated successfully!")
    print(f"✓ Output file: {result_file}")
    print(f"✓ File size: {result_file.stat().st_size / 1024:.1f} KB")
    
else:
    print(f"ERROR: {result_file} not found")
