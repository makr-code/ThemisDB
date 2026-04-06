# RocksDB WriteBatch for Atomic Multi-Key Index Writes

**Metadaten:**
- Source: RocksDB Documentation — "WriteBatch" (Meta/Facebook)
- URL: https://github.com/facebook/rocksdb/wiki/Basic-Operations#atomic-updates
- Tags: storage, consistency
- ThemisDB-Versionen: v1.0.0+
- Status: [x] Identified | [x] Partially Adopted | [x] Fully Adopted

## 📋 Summary

When a single logical database operation requires writing to multiple RocksDB keys (primary row + secondary index entries + metadata keys), writing them one-by-one with individual `Put` calls creates a window of partial visibility: a crash or concurrent read between the first and last write can observe an inconsistent state where some index entries exist but not others. RocksDB's `WriteBatch` API solves this by batching all mutations into a single atomic unit that is committed to the WAL and memtable in one operation, ensuring all-or-nothing semantics from the perspective of any reader.

ThemisDB's index subsystem (`src/index/`) adopted this pattern from v1.0.0 as the foundation for all index writes, guaranteeing that no partial index state is ever externally observable.

## 🎯 Core Principles

- **All-or-nothing atomicity**: Every logical write operation (insert, update, delete) that touches more than one key must use a `WriteBatch`; standalone `Put`/`Delete` calls are forbidden for index-bearing paths.
- **Batch construction before commit**: Assemble the complete batch in memory (primary key, all secondary index keys, tombstones for old index entries) before calling `DB::Write(batch)`.
- **WAL fsync guarantees durability**: `WriteOptions::sync = true` is set for index-critical batches; for high-throughput append paths, group commit (`sync_file_range`) is used instead.
- **Column family awareness**: Each secondary index lives in its own RocksDB Column Family; `WriteBatch` supports cross-CF mutations, keeping the batch atomic even across families.
- **Idempotent replay**: Batches are assigned a monotonically increasing sequence number; the WAL replay path skips already-applied sequence numbers to enable crash recovery without double-writes.

## 🔗 Adoption in ThemisDB

### Affected Modules

- `src/index/` — `IndexWriter::commitBatch(WriteBatch&)` is the single write entry point for all index mutations; callers build a `rocksdb::WriteBatch` and hand it to `commitBatch`.
- `src/index/btree_index.cpp` — Constructs batch entries for B-tree index node splits and key insertions.
- `src/index/hash_index.cpp` — Constructs batch entries for hash-bucket chain updates.
- `src/index/inverted_index.cpp` — Full-text posting list updates use WriteBatch to atomically update both the postings CF and the document-frequency CF.

### What Was Adopted?

- `rocksdb::WriteBatch batch;` is constructed per logical operation.
- For each secondary index: `batch.Put(index_cf_handle, index_key, index_value);`
- For deleted old index entries: `batch.Delete(index_cf_handle, old_index_key);`
- Primary row write is included in the same batch: `batch.Put(primary_cf_handle, row_key, row_value);`
- `db_->Write(write_opts, &batch)` commits the batch; on failure the entire operation is retried or returned as an error.
- `WriteBatch::SetSavePoint()` / `WriteBatch::RollbackToSavePoint()` used in complex update paths where partial batch construction can fail before commit.

### Deviations & Rationale

- **No `WriteBatchWithIndex` for read-your-writes**: `WriteBatchWithIndex` enables reading back uncommitted batch entries during construction. ThemisDB does not use this because index construction does not require reading back the batch mid-build; using the simpler `WriteBatch` avoids the overhead of maintaining the in-batch sorted index.
- **Sync option is workload-dependent**: For bulk-load import operations, `sync=false` is used with periodic manual `FlushWAL(true)` calls to trade per-batch fsync latency for throughput. This is documented in the bulk loader and not used in transactional paths.
- **Column family handles cached at startup**: `ColumnFamilyHandle*` pointers are resolved once at `DB::Open` time and stored; this avoids per-write CF lookup overhead.

## ⚠️ Trade-offs & Limitations

- **Memory overhead for large batches**: A `WriteBatch` accumulates all mutations in memory before commit. For very wide rows with many secondary indexes, this can be several MiB per batch. ThemisDB limits secondary index fan-out per schema.
- **No cross-shard atomicity**: `WriteBatch` is atomic within a single RocksDB instance. For distributed transactions spanning multiple shards, a two-phase commit protocol (not yet implemented) would be required.
- **WAL size**: Using `sync=true` on every batch increases WAL flush frequency. RocksDB's `WAL_ttl_seconds` and `WAL_size_limit_MB` must be tuned to avoid unbounded WAL growth.
- **Column family proliferation**: Giving each index type its own CF simplifies per-index compaction tuning but increases the number of open file descriptors and compaction threads needed.

## 🔬 Validation

- [x] Code reviewed against RocksDB WriteBatch documentation and `db_bench` atomicity tests
- [x] Unit tests in `tests/index/` inject failures mid-batch and verify no partial index state
- [x] Crash-recovery tests use `db_stress` tooling to verify consistency post-crash
- [x] Module README linked (`src/index/README.md`)
- [ ] implementation_influence index updated

## 📚 Related

- [Shared Mutex Read-Write Locks](shared_mutex_read_write_locks.md)
- [OpenTelemetry Tracing](opentelemetry_tracing.md)

---
**Last Updated:** 2026-04-06
