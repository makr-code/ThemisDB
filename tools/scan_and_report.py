#!/usr/bin/env python3
"""
Quick Start Guide for Gap Scanner v2

Three simple commands to get started:
1. python scan_and_report.py          # Full pipeline
2. python scan_and_report.py --compare # Compare v1 vs v2
3. python scan_and_report.py --help    # Show options
"""

import subprocess
import sys
import os
from pathlib import Path

def run_command(cmd, description):
    """Run a command and report status"""
    print(f"\n📊 {description}...")
    result = subprocess.run(cmd, shell=True, capture_output=False)
    if result.returncode != 0:
        print(f"❌ {description} failed")
        return False
    print(f"✅ {description} complete")
    return True

def main():
    """Run quick start"""
    
    if '--help' in sys.argv or '-h' in sys.argv:
        print("""
Quick Start: Gap Scanner v2

Commands:
  python scan_and_report.py                  # Full pipeline (scan + report + headers)
  python scan_and_report.py --scan-only      # Scan files only
  python scan_and_report.py --compare        # Compare v1 vs v2
  python scan_and_report.py --headers-only   # Update headers only
  python scan_and_report.py --detailed       # Use detailed multi-line headers
  python scan_and_report.py --no-headers     # Skip header updates

What Each Does:
  - scan_and_report.py
    → Runs gap_audit_pipeline_v2.py
    → Scans all 57 modules
    → Generates JSON reports
    → Updates file headers with statistics
    → Creates summary

  - --compare
    → Compares v1 (old scanner) vs v2 (new scanner)
    → Shows false-positive reduction
    → Validates improvements

  - --headers-only
    → Updates file headers without re-scanning
    → Uses existing scan results

Output Files:
  ai_working/
    ├── gap_scan_v2_aggregate.json       # Summary by module
    ├── gap_scan_v2_summary.json         # Overall metrics
    ├── gap_scan_v2_<module>.json        # Per-module (57 files)
    └── *.md files                       # Reports

File Headers Updated:
  Every source file now starts with:
  // THEMIS_GAP_STATS: gaps=5 unimpl=3 stub=2 ... scanned=2026-05-18

Examples:
  # Run full pipeline
  python scan_and_report.py

  # Just scan (don't update headers)
  python scan_and_report.py --scan-only

  # Compare improvements
  python scan_and_report.py --compare

  # Use detailed headers
  python scan_and_report.py --detailed
        """)
        return
    
    print("=" * 70)
    print("ThemisDB Gap Audit v2 — Quick Start")
    print("=" * 70)
    
    if '--compare' in sys.argv:
        # Compare v1 vs v2
        print("\n🔍 Comparing v1 vs v2 results...")
        run_command(
            'python tools/compare_scanners.py ai_working ai_working',
            'Comparison'
        )
        return
    
    if '--headers-only' in sys.argv:
        # Update headers only
        detailed = '--detailed' in sys.argv
        print("\n🔄 Updating file headers only...")
        cmd = 'python tools/file_header_updater.py ai_working/gap_scan_v2_aggregate.json .'
        if detailed:
            cmd += ' --detailed'
        run_command(cmd, 'Header update')
        return
    
    if '--scan-only' in sys.argv:
        # Scan only, no header update
        run_command(
            'python tools/gap_scanner_v2.py',
            'Gap scan'
        )
        return
    
    # Full pipeline
    detailed = '--detailed' in sys.argv
    no_headers = '--no-headers' in sys.argv
    
    cmd = 'python tools/gap_audit_pipeline_v2.py'
    if no_headers:
        cmd += ' --no-headers'
    if detailed:
        cmd += ' --detailed-headers'
    
    run_command(cmd, 'Full gap audit pipeline')
    
    print("\n" + "=" * 70)
    print("✅ Complete! Results in ai_working/")
    print("=" * 70)
    print("\n📖 Next Steps:")
    print("   1. Review gap_scan_v2_summary.json")
    print("   2. Check file headers: grep 'THEMIS_GAP_STATS' src/**/*.cpp")
    print("   3. Examine top modules: head ai_working/gap_scan_v2_summary.json")
    print("   4. Create GitHub issues: python tools/gap_clusterer.py")

if __name__ == '__main__':
    main()
