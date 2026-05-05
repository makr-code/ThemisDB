> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# Tensor Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status

Experimental — Phase 1 skeleton complete (2026-05-05).  Core interfaces
(`ITensorIndex`, `TensorIndexManager`, `HnswTTBridge`) and a linear-scan
reference implementation (`FlatTensorIndex`) are in place.
RocksDB persistence and hnswlib integration are Phase 2 targets.

## Completed ✅

- [x] `ITensorIndex` interface with add/search/norm/innerProduct/save/load
- [x] `FlatTensorIndex` — Phase-1 linear-scan reference implementation
- [x] `TensorIndexManager` — lifecycle registry, routing, tenant isolation
- [x] `HnswTTBridge` header + skeleton with two-layer architecture
- [x] TT inner-product sweep (Holtz 2012) in O(d·r³)
- [x] First-core sketch extraction for HNSW navigation layer
- [x] Module docs: README, ROADMAP, ARCHITECTURE, FUTURE_ENHANCEMENTS, AUDIT
- [x] Research docs: boundary analysis, arXiv draft, AdaLoRA bridge

## In Progress 🚧

- [~] Stub entries registered in `src/STUB_INVENTORY.md` (TTI-01..02, TIM-01..02, HTB-01..03)

## Planned Features 📋

### Short-term (Phase 2, Q4 2026)

- [ ] Wire `HnswTTBridge::HnswLayer` to hnswlib `HierarchicalNSW<float>` (Target: Q4 2026)
  - Sketches from first TT-core (dim = min(n₁, `sketch_dim`))
  - Build-time: no full vector needed; only first-core required for HNSW graph
  - AC: recall@10 ≥ 0.95 vs brute-force on 1 M 4096-dim vectors

- [ ] RocksDB persistence for `FlatTensorIndex` and `HnswTTBridge` (Target: Q4 2026)
  - Key schema: `__ttidx__:<tenant>:<collection>:<field>:<id>:G<k>:<version>`
  - save() writes all TT-cores atomically via WriteBatch
  - load() rebuilds in-memory index and HNSW graph

- [ ] `TensorIndexManager::dropTenantIndexes()` — RocksDB prefix-delete (Target: Q4 2026)
  - Use `DeleteRange(__ttmgr__:<tenant_id>:, __ttmgr__:<tenant_id+1>:)`

- [ ] `TensorIndexManager` wired to `TensorNetworkStorageEngine` for core persistence (Target: Q4 2026)

### Medium-term (Phase 3, Q1 2027)

- [ ] `TensorIndexManager::ggmlCorePtrs()` — mmap GGML bridge (Target: Q1 2027)
  - Pin TT-core pages via `mmap(MAP_SHARED)` or `cudaHostRegister`
  - Return `ggml_tensor*` compatible descriptor for llama.cpp injection
  - Zero-copy path: DB → VRAM without intermediate CPU buffer
  - AC: TTFT reduction ≥ 3× vs. classical JSON-RAG path

- [ ] `GgmlTensorBridge` integration in `src/storage/ggml_tensor_bridge.h` (Target: Q1 2027)
  - `GGML_TYPE_TT` type descriptor registration
  - llama.cpp graph node injection via `ggml_map_custom1`

- [ ] `TensorAwareQueryOptimizer` plan-node `TENSOR_CONTRACTION` routing (Target: Q1 2027)
  - Detect TT-stored operands in AQL plan
  - Route to `TensorContractionEngine` instead of reconstructing flat vectors

### Long-term (Phase 4, Q2 2027)

- [ ] `AdaLoRA ↔ TT Bridge` Phase 3 — `findSimilarAdapters()` via TensorFingerprintGraph (Target: Q3 2027)
- [ ] Distributed TT shard layout (one shard per TT-core index) (Target: Q4 2027)
- [ ] CUDA cuSOLVER backend for on-GPU TT-SVD during `addFlat()` (Target: Q4 2027)

## Implementation Phases

### Phase 1: Interface & Reference (Status: ✅ Complete — 2026-05-05)

- [x] `ITensorIndex` — interface definition
- [x] `FlatTensorIndex` — linear-scan impl
- [x] `TensorIndexManager` — lifecycle / routing
- [x] `HnswTTBridge` — header + skeleton
- [x] TT arithmetic: inner-product (O(d·r³)), norm, cosine similarity

### Phase 2: Production Core (Target: Q4 2026)

- [ ] hnswlib integration in `HnswTTBridge::HnswLayer`
- [ ] RocksDB persistence (save/load for all backends)
- [ ] Tenant prefix-delete in `dropTenantIndexes()`
- [ ] `TensorIndexManager` → `TensorNetworkStorageEngine` wire-up
- [ ] Phase-2 tests: TTI-10..20, HTB-10..20

### Phase 3: Zero-Copy Inference (Target: Q1 2027)

- [ ] `ggmlCorePtrs()` — mmap + page-pin
- [ ] `GgmlTensorBridge` full implementation
- [ ] `TensorAwareQueryOptimizer` TENSOR_CONTRACTION plan-node
- [ ] FLARE integration test (TTFT ≤ 90ms target)

### Phase 4: Distributed & GPU (Target: Q2–Q4 2027)

- [ ] Per-core shard distribution over Themis-Net
- [ ] CUDA TT-SVD during add()
- [ ] AdaLoRA `findSimilarAdapters()` wire-up

## Production Readiness Checklist

- [ ] All stub entries resolved (TTI-01..02, TIM-01..02, HTB-01..03)
- [ ] hnswlib integration tested with recall@10 ≥ 0.95
- [ ] RocksDB persistence tested with crash recovery
- [ ] GGML bridge validated end-to-end with llama.cpp
- [ ] Security audit: tenant key isolation
- [ ] Performance benchmarks: search latency vs. HNSW at dim=4096

## Known Issues & Limitations

- `FlatTensorIndex::search()` is O(n·d·r²) — only suitable for n ≤ 50 K
- 1-D TT-decomposition in `addFlat()` loses inter-mode correlations;
  multi-mode reshape requires explicit `shape` parameter
- `HnswTTBridge::HnswLayer` is a linear-scan stub (HTB-01)
- TT inner-product sweep is correct but not SIMD-optimised in Phase 1

## Breaking Changes

| Version | Change |
|---------|--------|
| v2.0.0  | `ITensorIndex::add()` signature may gain `TTAddOptions` for rank override |
| Phase 3 | `ggmlCorePtrs()` return type will change to `GgmlCoreDescriptor` struct |
