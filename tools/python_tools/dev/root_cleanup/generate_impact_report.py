#!/usr/bin/env python3
"""Generate comprehensive impact-based analysis report"""

import json
from collections import Counter, defaultdict
from pathlib import Path

def load_scan(filepath):
    """Load JSON scan results"""
    if not Path(filepath).exists():
        print(f"⚠️ Scan file not found: {filepath}")
        return None
    with open(filepath) as f:
        return json.load(f)

def generate_report(scan_data):
    """Generate multi-dimensional analysis report"""
    if not scan_data:
        return
    
    gaps = scan_data.get('gaps', [])
    print(f"\n{'='*80}")
    print(f"THEMISDB IMPACT-BASED FINDINGS ANALYSIS")
    print(f"{'='*80}")
    print(f"\nTotal Findings: {len(gaps)}")
    
    # 1. By Severity × Impact Matrix
    print(f"\n{'='*80}")
    print("1. SEVERITY × IMPACT MATRIX")
    print(f"{'='*80}")
    
    severity_levels = ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW']
    impact_levels = ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW', 'THIRD_PARTY']
    
    matrix = defaultdict(lambda: defaultdict(int))
    for g in gaps:
        sev = g.get('severity', 'MEDIUM')
        imp = g.get('impact_level', 'UNKNOWN')
        matrix[sev][imp] += 1
    
    print('\n               IMPACT LEVEL')
    print('              ' + '   '.join(f'{x:>10s}' for x in impact_levels))
    for sev in severity_levels:
        row = ' '.join(f'{matrix[sev][imp]:>10d}' for imp in impact_levels)
        print(f'{sev:>10s}  {row}')
    
    # 2. Critical Path Analysis
    print(f"\n{'='*80}")
    print("2. CRITICAL PATH ANALYSIS (Severity × Impact Priority)")
    print(f"{'='*80}")
    
    priorities = [
        ('CRITICAL', 'CRITICAL', '🔴 CRITICAL', 'Highest Risk - Must Fix'),
        ('CRITICAL', 'HIGH', '🟠 CRITICAL/HIGH', 'Very High Risk'),
        ('HIGH', 'CRITICAL', '🟡 HIGH/CRITICAL', 'High Risk'),
        ('HIGH', 'HIGH', '🟡 HIGH/HIGH', 'Significant Risk'),
    ]
    
    for sev, imp, icon, desc in priorities:
        count = sum(1 for g in gaps if g.get('severity')==sev and g.get('impact_level')==imp)
        print(f"\n{icon} {sev} Severity × {imp} Impact: {count} findings")
        print(f"   {desc}")
        if count > 0:
            subset = [g for g in gaps if g.get('severity')==sev and g.get('impact_level')==imp]
            for g in subset[:2]:
                print(f"   • {g['type']:40s} {g['file']:30s}:{g['line']}")
    
    # 3. Subsystem Breakdown
    print(f"\n{'='*80}")
    print("3. SUBSYSTEM BREAKDOWN")
    print(f"{'='*80}")
    
    subsys_stats = defaultdict(lambda: defaultdict(int))
    subsys_severity = defaultdict(lambda: defaultdict(int))
    
    for g in gaps:
        subsys = g.get('subsystem', 'UNKNOWN')
        imp = g.get('impact_level', 'UNKNOWN')
        sev = g.get('severity', 'MEDIUM')
        subsys_stats[subsys][imp] += 1
        subsys_severity[subsys][sev] += 1
    
    for subsys in sorted(subsys_stats.keys(), key=lambda x: -sum(subsys_stats[x].values())):
        total = sum(subsys_stats[subsys].values())
        print(f"\n{subsys.upper():20s}: {total:5d} findings")
        
        # Show by impact
        for imp in ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW']:
            count = subsys_stats[subsys][imp]
            if count > 0:
                print(f"  ├─ {imp:10s}: {count:3d}")
        
        # Show top severity
        top_sev = max(subsys_severity[subsys].items(), key=lambda x: x[1])[0]
        print(f"  └─ Top Severity: {top_sev}")
    
    # 4. Top Finding Types by Subsystem
    print(f"\n{'='*80}")
    print("4. TOP FINDING TYPES BY SUBSYSTEM")
    print(f"{'='*80}")
    
    subsys_types = defaultdict(lambda: Counter())
    for g in gaps:
        subsys = g.get('subsystem', 'UNKNOWN')
        gtype = g.get('type', '?')
        subsys_types[subsys][gtype] += 1
    
    for subsys in sorted(subsys_types.keys(), key=lambda x: -sum(subsys_types[x].values())):
        print(f"\n{subsys.upper()}:")
        for gtype, count in subsys_types[subsys].most_common(5):
            print(f"  • {gtype:40s}: {count:4d}")
    
    # 5. AI-Vibe Findings Summary
    print(f"\n{'='*80}")
    print("5. AI-VIBE FINDINGS (TODO, Simulation, Stub, etc.)")
    print(f"{'='*80}")
    
    ai_vibe_keywords = ['todo_', 'simulation_', 'stub_', 'hardcoded_llm', 'llm_prompt', 'unchecked_result']
    ai_vibe = [g for g in gaps if any(k in g.get('type','') for k in ai_vibe_keywords)]
    
    print(f"\nTotal AI-Vibe Findings: {len(ai_vibe)}")
    
    if ai_vibe:
        # Matrix for AI-Vibe only
        ai_matrix = defaultdict(lambda: defaultdict(int))
        for g in ai_vibe:
            sev = g.get('severity', 'MEDIUM')
            imp = g.get('impact_level', 'UNKNOWN')
            ai_matrix[sev][imp] += 1
        
        print('\n               IMPACT LEVEL')
        print('              ' + '   '.join(f'{x:>10s}' for x in impact_levels))
        for sev in severity_levels:
            row = ' '.join(f'{ai_matrix[sev][imp]:>10d}' for imp in impact_levels)
            print(f'{sev:>10s}  {row}')
        
        # Top AI-Vibe types
        print(f"\nTop AI-Vibe Types:")
        ai_types = Counter(g.get('type','') for g in ai_vibe)
        for gtype, count in ai_types.most_common(10):
            print(f"  • {gtype:40s}: {count:3d}")

if __name__ == '__main__':
    # Try the latest scan
    scans = [
        'ai_working/scan_src_with_impact.json',
        'ai_working/scan_graph_impact_fixed.json',
    ]
    
    for scan_file in scans:
        data = load_scan(scan_file)
        if data:
            print(f"\n📊 Analyzing: {scan_file}")
            generate_report(data)
            break
    else:
        print("❌ No scan results found")
