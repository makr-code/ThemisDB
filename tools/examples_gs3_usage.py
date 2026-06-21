#!/usr/bin/env python3
"""
GS3 Example Usage Scripts

This script demonstrates common usage patterns for the Gap Scanner V3 (GS3) CLI.
Run individual examples by uncommenting the desired function call.

Author: ThemisDB Development
Date: 2026-06-21
"""

import subprocess
import json
import sys
from pathlib import Path
from datetime import datetime


def run_command(cmd, description):
    """Run a shell command and display results."""
    print(f"\n{'='*80}")
    print(f"[EXAMPLE] {description}")
    print(f"{'='*80}")
    print(f"Command: {cmd}\n")
    
    result = subprocess.run(cmd, shell=True, capture_output=False, text=True)
    if result.returncode != 0:
        print(f"[ERROR] Command failed with exit code {result.returncode}")
        return False
    return True


def example_1_list_all_scanners():
    """Example 1: List all 46 scanners"""
    cmd = "python tools/gs3.py list-scanners"
    run_command(cmd, "List all 46 scanners")


def example_2_list_phase_1_scanners():
    """Example 2: List Phase 1 scanners only"""
    cmd = "python tools/gs3.py list-scanners --step 1"
    run_command(cmd, "List Phase 1 (baseline) scanners only")


def example_3_list_security_scanners():
    """Example 3: List Phase 3 (Security) scanners"""
    cmd = "python tools/gs3.py list-scanners --step 3"
    run_command(cmd, "List Phase 3 (security) scanners")


def example_4_quick_scan_include():
    """Example 4: Run quick scan on include/ directory"""
    cmd = "python tools/gs3.py scan include --scan-mode fast --output ai_working/example_scan_include.json"
    run_command(cmd, "Quick scan of include/ directory (fast mode)")


def example_5_scan_src():
    """Example 5: Scan src/ directory thoroughly"""
    cmd = "python tools/gs3.py scan src --scan-mode thorough --output ai_working/example_scan_src.json"
    run_command(cmd, "Thorough scan of src/ directory")


def example_6_generate_markdown_report():
    """Example 6: Generate Markdown report from scan results"""
    cmd = "python tools/gs3.py report ai_working/example_scan_include.json --format md --output ai_working/example_report.md"
    run_command(cmd, "Generate Markdown report")


def example_7_generate_json_report():
    """Example 7: Generate JSON report from scan results"""
    cmd = "python tools/gs3.py report ai_working/example_scan_include.json --format json --output ai_working/example_report.json"
    run_command(cmd, "Generate JSON report (machine-readable)")


def example_8_view_config():
    """Example 8: View current GS3 configuration"""
    cmd = "python tools/gs3.py config --show"
    run_command(cmd, "View current GS3 configuration")


def example_9_scan_multiple_dirs():
    """Example 9: Scan multiple directories in one pass"""
    cmd = "python tools/gs3.py scan src include benchmarks --scan-mode fast --output ai_working/example_scan_multi.json"
    run_command(cmd, "Scan multiple directories (src, include, benchmarks)")


def example_10_scan_with_verbose():
    """Example 10: Run scan with verbose output"""
    cmd = "python tools/gs3.py scan include --scan-mode fast -v --output ai_working/example_scan_verbose.json"
    run_command(cmd, "Scan with verbose output for debugging")


def example_11_ci_cd_pipeline():
    """Example 11: CI/CD Pipeline - Quick validation"""
    print(f"\n{'='*80}")
    print("[EXAMPLE] CI/CD Pipeline - Quick validation")
    print(f"{'='*80}")
    print("\nThis example shows how to use GS3 in a CI/CD pipeline:")
    print("""
# 1. Run quick scan
python tools/gs3.py scan src --scan-mode fast --output ci_scan.json

# 2. Check for critical blockers
if grep -q '\\"severity\\":\\"CRITICAL\\"\\"' ci_scan.json; then
    echo "FAILED: Critical issues detected"
    exit 1
fi

# 3. Generate report for review
python tools/gs3.py report ci_scan.json --format md --output ci_report.md

# 4. Fail on high-priority security gaps
if grep -q '\\"category\\":\\"security\\"' ci_scan.json && \\
   grep -q '\\"severity\\":\\"HIGH\\"' ci_scan.json; then
    echo "WARNING: Security issues detected"
fi

echo "CI/CD validation passed"
exit 0
    """)


def example_12_analyze_results():
    """Example 12: Analyze scan results with Python"""
    print(f"\n{'='*80}")
    print("[EXAMPLE] Analyze scan results with Python")
    print(f"{'='*80}")
    print("\nThis example shows how to analyze GS3 results programmatically:")
    print("""
import json

# Load scan results
with open('ai_working/example_scan_include.json', 'r') as f:
    results = json.load(f)

# Analyze by severity
severity_counts = {}
for gap in results['gaps']:
    severity = gap.get('severity', 'UNKNOWN')
    severity_counts[severity] = severity_counts.get(severity, 0) + 1

print("Gaps by Severity:")
for severity, count in sorted(severity_counts.items()):
    print(f"  {severity}: {count}")

# Analyze by category
category_counts = {}
for gap in results['gaps']:
    category = gap.get('category', 'UNKNOWN')
    category_counts[category] = category_counts.get(category, 0) + 1

print("\\nGaps by Category:")
for category, count in sorted(category_counts.items(), key=lambda x: -x[1])[:10]:
    print(f"  {category}: {count}")

# Find critical issues in themis_core
critical_core = [g for g in results['gaps']
                 if g.get('severity') == 'CRITICAL'
                 and g.get('scope') == 'themis_core']
print(f"\\nCritical issues in themis_core: {len(critical_core)}")
    """)


def example_13_integration_test_suite():
    """Example 13: Run integration test suite"""
    cmd = "python tools/test_gs3_integration.py"
    run_command(cmd, "Run GS3 integration test suite")


def example_14_list_design_scanners():
    """Example 14: List Design rules scanners (Phase 4)"""
    cmd = "python tools/gs3.py list-scanners --step 4"
    run_command(cmd, "List Phase 4 (design & quality) scanners")


def example_15_custom_analysis():
    """Example 15: Custom analysis example"""
    print(f"\n{'='*80}")
    print("[EXAMPLE] Custom analysis workflow")
    print(f"{'='*80}")
    print("""
# 1. Scan specific directory
python tools/gs3.py scan src/core --scan-mode fast --output core_scan.json

# 2. Generate both reports
python tools/gs3.py report core_scan.json --format md --output core_report.md
python tools/gs3.py report core_scan.json --format json --output core_report.json

# 3. Analyze with custom script
python -c "
import json
with open('core_scan.json') as f:
    data = json.load(f)
    
# Count by phase
phases = {}
for gap in data['gaps']:
    phase = gap.get('phase', 'unknown')
    phases[phase] = phases.get(phase, 0) + 1
    
print('Gaps by Phase:', phases)

# Top 5 gap types
gap_types = {}
for gap in data['gaps']:
    gt = gap.get('category', 'unknown')
    gap_types[gt] = gap_types.get(gt, 0) + 1
    
print('Top gap types:', sorted(gap_types.items(), key=lambda x: -x[1])[:5])
"

# 4. Review markdown report
cat core_report.md | head -100
    """)


def main():
    """Main entry point with example selection menu."""
    
    print("""
╔════════════════════════════════════════════════════════════════════════════╗
║                  GS3 (GAP SCANNER V3) - USAGE EXAMPLES                     ║
║                         Production-Ready Examples                          ║
╚════════════════════════════════════════════════════════════════════════════╝

This script demonstrates 15 common usage patterns for Gap Scanner V3.

Available Examples:
  1. List all 46 scanners
  2. List Phase 1 (baseline) scanners
  3. List Phase 3 (security) scanners
  4. Quick scan (include/)
  5. Thorough scan (src/)
  6. Generate Markdown report
  7. Generate JSON report
  8. View configuration
  9. Scan multiple directories
 10. Scan with verbose output
 11. CI/CD pipeline example (code)
 12. Analyze results with Python (code)
 13. Run integration tests
 14. List Phase 4 (design & quality) scanners
 15. Custom analysis workflow (code)

Quick reference:
  # List all scanners
  python tools/gs3.py list-scanners
  
  # Run scan
  python tools/gs3.py scan src include --scan-mode fast
  
  # Generate report
  python tools/gs3.py report results.json --format md

For full documentation, see: tools/GS3_CLI_GUIDE.md
    """)
    
    # Map examples to functions
    examples = {
        '1': example_1_list_all_scanners,
        '2': example_2_list_phase_1_scanners,
        '3': example_3_list_security_scanners,
        '4': example_4_quick_scan_include,
        '5': example_5_scan_src,
        '6': example_6_generate_markdown_report,
        '7': example_7_generate_json_report,
        '8': example_8_view_config,
        '9': example_9_scan_multiple_dirs,
        '10': example_10_scan_with_verbose,
        '11': example_11_ci_cd_pipeline,
        '12': example_12_analyze_results,
        '13': example_13_integration_test_suite,
        '14': example_14_list_design_scanners,
        '15': example_15_custom_analysis,
    }
    
    if len(sys.argv) > 1:
        example_num = sys.argv[1]
        if example_num in examples:
            examples[example_num]()
        else:
            print(f"ERROR: Unknown example '{example_num}'")
            print(f"Valid examples: {', '.join(examples.keys())}")
            sys.exit(1)
    else:
        # Run all examples
        for num in sorted(examples.keys(), key=int):
            try:
                examples[num]()
            except KeyboardInterrupt:
                print("\n[INTERRUPTED] Stopped by user")
                break
            except Exception as e:
                print(f"\n[ERROR] {e}")


if __name__ == '__main__':
    main()
