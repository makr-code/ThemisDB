> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-05-13 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · SECURITY.md · PERFORMANCE_EXPECTATIONS.md -->

# Changelog — Tensor Module

All notable changes to the Tensor module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

- Phase-2 HNSW navigation layer for `HnswTTBridge` (hnswlib wiring, Target Q4 2026)
- Phase-2 RocksDB persistence for `FlatTensorIndex::save/load` (Target Q4 2026)
- Phase-2 RocksDB prefix-delete for `TensorIndexManager::dropTenantIndexes()` (Target Q4 2026)
- Phase-3 real mmap `MAP_SHARED` for `TensorMmapBridge` (Target Q1 2027)
- Phase-3 `AdapterRepository::loadAdapter()` mmap path (Target Q1 2027)
- Phase-4 full TT inner-product for `TensorFingerprintGraph` (Target Q3 2027)
- Phase-5 Hierarchical Tucker index production implementation (Target Q2 2028)
- Phase-6 TNSR topology persistence bridge (Target Q3 2028)
- Phase-7 semantic encoder for `UTRConverter::fromDocument/fromImage` (Target Q4 2028)

## [1.2.0] — 2026-05-13

### Added
- `SECURITY.md` — threat model, security controls, Phase-2/3 security requirements,
  known limitations, and dependency security for the tensor module.
- `CHANGELOG.md` — Keep-a-Changelog tracking for all tensor module releases.
- Cross-reference links added to `ARCHITECTURE.md` (Related-docs block) and
  `README.md` (PERFORMANCE_EXPECTATIONS.md and AUDIT.md entries).

### Changed
- `AUDIT.md` updated to reflect current scope: 14 source files, 15 headers,
  16 open stubs (Phases 1–7); compliance table extended with SECURITY.md,
  CHANGELOG.md, and PERFORMANCE_EXPECTATIONS.md rows; last-audit date set to 2026-05-13.
- `ROADMAP.md` completed docs entry extended to include SECURITY, CHANGELOG,
  PERFORMANCE_EXPECTATIONS.

## [1.1.0] — 2026-05-05

### Added
- Phase 5 — Hierarchical Tucker Index:
  - `HTTrain`, `HTNode`, `HTContractionEngine` types in `include/tensor/ht_train.h`
  - `IHierarchicalTuckerIndex`, `FlatHTIndex` in `include/tensor/ht_index.h` / `src/tensor/ht_index.cpp`
- Phase 6 — Adaptive Structural Rounding:
  - `HissStructuralSearchEngine`, `HissReshaper`, `TemplateCatalog` in
    `include/tensor/hiss_structural_search.h` / `src/tensor/hiss_structural_search.cpp`
  - `TNSRTask` background topology-rounding task in `include/tensor/tnsr_task.h` /
    `src/tensor/tnsr_task.cpp` (STUB #252 — topology mutations not yet persisted)
- Phase 7 — Multi-Modal UTR Encoding:
  - `UTRConverter` in `include/tensor/utr_converter.h` / `src/tensor/utr_converter.cpp`
    (STUB #257/#258 — FNV-1a and raw-pixel fallbacks active)
  - `HyperIndexBuilder` in `include/tensor/hyper_index_builder.h` /
    `src/tensor/hyper_index_builder.cpp`
- `ARCHITECTURE.md`, `AUDIT.md`, `FUTURE_ENHANCEMENTS.md`, `PERFORMANCE_EXPECTATIONS.md`
  module documentation.
- Stubs #252, #254, #257, #258 registered in `AUDIT.md` and `README.md` stub table.

### Changed
- Stub table in `README.md` extended with Phase 5–7 entries.

## [1.0.0] — 2026-05-01

### Added
- Phase 1 — Core TT Index (Complete):
  - `ITensorIndex` interface in `include/tensor/tensor_index.h`
  - `FlatTensorIndex` — Phase-1 linear scan reference implementation
    (`src/tensor/tensor_index.cpp`); stubs: TTI-01 (`save`), TTI-02 (`load`)
  - `TensorIndexManager` lifecycle manager and routing bridge
    (`include/tensor/tensor_index_manager.h`, `src/tensor/tensor_index_manager.cpp`);
    stubs: TIM-01 (`ggmlCorePtrs` raw pointer), TIM-02 (`dropTenantIndexes` RocksDB delete)
  - `TensorIngestionBridge` — `ITensorDecompositionBackend` implementation
    (`include/tensor/tensor_ingestion_bridge.h`, `src/tensor/tensor_ingestion_bridge.cpp`)
  - `TensorCoreStorageBridge` — `ITensorCoreBridge` with `InMemoryTensorBackend`
    (`include/tensor/tensor_core_bridge.h`, `src/tensor/tensor_core_bridge.cpp`);
    stub #160
- Phase 2 — HNSW Hybrid Bridge (In Progress):
  - `HnswTTBridge` in `include/tensor/hnsw_tt_bridge.h` / `src/tensor/hnsw_tt_bridge.cpp`;
    stubs: HTB-01 (linear-scan HNSW layer), HTB-02/HTB-03 (save/load)
- Phase 3 — Zero-Copy GGML Bridge:
  - `TensorMmapBridge` in `include/tensor/tensor_mmap_bridge.h` /
    `src/tensor/tensor_mmap_bridge.cpp`; stub #176 (`MAP_ANONYMOUS`)
  - `AdapterRepository` in `include/tensor/adapter_repository.h` /
    `src/tensor/adapter_repository.cpp`; stub #172 (heap copy)
  - `TensorButterflyOperator` in `include/tensor/tensor_butterfly_operator.h` /
    `src/tensor/tensor_butterfly_operator.cpp`
    (FOURIER/WHT complete; RADON/GREENS_FUNCTION stubs)
- Phase 4 — Adapter Fingerprint Graph:
  - `TensorFingerprintGraph` in `include/tensor/tensor_fingerprint_graph.h` /
    `src/tensor/tensor_fingerprint_graph.cpp`; stub #276 (column-mean cosine)
- Tenant key isolation via `__ttmgr__:<tenant>:<collection>:<field>` scheme.
- Thread-safe reads on all `ITensorIndex` operations via `shared_mutex`.
- `README.md` with full component table and stub inventory.
