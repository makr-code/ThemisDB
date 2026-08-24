#!/usr/bin/env python3
"""Analyze GS3 scan results from JSON output."""

import json
from collections import defaultdict
from pathlib import Path

def analyze_scan_results(json_file):
    """Load and analyze GS3 scan results."""
    
    # Load the scan results
    with open(json_file, 'r') as f:
        data = json.load(f)
    
    gaps = data.get('gaps', [])
    metadata = data.get('metadata', {})
    
    print("=" * 100)
    print("GS3 FULL SCAN ANALYSIS - 2026-06-21")
    print("=" * 100)
    print()
    
    if metadata:
        print("📋 SCAN METADATA:")
        for key, value in metadata.items():
            if isinstance(value, list):
                print(f"  {key}: {', '.join(str(v) for v in value)}")
            else:
                print(f"  {key}: {value}")
        print()
    
    print(f"📊 TOTAL GAPS FOUND: {len(gaps):,}")
    print()
    
    # Analyze by severity
    severity_counts = defaultdict(int)
    impact_counts = defaultdict(int)
    gap_type_counts = defaultdict(int)
    file_counts = defaultdict(int)
    scanner_counts = defaultdict(int)
    step_counts = defaultdict(int)
    subsystem_counts = defaultdict(int)
    
    for gap in gaps:
        severity_counts[gap.get('severity', 'UNKNOWN')] += 1
        impact_level = gap.get('impact_level')
        impact_counts[impact_level if impact_level else 'UNCLASSIFIED'] += 1
        gap_type_counts[gap.get('type', 'UNKNOWN')] += 1
        file_counts[gap.get('file', 'UNKNOWN')] += 1
        scanner_counts[gap.get('scanner', 'UNKNOWN')] += 1
        step_counts[gap.get('step', 'UNKNOWN')] += 1
        subsys = gap.get('subsystem')
        subsystem_counts[subsys if subsys else 'UNCLASSIFIED'] += 1
    
    # Print severity breakdown
    print("🔴 SEVERITY DISTRIBUTION:")
    for severity in ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW', 'UNKNOWN']:
        count = severity_counts[severity]
        if count > 0:
            pct = (count / len(gaps) * 100) if gaps else 0
            bar_len = int(pct / 1.5)
            bar = "█" * bar_len
            print(f"  {severity:12s}: {count:7,} ({pct:5.2f}%) {bar}")
    print()
    
    # Print impact breakdown
    print("⚠️  IMPACT CLASSIFICATION:")
    impact_list = sorted(impact_counts.items(), key=lambda x: x[1], reverse=True)
    for impact_str, count in impact_list:
        pct = (count / len(gaps) * 100) if gaps else 0
        print(f"  {impact_str:25s}: {count:7,} ({pct:5.2f}%)")
    print()
    
    # Top 15 gap types
    print("🔍 TOP 15 GAP TYPES:")
    sorted_types = sorted(gap_type_counts.items(), key=lambda x: x[1], reverse=True)[:15]
    for idx, (gap_type, count) in enumerate(sorted_types, 1):
        pct = (count / len(gaps) * 100) if gaps else 0
        print(f"  {idx:2d}. {gap_type:40s}: {count:7,} ({pct:5.2f}%)")
    print()
    
    # Top 10 scanners
    print("🔧 TOP 10 SCANNER MODULES:")
    sorted_scanners = sorted(scanner_counts.items(), key=lambda x: x[1], reverse=True)[:10]
    for idx, (scanner, count) in enumerate(sorted_scanners, 1):
        pct = (count / len(gaps) * 100) if gaps else 0
        short_name = scanner[-50:] if len(scanner) > 50 else scanner
        print(f"  {idx:2d}. {short_name:50s}: {count:7,} ({pct:5.2f}%)")
    print()
    
    # Top 15 files with most gaps
    print("📁 TOP 15 FILES WITH MOST GAPS:")
    sorted_files = sorted(file_counts.items(), key=lambda x: x[1], reverse=True)[:15]
    for idx, (file_path, count) in enumerate(sorted_files, 1):
        short_path = file_path[-70:] if len(file_path) > 70 else file_path
        print(f"  {idx:2d}. {count:7,}  {short_path}")
    print()
    
    # Subsystems affected
    print("🏢 SUBSYSTEM IMPACT:")
    sorted_subsys = sorted(subsystem_counts.items(), key=lambda x: x[1], reverse=True)[:15]
    for subsys, count in sorted_subsys:
        pct = (count / len(gaps) * 100) if gaps else 0
        print(f"  {subsys:35s}: {count:7,} ({pct:5.2f}%)")
    print()
    
    # Execution phases
    print("⏸️  EXECUTION PHASES:")
    sorted_steps = sorted(step_counts.items(), key=lambda x: x[1], reverse=True)
    for step, count in sorted_steps:
        pct = (count / len(gaps) * 100) if gaps else 0
        print(f"  {step:35s}: {count:7,} ({pct:5.2f}%)")
    print()
    
    # CRITICAL severity breakdown
    critical_gaps = [g for g in gaps if g.get('severity') == 'CRITICAL']
    if critical_gaps:
        print(f"🚨 CRITICAL ISSUES ({len(critical_gaps):,} total):")
        critical_types = defaultdict(int)
        critical_scanners = defaultdict(int)
        for gap in critical_gaps:
            critical_types[gap.get('type', 'UNKNOWN')] += 1
            critical_scanners[gap.get('scanner', 'UNKNOWN')] += 1
        
        print("  Top gap types (critical):")
        sorted_critical = sorted(critical_types.items(), key=lambda x: x[1], reverse=True)[:5]
        for gap_type, count in sorted_critical:
            pct = (count / len(critical_gaps) * 100)
            print(f"    • {gap_type:40s}: {count:6,} ({pct:5.1f}%)")
        print()
    
    # HIGH severity breakdown
    high_gaps = [g for g in gaps if g.get('severity') == 'HIGH']
    if high_gaps:
        print(f"⚠️  HIGH-SEVERITY ISSUES ({len(high_gaps):,} total):")
        high_types = defaultdict(int)
        for gap in high_gaps:
            high_types[gap.get('type', 'UNKNOWN')] += 1
        
        print("  Top gap types (high severity):")
        sorted_high = sorted(high_types.items(), key=lambda x: x[1], reverse=True)[:5]
        for gap_type, count in sorted_high:
            pct = (count / len(high_gaps) * 100)
            print(f"    • {gap_type:40s}: {count:6,} ({pct:5.1f}%)")
        print()
    
    # Summary statistics
    print("=" * 100)
    print("SUMMARY STATISTICS")
    print("=" * 100)
    print(f"Total Gaps:           {len(gaps):,}")
    print(f"Unique Files:         {len(file_counts):,}")
    print(f"Gap Types:            {len(gap_type_counts):,}")
    print(f"Scanners Used:        {len(scanner_counts):,}")
    print(f"Subsystems Affected:  {len(subsystem_counts):,}")
    if file_counts:
        print(f"Avg Gaps/File:        {len(gaps) / len(file_counts):.1f}")
    print()
    
    # Risk assessment
    critical_count = severity_counts['CRITICAL']
    high_count = severity_counts['HIGH']
    urgent = critical_count + high_count
    
    print("🎯 RISK ASSESSMENT:")
    if urgent > 5000:
        status = "🔴 CRITICAL"
    elif urgent > 2000:
        status = "🟠 HIGH"
    elif urgent > 500:
        status = "🟡 MEDIUM"
    else:
        status = "🟢 LOW"
    
    print(f"  Status: {status}")
    print(f"  Urgent Issues: {urgent:,} (CRITICAL: {critical_count:,} + HIGH: {high_count:,})")
    print(f"  Medium/Low Issues: {severity_counts['MEDIUM'] + severity_counts['LOW']:,}")
    print()
    
    # Top recommendations
    print("💡 TOP RECOMMENDATIONS:")
    top_gap = sorted_types[0][0] if sorted_types else "UNKNOWN"
    top_count = sorted_types[0][1] if sorted_types else 0
    print(f"  1. Address '{top_gap}' ({top_count:,} occurrences)")
    
    if len(sorted_types) > 1:
        second_gap = sorted_types[1][0]
        second_count = sorted_types[1][1]
        print(f"  2. Address '{second_gap}' ({second_count:,} occurrences)")
    
    if critical_count > 0:
        print(f"  3. Immediately fix all {critical_count:,} CRITICAL severity issues")
    
    if high_count > 0:
        print(f"  4. Schedule fixing {high_count:,} HIGH severity issues")
    
    print()
    print("=" * 100)

if __name__ == '__main__':
    json_file = Path('ai_working/gs3_quick_scan.json')
    
    if not json_file.exists():
        print(f"❌ File not found: {json_file}")
        exit(1)
    
    analyze_scan_results(str(json_file))
