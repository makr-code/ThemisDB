#!/usr/bin/env python3
"""
GS3 CI/CD Integration Script

Quick validation script for use in CI/CD pipelines.
Scans code, checks for critical issues, and generates reports.

Usage:
  python tools/ci_gs3_validate.py [--fail-on-high] [--verbose]

Examples:
  # Basic validation (fail on critical)
  python tools/ci_gs3_validate.py
  
  # Strict mode (fail on high or critical)
  python tools/ci_gs3_validate.py --fail-on-high
  
  # With verbose output
  python tools/ci_gs3_validate.py --verbose

Exit Codes:
  0 - Validation passed
  1 - Critical issues detected (or --fail-on-high with high issues)
  2 - Command error
"""

import subprocess
import json
import sys
import argparse
from pathlib import Path
from datetime import datetime


def run_gs3_scan(directories, scan_mode="fast", verbose=False):
    """Run GS3 scan on specified directories."""
    
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    output_file = f"ai_working/ci_scan_{timestamp}.json"
    
    cmd = ["python", "tools/gs3.py", "scan"] + directories
    cmd.extend(["--scan-mode", scan_mode])
    cmd.extend(["--output", output_file])
    
    if verbose:
        cmd.append("-v")
    
    print(f"[*] Running GS3 scan...")
    print(f"    Directories: {', '.join(directories)}")
    print(f"    Mode: {scan_mode}")
    print(f"    Output: {output_file}\n")
    
    result = subprocess.run(cmd, capture_output=not verbose)
    
    if result.returncode != 0:
        print("[ERROR] GS3 scan failed")
        return None
    
    return output_file


def load_scan_results(filepath):
    """Load and parse scan results JSON."""
    try:
        with open(filepath, 'r') as f:
            return json.load(f)
    except Exception as e:
        print(f"[ERROR] Failed to load results: {e}")
        return None


def analyze_results(results, verbose=False):
    """Analyze scan results and return statistics."""
    
    gaps = results.get('gaps', [])
    
    # Count by severity
    severity_counts = {}
    severity_critical = []
    severity_high = []
    
    for gap in gaps:
        severity = gap.get('severity', 'UNKNOWN')
        severity_counts[severity] = severity_counts.get(severity, 0) + 1
        
        if severity == 'CRITICAL':
            severity_critical.append(gap)
        elif severity == 'HIGH':
            severity_high.append(gap)
    
    # Count by scope
    scope_counts = {}
    for gap in gaps:
        scope = gap.get('scope', 'UNKNOWN')
        scope_counts[scope] = scope_counts.get(scope, 0) + 1
    
    # Count by phase
    phase_counts = {}
    for gap in gaps:
        phase = gap.get('phase', 'UNKNOWN')
        phase_counts[phase] = phase_counts.get(phase, 0) + 1
    
    stats = {
        'total': len(gaps),
        'severity': severity_counts,
        'scope': scope_counts,
        'phase': phase_counts,
        'critical_gaps': severity_critical,
        'high_gaps': severity_high,
    }
    
    if verbose:
        print("[*] Detailed Analysis:")
        print(f"    Total gaps: {stats['total']}")
        print("\n    By Severity:")
        for sev, count in sorted(stats['severity'].items()):
            print(f"      {sev}: {count}")
        print("\n    By Scope:")
        for scope, count in sorted(stats['scope'].items()):
            print(f"      {scope}: {count}")
        print("\n    By Phase:")
        for phase, count in sorted(stats['phase'].items()):
            print(f"      {phase}: {count}")
    
    return stats


def generate_reports(scan_file, verbose=False):
    """Generate Markdown and JSON reports."""
    
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    md_report = f"ai_working/ci_report_{timestamp}.md"
    
    print(f"\n[*] Generating Markdown report: {md_report}")
    
    cmd = ["python", "tools/gs3.py", "report", scan_file, 
           "--format", "md", "--output", md_report]
    
    result = subprocess.run(cmd, capture_output=not verbose)
    
    if result.returncode == 0:
        print(f"[OK] Report generated successfully")
        return md_report
    else:
        print(f"[WARN] Failed to generate report")
        return None


def validate_critical_issues(stats, fail_on_high=False):
    """Check for critical issues and determine pass/fail."""
    
    critical_count = len(stats['critical_gaps'])
    high_count = len(stats['high_gaps'])
    
    print(f"\n[*] Validation Check:")
    print(f"    Critical issues (themis_core): {critical_count}")
    print(f"    High issues: {high_count}")
    
    # Filter for themis_core only (ignore third_party)
    critical_core = [g for g in stats['critical_gaps'] 
                     if g.get('scope') == 'themis_core']
    high_core = [g for g in stats['high_gaps']
                 if g.get('scope') == 'themis_core']
    
    print(f"    Critical in themis_core: {len(critical_core)}")
    print(f"    High in themis_core: {len(high_core)}")
    
    if critical_core:
        print(f"\n[FAIL] {len(critical_core)} CRITICAL issue(s) detected in themis_core")
        for i, gap in enumerate(critical_core[:3], 1):
            print(f"      [{i}] {gap.get('category', 'unknown')}: {gap.get('message', 'no message')[:60]}")
        if len(critical_core) > 3:
            print(f"      ... and {len(critical_core) - 3} more")
        return False
    
    if fail_on_high and high_core:
        print(f"\n[FAIL] {len(high_core)} HIGH issue(s) detected in themis_core (--fail-on-high mode)")
        for i, gap in enumerate(high_core[:3], 1):
            print(f"      [{i}] {gap.get('category', 'unknown')}: {gap.get('message', 'no message')[:60]}")
        if len(high_core) > 3:
            print(f"      ... and {len(high_core) - 3} more")
        return False
    
    print(f"\n[PASS] Validation passed")
    return True


def main():
    """Main CI/CD validation flow."""
    
    parser = argparse.ArgumentParser(
        description='GS3 CI/CD validation script',
        epilog='Exit codes: 0=passed, 1=critical issues, 2=error'
    )
    parser.add_argument('--fail-on-high', action='store_true',
                        help='Fail if HIGH severity issues detected')
    parser.add_argument('--verbose', '-v', action='store_true',
                        help='Verbose output')
    parser.add_argument('--directories', nargs='+', default=['src', 'include'],
                        help='Directories to scan (default: src include)')
    parser.add_argument('--scan-mode', choices=['fast', 'thorough'], default='fast',
                        help='Scan mode (default: fast)')
    
    args = parser.parse_args()
    
    print(f"""
╔═══════════════════════════════════════════════════════════════════════════╗
║                       GS3 CI/CD VALIDATION                               ║
║                 {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}                        ║
╚═══════════════════════════════════════════════════════════════════════════╝
    """)
    
    # Step 1: Run scan
    scan_file = run_gs3_scan(args.directories, args.scan_mode, args.verbose)
    if not scan_file:
        print("[ERROR] Scan failed")
        sys.exit(2)
    
    # Step 2: Load results
    results = load_scan_results(scan_file)
    if not results:
        print("[ERROR] Failed to load results")
        sys.exit(2)
    
    # Step 3: Analyze
    stats = analyze_results(results, args.verbose)
    
    # Step 4: Generate reports
    generate_reports(scan_file, args.verbose)
    
    # Step 5: Validate
    passed = validate_critical_issues(stats, args.fail_on_high)
    
    # Step 6: Exit
    if passed:
        print(f"\n[OK] CI/CD validation completed successfully")
        sys.exit(0)
    else:
        print(f"\n[FAIL] CI/CD validation failed")
        sys.exit(1)


if __name__ == '__main__':
    main()
