# ThemisDB RocksDB Configuration & Optimization Guide

**Version:** 1.4.0  
**Last Updated:** 2026-04-06  
**Target Audience:** Database Administrators, Performance Engineers, Storage Engineers

---

## Table of Contents

1. [RocksDB Fundamentals for ThemisDB](#rocksdb-fundamentals-for-themisdb)
2. [Memory Tuning](#memory-tuning)
3. [Compaction Strategy Selection](#compaction-strategy-selection)
4. [WAL (Write-Ahead Log) Optimization](#wal-write-ahead-log-optimization)
5. [Column Family Configuration](#column-family-configuration)
6. [Compression Configuration](#compression-configuration)
7. [Performance Monitoring](#performance-monitoring)

---

## RocksDB Fundamentals for ThemisDB

### LSM Tree Architecture

**Log-Structured Merge-Tree (LSM) Overview:**

```
┌─────────────────────────────────────────────────────────────┐
│                    LSM Tree Architecture                     │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  Writes → MemTable (in-memory)                               │
│           │                                                   │
│           ↓ (when full)                                      │
│           Immutable MemTable                                 │
│           │                                                   │
│           ↓ (flush to disk)                                  │
│           Level 0 (SST files, overlapping ranges)            │
│           │                                                   │
│           ↓ (compaction)                                     │
│           Level 1 (sorted, non-overlapping)                  │
│           │                                                   │
│           ↓ (compaction)                                     │
│           Level 2, 3, 4... (larger, less frequently accessed)│
│                                                               │
│  Reads → Check MemTable → L0 → L1 → L2 → ...                │
│          Use bloom filters to skip unnecessary levels        │
└─────────────────────────────────────────────────────────────┘
```

**Key Components:**

1. **MemTable:**
   - In-memory write buffer (typically SkipList)
   - Sorted by key
   - Fast writes (O(log n))
   - Configurable size (default: 256MB in ThemisDB)

2. **SST Files (Sorted String Tables):**
   - Immutable on-disk files
   - Contain sorted key-value pairs
   - Each file has index, bloom filter, and data blocks
   - Organized into levels

3. **Levels:**
   - L0: Freshly flushed MemTables (overlapping ranges)
   - L1-L6: Compacted data (non-overlapping ranges)
   - Each level is ~10x larger than previous

**ThemisDB Configuration:**

```yaml
rocksdb:
  # MemTable configuration
  memtable_size_mb: 256
  max_write_buffer_number: 3  # Number of memtables
  
  # Level configuration
  num_levels: 7
  max_bytes_for_level_base_mb: 256  # L1 size
  max_bytes_for_level_multiplier: 10  # Each level 10x larger
  
  # File configuration
  target_file_size_base_mb: 64  # SST file size
  target_file_size_multiplier: 1
```

### Compaction Strategies

**Compaction Purpose:**
- Merge sorted runs from multiple levels
- Remove deleted/obsolete versions
- Maintain read performance
- Control space amplification

**Level-Based Compaction (Default):**

```
L0: [File1] [File2] [File3]  (overlapping)
     │       │       │
     └───────┴───────┘
             ↓ Compact to L1
L1: [────────────────────────]  (non-overlapping)
             ↓ Compact to L2
L2: [────────────────────────────────────────]
```

**Universal Compaction:**

```
All files at same level, periodic full compaction
[File1] [File2] [File3] [File4] [File5]
   │       │       │       │       │
   └───────┴───────┴───────┴───────┘
              ↓ Compact all
   [─────────────────────────────────]
```

**Comparison:**

| Aspect | Level-Based | Universal |
|--------|-------------|-----------|
| Write Amplification | 10-30x | 2-5x |
| Read Amplification | Low (1-2 levels) | Medium (many files) |
| Space Amplification | Low (10-20%) | Medium (20-50%) |
| CPU Usage | Steady | Bursty |
| Best For | Read-heavy, balanced | Write-heavy, append-only |

### Write Amplification

**Definition:** Ratio of bytes written to disk vs. bytes written by application.

**Causes:**
1. MemTable flush to L0
2. L0 → L1 compaction
3. L1 → L2, L2 → L3, etc. compactions
4. WAL writes

**Example Calculation:**

```python
# Application writes 1GB of data
app_write = 1_000_000_000  # 1GB

# Write amplification breakdown:
# 1. MemTable flush to L0: 1GB
# 2. L0 → L1 compaction: 2GB (read L0 + existing L1, write merged)
# 3. L1 → L2 compaction: 4GB
# 4. L2 → L3 compaction: 8GB
# Total written to disk: 1 + 2 + 4 + 8 = 15GB

write_amplification = 15  # 15x

print(f"Write amplification: {write_amplification}x")
```

**Reduction Strategies:**

```yaml
rocksdb:
  # Increase level size ratio (fewer levels)
  max_bytes_for_level_multiplier: 10  # Default
  # Set to 20-50 to reduce compaction frequency
  
  # Larger MemTable (fewer flushes)
  memtable_size_mb: 512  # Increased from 256
  
  # Larger L1 (reduce L0 → L1 compaction frequency)
  max_bytes_for_level_base_mb: 512  # Increased from 256
  
  # Direct I/O (bypass OS page cache)
  use_direct_io_for_flush_and_compaction: true
```

### Read Amplification

**Definition:** Number of disk seeks required to satisfy a read.

**Worst Case Example:**

```python
# Read key "user:12345"
# Check order:
# 1. MemTable (1 check)
# 2. Immutable MemTables (2 checks, 2 immutable memtables)
# 3. L0 files (8 checks, 8 files in L0)
# 4. L1 (1 check, binary search in sorted level)
# 5. L2 (1 check)
# Total: 1 + 2 + 8 + 1 + 1 = 13 reads

read_amplification = 13
```

**Reduction Strategies:**

```yaml
rocksdb:
  # Bloom filters (skip files that don't contain key)
  bloom_bits_per_key: 10  # 1% false positive rate
  whole_key_filtering: true
  
  # Reduce L0 files
  level0_file_num_compaction_trigger: 4  # Default
  # Trigger compaction earlier
  
  # Block cache (cache frequently accessed blocks)
  block_cache_size_mb: 8192  # 8GB
  cache_index_and_filter_blocks: true
  pin_l0_filter_and_index_blocks_in_cache: true
```

---

## Memory Tuning

### MemTable Configuration

**MemTable Types:**

```yaml
rocksdb:
  # SkipList (default, balanced)
  memtable_factory: "skip_list"
  
  # HashSkipList (faster for point lookups)
  # memtable_factory: "hash_skiplist"
  # memtable_prefix_bloom_size_ratio: 0.02
  
  # Vector (sequential writes only)
  # memtable_factory: "vector"
```

**Sizing Guidelines:**

| Workload Type | MemTable Size | Write Buffers | Total Memory |
|---------------|---------------|---------------|--------------|
| OLTP | 128-256 MB | 3 | 384-768 MB |
| Balanced | 256-512 MB | 3 | 768 MB - 1.5 GB |
| Write-Heavy | 512-1024 MB | 6 | 3-6 GB |

**Configuration:**

```yaml
rocksdb:
  # Size of each MemTable
  memtable_size_mb: 256
  
  # Number of MemTables
  # 1 active + (max_write_buffer_number - 1) immutable
  max_write_buffer_number: 3
  
  # Minimum to merge before flush
  min_write_buffer_number_to_merge: 1
  
  # Total limit across all column families
  db_write_buffer_size_mb: 0  # 0 = unlimited
```

**Per-Column Family Configuration:**

```yaml
# In ThemisDB, different column families for different data types
column_families:
  - name: "entities"
    memtable_size_mb: 512  # Large entity data
    
  - name: "edges"
    memtable_size_mb: 256  # Medium edge data
    
  - name: "vectors"
    memtable_size_mb: 1024  # Large vector embeddings
```

### Block Cache Sizing

**Block Cache Purpose:**
- Cache frequently accessed data blocks
- Cache index and filter blocks
- Shared across all column families

**Sizing Formula:**

```python
# Calculate optimal block cache size
total_ram_gb = 64  # Available RAM

# Conservative: 25-30% of RAM
block_cache_conservative_gb = total_ram_gb * 0.25

# Aggressive: 50-60% of RAM (if dedicated DB server)
block_cache_aggressive_gb = total_ram_gb * 0.5

# Reserve for:
# - MemTables: 2-3 GB
# - OS page cache: 8-16 GB
# - Application: 4-8 GB
# - MVCC versions: 2-5 GB
# - Overhead: 2-4 GB

block_cache_optimal_gb = total_ram_gb - (3 + 12 + 6 + 4 + 3)  # = 36 GB

print(f"Recommended block cache: {block_cache_optimal_gb} GB")
```

**Configuration:**

```yaml
rocksdb:
  # Block cache size
  block_cache_size_mb: 36864  # 36 GB
  
  # Number of shards (reduce lock contention)
  block_cache_shard_bits: 6  # 64 shards (2^6)
  # Rule: 2^shard_bits >= num_threads / 2
  
  # Strict capacity limit
  block_cache_strict_capacity_limit: true
  
  # High-priority pool for index/filter blocks
  high_pri_pool_ratio: 0.5  # 50% for index/filters
  
  # Cache index and filter blocks
  cache_index_and_filter_blocks: true
  pin_l0_filter_and_index_blocks_in_cache: true
```

**Monitoring:**

```bash
# Check block cache hit rate
curl http://localhost:9091/metrics | grep block_cache

# Output:
# themisdb_rocksdb_block_cache_hits_total 1250000
# themisdb_rocksdb_block_cache_misses_total 50000
# Hit rate = 1250000 / (1250000 + 50000) = 96.15%
# Target: > 95% for read-heavy workloads
```

### Bloom Filter Optimization

**Bloom Filter Purpose:**
- Probabilistic data structure
- Quickly determine if key might be in SST file
- Avoids unnecessary disk reads

**Configuration:**

```yaml
rocksdb:
  # Bits per key (higher = lower false positive rate)
  bloom_bits_per_key: 10  # ~1% FPR
  
  # Filter whole key (not just prefix)
  whole_key_filtering: true
  
  # Block-based bloom filter (default)
  filter_policy: "bloomfilter"
  
  # Ribbon filter (more space-efficient, experimental)
  # filter_policy: "ribbon"
  # ribbon_bits_per_key: 10
```

**False Positive Rate vs. Memory:**

| Bits Per Key | False Positive Rate | Memory Overhead | Use Case |
|--------------|---------------------|-----------------|----------|
| 5 | ~5% | Low | Write-heavy, memory-constrained |
| 10 | ~1% | Medium | Balanced (recommended) |
| 15 | ~0.1% | High | Read-heavy, latency-sensitive |
| 20 | ~0.01% | Very High | Point query-heavy |

**Impact Calculation:**

```python
# Dataset: 100M keys, 10 bits per key
num_keys = 100_000_000
bits_per_key = 10

# Bloom filter memory
bloom_memory_bits = num_keys * bits_per_key
bloom_memory_mb = bloom_memory_bits / (8 * 1024 * 1024)

print(f"Bloom filter memory: {bloom_memory_mb:.1f} MB")
# Output: Bloom filter memory: 119.2 MB

# Read amplification reduction
# Without bloom filter: Check all 8 L0 files = 8 reads
# With bloom filter (1% FPR): Check 1 + 8*0.01 = 1.08 reads
# Reduction: 8 / 1.08 = 7.4x fewer disk reads
```

### Index Block Cache

**Configuration:**

```yaml
rocksdb:
  # Cache index blocks
  cache_index_and_filter_blocks: true
  
  # Pin L0 index/filter blocks (hot data)
  pin_l0_filter_and_index_blocks_in_cache: true
  
  # Pin top-level index
  pin_top_level_index_and_filter: true
  
  # Partition index blocks (for large files)
  enable_index_partitioning: true
  index_block_size_kb: 4  # 4KB index blocks
  
  # Metadata cache size (separate from block cache)
  metadata_cache_size_mb: 512
```

**Benefits:**

- Faster index lookups (no disk I/O)
- Reduced read latency (especially for random reads)
- Lower CPU usage (cached index traversal)

---

## Compaction Strategy Selection

### Level-Based Compaction (Analysis)

**Configuration:**

```yaml
rocksdb:
  compaction_style: "level"
  
  # Level 0 triggers
  level0_file_num_compaction_trigger: 4
  level0_slowdown_writes_trigger: 20
  level0_stop_writes_trigger: 36
  
  # Level sizes
  max_bytes_for_level_base_mb: 256  # L1 size
  max_bytes_for_level_multiplier: 10  # Each level 10x larger
  
  # File sizes
  target_file_size_base_mb: 64
  target_file_size_multiplier: 1
  
  # Compaction threads
  max_background_compactions: 8
  max_subcompactions: 2  # Parallel compaction within single job
```

**Tuning Parameters:**

| Parameter | Effect | Recommended Range |
|-----------|--------|-------------------|
| `level0_file_num_compaction_trigger` | Lower = more frequent compaction | 2-8 |
| `max_bytes_for_level_base_mb` | Higher = less frequent L0→L1 compaction | 128-512 MB |
| `max_bytes_for_level_multiplier` | Higher = fewer levels, more space amp | 5-20 |
| `target_file_size_base_mb` | Larger files = less compaction overhead | 32-128 MB |

**Workload-Specific Tuning:**

```yaml
# Read-Heavy Workload
rocksdb:
  compaction_style: "level"
  level0_file_num_compaction_trigger: 2  # Aggressive compaction
  max_bytes_for_level_base_mb: 512
  max_bytes_for_level_multiplier: 10
  target_file_size_base_mb: 128  # Larger files

# Write-Heavy Workload
rocksdb:
  compaction_style: "level"
  level0_file_num_compaction_trigger: 8  # Lazy compaction
  max_bytes_for_level_base_mb: 256
  max_bytes_for_level_multiplier: 20  # Fewer levels
  target_file_size_base_mb: 64

# Balanced Workload (default)
rocksdb:
  compaction_style: "level"
  level0_file_num_compaction_trigger: 4
  max_bytes_for_level_base_mb: 256
  max_bytes_for_level_multiplier: 10
  target_file_size_base_mb: 64
```

### Universal Compaction (Analysis)

**Configuration:**

```yaml
rocksdb:
  compaction_style: "universal"
  
  # Universal compaction options
  universal_compaction_options:
    # Size ratio for triggering compaction
    size_ratio: 1  # 1 = compact when files differ by 100%
    
    # Minimum files to compact together
    min_merge_width: 2
    
    # Maximum files to compact together
    max_merge_width: 10
    
    # Maximum space amplification
    max_size_amplification_percent: 200  # 200% = 2x space overhead
    
    # Compression threshold
    compression_size_percent: -1  # -1 = compress all levels
    
    # Stop style
    stop_style: "total_size"  # or "similar_size"
```

**When to Use Universal Compaction:**

✅ **Good for:**
- Write-heavy workloads (append-only logs)
- SSD-constrained environments (minimize write amplification)
- Time-series data
- Log aggregation systems

❌ **Avoid for:**
- Read-heavy workloads (higher read amplification)
- Update-heavy workloads (creates many versions)
- Memory-constrained environments (requires more memory for compaction)

**Example Configuration:**

```yaml
# Time-Series Data (Write-Heavy)
rocksdb:
  compaction_style: "universal"
  universal_compaction_options:
    size_ratio: 1
    min_merge_width: 2
    max_merge_width: 5
    max_size_amplification_percent: 150
    compression_size_percent: 50  # Compress lower 50% of data
```

### Comparison Table with Workload Recommendations

| Workload Characteristic | Level-Based | Universal |
|-------------------------|-------------|-----------|
| **Reads >> Writes** | ✅ Excellent | ⚠️ Acceptable |
| **Writes >> Reads** | ⚠️ Acceptable | ✅ Excellent |
| **Random Updates** | ✅ Good | ❌ Poor |
| **Append-Only** | ⚠️ Acceptable | ✅ Excellent |
| **Point Queries** | ✅ Excellent | ⚠️ Acceptable |
| **Range Scans** | ✅ Excellent | ⚠️ Acceptable |
| **SSD Write Endurance** | ❌ Higher wear | ✅ Lower wear |
| **Memory Usage** | ✅ Lower | ⚠️ Higher (during compaction) |
| **CPU Usage** | ✅ Steady | ⚠️ Bursty |

**Decision Matrix:**

```python
def recommend_compaction_strategy(workload):
    """
    Recommend compaction strategy based on workload characteristics.
    """
    read_write_ratio = workload.reads / workload.writes
    update_ratio = workload.updates / workload.total_ops
    
    if read_write_ratio > 2.0 and update_ratio > 0.3:
        return "level"  # Read-heavy with updates
    elif read_write_ratio < 0.5 and update_ratio < 0.1:
        return "universal"  # Write-heavy, append-only
    else:
        return "level"  # Default to level-based

# Example
workload = {
    'reads': 700, 
    'writes': 300, 
    'updates': 200, 
    'total_ops': 1000
}
print(recommend_compaction_strategy(workload))  # Output: "level"
```

### Parameter Tuning for Each Strategy

**Level-Based Tuning:**

```yaml
# OLTP (Low Latency)
rocksdb:
  compaction_style: "level"
  level0_file_num_compaction_trigger: 2
  level0_slowdown_writes_trigger: 10
  level0_stop_writes_trigger: 20
  max_background_compactions: 4
  max_subcompactions: 1

# Analytics (High Throughput)
rocksdb:
  compaction_style: "level"
  level0_file_num_compaction_trigger: 8
  level0_slowdown_writes_trigger: 32
  level0_stop_writes_trigger: 48
  max_background_compactions: 16
  max_subcompactions: 4

# Balanced
rocksdb:
  compaction_style: "level"
  level0_file_num_compaction_trigger: 4
  level0_slowdown_writes_trigger: 20
  level0_stop_writes_trigger: 36
  max_background_compactions: 8
  max_subcompactions: 2
```

**Universal Tuning:**

```yaml
# Write-Optimized (Low Write Amplification)
rocksdb:
  compaction_style: "universal"
  universal_compaction_options:
    size_ratio: 2  # Less frequent compaction
    min_merge_width: 2
    max_merge_width: 20  # Compact more files together
    max_size_amplification_percent: 300

# Space-Optimized (Low Space Amplification)
rocksdb:
  compaction_style: "universal"
  universal_compaction_options:
    size_ratio: 1
    min_merge_width: 2
    max_merge_width: 5
    max_size_amplification_percent: 150  # Limit space overhead
```

---

## WAL (Write-Ahead Log) Optimization

### WAL Format Options

**Configuration:**

```yaml
database:
  wal_path: "/var/lib/themisdb/wal"
  enable_wal: true
  
  # WAL sync mode
  wal_sync_mode: "normal"  # Options: none, normal, full
  
  # WAL file size
  wal_max_file_size_mb: 256
  wal_max_total_size_mb: 4096  # 4GB total
  
  # WAL recovery mode
  wal_recovery_mode: "point_in_time"  # or "absolute_consistency"
```

**WAL Sync Modes:**

| Mode | fsync Behavior | Durability | Latency | Throughput |
|------|----------------|------------|---------|------------|
| **none** | Never fsync | ❌ Low | ✅ Lowest | ✅ Highest |
| **normal** | Group commit | ✅ Good | ⚠️ Medium | ✅ High |
| **full** | Every write | ✅ Excellent | ❌ High | ❌ Low |

**Recommendation:**

```yaml
# Production (default)
database:
  wal_sync_mode: "normal"
  # Good balance: ~1ms latency, no data loss on crash

# Development/Testing
database:
  wal_sync_mode: "none"
  # Fastest, but data loss on crash

# Financial/Critical Data
database:
  wal_sync_mode: "full"
  # Slowest, but zero data loss
```

### Durability vs Performance Tradeoffs

**Benchmark Results:**

```python
# Tested on: 16-core CPU, NVMe SSD

sync_modes = {
    'none': {'latency_ms': 0.5, 'throughput_ops': 200000, 'durability': 'crash = data loss'},
    'normal': {'latency_ms': 1.2, 'throughput_ops': 80000, 'durability': 'crash = 0-100ms loss'},
    'full': {'latency_ms': 5.0, 'throughput_ops': 20000, 'durability': 'crash = no loss'},
}

for mode, metrics in sync_modes.items():
    print(f"{mode}: {metrics}")
```

**Configuration Examples:**

```yaml
# Scenario 1: Session storage (can tolerate data loss)
database:
  wal_sync_mode: "none"
  wal_max_file_size_mb: 64

# Scenario 2: E-commerce transactions (balance)
database:
  wal_sync_mode: "normal"
  wal_max_file_size_mb: 256

# Scenario 3: Financial transactions (no loss)
database:
  wal_sync_mode: "full"
  wal_max_file_size_mb: 128
```

### WAL File Management

**Automatic Management:**

```yaml
database:
  # Maximum WAL file size before rotation
  wal_max_file_size_mb: 256
  
  # Maximum total WAL size before archival/deletion
  wal_max_total_size_mb: 4096
  
  # WAL TTL (delete after this time)
  wal_ttl_seconds: 86400  # 24 hours
  
  # WAL size limit (delete when exceeded)
  wal_size_limit_mb: 8192  # 8GB
```

**Manual WAL Management:**

```bash
# List WAL files
themisdb-admin wal list

# Archive old WAL files
themisdb-admin wal archive \
  --destination=/mnt/backup/wal-archive \
  --older-than=1h

# Delete old WAL files (dangerous!)
themisdb-admin wal delete --older-than=24h --force

# Replay WAL for recovery
themisdb-admin wal replay \
  --wal-dir=/var/lib/themisdb/wal \
  --target-sequence=12345678
```

### WAL Recovery Procedures

**Recovery Modes:**

```yaml
database:
  # Point-in-time: Replay to specific sequence number
  wal_recovery_mode: "point_in_time"
  wal_recovery_target_sequence: 12345678
  
  # Absolute consistency: Replay all committed transactions
  # wal_recovery_mode: "absolute_consistency"
  
  # Skip corrupted records: Continue despite errors (risky!)
  # wal_recovery_mode: "skip_corrupted"
```

**Recovery Process:**

```bash
# Step 1: Stop ThemisDB
sudo systemctl stop themisdb

# Step 2: Verify WAL integrity
themisdb-admin wal verify --path=/var/lib/themisdb/wal

# Step 3: Recover to point in time
themisdb-admin recover \
  --wal-dir=/var/lib/themisdb/wal \
  --target-time="2026-01-18 10:30:00" \
  --verify

# Step 4: Start ThemisDB
sudo systemctl start themisdb

# Step 5: Verify database state
themisdb-cli --command="SELECT COUNT(*) FROM entities"
```

---

## Column Family Configuration

### When to Use Column Families

**Use Cases:**

✅ **Good for:**
- Different data types (entities, edges, vectors)
- Different access patterns (hot vs. cold data)
- Different compaction strategies
- Different compression algorithms
- Isolation of workloads

❌ **Avoid for:**
- Small datasets (overhead > benefit)
- Uniform access patterns
- Excessive number of CFs (> 100)

**ThemisDB Column Families:**

```yaml
column_families:
  - name: "default"
    # Metadata and small keys
    
  - name: "entities"
    # Relational entity storage
    
  - name: "edges"
    # Graph edge storage
    
  - name: "vectors"
    # Vector embeddings (large data)
    
  - name: "indexes"
    # Secondary indexes
```

### Per-CF Tuning Options

**Different Configurations per CF:**

```yaml
column_families:
  - name: "entities"
    # Balanced configuration
    memtable_size_mb: 256
    block_cache_size_mb: 4096
    compression: "lz4"
    level0_file_num_compaction_trigger: 4
    
  - name: "edges"
    # Write-optimized
    memtable_size_mb: 512
    block_cache_size_mb: 2048
    compression: "snappy"  # Faster compression
    level0_file_num_compaction_trigger: 8  # Lazy compaction
    
  - name: "vectors"
    # Large value optimization
    memtable_size_mb: 1024
    block_cache_size_mb: 8192
    compression: "zstd"  # Best compression
    enable_blobdb: true  # Store large values separately
    blob_size_threshold: 4096
    
  - name: "indexes"
    # Read-optimized
    memtable_size_mb: 128
    block_cache_size_mb: 4096
    cache_index_and_filter_blocks: true
    bloom_bits_per_key: 15  # Low false positive rate
```

### Separation Strategies (Relational/Graph/Vector)

**Strategy 1: Isolation by Data Type**

```yaml
# Relational data: Frequent small updates
relational_cf:
  memtable_size_mb: 256
  compaction_style: "level"
  target_file_size_base_mb: 64
  
# Graph data: Large range scans
graph_cf:
  memtable_size_mb: 512
  compaction_style: "level"
  target_file_size_base_mb: 128
  optimize_filters_for_range_scans: true
  
# Vector data: Large sequential writes
vector_cf:
  memtable_size_mb: 1024
  compaction_style: "universal"  # Low write amplification
  enable_blobdb: true
```

**Strategy 2: Isolation by Access Pattern**

```yaml
# Hot data: Frequently accessed
hot_cf:
  block_cache_size_mb: 8192
  pin_l0_filter_and_index_blocks_in_cache: true
  bloom_bits_per_key: 10
  
# Cold data: Infrequently accessed
cold_cf:
  block_cache_size_mb: 1024
  compression: "zstd"  # Maximum compression
  bottommost_compression: "zstd"
  level0_file_num_compaction_trigger: 8
```

### Dynamic Column Family Management

**Create CF at Runtime:**

```cpp
// C++ API
rocksdb::ColumnFamilyOptions cf_options;
cf_options.memtable_factory.reset(
    new rocksdb::SkipListFactory()
);
cf_options.write_buffer_size = 256 << 20;  // 256MB

rocksdb::ColumnFamilyHandle* cf_handle;
rocksdb::Status s = db->CreateColumnFamily(
    cf_options, 
    "new_cf", 
    &cf_handle
);
```

**Drop CF at Runtime:**

```cpp
// Drop column family
rocksdb::Status s = db->DropColumnFamily(cf_handle);
delete cf_handle;
```

**List Column Families:**

```bash
# CLI command
themisdb-admin cf list

# Output:
# Column Families:
# - default (size: 1.2GB, files: 45)
# - entities (size: 128GB, files: 2048)
# - edges (size: 256GB, files: 4096)
# - vectors (size: 512GB, files: 8192)
```

---

## Compression Configuration

### Algorithm Selection (LZ4 vs ZSTD)

**Comparison:**

| Algorithm | Compression Ratio | Compression Speed | Decompression Speed | CPU Usage | Use Case |
|-----------|-------------------|-------------------|---------------------|-----------|----------|
| **None** | 1.0x | N/A | N/A | None | Development only |
| **Snappy** | 2-3x | ✅ Very Fast | ✅ Very Fast | Low | Legacy/compatibility |
| **LZ4** | 2-3x | ✅ Very Fast | ✅ Very Fast | Low | Hot data, OLTP |
| **ZSTD** | 3-5x | ⚠️ Medium | ✅ Fast | Medium | Cold data, analytics |
| **ZLIB** | 3-4x | ❌ Slow | ⚠️ Medium | High | Rarely used |

**Benchmark Results:**

```python
# Tested on 1GB dataset, 16-core CPU

algorithms = {
    'none': {
        'compression_ratio': 1.0,
        'compression_mb_per_sec': float('inf'),
        'decompression_mb_per_sec': float('inf'),
        'disk_size_gb': 1.0
    },
    'lz4': {
        'compression_ratio': 2.5,
        'compression_mb_per_sec': 500,
        'decompression_mb_per_sec': 2000,
        'disk_size_gb': 0.4
    },
    'zstd': {
        'compression_ratio': 4.0,
        'compression_mb_per_sec': 150,
        'decompression_mb_per_sec': 800,
        'disk_size_gb': 0.25
    },
}

for algo, metrics in algorithms.items():
    print(f"{algo}: {metrics}")
```

**Configuration:**

```yaml
rocksdb:
  # Default compression for most levels
  compression: "lz4"
  
  # Aggressive compression for cold data (bottommost level)
  bottommost_compression: "zstd"
  
  # Per-level compression
  compression_per_level:
    - "none"  # L0: No compression (hot data)
    - "lz4"   # L1: Fast compression
    - "lz4"   # L2: Fast compression
    - "lz4"   # L3: Fast compression
    - "zstd"  # L4: Better compression
    - "zstd"  # L5: Better compression
    - "zstd"  # L6: Better compression (cold data)
```

### Compression Levels

**ZSTD Compression Levels:**

| Level | Compression Ratio | Speed | CPU Usage | Use Case |
|-------|-------------------|-------|-----------|----------|
| 1 | 3.0x | Fast | Low | Default |
| 3 | 3.5x | Medium | Medium | Balanced |
| 6 | 4.0x | Slow | High | Cold data |
| 9 | 4.5x | Very Slow | Very High | Archival |

**Configuration:**

```yaml
rocksdb:
  compression: "zstd"
  zstd_compression_level: 3  # Balanced
  
  # Bottommost level: Maximum compression
  bottommost_compression: "zstd"
  zstd_bottommost_compression_level: 6
  
  # Enable compression dictionary
  enable_compression_dict: true
  max_dict_bytes: 16384  # 16KB dictionary
```

### Performance Impact Analysis

**Latency Impact:**

```python
# Read latency by compression algorithm
latencies_ms = {
    'none': {'p50': 0.5, 'p99': 2.0, 'p999': 10.0},
    'lz4': {'p50': 0.6, 'p99': 2.5, 'p999': 12.0},
    'zstd_level1': {'p50': 0.7, 'p99': 3.0, 'p999': 15.0},
    'zstd_level3': {'p50': 0.9, 'p99': 4.0, 'p999': 20.0},
}

# LZ4 adds ~20% latency, ZSTD adds ~40-80%
```

**CPU Impact:**

```yaml
# Monitor CPU usage by compression
# LZ4: +5-10% CPU (compression) + +1-2% CPU (decompression)
# ZSTD: +15-30% CPU (compression) + +3-5% CPU (decompression)

# Mitigate with dedicated compaction threads
rocksdb:
  max_background_compactions: 8  # Isolate compression overhead
```

### Storage Savings Estimation

**Estimation Formula:**

```python
def estimate_storage_savings(
    raw_data_size_gb: float,
    compression_algorithm: str
) -> dict:
    """
    Estimate storage savings with compression.
    """
    ratios = {
        'none': 1.0,
        'snappy': 2.5,
        'lz4': 2.5,
        'zstd_level1': 3.0,
        'zstd_level3': 4.0,
        'zstd_level6': 4.5,
    }
    
    ratio = ratios.get(compression_algorithm, 1.0)
    compressed_size_gb = raw_data_size_gb / ratio
    savings_gb = raw_data_size_gb - compressed_size_gb
    savings_percent = (savings_gb / raw_data_size_gb) * 100
    
    return {
        'compressed_size_gb': compressed_size_gb,
        'savings_gb': savings_gb,
        'savings_percent': savings_percent,
    }

# Example: 1TB dataset with ZSTD
result = estimate_storage_savings(1000, 'zstd_level3')
print(result)
# Output: {'compressed_size_gb': 250, 'savings_gb': 750, 'savings_percent': 75.0}
```

---

## Performance Monitoring

### Key RocksDB Metrics

**Essential Metrics:**

```prometheus
# Compaction metrics
themisdb_rocksdb_compaction_pending
themisdb_rocksdb_compaction_running
themisdb_rocksdb_compaction_duration_seconds

# Level metrics
themisdb_rocksdb_level_files{level="0"}
themisdb_rocksdb_level_size_bytes{level="0"}

# Write metrics
themisdb_rocksdb_memtable_flush_pending
themisdb_rocksdb_write_stall_seconds_total

# Read metrics
themisdb_rocksdb_block_cache_hit_ratio
themisdb_rocksdb_bloom_filter_useful_ratio

# WAL metrics
themisdb_rocksdb_wal_files_total
themisdb_rocksdb_wal_size_bytes
```

**Collection:**

```bash
# Prometheus scrape
curl http://localhost:9091/metrics | grep rocksdb_

# RocksDB internal statistics
themisdb-admin stats rocksdb --format=json > rocksdb_stats.json
```

### Interpreting Statistics Output

**Key Statistics:**

```bash
# Get RocksDB statistics
themisdb-admin stats rocksdb

# Output interpretation:
# ** Compaction Stats **
# Level    Files   Size     Score  Read(MB)  Write(MB)  Rn(MB)  Rnp1(MB)  Wnew(MB)
# L0       8/4     512MB    2.0    0         512        0       0         512
# ^ 8 files in L0, trigger at 4 files, needs compaction (score > 1.0)

# L1       40      2.5GB    1.0    512       512        512     0         0
# ^ 40 files in L1, 2.5GB total, score 1.0 (at target size)

# ** Memtable Stats **
# MemTable: 256MB active, 2x 256MB immutable
# ^ 768MB total memtable usage

# ** Block Cache Stats **
# Hit ratio: 96.5%  (excellent)
# Miss ratio: 3.5%

# ** Write Stalls **
# Level0 slowdown: 3 times, 15 seconds total
# ^ Writes slowed down 3 times due to L0 file accumulation
```

**Alerts:**

```yaml
# Grafana alerts
- alert: HighL0Files
  expr: themisdb_rocksdb_level_files{level="0"} > 10
  annotations:
    summary: "Too many L0 files, compaction falling behind"

- alert: WriteStall
  expr: rate(themisdb_rocksdb_write_stall_seconds_total[5m]) > 0.1
  annotations:
    summary: "Write stalls detected"

- alert: LowBlockCacheHitRate
  expr: themisdb_rocksdb_block_cache_hit_ratio < 0.90
  annotations:
    summary: "Block cache hit rate below 90%"
```

### Identifying Bottlenecks

**Compaction Bottleneck:**

```bash
# Symptoms:
# - High L0 file count (> 10)
# - Write stalls
# - Slow writes

# Diagnosis:
curl http://localhost:9091/metrics | grep compaction_pending
# themisdb_rocksdb_compaction_pending 5  # Compaction backlog

# Solution:
cat > config.yaml <<EOF
rocksdb:
  max_background_compactions: 16  # Increase from 8
  max_subcompactions: 4  # Parallel sub-compactions
EOF
```

**Memory Bottleneck:**

```bash
# Symptoms:
# - Low block cache hit rate (< 90%)
# - High disk I/O
# - Slow reads

# Diagnosis:
curl http://localhost:9091/metrics | grep block_cache_hit_ratio
# themisdb_rocksdb_block_cache_hit_ratio 0.75  # Too low

# Solution:
cat > config.yaml <<EOF
rocksdb:
  block_cache_size_mb: 16384  # Increase from 8192
EOF
```

**Disk I/O Bottleneck:**

```bash
# Symptoms:
# - High disk utilization (> 80%)
# - Slow reads and writes
# - High latency

# Diagnosis:
iostat -x 5

# Output:
# Device: %util
# sda:    95%  # Disk saturated!

# Solutions:
# 1. Use faster SSDs (NVMe)
# 2. Distribute load across multiple disks
# 3. Enable direct I/O
cat > config.yaml <<EOF
rocksdb:
  use_direct_reads: true
  use_direct_io_for_flush_and_compaction: true
EOF
```

### Performance Regression Detection

**Baseline Measurement:**

```bash
# Establish baseline
themisdb-benchmark \
  --workload=ycsb_workloada \
  --duration=300 \
  --threads=16 \
  --output=baseline_$(date +%Y%m%d).json

# Store baseline metrics
jq '.latency.p99_ms' baseline_20260118.json
# 8.5ms
```

**Regression Detection:**

```python
import json

def detect_regression(baseline_file, current_file, threshold=1.2):
    """
    Detect performance regression.
    """
    with open(baseline_file) as f:
        baseline = json.load(f)
    
    with open(current_file) as f:
        current = json.load(f)
    
    baseline_p99 = baseline['latency']['p99_ms']
    current_p99 = current['latency']['p99_ms']
    
    if current_p99 > baseline_p99 * threshold:
        print(f"REGRESSION DETECTED!")
        print(f"Baseline p99: {baseline_p99}ms")
        print(f"Current p99: {current_p99}ms")
        print(f"Increase: {(current_p99 / baseline_p99 - 1) * 100:.1f}%")
        return True
    else:
        print("No regression detected")
        return False

# Run regression check
detect_regression('baseline_20260118.json', 'current_20260119.json')
```

---

## Related Documentation

- [Production Deployment Guide](../deployment/PRODUCTION_DEPLOYMENT_GUIDE.md)
- [MVCC Tuning Guide](../features/MVCC_TUNING_GUIDE.md)
- [Transaction Best Practices](../features/TRANSACTION_BEST_PRACTICES.md)
- [Operational Procedures](../operations/OPERATIONAL_PROCEDURES.md)
- [Monitoring Setup Guide](../operations/MONITORING_SETUP_GUIDE.md)
- [Troubleshooting Guide](../operations/TROUBLESHOOTING_GUIDE.md)

---

**Document Version:** 1.0  
**ThemisDB Compatibility:** 1.4.0+  
**Last Reviewed:** 2026-01-18
