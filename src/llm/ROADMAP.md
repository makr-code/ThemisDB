# LLM Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-20 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

The module provides production-grade LLM runtime surfaces across async inference, enhanced multi-model orchestration, adapter/plugin management, routing, streaming, and safety/policy controls.

### Module Evidence Summary (2026-07-19)

**Doxygen Documentation:**
- [x] All 90 .cpp source files have Doxygen @file headers with maturity/quality metadata
- [x] Canonical Doxygen format applied to final_layer_orchestrator.cpp and model_switch_workflow.cpp

**Build & Test Evidence:**
- Build preset: `windows-release`
- Test target: `module_llm_test_active_vram_allocator_focused`
- Latest validation: 2026-07-19
- Result: PASS (exit 0, [  PASSED  ] 45 tests)
- Test timeout budget: 120s

## Wiki Secondary Index — Phase A (2026-07-27)

### Implementation units delivered

- [x] `wiki_index_store.cpp` — `WikiIndexStore` (BM25 + HNSW + RRF) + `JsonWikiIndexReader`
- [x] `wiki_chunk_splitter.cpp` — `WikiChunkSplitter` (heading-aware, sliding-window)
- [x] `wiki_rag_source.cpp` — `WikiRagSource` (RAGStageHandler, fail-open)

### Test coverage

- [x] `tests/llm/test_wiki_index_store.cpp` — WIS-01..16 unit tests
- [x] `tests/llm/test_wiki_rag_quality.cpp` — WISQ-01..05 quality gate tests
  (Recall@5 ≥ 80 %, latency < 200 ms/10 queries on 100 chunks)

### Phase B (2026-07-27, partial delivery)

**Delivered:**

- [x] Fix `ingestion::BaseEntity` → `themis::BaseEntity` type mismatch in
      `wiki_index_store.cpp` — `writeChunk()` and `writeBatch()` now build
      storage-compatible entities via `chunkToEntity()` that uses
      `themis::BaseEntity::setField()`.  The `entityToChunk()` dead-code stub
      (which referenced the non-existent `getProperty()` API) was removed.
- [x] `tests/llm/test_wiki_index_store_phase_b.cpp` — WIS-B-01..16
      RocksDB-backed integration tests covering: construction, empty query,
      BM25-only retrieval, KNN-only retrieval, hybrid RRF, `writeBatch()`,
      `top_k`, `min_score`, pre-embedded chunks, concurrent reads,
      writer+reader concurrency, score monotonicity, `flush()` no-op, query
      embedding cache, and multi-doc-id ingestion.
- [x] Thread-safety hardening: removed `const_cast<EmbeddedLLM&>(llm_)` from
      `query()`; added `mutable EmbeddedLLM* llm_ptr_` +
      `mutable std::mutex query_embed_mutex_` +
      `mutable std::unordered_map<...> query_embed_cache_` so query-embedding
      caching is race-free under concurrent readers.

**Remaining (Target: Q3–Q4 2026):**

- [ ] Persistent embedding cache keyed by chunk_id in RocksDB
- [ ] Embedding dimension auto-probe on first `embed()` call
- [ ] Streaming ingest via `WriteBatch` with configurable batch size

---

## In Progress

- [~] Cross-node and shard-aware inference hardening (Target: Q3 2026)
- [~] Runtime cancellation semantics and timeout behavior consistency across engine variants (Target: Q3 2026)
- [~] Runtime benchmark and regression gate alignment for RAID/RAG-heavy inference paths (Target: Q3 2026)
- [~] GA Sign-off evidence bundling for delivered P5-L01/P5-L02 hardening (Target: Q3 2026)
  - [ ] Attach residual-risk register for exception-safety/memory/recovery paths (Target: Q3 2026)
  - [ ] Reconfirm focused + release-critical regression proof on current `develop` baseline (Target: Q3 2026)
  - [ ] Link ownership/failure-mode sign-off evidence into root gate board docs (Target: Q3 2026)

## Planned Features

- [ ] End-to-end distributed draft/verify optimization in speculative decoding paths (Target: Q4 2026)
- [ ] Stronger operational isolation for multi-tenant adapter lifecycle and cache surfaces (Target: Q4 2026)
- [ ] Extended operator diagnostics for model routing, queue pressure, and policy-deny causes (Target: Q4 2026)
- [~] Wave B B3: multi-task LoRA shared-base/domain-gating/joint-loss rollout (Target: Q1–Q2 2027) — core impl + ablation/benchmark tests done

## Implementation Phases

### Phase 1: Design / API Contract
- [ ] Define and freeze non-breaking API contracts for inference, streaming, and routing extension points (Target: Q3 2026)
- [ ] Document ownership/lifecycle boundaries for plugin, model, and adapter resources (Target: Q3 2026)

### Phase 2: Core Implementation
- [ ] Implement pending distributed inference and speculative decode integration items (Target: Q4 2026)
- [ ] Complete runtime wiring for queue/load telemetry propagation in all configured execution paths (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] Standardize failure envelopes for timeouts, cancellation, backend unavailability, and partial fan-out errors (Target: Q4 2026)
- [ ] Harden fallback behavior when optional acceleration/runtime features are unavailable (Target: Q4 2026)

### Phase 4: Tests
- [~] Expand focused tests for distributed orchestration, adapter hot-swap races, and stream abort handling (Target: Q4 2026)
  - [x] CBS-H-01..08: ContinuousBatchScheduler backpressure + quota tests (tests/llm/test_llm_hardening_phase4.cpp)
  - [x] TQM-H-01..04: TokenQuotaManager sliding-window semantics
  - [~] Distributed orchestration / remote-shard failure paths (pending)
- [~] Add deterministic regression suites for routing and policy enforcement under load (Target: Q4 2026)
  - [x] PCL-H-01..06: PromptPolicy concurrent access + hot-swap safety
  - [x] SHD-H-01..04: Engine + scheduler shutdown-under-load teardown

### Phase 5: Performance and Hardening
- [x] P5-L01: Exception safety audit + RAII wrapper hardening — 28 EXS-* tests delivered (tests/llm/test_llm_phase5_hardening.cpp)
- [x] P5-L02: Memory leak fixes (model loading, cache cleanup) — 24 MEM-* tests delivered (tests/llm/test_llm_phase5_hardening.cpp)
- [~] Lock performance gates to benchmark-backed thresholds and release baselines (Target: Q4 2026)
- [~] Validate memory-pressure and VRAM-recovery behavior under sustained multi-model load (Target: Q4 2026)

#### Recently Delivered — Phase 5 Hardening (2026-07-20)
- [x] EXS-01..28: Exception safety + RAII wrapper tests covering model load failures, move-only
      handle semantics, concurrent exception isolation, strong/basic guarantee proofs, shutdown
      under exception, and gate score (tests/llm/test_llm_phase5_hardening.cpp)
- [x] MEM-01..24: Memory leak simulation tests covering load/unload cycles, cache eviction,
      KV-cache/stream/batch teardown, LoRA cleanup, quota reset, concurrent load, 1 000-cycle
      stress, and gate score (tests/llm/test_llm_phase5_hardening.cpp)
- [x] 52 new deterministic GTest cases; CTest labels: llm;hardening;phase5; TIMEOUT 120

### Phase 6: Documentation and Acceptance
- [ ] Synchronize operator docs/runbooks with implemented runtime behavior and metrics (Target: Q4 2026)
- [ ] Publish acceptance checklist evidence for release sign-off (Target: Q4 2026)

## Production Readiness Checklist

- [ ] API contracts for inference and streaming verified against tests and docs
- [ ] Security and policy checks verified on all externally reachable LLM entry points
- [ ] Performance expectations validated by reproducible release-profile benchmarks
- [ ] Failure handling validated for cancellation, timeout, and backend degradation cases
- [ ] Audit and changelog documentation synchronized with implementation delta

## Known Issues and Limitations

- Some advanced distributed/remote execution optimizations depend on deployment wiring and are not universal defaults.
- Runtime behavior can vary with enabled backend/plugin combinations and available hardware acceleration.
- Not all benchmark targets currently represent transport- or topology-specific production mixes.

## Wave B (Q1–Q2 2027) Tracking — B3 Multi-Task LoRA Fine-Tuning

### Scope
- [x] Shared LoRA base with task-specific projections
- [x] Domain-gating mechanism
- [x] Joint loss with configurable task weighting
- [x] 3-task benchmark evaluation and robustness checks

### Validation
- [x] Unit tests `MTL-01..10`
- [x] Ablation study: shared vs separate adapters

### Acceptance Gates
- [ ] Average task performance ≥ +8% vs single-task baseline
- [ ] Training-time increase ≤ 15%
- [ ] Robustness across task configurations

### Dependencies
- [ ] Wave A deployment complete (Speculative Decoding, DPR, Fairness)
- [ ] Stable adapter lifecycle and benchmark baselines in LLM module

### References
- Detail tracker: `../ai/FUTURE_ENHANCEMENTS.md`
- Shared bibliography: `../../docs/research/ml_enhancements_bibliography.md`
- Issue scope: `https://github.com/makr-code/ThemisDB/issues/5039`

## Breaking Changes

- No breaking changes planned at roadmap level; any required API break must be explicitly documented in CHANGELOG and migration notes before merge.
