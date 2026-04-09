<!-- Status: current | validated: 2026-04-06 -->

# include/storage/ — Roadmap

> Public header API evolution roadmap for the ThemisDB Storage module.  
> Implementation roadmap: [`../../src/storage/`](../../src/storage/)

---

## Current Status

The storage public API is **production-ready** at v1.6.0.  All 39 headers are
stable and covered by integration tests.  The v1.6 release added zero-copy blob
transfer, write-optimised merge tree, NLP metadata extraction, and online schema
migration.

---

## Completed ✅

- [x] Core `IStorageEngine` abstraction (`storage_engine.h`)
- [x] RocksDB MVCC wrapper (`rocksdb_wrapper.h`, `mvcc_store.h`)
- [x] Multi-model key schema (`key_schema.h`)
- [x] Blob storage with pluggable backends
- [x] GCS blob backend (`blob_backend_gcs.h`)
- [x] Erasure-coded backend (`erasure_coding_backend.h`)
- [x] Adaptive and tiered compaction
- [x] Backup and PITR (`backup_manager.h`, `pitr_manager.h`)
- [x] GPU compression interface (`gpu_compression.h`, CUDA-optional)
- [x] Raft-MVCC bridge (`raft_mvcc_bridge.h`)
- [x] Security signatures (`security_signature.h`)
- [x] Audit logging (`storage_audit_logger.h`)
- [x] HLC interface (`hlc.h`)
- [x] Time-travel history (`history_manager.h`)
- [x] Online schema migration (`online_schema_migration.h`)
- [x] Zero-copy blob transfer (`zero_copy_blob_transfer.h`)
- [x] Write-optimised merge tree (`wom_tree.h`)
- [x] NVMe manager with io_uring integration (`nvme_manager.h`)

---

## Planned Features

### Q2 2026

- [ ] `vector_index_backend.h` — ANN / HNSW index backend interface
      (Target: Q2 2026)
  - Supports cosine, dot-product, and L2 distance metrics
  - Inputs: float32 / float16 embedding vectors, index config
  - Outputs: k-NN result set with distance scores
  - Constraints: interface must be backend-agnostic (FAISS, hnswlib, custom)
  - Tests: unit + recall@k benchmarks

- [ ] `encrypted_blob_backend.h` — transparent client-side encryption backend
      (Target: Q2 2026)
  - AES-256-GCM per-object key wrapping
  - Key material via `security_signature_manager.h`
  - Tests: encryption/decryption round-trip + key rotation tests

### Q3 2026

- [ ] `streaming_ingest_manager.h` — high-throughput streaming ingest interface
      (Target: Q3 2026)
  - Inputs: event streams (Kafka-compatible / raw TCP)
  - Outputs: durable writes via WAL with ≤ 50 ms end-to-end latency
  - Perf: ≥ 1M events/s sustained on 8-core node

- [ ] `columnar_cache.h` — in-memory columnar cache for analytics acceleration
      (Target: Q3 2026)
  - Apache Arrow-compatible memory layout
  - LRU eviction + pinned segment API
  - Perf: ≥ 10× scan speedup vs row-store on wide-table queries

- [ ] `change_data_capture.h` — CDC stream interface for downstream consumers
      (Target: Q3 2026)
  - Exposes WAL deltas as structured change events
  - Ordering guarantee: causal consistency per-key
  - Integration with `wal_storage.h` and `hlc.h`

### Q4 2026

- [ ] `remote_storage_backend.h` — S3-compatible object storage backend
      (Target: Q4 2026)
  - Supports AWS S3, MinIO, Ceph RADOS Gateway
  - Multipart upload, presigned URLs, lifecycle rules
  - Tests: mocked S3 + real MinIO integration tests

- [ ] `tiered_cache.h` — multi-tier block cache (DRAM → NVMe → SSD)
      (Target: Q4 2026)
  - Configurable admission and eviction policies
  - Integrates with `tiered_storage.h`
  - Perf: ≥ 90% cache hit rate on Zipf(1.1) workload

---

## Implementation Phases (Next Feature Cycle)

### Phase 1 — API Contract Design
- [ ] Draft `vector_index_backend.h` with full Doxygen documentation
- [ ] ADR for encrypted blob key management strategy
- [ ] Peer review of `streaming_ingest_manager.h` with data pipeline team

### Phase 2 — Core Implementation
- [ ] Vector index backend implementation file (planned) + unit tests
- [ ] Encrypted blob backend implementation file (planned) with AES-GCM core

### Phase 3 — Error Handling & Edge Cases
- [ ] Vector index: out-of-memory and dimension-mismatch errors
- [ ] Encrypted backend: key-not-found and decryption failure paths
- [ ] Streaming ingest: backpressure and overflow handling

### Phase 4 — Tests
- [ ] Vector: recall@10 benchmark at 1M, 10M, 100M vectors
- [ ] Encryption: round-trip + key-rotation + corrupt-ciphertext tests
- [ ] Streaming: throughput tests under sustained load

### Phase 5 — Performance & Hardening
- [ ] Vector index: ANN recall@10 ≥ 0.95 at ≥ 5000 QPS per core
- [ ] Encrypted backend: overhead ≤ 5% vs unencrypted path (AES-NI path)
- [ ] Streaming ingest: p99 write latency ≤ 50 ms at 1M events/s

### Phase 6 — Documentation & Acceptance
- [ ] Update ARCHITECTURE.md with new headers
- [ ] Update CHANGELOG.md with all new symbols
- [ ] Acceptance review against production-readiness checklist

---

## Production Readiness Checklist

- [x] All headers compile cleanly on GCC 12+, Clang 15+, MSVC 19.35+
- [x] All public symbols documented with Doxygen comments
- [x] No RocksDB internal types exposed in public headers
- [x] Thread-safety guarantees documented per class
- [x] Error return codes defined per interface
- [x] No STL types with ABI instability in public interface
- [x] Versioning macros present in `storage_engine.h`
- [ ] C-compatible wrapper headers for FFI consumers (Planned Q3 2026)
- [ ] Python / Rust binding stubs generated from headers (Planned Q4 2026)
