# compare_hyperscaler.py - Cloud Performance Comparison

## Overview

Maps ThemisDB sharding performance to equivalent AWS Aurora, Google Cloud Spanner, Azure Cosmos DB, and Redshift configurations. Outputs cost-per-million-operations comparisons to aid deployment decisions.

## Use Cases

- Compare ThemisDB cost/performance to major cloud databases
- Support build-vs-buy decisions
- Generate TCO analysis for different deployment options
- Benchmark against industry-standard databases
- Create pricing models for ThemisDB offerings

## Requirements

- Python 3.8+
- Benchmark results from shard_bench.py
- Current cloud pricing data (built-in or updated)

## Basic Usage

```bash
# Compare against all hyperscalers
python3 tools/compare_hyperscaler.py \
  --benchmark-results shard_bench_results.json \
  --output hyperscaler_comparison.json

# Include specific providers
python3 tools/compare_hyperscaler.py \
  --benchmark-results shard_bench_results.json \
  --providers aurora,spanner,cosmos \
  --output comparison.json
```

## Supported Platforms

- **AWS Aurora MySQL** (r6g instances)
- **Google Cloud Spanner** (multi-region)
- **Azure Cosmos DB** (provisioned throughput)
- **AWS Redshift** (RA3 nodes)
- **Self-Hosted** (EC2, GCE, Azure VMs)

## Output

Generates comparison tables:
- Cost per million operations
- Throughput capabilities
- Latency percentiles
- Monthly operational costs
- Break-even analysis

## See Also

- [shard_bench.py](shard-bench.md)
- [tco-calculator](../analysis/tco-calculator.md)
