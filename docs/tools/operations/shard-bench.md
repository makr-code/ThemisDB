# shard_bench.py - Sharding Performance Benchmark

## Overview

`shard_bench.py` is a comprehensive benchmarking tool that runs standardized workload mixes (A-E) across a sharded ThemisDB cluster. It measures throughput (ops/sec), latency percentiles (p50/p95/p99), and cross-shard query rates to evaluate sharding performance under different scenarios.

## Use Cases

- **Performance Testing:** Evaluate cluster performance under various workload patterns
- **Capacity Planning:** Determine optimal shard count for expected workload
- **Comparison Testing:** Compare different sharding strategies (hash vs. range)
- **Regression Detection:** Track performance across releases
- **Documentation:** Generate performance data for capacity planning guides

## Requirements

- Python 3.8 or later
- Access to a running sharded ThemisDB cluster
- Optional: `requests` library for HTTP benchmarks (`pip install requests`)
- Sufficient network bandwidth between benchmark client and cluster

## Workload Mixes

The tool supports five standardized workload mixes:

| Mix | Reads | Writes | Cross-Shard | Vector Ops | Join Complexity | Typical Scenario |
|-----|-------|--------|-------------|------------|-----------------|------------------|
| **A** | 80% | 20% | 5% | 0% | Low | Read-heavy OLTP |
| **B** | 50% | 50% | 10% | 0% | Medium | Balanced OLTP |
| **C** | 70% | 30% | 20% | 0% | High | Analytics-heavy |
| **D** | 60% | 40% | 15% | 20% | Medium | AI/ML workload |
| **E** | 30% | 70% | 5% | 0% | Low | Write-heavy OLTP |

## Installation

```bash
# Clone repository
cd /path/to/ThemisDB

# No additional installation required for basic usage
# For HTTP benchmarks:
pip install requests
```

## Basic Usage

### Single Workload Mix

```bash
python3 tools/shard_bench.py \
  --shards 4 \
  --mix A \
  --duration 60 \
  --output results_mix_a.json
```

### Multiple Shard Counts

```bash
# Test scaling from 1 to 16 shards
for shards in 1 2 4 8 16; do
  python3 tools/shard_bench.py \
    --shards $shards \
    --mix B \
    --duration 60 \
    --output results_${shards}_shards.json
done
```

### All Workload Mixes

```bash
# Run all mixes on 8-shard cluster
for mix in A B C D E; do
  python3 tools/shard_bench.py \
    --shards 8 \
    --mix $mix \
    --duration 120 \
    --output results_mix_${mix}.json
done
```

## Command-Line Options

```
usage: shard_bench.py [-h] --shards SHARDS --mix {A,B,C,D,E}
                      [--duration DURATION] [--output OUTPUT]
                      [--cluster-urls URLS] [--threads THREADS]
                      [--warmup WARMUP]

Options:
  --shards SHARDS        Number of shards in cluster (1-256)
  --mix {A,B,C,D,E}      Workload mix to run
  --duration DURATION    Benchmark duration in seconds (default: 60)
  --output OUTPUT        Output JSON file (default: shard_bench_results.json)
  --cluster-urls URLS    Comma-separated shard URLs (default: localhost:8080-8083)
  --threads THREADS      Number of concurrent worker threads (default: 16)
  --warmup WARMUP        Warmup duration in seconds (default: 10)
```

## Configuration

### Cluster Configuration

Create a configuration file `shard_bench_config.yaml`:

```yaml
cluster:
  shards:
    - url: http://localhost:8080
      id: shard_0
    - url: http://localhost:8081
      id: shard_1
    - url: http://localhost:8082
      id: shard_2
    - url: http://localhost:8083
      id: shard_3

benchmark:
  duration: 120
  warmup: 15
  threads: 32
  report_interval: 5  # Progress updates every 5 seconds

workload:
  mix: B
  read_op_size_kb: 4
  write_op_size_kb: 8
  vector_dimension: 1536  # For mix D
```

Use with:
```bash
python3 tools/shard_bench.py --config shard_bench_config.yaml
```

## Output Format

The tool generates JSON output with comprehensive metrics:

```json
{
  "metadata": {
    "timestamp": "2026-01-12T10:30:00Z",
    "duration_sec": 60,
    "shards": 4,
    "mix": "B",
    "threads": 16
  },
  "results": [
    {
      "mix": "B",
      "shard_count": 4,
      "duration_sec": 60,
      "total_ops": 480000,
      "throughput_ops_sec": 8000.0,
      "latency_p50_ms": 1.2,
      "latency_p95_ms": 3.8,
      "latency_p99_ms": 8.5,
      "cross_shard_queries": 48000,
      "cross_shard_rate": 0.10,
      "error_rate": 0.0001,
      "by_operation": {
        "read": {
          "count": 240000,
          "throughput": 4000.0,
          "latency_p50_ms": 0.8
        },
        "write": {
          "count": 240000,
          "throughput": 4000.0,
          "latency_p50_ms": 1.6
        }
      }
    }
  ]
}
```

## Advanced Usage

### Scaling Analysis

Generate a complete scaling curve:

```bash
#!/bin/bash
# scaling_test.sh

for shards in 1 2 4 8 16 32; do
  echo "Testing with $shards shards..."
  python3 tools/shard_bench.py \
    --shards $shards \
    --mix B \
    --duration 120 \
    --threads $((shards * 4)) \
    --output scaling_${shards}_shards.json
  
  # Wait between runs
  sleep 30
done

# Aggregate results
python3 tools/aggregate_shard_results.py \
  --results scaling_*_shards.json \
  --output scaling_analysis.json
```

### Custom Workload

Modify the script to create custom workload mixes:

```python
# In shard_bench.py, add custom mix:

class WorkloadMix(Enum):
    F = {'reads': 95, 'writes': 5, 'joins': 'low', 
         'cross_shard': 1, 'vector': 0}  # Extreme read-heavy
```

### Long-Running Stability Test

```bash
# 24-hour stability test
python3 tools/shard_bench.py \
  --shards 8 \
  --mix B \
  --duration 86400 \
  --output stability_24h.json \
  --threads 32
```

## Performance Targets

Typical performance targets for different cluster sizes:

| Shards | Mix A | Mix B | Mix C | Mix D | Mix E |
|--------|-------|-------|-------|-------|-------|
| 1      | 10k   | 8k    | 6k    | 5k    | 7k    |
| 2      | 18k   | 14k   | 10k   | 9k    | 12k   |
| 4      | 32k   | 26k   | 18k   | 16k   | 22k   |
| 8      | 58k   | 48k   | 32k   | 28k   | 40k   |
| 16     | 100k  | 85k   | 55k   | 48k   | 70k   |

*Ops/sec on standard hardware (16-core, 64GB RAM per shard)*

## Interpreting Results

### Throughput Analysis

- **Linear Scaling:** Throughput should increase proportionally with shard count
- **Sublinear Scaling:** Indicates coordination overhead or bottlenecks
- **Throughput Plateau:** May indicate network, CPU, or I/O saturation

### Latency Analysis

- **p50 (Median):** Should remain stable as cluster scales
- **p95/p99 (Tail):** Watch for increases indicating coordination overhead
- **Cross-shard Impact:** Compare single-shard vs. cross-shard query latencies

### Error Rate

- **Target:** < 0.01% error rate under normal conditions
- **Elevated Errors:** May indicate resource exhaustion or configuration issues

## Troubleshooting

### High Latency

**Symptoms:** p99 latency > 100ms

**Solutions:**
- Reduce concurrent threads
- Check network latency between shards
- Verify shard CPU/memory utilization
- Consider range sharding instead of hash sharding

### Poor Scaling

**Symptoms:** Throughput doesn't scale linearly

**Solutions:**
- Verify data is evenly distributed across shards
- Check for hot keys or skewed access patterns
- Increase connection pool sizes
- Review cross-shard query frequency

### Connection Errors

**Symptoms:** High error rates, connection timeouts

**Solutions:**
- Verify all shard URLs are accessible
- Increase connection timeout values
- Check firewall/network policies
- Review shard server logs

### Inconsistent Results

**Symptoms:** High variance between runs

**Solutions:**
- Increase warmup period to 30+ seconds
- Run longer duration tests (5+ minutes)
- Ensure no other workloads running on cluster
- Check for background tasks (compaction, backups)

## Best Practices

1. **Warmup Period:** Always include 10-30 second warmup to stabilize caches
2. **Duration:** Run tests for at least 60 seconds for statistical significance
3. **Isolation:** Run benchmarks on dedicated cluster without production traffic
4. **Repetition:** Run each test 3-5 times and report median/average
5. **Documentation:** Record hardware specs, network topology, and configuration
6. **Baseline:** Establish baseline performance before making changes

## Integration

### CI/CD Pipeline

```yaml
# .github/workflows/performance.yml
- name: Performance Benchmark
  run: |
    python3 tools/shard_bench.py \
      --shards 4 \
      --mix B \
      --duration 60 \
      --output benchmark_results.json
    
- name: Compare to Baseline
  run: |
    python3 scripts/compare_benchmark.py \
      --current benchmark_results.json \
      --baseline baseline/benchmark.json \
      --threshold 0.95  # Alert if < 95% of baseline
```

### Automated Reporting

```bash
# Generate weekly performance report
python3 tools/shard_bench.py --mix B --duration 300 \
  --output weekly_$(date +%Y%m%d).json

# Upload to monitoring system
curl -X POST https://metrics.example.com/upload \
  -H "Content-Type: application/json" \
  -d @weekly_$(date +%Y%m%d).json
```

## See Also

- [shard_loader.py](shard-loader.md) - Populate shards with test data
- [aggregate_shard_results.py](aggregate-shard-results.md) - Analyze benchmark results
- [fault_injector.py](fault-injector.md) - Test cluster resilience
- [Sharding Guide](../../sharding/README.md) - ThemisDB sharding documentation
- [SHARDING_BENCHMARKS_GUIDE.md](../../../tools/SHARDING_BENCHMARKS_GUIDE.md) - Detailed benchmarking guide

## License

Part of ThemisDB, licensed under the project's main license.
