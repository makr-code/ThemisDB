"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            metrics_exporter.py                                ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:42                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     277                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Performance Metrics Exporter

Exports benchmark results and regression data to Prometheus format
for monitoring and alerting via Grafana dashboards.
"""

import json
import argparse
from pathlib import Path
from typing import Dict, Any, List
from datetime import datetime


class PrometheusExporter:
    """Exports benchmark metrics to Prometheus format"""
    
    def __init__(self):
        self.metrics = []
    
    def add_metric(
        self,
        name: str,
        value: float,
        labels: Dict[str, str] = None,
        help_text: str = None,
        metric_type: str = "gauge"
    ):
        """
        Add a metric to export
        
        Args:
            name: Metric name (e.g., 'themisdb_benchmark_items_per_second')
            value: Metric value
            labels: Dictionary of label key-value pairs
            help_text: Help text for the metric
            metric_type: Type (gauge, counter, histogram, summary)
        """
        if help_text and not any(m['name'] == name for m in self.metrics if 'help' in m):
            self.metrics.append({
                'type': 'help',
                'name': name,
                'help': help_text
            })
            self.metrics.append({
                'type': 'type',
                'name': name,
                'metric_type': metric_type
            })
        
        self.metrics.append({
            'type': 'value',
            'name': name,
            'value': value,
            'labels': labels or {}
        })
    
    def export_baseline(self, baseline: Dict[str, Any]):
        """Export baseline benchmark metrics"""
        version = baseline.get('version', 'unknown')
        branch = baseline.get('branch', 'unknown')
        commit = baseline.get('commit', 'unknown')[:8]
        
        benchmarks = baseline.get('benchmarks', {})
        
        for bench_name, metrics in benchmarks.items():
            labels = {
                'benchmark_name': bench_name,
                'version': version,
                'branch': branch,
                'commit': commit,
                'type': 'baseline'
            }
            
            # Export each metric type
            if metrics.get('items_per_second'):
                self.add_metric(
                    'themisdb_benchmark_items_per_second',
                    metrics['items_per_second'],
                    labels,
                    'Benchmark throughput in items per second'
                )
            
            if metrics.get('bytes_per_second'):
                self.add_metric(
                    'themisdb_benchmark_bytes_per_second',
                    metrics['bytes_per_second'],
                    labels,
                    'Benchmark throughput in bytes per second'
                )
            
            if metrics.get('cpu_time'):
                self.add_metric(
                    'themisdb_benchmark_cpu_time_ms',
                    metrics['cpu_time'],
                    labels,
                    'Benchmark CPU time in milliseconds'
                )
            
            if metrics.get('real_time'):
                self.add_metric(
                    'themisdb_benchmark_real_time_ms',
                    metrics['real_time'],
                    labels,
                    'Benchmark wall clock time in milliseconds'
                )
    
    def export_regression_report(self, report_path: Path):
        """Export regression detection results"""
        if not report_path.exists():
            return
        
        with open(report_path, 'r') as f:
            data = json.load(f)
        
        summary = data.get('summary', {})
        
        # Export summary metrics
        self.add_metric(
            'themisdb_regression_count',
            summary.get('critical', 0),
            {'severity': 'critical'},
            'Number of critical performance regressions detected'
        )
        
        self.add_metric(
            'themisdb_regression_count',
            summary.get('major', 0),
            {'severity': 'major'},
            'Number of major performance regressions detected'
        )
        
        self.add_metric(
            'themisdb_regression_count',
            summary.get('minor', 0),
            {'severity': 'minor'},
            'Number of minor performance regressions detected'
        )
        
        self.add_metric(
            'themisdb_improvement_count',
            summary.get('improvements', 0),
            {},
            'Number of performance improvements detected'
        )
        
        # Export individual regressions
        comparisons = data.get('comparisons', [])
        for comp in comparisons:
            if comp.get('is_regression'):
                labels = {
                    'benchmark_name': comp['name'],
                    'metric': comp['metric'],
                    'severity': comp['severity']
                }
                
                self.add_metric(
                    'themisdb_benchmark_regression_pct',
                    comp['change_pct'],
                    labels,
                    'Performance regression percentage'
                )
    
    def export_to_file(self, output_path: Path):
        """Write metrics to file in Prometheus format"""
        lines = []
        
        for metric in self.metrics:
            if metric['type'] == 'help':
                lines.append(f"# HELP {metric['name']} {metric['help']}")
            elif metric['type'] == 'type':
                lines.append(f"# TYPE {metric['name']} {metric['metric_type']}")
            elif metric['type'] == 'value':
                label_str = ''
                if metric['labels']:
                    label_parts = [f'{k}="{v}"' for k, v in metric['labels'].items()]
                    label_str = '{' + ','.join(label_parts) + '}'
                
                lines.append(f"{metric['name']}{label_str} {metric['value']}")
        
        with open(output_path, 'w') as f:
            f.write('\n'.join(lines))
            f.write('\n')
        
        print(f"✅ Metrics exported to: {output_path}")
    
    def export_to_pushgateway(
        self,
        pushgateway_url: str,
        job_name: str = 'themisdb_benchmarks'
    ):
        """Push metrics to Prometheus Pushgateway"""
        import requests
        
        lines = []
        for metric in self.metrics:
            if metric['type'] == 'value':
                label_str = ''
                if metric['labels']:
                    label_parts = [f'{k}="{v}"' for k, v in metric['labels'].items()]
                    label_str = '{' + ','.join(label_parts) + '}'
                lines.append(f"{metric['name']}{label_str} {metric['value']}")
        
        metrics_data = '\n'.join(lines)
        
        url = f"{pushgateway_url}/metrics/job/{job_name}"
        response = requests.post(url, data=metrics_data)
        
        if response.status_code == 200:
            print(f"✅ Metrics pushed to Pushgateway: {pushgateway_url}")
        else:
            print(f"❌ Failed to push metrics: {response.status_code} {response.text}")


def main():
    parser = argparse.ArgumentParser(
        description="Export ThemisDB benchmark metrics to Prometheus format"
    )
    parser.add_argument('--baseline', help='Path to baseline JSON')
    parser.add_argument('--regression-report', help='Path to regression report JSON')
    parser.add_argument('--output', default='benchmark_metrics.prom',
                       help='Output file for Prometheus metrics')
    parser.add_argument('--pushgateway', help='Prometheus Pushgateway URL')
    parser.add_argument('--job-name', default='themisdb_benchmarks',
                       help='Job name for Pushgateway')
    
    args = parser.parse_args()
    
    exporter = PrometheusExporter()
    
    # Export baseline if provided
    if args.baseline:
        with open(args.baseline, 'r') as f:
            baseline = json.load(f)
        exporter.export_baseline(baseline)
        print(f"✅ Exported baseline metrics from: {args.baseline}")
    
    # Export regression report if provided
    if args.regression_report:
        report_path = Path(args.regression_report)
        exporter.export_regression_report(report_path)
        print(f"✅ Exported regression metrics from: {args.regression_report}")
    
    # Write to file
    output_path = Path(args.output)
    exporter.export_to_file(output_path)
    
    # Push to Pushgateway if specified
    if args.pushgateway:
        exporter.export_to_pushgateway(args.pushgateway, args.job_name)


if __name__ == "__main__":
    main()
