#!/usr/bin/env python3
"""Aggregate and analyze full 4-directory scan with impact classification"""

import json
import sys
from pathlib import Path
from collections import Counter, defaultdict
from datetime import datetime

def load_scan(filepath):
    """Load JSON scan results"""
    if not Path(filepath).exists():
        return None
    with open(filepath) as f:
        return json.load(f)

def analyze_all_scans():
    """Aggregate results from all 4 directory scans"""
    
    scan_files = [
        'ai_working/scan_src_2026_06_21.json',
        'ai_working/scan_include_2026_06_21.json',
        'ai_working/scan_tests_2026_06_21.json',
        'ai_working/scan_benchmarks_2026_06_21.json',
    ]
    
    all_gaps = []
    scan_results = {}
    
    print(f"\n{'='*90}")
    print(f"THEMISDB FULL CODEBASE ANALYSIS - Impact-Based Classification")
    print(f"{'='*90}\n")
    
    # Load all scans
    print("📂 Loading scan results...")
    for scan_file in scan_files:
        data = load_scan(scan_file)
        if data:
            gaps = data.get('gaps', [])
            dir_name = scan_file.split('scan_')[1].split('_2026')[0]
            scan_results[dir_name] = gaps
            all_gaps.extend(gaps)
            print(f"  ✓ {dir_name:12s}: {len(gaps):6d} findings")
    
    if not all_gaps:
        print("❌ No scan results found")
        return
    
    print(f"\n  {'='*85}")
    print(f"  TOTAL: {len(all_gaps):6d} findings across all directories")
    
    # 1. Summary Statistics
    print(f"\n{'='*90}")
    print("1. SEVERITY DISTRIBUTION")
    print(f"{'='*90}")
    
    severity_counts = Counter(g.get('severity', 'UNKNOWN') for g in all_gaps)
    for sev in ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW']:
        count = severity_counts.get(sev, 0)
        pct = 100.0 * count / len(all_gaps) if all_gaps else 0
        print(f"  {sev:10s}: {count:6d} ({pct:5.1f}%)")
    
    # 2. Impact Distribution
    print(f"\n{'='*90}")
    print("2. IMPACT DISTRIBUTION")
    print(f"{'='*90}")
    
    impact_counts = Counter(g.get('impact_level', 'UNKNOWN') for g in all_gaps)
    for imp in ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW', 'THIRD_PARTY']:
        count = impact_counts.get(imp, 0)
        pct = 100.0 * count / len(all_gaps) if all_gaps else 0
        print(f"  {imp:12s}: {count:6d} ({pct:5.1f}%)")
    
    # 3. Severity × Impact Matrix
    print(f"\n{'='*90}")
    print("3. SEVERITY × IMPACT MATRIX (ALL FINDINGS)")
    print(f"{'='*90}\n")
    
    severity_levels = ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW']
    impact_levels = ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW', 'THIRD_PARTY']
    
    matrix = defaultdict(lambda: defaultdict(int))
    for g in all_gaps:
        sev = g.get('severity', 'UNKNOWN')
        imp = g.get('impact_level', 'UNKNOWN')
        matrix[sev][imp] += 1
    
    print('             ', ' '.join(f'{x:>10s}' for x in impact_levels))
    for sev in severity_levels:
        row = ' '.join(f'{matrix[sev][imp]:>10d}' for imp in impact_levels)
        print(f'{sev:>10s}  {row}')
    
    # 4. AI-Vibe Findings Only
    print(f"\n{'='*90}")
    print("4. AI-VIBE FINDINGS (TODO, Stub, LLM, Error Handling, Header Drift)")
    print(f"{'='*90}\n")
    
    ai_vibe_keywords = ['todo_', 'simulation_', 'stub_', 'hardcoded_llm', 'llm_prompt', 'unchecked_result', 'missing_doxygen']
    ai_vibe = [g for g in all_gaps if any(k in g.get('type','').lower() for k in ai_vibe_keywords)]
    
    print(f"Total AI-Vibe Findings: {len(ai_vibe)}")
    
    if ai_vibe:
        print('\n             ', ' '.join(f'{x:>10s}' for x in impact_levels))
        ai_matrix = defaultdict(lambda: defaultdict(int))
        for g in ai_vibe:
            sev = g.get('severity', 'UNKNOWN')
            imp = g.get('impact_level', 'UNKNOWN')
            ai_matrix[sev][imp] += 1
        
        for sev in severity_levels:
            row = ' '.join(f'{ai_matrix[sev][imp]:>10d}' for imp in impact_levels)
            print(f'{sev:>10s}  {row}')
    
    # 5. Critical Path Analysis
    print(f"\n{'='*90}")
    print("5. CRITICAL PATH ANALYSIS (Priority Tiers)")
    print(f"{'='*90}\n")
    
    priorities = [
        ('CRITICAL', 'CRITICAL', '🔴 P0 CRITICAL/CRITICAL', 'Highest Risk - MUST FIX IMMEDIATELY'),
        ('CRITICAL', 'HIGH', '🟠 P0 CRITICAL/HIGH', 'Very High Risk - Fix This Sprint'),
        ('HIGH', 'CRITICAL', '🟡 P1 HIGH/CRITICAL', 'High Risk - High Impact Module'),
        ('HIGH', 'HIGH', '🟡 P1 HIGH/HIGH', 'Significant Risk'),
    ]
    
    for sev, imp, icon, desc in priorities:
        count = sum(1 for g in all_gaps if g.get('severity')==sev and g.get('impact_level')==imp)
        pct = 100.0 * count / len(all_gaps) if all_gaps else 0
        print(f"{icon:20s}: {count:6d} ({pct:5.2f}%) - {desc}")
        
        if count > 0 and count <= 10:
            subset = [g for g in all_gaps if g.get('severity')==sev and g.get('impact_level')==imp]
            for g in subset[:3]:
                subsys = g.get('subsystem', '?')
                print(f"    • {g['type'][:35]:35s} {g['file']:35s}:{g['line']:4d} [{subsys}]")
    
    # 6. Subsystem Breakdown
    print(f"\n{'='*90}")
    print("6. SUBSYSTEM BREAKDOWN (By Impact × Count)")
    print(f"{'='*90}\n")
    
    subsys_stats = defaultdict(lambda: defaultdict(int))
    for g in all_gaps:
        subsys = g.get('subsystem', 'UNKNOWN')
        imp = g.get('impact_level', 'UNKNOWN')
        subsys_stats[subsys][imp] += 1
    
    subsys_sorted = sorted(subsys_stats.keys(), key=lambda x: -sum(subsys_stats[x].values()))
    for subsys in subsys_sorted[:20]:  # Top 20 subsystems
        total = sum(subsys_stats[subsys].values())
        
        # Show breakdown by impact
        breakdown = []
        for imp in ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW']:
            count = subsys_stats[subsys][imp]
            if count > 0:
                breakdown.append(f"{imp}:{count}")
        
        print(f"  {subsys:20s}: {total:6d}  [{', '.join(breakdown)}]")
    
    # 7. Top Finding Types
    print(f"\n{'='*90}")
    print("7. TOP 20 FINDING TYPES")
    print(f"{'='*90}\n")
    
    type_counts = Counter(g.get('type', '?') for g in all_gaps)
    for gtype, count in type_counts.most_common(20):
        pct = 100.0 * count / len(all_gaps) if all_gaps else 0
        print(f"  {gtype:40s}: {count:6d} ({pct:5.1f}%)")
    
    # 8. Per-Directory Summary
    print(f"\n{'='*90}")
    print("8. PER-DIRECTORY SUMMARY")
    print(f"{'='*90}\n")
    
    for dir_name, gaps in sorted(scan_results.items()):
        if not gaps:
            continue
        
        sev_dist = Counter(g.get('severity', 'UNKNOWN') for g in gaps)
        imp_dist = Counter(g.get('impact_level', 'UNKNOWN') for g in gaps)
        ai_vibe_count = sum(1 for g in gaps if any(k in g.get('type','').lower() for k in ai_vibe_keywords))
        
        print(f"\n{dir_name.upper()}:")
        print(f"  Total: {len(gaps):6d}")
        print(f"  AI-Vibe: {ai_vibe_count:6d}")
        print(f"  Severity: CRIT={sev_dist['CRITICAL']} HIGH={sev_dist['HIGH']} MED={sev_dist['MEDIUM']} LOW={sev_dist['LOW']}")
        print(f"  Impact: CRIT={imp_dist['CRITICAL']} HIGH={imp_dist['HIGH']} MED={imp_dist['MEDIUM']} LOW={imp_dist['LOW']} 3P={imp_dist['THIRD_PARTY']}")
    
    # 9. Save Summary JSON
    print(f"\n{'='*90}")
    print("9. SAVING SUMMARY REPORT")
    print(f"{'='*90}\n")
    
    summary = {
        'timestamp': datetime.now().isoformat(),
        'total_findings': len(all_gaps),
        'ai_vibe_findings': len(ai_vibe),
        'severity_distribution': dict(severity_counts),
        'impact_distribution': dict(impact_counts),
        'critical_path': {
            'critical_critical': sum(1 for g in all_gaps if g.get('severity')=='CRITICAL' and g.get('impact_level')=='CRITICAL'),
            'critical_high': sum(1 for g in all_gaps if g.get('severity')=='CRITICAL' and g.get('impact_level')=='HIGH'),
            'high_critical': sum(1 for g in all_gaps if g.get('severity')=='HIGH' and g.get('impact_level')=='CRITICAL'),
            'high_high': sum(1 for g in all_gaps if g.get('severity')=='HIGH' and g.get('impact_level')=='HIGH'),
        },
        'top_subsystems': dict(subsys_sorted[:10]),
        'per_directory': {k: len(v) for k, v in scan_results.items()},
    }
    
    with open('ai_working/full_scan_summary_2026_06_21.json', 'w') as f:
        json.dump(summary, f, indent=2)
    
    print(f"  ✓ Saved: ai_working/full_scan_summary_2026_06_21.json")
    
    # 10. Print final recommendations
    print(f"\n{'='*90}")
    print("10. ACTIONABLE RECOMMENDATIONS")
    print(f"{'='*90}\n")
    
    p0_count = summary['critical_path']['critical_critical']
    p1_count = summary['critical_path']['critical_high'] + summary['critical_path']['high_critical']
    
    print(f"🔴 P0 (CRITICAL×CRITICAL): {p0_count:3d} - FIX IMMEDIATELY (Blocks release)")
    print(f"🟠 P1 (CRITICAL×HIGH + HIGH×CRITICAL): {p1_count:3d} - FIX THIS SPRINT")
    print(f"🟡 P2 (HIGH×HIGH): {summary['critical_path']['high_high']:3d} - FIX NEXT SPRINT")
    print(f"⚪ P3+ (Others): {len(all_gaps) - p0_count - p1_count - summary['critical_path']['high_high']:3d} - BACKLOG")
    
    print(f"\n📊 AI-Vibe Focus: {len(ai_vibe)} findings from specialized scanners")
    print(f"   - Priority: Focus on P0-P1 AI-Vibe findings in CRITICAL/HIGH modules")
    
    print(f"\n✅ Analysis complete: {datetime.now().isoformat()}")

if __name__ == '__main__':
    analyze_all_scans()
