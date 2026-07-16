#!/usr/bin/env python3
"""
Quick Start Guide for Gap Scanner v3

Three simple commands to get started:
1. python scan_and_report.py           # Full pipeline
2. python scan_and_report.py --scan-only # Run scanner only
3. python scan_and_report.py --help     # Show options
"""

import subprocess
import sys
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
Quick Start: Gap Scanner v3

Commands:
  python scan_and_report.py                  # Full pipeline (scan + report + headers)
  python scan_and_report.py --scan-only      # Scan files only
    python scan_and_report.py --compare        # Compare legacy v1/v2 baselines
  python scan_and_report.py --headers-only   # Update headers only
  python scan_and_report.py --no-headers     # Skip header updates

What Each Does:
  - scan_and_report.py
        → Runs gap_audit_pipeline_v3.py
        → Scans all module sources
    → Generates JSON reports
        → Updates file headers via canonical writer
        → Creates module gap notes

  - --compare
        → Compares legacy v1 vs v2 scanner outputs
    → Shows false-positive reduction
        → Keeps historical trend checks available

  - --headers-only
    → Updates file headers without re-scanning
        → Uses canonical code maturity writer

Output Files:
  ai_working/
        ├── gap_scan_v3_aggregate.json       # Summary by module
        ├── gap_scan_v3_summary.json         # Scanner summary
        ├── gap_scan_pipeline_v3_summary.json # Pipeline summary
        ├── gap_scan_v3_<module>.json        # Per-module details
    └── *.md files                       # Reports

Examples:
  # Run full pipeline
  python scan_and_report.py

  # Just scan (don't update headers)
  python scan_and_report.py --scan-only

    # Compare legacy improvements
  python scan_and_report.py --compare
        """)
        return
    
    print("=" * 70)
    print("ThemisDB Gap Audit v3 — Quick Start")
    print("=" * 70)
    
    if '--compare' in sys.argv:
        # Compare legacy v1 vs v2
        print("\n🔍 Comparing legacy v1 vs v2 results...")
        run_command(
            'python tools/compare_scanners.py ai_working ai_working',
            'Comparison'
        )
        return
    
    if '--headers-only' in sys.argv:
        # Update headers only
        print("\n🔄 Updating file headers only...")
        cmd = 'python .github/scripts/code_maturity_header_writer.py --root .'
        run_command(cmd, 'Header update')
        return
    
    if '--scan-only' in sys.argv:
        # Scan only, no header update
        run_command(
            'python tools/gap_scanner_v3.py',
            'Gap scan'
        )
        return
    
    # Full pipeline
    no_headers = '--no-headers' in sys.argv
    
    cmd = 'python tools/gap_audit_pipeline_v3.py'
    if no_headers:
        cmd += ' --no-headers'
    
    run_command(cmd, 'Full gap audit pipeline')
    
    print("\n" + "=" * 70)
    print("✅ Complete! Results in ai_working/")
    print("=" * 70)
    print("\n📖 Next Steps:")
    print("   1. Review gap_scan_v3_summary.json")
    print("   2. Review pipeline snapshot: gap_scan_pipeline_v3_summary.json")
    print("   3. Examine top modules in ai_working/module_gaps/MODULE_GAPS_INDEX.md")
    print("   4. Create GitHub issues: python tools/gap_clusterer.py")

if __name__ == '__main__':
    main()
