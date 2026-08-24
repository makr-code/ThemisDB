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
    
    print("=" * 90)
    print("GS3 FULL SCAN ANALYSIS - 2026-06-21")
    print("=" * 90)
    print()
    
    # Overall statistics
    if isinstance(data, list):
        gaps = data
    elif isinstance(data, dict) and 'gaps' in data:
        gaps = data['gaps']
    else:
        gaps = []
    
    print(f"📊 TOTAL GAPS FOUND: {len(gaps):,}")
    print()
    
    # Analyze by severity
    severity_counts = defaultdict(int)
    impact_counts = defaultdict(int)
    gap_type_counts = defaultdict(int)
    file_counts = defaultdict(int)
    category_counts = defaultdict(int)
    
    for gap in gaps:
        if isinstance(gap, dict):
            severity_counts[gap.get('severity', 'UNKNOWN')] += 1
            impact_counts[gap.get('impact_level', 'UNKNOWN')] += 1
            gap_type_counts[gap.get('gap_type', 'UNKNOWN')] += 1
            file_counts[gap.get('file_path', 'UNKNOWN')] += 1
            category_counts[gap.get('category', 'UNKNOWN')] += 1
    
    # Print severity breakdown
    print("🔴 SEVERITY DISTRIBUTION:")
    for severity in ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW']:
        count = severity_counts[severity]
        pct = (count / len(gaps) * 100) if gaps else 0
        bar_len = int(pct / 2)
        bar = "█" * bar_len
        print(f"  {severity:12s}: {count:7,} ({pct:5.2f}%) {bar}")
    print()
    
    # Print impact breakdown
    print("⚠️  IMPACT CLASSIFICATION:")
    impact_list = sorted([(k or "UNKNOWN", v) for k, v in impact_counts.items()], key=lambda x: x[0])
    for impact_str, count in impact_list:
        pct = (count / len(gaps) * 100) if gaps else 0
        print(f"  {impact_str:20s}: {count:7,} ({pct:5.2f}%)")
    print()
    
    # Print category breakdown
    print("📋 CATEGORY DISTRIBUTION:")
    sorted_categories = sorted(category_counts.items(), key=lambda x: x[1], reverse=True)
    for category, count in sorted_categories:
        pct = (count / len(gaps) * 100) if gaps else 0
        print(f"  {category:30s}: {count:7,} ({pct:5.2f}%)")
    print()
    
    # Top 15 gap types
    print("🔍 TOP 15 GAP TYPES:")
    sorted_types = sorted(gap_type_counts.items(), key=lambda x: x[1], reverse=True)[:15]
    for idx, (gap_type, count) in enumerate(sorted_types, 1):
        pct = (count / len(gaps) * 100) if gaps else 0
        print(f"  {idx:2d}. {gap_type:40s}: {count:7,} ({pct:5.2f}%)")
    print()
    
    # Files with most gaps (top 15)
    print("📁 TOP 15 FILES WITH MOST GAPS:")
    sorted_files = sorted(file_counts.items(), key=lambda x: x[1], reverse=True)[:15]
    for idx, (file_path, count) in enumerate(sorted_files, 1):
        short_path = file_path[-65:] if len(file_path) > 65 else file_path
        print(f"  {idx:2d}. {count:6,}  {short_path}")
    print()
    
    # CRITICAL severity by category
    critical_gaps = [g for g in gaps if g.get('severity') == 'CRITICAL']
    if critical_gaps:
        print(f"🚨 CRITICAL ISSUES BREAKDOWN ({len(critical_gaps):,} total):")
        critical_types = defaultdict(int)
        for gap in critical_gaps:
            critical_types[gap.get('gap_type', 'UNKNOWN')] += 1
        
        sorted_critical = sorted(critical_types.items(), key=lambda x: x[1], reverse=True)[:10]
        for gap_type, count in sorted_critical:
            pct = (count / len(critical_gaps) * 100)
            print(f"  {gap_type:40s}: {count:6,} ({pct:5.2f}%)")
        print()
    
    # Summary statistics
    print("=" * 90)
    print("SUMMARY STATISTICS")
    print("=" * 90)
    print(f"Total Gaps:        {len(gaps):,}")
    print(f"Unique Files:      {len(file_counts):,}")
    print(f"Gap Types:         {len(gap_type_counts):,}")
    print(f"Avg Gaps/File:     {len(gaps) / len(file_counts):.1f}" if file_counts else "Avg Gaps/File:     N/A")
    print()
    
    # Risk assessment
    critical_pct = (severity_counts['CRITICAL'] / len(gaps) * 100) if gaps else 0
    high_pct = (severity_counts['HIGH'] / len(gaps) * 100) if gaps else 0
    urgent = severity_counts['CRITICAL'] + severity_counts['HIGH']
    
    print("⚠️  RISK ASSESSMENT:")
    if urgent > 1000:
        print(f"  Status: 🔴 CRITICAL - {urgent:,} urgent issues require immediate attention")
    elif urgent > 500:
        print(f"  Status: 🟠 HIGH - {urgent:,} urgent issues should be addressed soon")
    elif urgent > 100:
        print(f"  Status: 🟡 MEDIUM - {urgent:,} issues should be tracked and resolved")
    else:
        print(f"  Status: 🟢 LOW - {urgent:,} urgent issues (manageable)")
    print()
    
    print("=" * 90)

if __name__ == '__main__':
    json_file = Path('ai_working/gs3_quick_scan.json')
    
    if not json_file.exists():
        print(f"❌ File not found: {json_file}")
        exit(1)
    
    analyze_scan_results(str(json_file))
