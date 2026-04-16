"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            analyze_benchmarks.py                              ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     236                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Analyze and compare Themis benchmark results with historical baselines
"""

import json
import os
import glob
from pathlib import Path
from datetime import datetime
from collections import defaultdict
import statistics

def load_json_file(filepath):
    """Load and parse a Google Benchmark JSON file"""
    try:
        with open(filepath, 'r') as f:
            return json.load(f)
    except Exception as e:
        print(f"ERROR loading {filepath}: {e}")
        return None

def extract_benchmark_metrics(data):
    """Extract key metrics from Google Benchmark JSON"""
    if not data or 'benchmarks' not in data:
        return {}
    
    metrics = {}
    for bench in data['benchmarks']:
        name = bench.get('name', 'unknown')
        metrics[name] = {
            'real_time': bench.get('real_time'),
            'cpu_time': bench.get('cpu_time'),
            'iterations': bench.get('iterations'),
            'items_per_second': bench.get('items_per_second'),
            'bytes_per_second': bench.get('bytes_per_second'),
        }
    return metrics

def compare_benchmarks(new_metrics, old_metrics):
    """Compare new vs old benchmark results and calculate delta"""
    comparison = {}
    
    for name, new_data in new_metrics.items():
        if name not in old_metrics:
            comparison[name] = {
                'status': 'NEW',
                'new': new_data,
                'old': None,
                'delta': None,
                'pct_change': None
            }
            continue
        
        old_data = old_metrics[name]
        
        # Calculate percent change for items_per_second (higher is better)
        pct_change = None
        delta = None
        status = 'UNCHANGED'
        
        if old_data.get('items_per_second') and new_data.get('items_per_second'):
            old_ips = old_data['items_per_second']
            new_ips = new_data['items_per_second']
            delta = new_ips - old_ips
            if old_ips != 0:
                pct_change = (delta / old_ips) * 100
                if pct_change > 5:
                    status = 'IMPROVED'
                elif pct_change < -5:
                    status = 'REGRESSED'
        
        comparison[name] = {
            'status': status,
            'new': new_data,
            'old': old_data,
            'delta': delta,
            'pct_change': pct_change
        }
    
    return comparison

def format_report(comparison):
    """Format comparison results into readable report"""
    report = []
    report.append("\n" + "=" * 100)
    report.append("BENCHMARK ANALYSIS REPORT")
    report.append("=" * 100)
    
    # Summary stats
    improved = sum(1 for c in comparison.values() if c['status'] == 'IMPROVED')
    regressed = sum(1 for c in comparison.values() if c['status'] == 'REGRESSED')
    new_benchmarks = sum(1 for c in comparison.values() if c['status'] == 'NEW')
    unchanged = sum(1 for c in comparison.values() if c['status'] == 'UNCHANGED')
    
    report.append(f"\n📊 SUMMARY:")
    report.append(f"  ✅ Improved:    {improved}")
    report.append(f"  ⚠️  Regressed:   {regressed}")
    report.append(f"  🆕 New:        {new_benchmarks}")
    report.append(f"  ➡️  Unchanged:  {unchanged}")
    report.append(f"  📝 Total:      {len(comparison)}")
    
    # Detailed results by status
    report.append(f"\n{'-'*100}")
    report.append("IMPROVED BENCHMARKS (Performance Gains):")
    report.append(f"{'-'*100}")
    
    improved_list = [(n, c) for n, c in comparison.items() if c['status'] == 'IMPROVED']
    improved_list.sort(key=lambda x: x[1]['pct_change'], reverse=True)
    
    if improved_list:
        for name, comp in improved_list:
            pct = comp['pct_change']
            report.append(f"  ✅ {name}")
            if comp['old'] and comp['old'].get('items_per_second'):
                report.append(f"     Old: {comp['old']['items_per_second']:,.0f} items/sec")
                report.append(f"     New: {comp['new']['items_per_second']:,.0f} items/sec")
                report.append(f"     Improvement: +{pct:.2f}%")
    else:
        report.append("  (none)")
    
    report.append(f"\n{'-'*100}")
    report.append("REGRESSED BENCHMARKS (Performance Loss):")
    report.append(f"{'-'*100}")
    
    regressed_list = [(n, c) for n, c in comparison.items() if c['status'] == 'REGRESSED']
    regressed_list.sort(key=lambda x: x[1]['pct_change'])
    
    if regressed_list:
        for name, comp in regressed_list:
            pct = comp['pct_change']
            report.append(f"  ⚠️  {name}")
            if comp['old'] and comp['old'].get('items_per_second'):
                report.append(f"     Old: {comp['old']['items_per_second']:,.0f} items/sec")
                report.append(f"     New: {comp['new']['items_per_second']:,.0f} items/sec")
                report.append(f"     Regression: {pct:.2f}%")
    else:
        report.append("  (none)")
    
    report.append(f"\n{'-'*100}")
    report.append("NEW BENCHMARKS:")
    report.append(f"{'-'*100}")
    
    new_list = [(n, c) for n, c in comparison.items() if c['status'] == 'NEW']
    if new_list:
        for name, comp in new_list:
            report.append(f"  🆕 {name}")
            if comp['new'].get('items_per_second'):
                report.append(f"     Performance: {comp['new']['items_per_second']:,.0f} items/sec")
    else:
        report.append("  (none)")
    
    report.append("\n" + "=" * 100)
    return "\n".join(report)

def main():
    # Find newest benchmark run
    benchmark_root = Path("C:/VCC/themis/benchmarks/benchmark_results")
    old_baseline = Path("C:/VCC/themis/msvc_bench_results/core_perf_windows.json")
    
    if not benchmark_root.exists():
        print("❌ No benchmark results directory found")
        return
    
    # Find newest timestamp directory
    timestamp_dirs = sorted([d for d in benchmark_root.iterdir() if d.is_dir()])
    if not timestamp_dirs:
        print("❌ No benchmark result directories found")
        return
    
    newest_dir = timestamp_dirs[-1]
    print(f"\n📂 Latest benchmark run: {newest_dir.name}")
    
    # Load all new benchmark JSON files
    new_json_files = list(newest_dir.glob("*.json"))
    print(f"📊 Found {len(new_json_files)} new benchmark JSON files")
    
    # Aggregate new metrics
    all_new_metrics = {}
    for json_file in new_json_files:
        data = load_json_file(json_file)
        if data:
            metrics = extract_benchmark_metrics(data)
            all_new_metrics.update(metrics)
            print(f"  ✓ {json_file.name}: {len(metrics)} benchmarks")
    
    print(f"\n📈 Total new benchmarks: {len(all_new_metrics)}")
    
    # Load old baseline
    if old_baseline.exists():
        old_data = load_json_file(old_baseline)
        old_metrics = extract_benchmark_metrics(old_data)
        print(f"📚 Old baseline: {len(old_metrics)} benchmarks")
        
        # Compare
        comparison = compare_benchmarks(all_new_metrics, old_metrics)
        report = format_report(comparison)
        print(report)
        
        # Save report
        report_path = newest_dir / "comparison_report.txt"
        with open(report_path, 'w', encoding='utf-8') as f:
            f.write(report)
        print(f"\n✅ Report saved to: {report_path}")
    else:
        print(f"⚠️  Old baseline not found: {old_baseline}")
        print("\nNew benchmarks (no comparison available):")
        for name in sorted(all_new_metrics.keys())[:20]:
            metrics = all_new_metrics[name]
            if metrics.get('items_per_second'):
                print(f"  • {name}: {metrics['items_per_second']:,.0f} items/sec")

if __name__ == "__main__":
    main()
