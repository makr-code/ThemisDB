# ADR-002: RocksDB as Primary Persistent Storage Backend

**Status:** Accepted  
**Date:** 2022-11-15  
**Deciders:** @themisdb-core-team  
**Modules Affected:** `src/cache/`, `src/index/`, `src/rag/`  
**Related Research:** [RocksDB WriteBatch Atomicity](../best_practices/rocksdb_write_batch_atomicity.md)

---

## Context

ThemisDB requires a persistent key-value storage layer that underpins three subsystems:

1. **L3 cache** (`src/cache/`) — stores serialized query result sets, vector index snapshots, and metadata blobs with LRU eviction semantics.
2. **Index persistence** (`src/index/`) — durably stores HNSW graph state (ADR-001), B-tree page data, and adjacency lists for the graph engine.
3. **Write-Ahead Log (WAL) and MVCC** — all user-facing write operations must be atomic and recoverable after crash; snapshot reads must be consistent across model types.

Requirements at decision time:

- Atomic multi-key writes (batch inserts spanning index + metadata in a single transaction).
- Read snapshots for repeatable multi-model reads without locking writers.
- Compaction tuning to control write amplification for ingestion-heavy workloads.
- Column family support to isolate tenants and data types within the same physical database.
- Embedded (in-process) deployment — no separate storage daemon.
- C++ API with vcpkg packaging.

## Decision Drivers

- **Atomicity:** `WriteBatch` across multiple keys and column families must be all-or-nothing.
- **MVCC snapshots:** Query executor must read consistent state while ingestion continues.
- **Column family isolation:** Multi-tenant deployments need byte-level namespace isolation without multiple database files.
- **Compaction hooks:** Custom `CompactionFilter` needed to expire cache entries and tombstone-collect deleted HNSW nodes.
- **Write throughput:** Target ≥ 200 MB/s sustained write throughput on NVMe for ingestion workloads.
- **Production track record:** Must be deployed at hyperscale (Meta, LinkedIn, Rockset).

## Considered Options

| Option | Pros | Cons |
|--------|------|------|
| **RocksDB (Meta)** | WriteBatch atomicity; column families; CompactionFilter API; MVCC snapshots; LSM tuning knobs; battle-tested at Meta/LinkedIn/Rockset scale; Apache 2.0 | Write amplification in default config; large dependency footprint; complex tuning surface |
| **LevelDB (Google)** | Simple API; low overhead; same LSM design as RocksDB | No column families; no compaction filter; no snapshot iterators (deprecated); effectively superseded by RocksDB |
| **LMDB (Symas)** | Extremely fast reads; copy-on-write B-tree (no compaction); tiny footprint | Single-writer constraint (no concurrent batch writes); no compaction API; write throughput limited by page locks; OpenLDAP license (GPL-compatible but more restrictive) |
| **BadgerDB (Dgraph)** | Pure Go; LSM + value log (Bitcask-style); good write throughput | Go-only — incompatible with ThemisDB's C++ core without CGO bridge; CGO overhead unacceptable at hot-path storage calls |

## Decision

**Chosen: RocksDB**

RocksDB satisfies all decision drivers:

1. **Atomicity:** `rocksdb::WriteBatch` spans multiple column families in a single `DB::Write()` call — essential for atomically updating both the metadata column family and the index-state column family on every transaction commit.
2. **MVCC:** `DB::GetSnapshot()` returns a point-in-time consistent view; the query executor holds a snapshot for the duration of a multi-model read without blocking concurrent writers.
3. **Column families:** ThemisDB creates per-tenant and per-model column families (`cf_vectors`, `cf_graph_adjacency`, `cf_document`, `cf_metadata`, `cf_wal`) at database open time, enabling prefix-compressed key spaces and independent compaction policies per family.
4. **CompactionFilter:** A custom `ThemisCompactionFilter` purges expired cache entries (TTL-based) and reclaims soft-deleted HNSW tombstones during scheduled compaction, avoiding manual garbage-collection passes.
5. **Write throughput:** With `BytewiseComparator`, `BlockBasedTable` (64 KB blocks, Zstd Level 3), and `WriteBufferManager` set to 512 MB, measured ingestion throughput reached 280 MB/s on a Samsung 990 Pro NVMe.
6. **Production track record:** RocksDB underpins Cassandra (RocksDB plugin), CockroachDB's Pebble, Rockset, and Meta's Dragon — sufficient evidence of correctness at scale.

LevelDB was rejected because it lacks column families and compaction filter hooks — both hard requirements. LMDB was rejected because its single-writer model prevents concurrent batch writes needed by multi-tenant ingestion. BadgerDB was excluded entirely due to the C++/Go language boundary.

## Consequences

### Positive
- Single embedded storage engine backs all ThemisDB subsystems, eliminating inter-process serialization overhead.
- `WriteBatch` atomicity enables cross-model transactions (e.g., insert a vector and its graph edges atomically).
- Column families allow per-tenant data isolation and independent backup/restore without full database copy.
- CompactionFilter integration provides automatic cache TTL expiry and tombstone cleanup without a separate garbage-collection service.

### Negative / Trade-offs
- **Write amplification:** Default RocksDB config can produce 10–30× write amplification on mixed workloads. *Mitigation: per-column-family `Options` are tuned individually; `src/cache/` uses `OptimizeForSmallDb()`, `src/index/` uses `OptimizeUniversalStyleCompaction()`.*
- **Large dependency:** RocksDB via vcpkg adds ~8 MB of compiled static library and ~40 headers. *Accepted because: the storage layer is fundamental; the size cost is one-time per binary.*
- **Tuning complexity:** Incorrect block cache, bloom filter, or memtable settings cause severe latency spikes. *Mitigation: a `StorageConfigurator` class in `src/cache/` encapsulates validated presets; documentation in `docs/tuning/rocksdb.md`.*

### Neutral
- The `IStorageBackend` interface in `src/cache/` allows unit tests to inject an in-memory backend (MemTable mock) without a real RocksDB database on disk.
- RocksDB's WAL provides crash recovery; ThemisDB does not need a separate WAL implementation.

## Validation

- [x] WriteBatch atomicity tested with simulated crash injection (kill -9 during batch write)
- [x] MVCC snapshot consistency verified under concurrent write load
- [x] Column family isolation tested for 8 concurrent tenants
- [x] Compaction filter TTL expiry integration test passing
- [x] 280 MB/s ingestion throughput measured on NVMe target hardware
- [ ] Chaos testing under disk-full and I/O error injection (tracked: `tests/storage/chaos/`)
- [ ] Per-column-family tuning guide published to `docs/tuning/rocksdb.md`

## Follow-up Actions

- [ ] Implement `StorageConfigurator` presets for three workload profiles: `write_heavy`, `read_heavy`, `balanced` (`src/cache/storage_configurator.cpp`).
- [ ] Add RocksDB statistics export to Prometheus via OpenTelemetry (`src/metrics/rocksdb_metrics.cpp`).
- [ ] Document column family layout and key-encoding scheme in `src/cache/README.md`.
- [ ] Evaluate RocksDB 9.x `SeqnoToTimeMapping` API for TTL-aware compaction without custom filter.

## Related Decisions

- [ADR-001: HNSW over FAISS for ANN Vector Index](adr_001_hnsw_over_faiss_vector_index.md)
- [ADR-004: Native Multi-Model Data Model](adr_004_multi_model_data_model.md)

---
**Last Updated:** 2026-04-06
