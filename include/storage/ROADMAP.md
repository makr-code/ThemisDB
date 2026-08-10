> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/storage/ROADMAP.md -->

# Storage Module — Public Header Roadmap

**Module Path:** `include/storage/`
**Canonical implementation roadmap:** [`../../src/storage/ROADMAP.md`](../../src/storage/ROADMAP.md)

---

## Overview

This document tracks the public storage API contract stability, planned header additions, and breaking-change history for `include/storage/`. Feature roadmap items affecting both implementation and headers are tracked in:

→ [`../../src/storage/ROADMAP.md`](../../src/storage/ROADMAP.md)

---

## Current Status

All production storage headers are present and `#pragma once` guarded. The MVCC, WAL, blob, and compaction interfaces are stable for v1.x. Tensor storage headers (`ggml_tensor_bridge.h`, `tensor_network_storage_engine.h`) are production-present but marked experimental pending full TT-decomposition benchmarks.

---

## Completed ✅

- [x] `storage_engine.h` / `rocksdb_wrapper.h` — core read/write/scan contract
- [x] `mvcc_store.h` / `mvcc_chain_pruner.h` — MVCC primitives and GC
- [x] `wal_storage.h` — WAL interface
- [x] `compaction_manager.h` / `adaptive_compaction.h` — compaction coordination
- [x] `blob_storage_manager.h` and backend hierarchy (`filesystem`, `gcs`, `encrypted`, `erasure_coding`)
- [x] `zero_copy_blob_transfer.h` — DMA/sendfile zero-copy path
- [x] `columnar_format.h` / `columnar_cache.h` — columnar scan support
- [x] `compression_strategy.h` + `codec_tags.h` + `gpu_compression.h`
- [x] `tensor_network_storage_engine.h` / `tensor_router.h` / `ggml_tensor_bridge.h`
- [x] `pitr_manager.h` / `backup_manager.h` — PITR and backup
- [x] `online_schema_migration.h` — zero-downtime schema changes
- [x] `key_schema.h` / `merge_operators.h` / `hlc.h`
- [x] `tiered_storage.h` / `disk_space_monitor.h` / `nvme_manager.h`
- [x] `security_signature.h` / `security_signature_manager.h`
- [x] `raft_mvcc_bridge.h` — Raft ↔ MVCC commit path

---

## In Progress

- [ ] Align `tensor_train_decomposer.h` and `hierarchical_tucker_decomposer.h` with production TT-decomposition benchmark results (Target: 2026-Q3)
- [ ] Link `vector_index_backend.h` to updated `include/index/` ANN contract (Target: 2026-Q3)
- [~] `federated_blob_router.h` — first production cut for multi-region blob routing landed; replica-policy expansion remains (Target: 2026-Q4)

---

## Planned

- [ ] `storage_observability.h` — structured per-operation metrics interface for the storage engine (Target: 2026-Q3)
- [ ] `incremental_snapshot_manager.h` — incremental snapshot protocol decoupled from full PITR (Target: 2026-Q4)

---

## Breaking Change History

None in v1.x. Any breaking change requires a MAJOR version bump; see `VERSIONING.md`.
