# Architecture - Timeseries Module

<!-- Status: current | validated: 2026-09-09 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · PHASE_6_ACCEPTANCE_CHECKLIST.md · PERFORMANCE_BASELINE.md · OPERATOR_GUIDE.md -->

## Overview

The timeseries module composes high-frequency ingest and storage behavior, compression and adaptive flush behavior, aggregation and query optimization behavior, and retention/encryption/remote-write lifecycle behavior into a bounded subsystem.

## Main Execution Planes

1. Ingest and storage plane
- timeseries API, TSStore, hypertable, and chunk storage behavior

2. Compression and query plane
- Gorilla codec, adaptive flush, query optimization, and range/downsampling behavior

3. Lifecycle and integration plane
- retention, aggregation scheduling, encryption, metrics, and remote-write behavior

## Core Contracts

| Contract | Behavior |
|---|---|
| ingest contract | deterministic write, batch, and flush behavior |
| query contract | bounded range, aggregation, and downsampling behavior |
| lifecycle contract | explicit retention and encryption transition behavior |
| integration contract | observable remote-write and metrics behavior |

## Failure Semantics

- ingest, compression, and flush faults are explicit.
- query and downsampling failures remain diagnosable and non-silent.
- retention and key-rotation lifecycle faults remain observable.
- remote-write and encrypted chunk failures surface deterministic outcomes.

## Sourcecode Verification (Module: timeseries/architecture)

- Verified files:
  - src/timeseries/timeseries.cpp
  - src/timeseries/tsstore.cpp
  - src/timeseries/gorilla.cpp
  - src/timeseries/ts_auto_buffer.cpp
  - src/timeseries/ts_auto_buffer_adaptive.cpp
  - src/timeseries/continuous_agg.cpp
  - src/timeseries/downsampling.cpp
  - src/timeseries/query_optimizer.cpp
  - src/timeseries/retention.cpp
  - src/timeseries/encrypted_chunk_store.cpp
  - src/timeseries/prometheus_remote_write.cpp
- Verified architecture claims:
  - ingest/storage + compression/query + lifecycle/integration plane split
  - explicit failure boundaries for flush, query, retention, and remote-write faults
  - module-local ownership of timeseries behavior

## Module Dependencies

### Direct Upstream Dependencies (this module uses)
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| storage | `include/storage/` | Chunk and hypertable persistence backend |
| utils | `include/utils/` | Logging, compression (Gorilla codec support), and thread helpers |

### Direct Downstream Consumers (modules that use this module)
| Module | Via | Notes |
|--------|-----|-------|
| server | `server/timeseries_api_handler.h` | Server exposes ingest, query, and retention lifecycle APIs for timeseries data |

## Integration Points

### Critical Integration: Server Timeseries API
**Files:** `src/timeseries/timeseries.cpp` ↔ `server/timeseries_api_handler.h`
**Contract:** Server delegates all timeseries write/query/retention operations; `TSStore` is the sole owner of chunk lifecycle.
**Thread Safety:** Ingest writes and query reads are concurrency-safe through per-hypertable locking; retention runs in a background thread with its own lock.

### Critical Integration: Storage Chunk Backend
**Files:** `src/timeseries/tsstore.cpp` ↔ `storage/`
**Contract:** TSStore writes chunks through storage interfaces; chunk compaction and encryption (EncryptedChunkStore) operate within storage transaction boundaries.
**Thread Safety:** Chunk writes are serialised per-hypertable; reads may proceed concurrently via MVCC snapshots.