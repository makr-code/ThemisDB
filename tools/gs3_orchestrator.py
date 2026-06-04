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
    
    print(f"\n[OK] Results exported to {output_path}")
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
