# ThemisDB Performance Tips & Optimization Guide

Comprehensive guide to optimizing ThemisDB for maximum performance.

## Table of Contents

- [Write-Amplification Optimization](#write-amplification-optimization)
- [Query Optimization Techniques](#query-optimization-techniques)
- [Index Selection and Tuning](#index-selection-and-tuning)
- [Memory Configuration](#memory-configuration)
- [Cache Tuning](#cache-tuning)
- [Lock-Free Data Structures](#lock-free-data-structures)
- [Batch Operations](#batch-operations)
- [Connection Pooling](#connection-pooling)
- [Hardware Recommendations](#hardware-recommendations)
- [Monitoring and Profiling](#monitoring-and-profiling)
- [Benchmarking Best Practices](#benchmarking-best-practices)

---

## Write-Amplification Optimization

### Understanding Write-Amplification

Write-amplification is the ratio of data written to storage versus data written by the application. In LSM-tree databases like ThemisDB (RocksDB), data is written multiple times as it moves through compaction levels.

**Problem:**
```
Application writes 100 MB
→ Written to memtable: 100 MB
→ Flushed to L0: 100 MB  
→ Compacted L0→L1: 100 MB
→ Compacted L1→L2: 100 MB
→ Compacted L2→L3: 100 MB
Total: 500 MB written (5x write-amplification)
```

**Impact:**
- Increased disk wear (especially SSD)
- Reduced write throughput
- Higher I/O latency
- More CPU for compaction

---

### Configuration Strategy (v1.5.0+)

**Default Configuration (Optimized for Write-Heavy Workloads):**

```yaml
rocksdb:
  # Memtable configuration (write buffer)
  memtable_size_mb: 512          # Larger memtables → fewer flushes
  max_write_buffer_number: 6     # More buffers → writes continue during flush
  db_write_buffer_size_mb: 2048  # 2GB total across all column families
  
  # Async I/O for better scan/read performance
  enable_async_io: true
  async_io_readahead_size_mb: 128
  
  # Background operations
  max_background_compactions: 8
  max_background_flushes: 2
```

**Benefits:**
- **30-40% reduction** in write-amplification
- **50% fewer** L0 file flushes
- **20-30% improvement** in write throughput
- Continues writing during memtable flush

**Trade-offs:**
- **Memory usage**: up to ~2GB total across all memtables (capped by `db_write_buffer_size_mb`; theoretical 6 × 512MB per CF)
- **Recovery time**: Longer WAL replay on restart
- **Burst latency**: Larger flush operations

---

### Tuning for Different Workloads

**High Write Throughput (Data Ingestion):**

```yaml
rocksdb:
  memtable_size_mb: 1024         # Even larger memtables
  max_write_buffer_number: 8     # More parallelism
  db_write_buffer_size_mb: 4096  # 4GB total
  disable_wal_for_benchmark: false  # Keep WAL for durability
  level0_file_num_compaction_trigger: 2  # Aggressive compaction
```

**Balanced (Mixed Workload):**

```yaml
rocksdb:
  memtable_size_mb: 512          # Default (recommended)
  max_write_buffer_number: 6
  db_write_buffer_size_mb: 2048
  enable_async_io: true
```

**Low Latency (OLTP):**

```yaml
rocksdb:
  memtable_size_mb: 256          # Smaller for faster flushes
  max_write_buffer_number: 4
  db_write_buffer_size_mb: 1024
  level0_file_num_compaction_trigger: 4
```

**Memory-Constrained:**

```yaml
rocksdb:
  memtable_size_mb: 128          # Reduce memory usage
  max_write_buffer_number: 3
  db_write_buffer_size_mb: 512
  block_cache_size_mb: 512       # Smaller cache
```

---

### Monitoring Write-Amplification

**Key Metrics to Track:**

```bash
# Check write-amplification ratio
curl http://localhost:8529/_admin/statistics | jq '.rocksdb.writeAmplification'

# Monitor memtable statistics
curl http://localhost:8529/_admin/statistics | jq '.rocksdb | {
  memtable_size: .memtable_size_bytes,
  num_immutable_memtables: .num_immutable_mem_table,
  flush_count: .flush_count
}'

# Compaction statistics
curl http://localhost:8529/_admin/statistics | jq '.rocksdb | {
  compaction_count: .compaction_count,
  bytes_written: .bytes_written,
  bytes_read: .bytes_read
}'
```

**Prometheus Queries:**

```promql
# Write-amplification ratio
rate(rocksdb_bytes_written_total[5m]) / rate(rocksdb_bytes_written_by_user[5m])

# Memtable flush rate
rate(rocksdb_flush_count_total[5m])

# Compaction pressure
rate(rocksdb_compaction_bytes_written[5m])
```

---

### Best Practices

**DO:**
- ✅ Use larger memtables (512 MB+) for write-heavy workloads
- ✅ Set `db_write_buffer_size_mb` to limit total memory
- ✅ Enable async I/O for better scan performance
- ✅ Monitor write-amplification regularly
- ✅ Tune `level0_file_num_compaction_trigger` based on workload

**DON'T:**
- ❌ Set memtables too large on memory-constrained systems
- ❌ Use unlimited `db_write_buffer_size_mb` with many column families
- ❌ Disable WAL unless you can afford data loss
- ❌ Set `max_write_buffer_number` < 3 (causes write stalls)
- ❌ Ignore L0 file count (monitor and adjust triggers)

---

### Async I/O Configuration

**Benefits of Async I/O:**
- 2-5x faster sequential scans
- Better prefetching for range queries
- Reduced read latency through parallelism

**Configuration:**

```yaml
rocksdb:
  enable_async_io: true
  async_io_readahead_size_mb: 128     # Prefetch buffer size
  async_io_multiget_batch_size: 100   # Batch size for MultiGet
  async_io_num_threads: 4             # I/O thread pool
```

**When to Enable:**
- Sequential scan workloads
- Range queries
- Index scans
- Large result sets

**When to Disable:**
- Point lookups only
- Memory-constrained systems
- Random access patterns

---

## Query Optimization Techniques

### Understanding Query Execution

**Always Start with EXPLAIN:**

```aql
-- Analyze query execution plan
EXPLAIN
FOR doc IN users
  FILTER doc.status == "active"
  FILTER doc.age > 25
  RETURN doc

-- With full analysis
EXPLAIN OPTIONS {allPlans: true, optimizer: {rules: ["-all"]}}
FOR doc IN users
  FILTER doc.status == "active"
  RETURN doc
```

**Key Metrics to Watch:**
- `estimatedCost`: Lower is better
- `estimatedNrItems`: Expected result count
- `rules`: Applied optimizer rules
- `indexes`: Which indexes are used

---

### Filter Optimization

**Order Matters:**

```aql
-- BAD: Non-indexed filter first
FOR doc IN users
  FILTER doc.lastName == "Smith"      // Not indexed
  FILTER doc.status == "active"       // Indexed
  RETURN doc

-- GOOD: Indexed filter first
FOR doc IN users
  FILTER doc.status == "active"       // Indexed - Reduces dataset first
  FILTER doc.lastName == "Smith"      // Then filter remaining
  RETURN doc
```

**Use Composite Indexes:**

```aql
-- Create composite index
db._collection("users").ensureIndex({
  type: "persistent",
  fields: ["status", "age"],
  name: "idx_status_age"
});

-- Optimize multi-field queries
FOR doc IN users
  FILTER doc.status == "active"
  FILTER doc.age > 25
  SORT doc.age DESC
  RETURN doc
```

---

### Join Optimization

**Avoid N+1 Queries:**

```aql
-- BAD: N+1 query pattern
FOR user IN users
  LIMIT 100
  LET orders = (
    FOR order IN orders
      FILTER order.userId == user._key
      RETURN order
  )
  RETURN {user, orders}

-- GOOD: Batch lookup
LET userKeys = (FOR u IN users LIMIT 100 RETURN u._key)
LET ordersByUser = (
  FOR order IN orders
    FILTER order.userId IN userKeys
    COLLECT userId = order.userId INTO userOrders
    RETURN {userId, orders: userOrders}
)
FOR user IN users
  FILTER user._key IN userKeys
  LET orders = FIRST(FOR o IN ordersByUser FILTER o.userId == user._key RETURN o.orders)
  RETURN {user, orders}
```

**Use Graph Traversals:**

```aql
-- Instead of multiple joins
FOR v, e, p IN 1..3 OUTBOUND 'users/john' edges
  FILTER v.active == true
  RETURN p
```

---

### Aggregation Optimization

**Push Limits Early:**

```aql
-- BAD: Limit after aggregation
FOR doc IN logs
  COLLECT day = DATE_FORMAT(doc.timestamp, "%yyyy-%mm-%dd")
  WITH COUNT INTO num
  SORT num DESC
  LIMIT 10
  RETURN {day, num}

-- GOOD: Use sorted index
FOR doc IN logs
  SORT doc.timestamp DESC
  COLLECT day = DATE_FORMAT(doc.timestamp, "%yyyy-%mm-%dd")
  WITH COUNT INTO num
  LIMIT 10
  RETURN {day, num}
```

**Pre-aggregate When Possible:**

```javascript
// Materialize aggregations for frequently accessed data
db._query(`
  INSERT {
    _key: CONCAT(DATE_FORMAT(DATE_NOW(), "%yyyy-%mm-%dd"), "_stats"),
    date: DATE_FORMAT(DATE_NOW(), "%yyyy-%mm-%dd"),
    totalOrders: LENGTH(orders),
    totalRevenue: SUM(FOR o IN orders RETURN o.amount),
    updatedAt: DATE_NOW()
  } INTO daily_stats
  OPTIONS {overwriteMode: "replace"}
`);
```

---

### Projection Optimization

**Return Only Needed Fields:**

```aql
-- BAD: Return entire documents
FOR doc IN users
  FILTER doc.status == "active"
  RETURN doc

-- GOOD: Project specific fields
FOR doc IN users
  FILTER doc.status == "active"
  RETURN {
    id: doc._key,
    name: doc.name,
    email: doc.email
  }
```

**💡 Pro Tip:** Reducing returned data can improve network transfer by 80% or more.

---

### Subquery Optimization

**Use LET for Common Expressions:**

```aql
-- BAD: Repeated calculation
FOR doc IN users
  RETURN {
    user: doc,
    avgOrderValue: (FOR o IN orders FILTER o.userId == doc._key RETURN o.amount) / COUNT(orders),
    orderCount: COUNT(FOR o IN orders FILTER o.userId == doc._key RETURN 1)
  }

-- GOOD: Calculate once
FOR doc IN users
  LET userOrders = (FOR o IN orders FILTER o.userId == doc._key RETURN o)
  RETURN {
    user: doc,
    avgOrderValue: SUM(userOrders[*].amount) / LENGTH(userOrders),
    orderCount: LENGTH(userOrders)
  }
```

---

## Index Selection and Tuning

### Index Types

**Persistent Index (Most Common):**

```aql
-- Single field
db.users.ensureIndex({
  type: "persistent",
  fields: ["email"],
  unique: true,
  sparse: false
});

-- Composite (order matters!)
db.orders.ensureIndex({
  type: "persistent",
  fields: ["status", "createdAt"],
  name: "idx_status_date"
});

-- For range queries
db.logs.ensureIndex({
  type: "persistent", 
  fields: ["timestamp"],
  name: "idx_timestamp"
});
```

**Hash Index (Exact Matches Only):**

```aql
-- Faster for equality checks
db.users.ensureIndex({
  type: "hash",
  fields: ["userId"],
  unique: true
});
```

**Fulltext Index:**

```aql
-- For text search
db.articles.ensureIndex({
  type: "fulltext",
  fields: ["content"],
  minLength: 3
});

-- Usage
FOR doc IN FULLTEXT(articles, "content", "search terms")
  RETURN doc
```

**Geo Index:**

```aql
-- For geospatial queries
db.locations.ensureIndex({
  type: "geo",
  fields: ["latitude", "longitude"]
});

-- Usage
FOR loc IN NEAR(locations, 40.7128, -74.0060, 10000)
  RETURN loc
```

**Vector Index (HNSW/FAISS):**

```cpp
// For similarity search and RAG applications
// Use the workload-optimized configuration helpers

// OLTP Workload: High-throughput, low-latency
auto oltpConfig = AdvancedVectorIndex::getWorkloadOptimizedConfig(
    dataset_size, dimension, AdvancedVectorIndex::WorkloadType::OLTP);
// Result: nlist=512, nprobe=32, index_type=IVF_FLAT

// Analytics Workload: Large batch queries, high recall
auto analyticsConfig = AdvancedVectorIndex::getWorkloadOptimizedConfig(
    dataset_size, dimension, AdvancedVectorIndex::WorkloadType::ANALYTICS);
// Result: nlist=2048, nprobe=128, index_type=IVF_PQ

// RAG Workload: Balance speed and accuracy for LLM
auto ragConfig = AdvancedVectorIndex::getWorkloadOptimizedConfig(
    dataset_size, dimension, AdvancedVectorIndex::WorkloadType::RAG);
// Result: nlist=1024, nprobe=64, index_type=IVF_PQ

// Use the configuration with AdvancedVectorIndex
AdvancedVectorIndex index(dimension, ragConfig);
```

---

### Vector Index Workload Optimization

**HNSW Parameter Tuning for Different Workloads:**

| Workload | M | ef_construction | ef_search | Use Case |
|----------|---|-----------------|-----------|----------|
| OLTP | 8-16 | 96-192 | 16-128 | Low latency, high throughput queries |
| Analytics | 24-48 | 288-600 | 64-512 | Batch processing, maximum recall |
| RAG | 16-32 | 192-384 | 32-256 | LLM retrieval, balanced performance |
| Mixed | 16-24 | 192-384 | 32-512 | General-purpose workload |
| Batch Insert | 8-12 | 96-144 | N/A | Optimized for bulk data loading |

**Workload-Specific Configuration Examples:**

```cpp
// OLTP: E-commerce product search, real-time recommendations
auto config = HnswParameterTuner::getWorkloadOptimizedConfig(
    100000,  // dataset_size
    HnswParameterTuner::WorkloadType::OLTP
);
// Result: M=12, ef_construction=144, ef_search=32, target_latency=5ms
// Note: Actual values depend on dataset size and will be adjusted accordingly

// Analytics: Batch similarity analysis, data mining
auto config = HnswParameterTuner::getWorkloadOptimizedConfig(
    1000000,  // dataset_size
    HnswParameterTuner::WorkloadType::ANALYTICS
);
// Result: M=32, ef_construction=768, ef_search=128, target_latency=50ms
// Note: Values scale with dataset size to maintain performance characteristics

// RAG: Document retrieval for LLM context
auto config = HnswParameterTuner::getWorkloadOptimizedConfig(
    500000,  // dataset_size
    HnswParameterTuner::WorkloadType::RAG
);
// Result: M=24, ef_construction=384, ef_search=64, target_latency=15ms
// Note: Balanced configuration for high-quality retrieval with acceptable latency
```

**Performance Characteristics by Workload:**

```
OLTP Workload (100K vectors, 768-dim):
├── Query Latency (p95): 2-5ms
├── Throughput: 5,000-10,000 QPS
├── Memory Usage: 2-4 GB
├── Recall@10: 90-95%
└── Insert Rate: 10,000-20,000 vectors/sec

Analytics Workload (1M vectors, 768-dim):
├── Query Latency (p95): 20-50ms
├── Throughput: 500-2,000 QPS
├── Memory Usage: 20-40 GB
├── Recall@10: 97-99%
└── Batch Query: 1,000-5,000 queries/batch

RAG Workload (500K vectors, 1536-dim):
├── Query Latency (p95): 5-15ms
├── Throughput: 2,000-5,000 QPS
├── Memory Usage: 8-16 GB
├── Recall@10: 95-97%
└── Context Retrieval: 10-50 docs per query
```

---

### Index Selection Strategy

**Decision Matrix:**

| Query Pattern | Index Type | Example |
|---------------|------------|---------|
| `field == value` | Hash or Persistent | `status == "active"` |
| `field IN [...]` | Persistent | `category IN ["A", "B"]` |
| `field > value` | Persistent | `age > 25` |
| Range queries | Persistent | `date BETWEEN x AND y` |
| Text search | Fulltext | `FULLTEXT(doc, "search")` |
| Geo queries | Geo | `NEAR(loc, lat, lng)` |
| Multiple fields | Composite Persistent | `status + date` |

---

### Index Best Practices

**1. Analyze Query Patterns:**

```bash
# Export slow query log
curl http://localhost:8529/_api/query/slow > slow_queries.json

# Analyze common filter fields
cat slow_queries.json | jq -r '.[] | .query' | grep "FILTER" | sort | uniq -c | sort -rn
```

**2. Index Selectivity:**

```aql
-- Check index selectivity (higher is better)
FOR idx IN db._collection("users").getIndexes()
  RETURN {
    name: idx.name,
    fields: idx.fields,
    selectivity: idx.selectivityEstimate,
    unique: idx.unique
  }

-- Low selectivity warning
-- Don't index: gender (2 values), boolean flags
-- Do index: userId, email, timestamps
```

**3. Composite Index Field Order:**

```aql
-- Rule: Most selective field first, then sort fields

-- If querying: status == "active" AND age > 25 SORT age
db.users.ensureIndex({
  type: "persistent",
  fields: ["status", "age"]  // Filter field, then sort field
});

-- For multiple equality filters: most selective first
db.orders.ensureIndex({
  type: "persistent",
  fields: ["customerId", "status", "createdAt"]
  // customerId (high selectivity) > status (low selectivity) > createdAt (sort)
});
```

**4. Sparse Indexes for Optional Fields:**

```aql
-- Use sparse for fields with many null values
db.users.ensureIndex({
  type: "persistent",
  fields: ["premiumExpiryDate"],
  sparse: true  // Only indexes non-null values
});
```

---

### Index Monitoring

**Track Index Usage:**

```javascript
// Create index usage tracker
db._query(`
  FOR idx IN @@collection.getIndexes()
    LET usage = idx.figures
    RETURN {
      name: idx.name,
      fields: idx.fields,
      selectivity: idx.selectivityEstimate,
      lookups: usage.lookups,
      inserts: usage.inserts,
      removes: usage.removes
    }
`, {
  "@collection": "users"
});
```

**Remove Unused Indexes:**

```bash
# Find indexes with no lookups in last 24h
themisdb-admin index-usage --min-age 24h --zero-lookups

# Drop unused index
db._collection("users").dropIndex("idx_unused");
```

**⚠️ Warning:** Each index adds overhead to write operations. Keep only necessary indexes.

---

### Vector Index Adaptive Tuning

**Enable Adaptive Parameter Adjustment:**

```cpp
// Automatic parameter tuning based on query performance
HnswParameterTuner::Config config;
config.adaptive = true;                           // Enable adaptation
config.target_recall = 0.95;                      // Target 95% recall
config.target_latency = std::chrono::milliseconds(10); // Target 10ms latency
config.scale_with_dataset = true;                 // Scale with data growth

HnswParameterTuner tuner(config);

// Query loop - tuner adapts ef_search automatically
while (processing_queries) {
    int optimal_ef = tuner.getOptimalEfSearch(k, dataset_size);
    auto results = index.search(query_vector, k, optimal_ef);
    
    // Record results for adaptation
    tuner.recordQueryResult(k, optimal_ef, latency_ms, recall);
}

// Monitor adaptation
auto stats = tuner.getStats();
std::cout << "Queries processed: " << stats.queries_processed << std::endl;
std::cout << "Current ef_search: " << stats.current_ef_search << std::endl;
std::cout << "Adaptations: " << stats.adaptations_count << std::endl;
std::cout << "Avg latency: " << stats.avg_latency_ms << "ms" << std::endl;
std::cout << "Avg recall: " << stats.avg_recall << std::endl;
```

**Workload Detection and Auto-Configuration:**

```cpp
// Pseudocode example: Detect workload pattern and configure automatically
// Note: QueryPattern, WorkloadDetector, and analyzeQueryPattern are conceptual
// and would need to be implemented based on your specific monitoring system

class WorkloadDetector {
public:
    HnswParameterTuner::WorkloadType detectWorkload(const QueryPattern& pattern) {
        if (pattern.avg_k < 20 && pattern.qps > 5000) {
            return HnswParameterTuner::WorkloadType::OLTP;  // Small k, high QPS
        }
        if (pattern.avg_k > 100 || pattern.batch_size > 100) {
            return HnswParameterTuner::WorkloadType::ANALYTICS;  // Large k or batch queries
        }
        if (pattern.avg_dim > 1024 && pattern.avg_k < 50) {
            return HnswParameterTuner::WorkloadType::RAG;  // High-dim for embeddings
        }
        return HnswParameterTuner::WorkloadType::MIXED;
    }
};

// Auto-configure based on detected workload
auto pattern = analyzeQueryPattern(recent_queries);
WorkloadDetector detector;
auto workload = detector.detectWorkload(pattern);
auto config = HnswParameterTuner::getWorkloadOptimizedConfig(dataset_size, workload);

// Apply configuration
updateIndexConfiguration(config);
```

**Performance Tuning Checklist:**

- [ ] Profile current workload (QPS, latency distribution, k values)
- [ ] Select appropriate workload type (OLTP/Analytics/RAG/Mixed)
- [ ] Configure index with workload-optimized parameters
- [ ] Enable adaptive tuning for runtime optimization
- [ ] Monitor recall and latency metrics
- [ ] Adjust target_recall and target_latency based on requirements
- [ ] Consider index rebuild when dataset grows 5x
- [ ] Use post-filtering overfetch for filtered queries
- [ ] Enable NUMA awareness for large datasets (>1M vectors)
- [ ] Use GPU acceleration for Analytics workloads

---

## Memory Configuration

### Memory Architecture

**ThemisDB Memory Layout:**

```
Total System Memory (64 GB)
├── Operating System (8 GB)
├── ThemisDB Process (48 GB)
│   ├── Query Execution (16 GB)
│   ├── Cache (24 GB)
│   │   ├── Document Cache (16 GB)
│   │   └── Query Result Cache (8 GB)
│   ├── RocksDB (6 GB)
│   └── Connections (2 GB)
└── Other Processes (8 GB)
```

---

### Optimal Memory Configuration

```yaml
# themisdb.conf

server:
  # Total memory limit (70-80% of system RAM)
  maxMemorySize: 48GB
  
  # Memory allocation strategy
  memoryAllocator: mimalloc  # Options: system, jemalloc, mimalloc
  
cache:
  # Total cache size (40-50% of maxMemorySize)
  size: 24GB
  
  # Cache eviction policy
  evictionPolicy: lru  # Options: lru, lfu, random
  
query:
  # Maximum memory per query
  maxMemoryPerQuery: 4GB
  
  # Query result cache
  cacheMaxMemory: 8GB
  cacheMaxEntries: 100000
  cacheMode: demand  # Options: off, demand, on
  
rocksdb:
  # Block cache for compressed blocks
  blockCacheSize: 6GB
  
  # Write buffer (v1.5.0 optimized defaults)
  writeBufferSize: 512MB         # Increased from 256MB
  maxWriteBufferNumber: 6        # Increased from 3-4
  dbWriteBufferSize: 2GB         # Total limit across all CFs
  
  # Async I/O (enabled by default)
  enableAsyncIO: true
  asyncIOReadaheadSize: 128MB
```

---

### Memory-Intensive Workload Tuning

**Analytics Workload:**

```yaml
# Optimize for large queries
query:
  maxMemoryPerQuery: 16GB
  spillToDisk: true
  spillDirectory: /fast-ssd/themisdb-spill/
  
cache:
  size: 32GB
  evictionPolicy: lfu  # Keep frequently used data
```

**OLTP Workload:**

```yaml
# Optimize for many small transactions
cache:
  size: 16GB
  evictionPolicy: lru
  
rocksdb:
  # v1.5.0: Optimized for write-amplification reduction
  writeBufferSize: 512MB         # Larger memtables
  maxWriteBufferNumber: 6        # More write buffers for high write throughput
  dbWriteBufferSize: 2048MB      # Total limit
  enableAsyncIO: true            # Better scan performance
```

**Mixed Workload:**

```yaml
# Balance between queries and writes
cache:
  size: 24GB
  collections:
    # Hot collections get more cache
    - name: "active_users"
      maxSize: 8GB
    - name: "recent_orders"
      maxSize: 4GB
```

---

### Memory Pressure Handling

**Automatic Memory Management:**

```yaml
server:
  # Trigger cache eviction at 80% memory usage
  memoryWarningThreshold: 0.8
  
  # Emergency eviction at 90%
  memoryCriticalThreshold: 0.9
  
  # Actions on memory pressure
  memoryPressureAction: evict  # Options: evict, reject, slowdown
```

**Query Memory Limits:**

```aql
-- Per-query memory limit
OPTIONS {maxMemoryPerQuery: 2GB}
FOR doc IN large_collection
  RETURN doc

-- Enable disk spillover for large sorts
OPTIONS {spillToDisk: true}
FOR doc IN huge_collection
  SORT doc.timestamp DESC
  RETURN doc
```

---

## Cache Tuning

### Vector Embedding Cache Optimization (v1.6.0)

**Cache-Miss Reduction for High-Dimensional Vectors:**

ThemisDB v1.6.0 introduces targeted cache optimizations for 1536-dimensional embedding vectors (OpenAI ada-002, GPT-4, etc.). These optimizations significantly reduce cache-miss penalties during vector similarity searches.

**Key Optimizations:**

1. **Memory Alignment** (5-15% improvement)
   - 32-byte aligned storage for AVX2/AVX-512 SIMD operations
   - Eliminates unaligned load penalties in distance calculations
   - Automatic alignment via `AlignedVectorAllocator`

2. **Prefetch Hints** (10-20% improvement)
   - Hardware prefetch instructions in SIMD distance functions
   - Prefetches 64 floats (256 bytes) ahead into L2 cache
   - Reduces memory stall cycles during computation

3. **Cache-Blocking** (5-10% improvement)
   - Process vectors in blocks of 8 (~48KB per block)
   - Improves temporal locality in L1/L2 caches
   - Multi-level prefetch for 1536D vectors (at offsets: 0, 384, 768, 1152)

**Usage Example:**

```cpp
#include <vector>

// Create embedding storage
std::vector<float> embedding(1536);

// Fill embedding from model
for (size_t i = 0; i < 1536; ++i) {
    embedding[i] = model_output[i];
}

// Store in cache (internally uses aligned storage for SIMD optimization)
cache.store("query_key", embedding);
```

**Configuration:**

```yaml
cache:
  embedding_cache:
    # Cache size (affects how many 1536D vectors fit in memory)
    # Each 1536D vector = ~6KB, so 100k vectors = ~600MB
    max_entries: 100000
    
    # Enable HNSW index for fast ANN search
    use_vector_index: true
    
    # Similarity threshold for cache hits
    similarity_threshold: 0.95
    
    # Cache directory (ensure SSD for best performance)
    cache_dir: /fast-ssd/themis_embedding_cache/
```

**Performance Measurement:**

```bash
# Benchmark embedding cache with 1536D vectors
themisdb-bench \
  --workload embedding_search \
  --vector-dim 1536 \
  --cache-size 100000 \
  --queries 10000 \
  --enable-alignment

# Expected results (compared to unaligned baseline):
# - Cache hit latency: ~0.5ms → ~0.4ms (-20%)
# - L2 cache misses: ~1500/query → ~1100/query (-27%)
# - Throughput: 2000 qps → 2400 qps (+20%)
```

**Architecture Considerations:**

- **x86-64 with AVX2**: 32-byte alignment optimal
- **x86-64 with AVX-512**: 64-byte alignment for best results (use `CacheLineVector`)
- **ARM NEON**: 16-byte alignment sufficient (use `SimdVector`)
- **Large L3 cache (>16MB)**: Increase block size to 16 vectors
- **NUMA systems**: Use NUMA-aware allocation (future enhancement)

---

### Document Cache

**Configuration:**

```yaml
cache:
  # Total cache size
  size: 24GB
  
  # Number of cache shards (reduces lock contention)
  shards: 16
  
  # Cache warmup on startup
  preload: true
  preloadCollections:
    - "users"
    - "products"
  
  # Collection-specific settings
  collections:
    - name: "users"
      maxSize: 4GB
      ttl: 3600  # Seconds
      cacheEnabled: true
    
    - name: "large_archive"
      cacheEnabled: false  # Don't cache cold data
```

---

### Query Result Cache

**Maximize Cache Hits:**

```javascript
// 1. Use consistent query patterns
// BAD: Dynamic limits
db._query(`FOR doc IN users LIMIT ${Math.random() * 100} RETURN doc`);

// GOOD: Fixed limits
db._query(`FOR doc IN users LIMIT 100 RETURN doc`);

// 2. Enable query cache explicitly
db._query({
  query: "FOR doc IN users FILTER doc.status == @status RETURN doc",
  bindVars: {status: "active"},
  options: {cache: true}
});

// 3. Cache expensive aggregations
const cacheKey = "daily_stats_" + currentDate;
let result = queryCache.get(cacheKey);
if (!result) {
  result = db._query(`/* expensive aggregation */`);
  queryCache.set(cacheKey, result, {ttl: 3600});
}
```

---

### CPU Prefetch Optimization (v1.4.1+)

**What is CPU Prefetch?**

CPU prefetch hints are low-level instructions that tell the CPU to load data from memory into cache before it's actually needed. This can significantly reduce memory access latency for predictable access patterns.

**When to Use:**

- ✅ **Random access patterns**: Point lookups, multiGet operations
- ✅ **Sequential scans**: Iterator-based operations, prefix/range scans
- ✅ **Large datasets**: When working set exceeds L3 cache size
- ✅ **Batch operations**: Processing multiple items in sequence
- ❌ **Small datasets**: When data already fits in cache
- ❌ **Streaming data**: One-time access with no reuse

**Configuration:**

```yaml
# RocksDB Storage Configuration
rocksdb:
  # Enable CPU prefetch hints (v1.4.1+)
  enable_cpu_prefetch: true
  
  # Number of items to prefetch ahead (1-8)
  # Higher values = more aggressive, more memory bandwidth
  # Lower values = less overhead, better for small batches
  prefetch_distance: 2  # Default: 2, recommended for most workloads
  
  # Minimum batch size to enable prefetch
  # Prefetch overhead isn't worth it for tiny batches
  prefetch_min_batch_size: 4  # Default: 4
```

**Performance Impact:**

| Operation | Without Prefetch | With Prefetch | Improvement |
|-----------|------------------|---------------|-------------|
| Random Point Reads | ~1.0 µs | ~0.75 µs | **25%** |
| Batch MultiGet (100 keys) | ~100 µs | ~65 µs | **35%** |
| Prefix Scan (1000 entries) | ~500 µs | ~425 µs | **15%** |
| Sequential Range Scan | ~300 µs | ~270 µs | **10%** |

**Tuning Guidelines:**

```yaml
# For random access workloads (OLTP)
rocksdb:
  enable_cpu_prefetch: true
  prefetch_distance: 3  # Moderate prefetch (2-3 typical)
  prefetch_min_batch_size: 4

# For sequential scan workloads (Analytics)
rocksdb:
  enable_cpu_prefetch: true
  prefetch_distance: 5  # Aggressive prefetch (4-6 typical)
  prefetch_min_batch_size: 8
  
# For mixed workloads
rocksdb:
  enable_cpu_prefetch: true
  prefetch_distance: 2  # Conservative default
  prefetch_min_batch_size: 4

# For very small databases (< 1GB)
rocksdb:
  enable_cpu_prefetch: false  # Data likely in cache already
```

**Best Practices:**

1. **Enable for large datasets**: Prefetch shows most benefit when working set > L3 cache
2. **Monitor cache hit rates**: Use RocksDB statistics to verify improvement
3. **Test with your workload**: Benchmark with representative data patterns
4. **Consider memory bandwidth**: Higher prefetch_distance uses more bandwidth
5. **Combine with async I/O**: For best results, enable both CPU prefetch and async I/O

**Monitoring Prefetch Effectiveness:**

```bash
# Check RocksDB block cache hit rate (should increase with prefetch)
curl http://localhost:8529/_admin/statistics | jq '.rocksdb.block_cache_hit_rate'

# Monitor memory bandwidth usage (adjust path based on your build directory)
cd <your-build-directory>/benchmarks  # e.g., build/benchmarks or out/benchmarks
perf stat -e cache-references,cache-misses,cycles,instructions ./bench_random_access_prefetch

# Expected improvement: 15-30% fewer cache misses
```

---

### Cache Monitoring

**Metrics to Track:**

```bash
# Cache hit rate (target: >80%)
curl http://localhost:8529/_admin/statistics | jq '.server.cacheHitRate'

# Cache evictions (lower is better)
curl http://localhost:8529/_admin/statistics | jq '.server.cacheEvictions'

# Memory usage
curl http://localhost:8529/_admin/statistics | jq '.server.physicalMemory'
```

**Cache Performance Dashboard:**

```javascript
// cache_dashboard.js
const stats = db._connection.GET('/_admin/statistics');

console.log('Cache Performance:');
console.log(`  Hit Rate: ${(stats.server.cacheHitRate * 100).toFixed(2)}%`);
console.log(`  Size: ${(stats.server.cacheSize / 1024 / 1024 / 1024).toFixed(2)} GB`);
console.log(`  Evictions: ${stats.server.cacheEvictions}`);
console.log(`  Misses: ${stats.server.cacheMisses}`);

// Alert if hit rate drops below 70%
if (stats.server.cacheHitRate < 0.7) {
  console.log('⚠️  WARNING: Low cache hit rate - consider increasing cache size');
}
```

---

### Cache Warming Strategies

**Preload Hot Data:**

```javascript
// startup_cache_warmer.js
const hotCollections = ['users', 'products', 'categories'];

for (const collName of hotCollections) {
  console.log(`Warming cache for ${collName}...`);
  
  // Load most accessed documents
  db._query(`
    FOR doc IN ${collName}
      FILTER doc.accessCount > 100
      RETURN doc
  `);
  
  // Precompute common aggregations
  db._query(`
    FOR doc IN ${collName}
      COLLECT status = doc.status WITH COUNT INTO num
      RETURN {status, num}
  `);
}

console.log('Cache warming complete');
```

---

## Lock-Free Data Structures

### Understanding Lock Contention

Lock contention occurs when multiple threads compete for the same mutex, causing:
- **Thread blocking**: Waiting threads waste CPU cycles
- **Cache line bouncing**: Lock ownership transfers between cores
- **Reduced parallelism**: Only one thread makes progress at a time
- **Priority inversion**: High-priority threads wait for low-priority lock holders

**Symptoms of Lock Contention:**
- High CPU usage with low throughput
- Thread pool saturation
- Increased p99 latency
- Lock wait time in profilers

---

### Lock-Free Alternatives

ThemisDB provides several lock-free data structures for high-performance scenarios:

#### 1. **Lock-Free Ring Buffer (SPSC)**

Single-Producer Single-Consumer ring buffer using atomic operations:

```cpp
#include "performance/lockfree_metrics_buffer.h"

using namespace themis::performance;

// Create lock-free buffer (must be power of 2)
LockFreeRingBuffer<MetricsEntry, 1024> buffer;

// Producer thread (lock-free push)
MetricsEntry entry{/* ... */};
if (!buffer.tryPush(entry)) {
    // Buffer full - handle overflow
}

// Consumer thread (lock-free pop)
MetricsEntry entry;
if (buffer.tryPop(entry)) {
    // Process entry
}
```

**Performance Characteristics:**
- Push/Pop: O(1) with ~10-20 CPU cycles
- No locks, no syscalls
- Cache-line aligned to prevent false sharing
- 64-byte padding between atomic counters

**Use Cases:**
- High-frequency metrics collection
- Event logging
- Message passing between threads
- Producer-consumer patterns

---

#### 2. **Reader-Writer Locks (std::shared_mutex)**

Allow concurrent reads while serializing writes:

```cpp
#include <shared_mutex>

class DataStore {
    std::shared_mutex rw_mutex_;
    std::map<std::string, std::string> data_;
    
public:
    // Multiple readers can run concurrently
    std::optional<std::string> read(const std::string& key) {
        std::shared_lock lock(rw_mutex_);  // Shared lock
        auto it = data_.find(key);
        return it != data_.end() ? std::optional(it->second) : std::nullopt;
    }
    
    // Writers get exclusive access
    void write(const std::string& key, const std::string& value) {
        std::unique_lock lock(rw_mutex_);  // Exclusive lock
        data_[key] = value;
    }
};
```

**Performance Gain:**
- Read-heavy workloads: 10-50x improvement
- Write latency: Same as regular mutex
- Memory overhead: Minimal (~8 bytes)

**Best for:**
- Read-to-write ratio > 10:1
- Caches and lookup tables
- Configuration data
- Log files (append-only writes, many reads)

**ThemisDB Examples:**
- WiscKey value log (see `src/performance/wisckey.cpp`)
- Index structures with read-heavy access patterns

---

#### 3. **RCU (Read-Copy-Update)**

Lock-free reads with deferred reclamation for read-dominated workloads (>90% reads):

```cpp
#include "performance/rcu.h"

using namespace themis::rcu;

// RCU-protected pointer
std::atomic<Config*> config_ptr;

// Reader: Lock-free access
{
    RCUPtr<Config> config(config_ptr);
    // Use config->setting
    // No locks, just atomic load
}

// Writer: Copy-modify-update pattern
void update_config(const Config& new_config) {
    Config* old_config = config_ptr.load();
    Config* new_copy = new Config(new_config);
    
    // Atomic swap
    config_ptr.store(new_copy, std::memory_order_release);
    
    // Defer deletion until all readers finish
    GracePeriodManager::instance().call_rcu([old_config]() {
        delete old_config;
    });
}
```

**Performance Characteristics:**
- Read latency: 2-5 CPU cycles (just atomic load)
- Write latency: Higher (due to copy + grace period)
- Memory usage: 2x during grace period

**Compile-time flag:**
```bash
# Enable RCU for index structures
cmake -DTHEMIS_USE_RCU_INDEX=ON ..
```

**Use Cases:**
- Configuration hot-reload
- Routing tables
- Read-heavy indexes
- Shared state with rare updates

**⚠️ Trade-off:** RCU sacrifices write performance for zero-cost reads. Only use when reads outnumber writes by 10:1 or more.

---

#### 4. **Thread-Local Storage**

Eliminate contention by giving each thread its own data:

```cpp
// Thread-local buffer (no synchronization needed)
thread_local ThreadLocalMetricsBuffer buffer;

void record_metric(const std::string& name, uint64_t value) {
    // Each thread writes to its own buffer
    buffer.record(name, value);  // Lock-free
}

// Background thread drains all thread-local buffers periodically
void drain_all_buffers() {
    // Snapshot thread buffer pointers (minimal lock time)
    std::vector<ThreadLocalMetricsBuffer*> buffers = get_thread_buffers();
    
    // Drain each buffer lock-free
    for (auto* buf : buffers) {
        buf->drain(output);  // No lock held
    }
}
```

**Pattern Benefits:**
- Zero contention during writes
- Batch processing during drain
- Cache-friendly (thread-local data)

**Used in:**
- Metrics collection (`async_metrics_exporter.cpp`)
- Memory allocators
- Logging systems

---

### Optimizing Lock-Prone Patterns

#### Before: Lock-Heavy Frontier Processing

```cpp
// ❌ BAD: Lock on every edge result
Frontier next_frontier;
std::mutex frontier_mutex;

process_edges(frontier, [&](NodeID src, NodeID dst) {
    if (should_add(dst)) {
        std::lock_guard lock(frontier_mutex);  // Lock contention!
        next_frontier.add(dst);
    }
});
```

**Problem:** O(edges) lock operations, high contention

#### After: Thread-Local Buffers

```cpp
// ✅ GOOD: Thread-local buffers, single merge
std::vector<std::vector<NodeID>> thread_buffers(num_threads);

process_edges(frontier, [&](NodeID src, NodeID dst) {
    if (should_add(dst)) {
        size_t tid = get_thread_id();
        thread_buffers[tid].push_back(dst);  // Lock-free
    }
});

// Single merge at end (minimal lock time)
for (const auto& buffer : thread_buffers) {
    for (NodeID v : buffer) {
        next_frontier.add(v);
    }
}
```

**Improvement:** O(edges) → O(threads) lock operations, ~100x reduction

---

### Lock-Free Best Practices

1. **Identify Hot Locks:**
```bash
# Use perf to find lock contention
perf record -e lock:contention_begin -g -p $(pgrep themisdb)
perf report --stdio
```

2. **Measure Before Optimizing:**
```cpp
// Profile lock hold time
auto start = std::chrono::high_resolution_clock::now();
{
    std::lock_guard lock(mutex_);
    // Critical section
}
auto duration = std::chrono::high_resolution_clock::now() - start;
```

3. **Choose the Right Alternative:**

| Lock Type | Reads | Writes | Complexity | Use When |
|-----------|-------|--------|------------|----------|
| `std::mutex` | Serial | Serial | Simple | Balanced R/W |
| `std::shared_mutex` | Concurrent | Serial | Simple | Read-heavy (10:1) |
| RCU | Lock-free | Copy-update | Medium | Read-dominated (90:1) |
| Lock-free structures | Lock-free | Lock-free | Complex | Extreme performance |
| Thread-local | No sync | Periodic merge | Simple | Per-thread data |

4. **Cache Line Awareness:**
```cpp
// Prevent false sharing with alignment
struct alignas(64) ThreadData {
    std::atomic<uint64_t> counter;
    // Padding ensures each counter is on separate cache line
};
```

5. **Memory Ordering:**
```cpp
// Relaxed: Fastest, no ordering guarantees
counter.fetch_add(1, std::memory_order_relaxed);

// Acquire/Release: Synchronize data access
ptr.store(value, std::memory_order_release);  // Writer
auto* p = ptr.load(std::memory_order_acquire); // Reader

// Sequential consistency: Strongest, slowest (default)
counter.fetch_add(1, std::memory_order_seq_cst);
```

---

### Performance Impact

**Real-World Results:**

| Optimization | Component | Improvement |
|--------------|-----------|-------------|
| Thread-local buffers | Metrics collection | 90% less contention |
| std::shared_mutex | WiscKey value log | 10-50x read throughput |
| Lock-free frontier | Ligra graph processing | 100x fewer lock ops |
| RCU indexes | Vector search | 5ns → 2ns read latency |

**⚠️ Warning:** Lock-free programming is complex. Always:
- Start with std::mutex
- Profile to identify bottlenecks
- Use existing lock-free structures when possible
- Test thoroughly with ThreadSanitizer (`-fsanitize=thread`)

---

## Batch Operations

**🚧 PLANNED: Optimized Batch Operations (Infrastructure Ready)**

ThemisDB includes new batch operation infrastructure with configurable durability modes. Full HTTP API integration is planned for an upcoming release:
- **Async mode (planned)**: 2-5x faster with async WAL (production-safe)
- **NoSync mode (planned)**: 10-20x faster with optimized configurations (bulk loads)
- **Adaptive batching (existing)**: Automatic size tuning via BatchOperationManager

**Current Status:**
- ✅ `BatchWriteOptimizer` component implemented
- ✅ Comprehensive documentation available
- 🚧 HTTP API integration planned (see BATCH_OPERATIONS_GUIDE.md)

📖 **See:** [Batch Operations Guide](./BATCH_OPERATIONS_GUIDE.md) | [Quick Start](./BATCH_OPERATIONS_QUICKSTART.md)

---

### Bulk Inserts

**Efficient Batch Insertion:**

```javascript
// BAD: Individual inserts (slow)
for (let i = 0; i < 10000; i++) {
  db.users.save({name: `User ${i}`, email: `user${i}@example.com`});
}

// GOOD: Batch insert
const documents = [];
for (let i = 0; i < 10000; i++) {
  documents.push({
    name: `User ${i}`,
    email: `user${i}@example.com`
  });
}
db.users.save(documents);  // Single round-trip

// BEST: Use streams for very large imports
const fs = require('fs');
const readline = require('readline');

const rl = readline.createInterface({
  input: fs.createReadStream('users.jsonl'),
});

let batch = [];
const BATCH_SIZE = 1000;

rl.on('line', (line) => {
  batch.push(JSON.parse(line));
  
  if (batch.length >= BATCH_SIZE) {
    db.users.save(batch);
    batch = [];
  }
});

rl.on('close', () => {
  if (batch.length > 0) {
    db.users.save(batch);
  }
});
```

---

### Batch Updates

**Update Multiple Documents:**

```aql
-- Update in batches to avoid memory issues
FOR doc IN users
  FILTER doc.status == "pending"
  LIMIT 10000
  UPDATE doc WITH {
    status: "active",
    updatedAt: DATE_NOW()
  } IN users
  OPTIONS {keepNull: false}

-- For very large updates, use cursor
LET batchSize = 10000
LET cursor = (FOR doc IN users FILTER doc.needsUpdate RETURN doc._key)

FOR batch IN cursor
  LIMIT batchSize
  FOR key IN batch
    UPDATE key WITH {updated: true} IN users
```

---

### Batch Deletes

```aql
-- Delete in chunks to avoid long locks
FOR doc IN old_logs
  FILTER doc.timestamp < DATE_SUBTRACT(DATE_NOW(), 90, "days")
  LIMIT 10000
  REMOVE doc IN old_logs

-- For production: Add delay between batches
// JavaScript
async function deleteOldLogs() {
  let deletedCount = 0;
  const BATCH_SIZE = 5000;
  
  while (true) {
    const result = await db._query(`
      FOR doc IN old_logs
        FILTER doc.timestamp < DATE_SUBTRACT(DATE_NOW(), 90, "days")
        LIMIT ${BATCH_SIZE}
        REMOVE doc IN old_logs
        RETURN OLD
    `);
    
    const count = result.count();
    deletedCount += count;
    
    if (count < BATCH_SIZE) break;
    
    // Avoid overwhelming the system
    await new Promise(resolve => setTimeout(resolve, 1000));
  }
  
  console.log(`Deleted ${deletedCount} old logs`);
}
```

---

### Transaction Batching

```javascript
// Group operations into transactions
const trx = db._createTransaction({
  collections: {
    write: ['users', 'orders', 'audit_log']
  }
});

try {
  // All or nothing
  trx.collection('users').save({_key: 'user1', name: 'John'});
  trx.collection('orders').save({userId: 'user1', amount: 100});
  trx.collection('audit_log').save({action: 'order_created', userId: 'user1'});
  
  trx.commit();
} catch (e) {
  trx.abort();
  throw e;
}
```

**💡 Pro Tip:** Batch operations can be 100x faster than individual operations.

---

## Connection Pooling

### Pool Configuration

**Node.js Driver:**

```javascript
const { Database } = require('themisdb');

const db = new Database({
  url: 'http://localhost:8529',
  databaseName: 'mydb',
  auth: { username: 'root', password: 'password' },
  
  // Connection pool settings
  pool: {
    min: 10,           // Minimum connections
    max: 100,          // Maximum connections
    acquireTimeoutMillis: 30000,
    idleTimeoutMillis: 30000,
    createTimeoutMillis: 10000,
    destroyTimeoutMillis: 5000,
    reapIntervalMillis: 1000,
    createRetryIntervalMillis: 200
  },
  
  // Request timeout
  timeout: 30000
});
```

**Python Driver:**

```python
from themisdb import ThemisClient
from themisdb.connection import ConnectionPool

pool = ConnectionPool(
    hosts='http://localhost:8529',
    database='mydb',
    username='root',
    password='password',
    
    # Pool settings
    pool_size=100,
    max_overflow=20,
    timeout=30,
    recycle=3600,
    pre_ping=True
)

client = ThemisClient(connection_pool=pool)
```

---

### Pool Sizing

**Formula:**

```
pool_size = ((core_count * 2) + effective_spindle_count)

For web applications:
- Minimum: 10
- Maximum: (available_connections / number_of_app_instances)
- Typical: 20-50 per instance
```

**Example Calculation:**

```
System: 8 cores, SSD storage (assume 4 effective spindles)
Pool size = (8 * 2) + 4 = 20 connections

With 5 application instances:
Max pool per instance = 100 total connections / 5 instances = 20
```

---

### Pool Monitoring

```javascript
// Monitor pool health
setInterval(() => {
  const stats = db.getPoolStatistics();
  
  console.log('Connection Pool Status:');
  console.log(`  Size: ${stats.size}`);
  console.log(`  Available: ${stats.available}`);
  console.log(`  Pending: ${stats.pending}`);
  console.log(`  Borrowed: ${stats.borrowed}`);
  
  // Alert if pool is exhausted
  if (stats.available === 0 && stats.pending > 10) {
    console.log('⚠️  WARNING: Connection pool exhausted!');
  }
  
  // Alert if too many idle connections
  if (stats.available > stats.size * 0.8) {
    console.log('💡 TIP: Consider reducing pool size');
  }
}, 60000);
```

---

### Client-Side Connection Pooling

**Wire Protocol Connection Pool (C++):**

ThemisDB provides client-side connection pooling helpers for the Wire Protocol, gRPC, and HTTP protocols to efficiently reuse outbound connections from your application to the server:

```cpp
#include "network/wire_protocol_connection_pool.h"

using namespace themis::network;

// Configure connection pool
WireProtocolConnectionPool::Config config;
config.min_connections_per_target = 2;
config.max_connections_per_target = 20;
config.idle_timeout = std::chrono::seconds(60);
config.connect_timeout = std::chrono::seconds(5);
config.acquire_timeout = std::chrono::seconds(10);
// NOTE: SSL/TLS is not yet implemented for wire protocol
config.enable_ssl = false;
config.enable_warmup = true;

// Create pool
auto pool = std::make_unique<WireProtocolConnectionPool>(config);

// Warm up connections for known targets
pool->warmup("localhost:8766");

// Acquire connection (RAII handle)
{
    auto conn = pool->acquireConnection("localhost:8766");
    // Use connection...
    // Automatically returned to pool when scope ends
}

// Monitor pool statistics
auto stats = pool->getStats();
std::cout << "Total connections: " << stats.total_connections << std::endl;
std::cout << "Available: " << stats.available_connections << std::endl;
std::cout << "In use: " << stats.in_use_connections << std::endl;
std::cout << "Reuse rate: " << (stats.getReuseRate() * 100.0) << "%" << std::endl;
```

**gRPC Channel Pool:**

```cpp
#include "utils/grpc_channel_pool.h"

using namespace themis::utils;

// Configure gRPC channel pool
GrpcChannelPool::Config config;
config.max_channels_per_target = 10;
config.idle_timeout = std::chrono::seconds(30);
config.enable_keepalive = true;
config.keepalive_time = std::chrono::seconds(30);
config.max_concurrent_streams = 100;

auto pool = std::make_unique<GrpcChannelPool>(config);

// Acquire channel
auto channel = pool->acquireChannel("localhost:50051", 
                                    grpc::InsecureChannelCredentials());

// Use channel for RPC
auto stub = MyService::NewStub(channel);

// Release when done
pool->releaseChannel("localhost:50051", channel);

// Monitor
auto stats = pool->getStats();
std::cout << "Channels created: " << stats.channels_created << std::endl;
std::cout << "Channels reused: " << stats.channels_reused << std::endl;
```

**HTTP Client Pool:**

```cpp
#include "utils/http_client_pool.h"

using namespace themis::utils;

// Configure HTTP pool
HTTPClientPool::Config config;
config.max_connections = 50;
config.idle_timeout = std::chrono::seconds(30);
config.enable_keepalive = true;
config.io_threads = 8;
config.lock_stripes = 8;  // Reduces lock contention

auto pool = std::make_unique<HTTPClientPool>(config);

// Make async requests
auto future = pool->post("http://api.example.com/endpoint",
                        json_body,
                        {{"Content-Type", "application/json"}});

// Wait for response
auto response = future.get();
if (response.isSuccess()) {
    // Process response.body
}
```

**Configuration File:**

See `config/connection_pool_config.yaml` for example connection pool presets and recommended settings. These are reference configurations that can be adapted to your application's connection pool initialization code:

```yaml
# Production balanced workload (example - adapt to your code)
production:
  wire_protocol:
    max_connections_per_target: 20
    enable_warmup: true
    enable_ssl: false  # TLS not yet implemented for wire protocol
    
  grpc:
    max_channels_per_target: 10
    enable_keepalive: true
    
  http:
    max_connections: 50
    io_threads: 8

# OLTP workload (high throughput)
oltp:
  wire_protocol:
    max_connections_per_target: 50
    
  grpc:
    max_channels_per_target: 20
    
  http:
    max_connections: 100

# Analytics workload (long queries)
analytics:
  wire_protocol:
    max_connections_per_target: 10
    idle_timeout: 300
    
  http:
    request_timeout: 300
```

**Performance Benefits:**

Connection pooling provides significant performance improvements:

- **TCP Wire Protocol Pool**: 15-20% throughput improvement, 30-40% latency reduction
- **gRPC Channel Pool**: 10-15% throughput improvement by reusing HTTP/2 channels
- **HTTP Client Pool**: 20-25% throughput improvement with Keep-Alive

**Best Practices:**

1. **Pool Sizing**: Use formula `pool_size = ((core_count * 2) + effective_spindle_count)`
2. **Monitoring**: Track reuse rates (target: >80%), acquire timeouts (<0.1%)
3. **Warmup**: Enable warmup for production to pre-create connections
4. **Health Checks**: Enable keepalive to detect dead connections early
5. **Timeout Configuration**: Balance between connection reuse and resource cleanup
6. **Load Balancing**: Use multiple pools for different service tiers

---

## Hardware Recommendations

### Storage

**SSD vs HDD:**

| Workload | Storage Type | RAID | Notes |
|----------|-------------|------|-------|
| OLTP (High writes) | NVMe SSD | RAID 10 | Low latency critical |
| Analytics | SATA SSD | RAID 5/6 | Sequential reads |
| Archive | HDD | RAID 6 | Cost-effective storage |
| Hybrid | Tiered (SSD+HDD) | RAID 10 + RAID 6 | Hot data on SSD |

**Filesystem:**

```bash
# XFS for large files (recommended)
mkfs.xfs -f -l size=128m -d su=64k,sw=2 /dev/md0

# Mount options
mount -o noatime,nodiratime,nobarrier /dev/md0 /var/lib/themisdb

# /etc/fstab
/dev/md0 /var/lib/themisdb xfs noatime,nodiratime,nobarrier 0 2
```

---

### CPU

**Recommendations:**

- **Minimum:** 4 cores (8 threads)
- **Recommended:** 16+ cores for production
- **Optimal:** High clock speed (3.0+ GHz) > core count for OLTP
- **Analytics:** More cores (32+) for parallel query execution

**CPU Governor:**

```bash
# Set performance governor
for cpu in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
  echo performance > $cpu
done

# Disable CPU idle states (reduces latency)
cpupower idle-set -D 0
```

---

### Memory

**Sizing Guidelines:**

```
Working Set Calculation:
- Document count: 10M
- Average document size: 2 KB
- Total data size: 20 GB

Memory Requirements:
- Active data: 20 GB
- Indexes: 4 GB (20% of data)
- Query execution: 8 GB
- Cache overhead: 4 GB
- OS + other: 8 GB
----------------------------
Total: 44 GB (use 64 GB system)
```

**NUMA Configuration:**

```bash
# Bind to single NUMA node for better performance
numactl --cpunodebind=0 --membind=0 themisdb-server --config /etc/themisdb/themisdb.conf

# Check NUMA topology
numactl --hardware
```

---

### Network

**For Distributed Deployments:**

- **Minimum:** 1 Gbps
- **Recommended:** 10 Gbps
- **Optimal:** 25+ Gbps for large clusters

**Network Tuning:**

```bash
# /etc/sysctl.conf

# Increase network buffers
net.core.rmem_max = 134217728
net.core.wmem_max = 134217728
net.ipv4.tcp_rmem = 4096 87380 67108864
net.ipv4.tcp_wmem = 4096 65536 67108864

# Increase connection backlog
net.core.somaxconn = 4096
net.ipv4.tcp_max_syn_backlog = 4096

# Enable TCP fast open
net.ipv4.tcp_fastopen = 3

# Apply settings
sysctl -p
```

---

## Monitoring and Profiling

### Key Performance Metrics

**1. Query Performance:**

```bash
# Average query time
curl http://localhost:8529/_admin/statistics | jq '.server.queryTime.avg'

# Slow queries
curl http://localhost:8529/_api/query/slow | jq '.'

# Query cache hit rate
curl http://localhost:8529/_admin/statistics | jq '.server.queryCacheHitRate'
```

**2. Throughput:**

```bash
# Operations per second
curl http://localhost:8529/_admin/statistics | jq '.server.opsPerSecond'

# Transactions per second
curl http://localhost:8529/_admin/statistics | jq '.server.transactionsPerSecond'
```

**3. Resource Usage:**

```bash
# Memory
ps -p $(pgrep themisdb-server) -o pid,%mem,rss,vsz

# CPU
top -p $(pgrep themisdb-server) -b -n 1

# Disk I/O
iostat -x 5 | grep dm
```

---

### Prometheus Integration

**Configuration:**

```yaml
# prometheus.yml
scrape_configs:
  - job_name: 'themisdb'
    static_configs:
      - targets: ['localhost:8529']
    metrics_path: '/_admin/metrics'
    scrape_interval: 15s
```

**Key Metrics:**

```promql
# Query latency (95th percentile)
histogram_quantile(0.95, rate(themisdb_query_duration_seconds_bucket[5m]))

# Cache hit rate
rate(themisdb_cache_hits_total[5m]) / (rate(themisdb_cache_hits_total[5m]) + rate(themisdb_cache_misses_total[5m]))

# Operations per second
rate(themisdb_operations_total[1m])

# Connection pool usage
themisdb_connection_pool_active / themisdb_connection_pool_max
```

---

### Grafana Dashboards

**Import Dashboard:**

```bash
# Download ThemisDB dashboard
curl -O https://grafana.com/api/dashboards/12345/revisions/1/download

# Import via Grafana UI or API
curl -X POST http://grafana:3000/api/dashboards/db \
  -H "Content-Type: application/json" \
  -d @dashboard.json
```

**Key Panels:**
- Query latency over time
- Operations per second
- Cache hit rate
- Memory usage
- Connection pool status
- Slow queries
- Error rate

---

### Performance Profiling

**CPU Profiling:**

```bash
# perf record
perf record -p $(pgrep themisdb-server) -g -- sleep 30
perf report --stdio > cpu_profile.txt

# Generate flamegraph
git clone https://github.com/brendangregg/FlameGraph.git
perf script | FlameGraph/stackcollapse-perf.pl | FlameGraph/flamegraph.pl > profile.svg
```

**Memory Profiling:**

```bash
# heaptrack
heaptrack themisdb-server --config /etc/themisdb/themisdb.conf
heaptrack_gui heaptrack.themisdb-server.12345.gz

# valgrind (slow, for development)
valgrind --tool=massif themisdb-server --config /etc/themisdb/themisdb.conf
ms_print massif.out.12345 > memory_profile.txt
```

---

## Benchmarking Best Practices

### Benchmark Types

**1. Synthetic Benchmarks:**

```bash
# themisdb-bench - Built-in benchmark tool
themisdb-bench \
  --workload write \
  --threads 32 \
  --duration 300 \
  --collection benchmark \
  --operations 1000000

# Mixed workload
themisdb-bench \
  --workload mixed \
  --read-ratio 0.8 \
  --write-ratio 0.2 \
  --threads 16
```

**2. Realistic Workload:**

```javascript
// benchmark.js - Simulate production load
const { Database } = require('themisdb');
const db = new Database({url: 'http://localhost:8529'});

async function benchmark() {
  const startTime = Date.now();
  const operations = 10000;
  let completed = 0;
  
  const tasks = [];
  
  for (let i = 0; i < operations; i++) {
    // Simulate real queries
    tasks.push(
      db._query(`
        FOR user IN users
          FILTER user.lastLogin > @since
          LIMIT 10
          RETURN user
      `, {since: Date.now() - 86400000})
    );
    
    // Simulate writes
    if (i % 10 === 0) {
      tasks.push(
        db.collection('audit_log').save({
          timestamp: Date.now(),
          action: 'query',
          userId: `user${i % 1000}`
        })
      );
    }
  }
  
  await Promise.all(tasks);
  
  const duration = (Date.now() - startTime) / 1000;
  const throughput = operations / duration;
  
  console.log(`Completed ${operations} operations in ${duration}s`);
  console.log(`Throughput: ${throughput.toFixed(2)} ops/sec`);
}

benchmark();
```

---

### Benchmark Methodology

**1. Warm-up Phase:**

```bash
# Always warm up before measuring
themisdb-bench --warmup 60 --duration 300 --workload read
```

**2. Isolation:**

```bash
# Ensure no other load
systemctl stop unnecessary-services

# Pin to specific CPUs
taskset -c 0-15 themisdb-bench ...
```

**3. Multiple Runs:**

```bash
# Run multiple times and average
for i in {1..5}; do
  echo "Run $i"
  themisdb-bench --workload mixed --duration 300 | tee run_${i}.log
  sleep 60  # Cool down between runs
done

# Analyze results
grep "ops/sec" run_*.log | awk '{sum+=$2; count++} END {print "Average:", sum/count}'
```

---

### Interpreting Results

**Baseline Performance:**

| Operation | Target Latency (p95) | Target Throughput |
|-----------|---------------------|-------------------|
| Point read | < 1 ms | 100K ops/sec |
| Point write | < 5 ms | 50K ops/sec |
| Simple query | < 10 ms | 10K queries/sec |
| Complex query | < 100 ms | 1K queries/sec |
| Batch insert (1K docs) | < 50 ms | 20K docs/sec |

**Red Flags:**

- p95 latency > 10x p50 (inconsistent performance)
- Cache hit rate < 70% (inadequate cache)
- CPU > 80% with low throughput (query inefficiency)
- Disk I/O wait > 20% (storage bottleneck)

---

### Performance Testing Checklist

- [ ] Test with production-like data volume
- [ ] Use realistic query patterns
- [ ] Include mixed read/write workloads
- [ ] Test with concurrent connections
- [ ] Monitor all resources (CPU, memory, disk, network)
- [ ] Test with and without indexes
- [ ] Measure cache warm vs cold performance
- [ ] Test failure scenarios
- [ ] Document hardware specifications
- [ ] Compare with previous versions

---

## Quick Reference

### Performance Command Cheatsheet

```bash
# Query analysis
db._explain(query)
db._query({query: query, options: {profile: 2}})

# Index management
db._collection("users").getIndexes()
db._collection("users").ensureIndex({type: "persistent", fields: ["email"]})

# Cache control
curl -X DELETE http://localhost:8529/_api/query/cache
curl -X POST http://localhost:8529/_admin/cache/clear

# Statistics
curl http://localhost:8529/_admin/statistics | jq '.'
curl http://localhost:8529/_api/query/slow

# Monitoring
watch -n 5 'curl -s http://localhost:8529/_admin/statistics | jq ".server.opsPerSecond"'
```

---

**Last Updated:** 2026-04-06  
**Version:** 1.4.0  
**Maintainer:** ThemisDB Team

---

## SIMD Optimization and Cache-Line Performance

### Hardware Acceleration with SIMD

**Vector Operations:**

ThemisDB uses SIMD (Single Instruction, Multiple Data) instructions to accelerate vector operations:

- **x86_64 Platforms**: AVX2 (8 floats) and AVX-512 (16 floats) with FMA
- **ARM64 Platforms**: NEON (4 floats) with FMA support on ARMv8+
- **Automatic Detection**: Falls back to scalar code if SIMD unavailable

**Enabled Operations:**
```cpp
// L2 distance computation (Euclidean)
float distance = simd::l2_distance(vecA, vecB, dimension);

// Squared L2 distance (faster, no sqrt)
float dist_sq = simd::l2_distance_sq(vecA, vecB, dimension);

// Batch operations for multiple vectors
simd::batch_l2_distance_sq(query, database, n_vectors, dim, results);
```

**Performance Benefits:**
- **4-8x faster** than scalar code for vector operations (typical for 128-512 dim vectors on AVX2+)
  - Best case: 8x on AVX-512 with 512-dim aligned vectors
  - Typical case: 4-6x on AVX2 with 128-256 dim vectors
  - ARM NEON: 3-4x on 128+ dim vectors with FMA
- Cache-line prefetching reduces memory latency by ~20-30%
- FMA instructions improve accuracy and throughput

---

### Cache-Line Optimization

**Memory Layout Best Practices:**

```cpp
// BAD: Random access pattern
for (int i = 0; i < n; i++) {
    process(data[random_indices[i]]);
}

// GOOD: Sequential access pattern (cache-friendly)
for (int i = 0; i < n; i++) {
    process(data[i]);
}

// BEST: Aligned and sequential with prefetching
// Use aligned allocation for large arrays
std::vector<float> vectors(n * dim);  // Standard container
// Or for guaranteed alignment:
auto* aligned_data = static_cast<float*>(std::aligned_alloc(64, n * dim * sizeof(float)));
// SIMD functions automatically prefetch 4 cache lines ahead
```

**Cache-Aware Data Structures:**

1. **Align Hot Data**: Use 64-byte alignment for frequently accessed arrays
   ```cpp
   alignas(64) float embeddings[1000000];
   ```

2. **Structure Padding**: Avoid false sharing in multi-threaded code
   ```cpp
   struct ThreadLocalData {
       float data[16];
       char padding[64 - sizeof(float) * 16];  // Separate cache lines
   };
   ```

3. **Batch Processing**: Process data in chunks that fit in L2 cache (256KB typical)
   ```cpp
   const size_t BATCH_SIZE = 32768;  // 128KB for float data
   for (size_t i = 0; i < n; i += BATCH_SIZE) {
       process_batch(data + i, std::min(BATCH_SIZE, n - i));
   }
   ```

**Compilation Flags:**

```bash
# Enable AVX2 on x86_64
cmake -DTHEMIS_ENABLE_AVX2=ON -DCMAKE_BUILD_TYPE=Release ..

# Verify SIMD support
./themisdb-server --version  # Shows: "SIMD: AVX2" or "SIMD: NEON"
```

**Monitoring SIMD Performance:**

```bash
# Check CPU usage patterns (should show high vectorization)
perf stat -e cycles,instructions,fp_arith_inst_retired.scalar_single,fp_arith_inst_retired.128b_packed_single ./benchmark

# Ratio should favor packed instructions for good SIMD utilization
```

---
