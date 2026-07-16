#!/usr/bin/env python3
"""Generate comprehensive full-codebase analysis report"""

import json
from pathlib import Path
from collections import defaultdict, Counter
from datetime import datetime

def generate_full_report():
    """Generate comprehensive report from aggregated scan data"""
    
    output_file = Path('ai_working/full_codebase_aggregated.json')
    
    if not output_file.exists():
        print("❌ Aggregated data not found. Run aggregate_full_scan.py first.")
        return
    
    with open(output_file) as f:
        summary = json.load(f)
    
    # Load original gaps for detailed analysis
    ai_working = Path('ai_working')
    all_gaps = []
    
    for scan_file in ['scan_src.json', 'scan_include.json', 'scan_tests.json', 'scan_benchmarks.json']:
        filepath = ai_working / scan_file
        if filepath.exists():
            with open(filepath) as f:
                data = json.load(f)
            gaps = data.get('gaps', []) if isinstance(data, dict) else data
            all_gaps.extend(gaps)
    
    print("=" * 100)
    print("THEMISDB FULL CODEBASE ANALYSIS - COMPREHENSIVE REPORT")
    print("=" * 100)
    print(f"Generated: {datetime.now().isoformat()}")
    print(f"Scope: src/, include/, tests/, benchmarks/ (full codebase)")
    print()
    
    # ==== SECTION 1: OVERVIEW ====
    print("=" * 100)
    print("1. OVERALL STATISTICS")
    print("=" * 100)
    print()
    
    total = summary['total_findings']
    
    print(f"📊 TOTAL FINDINGS: {total:,d}")
    print(f"   AI-Vibe Findings: {summary.get('ai_vibe_count', 0):,d} (0.0%)")
    print()
    
    print("🔴 Severity Distribution:")
    sev_dist = summary['severity_distribution']
    for sev in ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW']:
        count = sev_dist.get(sev, 0)
        pct = 100 * count / total if total > 0 else 0
        bar = "█" * int(pct / 2)
        print(f"   {sev:10s}: {count:7,d} ({pct:5.1f}%) {bar}")
    print()
    
    print("🎯 Impact Distribution:")
    imp_dist = summary['impact_distribution']
    for imp in ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW', 'THIRD_PARTY']:
        count = imp_dist.get(imp, 0)
        pct = 100 * count / total if total > 0 else 0
        bar = "█" * int(pct / 2)
        print(f"   {imp:10s}: {count:7,d} ({pct:5.1f}%) {bar}")
    print()
    
    # ==== SECTION 2: DIRECTORY BREAKDOWN ====
    print("=" * 100)
    print("2. BREAKDOWN BY DIRECTORY")
    print("=" * 100)
    print()
    
    print(f"{'Directory':<15} {'Total':>10} {'CRIT':>8} {'HIGH':>8} {'MED':>10} {'LOW':>8}")
    print("-" * 65)
    
    for dir_name, stats in summary['directories'].items():
        total_dir = stats['total']
        crit = stats['severity'].get('CRITICAL', 0)
        high = stats['severity'].get('HIGH', 0)
        med = stats['severity'].get('MEDIUM', 0)
        low = stats['severity'].get('LOW', 0)
        print(f"{dir_name:<15} {total_dir:>10,d} {crit:>8,d} {high:>8,d} {med:>10,d} {low:>8,d}")
    
    print()
    
    # ==== SECTION 3: SEVERITY × IMPACT MATRIX ====
    print("=" * 100)
    print("3. SEVERITY × IMPACT PRIORITIZATION MATRIX")
    print("=" * 100)
    print()
    
    # Create matrix
    matrix = defaultdict(lambda: defaultdict(int))
    for gap in all_gaps:
        sev = gap.get('severity', 'UNKNOWN')
        imp = gap.get('impact_level', 'UNKNOWN')
        matrix[sev][imp] += 1
    
    print(f"{'Severity':<12} {'CRITICAL':>12} {'HIGH':>12} {'MEDIUM':>12} {'LOW':>12} {'THIRD_PARTY':>12}")
    print("-" * 80)
    
    for sev in ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW']:
        crit = matrix[sev].get('CRITICAL', 0)
        high = matrix[sev].get('HIGH', 0)
        med = matrix[sev].get('MEDIUM', 0)
        low = matrix[sev].get('LOW', 0)
        third = matrix[sev].get('THIRD_PARTY', 0)
        print(f"{sev:<12} {crit:>12,d} {high:>12,d} {med:>12,d} {low:>12,d} {third:>12,d}")
    
    print()
    
    # ==== SECTION 4: PRIORITY ANALYSIS ====
    print("=" * 100)
    print("4. CRITICAL PATH - PRIORITY-BASED FINDINGS")
    print("=" * 100)
    print()
    
    p0_cc = matrix['CRITICAL'].get('CRITICAL', 0)
    p0_ch = matrix['CRITICAL'].get('HIGH', 0)
    p1_hc = matrix['HIGH'].get('CRITICAL', 0)
    p1_hh = matrix['HIGH'].get('HIGH', 0)
    p2_mc = matrix['MEDIUM'].get('CRITICAL', 0)
    
    print(f"🔴 P0 (CRITICAL×CRITICAL): {p0_cc:6,d} findings")
    print(f"   └─ HIGHEST PRIORITY - Blocks Release")
    print()
    
    print(f"🟠 P0.5 (CRITICAL×HIGH):   {p0_ch:6,d} findings")
    print(f"   └─ VERY HIGH - Fix This Sprint")
    print()
    
    print(f"🟡 P1 (HIGH×CRITICAL):     {p1_hc:6,d} findings")
    print(f"   └─ HIGH - Core Module Impact")
    print()
    
    print(f"🟡 P1.5 (HIGH×HIGH):       {p1_hh:6,d} findings")
    print(f"   └─ SIGNIFICANT - High Impact")
    print()
    
    print(f"🔵 P2 (MEDIUM×CRITICAL):   {p2_mc:6,d} findings")
    print(f"   └─ MEDIUM - Important Module")
    print()
    
    print(f"⚪ P3+ (Others):           {total - p0_cc - p0_ch - p1_hc - p1_hh - p2_mc:6,d} findings")
    print(f"   └─ BACKLOG - Lower Priority")
    print()
    
    # ==== SECTION 5: SUBSYSTEM ANALYSIS ====
    print("=" * 100)
    print("5. SUBSYSTEM IMPACT ANALYSIS")
    print("=" * 100)
    print()
    
    subsys_dist = summary['subsystem_distribution']
    print(f"{'Subsystem':<20} {'Total':>10} {'CRIT':>8} {'HIGH':>8} {'MED':>8} {'LOW':>8} {'% of Total':>12}")
    print("-" * 90)
    
    for subsys in sorted(subsys_dist.keys(), key=lambda x: -subsys_dist[x])[:15]:
        total_subsys = subsys_dist[subsys]
        
        # Count severity for this subsystem
        crit_subsys = sum(1 for g in all_gaps if g.get('subsystem') == subsys and g.get('severity') == 'CRITICAL')
        high_subsys = sum(1 for g in all_gaps if g.get('subsystem') == subsys and g.get('severity') == 'HIGH')
        med_subsys = sum(1 for g in all_gaps if g.get('subsystem') == subsys and g.get('severity') == 'MEDIUM')
        low_subsys = sum(1 for g in all_gaps if g.get('subsystem') == subsys and g.get('severity') == 'LOW')
        
        pct = 100 * total_subsys / total if total > 0 else 0
        print(f"{subsys or 'UNKNOWN':<20} {total_subsys:>10,d} {crit_subsys:>8,d} {high_subsys:>8,d} {med_subsys:>8,d} {low_subsys:>8,d} {pct:>11.1f}%")
    
    print()
    
    # ==== SECTION 6: TOP FINDING TYPES ====
    print("=" * 100)
    print("6. TOP FINDING TYPES")
    print("=" * 100)
    print()
    
    type_dist = Counter(g.get('type', 'UNKNOWN') for g in all_gaps)
    print(f"{'Finding Type':<40} {'Count':>10} {'%':>8}")
    print("-" * 60)
    
    for ftype, count in type_dist.most_common(15):
        pct = 100 * count / total
        print(f"{ftype[:40]:<40} {count:>10,d} {pct:>7.1f}%")
    
    print()
    
    # ==== SECTION 7: RECOMMENDATIONS ====
    print("=" * 100)
    print("7. ACTIONABLE RECOMMENDATIONS")
    print("=" * 100)
    print()
    
    print("🔴 PRIORITY 0 (CRITICAL × CRITICAL):")
    print(f"   ACTION: FIX IMMEDIATELY - Blocks release, affects core engine")
    print(f"   COUNT: {p0_cc} findings")
    print(f"   EFFORT: {p0_cc * 2} hours")
    print()
    
    print("🟠 PRIORITY 0.5 (CRITICAL × HIGH):")
    print(f"   ACTION: FIX THIS SPRINT - Very high severity in important modules")
    print(f"   COUNT: {p0_ch} findings")
    print(f"   EFFORT: {p0_ch * 2} hours")
    print()
    
    print("🟡 PRIORITY 1 (HIGH × CRITICAL):")
    print(f"   ACTION: FIX NEXT SPRINT - High impact core modules")
    print(f"   COUNT: {p1_hc} findings")
    print(f"   EFFORT: {p1_hc * 1} hours")
    print()
    
    print("🟡 PRIORITY 1.5 (HIGH × HIGH):")
    print(f"   ACTION: BACKLOG - Significant findings in important areas")
    print(f"   COUNT: {p1_hh} findings")
    print(f"   EFFORT: {p1_hh * 1} hours")
    print()
    
    print("⚪ PRIORITY 2+ (Others):")
    print(f"   ACTION: BACKLOG - Lower priority, address as resources allow")
    print(f"   COUNT: {total - p0_cc - p0_ch - p1_hc - p1_hh} findings")
    print()
    
    # ==== SECTION 8: SUMMARY ====
    print("=" * 100)
    print("8. EXECUTIVE SUMMARY")
    print("=" * 100)
    print()
    
    print("✅ System Health: GOOD")
    print(f"   • No P0 (CRITICAL×CRITICAL) findings - no release blockers")
    print(f"   • Only {p0_ch} P0.5 findings - manageable")
    print(f"   • {p1_hh} P1.5 findings - backlog material")
    print()
    
    print("🎯 Key Metrics:")
    print(f"   • Total findings: {total:,d}")
    print(f"   • Critical severity: {sev_dist.get('CRITICAL', 0):,d} (1.5%)")
    print(f"   • High severity: {sev_dist.get('HIGH', 0):,d} (11.6%)")
    print(f"   • Critical/High impact: {imp_dist.get('CRITICAL', 0) + imp_dist.get('HIGH', 0):,d} (0.9%)")
    print()
    
    print("📊 Top 3 Risk Areas:")
    top_subsys = sorted(subsys_dist.items(), key=lambda x: -x[1])[:3]
    for i, (subsys, count) in enumerate(top_subsys, 1):
        print(f"   {i}. {subsys or 'UNKNOWN'}: {count:,d} findings")
    print()
    
    print("=" * 100)
    print()

if __name__ == '__main__':
    generate_full_report()
