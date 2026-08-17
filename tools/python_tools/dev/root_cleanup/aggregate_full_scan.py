#!/usr/bin/env python3
"""Aggregate results from full 4-directory scan (src, include, tests, benchmarks)"""

import json
from pathlib import Path
from collections import defaultdict, Counter

def load_scan_file(filepath):
    """Load scan JSON file and extract gaps"""
    with open(filepath) as f:
        data = json.load(f)
    
    if isinstance(data, dict) and 'gaps' in data:
        return data['gaps']
    elif isinstance(data, list):
        return data
    return []

def aggregate_scans():
    """Aggregate all scan results"""
    ai_working = Path('ai_working')
    
    # Map directory names to scan files
    scan_mapping = {
        'src': 'scan_src.json',
        'include': 'scan_include.json',
        'tests': 'scan_tests.json',
        'benchmarks': 'scan_benchmarks.json'
    }
    
    all_gaps = []
    dir_stats = {}
    
    print("=" * 100)
    print("THEMISDB FULL CODEBASE SCAN AGGREGATION")
    print("=" * 100)
    print()
    
    for dir_name, filename in scan_mapping.items():
        filepath = ai_working / filename
        if filepath.exists():
            gaps = load_scan_file(filepath)
            all_gaps.extend(gaps)
            
            # Stats per directory
            severity_dist = Counter(g.get('severity', 'UNKNOWN') for g in gaps)
            impact_dist = Counter(g.get('impact_level', 'UNKNOWN') for g in gaps)
            
            dir_stats[dir_name] = {
                'total': len(gaps),
                'severity': dict(severity_dist),
                'impact': dict(impact_dist)
            }
            
            print(f"✅ {dir_name:12s}: {len(gaps):6,d} findings")
            print(f"   Severity: CRIT={severity_dist.get('CRITICAL', 0):4d} HIGH={severity_dist.get('HIGH', 0):4d} MED={severity_dist.get('MEDIUM', 0):4d} LOW={severity_dist.get('LOW', 0):4d}")
            print(f"   Impact:   CRIT={impact_dist.get('CRITICAL', 0):4d} HIGH={impact_dist.get('HIGH', 0):4d} MED={impact_dist.get('MEDIUM', 0):4d} LOW={impact_dist.get('LOW', 0):4d}")
            print()
        else:
            print(f"❌ {dir_name:12s}: File not found - {filename}")
    
    # Overall aggregation
    print("=" * 100)
    print("AGGREGATED RESULTS")
    print("=" * 100)
    
    total_findings = len(all_gaps)
    severity_dist = Counter(g.get('severity', 'UNKNOWN') for g in all_gaps)
    impact_dist = Counter(g.get('impact_level', 'UNKNOWN') for g in all_gaps)
    ai_vibe_keywords = (
        'todo_',
        'simulation_',
        'stub_',
        'unvalidated_llm_',
        'unsanitized_llm_',
        'unchecked_result',
        'missing_doxygen',
        'llm_prompt',
        'error_handling',
        'header_drift',
    )
    ai_vibe_count = sum(
        1
        for g in all_gaps
        if any(k in str(g.get('type', '')).lower() for k in ai_vibe_keywords)
    )
    subsystem_dist = Counter(g.get('subsystem', 'UNKNOWN') for g in all_gaps)
    
    print(f"\n📊 TOTAL FINDINGS: {total_findings:,d}")
    print(f"   AI-Vibe Findings: {ai_vibe_count:,d} ({100*ai_vibe_count/total_findings:.1f}%)")
    
    print(f"\n🔴 Severity Distribution:")
    for sev in ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW']:
        count = severity_dist.get(sev, 0)
        pct = 100 * count / total_findings if total_findings > 0 else 0
        print(f"   {sev:10s}: {count:7,d} ({pct:5.1f}%)")
    
    print(f"\n🎯 Impact Distribution:")
    for imp in ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW', 'THIRD_PARTY']:
        count = impact_dist.get(imp, 0)
        pct = 100 * count / total_findings if total_findings > 0 else 0
        print(f"   {imp:10s}: {count:7,d} ({pct:5.1f}%)")
    
    # Priority tiers
    print(f"\n⚠️  Priority Analysis:")
    p0 = sum(1 for g in all_gaps if g.get('severity') == 'CRITICAL' and g.get('impact_level') in ['CRITICAL', 'HIGH'])
    p1 = sum(1 for g in all_gaps if g.get('severity') == 'HIGH' and g.get('impact_level') in ['CRITICAL', 'HIGH'])
    p2 = sum(1 for g in all_gaps if g.get('severity') in ['CRITICAL', 'HIGH'] and g.get('impact_level') not in ['CRITICAL', 'HIGH'])
    
    print(f"   P0 (CRITICAL×CRIT/HIGH): {p0:6,d} findings")
    print(f"   P1 (HIGH×CRIT/HIGH):     {p1:6,d} findings")
    print(f"   P2 (Others):              {total_findings-p0-p1:6,d} findings")
    
    # Top subsystems
    print(f"\n📁 Top Subsystems by Finding Count:")
    for subsys, count in subsystem_dist.most_common(10):
        pct = 100 * count / total_findings
        print(f"   {subsys or 'UNKNOWN':20s}: {count:6,d} ({pct:5.1f}%)")
    
    # Save aggregated data
    output_file = ai_working / 'full_codebase_aggregated.json'
    output_data = {
        'total_findings': total_findings,
        'directories': dir_stats,
        'severity_distribution': dict(severity_dist),
        'impact_distribution': dict(impact_dist),
        'ai_vibe_count': ai_vibe_count,
        'subsystem_distribution': dict(subsystem_dist),
        'priority_analysis': {
            'p0': p0,
            'p1': p1,
            'other': total_findings - p0 - p1
        }
    }
    
    with open(output_file, 'w') as f:
        json.dump(output_data, f, indent=2)
    
    print(f"\n✅ Aggregated data saved: {output_file.name}")
    
    return all_gaps, output_data

if __name__ == '__main__':
    gaps, summary = aggregate_scans()
