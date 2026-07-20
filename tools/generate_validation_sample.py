#!/usr/bin/env python3
"""
Representative Gap Validation Report
Load stratified gaps with deterministic sampling and generate validation artifacts.
"""

import argparse
import json
from pathlib import Path
import random
from collections import defaultdict
from datetime import datetime
from typing import Dict, List

def load_all_gaps(aggregate_file: Path) -> List[Dict]:
    """Load all gaps from gap_scan_v3_aggregate.json"""
    all_gaps = []

    if not aggregate_file.exists():
        print(f"[ERROR] {aggregate_file} not found")
        return []
    
    try:
        with open(aggregate_file) as f:
            aggregate = json.load(f)
        
        # Aggregate structure: {module: {total, by_file: {file: [gaps]}}}
        for module_name, module_data in aggregate.items():
            if isinstance(module_data, dict) and 'by_file' in module_data:
                for file_path, gaps in module_data['by_file'].items():
                    if isinstance(gaps, list):
                        for gap in gaps:
                            gap['_module'] = module_name
                            gap['_file'] = file_path
                            # Ensure required fields
                            if 'severity' not in gap:
                                gap['severity'] = 'MEDIUM'
                            if 'category' not in gap and 'type' in gap:
                                gap['category'] = gap['type']
                            all_gaps.append(gap)
    except Exception as e:
        print(f"[ERROR] Could not load aggregate: {e}")
    
    return all_gaps

def stratified_sample(gaps: List[Dict], sample_size: int = 50, seed: int = 42) -> List[Dict]:
    """Stratified sampling by severity and category"""
    if not gaps:
        return []
    rng = random.Random(seed)
    
    # Group by (severity, category)
    strata = defaultdict(list)
    for gap in gaps:
        severity = str(gap.get('severity', 'MEDIUM')).upper()
        category = gap.get('category', 'unknown')
        key = (severity, category)
        strata[key].append(gap)
    
    # Sample from each stratum proportionally
    sample = []
    total = len(gaps)
    
    for (severity, category), group in strata.items():
        proportion = len(group) / total
        stratum_size = max(1, int(sample_size * proportion))
        sample.extend(rng.sample(group, min(stratum_size, len(group))))
    
    # Fill remaining quota with random gaps
    if len(sample) < sample_size:
        remaining_gaps = [g for g in gaps if g not in sample]
        additional = rng.sample(remaining_gaps, min(sample_size - len(sample), len(remaining_gaps)))
        sample.extend(additional)
    
    return sample[:sample_size]

def get_function_context(repo_root: Path, file_path: str, line_num: int, context_size: int = 15) -> str:
    """Extract function context around line"""
    try:
        file_full_path = repo_root / file_path
        if not file_full_path.exists():
            return "[FILE NOT FOUND]"
        
        with open(file_full_path, encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
        
        if line_num <= 0 or line_num > len(lines):
            return f"[LINE {line_num} OUT OF RANGE]"
        
        # Find function start (scan backwards for '{')
        func_start = line_num - 1
        for i in range(line_num - 1, max(0, line_num - 80), -1):
            if '{' in lines[i] and not lines[i].strip().startswith('//'):
                func_start = i
                break
        
        # Get context
        start = max(0, func_start)
        end = min(len(lines), line_num + context_size)
        
        context_lines = []
        for i in range(start, end):
            marker = ">>> " if (i + 1) == line_num else "    "
            line_text = lines[i].rstrip()
            context_lines.append(f"{i+1:5d} | {marker}{line_text}")
        
        return "\n".join(context_lines)
    except Exception as e:
        return f"[ERROR: {e}]"

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate deterministic stratified validation sample")
    parser.add_argument("--repo-root", default=str(Path(__file__).parent.parent), help="Repository root")
    parser.add_argument("--aggregate-file", default="ai_working/gap_scan_v3_aggregate.json", help="Aggregate JSON path")
    parser.add_argument("--sample-size", type=int, default=50, help="Number of sampled gaps")
    parser.add_argument("--seed", type=int, default=42, help="Random seed for reproducibility")
    parser.add_argument("--template-out", default="ai_working/SAMPLE_VALIDATION_TEMPLATE.md", help="Markdown template output")
    parser.add_argument("--metadata-out", default="ai_working/sample_validation_metadata.json", help="Metadata output")
    return parser.parse_args()


def main():
    args = parse_args()
    repo_root = Path(args.repo_root).resolve()
    aggregate_file = (repo_root / args.aggregate_file).resolve()
    template_out = (repo_root / args.template_out).resolve()
    metadata_out = (repo_root / args.metadata_out).resolve()

    print("\n" + "=" * 100)
    print("REPRESENTATIVE GAP VALIDATION SAMPLE (50 Gaps)")
    print("=" * 100)
    
    print("\n[LOADING GAPS...]")
    all_gaps = load_all_gaps(aggregate_file)
    print(f"Total gaps loaded: {len(all_gaps):,}")
    
    if not all_gaps:
        print("ERROR: No gaps could be loaded")
        return
    
    print("\n[SAMPLING STRATEGY]")
    print("  • Stratified by severity (CRITICAL, HIGH, MEDIUM)")
    print("  • Stratified by category (top categories)")
    print(f"  • Random sample size: {args.sample_size} gaps")
    print(f"  • Deterministic seed: {args.seed}")
    
    sample = stratified_sample(all_gaps, sample_size=args.sample_size, seed=args.seed)
    
    # Statistics
    by_severity = defaultdict(int)
    by_category = defaultdict(int)
    by_module = defaultdict(int)
    
    for gap in sample:
        by_severity[str(gap.get('severity', 'MEDIUM')).upper()] += 1
        by_category[gap.get('category', 'unknown')] += 1
        by_module[gap.get('_module', 'unknown')] += 1
    
    print(f"\n[SAMPLE COMPOSITION: {len(sample)} gaps]")
    print("\n  By Severity:")
    for sev in ['CRITICAL', 'HIGH', 'MEDIUM']:
        if sev in by_severity:
            pct = (by_severity[sev] / len(sample) * 100)
            print(f"    {sev:8s}: {by_severity[sev]:3d} ({pct:5.1f}%)")
    
    print("\n  Top Categories:")
    for cat, count in sorted(by_category.items(), key=lambda x: x[1], reverse=True)[:8]:
        pct = (count / len(sample) * 100)
        print(f"    {cat:20s}: {count:3d} ({pct:5.1f}%)")
    
    print("\n  Top Modules:")
    for mod, count in sorted(by_module.items(), key=lambda x: x[1], reverse=True)[:8]:
        pct = (count / len(sample) * 100)
        print(f"    {mod:20s}: {count:3d} ({pct:5.1f}%)")
    
    # Generate validation report
    print("\n" + "=" * 100)
    print("GAP VALIDATION DETAILS")
    print("=" * 100)
    print("\nFor each gap below:")
    print("  TP = True Positive (real issue)")
    print("  FP = False Positive (safe code, no issue)")
    print("  ? = Uncertain / Needs investigation")
    
    # Create detailed report
    report_lines = ["\n# REPRESENTATIVE GAP VALIDATION SAMPLE\n"]
    report_lines.append(f"Generated: 2026-06-02\n")
    report_lines.append(f"Total Sample Size: {len(sample)}\n")
    report_lines.append("\nFor each gap, assess as:\n")
    report_lines.append("- **TP** = True Positive (real issue to fix)\n")
    report_lines.append("- **FP** = False Positive (code is correct)\n")
    report_lines.append("- **?** = Uncertain\n")
    
    for idx, gap in enumerate(sample, 1):
        severity = str(gap.get('severity', 'MEDIUM')).upper()
        category = gap.get('category', 'unknown')
        module = gap.get('_module', 'unknown')
        file_path = gap.get('_file', 'unknown')
        line_num = gap.get('line', 0)
        message = gap.get('description', 'No description')
        
        print(f"\n[{idx:2d}/{len(sample)}] {module:20s} | {severity:8s} | {category:20s}")
        print(f"        File: {file_path}:{line_num}")
        print(f"        Issue: {message}")
        
        context = get_function_context(repo_root, file_path, line_num, context_size=12)
        print("\n        [CONTEXT]")
        for line in context.split('\n'):
            print(f"        {line}")
        
        print("\n        [ ] TP  [ ] FP  [ ] ?")
        print()
        
        # Add to report
        report_lines.append(f"\n## [{idx:2d}/50] {module} - {severity}\n")
        report_lines.append(f"**File:** `{file_path}:{line_num}`\n")
        report_lines.append(f"**Category:** {category}\n")
        report_lines.append(f"**Message:** {message}\n")
        report_lines.append("\n### Function Context\n")
        report_lines.append("```cpp\n")
        report_lines.append(context + "\n")
        report_lines.append("```\n")
        report_lines.append("\n### Assessment\n")
        report_lines.append("- [ ] **TP** - True Positive (real issue)\n")
        report_lines.append("- [ ] **FP** - False Positive (code is safe)\n")
        report_lines.append("- [ ] **?** - Uncertain / Needs investigation\n")
        report_lines.append("\n**Notes:** _[Add your reasoning here]_\n")
    
    # Save report template
    template_out.parent.mkdir(parents=True, exist_ok=True)
    with open(template_out, 'w', encoding='utf-8') as f:
        f.writelines(report_lines)
    
    print("\n" + "=" * 100)
    print(f"[✓] Validation template saved to: {template_out}")
    print("=" * 100)
    
    # Save sample metadata
    metadata = {
        'sample_date': datetime.utcnow().isoformat() + 'Z',
        'seed': args.seed,
        'source_aggregate': str(aggregate_file),
        'sample_size': len(sample),
        'total_gaps': len(all_gaps),
        'by_severity': dict(by_severity),
        'by_category': dict(by_category),
        'by_module': dict(by_module),
        'gaps': [
            {
                'module': g.get('_module'),
                'file': g.get('_file'),
                'line': g.get('line'),
                'severity': g.get('severity'),
                'category': g.get('category'),
                'description': g.get('description')
            }
            for g in sample
        ]
    }
    
    metadata_out.parent.mkdir(parents=True, exist_ok=True)
    with open(metadata_out, 'w') as f:
        json.dump(metadata, f, indent=2)
    
    print(f"[✓] Sample metadata saved to: {metadata_out}\n")

if __name__ == '__main__':
    main()
