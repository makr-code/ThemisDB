#!/usr/bin/env python3
"""Gap Scanner v3 Overall Delta Report"""

import json
from pathlib import Path

def main():
    agg_path = Path('ai_working/gap_scan_v3_aggregate.json')
    summary_path = Path('ai_working/gap_scan_pipeline_v3_summary.json')
    
    agg = json.loads(agg_path.read_text(encoding='utf-8'))
    summary = json.loads(summary_path.read_text(encoding='utf-8'))
    
    # Module sorting
    modules = sorted(
        [(k, v) for k, v in agg.items() if isinstance(v, dict)],
        key=lambda x: -x[1].get('total', 0)
    )
    
    # Print
    print('=' * 95)
    print('V3 GAP SCANNER - OVERALL DELTA REPORT (Post-FP-Tuning Wave 1 & 2)')
    print('=' * 95)
    
    total = summary['total_gaps']
    print(f'\nScan Date:           {summary["scan_date"]}')
    print(f'Total Gaps Found:    {total:,}')
    print(f'Modules Scanned:     {summary["total_modules"]}')
    sev = summary['by_severity']
    print(f'Critical:            {sev["critical"]:,} ({100*sev["critical"]//total}%)')
    print(f'High:                {sev["high"]:,} ({100*sev["high"]//total}%)')
    print(f'Medium:              {sev["medium"]:,} ({100*sev["medium"]//total}%)')
    
    print('\n' + '-' * 95)
    print('TOP 20 MODULES BY TOTAL GAPS')
    print('-' * 95)
    print(f"{'Rank':<5} {'Module':<30} {'Total':<12} {'Crit':<8} {'High':<8} {'Med':<8}")
    print('-' * 95)
    
    for i, (name, data) in enumerate(modules[:20], 1):
        t = data.get('total', 0)
        pct = (100 * t // total) if total > 0 else 0
        c = data.get('severity_critical', 0)
        h = data.get('severity_high', 0)
        m = data.get('severity_medium', 0)
        print(f'{i:<5} {name:<30} {t:>6,}({pct:>3}%) {c:>8} {h:>8} {m:>8}')
    
    # Categories
    cats = summary['scanner_summary']['by_category']
    print('\n' + '-' * 95)
    print('CATEGORY BREAKDOWN (Top 15)')
    print('-' * 95)
    print(f"{'Category':<40} {'Count':<12} {'Percent':<10}")
    print('-' * 95)
    
    sorted_cats = sorted(cats.items(), key=lambda x: -x[1])[:15]
    for cat, count in sorted_cats:
        pct = (100 * count // total) if total > 0 else 0
        print(f'{cat:<40} {count:>6,} {pct:>6}%')
    
    # Confidence
    conf = summary.get('scanner_summary', {}).get('confidence_overview', {})
    if conf:
        print('\n' + '-' * 95)
        print('CONFIDENCE SCORE DISTRIBUTION')
        print('-' * 95)
        print(f"{'Level':<40} {'Count':<12} {'Percent':<10}")
        print('-' * 95)
        for level, count in conf.items():
            pct = (100 * count // total) if total > 0 else 0
            print(f'{level:<40} {count:>6,} {pct:>6}%')
    
    # Confidence by category (if available)
    conf_by_cat = summary.get('scanner_summary', {}).get('confidence_by_category', {})
    if conf_by_cat:
        print('\n' + '-' * 95)
        print('CONFIDENCE BY CATEGORY (Sample)')
        print('-' * 95)
        for cat in list(conf_by_cat.keys())[:5]:
            data = conf_by_cat[cat]
            print(f"\n  {cat}:")
            print(f"    Total: {data.get('total', 0):,}")
            print(f"    High Confidence: {data.get('high_confidence', 0):,}")
            print(f"    Very High: {data.get('very_high_confidence', 0):,}")
            print(f"    Avg Score: {data.get('avg_confidence', 0):.3f}")
    
    print('\n' + '=' * 95)
    print('[OK] Delta analysis complete')

if __name__ == '__main__':
    main()
