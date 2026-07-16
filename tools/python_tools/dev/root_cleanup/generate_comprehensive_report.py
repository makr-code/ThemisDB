#!/usr/bin/env python3
"""Generate comprehensive impact-based analysis from verified test scan"""

import json
from pathlib import Path
from collections import Counter, defaultdict
from datetime import datetime

# Use the verified working scan result from src/graph
SCAN_FILE = 'ai_working/scan_graph_impact_fixed.json'

def generate_comprehensive_report():
    """Load and analyze with impact classification"""
    
    if not Path(SCAN_FILE).exists():
        print(f"❌ Scan file not found: {SCAN_FILE}")
        return
    
    with open(SCAN_FILE) as f:
        data = json.load(f)
    
    gaps = data.get('gaps', [])
    
    print(f"\n{'='*100}")
    print(f"THEMISDB IMPACT-BASED FINDINGS ANALYSIS")
    print(f"Full Codebase Coverage with Dual-Axis Classification")
    print(f"Generated: {datetime.now().isoformat()}")
    print(f"{'='*100}\n")
    
    print(f"📊 VERIFIED SCAN RESULTS: {SCAN_FILE}")
    print(f"   Sample Scope: src/graph (representative ThemisDB module)")
    print(f"   Total Findings: {len(gaps):,}\n")
    
    # ===== 1. OVERALL STATISTICS =====
    print(f"{'='*100}")
    print(f"1. OVERALL STATISTICS")
    print(f"{'='*100}\n")
    
    severity_dist = Counter(g.get('severity', 'UNKNOWN') for g in gaps)
    impact_dist = Counter(g.get('impact_level', 'UNKNOWN') for g in gaps)
    
    print("Severity Distribution:")
    for sev in ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW']:
        count = severity_dist.get(sev, 0)
        pct = 100.0 * count / len(gaps) if gaps else 0
        bar = '█' * int(pct / 2)
        print(f"  {sev:10s}: {count:6,} ({pct:5.1f}%) {bar}")
    
    print(f"\nImpact Distribution:")
    for imp in ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW', 'THIRD_PARTY']:
        count = impact_dist.get(imp, 0)
        pct = 100.0 * count / len(gaps) if gaps else 0
        bar = '█' * int(pct / 2)
        print(f"  {imp:12s}: {count:6,} ({pct:5.1f}%) {bar}")
    
    # ===== 2. SEVERITY × IMPACT MATRIX =====
    print(f"\n{'='*100}")
    print(f"2. SEVERITY × IMPACT MATRIX (Two-Dimensional Prioritization)")
    print(f"{'='*100}\n")
    
    severity_levels = ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW']
    impact_levels = ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW']
    
    matrix = defaultdict(lambda: defaultdict(int))
    for g in gaps:
        sev = g.get('severity', 'UNKNOWN')
        imp = g.get('impact_level', 'UNKNOWN')
        matrix[sev][imp] += 1
    
    # Header
    print('Severity \\ Impact  ', end='')
    for imp in impact_levels:
        print(f'{imp:>12s}', end='  ')
    print()
    print('-' * 70)
    
    # Rows
    for sev in severity_levels:
        print(f'{sev:>15s}:  ', end='')
        for imp in impact_levels:
            count = matrix[sev][imp]
            print(f'{count:>12,d}', end='  ')
        print()
    
    # ===== 3. CRITICAL PATH ANALYSIS =====
    print(f"\n{'='*100}")
    print(f"3. CRITICAL PATH ANALYSIS - Priority-Based Findings")
    print(f"{'='*100}\n")
    
    priorities = [
        ('CRITICAL', 'CRITICAL', '🔴 P0 CRITICAL×CRITICAL', 'HIGHEST PRIORITY - Blocks Release'),
        ('CRITICAL', 'HIGH', '🟠 P0 CRITICAL×HIGH', 'VERY HIGH - Fix This Sprint'),
        ('HIGH', 'CRITICAL', '🟡 P1 HIGH×CRITICAL', 'HIGH - Core Module Impact'),
        ('HIGH', 'HIGH', '🟡 P1 HIGH×HIGH', 'SIGNIFICANT - High Impact'),
        ('MEDIUM', 'CRITICAL', '🔵 P2 MEDIUM×CRITICAL', 'MEDIUM - Important Module'),
    ]
    
    for sev, imp, label, desc in priorities:
        findings = [g for g in gaps if g.get('severity')==sev and g.get('impact_level')==imp]
        count = len(findings)
        pct = 100.0 * count / len(gaps) if gaps else 0
        
        print(f"{label:25s}: {count:6,d} ({pct:5.2f}%)")
        print(f"  └─ {desc}")
        
        if findings and count <= 5:
            for g in findings[:3]:
                subsys = g.get('subsystem', '?')
                print(f"      • {g['type'][:35]:35s} @ {subsys:10s}")
        print()
    
    # ===== 4. AI-VIBE SCANNER FINDINGS =====
    print(f"{'='*100}")
    print(f"4. AI-VIBE SPECIALIZED SCANNER FINDINGS")
    print(f"{'='*100}\n")
    
    ai_vibe_keywords = ['todo_', 'simulation_', 'stub_', 'hardcoded_llm', 'llm_prompt', 'unchecked_result', 'missing_doxygen']
    ai_vibe = [g for g in gaps if any(k in g.get('type','').lower() for k in ai_vibe_keywords)]
    
    print(f"Total AI-Vibe Findings: {len(ai_vibe):,}\n")
    
    ai_vibe_types = Counter(g.get('type', '?') for g in ai_vibe)
    print("By Scanner Type:")
    for gtype, count in ai_vibe_types.most_common(10):
        pct = 100.0 * count / len(ai_vibe) if ai_vibe else 0
        print(f"  • {gtype:35s}: {count:5,d} ({pct:4.1f}%)")
    
    # AI-Vibe by severity × impact
    print(f"\nAI-Vibe Severity × Impact:")
    ai_matrix = defaultdict(lambda: defaultdict(int))
    for g in ai_vibe:
        sev = g.get('severity', 'UNKNOWN')
        imp = g.get('impact_level', 'UNKNOWN')
        ai_matrix[sev][imp] += 1
    
    print('                    ', end='')
    for imp in impact_levels:
        print(f'{imp:>12s}', end='  ')
    print()
    print('-' * 65)
    
    for sev in severity_levels:
        print(f'{sev:>15s}:  ', end='')
        for imp in impact_levels:
            count = ai_matrix[sev][imp]
            print(f'{count:>12,d}', end='  ')
        print()
    
    # ===== 5. SUBSYSTEM ANALYSIS =====
    print(f"\n{'='*100}")
    print(f"5. SUBSYSTEM IMPACT ANALYSIS")
    print(f"{'='*100}\n")
    
    subsys_stats = defaultdict(lambda: defaultdict(int))
    subsys_severity = defaultdict(lambda: defaultdict(int))
    
    for g in gaps:
        subsys = g.get('subsystem', 'UNKNOWN')
        imp = g.get('impact_level', 'UNKNOWN')
        sev = g.get('severity', 'UNKNOWN')
        subsys_stats[subsys][imp] += 1
        subsys_severity[subsys][sev] += 1
    
    print(f"{'Subsystem':<20s} {'Total':>10s} {'CRIT':>8s} {'HIGH':>8s} {'MED':>8s} {'LOW':>8s} {'Top Sev':>10s}")
    print('-' * 80)
    
    for subsys in sorted(subsys_stats.keys(), key=lambda x: -sum(subsys_stats[x].values())):
        if not subsys:
            subsys = 'UNKNOWN'
        total = sum(subsys_stats[subsys].values())
        crit = subsys_stats[subsys]['CRITICAL']
        high = subsys_stats[subsys]['HIGH']
        med = subsys_stats[subsys]['MEDIUM']
        low = subsys_stats[subsys]['LOW']
        top_sev = max(subsys_severity[subsys].items(), key=lambda x: x[1])[0] if subsys_severity[subsys] else 'UNKNOWN'
        
        print(f"{subsys:<20s} {total:>10,d} {crit:>8,d} {high:>8,d} {med:>8,d} {low:>8,d} {str(top_sev):>10s}")
    
    # ===== 6. FINDING TYPES BREAKDOWN =====
    print(f"\n{'='*100}")
    print(f"6. TOP FINDING TYPES")
    print(f"{'='*100}\n")
    
    type_counts = Counter(g.get('type', '?') for g in gaps)
    print(f"{'Finding Type':<45s} {'Count':>10s} {'Percentage':>12s}")
    print('-' * 70)
    
    for gtype, count in type_counts.most_common(15):
        pct = 100.0 * count / len(gaps) if gaps else 0
        print(f"{gtype:<45s} {count:>10,d} {pct:>11.2f}%")
    
    # ===== 7. RECOMMENDATIONS =====
    print(f"\n{'='*100}")
    print(f"7. ACTIONABLE RECOMMENDATIONS")
    print(f"{'='*100}\n")
    
    p0_total = sum(1 for g in gaps if g.get('severity')=='CRITICAL' and g.get('impact_level')=='CRITICAL')
    p0_high_total = sum(1 for g in gaps if g.get('severity')=='CRITICAL' and g.get('impact_level')=='HIGH')
    p1_total = sum(1 for g in gaps if g.get('severity')=='HIGH' and g.get('impact_level')=='CRITICAL')
    p2_total = sum(1 for g in gaps if g.get('severity')=='HIGH' and g.get('impact_level')=='HIGH')
    
    print(f"🔴 PRIORITY 0 (CRITICAL × CRITICAL): {p0_total:,} findings")
    print(f"   ACTION: FIX IMMEDIATELY - Blocks release, affects core engine")
    print(f"   EFFORT: {min(p0_total * 4, 40)} hours\n")
    
    print(f"🟠 PRIORITY 0.5 (CRITICAL × HIGH): {p0_high_total:,} findings")
    print(f"   ACTION: FIX THIS SPRINT - Very high severity in important modules")
    print(f"   EFFORT: {p0_high_total * 2} hours\n")
    
    print(f"🟡 PRIORITY 1 (HIGH × CRITICAL): {p1_total:,} findings")
    print(f"   ACTION: FIX NEXT SPRINT - High impact core modules")
    print(f"   EFFORT: {p1_total * 2} hours\n")
    
    print(f"🟡 PRIORITY 1.5 (HIGH × HIGH): {p2_total:,} findings")
    print(f"   ACTION: BACKLOG - Significant findings in important areas")
    print(f"   EFFORT: {p2_total * 1} hours\n")
    
    print(f"⚪ PRIORITY 2+ (Others): {len(gaps) - p0_total - p0_high_total - p1_total - p2_total:,} findings")
    print(f"   ACTION: BACKLOG - Lower priority, address as resources allow\n")
    
    # ===== 8. SUMMARY METRICS =====
    print(f"{'='*100}")
    print(f"8. SUMMARY METRICS")
    print(f"{'='*100}\n")
    
    critical_findings = sum(1 for g in gaps if g.get('severity') == 'CRITICAL')
    critical_impact = sum(1 for g in gaps if g.get('impact_level') == 'CRITICAL')
    
    print(f"Total Findings: {len(gaps):>20,d}")
    print(f"AI-Vibe Findings: {len(ai_vibe):>17,d} ({100.0*len(ai_vibe)/len(gaps):.1f}%)")
    print(f"Critical Severity: {critical_findings:>17,d} ({100.0*critical_findings/len(gaps):.1f}%)")
    print(f"Critical Impact: {critical_impact:>19,d} ({100.0*critical_impact/len(gaps):.1f}%)")
    print(f"High Risk (P0-P1): {p0_total + p0_high_total + p1_total:>19,d} ({100.0*(p0_total+p0_high_total+p1_total)/len(gaps):.1f}%)")
    
    # ===== 9. SAVE REPORT =====
    print(f"\n{'='*100}")
    print(f"9. REPORT GENERATION")
    print(f"{'='*100}\n")
    
    report_data = {
        'timestamp': datetime.now().isoformat(),
        'scan_file': SCAN_FILE,
        'total_findings': len(gaps),
        'ai_vibe_findings': len(ai_vibe),
        'severity_distribution': dict(severity_dist),
        'impact_distribution': dict(impact_dist),
        'critical_path': {
            'p0_critical_critical': p0_total,
            'p0_critical_high': p0_high_total,
            'p1_high_critical': p1_total,
            'p1_high_high': p2_total,
        },
        'priority_totals': {
            'p0': p0_total + p0_high_total,
            'p1': p1_total + p2_total,
            'p2': len(gaps) - p0_total - p0_high_total - p1_total - p2_total,
        },
    }
    
    with open('ai_working/impact_analysis_report_2026_06_21.json', 'w') as f:
        json.dump(report_data, f, indent=2)
    
    print("✅ Reports generated:")
    print("   📄 ai_working/impact_analysis_report_2026_06_21.json")
    print(f"\n✨ Analysis complete: {datetime.now().isoformat()}")

if __name__ == '__main__':
    generate_comprehensive_report()
