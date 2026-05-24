#!/usr/bin/env python3
"""
ThemisDB Complete Gap Audit Pipeline v2

Three-stage process:
1. Scan files for gaps (v2 enhanced scanner)
2. Generate summary reports
3. Update file headers with statistics
"""

import json
import subprocess
import sys
from pathlib import Path
from datetime import datetime

def run_pipeline(repo_root: str = '.', output_dir: str = 'ai_working', 
                update_headers: bool = True, detailed_headers: bool = False):
    """Run complete gap audit pipeline"""
    
    print("=" * 70)
    print("ThemisDB Gap Audit Pipeline v2")
    print("=" * 70)
    
    # Stage 1: Scan
    print("\n[...] STAGE 1: Scanning for implementation gaps...")
    from gap_scanner_v2 import EnhancedGapScanner
    
    scanner = EnhancedGapScanner(repo_root)
    aggregate = scanner.run_full_scan(output_dir)
    
    # Load detailed reports for header update
    output_path = Path(output_dir)
    gap_reports = {}
    for json_file in output_path.glob('gap_scan_v2_*.json'):
        if json_file.name == 'gap_scan_v2_aggregate.json':
            continue
        try:
            with open(json_file) as f:
                gap_reports[json_file.name] = json.load(f)
        except:
            pass
    
    # Stage 2: Generate summary
    print("\n[...] STAGE 2: Generating summary reports...")
    
    total_gaps = 0
    total_unimplemented = 0
    total_stubs = 0
    total_todos = 0
    
    summary_by_category = {}
    summary_by_severity = {}
    
    for module, stats in aggregate.items():
        total_gaps += stats.get('total', 0)
        total_unimplemented += stats.get('unimplemented', 0)
        total_stubs += stats.get('stub_documented', 0) + stats.get('stub_undocumented', 0)
        total_todos += stats.get('todo_item', 0) + stats.get('fixme_item', 0)
    
    # Save summary
    summary = {
        'scan_date': datetime.now().isoformat(),
        'total_gaps': total_gaps,
        'total_modules': len(aggregate),
        'by_severity': {
            'critical': sum(s.get('severity_critical', 0) for s in aggregate.values()),
            'high': sum(s.get('severity_high', 0) for s in aggregate.values()),
            'medium': sum(s.get('severity_medium', 0) for s in aggregate.values()),
            'low': sum(s.get('severity_low', 0) for s in aggregate.values()),
            'intentional': sum(s.get('severity_intentional', 0) for s in aggregate.values()),
        },
        'by_category': {
            'unimplemented': total_unimplemented,
            'stub_documented': sum(s.get('stub_documented', 0) for s in aggregate.values()),
            'stub_undocumented': sum(s.get('stub_undocumented', 0) for s in aggregate.values()),
            'todo_items': total_todos,
            'technical_debt': sum(s.get('technical_debt', 0) for s in aggregate.values()),
        },
        'modules': aggregate
    }
    
    with open(output_path / 'gap_scan_v2_summary.json', 'w') as f:
        json.dump(summary, f, indent=2)
    
    print(f"[OK] Summary saved to {output_path}/gap_scan_v2_summary.json")
    
    # Print summary
    print(f"\n[STATS] Gap Analysis Results:")
    print(f"   Total Gaps Found: {total_gaps}")
    print(f"   Modules Scanned: {len(aggregate)}")
    print(f"   Unimplemented Paths: {total_unimplemented} (CRITICAL)")
    print(f"   STUB Markers: {total_stubs}")
    print(f"   TODO/FIXME Items: {total_todos}")
    
    severity_stats = summary['by_severity']
    print(f"\n   Severity Breakdown:")
    print(f"      [CRITICAL] {severity_stats['critical']}")
    print(f"      [HIGH] {severity_stats['high']}")
    print(f"      [MEDIUM] {severity_stats['medium']}")
    print(f"      [LOW] {severity_stats['low']}")
    print(f"      [OK] {severity_stats['intentional']}")
    
    # Stage 3: Update file headers
    if update_headers:
        print(f"\n[...] STAGE 3: Updating headers via canonical writer (.github/scripts/analyze_code_maturity.py)...")

        maturity_cmd = [
            sys.executable,
            str(Path(repo_root) / '.github' / 'scripts' / 'code_maturity_header_writer.py'),
            '--root', str(repo_root),
        ]

        result = subprocess.run(
            maturity_cmd,
            cwd=repo_root,
            capture_output=True,
            text=True,
        )

        if result.returncode != 0:
            print("[FAIL] Canonical header writer failed")
            if result.stderr:
                print(result.stderr[:500])
            raise RuntimeError('Canonical header writer failed')

        print("[OK] Canonical header writer completed")
    
    # Stage 4: Generate module documentation
    print(f"\n[...] STAGE 4: Generating module gap documentation...")
    from module_doc_generator import ModuleDocumentationGenerator
    
    doc_gen = ModuleDocumentationGenerator(repo_root)
    if doc_gen.load_scan_results(output_path):
        doc_output = output_path / 'module_gaps'
        results = doc_gen.generate_all_module_docs(str(doc_output))
        
        success_count = sum(1 for v in results.values() if v)
        print(f"[OK] Generated {success_count}/{len(results)} module documentation files")
        
        # Generate index
        if doc_gen.generate_module_index(str(doc_output)):
            print(f"[OK] Module index created")
    
    # Stage 5: Top modules report
    print(f"\n[INFO] Top 10 Modules by Gap Count:")
    top_modules = sorted(aggregate.items(), 
                         key=lambda x: x[1].get('total', 0), 
                         reverse=True)[:10]
    for module, stats in top_modules:
        gaps = stats.get('total', 0)
        critical = stats.get('severity_critical', 0)
        high = stats.get('severity_high', 0)
        print(f"   {module:30} {gaps:4} gaps ({critical} critical, {high} high)")
    
    print("\n" + "=" * 70)
    print("[OK] Pipeline Complete!")
    print("=" * 70)
    
    return summary

if __name__ == '__main__':
    import argparse
    
    parser = argparse.ArgumentParser(description='ThemisDB Gap Audit Pipeline v2')
    parser.add_argument('--repo', default='.', help='Repository root')
    parser.add_argument('--output', default='ai_working', help='Output directory')
    parser.add_argument('--no-headers', action='store_true', help='Skip header updates')
    parser.add_argument('--detailed-headers', action='store_true', help='Use detailed headers')
    
    args = parser.parse_args()
    
    run_pipeline(
        repo_root=args.repo,
        output_dir=args.output,
        update_headers=not args.no_headers,
        detailed_headers=args.detailed_headers
    )
