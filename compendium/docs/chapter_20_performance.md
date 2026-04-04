# Kapitel 20: Performance Tuning

> **Zusammenfassung:** Performance-Optimierung in ThemisDB erfordert systematisches Tuning von RocksDB, Ingestion-Pipelines, Hardware-Ressourcen und Query-Strategien. Wissenschaftliche Benchmarks zeigen 85.7% Scaling-Effizienz bei 10 Cores und bis zu 250% Throughput-Steigerung durch gezielte Optimierungen.
>
> **Voraussetzungen:** [Kapitel 2: Architektur](chapter_02_architecture.md), [Kapitel 8: Storage Layer](chapter_08_storage_layer.md)
>
> **Lernziele:**
> - RocksDB Memory-Hierarchie und Compaction optimieren
> - Ingestion-Throughput durch Batching und Compression steigern
> - Hardware-spezifische Tuning-Strategien anwenden
> - Query-Performance durch Indexierung und Caching verbessern
> - Systematische Benchmarking-Methodologien nutzen

---

## 20.1 Performance Baselines

### Benchmark-Umgebung (v1.3.0)

**Hardware-Profile:**
- **CPU**: 10 Cores @ 3.7 GHz
- **RAM**: 64 GB
- **Storage**: NVMe SSD, 100K IOPS, 3.0 GB/sec bandwidth
- **L3 Cache**: 16 MB per core

**Key Metrics:**
| Operation | Throughput | P50/P99/P999 Latency |
|-----------|-----------|----------------------|
| Random Read | 1.2M ops/sec | 6.0µs / 55µs / 550µs |
| Random Write | 450K ops/sec | 11µs / 110µs / 1.1ms |
| Sequential Scan | 1.8 GB/sec | ~90% RocksDB baseline |
| YCSB Mixed (A) | 500K ops/sec | 2.5ms P99 |
| YCSB Read-heavy (C) | 2M ops/sec | 1.2ms P99 |
| TPC-C OLTP | 50K TPMC | <10ms P99 |
| TPC-H OLAP | 15.8K QPhH | 5.2s avg |

**Scaling Efficiency:** 85.7% across 10 cores (excellent, above theoretical 75%)

---

## 20.2 RocksDB Optimization

### Memory Hierarchy Tuning

**Block Cache Configuration:**
```cpp
rocksdb::BlockBasedTableOptions table_options;
table_options.block_cache = rocksdb::NewLRUCache(4 * 1024 * 1024 * 1024);  // 4 GB
table_options.cache_index_and_filter_blocks = true;
table_options.pin_l0_filter_and_index_blocks_in_cache = true;
table_options.high_pri_pool_ratio = 0.5;  // Index/filter priority
table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10));  // 10 bits/key
```

**Write Buffer (Memtable) Sizing:**
| System RAM | Memtable Size | Max Buffers | Total | Expected Gain |
|-----------|--------------|-------------|-------|---------------|
| 8 GB | 128 MB | 3 | 384 MB | Baseline |
| 16 GB | 256 MB | 4 | 1 GB | +25% |
| 32 GB | 512 MB | 6 | 3 GB | +50% |
| 64 GB | 1 GB | 6 | 6 GB | +100% |

```cpp
options.write_buffer_size = 1024 * 1024 * 1024;  // 1 GB
options.max_write_buffer_number = 6;
options.db_write_buffer_size = 6 * 1024 * 1024 * 1024;  // 6 GB total
```

### Compression Strategy

**Hybrid Compression (Recommended):**
```cpp
options.compression = rocksdb::kLZ4Compression;  // L0-L5: Fast
options.bottommost_compression = rocksdb::kZSTD;  // L6+: Space-efficient
options.compression_opts.level = 3;  // Balanced
```

**Benchmarks:**
| Algorithm | Ratio | Write Speed | Read Speed | Use Case |
|-----------|-------|-------------|-----------|----------|
| None | 1.0x | 34.5 MB/s | 125 MB/s | Large BLOBs (>8KB) |
| LZ4 | 2-3x | 24.1 MB/s | 118 MB/s | ✅ Default |
| ZSTD | 3-5x | 25.6 MB/s | 112 MB/s | ✅ Bottommost |

**Impact:** Hybrid config reduces DB size 2.4-2.9× with <10% performance penalty

### Compaction Tuning

**Aggressive L0 Compaction (High-Throughput Ingestion):**
```cpp
options.level0_file_num_compaction_trigger = 2;  // Start earlier (default 4)
options.level0_slowdown_writes_trigger = 8;      // Slow down threshold
options.level0_stop_writes_trigger = 16;         // Stop threshold
options.max_background_compactions = 8;          // Parallel threads
options.max_subcompactions = 2;                  // Sub-parallel
```

**Impact:** Eliminates write stalls, -50-70% P99 latency (+15-25% CPU cost)

### WAL Optimization

**Separate Storage Architecture:**
```
/nvme0/      → Main database (db_path)
/nvme1/      → WAL (wal_dir) — sequential I/O only
/nvme2, /nvme3/ → SSTables (db_paths) — random/sequential I/O
```

**Configuration:**
```cpp
options.wal_dir = "/nvme1/themisdb/wal";
options.use_direct_io_for_flush_and_compaction = true;
options.wal_bytes_per_sync = 64 * 1024 * 1024;  // 64 MB group commit
```

**Impact:** 30-50% write latency reduction

---

## 20.3 Ingestion Optimization

### Multi-Layer Optimization Stack

```
Client → Connection Pooling → HTTP/2 Multiplexing → Auto-Batching → 
Payload Compression → Server Auto-Buffers → RocksDB → Disk I/O
```

### Quick Wins (Phase 1, Week 1-2)

**1. HTTP/2 Multiplexing (+50-100% throughput, 1 day)**
```yaml
server:
  enable_http2: true
  http2_max_concurrent_streams: 1000
```

**2. Adaptive Write Buffer (+40-60% throughput, 2 days)**
```cpp
config.write_buffer_size = calculate_optimal_size(available_ram);
```

**3. Level0 Compaction Tuning (-50-70% P99 latency, 2 days)**
```cpp
options.level0_file_num_compaction_trigger = 2;
```

**4. Payload Compression (-70-85% network traffic, 3 days)**
```yaml
server:
  compression_type: zstd
  compression_level: 3
```

**5. Connection Pooling (+30-50% throughput, 2 days)**
```cpp
pool_size = num_cores * 10;  // Rule of thumb
```

### Time Series Example

**Before Optimization:**
- Latency: 50ms per point
- Throughput: 20 points/sec
- Protocol: HTTP/1.1 + JSON

**After Phase 1:**
- Latency: 0.5ms per point (**100× faster**)
- Throughput: 2,000 points/sec (**100× higher**)
- Network: 229 bytes → 0.087 bytes (**2,600× less**)

---

## 20.4 Query Optimization

### Pagination Strategies

**Cursor-Based (Recommended for large datasets):**
```sql
-- Cost: O(log n) with binary search
SELECT * FROM entities 
WHERE (entity_id > :cursor_pk) 
  OR (entity_id = :cursor_pk AND primary_key > :cursor_value)
LIMIT :count + 1
```

**vs. Offset-Based (Avoid for large offsets):**
```sql
-- Cost: O(n) where n = offset value
SELECT * FROM entities LIMIT :count OFFSET :offset
```

### Index Selection

**B-Tree vs Hash Index:**
| Index Type | Point Lookup | Range Scan | Memory | Use Case |
|-----------|--------------|-----------|--------|----------|
| Hash | O(1) | ❌ No | Low | Exact matches |
| B-Tree | O(log n) | ✅ Yes | Medium | Range queries |
| LSM-Tree | O(log n) | ✅ Yes | High | Write-heavy |

### Vector Search Tuning

**HNSW Parameters:**
```cpp
hnsw_m = 16                      // Neighborhood degree
hnsw_ef_construction = 200       // Build quality
efSearch = 32-128                // Runtime (tune per SLO)
```

**Performance Profile:**
- Insert: 500+ vectors/sec
- Search: 1-10 ms for top-K on 1M vectors
- Memory: ~3.2 KB per vector

---

## 20.5 Configuration Templates

### Standard (16GB RAM)
```yaml
storage:
  memtable_size_mb: 256
  max_write_buffer_number: 4
  db_write_buffer_size_mb: 1024
  max_background_jobs: 4
  compression_default: "lz4"
  compression_bottommost: "zstd"

server:
  enable_http2: true
  num_threads: 8
```

### High-Throughput Ingestion (64GB RAM)
```yaml
storage:
  memtable_size_mb: 1024          # 4× larger
  max_write_buffer_number: 6
  db_write_buffer_size_mb: 6144   # 6× total
  max_background_jobs: 16
  max_background_compactions: 8
  level0_file_num_compaction_trigger: 2
  wal_dir: /nvme/rocksdb/wal
  use_direct_io_for_flush_and_compaction: true

server:
  enable_http2: true
  http2_max_concurrent_streams: 1000
  compression_type: zstd
```

### Bulk Import
```yaml
storage:
  memtable_size_mb: 2048
  max_write_buffer_number: 8
  max_background_compactions: 12
  use_direct_io_for_flush_and_compaction: true
  sync: false                     # ⚠️ CAUTION: data loss risk
  wal_bytes_per_sync: 67108864    # 64 MB group commit
```

---

## 20.6 Monitoring

### Key Prometheus Metrics

```promql
# Write throughput
rate(rocksdb_writes_total[1m])

# P99 write latency
histogram_quantile(0.99, rate(rocksdb_write_latency_bucket[1m]))

# Level0 file count (should stay low, <8)
rocksdb_level0_files

# Write stalls (should be zero)
rate(rocksdb_write_stalls_total[1m])

# Compaction activity
rate(rocksdb_compaction_duration_micros[1m])
```

---

## 20.7 Zusammenfassung

### Performance Grading

| Component | Score | Grade | Status |
|-----------|-------|-------|--------|
| Read Performance | 60% | 🟠 C | Optimization needed |
| Write Performance | 90% | 🟡 B+ | Good |
| Scan Performance | 90% | 🟡 B+ | Good |
| Scaling Efficiency | 85.7% | 🟡 B | Excellent |
| **Overall** | **75%** | **🟡 B** | **Production Ready** |

### Optimization Roadmap

**Phase 1: Foundation (Week 1-2, +150-250% throughput)**
- HTTP/2 multiplexing
- Adaptive write buffer sizing
- Level0 compaction tuning

**Phase 2: Scale (Month 1-2, +200-500% specific workloads)**
- Product quantization for embeddings
- Memory-mapped bulk import
- Direct I/O optimization

### Weiterführende Ressourcen

- **Horizontal Scaling**: [Kapitel 17: Horizontal Scaling](chapter_17_scaling.md)
- **Monitoring**: [Kapitel 19: Observability](chapter_19_monitoring.md)

**Externe Quellen:**
- [RocksDB Tuning Guide](https://github.com/facebook/rocksdb/wiki/RocksDB-Tuning-Guide)
- [YCSB Benchmark](https://github.com/brianfrankcooper/YCSB)

---

**Nächstes Kapitel:** [Kapitel 21: Authentication](chapter_21_auth.md)  
**Vorheriges Kapitel:** [Kapitel 19: Monitoring](chapter_19_monitoring.md)
