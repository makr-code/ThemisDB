# ThemisDB Ingestion Architecture & Optimization Layers

## 🏗️ Complete Ingestion Stack

```
┌─────────────────────────────────────────────────────────────────────┐
│                         CLIENT APPLICATIONS                         │
│  (Python, JavaScript, Ruby, PHP, Rust, Go, Java, .NET SDKs)       │
└────────────────────────┬────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────────┐
│                    CLIENT-SIDE OPTIMIZATIONS                        │
│                                                                     │
│  💡 Optimization Ideas:                                            │
│  ✅ Connection Pooling (100 connections)                           │
│  ✅ Auto-Batching (1000 ops → 1 request)                          │
│  ✅ Payload Compression (Zstd: -70%)                              │
│  ✅ HTTP/2 Multiplexing (10-100× concurrent)                      │
│  ⚡ Impact: +500-1000% throughput                                 │
└────────────────────────┬────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────────┐
│                    NETWORK LAYER                                    │
│                                                                     │
│  Protocol Options:                                                 │
│  1. HTTP/1.1 + JSON     (Baseline: 100%)                          │
│  2. HTTP/2 + JSON       (+50% throughput)                         │
│  3. HTTP/2 + MessagePack (+140% throughput)                       │
│  4. Binary/MessagePack   (+200% throughput)                       │
│  5. gRPC/Protobuf       (+250% throughput)                        │
│                                                                     │
│  💡 Optimization Ideas:                                            │
│  ✅ Enable HTTP/2                                                  │
│  ✅ Content-Encoding: zstd                                         │
│  ✅ Zero-Copy Transfers                                            │
│  ⚡ Impact: +100-300% throughput, -40-60% latency                │
└────────────────────────┬────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────────┐
│                    SERVER-SIDE BUFFERS (Layer 1)                    │
│                                                                     │
│  Auto-Buffer Components:                                           │
│  ┌──────────────────┬──────────────────┬─────────────────────┐    │
│  │  TSAutoBuffer    │ VectorAutoBuffer │  GraphAutoBuffer    │    │
│  │  (Time Series)   │  (Embeddings)    │  (Nodes/Edges)      │    │
│  ├──────────────────┼──────────────────┼─────────────────────┤    │
│  │ ✅ Implemented   │ ✅ Implemented   │ ✅ Implemented      │    │
│  │ 10-50× throughput│ 10-50× throughput│ 2-5× throughput     │    │
│  │ Gorilla compress │ PQ compress (?)  │ Zstd compress       │    │
│  └──────────────────┴──────────────────┴─────────────────────┘    │
│                                                                     │
│  💡 Optimization Ideas:                                            │
│  ✅ Adaptive Batch Sizing (10-10000 items)                        │
│  ✅ Priority Queues (Critical/High/Normal/Low)                    │
│  ✅ Multi-Threshold Flush (Size/Time/Memory)                      │
│  ⚡ Impact: +15-25% throughput, -20-30% P99 latency              │
└────────────────────────┬────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────────┐
│                    ROCKSDB WRITE PATH (Layer 2)                     │
│                                                                     │
│  Write Flow:                                                       │
│  ┌─────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐    │
│  │ Write   │───▶│ Memtable │───▶│   WAL    │───▶│  Commit  │    │
│  │ Batch   │    │ (Active) │    │  (Sync)  │    │          │    │
│  └─────────┘    └──────────┘    └──────────┘    └──────────┘    │
│                       │                                            │
│                       ▼                                            │
│                  ┌──────────┐                                      │
│                  │ Memtable │  (Immutable)                         │
│                  │ (Frozen) │                                      │
│                  └──────────┘                                      │
│                       │                                            │
│                       ▼                                            │
│                  Flush to L0 SST Files                             │
│                                                                     │
│  💡 Optimization Ideas:                                            │
│  ✅ Adaptive Write Buffer Sizing                                   │
│     • 16GB RAM: 256MB memtable                                    │
│     • 64GB RAM: 1GB memtable (+4×)                                │
│     • 128GB RAM: 2GB memtable (+8×)                               │
│  ✅ Parallel Memtable Writes                                       │
│     • allow_concurrent_memtable_write: true                       │
│     • enable_pipelined_write: true                                │
│     • write_policy: WritePrepared                                 │
│  ✅ Group Commit (Async WAL)                                       │
│     • sync: false (⚠️ data loss risk)                             │
│     • wal_bytes_per_sync: 64MB                                    │
│  ⚡ Impact: +40-60% write throughput                              │
└────────────────────────┬────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────────┐
│                    COMPACTION & STORAGE (Layer 3)                   │
│                                                                     │
│  LSM-Tree Levels:                                                  │
│  Level 0 (L0):  [SST][SST][SST][SST]  ← Memtable Flushes         │
│  Level 1 (L1):  [SST][SST][SST][SST][SST][SST]                    │
│  Level 2 (L2):  [SST] ... (10× L1)                                │
│  Level 3 (L3):  [SST] ... (10× L2)                                │
│  Level 4 (L4):  [SST] ... (10× L3)                                │
│  Level 5 (L5):  [SST] ... (10× L4)                                │
│  Level 6 (L6):  [SST] ... (Bottommost)                            │
│                                                                     │
│  Compaction Flow:                                                  │
│  L0 → L1 → L2 → L3 → L4 → L5 → L6                                 │
│   ↓    ↓    ↓    ↓    ↓    ↓    ↓                                │
│  Merge + Sort + Compress                                           │
│                                                                     │
│  💡 Optimization Ideas:                                            │
│  ✅ Aggressive Level0 Compaction                                   │
│     • level0_file_num_compaction_trigger: 2 (default: 4)         │
│     • level0_slowdown_writes_trigger: 8 (default: 20)            │
│     • level0_stop_writes_trigger: 16 (default: 36)               │
│     • max_background_compactions: 8 (default: 4)                 │
│     • max_subcompactions: 2 (parallel)                            │
│  ✅ Parallel Compression                                           │
│     • compression_opts.parallel_threads: 8                        │
│     • compression: zstd (level 3)                                 │
│  ✅ Tiered Storage                                                 │
│     • Hot data (L0-L2): NVMe SSD                                  │
│     • Warm data (L3-L4): SATA SSD                                 │
│     • Cold data (L5-L6): HDD                                      │
│  ⚡ Impact: -50-70% P99 latency, +20-40% sustained throughput    │
└────────────────────────┬────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────────┐
│                    DISK I/O (Layer 4)                               │
│                                                                     │
│  Storage Layout:                                                   │
│  /nvme/rocksdb/wal/          ← WAL files (low latency)            │
│  /nvme/rocksdb/hot/          ← L0-L2 SST files                    │
│  /ssd/rocksdb/warm/          ← L3-L4 SST files                    │
│  /data/rocksdb/cold/         ← L5-L6 SST files                    │
│                                                                     │
│  💡 Optimization Ideas:                                            │
│  ✅ Separate WAL Directory (NVMe)                                  │
│     • wal_dir: /nvme/rocksdb/wal                                  │
│     • -30-50% write latency                                       │
│  ✅ Direct I/O (bypass OS cache)                                   │
│     • use_direct_io_for_flush_and_compaction: true               │
│     • +30-50% bulk import speed                                   │
│  ✅ Async I/O with Prefetching                                     │
│     • enable_async_io: true                                       │
│     • async_io_readahead_size_mb: 64                              │
│     • +20-40% read performance                                    │
│  ✅ Memory-Mapped File Import                                      │
│     • Zero-copy, lazy loading                                     │
│     • +100-300% for large files                                   │
│  ⚡ Impact: -30-50% write latency, +50-100% bulk import          │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 📊 Performance Impact Summary

### Quick Wins (Week 1-2)
| Layer | Optimization | Impact | Effort |
|-------|--------------|--------|--------|
| Network | HTTP/2 | +50-100% throughput | 1 day |
| Client | Connection Pooling | +50-100% throughput | 2 days |
| Client | Payload Compression | -70% traffic | 3 days |
| RocksDB | Adaptive Write Buffer | +40-60% throughput | 2 days |
| RocksDB | Level0 Compaction Tuning | -50-70% P99 latency | 2 days |
| **Total** | **Combined** | **+150-250% throughput** | **10 days** |

### Medium-term (Month 1-2)
| Layer | Optimization | Impact | Effort |
|-------|--------------|--------|--------|
| Server | Product Quantization | -90-97% storage | 2 weeks |
| Disk I/O | Memory-Mapped Import | +100-300% bulk | 1 week |
| Disk I/O | Direct I/O | +30-50% bulk | 1 week |
| RocksDB | Per-Thread Memtables | +150-250% (@64 threads) | 1 month |
| **Total** | **Combined** | **+200-500% specific** | **2 months** |

---

## 🎯 Optimization Priority Matrix

```
High Impact │
            │  ┌─HTTP/2──────┐  ┌─Adaptive WB─┐  ┌─Level0 Tune┐
            │  │             │  │             │  │             │
            │  │  ⭐⭐⭐⭐⭐  │  │  ⭐⭐⭐⭐⭐  │  │  ⭐⭐⭐⭐⭐  │
            │  └─1 day──────┘  └─2 days─────┘  └─2 days─────┘
            │
            │  ┌─Compression─┐  ┌─Connection──┐
            │  │             │  │  Pooling    │
            │  │  ⭐⭐⭐⭐    │  │  ⭐⭐⭐⭐⭐  │
            │  └─3 days─────┘  └─2 days─────┘
            │
Medium      │  ┌─Product PQ──┐  ┌─MMap Import┐
Impact      │  │             │  │             │
            │  │  ⭐⭐⭐⭐    │  │  ⭐⭐⭐⭐    │
            │  └─2 weeks────┘  └─1 week─────┘
            │
            │  ┌─Adaptive────┐  ┌─Direct I/O──┐
            │  │  Batching   │  │             │
            │  │  ⭐⭐⭐      │  │  ⭐⭐⭐      │
Low Impact  │  └─1 month────┘  └─1 week─────┘
            │
            └────────────────────────────────────────────────────▶
              Low Effort        Medium          High Effort
```

**Priority Ranking:**
1. ⭐⭐⭐⭐⭐ HTTP/2, Adaptive Write Buffer, Level0 Tuning
2. ⭐⭐⭐⭐ Compression, Connection Pooling, Product Quantization
3. ⭐⭐⭐ Adaptive Batching, Direct I/O

---

## 🔄 Data Flow Example: Time Series Ingestion

### Before Optimization
```
Client (Python SDK)
  │
  ├─ POST /ts/put (single point, HTTP/1.1, JSON)
  │  Latency: 50ms, Size: 229 bytes
  │  ↓
  └─ HTTP Handler
     ↓
     RocksDB Put (single write, sync WAL)
     Latency: 45ms
     
Total: 50ms per point
Throughput: 20 points/sec per thread
```

### After Optimization (Phase 1)
```
Client (Python SDK)
  │
  ├─ Auto-Batching (1000 points → 1 request)
  │  ↓
  ├─ Zstd Compression (229 bytes → 87 bytes, -62%)
  │  ↓
  ├─ HTTP/2 Multiplexing (10 concurrent requests)
  │  ↓
  └─ POST /ts/put/buffered (Binary Protocol)
     Size: 87 KB (1000 points)
     Latency: 15ms
     ↓
     Server-Side TSAutoBuffer
     ├─ Accumulate: 10,000 points
     ├─ Gorilla Compress: 10-20× reduction
     ├─ Flush interval: 5 seconds
     ↓
     RocksDB WriteBatch (1 transaction, 10k points)
     ├─ Memtable: 1GB (large buffer)
     ├─ WAL: Async (group commit)
     └─ Latency: 5ms per batch

Total: 0.5ms per point (100× faster)
Throughput: 2000 points/sec per thread
```

**Improvements:**
- Latency: 50ms → 0.5ms (**100× faster**)
- Throughput: 20 pts/s → 2000 pts/s (**100× higher**)
- Network: 229 bytes → 0.087 bytes per point (**2600× less**)

---

## 🛠️ Implementation Checklist

### Week 1
- [ ] Enable HTTP/2 in server configuration
- [ ] Implement client-side connection pooling
- [ ] Add payload compression (Zstd) to HTTP handler
- [ ] Configure adaptive write buffer sizing
- [ ] Tune Level0 compaction parameters
- [ ] Deploy to staging environment
- [ ] Run benchmarks and validate metrics

### Week 2
- [ ] Monitor production metrics
- [ ] Fine-tune configuration based on real workload
- [ ] Update client SDKs with connection pooling
- [ ] Document best practices
- [ ] Train team on new configurations

### Month 1-2
- [ ] Implement Product Quantization for embeddings
- [ ] Add memory-mapped file import
- [ ] Enable Direct I/O for bulk imports
- [ ] Test and benchmark medium-term optimizations
- [ ] Gradually roll out to production

---

**For complete details, see:**
- [INGESTION_OPTIMIZATION_IDEAS.md](INGESTION_OPTIMIZATION_IDEAS.md) - Full technical guide
- [INGESTION_OPTIMIZATION_SUMMARY.md](INGESTION_OPTIMIZATION_SUMMARY.md) - Executive summary
- [ingestion-optimized.yaml](../../../config/ingestion-optimized.yaml) - Configuration template

**Status:** Architecture documented ✅  
**Next:** Implementation Phase 1
