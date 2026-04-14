"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            compare_hyperscaler.py                             ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 07:24:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     148                                            ║
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
Compare Hyperscaler: Maps Themis sharding results to Aurora/Spanner/Cosmos pricing/performance.
Outputs cost-per-million-ops comparison.
"""
import argparse
import json
import csv
from typing import Dict, List, Any

class HyperscalerComparator:
    """
    Public pricing (12/2025 snapshot):
    - Aurora MySQL 8.0, r6g.4xlarge: 16 vCPU, 128GB RAM, ~$1.536/h
    - Spanner multi-region, 6 nodes: ~$4.80/h (discounted)
    - Cosmos SQL, provisioned 50kRU: ~$3.80/h
    - Redshift RA3, 4xlarge: ~$3.26/h
    """
    HYPERSCALER_SKUS = {
        'aurora_mysql_8_16c': {
            'vcpu': 16,
            'ram_gb': 128,
            'price_per_hour_usd': 1.536,
            'throughput_est_ops_s': 80000,
            'latency_p99_ms': 1.2,
            'name': 'Aurora MySQL 8.0 r6g.4xlarge'
        },
        'spanner_6node': {
            'vcpu': 6 * 6,  # 6 nodes
            'ram_gb': 24 * 6,  # 24GB per node
            'price_per_hour_usd': 4.80,
            'throughput_est_ops_s': 120000,
            'latency_p99_ms': 1.0,
            'name': 'Google Spanner (6 nodes, multi-region)'
        },
        'cosmos_sql_50kru': {
            'vcpu': 16,  # Equivalent
            'ram_gb': 64,
            'price_per_hour_usd': 3.80,
            'throughput_est_ops_s': 50000,
            'latency_p99_ms': 1.5,
            'name': 'Azure Cosmos SQL (50k RU/s)'
        },
        'redshift_ra3_4xl': {
            'vcpu': 12,
            'ram_gb': 96,
            'price_per_hour_usd': 3.26,
            'throughput_est_ops_s': 100000,
            'latency_p99_ms': 2.0,
            'name': 'AWS Redshift RA3 4xlarge'
        }
    }
    
    def __init__(self, themis_results_file: str):
        with open(themis_results_file, 'r') as f:
            self.themis_data = json.load(f)
    
    def compute_themis_cost(self, throughput_ops_s: float, hardware_cost_per_hour: float = 5.0) -> float:
        """
        Themis cost model (self-hosted, 8-node cluster):
        - 8 nodes × 16vCPU/32GB @ ~$0.60/h on-demand (AWS c6i.4xlarge equivalent)
        - Total: 8 × $0.60 = $4.80/h baseline
        - Add storage/networking/mgmt: +$0.20/h = ~$5.00/h
        """
        cost_per_hour = hardware_cost_per_hour
        ops_per_hour = throughput_ops_s * 3600
        cost_per_million_ops = (cost_per_hour / ops_per_hour) * 1_000_000
        return cost_per_million_ops
    
    def compare(self) -> List[Dict[str, Any]]:
        """Generate cost-per-million-ops comparison."""
        results = self.themis_data.get('results', [])
        
        comparisons = []
        for result in results:
            mix = result['mix']
            throughput = result['throughput_ops_sec']
            
            themis_cost = self.compute_themis_cost(throughput)
            
            row = {
                'mix': mix,
                'shard_count': result['shard_count'],
                'themis_throughput_ops_s': int(throughput),
                'themis_cost_per_m_ops': round(themis_cost, 2),
                'themis_p99_ms': result['latency_p99_ms']
            }
            
            # Add hyperscaler comparisons
            for sku_name, sku in self.HYPERSCALER_SKUS.items():
                hs_cost = (sku['price_per_hour_usd'] / sku['throughput_est_ops_s']) * 1_000_000
                row[f'{sku_name}_cost_per_m_ops'] = round(hs_cost, 2)
                row[f'{sku_name}_cost_vs_themis_pct'] = round((hs_cost / themis_cost - 1) * 100, 1)
            
            comparisons.append(row)
        
        return comparisons
    
    def to_csv(self, comparisons: List[Dict], output_file: str):
        """Write comparisons to CSV."""
        if not comparisons:
            return
        
        keys = comparisons[0].keys()
        with open(output_file, 'w', newline='') as f:
            writer = csv.DictWriter(f, fieldnames=keys)
            writer.writeheader()
            writer.writerows(comparisons)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Compare Hyperscaler Costs')
    parser.add_argument('--results', default='sharding_summary.json', help='Themis sharding results')
    parser.add_argument('--output', default='cost_comparison.csv')
    args = parser.parse_args()
    
    comparator = HyperscalerComparator(args.results)
    comparisons = comparator.compare()
    comparator.to_csv(comparisons, args.output)
    
    print(f"Cost comparison saved to {args.output}")
    print("\nSample comparison (Mix A):")
    if comparisons:
        sample = comparisons[0]
        print(f"  Themis:       ${sample['themis_cost_per_m_ops']}/M ops")
        print(f"  Aurora:       ${sample['aurora_mysql_8_16c_cost_per_m_ops']}/M ops ({sample['aurora_mysql_8_16c_cost_vs_themis_pct']:+.1f}%)")
        print(f"  Spanner:      ${sample['spanner_6node_cost_per_m_ops']}/M ops ({sample['spanner_6node_cost_vs_themis_pct']:+.1f}%)")
