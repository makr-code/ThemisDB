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

# Add tools/ to path
sys.path.insert(0, str(Path(__file__).parent))

from gs3_base_scanner import BaseGapScanner, Gap, ScannerRegistry, GapScannerPipeline, ScannerPriority

# Import all scanner classes
from scanners.gs3_step00_uniform_full import UniformFullScanner


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
    
    # Export results
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    
    pipeline.export_json(output_path)
    
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
    
    print("=" * 80)
    
    return 0 if gaps else 1


if __name__ == "__main__":
    sys.exit(main())
