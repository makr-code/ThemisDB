"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            aggregate_shard_results.py                         ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 07:24:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     144                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Aggregate Shard Results: Combines shard_bench.py + fault_injector.py outputs into summary.
"""
import argparse
import json
import statistics
from typing import Dict, List, Any

class ShardAggregator:
    def __init__(self, shard_results_file: str, fault_results_files: List[str] = None):
        with open(shard_results_file, 'r') as f:
            self.shard_results = json.load(f)
        
        self.fault_results = []
        if fault_results_files:
            for fname in fault_results_files:
                with open(fname, 'r') as f:
                    self.fault_results.append(json.load(f))
    
    def compute_scaling_curve(self) -> Dict[str, Any]:
        """Analyze throughput scaling across shard counts."""
        results_by_shard = {}
        for result in self.shard_results.get('results', []):
            shard_count = result['shard_count']
            if shard_count not in results_by_shard:
                results_by_shard[shard_count] = []
            results_by_shard[shard_count].append(result['throughput_ops_sec'])
        
        scaling = {}
        baseline = None
        for shard_count in sorted(results_by_shard.keys()):
            avg_throughput = statistics.mean(results_by_shard[shard_count])
            if baseline is None:
                baseline = avg_throughput
            
            scaling[f'shards_{shard_count}'] = {
                'avg_throughput': avg_throughput,
                'expected_linear': baseline * shard_count,
                'efficiency_pct': (avg_throughput / (baseline * shard_count)) * 100
            }
        
        return scaling
    
    def compute_latency_stats(self) -> Dict[str, Any]:
        """Aggregate latency percentiles across mixes."""
        latency_stats = {}
        for result in self.shard_results.get('results', []):
            mix = result['mix']
            if mix not in latency_stats:
                latency_stats[mix] = {'p50': [], 'p95': [], 'p99': []}
            
            latency_stats[mix]['p50'].append(result['latency_p50_ms'])
            latency_stats[mix]['p95'].append(result['latency_p95_ms'])
            latency_stats[mix]['p99'].append(result['latency_p99_ms'])
        
        # Summarize
        for mix in latency_stats:
            latency_stats[mix] = {
                'p50_avg_ms': statistics.mean(latency_stats[mix]['p50']),
                'p95_avg_ms': statistics.mean(latency_stats[mix]['p95']),
                'p99_avg_ms': statistics.mean(latency_stats[mix]['p99'])
            }
        
        return latency_stats
    
    def compute_fault_resilience(self) -> Dict[str, Any]:
        """Analyze fault injection results."""
        resilience = {}
        for fault in self.fault_results:
            scenario = fault.get('scenario', 'unknown')
            if scenario not in resilience:
                resilience[scenario] = []
            
            throughput_impact = (fault['throughput_during_ops_sec'] / fault['throughput_before_ops_sec'] - 1) * 100
            latency_impact = (fault['latency_p99_during_ms'] / fault['latency_p99_before_ms'] - 1) * 100
            
            resilience[scenario].append({
                'throughput_impact_pct': throughput_impact,
                'latency_impact_pct': latency_impact,
                'recovery_time_sec': fault['recovery_time_sec'],
                'data_loss': fault.get('data_loss', False)
            })
        
        # Summarize per scenario
        for scenario in resilience:
            impacts = resilience[scenario]
            resilience[scenario] = {
                'avg_throughput_drop_pct': statistics.mean(r['throughput_impact_pct'] for r in impacts),
                'avg_latency_increase_pct': statistics.mean(r['latency_impact_pct'] for r in impacts),
                'avg_recovery_time_sec': statistics.mean(r['recovery_time_sec'] for r in impacts),
                'data_loss_incidents': sum(1 for r in impacts if r['data_loss'])
            }
        
        return resilience
    
    def aggregate(self) -> Dict[str, Any]:
        """Combine all analyses."""
        return {
            'config': self.shard_results.get('config', {}),
            'scaling_efficiency': self.compute_scaling_curve(),
            'latency_summary': self.compute_latency_stats(),
            'fault_resilience': self.compute_fault_resilience(),
            'timestamp': __import__('time').strftime('%Y-%m-%d %H:%M:%S')
        }

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Aggregate Shard Results')
    parser.add_argument('--input', default='shard_results.json', help='Shard benchmark results')
    parser.add_argument('--fault-input', nargs='+', help='Fault injection results (optional)')
    parser.add_argument('--output', default='sharding_summary.json')
    args = parser.parse_args()
    
    aggregator = ShardAggregator(args.input, args.fault_input)
    summary = aggregator.aggregate()
    
    with open(args.output, 'w') as f:
        json.dump(summary, f, indent=2)
    
    print(f"Aggregated results saved to {args.output}")
    print("\n=== SUMMARY ===")
    print(json.dumps(summary, indent=2)[:500] + "...")
