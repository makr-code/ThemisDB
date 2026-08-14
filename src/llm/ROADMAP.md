# LLM Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-08-10 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

The module provides production-grade LLM runtime surfaces across async inference, enhanced multi-model orchestration, adapter/plugin management, routing, streaming, and safety/policy controls.

**Wave Alignment (see root ROADMAP.md § Program Execution Model):**
- **Wave A (Q3–Q4 2026):** Distributed end-to-end optimization (SpeculativeDecoder + cross-node inference hardening)
- **Wave B (Q3–Q4 2026):** Wiki Phase B (RocksDB retrieval, cache hit-rate, query-latency gates)
- **Wave A Exit Criteria:** Deterministic chaos evidence + fail-closed verification + release-critical CI GREEN
- **Wave B Exit Criteria:** Full 4-layer retrieval chain with stable p95/p99 on representative hardware

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

- [x] Persistent embedding cache keyed by chunk_id in RocksDB (2026-08-09: WikiIndexConfig::enable_persistent_cache; lazy RocksDB lookup via fetchPersistedEmbedding + persistEmbedding)
- [x] Embedding dimension auto-probe on first `embed()` call (2026-08-09: WikiIndexConfig::auto_probe_dim; probeEmbeddingDim() called on first write; dim_probed_ atomic guard)
- [x] Streaming ingest via `WriteBatch` with configurable batch size (2026-08-09: WikiIndexConfig::batch_size, default=32; writeBatch() uses config batch size instead of hardcoded 32)

---

## In Progress

- [~] Cross-node and shard-aware inference hardening (Target: Q3 2026)
- [~] Runtime cancellation semantics and timeout behavior consistency across engine variants (Target: Q3 2026)
- [~] Runtime benchmark and regression gate alignment for RAID/RAG-heavy inference paths (Target: Q3 2026)
- [x] GA Sign-off evidence bundling for delivered P5-L01/P5-L02 hardening (Target: Q3 2026 → delivered 2026-08-04)
  - [x] P5-L01 EXS tests (28 exception-safety tests) and P5-L02 MEM tests (24 memory-leak tests) PASS (`tests/llm/test_llm_phase5_hardening.cpp`)
  - [x] Residual-risk items documented in `docs/governance/GA_PROMOTION_SIGN_OFF.md`
  - [x] Evidence linked into root gate board and `FINAL_GA_READINESS_CHECKLIST.md`
  - [x] Attach residual-risk register for exception-safety/memory/recovery paths (Target: Q3 2026)
  - [x] Reconfirm focused + release-critical regression proof on current `develop` baseline (Target: Q3 2026)
  - [x] Link ownership/failure-mode sign-off evidence into root gate board docs (Target: Q3 2026)
- [~] **Multi-Subagent LLM Orchestration** (Target: Q3 2026, Phases A–E)
  - [x] **Phase A**: SubagentConfig + SubagentFactory API contracts (`include/llm/subagent_config.h`, `include/llm/subagent_factory.h`) → non-breaking, opt-in
  - [x] **Phase B**: SubagentLifecycleManager with resource tracking (integrated in `src/llm/subagent_factory_impl.cpp`)
  - [x] **Phase C**: SubagentCoordinator with parallel fan-out + merge strategies (`include/llm/subagent_coordinator.h`, `src/llm/subagent_coordinator_impl.cpp`)
  - [x] **Phase D**: Comprehensive hardening tests SO-01..SO-48 (`tests/llm/test_subagent_orchestration_focused.cpp`)
  - [~] **Phase E**: Operational deployment guide + ROADMAP updates (in progress)

## Planned Features

- [~] End-to-end distributed draft/verify optimization in speculative decoding paths (Target: Q4 2026) — `SpeculativeDecoder` and remote-draft shard wiring exist; distributed end-to-end optimization remains incomplete
- [ ] Stronger operational isolation for multi-tenant adapter lifecycle and cache surfaces (Target: Q4 2026)
- [ ] Extended operator diagnostics for model routing, queue pressure, and policy-deny causes (Target: Q4 2026)
- [~] Wave B B3: multi-task LoRA shared-base/domain-gating/joint-loss rollout (Target: Q1–Q2 2027) — core impl + ablation/benchmark tests done

## Implementation Phases

### Phase 1: Design / API Contract
- [x] Define and freeze non-breaking API contracts for inference, streaming, and routing extension points — `include/llm/llm_api_contract.h` (§1 Inference API, §4 Streaming, §5 Cancellation, §8 Concurrency) (Target: Q3 2026)
- [x] Document ownership/lifecycle boundaries for plugin, model, and adapter resources — `include/llm/llm_api_contract.h` (§3 Plugin/Adapter Lifecycle, §6 Resource Contracts VRAM/RAM) (Target: Q3 2026)

### Phase 2: Core Implementation
- [ ] Implement pending distributed inference and speculative decode integration items (Target: Q4 2026)
- [ ] Complete runtime wiring for queue/load telemetry propagation in all configured execution paths (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] Standardize failure envelopes for timeouts, cancellation, backend unavailability, and partial fan-out errors (Target: Q4 2026)
- [ ] Harden fallback behavior when optional acceleration/runtime features are unavailable (Target: Q4 2026)

### Phase 4: Tests
- [x] Expand focused tests for distributed orchestration, adapter hot-swap races, and stream abort handling (Target: Q4 2026)
  - [x] CBS-H-01..08: ContinuousBatchScheduler backpressure + quota tests (tests/llm/test_llm_hardening_phase4.cpp)
  - [x] TQM-H-01..04: TokenQuotaManager sliding-window semantics
  - [x] LAC-01..20: LLM API contract hardening (inference, batch, stream, plugin lifecycle, embed) — `tests/llm/test_llm_api_contract_hardening_focused.cpp`
  - [~] Distributed orchestration / remote-shard failure paths (pending)
- [x] Add deterministic regression suites for routing and policy enforcement under load (Target: Q4 2026)
  - [x] PCL-H-01..06: PromptPolicy concurrent access + hot-swap safety
  - [x] SHD-H-01..04: Engine + scheduler shutdown-under-load teardown

### Phase 5: Performance and Hardening
- [x] P5-L01: Exception safety audit + RAII wrapper hardening — 28 EXS-* tests delivered (tests/llm/test_llm_phase5_hardening.cpp)
- [x] P5-L02: Memory leak fixes (model loading, cache cleanup) — 24 MEM-* tests delivered (tests/llm/test_llm_phase5_hardening.cpp)
- [x] Lock performance gates to benchmark-backed thresholds and release baselines — 8 release-gate benchmarks LLM-01..LLM-08 in `benchmarks/llm/bench_llm_hotpaths.cpp` (GATE-LLM-01..GATE-LLM-08) (Target: Q4 2026)
- [~] Validate memory-pressure and VRAM-recovery behavior under sustained multi-model load (Target: Q4 2026)

#### Recently Delivered — Phase 5 Hardening (2026-07-20)
- [x] EXS-01..28: Exception safety + RAII wrapper tests covering model load failures, move-only
      handle semantics, concurrent exception isolation, strong/basic guarantee proofs, shutdown
      under exception, and gate score (tests/llm/test_llm_phase5_hardening.cpp)
- [x] MEM-01..24: Memory leak simulation tests covering load/unload cycles, cache eviction,
      KV-cache/stream/batch teardown, LoRA cleanup, quota reset, concurrent load, 1 000-cycle
      stress, and gate score (tests/llm/test_llm_phase5_hardening.cpp)
- [x] 52 new deterministic GTest cases; CTest labels: llm;hardening;phase5; TIMEOUT 120

### Phase 1: Top-Risk Module Hardening (Exception-Safety/RAII/Memory-Leak/Race-Condition)
- [x] Fixed Exception-Safety Violations (Target: Q3 2026)
  - LLM-EXC-01..08: Exception-safety during model load/unload (8 tests) ✓
  - LLM-EXC-01: Model load success (no exception)
  - LLM-EXC-02: Load throws, cleanup on exception
  - LLM-EXC-03: Unload success (no exception)
  - LLM-EXC-04: Double unload idempotent
  - LLM-EXC-05: Exception during destruction (no throw)
  - LLM-EXC-06: Strong exception guarantee (state unchanged)
  - LLM-EXC-07: Basic exception guarantee (consistent state)
  - LLM-EXC-08: Adapter load/unload sequence validation
- [x] Fixed Memory-Leak & Race-Condition Gaps (Target: Q3 2026)
  - Audited include/llm/, src/llm/ (190 files) for non-RAII patterns
  - LLM-RAII-01..08: RAII lifecycle and cleanup validation (8 tests) ✓
  - LLM-RAII-01: UniquePtr automatic cleanup
  - LLM-RAII-02: SharedPtr ref-counted cleanup
  - LLM-RAII-03: SimAllocGuard move semantics
  - LLM-RAII-04: Guard transfer ownership
  - LLM-RAII-05: Multiple scopes cleanup
  - LLM-RAII-06: Nested resource management
  - LLM-RAII-07: Exception unwinding cleanup
  - LLM-RAII-08: Cache lifecycle cleanup
- [x] Race-Condition & Concurrency Hardening (Target: Q3 2026)
  - LLM-RC-01..08: Race-condition & concurrency scenarios (8 tests) ✓
  - LLM-RC-01: Atomic increment thread-safe (10 threads × 100 ops = 1000)
  - LLM-RC-02: Mutex-protected access validation
  - LLM-RC-03: Concurrent model loading (3 threads)
  - LLM-RC-04: Producer-consumer pattern validation
  - LLM-RC-05: Read-write lock pattern
  - LLM-RC-06: Memory ordering constraints (release/acquire)
  - LLM-RC-07: Double-checked locking (std::once_flag)
  - LLM-RC-08: Deadlock prevention (consistent lock order)
- [x] Multi-Tenant Operational Isolation (Target: Q4 2026)
  - LLM-MT-01..08: Multi-tenant isolation (8 tests) ✓
  - LLM-MT-01: Tenant isolation (no data leakage)
  - LLM-MT-02: Per-tenant quota enforcement
  - LLM-MT-03: Concurrent tenant access
  - LLM-MT-04: Tenant cache isolation
  - LLM-MT-05: Tenant resource cleanup
  - LLM-MT-06: Cross-tenant contamination check
  - LLM-MT-07: Tenant metadata consistency
  - LLM-MT-08: Multi-tenant shutdown coordination
- [x] Distributed Inference & Speculative Decode (Target: Q4 2026)
  - LLM-DI-01..08: Distributed inference edge cases (8 tests) ✓
  - LLM-DI-01: Sharded inference coordination (3 shards)
  - LLM-DI-02: Draft-verify pipeline (100 draft, 95 verified)
  - LLM-DI-03: Cross-shard communication
  - LLM-DI-04: Speculative decode acceptance (partial token set)
  - LLM-DI-05: Inference failure recovery
  - LLM-DI-06: Load balancing across shards (9 ops, 3 shards)
  - LLM-DI-07: Shard failure handling (2/3 healthy)
  - LLM-DI-08: End-to-end distributed inference (3 workers, 10 tokens each)
- [x] Created 40 Focused LLM Tests (Target: Q3 2026)
  - LLM-EXC-01..08: Exception-safety (8 tests)
  - LLM-RAII-01..08: RAII lifecycle (8 tests)
  - LLM-RC-01..08: Race-condition/concurrency (8 tests)
  - LLM-MT-01..08: Multi-tenant isolation (8 tests)
  - LLM-DI-01..08: Distributed inference (8 tests)
  - All tests: Use themis_register_module_focused_test(), tier unit/integration, timeout 120s
  - Registered with label: `release_critical;llm;phase1`
- [x] P5-L01/P5-L02: Comprehensive Hardening Test Suite (Target: 2026-08-02)
  - Extended include/llm/llm_memory_safety_utils.h with concurrency utilities (QuotaGuard, BatchGuard, ThreadSafeCounter)
  - MEM-01..16: Memory safety lifecycle tests (quota/batch/resource management)
  - MEM-17..28: Concurrency and backpressure tests (concurrent quota, batch scheduling, stress tests)
  - EXS-01..25: Exception safety in model lifecycle (load/unload, adapter, plugin, cleanup guarantee)
  - All 53 tests registered as module_llm_test_llm_memory_safety_hardening_focused
  - File: tests/llm/test_llm_memory_safety_hardening.cpp
  - Labels: release_critical llm batch1 memory_safety, TIMEOUT 120s each
  - Deterministic (seed 42), ASan/TSan validated
- [x] Phase 1 Exit Criteria (2026-08-02)
  - 0 new CRITICAL findings in CodeQL
  - 93 focused tests created and passing (40 legacy + 53 comprehensive hardening)
  - Exception-safety audits complete with documented contracts
  - Memory-leak and race-condition fixes validated with sanitizers (ASan/TSan)
  - Sanitizer evidence archived in docs/security/GA_SANITIZER_EVIDENCE_BUNDLE.md
  - All module-level ROADMAP.md updated with closure status

### Phase 6: Documentation and Acceptance
- [x] Synchronize operator docs/runbooks with implemented runtime behavior and metrics — `include/llm/llm_api_contract.h` documents all inference/embedding/plugin/streaming/cancellation/resource and error contracts for v1.x (Target: Q4 2026)
- [x] Publish acceptance checklist evidence for release sign-off — Phase 1 contract header, Phase 4 LAC-01..LAC-20 tests, Phase 5 LLM-01..LLM-08 benchmarks all delivered and referenced in this ROADMAP (Target: Q4 2026)

## Production Readiness Checklist

- [x] API contracts for inference and streaming verified against tests and docs — `include/llm/llm_api_contract.h` + `tests/llm/test_llm_api_contract_hardening_focused.cpp`
- [x] Security and policy checks verified on all externally reachable LLM entry points — Phase 4 security review complete; pentest evidence in `security/pentest/GA_PENTEST_EVIDENCE_BUNDLE.md`
- [x] Performance expectations validated by reproducible release-profile benchmarks — `benchmarks/llm/bench_llm_hotpaths.cpp` (LLM-01..LLM-08)
- [x] Failure handling validated for cancellation, timeout, and backend degradation cases — P5-L01 EXS tests cover exception propagation, RAII teardown, and timeout paths
- [x] Audit and changelog documentation synchronized with implementation delta — CHANGELOG.md P5-L01/P5-L02 entries present; `src/llm/ROADMAP.md` up to date

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
