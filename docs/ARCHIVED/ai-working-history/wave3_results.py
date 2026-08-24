#!/usr/bin/env python3
"""Wave 3 Results Analyzer"""
import json
from pathlib import Path

baseline = json.loads(Path('ai_working/gap_scan_v3_baseline_summary.json').read_text())
current = json.loads(Path('ai_working/gap_scan_pipeline_v3_summary.json').read_text())

baseline_by_cat = baseline.get('scanner_summary', {}).get('by_category', {})
current_by_cat = current.get('scanner_summary', {}).get('by_category', {})

print("\n" + "="*80)
print("WAVE 3 FP TUNING RESULTS — copy_overhead & manual_cleanup Refinements")
print("="*80)

targets = ['copy_overhead', 'manual_cleanup', 'container', 'raii']

for cat in targets:
    b = baseline_by_cat.get(cat, 0)
    c = current_by_cat.get(cat, 0)
    delta = c - b
    pct = (delta / b * 100) if b > 0 else 0
    
    print(f"\n{cat.upper():<20}")
    print(f"  Baseline:    {b:6d}")
    print(f"  Current:     {c:6d}")
    print(f"  Delta:       {delta:+6d} ({pct:+6.1f}%)")
    print(f"  Status:      {'✅ PASS' if b > 0 and pct < -20 else '⚠️ PARTIAL' if b > 0 else 'N/A'}")

b_total = baseline.get('total_gaps', 0)
c_total = current.get('total_gaps', 0)
overall_pct = ((c_total - b_total) / b_total * 100) if b_total > 0 else 0

print(f"\n{'-'*80}")
print(f"\n📈 OVERALL IMPACT")
print(f"  Baseline Total:  {b_total:,} gaps")
print(f"  Current Total:   {c_total:,} gaps")
print(f"  Net Change:      {c_total - b_total:+,} ({overall_pct:+.1f}%)")

print(f"\n{'='*80}")
