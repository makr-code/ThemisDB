#!/usr/bin/env python3
"""
Sample 10 random gaps and show FULL FUNCTION CONTEXT for manual review.
Purpose: Determine if gaps are TP (true issue) or FP (false positive) when given full function context.
"""

import json
import random
import os
import sys
from pathlib import Path

def load_all_gaps():
    """Load all gaps from individual module JSON files"""
    ai_working = Path(__file__).parent.parent / "ai_working"
    all_gaps = []
    
    gap_files = sorted(ai_working.glob("gap_scan_v3_*.json"))
    gap_files = [f for f in gap_files if not any(x in f.name for x in ['aggregate', 'summary', 'confidence', 'actionable', 'review'])]
    
    print(f"[INFO] Found {len(gap_files)} gap files\n")
    
    for gap_file in gap_files:
        try:
            with open(gap_file, 'r', encoding='utf-8') as f:
                data = json.load(f)
            
            # Structure: {module: {by_file: {file: [gaps]}, ...}}
            for module_name, module_data in data.items():
                if isinstance(module_data, dict) and 'by_file' in module_data:
                    by_file = module_data['by_file']
                    for file_path, file_gaps in by_file.items():
                        if isinstance(file_gaps, list):
                            for gap in file_gaps:
                                gap['module'] = module_name  # Add module info
                                all_gaps.append(gap)
        except Exception as e:
            print(f"[WARN] {gap_file.name}: {e}")
    
    print(f"[INFO] Loaded {len(all_gaps)} total gaps from {len(gap_files)} files\n")
    return all_gaps


def get_function_context(file_path, line_num, context_lines=30):
    """Read FULL FUNCTION from source file, not just ±5 lines."""
    try:
        if not os.path.exists(file_path):
            return f"[FILE NOT FOUND] {file_path}", None
        
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
        
        if line_num < 1 or line_num > len(lines):
            return f"[LINE OUT OF RANGE] {line_num} in {len(lines)} lines", None
        
        # Find function start by scanning backwards for function signature
        func_start = line_num - 1
        for i in range(line_num - 2, -1, -1):
            line = lines[i].strip()
            # Heuristic: function signature patterns
            if line.endswith('{'):
                func_start = i
                break
            elif ')' in line and ('{' in line or lines[i+1].strip().startswith('{')):
                func_start = i
                break
            if i < line_num - 100:  # Don't search too far back
                break
        
        # Find function end by scanning forwards for matching closing brace
        brace_count = 0
        func_end = min(line_num + context_lines, len(lines) - 1)
        for i in range(func_start, len(lines)):
            brace_count += lines[i].count('{') - lines[i].count('}')
            if brace_count <= 0 and i > func_start:
                func_end = i + 1
                break
        
        context = "".join(lines[func_start:func_end])
        return context, (func_start + 1, func_end)
    except Exception as e:
        return f"[ERROR] {e}", None

def main():
    print("[SAMPLE GAPS FULL CONTEXT ANALYZER]")
    print("=" * 80)
    
    gaps = load_all_gaps()
    if not gaps:
        print("[ERROR] No gaps found in module JSON files")
        return
    
    print(f"[INFO] Total gaps in aggregate: {len(gaps)}\n")
    
    # Sample 10 random gaps
    sample = random.sample(gaps, min(10, len(gaps)))
    
    results = []
    
    for idx, gap in enumerate(sample, 1):
        print(f"\n{'='*80}")
        print(f"[SAMPLE {idx}/10]")
        print(f"{'='*80}")
        
        # Extract gap info (flexible field names)
        module = gap.get('module', 'UNKNOWN')
        category = gap.get('category', gap.get('type', 'UNKNOWN'))
        severity = gap.get('severity', 'UNKNOWN')
        file_path = gap.get('file', 'UNKNOWN')
        line_num = gap.get('line', 0)
        message = gap.get('description', gap.get('message', gap.get('pattern', 'NO MESSAGE')))
        code_snippet = gap.get('snippet', gap.get('code_snippet', gap.get('context', '')))
        
        print(f"Module:    {module}")
        print(f"Category:  {category}")
        print(f"Severity:  {severity}")
        print(f"File:      {file_path}")
        print(f"Line:      {line_num}")
        print(f"Message:   {message}\n")
        
        # Get full function context
        repo_root = Path(__file__).parent.parent
        abs_file_path = repo_root / file_path.lstrip('./')
        
        context, line_range = get_function_context(str(abs_file_path), line_num)
        
        print("[FULL FUNCTION CONTEXT]")
        print("-" * 80)
        
        # Add line numbers
        context_lines = context.split('\n')
        if line_range:
            start_line = line_range[0]
        else:
            start_line = max(1, line_num - 15)
        
        for i, ctx_line in enumerate(context_lines[:100]):  # Limit output
            current_line = start_line + i
            marker = " >>> " if current_line == line_num else "     "
            print(f"{marker}{current_line:4d} | {ctx_line}")
        
        if len(context_lines) > 100:
            print(f"     ... ({len(context_lines) - 100} more lines)")
        
        print("\n[SHORT SNIPPET FROM SCAN]")
        print("-" * 80)
        print(code_snippet)
        
        print("\n[YOUR ASSESSMENT]")
        print("-" * 80)
        print("Is this a TRUE POSITIVE (TP) or FALSE POSITIVE (FP)?")
        print("TP = Real issue that should be fixed")
        print("FP = False alarm, code is actually safe/correct")
        
        results.append({
            'sample': idx,
            'category': category,
            'severity': severity,
            'file': file_path,
            'line': line_num,
            'message': message,
        })
    
    # Summary
    print(f"\n{'='*80}")
    print("[SUMMARY FOR MANUAL REVIEW]")
    print(f"{'='*80}")
    for r in results:
        print(f"{r['sample']:2d}. {r['category']:20s} {r['severity']:8s} {r['file']}:{r['line']}")
    
    print("\nAfter reviewing all 10 samples:")
    print("- If MOST are TP (echte Fehler) → Gap-Reduktion ist nicht das Problem")
    print("- If MOST are FP (falsche Alarme) → Erhöhen Sie context_lines von ±5 auf ganze Function")

if __name__ == '__main__':
    main()
