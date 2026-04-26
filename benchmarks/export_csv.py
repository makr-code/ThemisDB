"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            export_csv.py                                      ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     98                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Export Benchmark Results to CSV for Excel/Analysis
"""

import json
import csv
from pathlib import Path
from datetime import datetime

def export_to_csv():
    benchmark_root = Path("C:/VCC/themis/benchmarks/benchmark_results/20251229_184507")
    
    # Load core performance benchmarks
    core_perf_file = benchmark_root / "bench_core_performance.json"
    
    if not core_perf_file.exists():
        print(f"File not found: {core_perf_file}")
        return
    
    with open(core_perf_file, 'r', encoding='utf-8') as f:
        data = json.load(f)
    
    # Prepare CSV output
    csv_file = benchmark_root / "benchmark_summary.csv"
    
    with open(csv_file, 'w', newline='', encoding='utf-8') as csvf:
        fieldnames = ['Benchmark Name', 'Iterations', 'Real Time (ns)', 'CPU Time (ns)', 
                     'Items/Second', 'Time Unit', 'Threads']
        writer = csv.DictWriter(csvf, fieldnames=fieldnames)
        writer.writeheader()
        
        for bench in data['benchmarks']:
            writer.writerow({
                'Benchmark Name': bench.get('name', ''),
                'Iterations': bench.get('iterations', 0),
                'Real Time (ns)': f"{bench.get('real_time', 0):.2f}",
                'CPU Time (ns)': f"{bench.get('cpu_time', 0):.2f}",
                'Items/Second': f"{bench.get('items_per_second', 0):,.0f}",
                'Time Unit': bench.get('time_unit', 'ns'),
                'Threads': bench.get('threads', 1),
            })
    
    print(f"✅ CSV exported to: {csv_file}")
    
    # Show summary stats
    benchmarks = data['benchmarks']
    
    print(f"\n📊 BENCHMARK STATISTICS")
    print(f"   Total benchmarks: {len(benchmarks)}")
    
    # Find fastest and slowest
    by_ips = [b for b in benchmarks if b.get('items_per_second')]
    if by_ips:
        fastest = max(by_ips, key=lambda x: x.get('items_per_second', 0))
        slowest = min([b for b in by_ips if b.get('items_per_second', 0) > 0], 
                     key=lambda x: x.get('items_per_second', float('inf')))
        
        print(f"\n   🚀 Fastest: {fastest['name']}")
        print(f"      {fastest['items_per_second']:,.0f} items/sec")
        
        print(f"\n   🐌 Slowest: {slowest['name']}")
        print(f"      {slowest['items_per_second']:,.0f} items/sec")
    
    # Latency stats
    by_time = [b for b in benchmarks if b.get('real_time', 0) > 0]
    if by_time:
        times = [b['real_time'] for b in by_time]
        avg_time = sum(times) / len(times)
        
        print(f"\n   ⏱️  Average Latency: {avg_time:,.2f} ns")
        print(f"   Min: {min(times):,.2f} ns")
        print(f"   Max: {max(times):,.2f} ns")

if __name__ == "__main__":
    export_to_csv()
