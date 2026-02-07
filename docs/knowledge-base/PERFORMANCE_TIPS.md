# ThemisDB Performance Tips & Optimization Guide

Comprehensive guide to optimizing ThemisDB for maximum performance.

## Table of Contents

- [Write-Amplification Optimization](#write-amplification-optimization)
- [Query Optimization Techniques](#query-optimization-techniques)
- [Index Selection and Tuning](#index-selection-and-tuning)
- [Memory Configuration](#memory-configuration)
- [Cache Tuning](#cache-tuning)
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

## Batch Operations

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

**Last Updated:** 2024-01-24  
**Version:** 1.4.0  
**Maintainer:** ThemisDB Team
