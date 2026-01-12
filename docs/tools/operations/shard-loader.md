# shard_loader.py - Shard Data Population Tool

## Overview

`shard_loader.py` populates a sharded ThemisDB cluster with configurable test datasets. It supports both hash-based and range-based sharding strategies, parallel loading across multiple workers, and customizable data distributions to facilitate realistic performance testing.

## Use Cases

- **Performance Testing Setup:** Load test data before running benchmarks
- **Scaling Tests:** Populate shards with known data volumes
- **Data Distribution Testing:** Verify even distribution across shards
- **Capacity Planning:** Estimate storage and memory requirements
- **Development:** Create realistic test environments

## Requirements

- Python 3.8 or later
- Access to running sharded ThemisDB cluster
- Sufficient network bandwidth for parallel loading
- Optional: `requests` for HTTP API access (`pip install requests`)

## Installation

```bash
cd /path/to/ThemisDB
# No installation required - standalone script
```

## Basic Usage

### Load Default Dataset

```bash
# Load 1 million documents across 4 shards
python3 tools/shard_loader.py \
  --shards 4 \
  --dataset-size 1000000 \
  --output load_results.json
```

### Hash-Based Sharding

```bash
# Use hash sharding (default)
python3 tools/shard_loader.py \
  --shards 8 \
  --dataset-size 5000000 \
  --sharding-strategy hash \
  --workers 16
```

### Range-Based Sharding

```bash
# Use range sharding
python3 tools/shard_loader.py \
  --shards 4 \
  --dataset-size 1000000 \
  --sharding-strategy range \
  --range-key user_id \
  --range-boundaries 1000000,2000000,3000000
```

## Command-Line Options

```
usage: shard_loader.py [-h] --shards SHARDS --dataset-size SIZE
                      [--sharding-strategy {hash,range}]
                      [--workers WORKERS] [--output OUTPUT]
                      [--cluster-urls URLS] [--batch-size BATCH]
                      [--data-type TYPE]

Options:
  --shards SHARDS            Number of shards in cluster
  --dataset-size SIZE        Total documents to load
  --sharding-strategy STRAT  Sharding strategy: hash or range (default: hash)
  --workers WORKERS          Parallel worker threads (default: 8)
  --output OUTPUT            Output JSON file (default: shard_loader_results.json)
  --cluster-urls URLS        Comma-separated shard URLs
  --batch-size BATCH         Documents per batch insert (default: 1000)
  --data-type TYPE           Data type: users, orders, logs, mixed (default: mixed)
  --range-key KEY            Key field for range sharding
  --range-boundaries RANGES  Comma-separated range boundaries
```

## Configuration

Create `shard_loader_config.yaml`:

```yaml
cluster:
  shards:
    - id: shard_0
      url: http://localhost:8080
    - id: shard_1
      url: http://localhost:8081
    - id: shard_2
      url: http://localhost:8082
    - id: shard_3
      url: http://localhost:8083

loading:
  dataset_size: 10000000
  batch_size: 1000
  workers: 16
  retry_on_error: true
  max_retries: 3

sharding:
  strategy: hash  # or 'range'
  hash_function: murmur3
  # For range sharding:
  range_key: user_id
  range_boundaries: [2500000, 5000000, 7500000]

data_generation:
  data_type: mixed  # users, orders, logs, or mixed
  distributions:
    users: 0.4    # 40% users
    orders: 0.35  # 35% orders
    logs: 0.25    # 25% logs
  
  users:
    id_start: 1
    name_prefix: "user_"
    age_range: [18, 85]
    fields: [id, name, email, age, created_at]
  
  orders:
    id_start: 1000000
    amount_range: [10, 5000]
    fields: [id, user_id, amount, status, timestamp]
  
  logs:
    severity_distribution:
      info: 0.70
      warning: 0.20
      error: 0.10
    fields: [timestamp, severity, message, source]
```

Use with:
```bash
python3 tools/shard_loader.py --config shard_loader_config.yaml
```

## Output Format

```json
{
  "metadata": {
    "timestamp": "2026-01-12T13:00:00Z",
    "dataset_size": 1000000,
    "shards": 4,
    "strategy": "hash",
    "workers": 8
  },
  "results": {
    "total_documents": 1000000,
    "total_time_sec": 125.5,
    "throughput_docs_sec": 7968.0,
    "batches_sent": 1000,
    "batches_failed": 0,
    "by_shard": {
      "shard_0": {
        "documents": 250234,
        "percentage": 25.02,
        "time_sec": 124.8,
        "throughput": 2005.0
      },
      "shard_1": {
        "documents": 249876,
        "percentage": 24.99,
        "time_sec": 125.2,
        "throughput": 1995.0
      },
      "shard_2": {
        "documents": 250112,
        "percentage": 25.01,
        "time_sec": 125.5,
        "throughput": 1993.0
      },
      "shard_3": {
        "documents": 249778,
        "percentage": 24.98,
        "time_sec": 125.3,
        "throughput": 1993.0
      }
    },
    "distribution_variance": 0.0012,
    "errors": []
  }
}
```

## Data Types

### Users Dataset

```python
{
  "id": 42,
  "name": "user_42",
  "email": "user_42@example.com",
  "age": 34,
  "country": "US",
  "created_at": "2024-03-15T10:30:00Z",
  "preferences": {
    "newsletter": true,
    "notifications": "email"
  }
}
```

### Orders Dataset

```python
{
  "id": 1000042,
  "user_id": 42,
  "amount": 249.99,
  "currency": "USD",
  "status": "completed",
  "items": [
    {"product_id": 101, "quantity": 2, "price": 99.99},
    {"product_id": 205, "quantity": 1, "price": 49.99}
  ],
  "timestamp": "2024-06-20T14:25:33Z"
}
```

### Logs Dataset

```python
{
  "timestamp": "2024-06-20T14:25:33.123Z",
  "severity": "info",
  "source": "api-server-3",
  "message": "Request processed successfully",
  "request_id": "req_abc123",
  "duration_ms": 45
}
```

## Advanced Usage

### Load with Custom Distribution

```bash
# Skewed distribution for hot-key testing
python3 tools/shard_loader.py \
  --shards 4 \
  --dataset-size 1000000 \
  --distribution-pattern zipf \
  --zipf-alpha 1.1
```

### Multi-Phase Loading

```bash
#!/bin/bash
# Load data in phases

# Phase 1: Base dataset
python3 tools/shard_loader.py \
  --shards 8 \
  --dataset-size 5000000 \
  --data-type users \
  --output load_phase1.json

# Phase 2: Transactions
python3 tools/shard_loader.py \
  --shards 8 \
  --dataset-size 10000000 \
  --data-type orders \
  --output load_phase2.json

# Phase 3: Logs
python3 tools/shard_loader.py \
  --shards 8 \
  --dataset-size 50000000 \
  --data-type logs \
  --output load_phase3.json
```

### Incremental Loading

```bash
# Load data incrementally for continuous testing
while true; do
  python3 tools/shard_loader.py \
    --shards 8 \
    --dataset-size 10000 \
    --append-mode \
    --output load_increment_$(date +%s).json
  
  sleep 60  # Add 10k documents every minute
done
```

## Performance Tuning

### Batch Size

| Batch Size | Use Case | Throughput |
|------------|----------|------------|
| 100 | Low latency testing | Lower |
| 1,000 | Default balanced | Medium |
| 10,000 | Bulk loading | Higher |

### Worker Threads

```bash
# Rule of thumb: 2-4 workers per shard
SHARDS=8
WORKERS=$((SHARDS * 3))

python3 tools/shard_loader.py \
  --shards $SHARDS \
  --workers $WORKERS \
  --dataset-size 10000000
```

### Network Optimization

```yaml
# In config file
loading:
  batch_size: 5000
  workers: 32
  connection_pool_size: 50
  timeout_sec: 30
  compression: true  # Enable data compression
```

## Monitoring Progress

The loader provides real-time progress updates:

```
Loading 1,000,000 documents across 4 shards...
[=====>                    ] 25% | 250,000 docs | 2,000 docs/sec | ETA: 6m 15s
```

## Troubleshooting

### Uneven Distribution

**Symptoms:** One shard receives significantly more data

**Solutions:**
- Verify hash function is working correctly
- Check range boundaries for range sharding
- Review data distribution pattern
- Inspect shard routing logic

### Low Throughput

**Symptoms:** Loading takes much longer than expected

**Solutions:**
- Increase worker count: `--workers 32`
- Increase batch size: `--batch-size 5000`
- Check network latency between client and shards
- Verify shards have sufficient resources

### Connection Timeouts

**Symptoms:** Frequent timeout errors

**Solutions:**
- Increase timeout: `--timeout 60`
- Reduce batch size: `--batch-size 500`
- Reduce worker count: `--workers 4`
- Check shard server capacity

### Memory Errors

**Symptoms:** Out of memory during loading

**Solutions:**
- Reduce batch size: `--batch-size 500`
- Reduce worker count: `--workers 4`
- Enable streaming mode: `--stream`
- Load in smaller chunks

## Validation

### Verify Data Distribution

```bash
# After loading, verify distribution
python3 tools/shard_loader.py \
  --verify-only \
  --shards 4 \
  --expected-total 1000000

# Check variance (should be < 5%)
```

### Count Documents per Shard

```bash
# Query each shard
for i in 0 1 2 3; do
  curl http://localhost:808$i/api/count
done
```

## Integration

### Pre-Benchmark Loading

```bash
#!/bin/bash
# prepare_benchmark.sh

# Load test data
python3 tools/shard_loader.py \
  --shards 8 \
  --dataset-size 10000000 \
  --output load_results.json

# Verify distribution
python3 scripts/verify_distribution.py load_results.json

# Run benchmark
python3 tools/shard_bench.py \
  --shards 8 \
  --mix B \
  --duration 300
```

### CI/CD Integration

```yaml
# .github/workflows/benchmark.yml
- name: Load Test Data
  run: |
    python3 tools/shard_loader.py \
      --shards 4 \
      --dataset-size 100000 \
      --output load_results.json

- name: Verify Distribution
  run: |
    python3 -c "
    import json
    with open('load_results.json') as f:
        data = json.load(f)
    variance = data['results']['distribution_variance']
    assert variance < 0.05, f'High variance: {variance}'
    "
```

## See Also

- [shard_bench.py](shard-bench.md) - Benchmark loaded data
- [aggregate_shard_results.py](aggregate-shard-results.md) - Analyze results
- [Sharding Guide](../../sharding/README.md) - Sharding concepts
- [SHARDING_BENCHMARKS_GUIDE.md](../../../tools/SHARDING_BENCHMARKS_GUIDE.md)

## License

Part of ThemisDB, licensed under the project's main license.
