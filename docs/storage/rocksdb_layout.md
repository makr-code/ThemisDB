# RocksDB Storage Layout

**Version:** 1.0.0  
**Date:** February 9, 2026  
**Category:** 💾 Storage

---

## Overview

This document describes the physical storage layout and key organization in ThemisDB's RocksDB-based storage engine.

## Key Prefixes & Namespaces

ThemisDB uses a hierarchical prefix scheme to logically separate different data types within RocksDB. This enables efficient range scans and compaction strategies.

### Entity Storage

Primary entity data storage:

```
entity:<table>:<pk> → <BaseEntity serialized blob>
```

**Example:**
```
entity:users:12345 → {id: 12345, name: "Alice", email: "alice@example.com"}
```

### Secondary Indexes

Standard secondary indexes for efficient lookups:

```
idx:<table>:<column>:<value>:<pk> → (empty or minimal metadata)
```

**Example:**
```
idx:users:email:alice@example.com:12345 → ""
```

### Range Indexes

Sorted indexes for range queries:

```
ridx:<table>:<column>:<value>:<pk> → (empty or minimal metadata)
```

**Example:**
```
ridx:orders:created_at:20260209:1001 → ""
```

### Sparse Indexes

Indexes for fields with many null values:

```
sidx:<table>:<column>:<value>:<pk> → (empty)
```

### TTL Indexes

Time-to-live indexes for automatic expiration:

```
ttlidx:<table>:<expiry_timestamp>:<pk> → (empty)
```

### Full-Text Indexes

Inverted indexes for full-text search:

```
ftidx:<table>:<column>:<term>:<pk> → (relevance score)
```

### Graph Adjacency

Bi-directional edge storage for graph queries:

**Outgoing edges:**
```
graph:out:<from_pk>:<edge_id> → <to_pk>
```

**Incoming edges:**
```
graph:in:<to_pk>:<edge_id> → <from_pk>
```

**Example:**
```
graph:out:user123:follows456 → user789
graph:in:user789:follows456 → user123
```

### Vector Indexes

Metadata for vector embeddings (actual vectors stored in specialized index structures):

```
vector:<table>:<pk> → {dimension, norm, metadata}
```

### Change Feed

Event log for change data capture:

```
changefeed:<sequence> → {operation, table, key, old_value, new_value, timestamp}
```

### Time-Series Data

Optimized storage for time-series metrics:

```
ts:<metric>:<timestamp>:<tags> → <value(s)>
```

**Example:**
```
ts:cpu_usage:1707465600:host=server1 → 45.2
```

## Column Families

ThemisDB uses RocksDB column families to physically separate different data types, enabling independent tuning and compaction strategies.

### Default Column Family

- Used for entity storage and general-purpose data
- Default configuration suitable for mixed workloads

### Specialized Column Families (Optional)

For large workloads, data can be separated into dedicated column families:

- **`cf_entities`** - Primary entity storage
- **`cf_indexes`** - Secondary, range, sparse, and full-text indexes
- **`cf_graph`** - Graph adjacency lists
- **`cf_changefeed`** - Change data capture events
- **`cf_ts`** - Time-series data
- **`cf_vector`** - Vector index metadata

**Benefits:**
- Independent LSM compaction per data type
- Optimized block cache allocation
- Separate bloom filter tuning
- Isolated backup/restore operations

## Write-Ahead Log (WAL)

The WAL ensures durability and crash recovery:

### WAL Structure

- Sequential append-only log file
- Each write batch is logged atomically
- Automatically synced based on durability settings

### WAL Configuration

```cpp
WriteOptions write_options;
write_options.sync = true;  // fsync for maximum durability
write_options.disableWAL = false;  // enable WAL
```

### WAL Management

- **Automatic pruning**: WAL files are deleted after data is flushed to SST files
- **Manual checkpointing**: Force WAL rotation and cleanup
- **Size limits**: `max_total_wal_size` prevents unbounded growth

## Snapshots & MVCC

### Snapshot Isolation

RocksDB snapshots provide consistent point-in-time views:

```cpp
auto snapshot = db->GetSnapshot();
ReadOptions read_opts;
read_opts.snapshot = snapshot;

// Read operations see data as of snapshot creation time
auto result = db->Get(read_opts, key, &value);

db->ReleaseSnapshot(snapshot);
```

### MVCC Implementation

- Each version includes a sequence number
- Readers use snapshots to see consistent views
- Writers create new versions without blocking readers
- Old versions are cleaned up during compaction

## Compaction Strategies

### Level-Based Compaction (Default)

- LSM-tree structure with multiple levels
- Level 0: Unsorted, newly flushed files
- Level 1+: Sorted, non-overlapping files
- Optimized for write-heavy workloads

### Universal Compaction

- Alternative strategy for read-heavy workloads
- All files maintained at same level
- Periodic full compaction

### FIFO Compaction

- For time-series data with automatic expiration
- Old files deleted based on time or size
- Minimal overhead

## Monitoring & Tuning

### Key Metrics

- **Compaction stats**: Throughput, amplification
- **Block cache hit rate**: Read performance indicator
- **Write amplification**: Write efficiency measure
- **Stall events**: Resource contention warnings

### Performance Tuning

See also: [RocksDB Optimization Guide](../en/storage/ROCKSDB_OPTIMIZATION_GUIDE.md)

**Common optimizations:**
- Increase block cache size for read-heavy workloads
- Tune bloom filter bits for point lookups
- Adjust compaction threads for I/O saturation
- Configure rate limiting for background operations

## See Also

- [RocksDB Wrapper Documentation](../de/src/storage/rocksdb_wrapper.cpp.md)
- [RocksDB Storage Operations (DE)](../de/storage/storage_rocksdb.md)
- [KeySchema Documentation](../de/src/storage/key_schema.cpp.md)
- [Storage Module Overview](README.md)

---

**Version:** 1.0.0 | **License:** MIT | **Support:** [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
