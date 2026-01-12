# aggregate_shard_results.py - Benchmark Results Aggregator

## Overview

Combines and analyzes results from `shard_bench.py` and `fault_injector.py` to generate comprehensive performance reports, scaling curves, and fault tolerance metrics.

## Use Cases

- Analyze scaling efficiency across shard counts
- Generate performance reports for documentation
- Compare fault tolerance scenarios
- Track performance trends over time
- Create executive summaries of benchmark results

## Requirements

- Python 3.8+
- JSON result files from shard_bench.py and/or fault_injector.py

## Basic Usage

```bash
# Aggregate shard benchmark results
python3 tools/aggregate_shard_results.py \
  --shard-results shard_bench_results.json \
  --output aggregated_report.json

# Include fault injection results
python3 tools/aggregate_shard_results.py \
  --shard-results shard_bench_results.json \
  --fault-results fault_*.json \
  --output complete_analysis.json
```

## Output

Generates comprehensive analysis including:
- **Scaling Curve:** Throughput vs. shard count
- **Efficiency Metrics:** Linear scaling coefficient
- **Latency Analysis:** p50/p95/p99 across configurations
- **Fault Tolerance:** Recovery time statistics
- **Summary Tables:** Key metrics at a glance

## See Also

- [shard_bench.py](shard-bench.md)
- [fault_injector.py](fault-injector.md)
- [SHARDING_BENCHMARKS_GUIDE.md](../../../tools/SHARDING_BENCHMARKS_GUIDE.md)
