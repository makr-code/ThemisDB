"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            analyze_results.py                                 ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     152                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

import json
from pathlib import Path
from collections import defaultdict
from statistics import mean, median

# Lade Benchmark-Ergebnisse
results_file = Path("c:/VCC/themis/benchmarks/comparative/docker_benchmarks_results_20251210_163419/reports/benchmark_results.json")
data = json.load(results_file.open())

metrics = data['metrics']

# Grundlegende Statistiken
print(f"═══════════════════════════════════════════")
print(f"BENCHMARK RESULTS ANALYSIS")
print(f"═══════════════════════════════════════════")
print(f"Total metrics: {len(metrics)}")
print(f"Timestamp: {data['timestamp']}")
print(f"Version: {data['version']}")

# Workloads
workloads = sorted(set(m['workload'] for m in metrics))
print(f"\nWorkloads tested ({len(workloads)}):")
for w in workloads:
    count = len([m for m in metrics if m['workload'] == w])
    print(f"  - {w}: {count} tests")

# Competitors
competitors = sorted(set(m['competitor'] for m in metrics))
print(f"\nCompetitors tested ({len(competitors)}):")
for c in competitors:
    count = len([m for m in metrics if m['competitor'] == c])
    print(f"  - {c}: {count} tests")

# Protocols
protocols = sorted(set(m['protocol'] for m in metrics))
print(f"\nProtocols tested ({len(protocols)}): {protocols}")

# ThemisDB vs Competitors Vergleich
print(f"\n{'='*80}")
print("PERFORMANCE COMPARISON: ThemisDB vs Competitors")
print(f"{'='*80}")

# Gruppiere nach Workload und Test
by_workload_test = defaultdict(list)
for m in metrics:
    key = (m['workload'], m['test_name'], m['protocol'])
    by_workload_test[key].append(m)

# Für jeden Test: ThemisDB vs beste Alternative
comparisons = []
for (workload, test, protocol), test_metrics in by_workload_test.items():
    themis = [m for m in test_metrics if m['competitor'] == 'ThemisDB']
    others = [m for m in test_metrics if m['competitor'] != 'ThemisDB']
    
    if themis and others:
        themis_latency = themis[0]['latency_ms']
        themis_throughput = themis[0]['throughput']
        
        # Finde schnellsten Competitor
        best_competitor = min(others, key=lambda x: x['latency_ms'])
        
        latency_improvement = ((best_competitor['latency_ms'] - themis_latency) / best_competitor['latency_ms']) * 100
        throughput_improvement = ((themis_throughput - best_competitor['throughput']) / best_competitor['throughput']) * 100
        
        comparisons.append({
            'workload': workload,
            'test': test,
            'protocol': protocol,
            'themis_latency': themis_latency,
            'competitor': best_competitor['competitor'],
            'competitor_latency': best_competitor['latency_ms'],
            'latency_improvement': latency_improvement,
            'throughput_improvement': throughput_improvement
        })

# Sortiere nach Improvement
comparisons.sort(key=lambda x: x['latency_improvement'], reverse=True)

# Top 10 Performance Wins
print("\nTop 10 Performance Wins (Latency Improvement):")
print(f"{'Workload':<15} {'Test':<20} {'Protocol':<8} {'ThemisDB':<10} {'Competitor':<15} {'Comp.Lat':<10} {'Improvement':<12}")
print("-" * 110)
for comp in comparisons[:10]:
    print(f"{comp['workload']:<15} {comp['test']:<20} {comp['protocol']:<8} "
          f"{comp['themis_latency']:<10.2f} {comp['competitor']:<15} "
          f"{comp['competitor_latency']:<10.2f} {comp['latency_improvement']:>10.1f}%")

# Durchschnittliche Performance pro Workload
print(f"\n{'='*80}")
print("AVERAGE PERFORMANCE BY WORKLOAD")
print(f"{'='*80}")

for workload in workloads:
    themis_metrics = [m for m in metrics if m['workload'] == workload and m['competitor'] == 'ThemisDB']
    other_metrics = [m for m in metrics if m['workload'] == workload and m['competitor'] != 'ThemisDB']
    
    if themis_metrics and other_metrics:
        themis_avg_lat = mean(m['latency_ms'] for m in themis_metrics)
        other_avg_lat = mean(m['latency_ms'] for m in other_metrics)
        themis_avg_thr = mean(m['throughput'] for m in themis_metrics)
        other_avg_thr = mean(m['throughput'] for m in other_metrics)
        
        lat_imp = ((other_avg_lat - themis_avg_lat) / other_avg_lat) * 100
        thr_imp = ((themis_avg_thr - other_avg_thr) / other_avg_thr) * 100
        
        print(f"\n{workload.upper()}:")
        print(f"  ThemisDB: {themis_avg_lat:.2f}ms latency, {themis_avg_thr:.0f} ops/sec")
        print(f"  Others:   {other_avg_lat:.2f}ms latency, {other_avg_thr:.0f} ops/sec")
        print(f"  Improvement: {lat_imp:+.1f}% latency, {thr_imp:+.1f}% throughput")

# Resource Efficiency
print(f"\n{'='*80}")
print("RESOURCE EFFICIENCY")
print(f"{'='*80}")

themis_all = [m for m in metrics if m['competitor'] == 'ThemisDB']
others_all = [m for m in metrics if m['competitor'] != 'ThemisDB']

themis_avg_cpu = mean(m['cpu_percent'] for m in themis_all)
themis_avg_mem = mean(m['memory_mb'] for m in themis_all)
others_avg_cpu = mean(m['cpu_percent'] for m in others_all)
others_avg_mem = mean(m['memory_mb'] for m in others_all)

print(f"\nThemisDB:   {themis_avg_cpu:.1f}% CPU, {themis_avg_mem:.0f}MB RAM")
print(f"Competitors: {others_avg_cpu:.1f}% CPU, {others_avg_mem:.0f}MB RAM")
print(f"Difference:  {themis_avg_cpu - others_avg_cpu:+.1f}% CPU, {themis_avg_mem - others_avg_mem:+.0f}MB RAM")

print(f"\n{'='*80}")
print("Analysis complete!")
print(f"{'='*80}")
