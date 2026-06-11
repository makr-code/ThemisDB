#!/usr/bin/env python3
"""
Gap Scanner V3 — Unified Orchestrator

Main entry point for the gap scanner pipeline.
Loads scanners by priority tier and executes end-to-end.
"""

import sys
import json
from pathlib import Path
import time
from collections import Counter
from typing import Dict, List, Tuple

# Add tools/ to path
sys.path.insert(0, str(Path(__file__).parent))

from gs3_base_scanner import BaseGapScanner, Gap, ScannerRegistry, GapScannerPipeline, ScannerPriority

# Import all scanner classes
from scanners.gs3_step00_uniform_full import UniformFullScanner


def _normalize_path(path: str) -> str:
    return (path or '').replace('\\', '/').lower()


def _classify_scope(path: str) -> str:
    normalized = _normalize_path(path)
    if normalized.startswith('tests/'):
        return 'themis_tests'
    if normalized.startswith('benchmarks/'):
        return 'themis_benchmarks'
    if normalized.startswith(('src/', 'include/', 'tools/', 'scripts/', 'cmake/', 'docs/', 'examples/')):
        return 'themis_core'
    return 'third_party'


def _build_scope_breakdown(gaps: list[Gap]) -> dict:
    """Build scope-aware summary used by console output and exported JSON metadata."""
    scope_counts = Counter()
    for gap in gaps:
        scope_counts[_classify_scope(gap.file)] += 1

    total = len(gaps)
    percentages = {
        key: round((count * 100.0 / total), 2) if total else 0.0
        for key, count in scope_counts.items()
    }

    # Ensure stable keys in output, even when count is zero.
    for key in ('themis_core', 'themis_tests', 'themis_benchmarks', 'third_party'):
        scope_counts.setdefault(key, 0)
        percentages.setdefault(key, 0.0)

    return {
        'policy': {
            'themis_core': ['src/', 'include/', 'tools/', 'scripts/', 'cmake/', 'docs/', 'examples/'],
            'themis_tests': ['tests/'],
            'themis_benchmarks': ['benchmarks/'],
            'third_party': ['all other paths']
        },
        'counts': dict(scope_counts),
        'percentages': percentages
    }


def _classify_module(path: str) -> str:
    normalized = _normalize_path(path).lstrip('./')
    parts = [p for p in normalized.split('/') if p]
    if not parts:
        return '_unscoped'
    if parts[0] == 'src' and len(parts) > 1:
        return parts[1]
    if parts[0] in ('include', 'tests', 'benchmarks', 'internal', 'docs', 'tools'):
        return parts[0]
    return parts[0]


def _classify_gap_class(pattern: str) -> str:
    p = (pattern or '').lower()
    if any(k in p for k in ('data_race', 'deadlock', 'race', 'atomic')):
        return 'Concurrency'
    if any(k in p for k in ('null_dereference', 'buffer', 'pointer', 'use_after', 'double_free', 'size_assumption', 'leak')):
        return 'MemorySafety'
    if any(k in p for k in ('version_tracking', 'conflict_resolution', 'stale_write', 'consistency')):
        return 'VersioningConflict'
    if any(k in p for k in ('missing_doxygen', 'docs_', 'markdown_anchor', 'markdown_link')):
        return 'Documentation'
    if any(k in p for k in ('no_retry', 'timeout', 'circuit_breaker', 'fallback')):
        return 'ReliabilityRetry'
    if any(k in p for k in ('integrity', 'injection', 'signature', 'auth', 'crypto', 'tls')):
        return 'SecurityIntegrity'
    if any(k in p for k in ('layer_dependency', 'interface', 'abstract_', 'i_prefix', 'module_doc', 'adr_reference')):
        return 'ArchitectureContract'
    return 'General'


def _build_ollama_markdown_report(
    gaps: list[Gap],
    output_path: Path,
    source_json_path: Path,
    scan_mode: str,
    docs_doxygen: bool,
    scope_breakdown: dict,
    max_prompt_buckets: int = 0,
    context_chars: int = 120,
    template_lines: int = 10,
) -> None:
    actionable_rows: List[Tuple[int, str, int, str, str, str]] = []
    grouped_actionable: Dict[Tuple[str, str, str, str], int] = Counter()
    grouped_actionable_meta: Dict[Tuple[str, str, str, str], Dict[str, object]] = {}

    for gap in gaps:
        sev = str(gap.severity or 'LOW').upper()
        if sev not in ('CRITICAL', 'HIGH'):
            continue

        scope = _classify_scope(gap.file)
        if scope == 'third_party':
            continue

        module = _classify_module(gap.file)
        pattern = str(gap.type or 'unknown')
        key = (module, gap.file, pattern, sev)
        grouped_actionable[key] += 1

        meta = grouped_actionable_meta.setdefault(
            key,
            {
                'line': int(getattr(gap, 'line', 0) or 0),
                'description': str(getattr(gap, 'description', '') or ''),
                'context': str(getattr(gap, 'context', '') or ''),
            },
        )
        current_line = int(getattr(gap, 'line', 0) or 0)
        if current_line and (not meta.get('line') or current_line < int(meta.get('line') or 0)):
            meta['line'] = current_line
        if not meta.get('description'):
            meta['description'] = str(getattr(gap, 'description', '') or '')
        if not meta.get('context'):
            meta['context'] = str(getattr(gap, 'context', '') or '')

    for (module, file_path, pattern, sev), count in grouped_actionable.items():
        score = count * (2 if sev == 'CRITICAL' else 1)
        actionable_rows.append((score, module, count, sev, file_path, pattern))

    actionable_rows.sort(key=lambda x: (-x[0], -x[2], x[3], x[1], x[4], x[5]))
    prompt_rows = actionable_rows if max_prompt_buckets <= 0 else actionable_rows[:max_prompt_buckets]

    lines: List[str] = []
    lines.append('# ThemisDB Gap Worklist for Remote Ollama gemma4')
    lines.append('')
    lines.append('- [ ] Scope: actionable themis_core findings only (third_party is informational).')
    lines.append('')
    lines.append('## Work Items')
    lines.append('')

    for i, (_score, module, _count, sev, file_path, pattern) in enumerate(prompt_rows, 1):
        meta = grouped_actionable_meta.get((module, file_path, pattern, sev), {})
        line = int(meta.get('line') or 0)
        description = str(meta.get('description') or '').strip() or 'n/a'
        context = str(meta.get('context') or '').strip()
        short_context = 'n/a'
        if context:
            short_context = context.replace('\n', ' ').strip()
            if len(short_context) > context_chars:
                short_context = short_context[:max(0, context_chars - 3)] + '...'

        cls = _classify_gap_class(pattern)
        loc = f'{file_path}:{line if line else "n/a"}'

        lines.append(f'- [ ] {sev} | {module} | {pattern} | {loc}')
        lines.append('```text')
        if template_lines <= 6:
            # strict 6-line ultra-compact block per item
            lines.append('// ROUTING HINT: ollama-local | Model: gemma4:latest')
            lines.append(f'// Class: {cls} | Problem: {pattern}')
            lines.append(f'// Location: {loc}')
            lines.append(f'// Description: {description}')
            lines.append(f'// Context: {short_context}')
            lines.append('Task+Output+Constraints: fix only this finding; return files+diff+tests+scanner-delta; max 3 files; preserve API/ABI; update Doxygen for public C++ API changes.')
        else:
            # strict 10-line compact block per item
            lines.append('// ROUTING HINT: ollama-local')
            lines.append('// Model: gemma4:latest')
            lines.append(f'// Class: {cls}')
            lines.append(f'// Location: {loc}')
            lines.append(f'// Problem: {pattern}')
            lines.append(f'// Description: {description}')
            lines.append(f'// Context: {short_context}')
            lines.append('Task: Fix this finding only with minimal changes.')
            lines.append('Output: files, diff, tests, scanner-delta; keep it concise.')
            lines.append('Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.')
        lines.append('```')
        lines.append('')

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text('\n'.join(lines) + '\n', encoding='utf-8')


def main():
    """Main orchestrator entry point"""
    import argparse
    
    parser = argparse.ArgumentParser(description="ThemisDB Gap Scanner V3 Pipeline")
    parser.add_argument('source_dir', nargs='?', default='./src',
                        help='Source directory to scan (default: ./src)')
    parser.add_argument('--output', '-o', default='ai_working/gap_scan_results.json',
                        help='Output JSON file (default: ai_working/gap_scan_results.json)')
    parser.add_argument('--verbose', '-v', action='store_true',
                        help='Verbose output')
    parser.add_argument('--scan-mode', choices=['fast', 'full'], default='full',
                        help='Scanner mode: fast skips expensive docs checks, full runs all checks (default: full)')
    parser.add_argument('--docs-doxygen', action='store_true',
                        help='Run optional XML-first Doxygen checks inside docs scanner (prefers Doxyfile.audit and validates XML index)')
    parser.add_argument('--md-report', default='ai_working/gap_scan_report_ollama_gemma4.md',
                        help='Write markdown remediation report template for remote Ollama gemma4 (default: ai_working/gap_scan_report_ollama_gemma4.md)')
    parser.add_argument('--md-report-max-prompt-buckets', type=int, default=0,
                        help='Maximum actionable buckets to turn into copy/paste gemma4 prompts (default: 0 = all)')
    parser.add_argument('--md-report-context-chars', type=int, default=120,
                        help='Maximum context characters embedded in each gemma4 prompt (default: 120)')
    parser.add_argument('--md-report-template-lines', type=int, choices=[6, 10], default=10,
                        help='Prompt template size per work item (6 ultra-compact or 10 compact, default: 10)')
    
    args = parser.parse_args()
    
    # Create registry
    registry = ScannerRegistry()

    registry.register(UniformFullScanner(scan_mode=args.scan_mode, docs_doxygen=args.docs_doxygen))
    
    # Create and run pipeline
    pipeline = GapScannerPipeline(registry)
    
    print("\n" + "=" * 80)
    print("ThemisDB Gap Scanner V3 Pipeline")
    print("=" * 80)
    print(f"[CONFIG] scan_mode={args.scan_mode}, docs_doxygen={args.docs_doxygen}")
    
    start_time = time.time()
    gaps = pipeline.execute(args.source_dir, verbose=args.verbose)
    elapsed = time.time() - start_time
    scope_breakdown = _build_scope_breakdown(gaps)
    
    # Export results
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    
    pipeline.export_json(output_path)

    # Enrich exported JSON with scope-separated summary for stable post-processing.
    with open(output_path, 'r', encoding='utf-8') as f:
        exported = json.load(f)
    exported.setdefault('metadata', {})['scope_breakdown'] = scope_breakdown
    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump(exported, f, indent=2)

    md_report_path = Path(args.md_report)
    if not md_report_path.is_absolute():
        md_report_path = Path.cwd() / md_report_path
    _build_ollama_markdown_report(
        gaps=gaps,
        output_path=md_report_path,
        source_json_path=output_path,
        scan_mode=args.scan_mode,
        docs_doxygen=args.docs_doxygen,
        scope_breakdown=scope_breakdown,
        max_prompt_buckets=args.md_report_max_prompt_buckets,
        context_chars=args.md_report_context_chars,
        template_lines=args.md_report_template_lines,
    )
    
    print(f"\n[OK] Results exported to {output_path}")
    print(f"[OK] Markdown report exported to {md_report_path}")
    print(f"[OK] Completed in {elapsed:.2f}s")
    
    # Print summary
    by_severity = {}
    by_type = {}
    
    for gap in gaps:
        sev = gap.severity
        by_severity[sev] = by_severity.get(sev, 0) + 1
        
        typ = gap.type
        by_type[typ] = by_type.get(typ, 0) + 1
    
    print(f"\n[SUMMARY]")
    print(f"Total gaps: {len(gaps)}")
    print(f"\nBy Severity:")
    for sev in ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW']:
        if sev in by_severity:
            print(f"  {sev}: {by_severity[sev]}")
    
    print(f"\nTop Gap Types:")
    for typ, count in sorted(by_type.items(), key=lambda x: -x[1])[:10]:
        print(f"  {typ}: {count}")

    print(f"\nBy Scope (ThemisDB vs Tests/Benchmarks vs Third-Party):")
    for key in ('themis_core', 'themis_tests', 'themis_benchmarks', 'third_party'):
        count = scope_breakdown['counts'].get(key, 0)
        pct = scope_breakdown['percentages'].get(key, 0.0)
        print(f"  {key}: {count} ({pct}%)")
    
    print("=" * 80)
    
    return 0 if gaps else 1


if __name__ == "__main__":
    sys.exit(main())
