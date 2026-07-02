#!/usr/bin/env python3
"""
Phase 1-4 Scanner CI/CD Integration Runner

This script orchestrates the Phase 1-4 scanner suite within CI/CD pipelines,
aggregates results, generates reports, and tracks remediation metrics.

Usage:
    # Run all scanners
    python3 tools/ci_phase_1_4_scanner_runner.py --repo-root . --all

    # Run specific scanner
    python3 tools/ci_phase_1_4_scanner_runner.py --repo-root . --security

    # Compare against baseline and generate metrics
    python3 tools/ci_phase_1_4_scanner_runner.py \
        --repo-root . \
        --all \
        --compare-baseline ai_working/baseline_scan.json \
        --output-metrics metrics.html

    # Generate GitHub issues for top gaps
    python3 tools/ci_phase_1_4_scanner_runner.py \
        --repo-root . \
        --all \
        --generate-issues \
        --batch A

Author: AI Agent (Phase 1-4 Implementation)
Date: 2026-07-02
"""

import json
import argparse
import subprocess
import sys
import time
from pathlib import Path
from typing import Dict, List, Any, Tuple
from datetime import datetime
import shutil


class Phase1_4ScannerRunner:
    """Orchestrate Phase 1-4 scanner suite in CI/CD"""

    def __init__(self, repo_root: str):
        self.repo_root = Path(repo_root)
        self.tools_dir = self.repo_root / "tools"
        self.ai_working_dir = self.repo_root / "ai_working"
        self.results = {}
        self.start_time = None
        self.end_time = None

    def run_scanner(self, scanner_name: str, scanner_script: str) -> Tuple[bool, Dict[str, Any]]:
        """Run individual scanner and capture results"""
        print(f"🔍 Running {scanner_name}...")
        start = time.time()

        try:
            result = subprocess.run(
                [sys.executable, str(self.tools_dir / scanner_script)],
                cwd=str(self.repo_root),
                capture_output=True,
                text=True,
                timeout=120
            )

            elapsed = time.time() - start

            if result.returncode != 0:
                print(f"  ⚠️ Scanner exited with code {result.returncode}")
                print(f"  Stderr: {result.stderr[:200]}")
                return False, {"error": result.stderr, "elapsed": elapsed}

            output = result.stdout
            stats = {
                "elapsed": elapsed,
                "status": "success",
                "output_lines": len(output.split('\n')),
            }

            print(f"  ✅ Completed in {elapsed:.1f}s")
            return True, stats

        except subprocess.TimeoutExpired:
            print(f"  ❌ Scanner timeout (120s)")
            return False, {"error": "timeout", "elapsed": 120}
        except Exception as e:
            print(f"  ❌ Scanner error: {e}")
            return False, {"error": str(e), "elapsed": 0}

    def aggregate_reports(self) -> Dict[str, Any]:
        """Aggregate all scanner JSON outputs"""
        aggregated = {
            "timestamp": datetime.now().isoformat(),
            "repo_root": str(self.repo_root),
            "scanners": {},
            "total_gaps": 0,
            "gaps_by_severity": {"CRITICAL": 0, "HIGH": 0, "MEDIUM": 0},
            "gaps_by_module": {},
        }

        report_patterns = [
            ("security", "fp_tuning_after/gap_scan_v3_security_aggregate.json"),
            ("memory", "fp_tuning_after/gap_scan_v3_memory_aggregate.json"),
            ("concurrency", "fp_tuning_after/gap_scan_v3_concurrency_aggregate.json"),
        ]

        for scanner_name, report_path in report_patterns:
            full_path = self.ai_working_dir / report_path
            
            if not full_path.exists():
                print(f"  ⚠️ Report not found: {report_path}")
                continue

            try:
                with open(full_path) as f:
                    report = json.load(f)
                
                scanner_stats = {
                    "status": "loaded",
                    "modules_scanned": len(report),
                    "total_gaps": 0,
                    "severity_breakdown": {"CRITICAL": 0, "HIGH": 0, "MEDIUM": 0},
                }

                # Aggregate statistics
                for module, module_data in report.items():
                    if not isinstance(module_data, dict):
                        continue
                    
                    module_total = module_data.get('total', 0)
                    scanner_stats['total_gaps'] += module_total
                    
                    severity_critical = module_data.get('severity_critical', 0)
                    severity_high = module_data.get('severity_high', 0)
                    
                    scanner_stats['severity_breakdown']['CRITICAL'] += severity_critical
                    scanner_stats['severity_breakdown']['HIGH'] += severity_high

                    # Track by module
                    if module not in aggregated['gaps_by_module']:
                        aggregated['gaps_by_module'][module] = {}
                    aggregated['gaps_by_module'][module][scanner_name] = module_total

                aggregated['scanners'][scanner_name] = scanner_stats
                aggregated['total_gaps'] += scanner_stats['total_gaps']
                aggregated['gaps_by_severity']['CRITICAL'] += scanner_stats['severity_breakdown']['CRITICAL']
                aggregated['gaps_by_severity']['HIGH'] += scanner_stats['severity_breakdown']['HIGH']

            except (json.JSONDecodeError, IOError) as e:
                print(f"  ❌ Error loading {report_path}: {e}")
                continue

        return aggregated

    def compare_with_baseline(self, baseline_path: str) -> Dict[str, Any]:
        """Compare current scan with baseline"""
        try:
            with open(baseline_path) as f:
                baseline = json.load(f)
        except (FileNotFoundError, json.JSONDecodeError):
            print(f"  ⚠️ Baseline not found or invalid: {baseline_path}")
            return {}

        comparison = {
            "baseline_date": baseline.get("timestamp", "unknown"),
            "current_date": datetime.now().isoformat(),
            "total_gap_change": 0,
            "remediated_gaps": 0,
            "new_gaps": 0,
            "modules_improved": [],
            "modules_regressed": [],
        }

        baseline_total = baseline.get("total_gaps", 0)
        current_total = self.results.get("total_gaps", 0)

        comparison['total_gap_change'] = baseline_total - current_total
        if comparison['total_gap_change'] > 0:
            comparison['remediated_gaps'] = comparison['total_gap_change']
            print(f"  ✅ Gap reduction: {comparison['remediated_gaps']} gaps remediated")
        elif comparison['total_gap_change'] < 0:
            comparison['new_gaps'] = abs(comparison['total_gap_change'])
            print(f"  ⚠️ Gap increase: {comparison['new_gaps']} new gaps detected")

        # Module-level comparison
        baseline_modules = baseline.get("gaps_by_module", {})
        current_modules = self.results.get("gaps_by_module", {})

        for module, current_count in current_modules.items():
            if isinstance(current_count, dict):
                current_total = sum(current_count.values())
            else:
                current_total = current_count
            
            baseline_count = baseline_modules.get(module, {})
            if isinstance(baseline_count, dict):
                baseline_total = sum(baseline_count.values())
            else:
                baseline_total = baseline_count

            if baseline_total > current_total:
                comparison['modules_improved'].append({
                    "module": module,
                    "reduction": baseline_total - current_total,
                    "baseline": baseline_total,
                    "current": current_total,
                })

        return comparison

    def generate_html_report(self, output_file: str, comparison: Dict[str, Any] = None) -> None:
        """Generate HTML report from aggregated results"""
        html_content = f"""<!DOCTYPE html>
<html>
<head>
    <title>Phase 1-4 Scanner Report - {datetime.now().strftime('%Y-%m-%d')}</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }}
        .container {{ max-width: 1200px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; }}
        h1 {{ color: #333; border-bottom: 3px solid #007bff; padding-bottom: 10px; }}
        h2 {{ color: #555; margin-top: 30px; }}
        table {{ border-collapse: collapse; width: 100%; margin: 20px 0; }}
        th, td {{ border: 1px solid #ddd; padding: 12px; text-align: left; }}
        th {{ background: #007bff; color: white; }}
        tr:nth-child(even) {{ background: #f9f9f9; }}
        .stat-box {{ display: inline-block; margin: 10px; padding: 15px; background: #f0f0f0; border-radius: 5px; }}
        .stat-number {{ font-size: 24px; font-weight: bold; color: #007bff; }}
        .stat-label {{ color: #666; font-size: 14px; }}
        .success {{ color: #28a745; }}
        .warning {{ color: #ffc107; }}
        .danger {{ color: #dc3545; }}
    </style>
</head>
<body>
    <div class="container">
        <h1>📊 Phase 1-4 Scanner Report</h1>
        <p><strong>Generated:</strong> {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>
        <p><strong>Repository:</strong> {self.repo_root}</p>
        
        <h2>Overview</h2>
        <div class="stat-box">
            <div class="stat-number">{self.results.get('total_gaps', 0)}</div>
            <div class="stat-label">Total Gaps</div>
        </div>
        <div class="stat-box">
            <div class="stat-number success">{self.results.get('gaps_by_severity', {}).get('CRITICAL', 0)}</div>
            <div class="stat-label">CRITICAL Gaps</div>
        </div>
        <div class="stat-box">
            <div class="stat-number warning">{self.results.get('gaps_by_severity', {}).get('HIGH', 0)}</div>
            <div class="stat-label">HIGH Gaps</div>
        </div>
        <div class="stat-box">
            <div class="stat-number">{len(self.results.get('scanners', {}))}</div>
            <div class="stat-label">Scanners Run</div>
        </div>
        
        <h2>Scanners</h2>
        <table>
            <tr>
                <th>Scanner</th>
                <th>Status</th>
                <th>Gaps</th>
                <th>CRITICAL</th>
                <th>HIGH</th>
            </tr>
"""
        
        for scanner_name, stats in self.results.get('scanners', {}).items():
            status = stats.get('status', 'unknown')
            total = stats.get('total_gaps', 0)
            critical = stats.get('severity_breakdown', {}).get('CRITICAL', 0)
            high = stats.get('severity_breakdown', {}).get('HIGH', 0)
            
            html_content += f"""            <tr>
                <td>{scanner_name}</td>
                <td>{status}</td>
                <td>{total}</td>
                <td class="danger">{critical}</td>
                <td class="warning">{high}</td>
            </tr>
"""
        
        html_content += """        </table>"""
        
        if comparison:
            html_content += f"""
        <h2>Remediation Progress</h2>
        <p><strong>Baseline Date:</strong> {comparison.get('baseline_date', 'N/A')}</p>
        <p><strong>Current Date:</strong> {comparison.get('current_date', 'N/A')}</p>
        <div class="stat-box">
            <div class="stat-number success">{comparison.get('remediated_gaps', 0)}</div>
            <div class="stat-label">Gaps Remediated</div>
        </div>
        <div class="stat-box">
            <div class="stat-number danger">{comparison.get('new_gaps', 0)}</div>
            <div class="stat-label">New Gaps</div>
        </div>
        
        <h2>Improved Modules</h2>
"""
            if comparison.get('modules_improved'):
                html_content += """        <table>
            <tr>
                <th>Module</th>
                <th>Baseline</th>
                <th>Current</th>
                <th>Remediated</th>
            </tr>
"""
                for module in comparison['modules_improved']:
                    html_content += f"""            <tr>
                <td>{module['module']}</td>
                <td>{module['baseline']}</td>
                <td>{module['current']}</td>
                <td class="success">{module['reduction']}</td>
            </tr>
"""
                html_content += """        </table>"""
            else:
                html_content += "        <p>No improvements yet. Remediation in progress.</p>"
        
        html_content += """
    </div>
</body>
</html>
"""
        
        with open(output_file, 'w') as f:
            f.write(html_content)
        
        print(f"✅ Report generated: {output_file}")

    def run_all(self) -> bool:
        """Run all scanners in sequence"""
        self.start_time = time.time()
        print("🚀 Phase 1-4 Scanner Suite Starting...\n")

        scanners = [
            ("Security Scanner", "gap_scanner_v3_security.py"),
            ("Memory Scanner", "gap_scanner_v3_memory.py"),
            ("Concurrency Scanner", "gap_scanner_v3_concurrency.py"),
        ]

        all_success = True
        for scanner_name, scanner_script in scanners:
            success, stats = self.run_scanner(scanner_name, scanner_script)
            self.results[scanner_name] = stats
            if not success:
                all_success = False

        self.end_time = time.time()
        elapsed_total = self.end_time - self.start_time

        print(f"\n✅ All scanners completed in {elapsed_total:.1f}s")
        return all_success

    def run_security(self) -> bool:
        """Run security scanner only"""
        self.start_time = time.time()
        success, stats = self.run_scanner("Security Scanner", "gap_scanner_v3_security.py")
        self.end_time = time.time()
        self.results["Security Scanner"] = stats
        return success

    def run_memory(self) -> bool:
        """Run memory scanner only"""
        self.start_time = time.time()
        success, stats = self.run_scanner("Memory Scanner", "gap_scanner_v3_memory.py")
        self.end_time = time.time()
        self.results["Memory Scanner"] = stats
        return success

    def run_concurrency(self) -> bool:
        """Run concurrency scanner only"""
        self.start_time = time.time()
        success, stats = self.run_scanner("Concurrency Scanner", "gap_scanner_v3_concurrency.py")
        self.end_time = time.time()
        self.results["Concurrency Scanner"] = stats
        return success


def main():
    parser = argparse.ArgumentParser(description="Phase 1-4 Scanner CI/CD Integration")
    parser.add_argument("--repo-root", default=".", help="Repository root directory")
    parser.add_argument("--all", action="store_true", help="Run all scanners")
    parser.add_argument("--security", action="store_true", help="Run security scanner")
    parser.add_argument("--memory", action="store_true", help="Run memory scanner")
    parser.add_argument("--concurrency", action="store_true", help="Run concurrency scanner")
    parser.add_argument("--compare-baseline", help="Baseline JSON for comparison")
    parser.add_argument("--output-metrics", help="Output HTML metrics file")
    parser.add_argument("--generate-issues", action="store_true", help="Generate GitHub issues")
    parser.add_argument("--batch", choices=["A", "B", "C", "D", "E"], help="Batch for issue generation")
    args = parser.parse_args()

    runner = Phase1_4ScannerRunner(args.repo_root)

    # Run scanners
    if args.all:
        runner.run_all()
    elif args.security:
        runner.run_security()
    elif args.memory:
        runner.run_memory()
    elif args.concurrency:
        runner.run_concurrency()
    else:
        parser.print_help()
        return 1

    # Aggregate and analyze
    runner.results = runner.aggregate_reports()

    print(f"\n📊 Aggregated Results:")
    print(f"  Total Gaps: {runner.results.get('total_gaps', 0)}")
    print(f"  CRITICAL: {runner.results.get('gaps_by_severity', {}).get('CRITICAL', 0)}")
    print(f"  HIGH: {runner.results.get('gaps_by_severity', {}).get('HIGH', 0)}")

    # Compare with baseline if provided
    comparison = {}
    if args.compare_baseline:
        print(f"\n📈 Comparing with baseline: {args.compare_baseline}")
        comparison = runner.compare_with_baseline(args.compare_baseline)

    # Generate HTML report if requested
    if args.output_metrics:
        runner.generate_html_report(args.output_metrics, comparison)

    # Generate GitHub issues if requested
    if args.generate_issues and args.batch:
        print(f"\n📝 Generating GitHub issues for Batch {args.batch}...")
        print(f"  Run: python3 tools/generate_github_issues_phase_1_4.py --batch {args.batch}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
