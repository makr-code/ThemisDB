"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            summary_analysis.py                                ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:45                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     142                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Comprehensive Benchmark Analysis and Summary
"""

import json
import os
from pathlib import Path
from collections import defaultdict

def load_json_safe(filepath):
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            return json.load(f)
    except:
        return None

def analyze_benchmarks():
    benchmark_root = Path("C:/VCC/themis/benchmarks/benchmark_results")
    
    # All benchmark runs
    all_runs = sorted([d for d in benchmark_root.iterdir() if d.is_dir()])
    
    print("\n" + "="*80)
    print("THEMIS BENCHMARK ANALYSIS - COMPLETE OVERVIEW")
    print("="*80)
    
    # Summary by run
    print(f"\n📊 BENCHMARK RUNS: {len(all_runs)}\n")
    
    total_benchmarks = 0
    for run_dir in all_runs[-5:]:  # Last 5 runs
        print(f"📁 {run_dir.name}")
        
        json_files = list(run_dir.glob("*.json"))
        txt_files = list(run_dir.glob("*.txt"))
        csv_files = list(run_dir.glob("*.csv"))
        
        run_benchmarks = 0
        for jf in json_files:
            data = load_json_safe(jf)
            if data and 'benchmarks' in data:
                count = len(data['benchmarks'])
                run_benchmarks += count
                print(f"  • {jf.name}: {count} benchmarks")
        
        if csv_files:
            print(f"  • CSV exports: {len(csv_files)} files")
        if txt_files:
            for tf in txt_files:
                print(f"  • {tf.name}")
        
        total_benchmarks += run_benchmarks
        print()
    
    print(f"💾 Total benchmarks measured: {total_benchmarks}\n")
    
    # Analyze latest complete benchmark
    latest_dir = all_runs[-1]
    print("="*80)
    print(f"DETAILED ANALYSIS: {latest_dir.name}")
    print("="*80)
    
    json_files = list(latest_dir.glob("*.json"))
    if json_files:
        all_benchmarks = {}
        for jf in json_files:
            data = load_json_safe(jf)
            if data and 'benchmarks' in data:
                print(f"\n📈 {jf.name}")
                print(f"   Hardware Context:")
                if 'context' in data:
                    ctx = data['context']
                    if 'cpu_info' in ctx:
                        print(f"   CPU: {ctx['cpu_info']}")
                
                # Top 5 benchmarks by items/sec
                benches = data['benchmarks']
                benches_with_ips = [b for b in benches if 'items_per_second' in b and b['items_per_second'] > 0]
                
                if benches_with_ips:
                    benches_with_ips.sort(key=lambda x: x['items_per_second'], reverse=True)
                    print(f"\n   Top 5 Performance:")
                    for i, b in enumerate(benches_with_ips[:5], 1):
                        name = b.get('name', 'unknown')[:50]
                        ips = b.get('items_per_second', 0)
                        print(f"   {i}. {name}")
                        print(f"      {ips:,.0f} items/sec")
                
                # Analyze latency
                benches_with_time = [b for b in benches if 'real_time' in b and b['real_time'] > 0]
                if benches_with_time:
                    times = [b['real_time'] for b in benches_with_time]
                    avg_time = sum(times) / len(times) if times else 0
                    print(f"\n   Latency Analysis:")
                    print(f"   Average time: {avg_time:.2f} ns")
                    print(f"   Min: {min(times):.2f} ns")
                    print(f"   Max: {max(times):.2f} ns")
    
    # Show comparison report if exists
    for run_dir in all_runs[-3:]:
        report_file = run_dir / "comparison_report.txt"
        if report_file.exists():
            print(f"\n{'='*80}")
            print(f"COMPARISON REPORT: {run_dir.name}")
            print(f"{'='*80}")
            with open(report_file, 'r', encoding='utf-8') as f:
                content = f.read()
                # Print first 50 lines
                lines = content.split('\n')
                for line in lines[:50]:
                    print(line)
            break
    
    print("\n" + "="*80)
    print("✅ Analysis complete")
    print("="*80 + "\n")

if __name__ == "__main__":
    analyze_benchmarks()
