"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            performance_regression_detector.py                 ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     465                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Performance Regression Detector for ThemisDB

Detects performance regressions by comparing current benchmark results
against baseline measurements. Supports configurable thresholds and
generates detailed reports.
"""

import json
import argparse
import sys
from pathlib import Path
from typing import Dict, List, Tuple, Any, Optional
from dataclasses import dataclass
from enum import Enum


class RegressionSeverity(Enum):
    """Severity levels for performance regressions"""
    NONE = "none"
    MINOR = "minor"           # < 10% regression
    MAJOR = "major"           # 10-20% regression
    CRITICAL = "critical"     # > 20% regression


@dataclass
class BenchmarkComparison:
    """Comparison result for a single benchmark"""
    name: str
    baseline_value: float
    current_value: float
    percent_change: float
    severity: RegressionSeverity
    metric_name: str
    
    def is_regression(self) -> bool:
        """Check if this is a regression (performance degradation)"""
        # For throughput metrics (items/bytes per second), lower is worse
        # For time metrics (real_time, cpu_time), higher is worse
        if self.metric_name in ['items_per_second', 'bytes_per_second']:
            return self.percent_change < 0  # Negative change is bad
        else:
            return self.percent_change > 0  # Positive change is bad
    
    def is_improvement(self) -> bool:
        """Check if this is a performance improvement"""
        if self.metric_name in ['items_per_second', 'bytes_per_second']:
            return self.percent_change > 0  # Positive change is good
        else:
            return self.percent_change < 0  # Negative change is good


class PerformanceRegressionDetector:
    """Detects performance regressions in benchmark results"""
    
    # Default thresholds (configurable)
    DEFAULT_THRESHOLDS = {
        'minor': 5.0,      # 5% change
        'major': 10.0,     # 10% change
        'critical': 20.0   # 20% change
    }
    
    def __init__(
        self,
        thresholds: Dict[str, float] = None,
        metrics: List[str] = None
    ):
        """
        Initialize detector
        
        Args:
            thresholds: Custom threshold values (minor, major, critical)
            metrics: List of metrics to compare (default: all available)
        """
        self.thresholds = thresholds or self.DEFAULT_THRESHOLDS
        self.metrics = metrics or [
            'items_per_second',
            'bytes_per_second',
            'real_time',
            'cpu_time'
        ]
    
    def compare_benchmarks(
        self,
        baseline: Dict[str, Any],
        current: Dict[str, Any]
    ) -> List[BenchmarkComparison]:
        """
        Compare current results against baseline
        
        Args:
            baseline: Baseline benchmark data
            current: Current benchmark data
            
        Returns:
            List of comparison results
        """
        baseline_benchmarks = baseline.get('benchmarks', {})
        current_benchmarks = current.get('benchmarks', {})
        
        comparisons = []
        
        # Compare each benchmark that exists in both baseline and current
        for bench_name in current_benchmarks.keys():
            if bench_name not in baseline_benchmarks:
                # New benchmark - skip comparison
                continue
            
            baseline_data = baseline_benchmarks[bench_name]
            current_data = current_benchmarks[bench_name]
            
            # Compare each metric
            for metric in self.metrics:
                baseline_value = baseline_data.get(metric)
                current_value = current_data.get(metric)
                
                if baseline_value is None or current_value is None:
                    continue
                
                if baseline_value == 0:
                    continue
                
                # Calculate percent change
                percent_change = ((current_value - baseline_value) / baseline_value) * 100
                
                # Determine severity
                severity = self._classify_severity(abs(percent_change))
                
                comparison = BenchmarkComparison(
                    name=bench_name,
                    baseline_value=baseline_value,
                    current_value=current_value,
                    percent_change=percent_change,
                    severity=severity,
                    metric_name=metric
                )
                
                comparisons.append(comparison)
        
        return comparisons
    
    def _classify_severity(self, abs_percent_change: float) -> RegressionSeverity:
        """Classify the severity of a change based on thresholds"""
        if abs_percent_change >= self.thresholds['critical']:
            return RegressionSeverity.CRITICAL
        elif abs_percent_change >= self.thresholds['major']:
            return RegressionSeverity.MAJOR
        elif abs_percent_change >= self.thresholds['minor']:
            return RegressionSeverity.MINOR
        else:
            return RegressionSeverity.NONE
    
    def generate_report(
        self,
        comparisons: List[BenchmarkComparison],
        baseline_info: Dict[str, Any],
        current_info: Dict[str, Any]
    ) -> str:
        """
        Generate a detailed regression report
        
        Args:
            comparisons: List of benchmark comparisons
            baseline_info: Baseline metadata
            current_info: Current metadata
            
        Returns:
            Formatted report string
        """
        # Filter regressions by severity
        critical = [c for c in comparisons 
                   if c.severity == RegressionSeverity.CRITICAL and c.is_regression()]
        major = [c for c in comparisons 
                if c.severity == RegressionSeverity.MAJOR and c.is_regression()]
        minor = [c for c in comparisons 
                if c.severity == RegressionSeverity.MINOR and c.is_regression()]
        
        # Also track improvements
        improvements = [c for c in comparisons if c.is_improvement() and c.severity != RegressionSeverity.NONE]
        
        lines = []
        lines.append("=" * 100)
        lines.append("PERFORMANCE REGRESSION DETECTION REPORT")
        lines.append("=" * 100)
        
        # Metadata
        lines.append("\n📊 COMPARISON INFO:")
        lines.append(f"  Baseline: {baseline_info.get('version', 'unknown')} @ {baseline_info.get('commit', 'unknown')[:8]}")
        lines.append(f"  Current:  {current_info.get('version', 'unknown')} @ {current_info.get('commit', 'unknown')[:8]}")
        lines.append(f"  Timestamp: {current_info.get('timestamp', 'unknown')}")
        
        # Summary
        lines.append("\n🎯 SUMMARY:")
        lines.append(f"  ❌ Critical Regressions: {len(critical)}")
        lines.append(f"  ⚠️  Major Regressions:    {len(major)}")
        lines.append(f"  ℹ️  Minor Regressions:    {len(minor)}")
        lines.append(f"  ✅ Improvements:         {len(improvements)}")
        lines.append(f"  📝 Total Compared:       {len(comparisons)}")
        
        # Thresholds
        lines.append("\n⚙️  THRESHOLDS:")
        lines.append(f"  Minor:    {self.thresholds['minor']}%")
        lines.append(f"  Major:    {self.thresholds['major']}%")
        lines.append(f"  Critical: {self.thresholds['critical']}%")
        
        # Critical regressions
        if critical:
            lines.append("\n" + "=" * 100)
            lines.append("❌ CRITICAL REGRESSIONS (> 20%)")
            lines.append("=" * 100)
            for comp in sorted(critical, key=lambda x: abs(x.percent_change), reverse=True):
                self._format_comparison(comp, lines)
        
        # Major regressions
        if major:
            lines.append("\n" + "=" * 100)
            lines.append("⚠️  MAJOR REGRESSIONS (10-20%)")
            lines.append("=" * 100)
            for comp in sorted(major, key=lambda x: abs(x.percent_change), reverse=True):
                self._format_comparison(comp, lines)
        
        # Minor regressions
        if minor:
            lines.append("\n" + "=" * 100)
            lines.append("ℹ️  MINOR REGRESSIONS (5-10%)")
            lines.append("=" * 100)
            for comp in sorted(minor, key=lambda x: abs(x.percent_change), reverse=True)[:10]:
                self._format_comparison(comp, lines)
            if len(minor) > 10:
                lines.append(f"\n  ... and {len(minor) - 10} more minor regressions")
        
        # Top improvements
        if improvements:
            lines.append("\n" + "=" * 100)
            lines.append("✅ TOP IMPROVEMENTS")
            lines.append("=" * 100)
            for comp in sorted(improvements, key=lambda x: abs(x.percent_change), reverse=True)[:10]:
                self._format_comparison(comp, lines)
        
        lines.append("\n" + "=" * 100)
        
        # Final verdict
        lines.append("\n🏁 VERDICT:")
        if critical or major:
            lines.append("  ❌ FAILED - Significant performance regressions detected")
            lines.append(f"     {len(critical)} critical and {len(major)} major regressions found")
        elif minor:
            lines.append("  ⚠️  WARNING - Minor performance regressions detected")
            lines.append(f"     {len(minor)} minor regressions found")
        else:
            lines.append("  ✅ PASSED - No significant regressions detected")
        
        lines.append("=" * 100)
        
        return "\n".join(lines)
    
    def _format_comparison(self, comp: BenchmarkComparison, lines: List[str]):
        """Format a single comparison for the report"""
        sign = "+" if comp.percent_change > 0 else ""
        lines.append(f"\n  • {comp.name}")
        lines.append(f"    Metric: {comp.metric_name}")
        lines.append(f"    Baseline: {comp.baseline_value:,.2f}")
        lines.append(f"    Current:  {comp.current_value:,.2f}")
        lines.append(f"    Change:   {sign}{comp.percent_change:.2f}%")
    
    def has_blocking_regressions(
        self,
        comparisons: List[BenchmarkComparison],
        block_threshold: str = 'major'
    ) -> bool:
        """
        Check if there are regressions that should block a PR
        
        Args:
            comparisons: List of comparisons
            block_threshold: Severity level that blocks ('major' or 'critical')
            
        Returns:
            True if blocking regressions exist
        """
        for comp in comparisons:
            if not comp.is_regression():
                continue
            
            if block_threshold == 'critical':
                if comp.severity == RegressionSeverity.CRITICAL:
                    return True
            else:  # 'major' (default)
                if comp.severity in [RegressionSeverity.MAJOR, RegressionSeverity.CRITICAL]:
                    return True
        
        return False
    
    def save_report(
        self,
        report: str,
        output_path: Path,
        comparisons: List[BenchmarkComparison] = None
    ):
        """
        Save report to file and optionally JSON data
        
        Args:
            report: Formatted report string
            output_path: Path to save report
            comparisons: Optional comparison data for JSON export
        """
        # Save text report
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(report)
        print(f"✅ Report saved: {output_path}")
        
        # Also save JSON data
        if comparisons:
            json_path = output_path.with_suffix('.json')
            json_data = {
                'summary': {
                    'critical': len([c for c in comparisons if c.severity == RegressionSeverity.CRITICAL and c.is_regression()]),
                    'major': len([c for c in comparisons if c.severity == RegressionSeverity.MAJOR and c.is_regression()]),
                    'minor': len([c for c in comparisons if c.severity == RegressionSeverity.MINOR and c.is_regression()]),
                    'improvements': len([c for c in comparisons if c.is_improvement()])
                },
                'comparisons': [
                    {
                        'name': c.name,
                        'metric': c.metric_name,
                        'baseline': c.baseline_value,
                        'current': c.current_value,
                        'change_pct': c.percent_change,
                        'severity': c.severity.value,
                        'is_regression': c.is_regression()
                    }
                    for c in comparisons
                ]
            }
            with open(json_path, 'w') as f:
                json.dump(json_data, f, indent=2)
            print(f"✅ JSON data saved: {json_path}")


def main():
    parser = argparse.ArgumentParser(
        description="Detect performance regressions in ThemisDB benchmarks"
    )
    parser.add_argument('--baseline', required=True,
                       help='Path to baseline JSON file')
    parser.add_argument('--current', required=True,
                       help='Path to current benchmark results (file or directory)')
    parser.add_argument('--output', default='regression_report.txt',
                       help='Output report path')
    parser.add_argument('--fail-on', default='major',
                       choices=['minor', 'major', 'critical'],
                       help='Exit with error if regressions at or above this level')
    parser.add_argument('--threshold-minor', type=float, default=5.0,
                       help='Threshold for minor regressions (%%)')
    parser.add_argument('--threshold-major', type=float, default=10.0,
                       help='Threshold for major regressions (%%)')
    parser.add_argument('--threshold-critical', type=float, default=20.0,
                       help='Threshold for critical regressions (%%)')
    
    args = parser.parse_args()
    
    # Load baseline
    with open(args.baseline, 'r') as f:
        baseline = json.load(f)
    print(f"✅ Loaded baseline: {args.baseline}")
    
    # Load current results
    current_path = Path(args.current)
    if current_path.is_file():
        with open(current_path, 'r') as f:
            current_data = json.load(f)
        
        # If it's a raw Google Benchmark JSON, wrap it
        if 'benchmarks' in current_data and 'version' not in current_data:
            current = {
                'version': 'unknown',
                'branch': 'unknown',
                'commit': 'unknown',
                'timestamp': 'unknown',
                'benchmarks': {}
            }
            for bench in current_data['benchmarks']:
                name = bench.get('name', 'unknown')
                current['benchmarks'][name] = {
                    'real_time': bench.get('real_time'),
                    'cpu_time': bench.get('cpu_time'),
                    'iterations': bench.get('iterations'),
                    'items_per_second': bench.get('items_per_second'),
                    'bytes_per_second': bench.get('bytes_per_second'),
                }
        else:
            current = current_data
    else:
        print(f"❌ Current results file not found: {args.current}")
        return 1
    
    print(f"✅ Loaded current results: {args.current}")
    
    # Create detector with custom thresholds
    thresholds = {
        'minor': args.threshold_minor,
        'major': args.threshold_major,
        'critical': args.threshold_critical
    }
    detector = PerformanceRegressionDetector(thresholds=thresholds)
    
    # Compare
    comparisons = detector.compare_benchmarks(baseline, current)
    print(f"✅ Compared {len(comparisons)} benchmark metrics")
    
    # Generate report
    report = detector.generate_report(comparisons, baseline, current)
    print(report)
    
    # Save report
    output_path = Path(args.output)
    detector.save_report(report, output_path, comparisons)
    
    # Check if we should fail
    should_fail = False
    if args.fail_on == 'minor':
        should_fail = any(c.is_regression() and c.severity != RegressionSeverity.NONE 
                         for c in comparisons)
    elif args.fail_on == 'major':
        should_fail = any(c.is_regression() and 
                         c.severity in [RegressionSeverity.MAJOR, RegressionSeverity.CRITICAL]
                         for c in comparisons)
    else:  # critical
        should_fail = any(c.is_regression() and c.severity == RegressionSeverity.CRITICAL 
                         for c in comparisons)
    
    if should_fail:
        print(f"\n❌ FAILED: Regressions detected at '{args.fail_on}' level or above")
        return 1
    else:
        print(f"\n✅ PASSED: No blocking regressions detected")
        return 0


if __name__ == "__main__":
    sys.exit(main())
