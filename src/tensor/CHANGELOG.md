---
Author: "ThemisDB Contributors"
Created: "2026-05-31"
Last Updated: "2026-11-21"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

> Exact model backend not verifiable from final document text alone.
>
> Historical changelog entries describe implementation state at the time they
> were recorded.

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog - Tensor Module

All notable changes to the tensor module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Changed
- `themis_tensor` and tensor test targets now omit source files already compiled into static owners or eligible shared owners. Shared owners require Windows auto-export or non-hidden C++ visibility on other platforms. The module owner link is private; controlled by `THEMIS_TENSOR_DEDUP_OWNER_SOURCES` (default `ON`). CMake Tools built `themis_tensor`, four focused Tensor test targets, and `test_tensor_recompress`; the recompress target now links `tensor_compaction_filter.cpp` directly. CTest execution remained blocked by CMake Tools build-process failures; non-exporting shared-library overlap remains unresolved.
- Registered `themis_tensor` independently of `THEMIS_BUILD_ENTERPRISE_PLUGINS` and before tests/benchmarks, so guarded tensor test registrations see the module target. No configure, build, or test was run for this change.
- Documentation now distinguishes implemented tensor behavior from planned requirements and historical benchmark claims; added current C++17 build/test guidance and marked unverified benchmark snapshots as historical.
- Documentation governance sync: README, ARCHITECTURE, SECURITY, ROADMAP, FUTURE_ENHANCEMENTS, AUDIT, and PERFORMANCE_EXPECTATIONS aligned to source-verifiable module behavior.
- Performance expectations updated to explicit verified benchmark symbols from tensor fingerprint graph, tensor fingerprint, and tensor dedup benchmark suites.
- Hardened `TensorCoreStorageBridge` fail-closed behavior: backend `put()` exceptions are now converted to `ERR_STORAGE_TRANSACTION_FAILED` results, and `getRaw()` now returns `nullopt` when backend `get()` throws.
- Hardened `FlatTensorIndex::load()`: `.ttidx` input is treated as untrusted (bounded counts, overflow-checked core sizes, TT rank-chain validation, duplicate-ID rejection); on any failure the previous contents are kept and no exception escapes.
- `TensorIndexManager::createIndex()` now honours `max_rank`/`epsilon` via the new `createFlatTensorIndex()` factory and returns `nullptr` for epsilon outside (0, 1); the concurrent-create gate now uses an atomic compare-exchange loop.
- Removed the `#include "tensor/tensor_index.cpp"` from `tensor_index_manager.cpp`; `tensor_index.cpp` is now listed in every source list that compiles the manager.
- The `main_server.cpp` tensor-index bootstrap now injects its live decomposition and RocksDB core-storage bridges into the server's `ToolboxRegistry` when `tensor_index` is configured.

## [2.1.x] - 2026

### Added
- tensor index/bridge/graph hardening improvements.

## [2.0.x] - 2025-2026

### Added
- expanded tensor bridge, fingerprint graph, and structural helper surfaces.

## [1.x] - 2024-2025

### Added
- foundational tensor index and bridge infrastructure.