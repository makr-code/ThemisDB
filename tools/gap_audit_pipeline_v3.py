#!/usr/bin/env python3
"""
ThemisDB Complete Gap Audit Pipeline v3

Pipeline stages:
1. Run gap_scanner_v3.py (optional, can reuse existing artifacts)
2. Read v3 aggregate + summary and print consolidated stats
3. Update code maturity headers via canonical writer
4. Generate and mirror complete module docs (MODULE_GAPS.md)
"""

from __future__ import annotations

import json
import subprocess
import sys
from datetime import datetime
from pathlib import Path
from typing import Dict


def _run_command(command: list[str], cwd: Path, description: str) -> None:
    result = subprocess.run(command, cwd=str(cwd), capture_output=True, text=True)
    if result.returncode != 0:
        print(f"[FAIL] {description}")
        if result.stdout.strip():
            print(result.stdout[-1000:])
        if result.stderr.strip():
            print(result.stderr[-1000:])
        raise RuntimeError(f"{description} failed")


def _load_json(path: Path) -> Dict:
    if not path.exists():
        raise FileNotFoundError(f"Missing required artifact: {path}")
    return json.loads(path.read_text(encoding='utf-8'))


def run_pipeline(
    repo_root: str = '.',
    output_dir: str = 'ai_working',
    update_headers: bool = True,
    run_scan: bool = True,
) -> Dict:
    """Run complete v3 gap audit pipeline."""
    root = Path(repo_root).resolve()
    output_path = (root / output_dir).resolve()
    output_path.mkdir(parents=True, exist_ok=True)

    print('=' * 70)
    print('ThemisDB Gap Audit Pipeline v3')
    print('=' * 70)

    if run_scan:
        print('\n[...] STAGE 1: Running gap scanner v3...')
        _run_command(
            [sys.executable, str(root / 'tools' / 'gap_scanner_v3.py')],
            cwd=root,
            description='gap_scanner_v3.py',
        )
        print('[OK] gap_scanner_v3.py completed')
    else:
        print('\n[SKIP] STAGE 1: Scanner run skipped (using existing v3 artifacts)')

    print('\n[...] STAGE 2: Reading v3 aggregate + summary...')
    aggregate = _load_json(output_path / 'gap_scan_v3_aggregate.json')
    scanner_summary = _load_json(output_path / 'gap_scan_v3_summary.json')

    modules = {
        module: payload
        for module, payload in aggregate.items()
        if isinstance(payload, dict)
    }

    total_gaps = sum(int(payload.get('total', 0) or 0) for payload in modules.values())
    summary = {
        'scan_date': datetime.now().isoformat(),
        'total_gaps': total_gaps,
        'total_modules': len(modules),
        'by_severity': {
            'critical': sum(int(payload.get('severity_critical', 0) or 0) for payload in modules.values()),
            'high': sum(int(payload.get('severity_high', 0) or 0) for payload in modules.values()),
            'medium': sum(int(payload.get('severity_medium', 0) or 0) for payload in modules.values()),
            'low': sum(int(payload.get('severity_low', 0) or 0) for payload in modules.values()),
        },
        'scanner_summary': scanner_summary,
    }

    pipeline_summary_path = output_path / 'gap_scan_pipeline_v3_summary.json'
    pipeline_summary_path.write_text(json.dumps(summary, indent=2), encoding='utf-8')
    print(f"[OK] Pipeline summary saved to {pipeline_summary_path}")

    print('\n[STATS] Gap Analysis Results (v3 aggregate):')
    print(f"   Total Gaps Found: {summary['total_gaps']}")
    print(f"   Modules Scanned: {summary['total_modules']}")
    print('   Severity Breakdown:')
    print(f"      [CRITICAL] {summary['by_severity']['critical']}")
    print(f"      [HIGH] {summary['by_severity']['high']}")
    print(f"      [MEDIUM] {summary['by_severity']['medium']}")
    print(f"      [LOW] {summary['by_severity']['low']}")

    if update_headers:
        print('\n[...] STAGE 3: Updating headers via canonical writer...')
        _run_command(
            [
                sys.executable,
                str(root / '.github' / 'scripts' / 'code_maturity_header_writer.py'),
                '--root',
                str(root),
            ],
            cwd=root,
            description='code_maturity_header_writer.py',
        )
        print('[OK] Canonical header writer completed')
    else:
        print('\n[SKIP] STAGE 3: Header update skipped')

    print('\n[...] STAGE 4: Generating complete module documentation...')
    from module_doc_generator import ModuleDocumentationGenerator

    doc_gen = ModuleDocumentationGenerator(str(root))
    if doc_gen.load_scan_results(output_path):
        doc_output = output_path / 'module_gaps'
        results = doc_gen.generate_all_module_docs(str(doc_output))
        success_count = sum(1 for value in results.values() if value)
        print(f"[OK] Generated {success_count}/{len(results)} module documentation files")
        if doc_gen.generate_module_index(str(doc_output)):
            print('[OK] Module index created')
    else:
        raise RuntimeError('Could not load v3 scan results for module documentation')

    print('\n[INFO] Top 10 Modules by Gap Count:')
    top_modules = sorted(
        modules.items(),
        key=lambda item: int(item[1].get('total', 0) or 0),
        reverse=True,
    )[:10]
    for module, payload in top_modules:
        gaps = int(payload.get('total', 0) or 0)
        critical = int(payload.get('severity_critical', 0) or 0)
        high = int(payload.get('severity_high', 0) or 0)
        print(f"   {module:30} {gaps:5} gaps ({critical} critical, {high} high)")

    print('\n' + '=' * 70)
    print('[OK] Pipeline Complete!')
    print('=' * 70)

    return summary


if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description='ThemisDB Gap Audit Pipeline v3')
    parser.add_argument('--repo', default='.', help='Repository root')
    parser.add_argument('--output', default='ai_working', help='Output directory')
    parser.add_argument('--no-headers', action='store_true', help='Skip header updates')
    parser.add_argument('--no-scan', action='store_true', help='Reuse existing v3 artifacts')

    args = parser.parse_args()

    run_pipeline(
        repo_root=args.repo,
        output_dir=args.output,
        update_headers=not args.no_headers,
        run_scan=not args.no_scan,
    )
