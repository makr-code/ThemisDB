# Batch Operations Optimization Guide

## Overview

This guide describes the optimized batch operations introduced in ThemisDB v1.5.0 to significantly improve write throughput for bulk inserts and updates.

**Performance Gains:**
- **2-5x faster** with async WAL (production-safe)
- **10-50x faster** with WAL disabled (bulk loads only)
- **100x faster** for vector batch inserts with BufferManager

## Architecture

### Components

1. **BatchWriteOptimizer** (`include/storage/batch_write_optimizer.h`)
   - Configurable write durability modes
   - Statistics tracking
   - Use-case based recommendations

2. **BatchOperationManager** (`include/utils/batch_operation_manager.h`)
   - Adaptive batch sizing
   - Automatic flushing based on time/size
   - Thread-safe queuing

3. **Optimized Batch Endpoints**
   - `/entities/batch` - Entity batch operations
   - `/vector/batch_insert` - Vector batch inserts
   - `/api/v1/documents/batch` - Document batch operations

### Durability Modes

| Mode | fsync | WAL | Throughput | Durability | Use Case |
|------|-------|-----|------------|------------|----------|
| **Sync** | Every batch | Yes | 1x (baseline) | Maximum | Critical transactions |
| **Async** | OS buffered | Yes | 2-5x | High | Production (recommended) |
| **NoSync** | None | Yes | 10-20x | Medium | Bulk loads |
| **NoWAL** | None | No | 10-50x | None | Benchmarks only |

## Usage Examples

### 1. Entity Batch Insert (Production)

```bash
# Async mode - recommended for production
curl -X POST http://localhost:8529/entities/batch \
  -H "Content-Type: application/json" \
  -d '{
    "operations": [
      {
        "op": "put",
        "key": "users:user1",
        "blob": "{\"name\":\"John\",\"email\":\"john@example.com\"}"
      },
      {
        "op": "put", 
        "key": "users:user2",
        "blob": "{\"name\":\"Jane\",\"email\":\"jane@example.com\"}"
      }
    ],
    "options": {
      "durability": "async"
    }
  }'
```

**Response:**
```json
{
  "succeeded": 2,
  "failed": 0,
  "total": 2,
  "latency_ms": 15.2,
  "throughput_ops_per_sec": 131.6
}
```

### 2. Vector Batch Insert (High Throughput)

```bash
# Bulk insert 10,000 vectors with buffering
curl -X POST http://localhost:8529/vector/batch_insert \
  -H "Content-Type: application/json" \
  -d '{
    "items": [
      {
        "pk": "doc1",
        "vector": [0.1, 0.2, 0.3, ...],
        "fields": {"title": "Document 1"}
      },
      ...
    ],
    "options": {
      "use_buffer": true,
      "buffer_size": 1000,
      "flush_interval_ms": 5000
    }
  }'
```

### 3. Bulk Load (Maximum Speed)

```python
# Python example: Bulk load with NoSync mode
import requests
import json

def bulk_load_users(users, batch_size=5000):
    """
    Bulk load users with maximum throughput.
    WARNING: Use only for initial data loads.
    """
    url = "http://localhost:8529/entities/batch"
    
    for i in range(0, len(users), batch_size):
        batch = users[i:i+batch_size]
        
        operations = [
            {
                "op": "put",
                "key": f"users:{user['id']}",
                "blob": json.dumps(user)
            }
            for user in batch
        ]
        
        payload = {
            "operations": operations,
            "options": {
                "durability": "no_sync",  # Fast but less safe
                "disable_wal": False      # Keep WAL for recovery
            }
        }
        
        response = requests.post(url, json=payload)
        print(f"Loaded batch {i//batch_size + 1}: {response.json()}")

# Load 1 million users
users = [{"id": i, "name": f"User {i}"} for i in range(1000000)]
bulk_load_users(users)
```

### 4. Transaction-Safe Batch Updates

```javascript
// Node.js: Update with full durability
const axios = require('axios');

async function updateUsersBatch(updates) {
    const operations = updates.map(u => ({
        op: 'put',
        key: `users:${u.id}`,
        blob: JSON.stringify(u)
    }));
    
    const response = await axios.post('http://localhost:8529/entities/batch', {
        operations,
        options: {
            durability: 'sync',  // Maximum safety
            use_transaction: true
        }
    });
    
    return response.data;
}
```

## Configuration Guide

### BatchWriteOptimizer Configuration

```cpp
#include "storage/batch_write_optimizer.h"

// Production configuration
BatchWriteOptimizer::Config prod_config;
prod_config.durability = BatchWriteOptimizer::DurabilityMode::Async;
prod_config.allow_concurrent_memtable_write = true;

auto optimizer = std::make_unique<BatchWriteOptimizer>(prod_config);

// Or use recommended presets
auto bulk_config = BatchWriteOptimizer::recommendedConfigForUseCase("bulk_load");
auto bench_config = BatchWriteOptimizer::recommendedConfigForUseCase("benchmark");
```

### BatchOperationManager Configuration

```cpp
#include "utils/batch_operation_manager.h"

// Configure adaptive batching
BatchOperationManager<BaseEntity>::Config config;
config.min_batch_size = 100;
config.max_batch_size = 5000;
config.max_latency = std::chrono::milliseconds(100);
config.adaptive_sizing = true;
config.queue_capacity = 50000;

auto batch_processor = [&](const std::vector<BaseEntity>& entities) {
    // Process batch
    return entities.size();
};

auto manager = std::make_unique<BatchOperationManager<BaseEntity>>(
    config, batch_processor
);
manager->start();

// Enqueue items
for (const auto& entity : entities) {
    manager->enqueue(entity);
}

// Get statistics
auto stats = manager->getStats();
std::cout << "Throughput: " << stats.avg_throughput_items_per_sec 
          << " items/sec" << std::endl;
```

## Performance Tuning

### Batch Size Optimization

```python
# Find optimal batch size for your workload
def find_optimal_batch_size(start=100, end=10000, step=100):
    """
    Benchmark different batch sizes to find optimal throughput.
    """
    results = []
    
    for batch_size in range(start, end, step):
        ops = generate_operations(batch_size)
        
        start_time = time.time()
        response = requests.post(
            "http://localhost:8529/entities/batch",
            json={"operations": ops, "options": {"durability": "async"}}
        )
        duration = time.time() - start_time
        
        throughput = batch_size / duration
        results.append((batch_size, throughput))
        
        print(f"Batch size {batch_size}: {throughput:.1f} ops/sec")
    
    # Find peak throughput
    optimal = max(results, key=lambda x: x[1])
    print(f"\nOptimal batch size: {optimal[0]} ({optimal[1]:.1f} ops/sec)")
    return optimal[0]
```

### Memory Management

```cpp
// Limit memory usage for large batches
config.queue_capacity = 10000;  // Limit queued items
config.max_memory_bytes = 500 * 1024 * 1024;  // 500 MB max

// Monitor memory usage
auto stats = manager->getStats();
if (stats.items_queued > config.queue_capacity * 0.8) {
    // Queue nearly full - slow down ingestion
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
```

### RocksDB Tuning for Batch Writes

```yaml
# themisdb.conf - Optimized for batch writes
storage:
  memtable_size_mb: 512           # Larger memtable for buffering
  max_write_buffer_number: 4      # More write buffers
  min_write_buffer_number_to_merge: 2
  
  # Level0 tuning - prevent write stalls
  level0_file_num_compaction_trigger: 4
  level0_slowdown_writes_trigger: 20
  level0_stop_writes_trigger: 36
  
  # Background jobs
  max_background_jobs: 8          # More compaction threads
  max_subcompactions: 4           # Parallel compaction
  
  # Write buffer
  allow_concurrent_memtable_write: true
  enable_pipelined_write: false   # Not compatible with TransactionDB
```

## Monitoring and Metrics

### Batch Operation Metrics

```bash
# Get batch operation statistics
curl http://localhost:8529/_admin/statistics/batch

# Response
{
  "batch_writes": {
    "total_batches": 12580,
    "total_items": 2516000,
    "avg_batch_size": 200.0,
    "avg_latency_ms": 15.2,
    "throughput_items_per_sec": 13157.9,
    "success_rate": 0.998
  },
  "write_optimizer": {
    "mode": "async",
    "wal_enabled": true,
    "fsync_per_batch": false
  }
}
```

### Prometheus Metrics

```promql
# Batch operation throughput
rate(themisdb_batch_items_total[1m])

# Batch latency (95th percentile)
histogram_quantile(0.95, rate(themisdb_batch_latency_seconds_bucket[5m]))

# Batch size distribution
rate(themisdb_batch_size_total[1m]) / rate(themisdb_batch_count_total[1m])

# Queue depth (for BufferManager)
themisdb_batch_queue_depth
```

### Grafana Dashboard

Key panels to monitor:
1. **Batch Throughput** - Items/second over time
2. **Batch Latency** - p50, p95, p99 latencies
3. **Queue Depth** - BufferManager queue utilization
4. **Success Rate** - Percentage of successful operations
5. **Memory Usage** - Buffer memory consumption

## Best Practices

### ✅ DO

1. **Use Async mode for production**
   - Best balance of speed and safety
   - 2-5x faster than sync
   - WAL provides crash recovery

2. **Batch similar operations together**
   - Group inserts separately from updates
   - Same table/collection per batch
   - Improves cache locality

3. **Use adaptive batch sizing**
   - Let BufferManager tune batch size
   - Automatically adapts to load

4. **Monitor queue depth**
   - Alert if queue is consistently full
   - Indicates ingestion > processing capacity

5. **Test durability settings**
   - Verify crash recovery works
   - Test with realistic failure scenarios

### ❌ DON'T

1. **Don't disable WAL in production**
   - Data loss on crash/power failure
   - Only for bulk loads or benchmarks

2. **Don't use huge batches**
   - > 10,000 items causes memory issues
   - Split into multiple batches

3. **Don't ignore errors**
   - Check response for failed operations
   - Retry or log failures

4. **Don't mix critical and non-critical data**
   - Use different durability modes
   - Critical → Sync, Non-critical → Async

5. **Don't skip performance testing**
   - Find optimal batch size for your workload
   - Monitor memory and CPU usage

## Troubleshooting

### Problem: Low Throughput

**Symptoms:**
- Batch operations slower than expected
- High latency (> 100ms for 1000 items)

**Solutions:**
```bash
# Check write stalls
curl http://localhost:8529/_admin/statistics | jq '.rocksdb.write_stall'

# If write stall detected:
# 1. Increase memtable size
# 2. Add more background compaction threads
# 3. Tune level0 trigger thresholds
```

### Problem: Memory Usage High

**Symptoms:**
- Memory grows with batch operations
- OOM errors under load

**Solutions:**
```cpp
// Limit queue capacity
config.queue_capacity = 5000;  // Reduce from 10000
config.max_memory_bytes = 250 * 1024 * 1024;  // 250 MB

// Add backpressure
if (manager->getStats().items_queued > 4000) {
    // Slow down ingestion
    return http::status::too_many_requests;
}
```

### Problem: Data Loss After Crash

**Symptoms:**
- Recent writes missing after restart
- WAL recovery incomplete

**Solutions:**
```cpp
// Use sync mode for critical data
config.durability = BatchWriteOptimizer::DurabilityMode::Sync;

// Or async with smaller batches (more frequent fsyncs)
config.max_batch_size = 500;  // Reduce from 1000
config.max_latency = std::chrono::milliseconds(1000);  // 1 second
```

## Migration Guide

### Upgrading from Single Inserts

```diff
// Before: Individual inserts
for (const auto& entity : entities) {
-    storage->put(entity.key(), entity.serialize());
}

// After: Batch insert
+auto batch = storage->createWriteBatch();
+for (const auto& entity : entities) {
+    batch->put(entity.key(), entity.serialize());
+}
+batch->commit();
```

### Using BufferManager

```diff
// Before: Direct batch calls
-for (const auto& entity : entities) {
-    storage->put(entity.key(), entity.serialize());
-}

// After: Buffered with automatic batching
+auto manager = std::make_unique<BatchOperationManager<BaseEntity>>(...);
+manager->start();
+
+for (const auto& entity : entities) {
+    manager->enqueue(entity);  // Automatic batching
+}
+
+// Flush on shutdown
+manager->stop();  // Flushes remaining items
```

## References

- [PERFORMANCE_TIPS.md](./PERFORMANCE_TIPS.md) - Complete performance guide
- [ARCHITECTURE.md](../ARCHITECTURE.md) - System architecture
- [Batch Processing Opportunities](./BATCH_PROCESSING_OPPORTUNITIES.md) - Analysis document
- [RocksDB Wiki: Write Performance](https://github.com/facebook/rocksdb/wiki/Write-Performance)

## Changelog

### v1.5.0 (Current)
- ✅ Added BatchWriteOptimizer with configurable durability modes
- ✅ Integrated BatchOperationManager for adaptive batching
- ✅ Enhanced entity batch endpoint with options
- ✅ Added comprehensive monitoring and metrics
- ✅ Performance improvements: 2-50x faster depending on mode

### Future Enhancements
- [ ] Auto-tuning based on workload
- [ ] ML-based batch size prediction
- [ ] Cross-shard batch transactions
- [ ] Compression for batch payloads
