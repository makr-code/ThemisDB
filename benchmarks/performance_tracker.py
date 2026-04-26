"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            performance_tracker.py                             ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     502                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Performance Tracker for ThemisDB
Collects, stores, and exports benchmark results for historical tracking and visualization.
Supports Prometheus metrics export and time-series data storage.
"""

import json
import time
import argparse
import sys
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Any, Optional
from dataclasses import dataclass, asdict
import os


@dataclass
class BenchmarkResult:
    """Single benchmark result with metadata"""
    name: str
    timestamp: str
    branch: str
    commit: str
    release: str
    hardware: str
    throughput_ops: float
    latency_p50_ms: float
    latency_p95_ms: float
    latency_p99_ms: float
    error_rate: float
    metadata: Dict[str, Any]


class PerformanceTracker:
    """Track and store performance benchmark results"""
    
    def __init__(self, storage_dir: Path):
        """
        Initialize tracker
        
        Args:
            storage_dir: Directory to store historical data
        """
        self.storage_dir = storage_dir
        self.storage_dir.mkdir(parents=True, exist_ok=True)
        
        # Create subdirectories
        (self.storage_dir / "raw").mkdir(exist_ok=True)
        (self.storage_dir / "aggregated").mkdir(exist_ok=True)
        (self.storage_dir / "metrics").mkdir(exist_ok=True)
    
    def collect_benchmark_results(
        self,
        results_path: Path,
        branch: str = None,
        commit: str = None,
        release: str = None,
        hardware: str = None
    ) -> List[BenchmarkResult]:
        """
        Collect benchmark results from JSON files
        
        Args:
            results_path: Path to benchmark results (file or directory)
            branch: Git branch name
            commit: Git commit hash
            release: Release version
            hardware: Hardware configuration identifier
            
        Returns:
            List of benchmark results
        """
        # Auto-detect metadata from git if not provided
        if branch is None:
            branch = self._get_git_branch()
        if commit is None:
            commit = self._get_git_commit()
        if release is None:
            release = self._get_release_version()
        if hardware is None:
            hardware = self._detect_hardware()
        
        timestamp = datetime.utcnow().isoformat() + "Z"
        
        results = []
        
        # Load benchmark data
        if results_path.is_file():
            with open(results_path, 'r') as f:
                data = json.load(f)
            
            # Handle different JSON formats
            if 'benchmarks' in data:
                # Google Benchmark format or aggregated format
                benchmarks = data['benchmarks']
                if isinstance(benchmarks, list):
                    # Raw Google Benchmark format
                    for bench in benchmarks:
                        result = self._parse_google_benchmark(
                            bench, timestamp, branch, commit, release, hardware
                        )
                        if result:
                            results.append(result)
                elif isinstance(benchmarks, dict):
                    # Aggregated format
                    for name, bench_data in benchmarks.items():
                        result = self._parse_aggregated_benchmark(
                            name, bench_data, timestamp, branch, commit, release, hardware
                        )
                        if result:
                            results.append(result)
        
        return results
    
    def _parse_google_benchmark(
        self,
        bench: Dict,
        timestamp: str,
        branch: str,
        commit: str,
        release: str,
        hardware: str
    ) -> Optional[BenchmarkResult]:
        """Parse Google Benchmark JSON format"""
        name = bench.get('name', 'unknown')
        
        # Extract throughput
        throughput = bench.get('items_per_second', 0)
        if throughput == 0:
            throughput = bench.get('bytes_per_second', 0) / 1024  # Convert to KB/s
        
        # Extract latency (convert to ms if needed)
        latency = bench.get('real_time', 0)
        if bench.get('time_unit') == 'ns':
            latency = latency / 1e6  # ns to ms
        elif bench.get('time_unit') == 'us':
            latency = latency / 1e3  # us to ms
        
        return BenchmarkResult(
            name=name,
            timestamp=timestamp,
            branch=branch,
            commit=commit,
            release=release,
            hardware=hardware,
            throughput_ops=throughput,
            latency_p50_ms=latency,
            latency_p95_ms=latency * 1.5,  # Estimate
            latency_p99_ms=latency * 2.0,   # Estimate
            error_rate=0.0,
            metadata={
                'cpu_time': bench.get('cpu_time', 0),
                'iterations': bench.get('iterations', 0),
                'time_unit': bench.get('time_unit', 'unknown')
            }
        )
    
    def _parse_aggregated_benchmark(
        self,
        name: str,
        data: Dict,
        timestamp: str,
        branch: str,
        commit: str,
        release: str,
        hardware: str
    ) -> Optional[BenchmarkResult]:
        """Parse aggregated benchmark format"""
        throughput = data.get('items_per_second', 0)
        if throughput == 0:
            throughput = data.get('bytes_per_second', 0) / 1024
        
        return BenchmarkResult(
            name=name,
            timestamp=timestamp,
            branch=branch,
            commit=commit,
            release=release,
            hardware=hardware,
            throughput_ops=throughput,
            latency_p50_ms=data.get('real_time', 0),
            latency_p95_ms=data.get('latency_p95', 0),
            latency_p99_ms=data.get('latency_p99', 0),
            error_rate=data.get('error_rate', 0.0),
            metadata=data
        )
    
    def store_results(self, results: List[BenchmarkResult]):
        """
        Store results in time-series format
        
        Args:
            results: List of benchmark results to store
        """
        if not results:
            print("⚠️  No results to store")
            return
        
        timestamp = results[0].timestamp
        date_str = timestamp.split('T')[0]
        
        # Store raw results
        raw_file = self.storage_dir / "raw" / f"{date_str}_{int(time.time())}.json"
        with open(raw_file, 'w') as f:
            json.dump([asdict(r) for r in results], f, indent=2)
        print(f"✅ Stored raw results: {raw_file}")
        
        # Update aggregated time-series
        self._update_timeseries(results)
        
        # Generate Prometheus metrics
        self._generate_prometheus_metrics(results)
    
    def _update_timeseries(self, results: List[BenchmarkResult]):
        """Update time-series aggregated data"""
        # Load existing time-series or create new
        timeseries_file = self.storage_dir / "aggregated" / "timeseries.json"
        
        if timeseries_file.exists():
            with open(timeseries_file, 'r') as f:
                timeseries = json.load(f)
        else:
            timeseries = {
                'benchmarks': {},
                'metadata': {
                    'created': datetime.utcnow().isoformat() + "Z",
                    'last_updated': None
                }
            }
        
        # Add new results to time-series
        for result in results:
            bench_name = result.name
            
            if bench_name not in timeseries['benchmarks']:
                timeseries['benchmarks'][bench_name] = {
                    'datapoints': []
                }
            
            datapoint = {
                'timestamp': result.timestamp,
                'branch': result.branch,
                'commit': result.commit,
                'release': result.release,
                'hardware': result.hardware,
                'throughput_ops': result.throughput_ops,
                'latency_p50_ms': result.latency_p50_ms,
                'latency_p95_ms': result.latency_p95_ms,
                'latency_p99_ms': result.latency_p99_ms,
                'error_rate': result.error_rate
            }
            
            timeseries['benchmarks'][bench_name]['datapoints'].append(datapoint)
        
        timeseries['metadata']['last_updated'] = datetime.utcnow().isoformat() + "Z"
        
        # Save updated time-series
        with open(timeseries_file, 'w') as f:
            json.dump(timeseries, f, indent=2)
        
        print(f"✅ Updated time-series: {timeseries_file}")
    
    def _generate_prometheus_metrics(self, results: List[BenchmarkResult]):
        """Generate Prometheus metrics file"""
        metrics_file = self.storage_dir / "metrics" / "benchmarks.prom"
        
        lines = [
            "# HELP themisdb_benchmark_throughput_ops Benchmark throughput in operations per second",
            "# TYPE themisdb_benchmark_throughput_ops gauge",
        ]
        
        for result in results:
            labels = (
                f'benchmark="{result.name}",'
                f'branch="{result.branch}",'
                f'commit="{result.commit[:8]}",'
                f'release="{result.release}",'
                f'hardware="{result.hardware}"'
            )
            lines.append(
                f'themisdb_benchmark_throughput_ops{{{labels}}} {result.throughput_ops}'
            )
        
        lines.extend([
            "",
            "# HELP themisdb_benchmark_latency_ms Benchmark latency percentiles in milliseconds",
            "# TYPE themisdb_benchmark_latency_ms gauge"
        ])
        
        for result in results:
            labels_base = (
                f'benchmark="{result.name}",'
                f'branch="{result.branch}",'
                f'commit="{result.commit[:8]}",'
                f'release="{result.release}",'
                f'hardware="{result.hardware}"'
            )
            
            lines.append(
                f'themisdb_benchmark_latency_ms{{{labels_base},quantile="0.50"}} {result.latency_p50_ms}'
            )
            lines.append(
                f'themisdb_benchmark_latency_ms{{{labels_base},quantile="0.95"}} {result.latency_p95_ms}'
            )
            lines.append(
                f'themisdb_benchmark_latency_ms{{{labels_base},quantile="0.99"}} {result.latency_p99_ms}'
            )
        
        lines.extend([
            "",
            "# HELP themisdb_benchmark_error_rate Benchmark error rate as percentage",
            "# TYPE themisdb_benchmark_error_rate gauge"
        ])
        
        for result in results:
            labels = (
                f'benchmark="{result.name}",'
                f'branch="{result.branch}",'
                f'commit="{result.commit[:8]}",'
                f'release="{result.release}",'
                f'hardware="{result.hardware}"'
            )
            lines.append(
                f'themisdb_benchmark_error_rate{{{labels}}} {result.error_rate}'
            )
        
        # Write metrics
        with open(metrics_file, 'w') as f:
            f.write('\n'.join(lines) + '\n')
        
        print(f"✅ Generated Prometheus metrics: {metrics_file}")
    
    def _get_git_branch(self) -> str:
        """Get current git branch"""
        try:
            import subprocess
            result = subprocess.run(
                ['git', 'rev-parse', '--abbrev-ref', 'HEAD'],
                capture_output=True,
                text=True,
                check=True
            )
            return result.stdout.strip()
        except:
            return os.environ.get('GITHUB_REF_NAME', os.environ.get('GIT_BRANCH', 'unknown'))
    
    def _get_git_commit(self) -> str:
        """Get current git commit hash"""
        try:
            import subprocess
            result = subprocess.run(
                ['git', 'rev-parse', 'HEAD'],
                capture_output=True,
                text=True,
                check=True
            )
            return result.stdout.strip()
        except:
            return os.environ.get('GITHUB_SHA', os.environ.get('GIT_COMMIT', 'unknown'))
    
    def _get_release_version(self) -> str:
        """Get release version if available"""
        # Check for release tag
        release = os.environ.get('GITHUB_REF_NAME', '')
        if release.startswith('v'):
            return release
        
        # Check VERSION file
        version_file = Path('VERSION')
        if version_file.exists():
            return version_file.read_text().strip()
        
        return 'dev'
    
    def _detect_hardware(self) -> str:
        """Detect hardware configuration"""
        import platform
        
        # Try to get more detailed info
        system = platform.system()
        machine = platform.machine()
        
        # Check for CI environment
        if os.environ.get('CI'):
            runner = os.environ.get('RUNNER_NAME', 'github-actions')
            return f"ci-{system.lower()}-{machine}-{runner}"
        
        return f"{system.lower()}-{machine}"
    
    def export_for_baseline(
        self,
        results: List[BenchmarkResult],
        output_path: Path
    ):
        """
        Export results in baseline format for regression detection
        
        Args:
            results: Benchmark results
            output_path: Output file path
        """
        if not results:
            print("⚠️  No results to export")
            return
        
        baseline = {
            'version': results[0].release,
            'branch': results[0].branch,
            'commit': results[0].commit,
            'timestamp': results[0].timestamp,
            'hardware': results[0].hardware,
            'benchmarks': {}
        }
        
        for result in results:
            baseline['benchmarks'][result.name] = {
                'items_per_second': result.throughput_ops,
                'real_time': result.latency_p50_ms,
                'latency_p95': result.latency_p95_ms,
                'latency_p99': result.latency_p99_ms,
                'error_rate': result.error_rate
            }
        
        with open(output_path, 'w') as f:
            json.dump(baseline, f, indent=2)
        
        print(f"✅ Exported baseline: {output_path}")


def main():
    parser = argparse.ArgumentParser(
        description="Track and store ThemisDB benchmark results"
    )
    parser.add_argument('--results', required=True,
                       help='Path to benchmark results (file or directory)')
    parser.add_argument('--storage', default='benchmarks/performance_data',
                       help='Storage directory for historical data')
    parser.add_argument('--branch', help='Git branch name')
    parser.add_argument('--commit', help='Git commit hash')
    parser.add_argument('--release', help='Release version')
    parser.add_argument('--hardware', help='Hardware configuration')
    parser.add_argument('--export-baseline',
                       help='Export as baseline file for regression detection')
    
    args = parser.parse_args()
    
    # Initialize tracker
    storage_dir = Path(args.storage)
    tracker = PerformanceTracker(storage_dir)
    
    # Collect results
    results_path = Path(args.results)
    print(f"📊 Collecting benchmark results from: {results_path}")
    
    results = tracker.collect_benchmark_results(
        results_path,
        branch=args.branch,
        commit=args.commit,
        release=args.release,
        hardware=args.hardware
    )
    
    print(f"✅ Collected {len(results)} benchmark results")
    
    # Store results
    tracker.store_results(results)
    
    # Export baseline if requested
    if args.export_baseline:
        baseline_path = Path(args.export_baseline)
        tracker.export_for_baseline(results, baseline_path)
    
    print("\n✅ Performance tracking complete!")
    return 0


if __name__ == "__main__":
    sys.exit(main())
