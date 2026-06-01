> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/storage/FUTURE_ENHANCEMENTS.md -->

# Storage Module — Public Header Future Enhancements

**Module Path:** `include/storage/`
**Canonical implementation enhancements:** [`../../src/storage/FUTURE_ENHANCEMENTS.md`](../../src/storage/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/storage/` — new interfaces, deprecation removals, and header-level API improvements. Implementation-level enhancements are in:

→ [`../../src/storage/FUTURE_ENHANCEMENTS.md`](../../src/storage/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Backward-compatible within a major version; new capabilities via new methods or versioned types.
- `[x]` `#pragma once` on every header.
- `[x]` `IStorageEngine` must remain a pure interface — no default method bodies.
- `[x]` `[[nodiscard]]` on all factory functions and error-returning methods.
- `[x]` Build-conditional GPU headers (`gpu_compression.h`) must not be pulled in unconditionally.

---

## Required Interfaces (Header Contract)

| Interface | Declared In | Consumer | Status |
|-----------|------------|----------|--------|
| `IStorageEngine::read() / write() / scan()` | `storage_engine.h` | Query, transaction, replication | ✅ Stable |
| `MVCCStore::put() / get() / snapshot()` | `mvcc_store.h` | Transaction manager | ✅ Stable |
| `WALStorage::append() / recover()` | `wal_storage.h` | Bootstrap, crash recovery | ✅ Stable |
| `IBlobStorageBackend::store() / retrieve()` | `blob_storage_backend.h` | Blob manager, LLM model loader | ✅ Stable |
| `CompactionManager::triggerCompaction()` | `compaction_manager.h` | Maintenance scheduler | ✅ Stable |
| `PITRManager::createCheckpoint()` | `pitr_manager.h` | Backup and recovery | ✅ Stable |
| `TensorRouter::route()` | `tensor_router.h` | Tensor mid-layer | ✅ Stable |
| `HybridLogicalClock::now() / update()` | `hlc.h` | Distributed transaction coordinator | ✅ Stable |

---

## Planned Enhancements

### Short-Term (Q3 2026)

- `storage_observability.h` — structured per-operation metrics (latency histograms, byte counters, error codes) emitted via `IStorageMetricsSink`; decouples storage from Prometheus specifics.
- Extend `TieredStorageManager` with a `promoteToHot(key)` / `demoteToWarm(key)` API for manual tier management by the tensor router.
- `IStorageEngine::beginBatch()` — explicit batch context API to replace ad-hoc `BatchWriteOptimizer` calls.

### Medium-Term (Q4 2026)

- `incremental_snapshot_manager.h` — lightweight incremental snapshot decoupled from full PITR, targeting sub-second snapshot latency for the tensor mid-layer.
- `federated_blob_router.h` — region-aware blob routing for geo-distributed deployments; builds on `blob_storage_backend.h` backend abstraction.
- Deprecate direct `RocksDBWrapper` usage outside `src/storage/`; expose only `IStorageEngine` at the public header level.

### Long-Term

- Unified tensor-native storage engine interface that subsumes `IStorageEngine` + `TensorNetworkStorageEngine` into a single polymorphic surface (`ITensorStorageEngine`).
- Zero-copy API extensions: `readDirect()` returning a `std::span<const std::byte>` backed by mmap for the graph truth layer's hot-path reads.
