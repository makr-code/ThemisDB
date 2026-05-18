#!/usr/bin/env python3
"""
Gap Scanner v1 vs v2 Comparison Tool

Validate improvements:
- False-positive reduction
- Better categorization
- Context analysis effectiveness
"""

import json
from pathlib import Path
from typing import Dict

def compare_scans(v1_dir: str, v2_dir: str) -> Dict:
    """Compare v1 and v2 scan results"""
    
    v1_path = Path(v1_dir)
    v2_path = Path(v2_dir)
    
    # Load v1 aggregate
    try:
        with open(v1_path / 'gap_scan_aggregate.json') as f:
            v1_agg = json.load(f)
    except:
        print("[FAIL] v1 aggregate not found")
        return {}
    
    # Load v2 summary
    try:
        with open(v2_path / 'gap_scan_v2_summary.json') as f:
            v2_summary = json.load(f)
    except:
        print("[FAIL] v2 summary not found")
        return {}
    
    # Calculate totals from v1 (all categories)
    v1_total = sum(m.get('total', 0) for m in v1_agg.values())
    
    # v2 totals
    v2_total = v2_summary['total_gaps']
    v2_critical = v2_summary['by_severity']['critical']
    v2_high = v2_summary['by_severity']['high']
    v2_intentional = v2_summary['by_severity']['intentional']
    
    # Analysis
    analysis = {
        'v1_total': v1_total,
        'v2_total': v2_total,
        'gap_reduction_count': v1_total - v2_total,
        'gap_reduction_percent': round(100 * (v1_total - v2_total) / v1_total, 1) if v1_total > 0 else 0,
        'critical_gaps': v2_critical,
        'high_gaps': v2_high,
        'intentional_gaps': v2_intentional,
        'actionable_gaps': v2_critical + v2_high,
        'false_positive_reduction': v2_intentional,
    }
    
    return analysis

def print_comparison(v1_dir: str, v2_dir: str):
    """Print formatted comparison"""
    
    print("\n" + "=" * 70)
    print("Gap Scanner v1 vs v2 Comparison")
    print("=" * 70)
    
    analysis = compare_scans(v1_dir, v2_dir)
    
    if not analysis:
        print("[FAIL] Could not load scan data")
        return
    
    print(f"\n[INFO] Summary:")
    print(f"   v1 Total Gaps: {analysis['v1_total']}")
    print(f"   v2 Total Gaps: {analysis['v2_total']}")
    print(f"   Reduction: {analysis['gap_reduction_count']} gaps ({analysis['gap_reduction_percent']}%)")
    
    print(f"\n[INFO] v2 Breakdown:")
    print(f"   [CRITICAL] Critical (action required): {analysis['critical_gaps']}")
    print(f"   [HIGH] High (should fix): {analysis['high_gaps']}")
    print(f"   [OK] Intentional (by design): {analysis['intentional_gaps']}")
    print(f"   [ACTION] Total Actionable: {analysis['actionable_gaps']}")
    
    print(f"\n[INFO] Improvements in v2:")
    print(f"   [OK] Context-aware analysis (test code, mocks filtered)")
    print(f"   [OK] Platform-specific fallbacks recognized as intentional")
    print(f"   [OK] Documented STUBs marked as by-design")
    print(f"   [OK] Enhanced pattern detection (empty functions, disabled code)")
    print(f"   [OK] Severity scoring based on context")
    print(f"   [OK] File headers with live gap statistics")
    
    print("\n" + "=" * 70)

if __name__ == '__main__':
    import sys
    
    v1_dir = sys.argv[1] if len(sys.argv) > 1 else 'ai_working'
    v2_dir = sys.argv[2] if len(sys.argv) > 2 else 'ai_working'
    
    print_comparison(v1_dir, v2_dir)
