#!/usr/bin/env python3
"""
Representative Gap Sampling Tool
Stratified sampling from 18,795 remaining gaps for manual true positive validation.

Strategy:
- Sample 50 gaps across severities, categories, and modules
- Show full function context (not just ±5 lines)
- Provide structured TP/FP assessment
- Generate statistics for scanner tuning
"""

import json
from pathlib import Path
import random
from collections import defaultdict
from typing import Dict, List, Tuple

def load_gaps():
    """Load gaps from gap_scan_v3_aggregate.json"""
    summary_file = Path(__file__).parent.parent / "ai_working" / "gap_scan_v3_summary.json"
    
    if not summary_file.exists():
        print(f"Error: {summary_file} not found")
        return None
    
    with open(summary_file) as f:
        summary = json.load(f)
    
    return summary

def sample_representative_gaps(repo_root: Path, target_sample_size: int = 50) -> List[Dict]:
    """
    Sample gaps stratified by:
    - Severity (CRITICAL, HIGH, MEDIUM)
    - Category (top categories)
    - Module (top modules)
    """
    summary_file = repo_root / "ai_working" / "gap_scan_v3_summary.json"
    
    if not summary_file.exists():
        print(f"No summary found at {summary_file}")
        return []
    
    with open(summary_file) as f:
        summary = json.load(f)
    
    # Load individual module reports to get actual gaps
    all_gaps = []
    module_files = list((repo_root / "ai_working").glob("gap_scan_v3_*.json"))
    
    for module_file in module_files:
        # Skip aggregate/summary files
        if any(x in module_file.name for x in ["summary", "aggregate", "confidence", "preflight"]):
            continue
        
        try:
            with open(module_file) as f:
                module_data = json.load(f)
            
            # Extract gaps from by_file structure
            if isinstance(module_data, dict) and 'by_file' in module_data:
                for file_path, gaps in module_data['by_file'].items():
                    if isinstance(gaps, list):
                        for gap in gaps:
                            gap['_module'] = module_file.stem.replace('gap_scan_v3_', '')
                            gap['_file'] = file_path
                            all_gaps.append(gap)
        except:
            pass
    
    if not all_gaps:
        print("No gaps found in module files")
        return []
    
    # Stratified sampling
    sample = []
    
    # Group by severity
    by_severity = defaultdict(list)
    for gap in all_gaps:
        severity = str(gap.get('severity', 'MEDIUM')).upper()
        by_severity[severity].append(gap)
    
    # Sample proportionally from each severity
    severities = ['CRITICAL', 'HIGH', 'MEDIUM']
    severity_counts = {s: len(by_severity[s]) for s in severities}
    total = sum(severity_counts.values())
    
    print(f"\n[GAP DISTRIBUTION]")
    for sev in severities:
        pct = (severity_counts[sev] / total * 100) if total > 0 else 0
        print(f"  {sev}: {severity_counts[sev]} ({pct:.1f}%)")
    
    # Allocate samples per severity (proportional)
    for severity in severities:
        if severity_counts[severity] == 0:
            continue
        
        proportion = severity_counts[severity] / total
        samples_for_severity = max(1, int(target_sample_size * proportion))
        
        # Further stratify by category within severity
        category_groups = defaultdict(list)
        for gap in by_severity[severity]:
            cat = gap.get('category', 'unknown')
            category_groups[cat].append(gap)
        
        # Sample from each category
        for category, gaps_in_cat in category_groups.items():
            cat_sample_size = max(1, int(samples_for_severity * len(gaps_in_cat) / len(by_severity[severity])))
            sample.extend(random.sample(gaps_in_cat, min(cat_sample_size, len(gaps_in_cat))))
    
    # Ensure we have enough samples
    if len(sample) < target_sample_size:
        remaining = random.sample(
            [g for g in all_gaps if g not in sample],
            min(target_sample_size - len(sample), len(all_gaps) - len(sample))
        )
        sample.extend(remaining)
    
    return sample[:target_sample_size]

def get_function_context(repo_root: Path, file_path: str, line_num: int, context_lines: int = 20) -> str:
    """Get full function context around line"""
    try:
        file_full_path = repo_root / file_path
        if not file_full_path.exists():
            return "[FILE NOT FOUND]"
        
        with open(file_full_path, encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
        
        if line_num <= 0 or line_num > len(lines):
            return f"[LINE {line_num} OUT OF RANGE (file has {len(lines)} lines)]"
        
        # Try to find function start (scan backwards for common function patterns)
        func_start = max(0, line_num - 50)
        func_end = min(len(lines), line_num + context_lines)
        
        # Look backwards for function signature or class/namespace
        for i in range(line_num - 1, max(0, line_num - 50), -1):
            if '{' in lines[i] or 'void ' in lines[i] or 'int ' in lines[i] or 'auto ' in lines[i]:
                func_start = i
                break
        
        context = []
        for i in range(func_start, func_end):
            marker = ">>> " if (i + 1) == line_num else "    "
            context.append(f"{i+1:5d} {marker} {lines[i].rstrip()}")
        
        return "\n".join(context)
    except Exception as e:
        return f"[ERROR READING FILE: {e}]"

def main():
    repo_root = Path(__file__).parent.parent
    print("\n" + "=" * 80)
    print("[REPRESENTATIVE GAP SAMPLING FOR VALIDATION]")
    print("=" * 80)
    print(f"\nRepository: {repo_root}")
    
    # Sample gaps
    print("\n[SAMPLING STRATEGY]")
    print("  • Stratified by severity (CRITICAL, HIGH, MEDIUM)")
    print("  • Stratified by category (security, memory, reliability, etc.)")
    print("  • Stratified by module (top modules)")
    print("  • Target sample size: 50 gaps")
    
    sample = sample_representative_gaps(repo_root, target_sample_size=50)
    
    if not sample:
        print("\n[ERROR] No gaps could be sampled")
        return
    
    print(f"\n[SAMPLE COMPOSITION]")
    print(f"  Total sampled: {len(sample)}")
    
    # Group by severity
    by_severity = defaultdict(int)
    by_category = defaultdict(int)
    by_module = defaultdict(int)
    
    for gap in sample:
        by_severity[str(gap.get('severity', 'MEDIUM')).upper()] += 1
        by_category[gap.get('category', 'unknown')] += 1
        by_module[gap.get('_module', 'unknown')] += 1
    
    print("\n  By Severity:")
    for sev in sorted(by_severity.keys()):
        print(f"    {sev}: {by_severity[sev]}")
    
    print("\n  Top Categories:")
    for cat, count in sorted(by_category.items(), key=lambda x: x[1], reverse=True)[:5]:
        print(f"    {cat}: {count}")
    
    print("\n  Top Modules:")
    for mod, count in sorted(by_module.items(), key=lambda x: x[1], reverse=True)[:5]:
        print(f"    {mod}: {count}")
    
    # Display each gap with context
    print("\n" + "=" * 80)
    print("[VALIDATION SAMPLES - ASSESS EACH AS TP (True Positive) OR FP (False Positive)]")
    print("=" * 80)
    
    tp_count = 0
    fp_count = 0
    results = []
    
    for idx, gap in enumerate(sample, 1):
        print(f"\n{idx:2d}. {gap.get('_module', 'unknown').upper()}")
        print(f"    Category:  {gap.get('category', 'unknown')}")
        print(f"    Severity:  {gap.get('severity', 'MEDIUM')}")
        print(f"    File:      {gap.get('_file', 'unknown')}")
        print(f"    Line:      {gap.get('line', '?')}")
        print(f"    Message:   {gap.get('description', 'No description')}")
        
        # Show context
        context = get_function_context(
            repo_root,
            gap.get('_file', ''),
            gap.get('line', 0)
        )
        
        print("\n    [FUNCTION CONTEXT]")
        for line in context.split('\n'):
            print(f"      {line}")
        
        print("\n    [YOUR ASSESSMENT]")
        print("      TP = Real issue that should be fixed")
        print("      FP = False alarm, code is actually safe")
        
        # This is where manual assessment happens
        # For now, ask user (in real workflow, collect responses)
        response = input("      Assessment (TP/FP/SKIP): ").strip().upper()
        
        if response == 'TP':
            tp_count += 1
            assessment = 'TP'
        elif response == 'FP':
            fp_count += 1
            assessment = 'FP'
        else:
            assessment = 'SKIP'
        
        results.append({
            'index': idx,
            'module': gap.get('_module', 'unknown'),
            'category': gap.get('category', 'unknown'),
            'severity': gap.get('severity', 'MEDIUM'),
            'file': gap.get('_file', 'unknown'),
            'line': gap.get('line', 0),
            'assessment': assessment
        })
    
    # Summary
    print("\n" + "=" * 80)
    print("[VALIDATION SUMMARY]")
    print("=" * 80)
    
    total_assessed = tp_count + fp_count
    if total_assessed > 0:
        tp_pct = (tp_count / total_assessed * 100)
        fp_pct = (fp_count / total_assessed * 100)
        
        print(f"\nTrue Positives (TP): {tp_count}/{total_assessed} ({tp_pct:.1f}%)")
        print(f"False Positives (FP): {fp_count}/{total_assessed} ({fp_pct:.1f}%)")
        print(f"Not assessed:        {len(sample) - total_assessed}")
        
        print(f"\n[EXTRAPOLATION TO FULL SET]")
        print(f"  If FP rate is {fp_pct:.1f}%:")
        print(f"    Expected FPs in 18,795 gaps: {int(18795 * fp_pct / 100):,}")
        print(f"    Expected TPs in 18,795 gaps: {int(18795 * tp_pct / 100):,}")
    
    # Save results
    results_file = repo_root / "ai_working" / "representative_sample_results.json"
    with open(results_file, 'w') as f:
        json.dump({
            'total_sampled': len(sample),
            'total_assessed': total_assessed,
            'tp_count': tp_count,
            'fp_count': fp_count,
            'tp_rate': (tp_count / total_assessed * 100) if total_assessed > 0 else 0,
            'fp_rate': (fp_count / total_assessed * 100) if total_assessed > 0 else 0,
            'results': results
        }, f, indent=2)
    
    print(f"\n[SAVED] Results to: {results_file}")

if __name__ == '__main__':
    main()
