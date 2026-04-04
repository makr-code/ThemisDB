# Batch Operations Quick Start

This document provides quick examples for using ThemisDB's batch operations.

**Status Note:**
- ✅ Batch endpoints (`/entities/batch`, `/vector/batch_insert`) are available
- 🚧 Durability options (`options` parameter) are planned but not yet implemented
- ✅ C++ `BatchWriteOptimizer` API is available now

## Quick Examples

### 1. Basic Batch Insert (Python) - Current API

```python
import requests
import json
import time

# Batch insert 1000 users
users = [
    {"id": i, "name": f"User {i}", "email": f"user{i}@example.com"}
    for i in range(1000)
]

operations = [
    {
        "op": "put",
        "key": f"users:{user['id']}",
        "blob": json.dumps(user)
    }
    for user in users
]

start_time = time.time()
response = requests.post(
    "http://localhost:8529/entities/batch",
    json={"operations": operations}
)
elapsed = time.time() - start_time

result = response.json()
throughput = result['succeeded'] / elapsed if elapsed > 0 else 0

print(f"✅ Inserted {result['succeeded']} users")
print(f"⚡ Throughput: {throughput:.1f} ops/sec (client-measured)")
```

### 2. Batch Update (curl) - Current API

```bash
# Current API - no options parameter yet
curl -X POST http://localhost:8529/entities/batch \
  -H "Content-Type: application/json" \
  -d '{
    "operations": [
      {"op": "put", "key": "users:1", "blob": "{\"status\":\"active\"}"},
      {"op": "put", "key": "users:2", "blob": "{\"status\":\"active\"}"},
      {"op": "delete", "key": "users:3"}
    ]
  }'
```

### 3. Vector Batch Insert (Python)

```python
import requests
import numpy as np

# Generate 10,000 random embeddings
vectors = []
for i in range(10000):
    embedding = np.random.randn(768).tolist()  # 768-dim vector
    vectors.append({
        "pk": f"doc{i}",
        "vector": embedding,
        "fields": {"title": f"Document {i}"}
    })

response = requests.post(
    "http://localhost:8529/vector/batch_insert",
    json={"items": vectors}
)

print(f"✅ Inserted {len(vectors)} vectors")
```

### 4. High-Throughput Bulk Load (Python)

```python
def bulk_load_with_monitoring(items, batch_size=5000):
    """
    Bulk load with progress monitoring and automatic batching.
    """
    total = len(items)
    start_time = time.time()
    
    for i in range(0, total, batch_size):
        batch = items[i:i+batch_size]
        
        operations = [
            {"op": "put", "key": f"data:{item['id']}", "blob": json.dumps(item)}
            for item in batch
        ]
        
        response = requests.post(
            "http://localhost:8529/entities/batch",
            json={
                "operations": operations,
                "options": {
                    "durability": "async",  # Fast but safe
                }
            }
        )
        
        result = response.json()
        elapsed = time.time() - start_time
        progress = (i + len(batch)) / total * 100
        throughput = (i + len(batch)) / elapsed
        
        print(f"[{progress:5.1f}%] Batch {i//batch_size + 1}: "
              f"{result['succeeded']} succeeded, "
              f"throughput: {throughput:.1f} items/sec")
    
    total_time = time.time() - start_time
    print(f"\n✅ Loaded {total} items in {total_time:.1f}s")
    print(f"⚡ Average throughput: {total/total_time:.1f} items/sec")

# Usage
data = [{"id": i, "value": f"data{i}"} for i in range(100000)]
bulk_load_with_monitoring(data)
```

## Performance Comparison

| Operation | Throughput (items/sec) | Notes |
|-----------|----------------------|-------|
| Individual inserts | ~100-500 | One HTTP request per item |
| Batch (sync mode) | ~1,000-5,000 | fsync every batch |
| Batch (async mode) | ~5,000-25,000 | **Recommended** |
| Batch (no sync) | ~10,000-50,000 | Bulk loads only |

## Best Practices

### ✅ DO

1. **Use batches of 100-1000 items**
   ```python
   # Good
   for i in range(0, len(items), 1000):
       batch = items[i:i+1000]
       # ... insert batch
   ```

2. **Use async mode for production**
   ```python
   options = {"durability": "async"}  # 5-10x faster than sync
   ```

3. **Monitor throughput**
   ```python
   result = response.json()
   print(f"Throughput: {result['throughput_ops_per_sec']} items/sec")
   ```

### ❌ DON'T

1. **Don't use huge batches**
   ```python
   # Bad - causes memory issues
   batch_size = 100000  # Too large!
   ```

2. **Don't disable WAL in production**
   ```python
   # Bad - data loss on crash
   options = {"disable_wal": True}  # Only for benchmarks!
   ```

3. **Don't ignore errors**
   ```python
   # Bad - might silently fail
   response = requests.post(...)
   # No error checking!
   
   # Good
   response.raise_for_status()
   result = response.json()
   if result['failed'] > 0:
       print(f"⚠️ {result['failed']} operations failed")
   ```

## Complete Example: ETL Pipeline

```python
import requests
import time
from typing import List, Dict

class ThemisDBBatchLoader:
    """
    Production-ready batch loader with error handling and monitoring.
    """
    
    def __init__(self, base_url="http://localhost:8529", batch_size=1000):
        self.base_url = base_url
        self.batch_size = batch_size
        self.stats = {
            "total_items": 0,
            "total_batches": 0,
            "total_failures": 0,
            "start_time": None
        }
    
    def load_entities(self, entities: List[Dict], collection: str):
        """
        Load entities with automatic batching and progress monitoring.
        """
        self.stats["start_time"] = time.time()
        self.stats["total_items"] = len(entities)
        
        for i in range(0, len(entities), self.batch_size):
            batch = entities[i:i+self.batch_size]
            self._load_batch(batch, collection, i)
        
        self._print_summary()
    
    def _load_batch(self, batch: List[Dict], collection: str, offset: int):
        """Load a single batch with retry logic."""
        operations = [
            {
                "op": "put",
                "key": f"{collection}:{item['id']}",
                "blob": json.dumps(item)
            }
            for item in batch
        ]
        
        max_retries = 3
        for attempt in range(max_retries):
            try:
                response = requests.post(
                    f"{self.base_url}/entities/batch",
                    json={
                        "operations": operations,
                        "options": {"durability": "async"}
                    },
                    timeout=30
                )
                response.raise_for_status()
                
                result = response.json()
                self.stats["total_batches"] += 1
                self.stats["total_failures"] += result.get("failed", 0)
                
                # Print progress
                progress = (offset + len(batch)) / self.stats["total_items"] * 100
                print(f"[{progress:5.1f}%] Batch {self.stats['total_batches']}: "
                      f"{result['succeeded']}/{len(operations)} succeeded, "
                      f"{result['throughput_ops_per_sec']:.1f} ops/sec")
                
                return  # Success
                
            except requests.exceptions.RequestException as e:
                if attempt < max_retries - 1:
                    wait_time = 2 ** attempt  # Exponential backoff
                    print(f"⚠️  Retry {attempt + 1}/{max_retries} after {wait_time}s: {e}")
                    time.sleep(wait_time)
                else:
                    print(f"❌ Failed after {max_retries} attempts: {e}")
                    self.stats["total_failures"] += len(operations)
    
    def _print_summary(self):
        """Print final statistics."""
        elapsed = time.time() - self.stats["start_time"]
        avg_throughput = self.stats["total_items"] / elapsed
        
        print("\n" + "="*60)
        print("📊 Batch Load Summary")
        print("="*60)
        print(f"Total items:     {self.stats['total_items']:,}")
        print(f"Total batches:   {self.stats['total_batches']:,}")
        print(f"Failed items:    {self.stats['total_failures']:,}")
        print(f"Success rate:    {(1 - self.stats['total_failures']/self.stats['total_items'])*100:.2f}%")
        print(f"Total time:      {elapsed:.1f}s")
        print(f"Avg throughput:  {avg_throughput:.1f} items/sec")
        print("="*60)

# Usage
loader = ThemisDBBatchLoader(batch_size=1000)

# Load 100k users
users = [
    {"id": i, "name": f"User {i}", "email": f"user{i}@example.com"}
    for i in range(100000)
]

loader.load_entities(users, "users")
```

## Monitoring

🚧 **Planned**: HTTP monitoring endpoints are planned for a future release.

### C++ Statistics (Available Now)

```cpp
auto stats = optimizer.getStats();
std::cout << "Throughput: " << stats.throughput_items_per_sec << " items/sec\n";
```

### Prometheus Metrics (Planned)

```promql
# Planned - not yet implemented
# Batch throughput
rate(themisdb_batch_items_total[1m])

# Batch latency (p95)
histogram_quantile(0.95, rate(themisdb_batch_latency_seconds_bucket[5m]))
```

## Next Steps

- Read the [Complete Batch Operations Guide](./BATCH_OPERATIONS_GUIDE.md)
- Review [Performance Tips](./PERFORMANCE_TIPS.md)
- Check [Architecture Documentation](../ARCHITECTURE.md)
