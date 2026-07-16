#!/usr/bin/env python3
"""Identify remaining false-positive candidates in v3 scanners"""

import json
from pathlib import Path
from collections import defaultdict

def main():
    agg_path = Path('ai_working/gap_scan_v3_aggregate.json')
    summary_path = Path('ai_working/gap_scan_pipeline_v3_summary.json')
    
    if not agg_path.exists() or not summary_path.exists():
        print("[FAIL] Missing v3 scan artifacts")
        return
    
    agg = json.loads(agg_path.read_text(encoding='utf-8'))
    summary = json.loads(summary_path.read_text(encoding='utf-8'))
    
    # Tuned categories (from Wave 1 & 2)
    tuned = {
        'data_race', 'no_timeout', 'pointer_arithmetic',
        'null_dereference', 'iterator_invalidation',
        'deadlock_risk', 'lock_in_loop', 'uncaught_exception'
    }
    
    # Collect per-category stats
    cat_stats = defaultdict(lambda: {
        'total': 0, 'critical': 0, 'high': 0, 'medium': 0,
        'high_conf': 0, 'very_high_conf': 0, 'avg_conf': 0,
        'files': set()
    })
    
    for module_name, data in agg.items():
        if not isinstance(data, dict):
            continue
        
        by_file = data.get('by_file', {})
        for file_path, findings in (by_file or {}).items():
            for finding in findings:
                cat = finding.get('category') or finding.get('type') or 'uncategorized'
                sev = str(finding.get('severity', 'INFO')).upper()
                conf = finding.get('confidence_band', 'medium')
                
                cat_stats[cat]['total'] += 1
                cat_stats[cat]['files'].add(f"{module_name}/{file_path}")
                
                if sev == 'CRITICAL':
                    cat_stats[cat]['critical'] += 1
                elif sev == 'HIGH':
                    cat_stats[cat]['high'] += 1
                elif sev == 'MEDIUM':
                    cat_stats[cat]['medium'] += 1
                
                if conf in ('high', 'very_high'):
                    if conf == 'very_high':
                        cat_stats[cat]['very_high_conf'] += 1
                    else:
                        cat_stats[cat]['high_conf'] += 1
    
    # Convert sets to counts
    for cat in cat_stats:
        cat_stats[cat]['files'] = len(cat_stats[cat]['files'])
    
    # Calculate confidence percentages
    for cat, stats in cat_stats.items():
        total = stats['total']
        if total > 0:
            high_conf_total = stats['high_conf'] + stats['very_high_conf']
            stats['conf_pct'] = (100 * high_conf_total) // total
        else:
            stats['conf_pct'] = 0
    
    # Print header
    print('=' * 110)
    print('FALSE-POSITIVE CANDIDATE ANALYSIS - Untuned Scanners')
    print('=' * 110)
    
    print("\n[CONTEXT] Previously tuned categories (Wave 1 & 2):")
    print(f"  {', '.join(sorted(tuned))}")
    
    print("\n[STATUS] Tuned reduction impact:")
    print("  data_race: 1,243 -> 1,120 (-123)")
    print("  no_timeout: 823 -> 145 (-678)")
    print("  pointer_arithmetic: 413 -> 298 (-115)")
    print("  null_dereference: 1,634 -> 654 (-980)")
    print("  iterator_invalidation: 521 -> 227 (-294)")
    print("  deadlock_risk: 230 -> 78 (-152)")
    print("  lock_in_loop: 1,166 -> 74 (-1,092)")
    print("  uncaught_exception: 3,641 -> 1,526 (-2,115)")
    print(f"  Total Wave 1+2: -5,549 (32.8% reduction)")
    
    # Sort by total findings, descending
    sorted_cats = sorted(
        cat_stats.items(),
        key=lambda x: -x[1]['total']
    )
    
    print("\n" + '-' * 110)
    print("HIGH-PRIORITY FP CANDIDATES (Untuned + High Finding Count)")
    print('-' * 110)
    print(f"{'Category':<35} {'Total':<8} {'Crit':<6} {'High':<6} {'Conf%':<8} {'Files':<8} {'Risk':<10}")
    print('-' * 110)
    
    # Flag candidates with high finding count but low confidence
    fp_candidates = []
    
    for cat, stats in sorted_cats:
        total = stats['total']
        if total == 0:
            continue
        
        is_tuned = cat in tuned
        conf_pct = stats['conf_pct']
        crit = stats['critical']
        high = stats['high']
        files = stats['files']
        actionable = crit + high
        
        # FP risk scoring: untuned + high volume + low confidence
        fp_risk = 'NONE'
        if not is_tuned:
            if total > 3000:
                if conf_pct < 40:
                    fp_risk = 'CRITICAL'
                elif conf_pct < 60:
                    fp_risk = 'HIGH'
                else:
                    fp_risk = 'MEDIUM'
            elif total > 1000:
                if conf_pct < 30:
                    fp_risk = 'HIGH'
                else:
                    fp_risk = 'MEDIUM'
            elif total > 500:
                if conf_pct < 25:
                    fp_risk = 'MEDIUM'
        
        # Print all untuned ones
        if not is_tuned:
            status = "[TUNED]" if is_tuned else "[UNTUNED]"
            print(f"{cat:<35} {total:<8,} {crit:<6} {high:<6} {conf_pct:<8}% {files:<8} {fp_risk:<10}")
            if fp_risk in ('CRITICAL', 'HIGH'):
                fp_candidates.append((cat, total, conf_pct, fp_risk))
    
    print("\n" + '-' * 110)
    print("RECOMMENDED NEXT TUNING WAVE (Wave 3)")
    print('-' * 110)
    
    # Sort by volume and risk
    fp_candidates.sort(key=lambda x: -x[1])
    
    if not fp_candidates:
        print("  No high-priority FP candidates identified.")
    else:
        print(f"\n  Total candidates: {len(fp_candidates)}\n")
        for i, (cat, total, conf_pct, risk) in enumerate(fp_candidates[:10], 1):
            print(f"  {i:2d}. {cat:<35} {total:>6,} findings (conf={conf_pct:>3}%) [{risk}]")
    
    # Estimate impact
    print("\n" + '-' * 110)
    print("POTENTIAL IMPACT (If candidates achieve 30% FP reduction like Wave 2 avg)")
    print('-' * 110)
    
    total_candidate_volume = sum(c[1] for c in fp_candidates)
    potential_reduction = int(total_candidate_volume * 0.30)
    new_total = summary['total_gaps'] - potential_reduction
    
    print(f"  Current total:        {summary['total_gaps']:,}")
    print(f"  Candidate volume:     {total_candidate_volume:,}")
    print(f"  Est. reduction (-30%): {potential_reduction:,}")
    print(f"  New total:            {new_total:,}")
    print(f"  Reduction pct:        {100*potential_reduction//summary['total_gaps']}%")
    
    # Scanner-level recommendations
    print("\n" + '-' * 110)
    print("SCANNER TUNING RECOMMENDATIONS FOR WAVE 3")
    print('-' * 110)
    
    recs = {
        'container': [
            '- Check for false-positive STL iterator patterns (push_back + iteration)',
            '- Verify reserve() heuristics: false triggers on vector pre-allocation',
            '- Audit move-semantics patterns: moving from containers that are still alive'
        ],
        'performance_patterns': [
            '- Expand lock detection context beyond loop-head patterns',
            '- Review "slow operations in loop" for library calls that are intrinsically slow',
            '- Check for macro-hidden performance patterns (e.g., macros containing loops)'
        ],
        'llm_ai_safety': [
            '- Phase 9 scanner new - likely has overly broad heuristics',
            '- Verify model-load patterns: not all external model loads are unsafe',
            '- Check token-validation patterns: false triggers on format validation'
        ],
        'reliability': [
            '- Review exception handling: constructor cleanup vs legitimately uncaught',
            '- Verify timeout patterns: config/tuning timeouts vs missing timeout checks'
        ],
        'exception_safety': [
            '- Check move-constructor patterns: legitimate exception-safe moves flagged as unsafe',
            '- Verify swap patterns: custom exception-safe swaps not recognized'
        ],
        'platform': [
            '- Audit OS-specific code detection: preprocessor conditionals may hide platform checks',
            '- Verify Windows/Linux-specific patterns: may fire on intentional dual-path code'
        ]
    }
    
    for cat in [c[0] for c in fp_candidates[:5]]:
        if cat in recs:
            print(f"\n  {cat.upper()}:")
            for rec in recs[cat]:
                print(f"    {rec}")
    
    print("\n" + '=' * 110)

if __name__ == '__main__':
    main()
