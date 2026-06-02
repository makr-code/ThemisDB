#!/usr/bin/env python3
"""
Wave 3 FP Tuning Validator: Before/After Comparison
Compares copy_overhead and manual_cleanup findings before and after tuning
"""

import json
import sys
from pathlib import Path
from collections import defaultdict

def load_summary(path):
    """Load gap_scan_pipeline_v3_summary.json"""
    try:
        with open(path, 'r') as f:
            return json.load(f)
    except:
        return None

def analyze_category(summary, category):
    """Extract findings for a specific category"""
    if not summary:
        return {}
    
    modules_by_category = summary.get('modules_by_category', {})
    cat_data = modules_by_category.get(category, {})
    
    return {
        'total': cat_data.get('total_findings', 0),
        'modules': len(cat_data.get('modules', [])),
        'avg_confidence': cat_data.get('avg_confidence_band', 0),
        'breakdown': cat_data.get('module_counts', {}),
    }

def main():
    baseline_summary = Path('ai_working/gap_scan_v3_baseline_summary.json')
    current_summary = Path('ai_working/gap_scan_pipeline_v3_summary.json')
    
    if not baseline_summary.exists():
        print("[!] ERROR: No baseline found. Create baseline first:")
        print(f"    cp ai_working/gap_scan_pipeline_v3_summary.json ai_working/gap_scan_v3_baseline_summary.json")
        sys.exit(1)
    
    baseline = load_summary(baseline_summary)
    current = load_summary(current_summary)
    
    if not baseline or not current:
        print("[!] Failed to load summary files")
        sys.exit(1)
    
    print("\n" + "="*70)
    print("WAVE 3 FP TUNING — DELTA ANALYSIS")
    print("="*70)
    
    # Analyze key categories
    categories_of_interest = ['copy_overhead', 'manual_cleanup']
    
    for cat in categories_of_interest:
        baseline_data = analyze_category(baseline, cat)
        current_data = analyze_category(current, cat)
        
        baseline_count = baseline_data.get('total', 0)
        current_count = current_data.get('total', 0)
        
        if baseline_count == 0:
            percent_change = 0
        else:
            percent_change = ((current_count - baseline_count) / baseline_count) * 100
        
        print(f"\n📊 {cat.upper()}")
        print(f"   Baseline:      {baseline_count:5d} findings")
        print(f"   Current:       {current_count:5d} findings")
        print(f"   Change:        {percent_change:+6.1f}% ({current_count - baseline_count:+d})")
        
        if current_count > 0:
            print(f"   Confidence:    {current_data.get('avg_confidence', 'N/A')}")
            print(f"   Modules:       {current_data.get('modules', 'N/A')}")
    
    # Overall metrics
    baseline_total = baseline.get('total_findings', 0)
    current_total = current.get('total_findings', 0)
    overall_change = ((current_total - baseline_total) / baseline_total * 100) if baseline_total > 0 else 0
    
    print(f"\n📈 OVERALL")
    print(f"   Baseline:      {baseline_total:5d} total gaps")
    print(f"   Current:       {current_total:5d} total gaps")
    print(f"   Change:        {overall_change:+6.1f}% ({current_total - baseline_total:+d})")
    
    # Success criteria
    copy_oh = analyze_category(baseline, 'copy_overhead')
    manual_cl = analyze_category(baseline, 'manual_cleanup')
    
    target_reduction = 0.20  # 20% target
    
    copy_oh_baseline = copy_oh.get('total', 0)
    manual_cl_baseline = manual_cl.get('total', 0)
    
    print(f"\n✅ SUCCESS CRITERIA (>20% reduction target):")
    
    if copy_oh_baseline > 0:
        copy_oh_current = analyze_category(current, 'copy_overhead').get('total', 0)
        copy_oh_reduction = (copy_oh_baseline - copy_oh_current) / copy_oh_baseline
        status = "✓ PASS" if copy_oh_reduction >= target_reduction else "✗ FAIL"
        print(f"   copy_overhead: {copy_oh_reduction*100:.1f}% reduction {status}")
    
    if manual_cl_baseline > 0:
        manual_cl_current = analyze_category(current, 'manual_cleanup').get('total', 0)
        manual_cl_reduction = (manual_cl_baseline - manual_cl_current) / manual_cl_baseline
        status = "✓ PASS" if manual_cl_reduction >= target_reduction else "✗ FAIL"
        print(f"   manual_cleanup: {manual_cl_reduction*100:.1f}% reduction {status}")
    
    print("\n" + "="*70)

if __name__ == '__main__':
    main()
