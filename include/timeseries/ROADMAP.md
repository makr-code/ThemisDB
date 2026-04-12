<!-- Status: current | validated: 2026-04-06 -->

# Roadmap — include/timeseries/

**Latest released version:** v1.5.0 (2026-03-12) | **Active headers:** 17

---

## Current Status

API stable — no breaking changes planned before v2.0.0.

---

## Completed ✅

- [x] `TimeSeriesEngine` lifecycle API (`timeseries.h`)
- [x] Hypertable time-range partitioning (`hypertable.h`)
- [x] Gorilla XOR compression (`gorilla.h`)
- [x] SIMD-accelerated Gorilla paths (`gorilla_simd.h`)
- [x] Low-level chunk store with reference counting (`tsstore.h`)
- [x] AES-256-GCM encrypted chunk store (`encrypted_chunk_store.h`)
- [x] Online key rotation API (`ts_encrypted_key_rotation.h`)
- [x] Continuous aggregates with materialisation (`continuous_agg.h`)
- [x] Background aggregate scheduler (`aggregate_scheduler.h`)
- [x] LTTB and time-bucket downsampling (`downsampling.h`)
- [x] Predicate push-down query optimiser (`query_optimizer.h`)
- [x] Time-based and size-based retention (`retention.h`)
- [x] Prometheus remote-write export (`prometheus_remote_write.h`)
- [x] Fixed-capacity ingest ring buffer (`ts_auto_buffer.h`)
- [x] Adaptive ingest ring buffer (`ts_auto_buffer_adaptive.h`)
- [x] Public metrics/instrumentation API (`timeseries_metrics.h`)
- [x] Scalar aggregate primitives (`aggregates.h`)

---

## Planned Features

### Q3 2026

- [ ] **Streaming cursor API** — `ts_stream_cursor.h` (Target: Q3 2026)
  - Inputs: `QueryPlan` + `ChunkRef` iterator
  - Outputs: lazy row-iterator with back-pressure support
  - Constraints: zero-copy; caller owns result memory
  - Errors: iterator invalidation on concurrent chunk rotation
  - Tests: unit + concurrent stress + ASan memory-leak
  - Perf: ≥ 500 MB/s sustained scan throughput on NVMe storage

- [x] **Multi-metric batch write API** — `TSStore::putBatch(std::span<const TSRow>)` in `tsstore.h` (Target: Q3 2026)
  - `TSRow` (string_view metric/entity, int64_t timestamp_ms, double value) — zero allocation at call site
  - `BatchWriteResult` with `ok_count`, `failed_count`, `row_errors` (per-row index + message)
  - Single `rocksdb::WriteBatch` commit for the entire span (atomic, O(1) WAL writes)
  - Gorilla-compression path: groups by metric:entity, sorts by timestamp, Gorilla-encodes per group
  - 14 focused tests in `tests/test_tsstore_batch.cpp` (TB-01…TB-14)

- [ ] **OpenTelemetry Metrics export** — `ts_otel_exporter.h` (Target: Q3 2026)

### Q4 2026

- [ ] **Columnar layout store** — `ts_columnar_store.h` (Target: Q4 2026)
  - Apache Arrow-compatible column chunks; direct Parquet serialisation
  - Perf: ≥ 4× scan speedup vs row store for single-metric queries

- [ ] **Federated query API** — `ts_federated_query.h` (Target: Q4 2026)
- [ ] **Compression codec registry** — pluggable codec interface (Target: Q4 2026)

---

## Implementation Phases

### Phase 1 — Design / API Contract
- [x] Define `TimeSeriesEngine` and `TSConfig` contracts
- [x] Establish chunk-sealed-immutable semantics
- [x] Define `ChunkKey` ownership and copy semantics
- [ ] Draft `ts_stream_cursor.h` interface (Target: Q3 2026)
- [ ] Draft `ts_columnar_store.h` Arrow ABI contract (Target: Q4 2026)

### Phase 2 — Core Implementation
- [x] Hypertable partition router
- [x] Gorilla encoder/decoder (scalar + SIMD)
- [x] AES-256-GCM chunk encryption
- [x] Continuous aggregate materialisation engine
- [ ] Columnar chunk writer (Target: Q4 2026)

### Phase 3 — Error Handling & Edge Cases
- [x] IV uniqueness enforcement in `EncryptedChunkStore`
- [x] Adaptive buffer OOM guard in `TSAutoBufferAdaptive`
- [x] Chunk-reference dangling-handle prevention in `TSStore`
- [ ] Streaming cursor iterator-invalidation handling (Target: Q3 2026)

### Phase 4 — Tests
- [x] Unit tests for all v1.x headers (≥ 90 % line coverage)
- [x] Gorilla round-trip fuzz tests
- [x] Encryption key-rotation integration tests
- [ ] Columnar store Arrow-compatibility suite (Target: Q4 2026)

### Phase 5 — Performance / Hardening
- [x] SIMD dispatch for Gorilla (AVX2 + NEON)
- [x] Zero-copy query result path
- [x] Adaptive buffer throughput benchmarks
- [ ] Columnar scan micro-benchmarks vs row store (Target: Q4 2026)

### Phase 6 — Documentation & Acceptance
- [x] ARCHITECTURE.md, AUDIT.md, SECURITY.md, CHANGELOG.md
- [ ] API reference (Doxygen HTML) published to docs site (Target: Q3 2026)
- [ ] Migration guide v1.x → v2.0 (Target: Q4 2026)

---

## Production Readiness Checklist

- [x] All public headers have `#pragma once`
- [x] No raw-pointer ownership ambiguity in public APIs
- [x] `[[nodiscard]]` applied to all result-bearing free functions
- [x] Thread-safety documented per class in Doxygen comments
- [x] ABI stability: no virtual destructors removed since v1.0.0
- [x] Security: encryption APIs require explicit key argument
- [x] Metrics: all hot-path counters are lock-free atomics
- [x] CI: headers compile under C++17 and C++20 with `-Wall -Wextra -Werror`
- [ ] API reference published on docs site (Target: Q3 2026)
- [ ] Semantic versioning enforcement via `ts_version.h` (Target: Q3 2026)
