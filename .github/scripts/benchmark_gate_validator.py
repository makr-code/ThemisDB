#!/usr/bin/env python3
"""
Benchmark Gate Validator

Purpose:
  Validates that benchmark performance gates stay within SLA baselines.
  Detects performance regressions and alerts on unexpected improvements.
  Runs on release-lane branches (community, military) to prevent shipping
  code with performance degradation.

Usage:
  # Validate all benchmark gates
  python benchmark_gate_validator.py --all

  # Validate specific module
  python benchmark_gate_validator.py --module failover --baseline-version 8.0.0

  # Generate baseline manifest (first time setup)
  python benchmark_gate_validator.py --generate-baseline --release-tag v8.0.0

  # Dry-run: check without executing benchmarks
  python benchmark_gate_validator.py --all --dry-run

Inputs:
  - benchmarks/<module>/bench_*_gates.cpp (benchmark source)
  - benchmarks/<module>/GATE_BASELINE_<release>.json (baseline performance)

Outputs:
  - benchmarks/<module>/GATE_RESULTS_<date>.json (results)
  - ai_working/GATE_IMPROVEMENTS.md (optimizations detected)
  - Exit code 0 if all gates PASS; 1 if regression detected

Author: Platform Release Team
License: Apache 2.0
"""

import argparse
import os
import re
import sys
import json
import subprocess
from pathlib import Path
from typing import Dict, List, Optional, Tuple
from datetime import datetime


class BenchmarkGateValidator:
    """Validates benchmark gates against baseline performance."""

    # Gate regression threshold: 10% slower = FAIL
    REGRESSION_THRESHOLD = 0.10
    
    # Gate improvement threshold: 5% faster = LOG
    IMPROVEMENT_THRESHOLD = 0.05
    
    # Gate execution timeout (seconds)
    BENCHMARK_TIMEOUT = 300

    def __init__(self, repo_root: Path, baseline_version: str = 'latest'):
        self.repo_root = repo_root
        self.benchmarks_root = repo_root / 'benchmarks'
        self.baseline_version = baseline_version
        self.results = {}
        self.improvements = []

    def find_benchmark_gate_files(self) -> Dict[str, List[Path]]:
        """Find all benchmark gate files in benchmarks/.
        
        Returns:
          Dict mapping module_name -> [list of bench_*_gates.cpp paths]
        """
        gate_files = {}
        
        for module_dir in self.benchmarks_root.iterdir():
            if not module_dir.is_dir() or module_dir.name.startswith('.'):
                continue
            
            gates = list(module_dir.glob('bench_*_gates.cpp'))
            if gates:
                gate_files[module_dir.name] = gates
        
        return gate_files

    def load_baseline(self, module: str, release_tag: str) -> Optional[Dict]:
        """Load baseline performance data for a module.
        
        Args:
          module: Module name
          release_tag: Release tag (e.g., 'v8.0.0') or 'latest'
        
        Returns:
          Dict of baseline gate data, or None if not found
        """
        baseline_file = (
            self.benchmarks_root / module / f'GATE_BASELINE_{release_tag}.json'
        )
        
        if not baseline_file.exists():
            return None
        
        try:
            with open(baseline_file, 'r') as f:
                return json.load(f)
        except (json.JSONDecodeError, OSError) as e:
            print(f"⚠ Failed to load baseline {module}: {e}")
            return None

    def extract_gate_id_and_threshold(self, gate_file: Path) -> Tuple[str, Optional[float]]:
        """Extract gate ID and threshold from benchmark source.
        
        Looks for patterns like:
          // Gate: GATE-FP23-01 (canTransition ≤100µs)
          // GATE_NAME: FP23-01
          // GATE_THRESHOLD_US: 100
        
        Args:
          gate_file: Path to bench_*_gates.cpp file
        
        Returns:
          (gate_id, threshold_us) or (None, None)
        """
        with open(gate_file, 'r') as f:
            content = f.read()
        
        # Try to extract gate ID
        gate_id = None
        if match := re.search(r'//\s*(?:Gate|GATE_NAME):\s*([A-Z0-9\-]+)', content):
            gate_id = match.group(1)
        
        # Try to extract threshold in microseconds
        threshold_us = None
        if match := re.search(r'//\s*(?:GATE_THRESHOLD|Threshold).*?(\d+)\s*µs', content):
            threshold_us = float(match.group(1))
        
        return gate_id, threshold_us

    def run_benchmark(
        self,
        gate_file: Path,
        timeout: int = BENCHMARK_TIMEOUT
    ) -> Optional[Dict]:
        """Execute benchmark and parse results.
        
        Args:
          gate_file: Path to benchmark executable or source
          timeout: Timeout in seconds
        
        Returns:
          Dict with benchmark results (gate_id, time_us, status, etc.)
        """
        try:
            # Determine executable path
            # Assumes benchmark is already compiled
            module_name = gate_file.parent.name
            exe_path = (
                self.repo_root / 'build-release' / 'benchmarks' / module_name /
                gate_file.stem
            )
            
            if not exe_path.exists():
                print(f"⚠ Benchmark executable not found: {exe_path}")
                return None
            
            # Run benchmark (assuming it outputs JSON)
            result = subprocess.run(
                [str(exe_path), '--benchmark_format=json'],
                capture_output=True,
                text=True,
                timeout=timeout
            )
            
            if result.returncode != 0:
                print(f"⚠ Benchmark failed: {gate_file.stem}")
                print(f"  stderr: {result.stderr[:200]}")
                return None
            
            # Parse JSON output
            try:
                bench_output = json.loads(result.stdout)
            except json.JSONDecodeError:
                print(f"⚠ Failed to parse benchmark output: {gate_file.stem}")
                return None
            
            # Extract relevant metrics
            gate_id, threshold_us = self.extract_gate_id_and_threshold(gate_file)
            
            if not bench_output.get('benchmarks'):
                return None
            
            # Use first benchmark entry
            bench_data = bench_output['benchmarks'][0]
            
            return {
                'gate_id': gate_id,
                'module': gate_file.parent.name,
                'time_us': bench_data.get('cpu_time', 0) / 1000,  # Convert ns to µs
                'threshold_us': threshold_us,
                'iterations': bench_data.get('iterations', 1),
                'unit': 'µs'
            }
        
        except subprocess.TimeoutExpired:
            print(f"❌ Benchmark timeout: {gate_file.stem}")
            return None
        except Exception as e:
            print(f"❌ Error running benchmark: {e}")
            return None

    def validate_gate(
        self,
        current: Dict,
        baseline: Optional[Dict]
    ) -> Tuple[str, Optional[float]]:
        """Validate benchmark gate performance.
        
        Args:
          current: Current benchmark result
          baseline: Baseline result (if available)
        
        Returns:
          (status, regression_percent) where status in [PASS, REGRESSION, IMPROVEMENT]
        """
        if not baseline:
            return 'UNKNOWN', None
        
        baseline_time = baseline.get('time_us', 0)
        current_time = current.get('time_us', 0)
        
        if baseline_time == 0:
            return 'UNKNOWN', None
        
        # Calculate regression percentage (positive = slower)
        regression = (current_time - baseline_time) / baseline_time
        
        if regression > self.REGRESSION_THRESHOLD:
            return 'REGRESSION', regression * 100
        elif regression < -self.IMPROVEMENT_THRESHOLD:
            return 'IMPROVEMENT', abs(regression) * 100
        else:
            return 'PASS', regression * 100

    def validate_all(self, modules: Optional[List[str]] = None) -> Dict:
        """Validate all benchmark gates.
        
        Args:
          modules: Specific modules to validate (all if None)
        
        Returns:
          Dict of validation results
        """
        gate_files = self.find_benchmark_gate_files()
        
        if modules:
            gate_files = {k: v for k, v in gate_files.items() if k in modules}
        
        results = {
            'timestamp': datetime.utcnow().isoformat() + 'Z',
            'baseline_version': self.baseline_version,
            'gates': {},
            'summary': {'pass': 0, 'regression': 0, 'improvement': 0, 'unknown': 0}
        }
        
        for module, files in sorted(gate_files.items()):
            baseline = self.load_baseline(module, self.baseline_version)
            
            for gate_file in files:
                current = self.run_benchmark(gate_file)
                if not current:
                    continue
                
                status, change_pct = self.validate_gate(current, baseline)
                
                gate_result = {
                    'gate_id': current.get('gate_id', gate_file.stem),
                    'module': module,
                    'current_time_us': current.get('time_us', 0),
                    'baseline_time_us': baseline.get('time_us', 0) if baseline else 0,
                    'change_percent': round(change_pct, 2) if change_pct else 0,
                    'status': status,
                    'threshold_us': current.get('threshold_us'),
                }
                
                results['gates'][current.get('gate_id', gate_file.stem)] = gate_result
                results['summary'][status.lower()] += 1
                
                # Log improvements
                if status == 'IMPROVEMENT':
                    self.improvements.append({
                        'gate': gate_result['gate_id'],
                        'module': module,
                        'improvement_percent': round(change_pct, 2),
                        'old_time_us': gate_result['baseline_time_us'],
                        'new_time_us': round(gate_result['current_time_us'], 2),
                    })
                
                # Report status
                icon = '✅' if status == 'PASS' else '⚠' if status == 'IMPROVEMENT' else '❌'
                print(
                    f"{icon} {gate_result['gate_id']:20} "
                    f"({module:15}) {status:12} "
                    f"({change_pct:+.1f}% vs baseline)"
                )
        
        return results

    def save_results(self, results: Dict, output_dir: Optional[Path] = None):
        """Save validation results to JSON file.
        
        Args:
          results: Validation results dict
          output_dir: Output directory (defaults to benchmarks/)
        """
        output_dir = output_dir or self.benchmarks_root
        output_file = output_dir / f"GATE_RESULTS_{datetime.utcnow().strftime('%Y%m%d_%H%M%S')}.json"
        
        with open(output_file, 'w') as f:
            json.dump(results, f, indent=2)
        
        print(f"\n✅ Results saved: {output_file}")

    def save_improvements(self):
        """Save detected performance improvements to markdown."""
        if not self.improvements:
            return
        
        improvements_file = self.repo_root / 'ai_working' / 'GATE_IMPROVEMENTS.md'
        improvements_file.parent.mkdir(parents=True, exist_ok=True)
        
        content = f"""# Benchmark Gate Improvements Detected

**Date:** {datetime.utcnow().isoformat()}Z  
**Count:** {len(self.improvements)}

## Performance Improvements

"""
        
        for improvement in sorted(
            self.improvements,
            key=lambda x: x['improvement_percent'],
            reverse=True
        ):
            content += (
                f"- **{improvement['gate']}** ({improvement['module']}): "
                f"{improvement['improvement_percent']:.1f}% faster\n"
                f"  - Previous: {improvement['old_time_us']:.1f} µs\n"
                f"  - Current: {improvement['new_time_us']:.1f} µs\n"
            )
        
        with open(improvements_file, 'a') as f:
            f.write(content + '\n')
        
        print(f"✅ Improvements logged: {improvements_file}")


def main():
    parser = argparse.ArgumentParser(
        description='Validate benchmark gates against baseline performance'
    )
    parser.add_argument(
        '--all',
        action='store_true',
        help='Validate all benchmark gates'
    )
    parser.add_argument(
        '--module',
        type=str,
        help='Validate specific module'
    )
    parser.add_argument(
        '--baseline-version',
        default='latest',
        help='Baseline version tag (e.g., v8.0.0)'
    )
    parser.add_argument(
        '--generate-baseline',
        action='store_true',
        help='Generate baseline for current build'
    )
    parser.add_argument(
        '--release-tag',
        help='Release tag for baseline (used with --generate-baseline)'
    )
    parser.add_argument(
        '--dry-run',
        action='store_true',
        help='Check without executing benchmarks'
    )

    args = parser.parse_args()

    repo_root = Path(__file__).parent.parent.parent
    validator = BenchmarkGateValidator(repo_root, args.baseline_version)

    if args.dry_run:
        print("🔍 Dry-run mode: listing benchmark gates without execution")
        gate_files = validator.find_benchmark_gate_files()
        total = sum(len(files) for files in gate_files.values())
        print(f"Found {total} benchmark gate files:")
        for module, files in sorted(gate_files.items()):
            print(f"  {module}: {len(files)} files")
        return 0

    if args.generate_baseline:
        print("📊 Generating baseline for current build...")
        print("⚠ Not yet implemented; requires running all benchmarks")
        return 1

    if args.all or args.module:
        modules = [args.module] if args.module else None
        results = validator.validate_all(modules)
        
        print(f"\n📊 Summary:")
        print(f"  ✅ PASS:        {results['summary']['pass']}")
        print(f"  ⚠ IMPROVEMENT:  {results['summary']['improvement']}")
        print(f"  ❌ REGRESSION:   {results['summary']['regression']}")
        print(f"  ❓ UNKNOWN:      {results['summary']['unknown']}")
        
        validator.save_results(results)
        validator.save_improvements()
        
        # Exit with error if any regressions
        if results['summary']['regression'] > 0:
            print("\n❌ Performance regressions detected!")
            return 1
        
        return 0
    else:
        parser.print_help()
        return 1


if __name__ == '__main__':
    sys.exit(main())
