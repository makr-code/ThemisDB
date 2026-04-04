# Batch Operations Guide

Learn how to efficiently handle bulk data operations in ThemisDB. Master batching strategies, performance optimization, and error handling for high-throughput scenarios.

## 🎯 What You'll Learn

- ✅ Batch insert, update, and delete operations
- ✅ Optimal batch sizing strategies
- ✅ Transaction batching patterns
- ✅ Error handling in batch operations
- ✅ Performance optimization techniques
- ✅ Streaming large datasets

**Prerequisites:** [CRUD Tutorial](CRUD_TUTORIAL.md)  
**Time Required:** 25 minutes  
**Difficulty:** Intermediate

---

## Table of Contents

1. [Why Batch Operations?](#why-batch-operations)
2. [Batch Insert](#batch-insert)
3. [Batch Update](#batch-update)
4. [Batch Delete](#batch-delete)
5. [Optimal Batch Sizing](#optimal-batch-sizing)
6. [Error Handling](#error-handling)
7. [Performance Optimization](#performance-optimization)

---

## Why Batch Operations?

### Performance Comparison

**Individual Operations (Bad):**
```bash
# 1,000 individual inserts: ~60 seconds
for i in {1..1000}; do
  curl -X PUT http://localhost:8080/entities/item:$i \
    -d "{\"blob\": \"{\\\"value\\\": $i}\"}"
done
```

**Batch Operation (Good):**
```bash
# 1,000 batched inserts: ~0.6 seconds (100x faster!)
curl -X POST http://localhost:8080/batch/create \
  -d '{
    "entities": [/* 1000 entities */]
  }'
```

**Why So Fast?**
- Single network round-trip
- Single transaction
- Optimized internal processing
- Reduced lock contention

---

## Batch Insert

### Basic Batch Create

```bash
curl -X POST http://localhost:8080/batch/create \
  -H "Content-Type: application/json" \
  -d '{
    "entities": [
      {
        "entity_id": "products:item-001",
        "blob": "{\"name\":\"Widget A\",\"price\":10.99,\"stock\":100}"
      },
      {
        "entity_id": "products:item-002",
        "blob": "{\"name\":\"Widget B\",\"price\":15.99,\"stock\":50}"
      },
      {
        "entity_id": "products:item-003",
        "blob": "{\"name\":\"Widget C\",\"price\":12.99,\"stock\":75}"
      }
    ]
  }'
```

**Expected Output:**
```json
{
  "status": "success",
  "created": 3,
  "failed": 0,
  "results": [
    {"entity": "products:item-001", "version": 1, "status": "created"},
    {"entity": "products:item-002", "version": 1, "status": "created"},
    {"entity": "products:item-003", "version": 1, "status": "created"}
  ],
  "duration_ms": 45
}
```

### Batch Create from File

```bash
# Create JSON file with entities
cat > products.json << 'EOF'
{
  "entities": [
    {"entity_id": "products:1001", "blob": "{\"name\":\"Product 1\"}"},
    {"entity_id": "products:1002", "blob": "{\"name\":\"Product 2\"}"},
    {"entity_id": "products:1003", "blob": "{\"name\":\"Product 3\"}"}
  ]
}
EOF

# Upload batch
curl -X POST http://localhost:8080/batch/create \
  -H "Content-Type: application/json" \
  -d @products.json
```

### Generate and Batch Insert

**Python Example:**
```python
import requests
import json

def batch_insert_products(count, batch_size=100):
    base_url = "http://localhost:8080"
    
    for start in range(0, count, batch_size):
        entities = []
        for i in range(start, min(start + batch_size, count)):
            entity = {
                "entity_id": f"products:item-{i:06d}",
                "blob": json.dumps({
                    "name": f"Product {i}",
                    "price": 10.0 + (i % 100),
                    "stock": 50 + (i % 50),
                    "category": f"category-{i % 10}"
                })
            }
            entities.append(entity)
        
        response = requests.post(
            f"{base_url}/batch/create",
            json={"entities": entities}
        )
        
        if response.status_code == 200:
            result = response.json()
            print(f"Batch {start//batch_size + 1}: Created {result['created']} entities")
        else:
            print(f"Error in batch {start//batch_size + 1}: {response.text}")

# Insert 10,000 products in batches of 500
batch_insert_products(10000, batch_size=500)
```

**Expected Output:**
```
Batch 1: Created 500 entities
Batch 2: Created 500 entities
...
Batch 20: Created 500 entities
Total time: ~12 seconds (vs. 10+ minutes individually!)
```

---

## Batch Update

### Basic Batch Update

```bash
curl -X POST http://localhost:8080/batch/update \
  -H "Content-Type: application/json" \
  -d '{
    "updates": [
      {
        "entity_id": "products:item-001",
        "attributes": {"price": 11.99, "stock": 95}
      },
      {
        "entity_id": "products:item-002",
        "attributes": {"price": 16.99, "stock": 45}
      },
      {
        "entity_id": "products:item-003",
        "attributes": {"price": 13.99, "stock": 70}
      }
    ]
  }'
```

**Expected Output:**
```json
{
  "status": "success",
  "updated": 3,
  "failed": 0,
  "results": [
    {"entity": "products:item-001", "version": 2, "status": "updated"},
    {"entity": "products:item-002", "version": 2, "status": "updated"},
    {"entity": "products:item-003", "version": 2, "status": "updated"}
  ]
}
```

### Conditional Batch Update

```bash
# Update only if version matches (optimistic locking)
curl -X POST http://localhost:8080/batch/update \
  -H "Content-Type: application/json" \
  -d '{
    "updates": [
      {
        "entity_id": "products:item-001",
        "expected_version": 1,
        "attributes": {"price": 12.99}
      }
    ],
    "on_conflict": "skip"
  }'
```

**on_conflict options:**
- `skip` - Skip conflicting updates
- `abort` - Abort entire batch on first conflict
- `retry` - Retry with latest version

### Batch Partial Update

```bash
# Update only specific fields, leave others unchanged
curl -X POST http://localhost:8080/batch/patch \
  -H "Content-Type: application/json" \
  -d '{
    "patches": [
      {
        "entity_id": "products:item-001",
        "attributes": {"stock": 90}
      },
      {
        "entity_id": "products:item-002",
        "attributes": {"stock": 40}
      }
    ]
  }'
```

### Bulk Price Update Example

**Python:**
```python
import requests
import json

def bulk_price_update(category, discount_percent):
    """Apply discount to all products in a category"""
    base_url = "http://localhost:8080"
    
    # First, query products in category
    query_response = requests.post(
        f"{base_url}/query",
        json={
            "table": "products",
            "predicates": [{"column": "category", "value": category}],
            "return": "entities"
        }
    )
    
    products = query_response.json()["entities"]
    
    # Prepare batch update
    updates = []
    for product in products:
        data = json.loads(product["blob"])
        old_price = data["price"]
        new_price = old_price * (1 - discount_percent / 100)
        
        updates.append({
            "entity_id": product["entity_id"],
            "attributes": {
                "price": round(new_price, 2),
                "discount_applied": discount_percent
            }
        })
    
    # Execute batch update
    update_response = requests.post(
        f"{base_url}/batch/update",
        json={"updates": updates}
    )
    
    result = update_response.json()
    print(f"Updated {result['updated']} products with {discount_percent}% discount")

# Apply 20% discount to all electronics
bulk_price_update("electronics", 20)
```

---

## Batch Delete

### Basic Batch Delete

```bash
curl -X POST http://localhost:8080/batch/delete \
  -H "Content-Type: application/json" \
  -d '{
    "entity_ids": [
      "products:item-001",
      "products:item-002",
      "products:item-003"
    ]
  }'
```

**Expected Output:**
```json
{
  "status": "success",
  "deleted": 3,
  "failed": 0,
  "results": [
    {"entity": "products:item-001", "status": "deleted"},
    {"entity": "products:item-002", "status": "deleted"},
    {"entity": "products:item-003", "status": "deleted"}
  ]
}
```

### Conditional Batch Delete

```bash
# Delete only if version matches
curl -X POST http://localhost:8080/batch/delete \
  -H "Content-Type: application/json" \
  -d '{
    "deletes": [
      {
        "entity_id": "products:item-001",
        "expected_version": 2
      }
    ],
    "on_conflict": "skip"
  }'
```

### Delete by Query Result

**Python:**
```python
import requests

def delete_out_of_stock_products():
    """Delete all products with zero stock"""
    base_url = "http://localhost:8080"
    
    # Query products with stock = 0
    query_response = requests.post(
        f"{base_url}/query",
        json={
            "table": "products",
            "predicates": [{"column": "stock", "value": 0}],
            "return": "ids_only"
        }
    )
    
    entity_ids = query_response.json()["entity_ids"]
    
    if not entity_ids:
        print("No out-of-stock products found")
        return
    
    # Batch delete
    delete_response = requests.post(
        f"{base_url}/batch/delete",
        json={"entity_ids": entity_ids}
    )
    
    result = delete_response.json()
    print(f"Deleted {result['deleted']} out-of-stock products")

delete_out_of_stock_products()
```

---

## Optimal Batch Sizing

### Finding the Sweet Spot

Batch size affects:
- **Network latency** - Larger batches = fewer round-trips
- **Memory usage** - Larger batches = more memory
- **Transaction size** - Very large batches may timeout

### Recommended Batch Sizes

| Operation Type | Recommended Size | Max Size |
|----------------|------------------|----------|
| Create (small entities) | 500-1000 | 5000 |
| Create (large entities) | 100-500 | 1000 |
| Update | 500-1000 | 5000 |
| Delete | 1000-2000 | 10000 |

### Adaptive Batch Sizing

**Python:**
```python
import requests
import time

def adaptive_batch_insert(entities, target_time_ms=1000):
    """Automatically adjust batch size based on performance"""
    base_url = "http://localhost:8080"
    batch_size = 100  # Start with small batch
    
    i = 0
    while i < len(entities):
        batch = entities[i:i+batch_size]
        
        start = time.time()
        response = requests.post(
            f"{base_url}/batch/create",
            json={"entities": batch}
        )
        duration_ms = (time.time() - start) * 1000
        
        if response.status_code == 200:
            result = response.json()
            print(f"Batch size {batch_size}: {result['created']} entities in {duration_ms:.0f}ms")
            
            # Adjust batch size based on performance
            if duration_ms < target_time_ms * 0.5:
                batch_size = min(batch_size * 2, 5000)  # Double if too fast
            elif duration_ms > target_time_ms * 1.5:
                batch_size = max(batch_size // 2, 10)   # Halve if too slow
            
            i += len(batch)
        else:
            print(f"Error: {response.text}")
            batch_size = max(batch_size // 2, 10)  # Reduce on error
    
    print(f"Completed. Final batch size: {batch_size}")
```

---

## Error Handling

### Partial Failure Handling

```bash
curl -X POST http://localhost:8080/batch/create \
  -H "Content-Type: application/json" \
  -d '{
    "entities": [
      {"entity_id": "valid:001", "blob": "{\"data\": \"value1\"}"},
      {"entity_id": "valid:002", "blob": "{\"data\": \"value2\"}"},
      {"entity_id": "duplicate:001", "blob": "{\"data\": \"exists\"}"}
    ],
    "continue_on_error": true
  }'
```

**Response with partial failure:**
```json
{
  "status": "partial_success",
  "created": 2,
  "failed": 1,
  "results": [
    {"entity": "valid:001", "status": "created"},
    {"entity": "valid:002", "status": "created"},
    {"entity": "duplicate:001", "status": "error", "error": "Entity already exists"}
  ]
}
```

### Retry Strategy

**Python:**
```python
import requests
import time
from typing import List, Dict

def batch_create_with_retry(entities: List[Dict], max_retries=3):
    """Batch create with exponential backoff retry"""
    base_url = "http://localhost:8080"
    failed_entities = entities.copy()
    
    for attempt in range(max_retries):
        if not failed_entities:
            break
        
        response = requests.post(
            f"{base_url}/batch/create",
            json={"entities": failed_entities, "continue_on_error": true}
        )
        
        if response.status_code == 200:
            result = response.json()
            print(f"Attempt {attempt + 1}: Created {result['created']}, Failed {result['failed']}")
            
            # Collect failed entities for retry
            failed_entities = [
                entity for i, entity in enumerate(failed_entities)
                if result['results'][i]['status'] == 'error'
            ]
            
            if failed_entities and attempt < max_retries - 1:
                wait_time = 2 ** attempt  # Exponential backoff
                print(f"Retrying {len(failed_entities)} failed entities in {wait_time}s...")
                time.sleep(wait_time)
        else:
            print(f"HTTP Error: {response.status_code}")
            break
    
    return len(failed_entities) == 0

# Usage
success = batch_create_with_retry(entities)
if success:
    print("All entities created successfully!")
else:
    print(f"Failed to create {len(failed_entities)} entities after retries")
```

### Transaction Rollback

```bash
# Batch operation in transaction (all or nothing)
tx_response=$(curl -X POST http://localhost:8080/tx/begin)
tx_id=$(echo $tx_response | jq -r '.tx_id')

curl -X POST http://localhost:8080/tx/$tx_id/batch/create \
  -d '{
    "entities": [/* large batch */]
  }'

# Commit or rollback
if [ $? -eq 0 ]; then
  curl -X POST http://localhost:8080/tx/$tx_id/commit
else
  curl -X POST http://localhost:8080/tx/$tx_id/rollback
fi
```

---

## Performance Optimization

### 1. Use Compression

```bash
# Enable gzip compression for large batches
curl -X POST http://localhost:8080/batch/create \
  -H "Content-Type: application/json" \
  -H "Content-Encoding: gzip" \
  -H "Accept-Encoding: gzip" \
  --data-binary @products.json.gz
```

**Savings:** 70-90% bandwidth reduction!

### 2. Parallel Batch Processing

**Python with threading:**
```python
import requests
import concurrent.futures
from typing import List

def process_batch(batch: List, batch_num: int):
    """Process a single batch"""
    response = requests.post(
        "http://localhost:8080/batch/create",
        json={"entities": batch}
    )
    return batch_num, response.json()

def parallel_batch_insert(all_entities: List, batch_size=500, workers=4):
    """Process multiple batches in parallel"""
    batches = [
        all_entities[i:i+batch_size] 
        for i in range(0, len(all_entities), batch_size)
    ]
    
    with concurrent.futures.ThreadPoolExecutor(max_workers=workers) as executor:
        futures = [
            executor.submit(process_batch, batch, i) 
            for i, batch in enumerate(batches)
        ]
        
        for future in concurrent.futures.as_completed(futures):
            batch_num, result = future.result()
            print(f"Batch {batch_num}: {result['created']} entities created")

# Process 10,000 entities with 4 parallel workers
parallel_batch_insert(entities, batch_size=500, workers=4)
```

**Performance:** 3-4x faster on multi-core systems!

### 3. Streaming Large Datasets

```bash
# Stream large CSV to batch inserts
cat large_dataset.csv | while IFS=, read -r id name price; do
  echo "{\"entity_id\":\"products:$id\",\"blob\":\"{\\\"name\\\":\\\"$name\\\",\\\"price\\\":$price}\"}"
done | jq -s '{entities: .}' | \
  curl -X POST http://localhost:8080/batch/create \
    -H "Content-Type: application/json" \
    -d @-
```

### 4. Monitor Performance

```bash
# Check batch operation metrics
curl http://localhost:4318/metrics | grep batch_operations

# Example output:
# batch_operations_total{operation="create"} 150
# batch_operations_duration_ms{operation="create",p99="850"}
# batch_operations_size_avg{operation="create"} 512
```

---

## Best Practices

### ✅ DO:

1. **Use batch operations for bulk data**
   ```python
   # Good: Batch 1000 entities
   batch_insert(entities)
   ```

2. **Choose appropriate batch sizes**
   ```python
   # 500-1000 for most use cases
   batch_size = 500
   ```

3. **Handle partial failures gracefully**
   ```python
   response.json()["failed"] > 0  # Check for failures
   ```

4. **Use compression for large batches**
   ```bash
   -H "Content-Encoding: gzip"
   ```

5. **Monitor performance metrics**
   ```bash
   curl /metrics | grep batch_
   ```

### ❌ DON'T:

1. **Don't use individual ops for bulk data**
   ```python
   # Bad: 1000 individual requests
   for item in items:
       create_entity(item)
   ```

2. **Don't use excessive batch sizes**
   ```python
   # Bad: 100,000 entities in one batch
   batch_size = 100000
   ```

3. **Don't ignore errors**
   ```python
   # Bad: Assuming all succeeded
   batch_create(entities)
   ```

4. **Don't retry infinitely**
   ```python
   # Bad: Infinite retry loop
   while True:
       try_again()
   ```

---

## Performance Benchmarks

| Operation | Individual (ops/sec) | Batch (ops/sec) | Speedup |
|-----------|---------------------|-----------------|---------|
| Insert | 150 | 15,000 | 100x |
| Update | 200 | 18,000 | 90x |
| Delete | 300 | 25,000 | 83x |

*Benchmarked on: 8-core CPU, SSD, 1KB entities*

---

## What You've Learned ✅

- ✅ Batch operations are 50-100x faster
- ✅ Optimal batch sizes: 500-1000 for most cases
- ✅ Always handle partial failures
- ✅ Use parallel processing for massive datasets
- ✅ Monitor performance metrics

---

## Next Steps

1. **[Schema Design Tutorial](SCHEMA_DESIGN.md)** - Design for batch efficiency
2. **[Best Practices Guide](BEST_PRACTICES.md)** - Production patterns
3. **Try Examples:** [IoT Sensor Network](../../examples/09_iot_sensor_network/)

---

**Questions?** See [FAQ](../FAQ.md) or ask in [Discussions](https://github.com/makr-code/ThemisDB/discussions)
