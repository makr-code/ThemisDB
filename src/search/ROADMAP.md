# Search Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-08-06 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production-capable search runtime with Phase 1 (Design/API Contract Freeze) complete as of 2026-08-06.

### Phase 1 Completion Summary (2026-08-06)
- [x] Frozen retrieval/fusion/distributed contracts (HybridSearch v2.0.0, DistributedHybridSearch v2.1.0, SearchResultStream v2.0.0)
- [x] Defined explicit error taxonomy via search_error_codes.h (32 error codes across 5 categories)
- [x] Documented contract invariants in ARCHITECTURE.md
- [x] Fixed CRITICAL gaps: exception_in_destructor (hybrid_search.cpp), no_timeout (search_result_stream.cpp with 30s default)
- [x] Contract boundaries documented for core components (retrieval, fusion, distributed, utility, analytics)

## In Progress

- [~] Phase 2: Core Implementation Hardening (Q4 2026)
  - Distributed merge edge-case resilience enhancement
  - Bounded resource limit enforcement across fusion/utility paths
  - Cross-component resilience testing for partial backend failures

## Planned Features

### Short-term (Q4 2026)
- [ ] Phase 2: Core Implementation Hardening
  - [ ] Enhanced distributed_hybrid_search shard-failure handling
  - [ ] Bounded resource limits enforced across all fusion/expansion/reranking paths
  - [ ] Cross-component resilience tests for partial backend failures
  - Target: Q4 2026

- [ ] Phase 3: Error Handling and Edge Cases
  - [ ] Standardize fail-safe behavior for shard failures, merge limits, rerank faults
  - [ ] Unify diagnostics across retrieval/fusion/utility incident classes
  - Target: Q4 2026

- [ ] Phase 4: Test Expansion
  - [ ] test_search_distributed_merge_stress.cpp with 8+ deterministic stress cases
  - [ ] test_search_edge_cases_*.cpp files covering all 14 components
  - Target: Q4 2026

### Mid-term (Q1 2027)
- [ ] Phase 5: Performance Gatekeeping
  - [ ] Benchmark suite completion (SRCP-1, SRCP-2, SRCP-3 + advanced scenarios)
  - [ ] Release gate execution with regression ≤10% baseline
  - [ ] Benchmark documentation in benchmarks/search/README.md
  - Target: Q1 2027

- [ ] Phase 6: Documentation & Acceptance
  - [ ] Doxygen file headers across all 14 components with maturity metadata
  - [ ] PRODUCTION_REQUIREMENTS.md
  - [ ] FINAL_ACCEPTANCE_CHECKLIST.md
  - Target: Q1 2027

## Implementation Phases

### Phase 1: Design / API Contract Freeze (COMPLETE 2026-08-06)
- [x] freeze retrieval/fusion/distributed contracts for current major line
- [x] define explicit error taxonomy for search failure classes
- [x] document contract boundaries for all 14 search components
- **Deliverables:** HybridSearch v2.0.0, DistributedHybridSearch v2.1.0, SearchResultStream v2.0.0, search_error_codes.h v1.0.0
- **Status:** COMPLETE

### Phase 2: Core Implementation Hardening (Q4 2026)
- [ ] complete hardening for hybrid/distributed merge internals
- [ ] align utility-layer behavior to bounded runtime contracts
- **Deliverables:** Enhanced distributed_hybrid_search with explicit shard-failure handling; bounded resource limits enforced across all fusion/expansion/reranking paths; cross-component resilience tests
- **Status:** IN PROGRESS

### Phase 3: Error Handling and Edge Cases (Q4 2026)
- [ ] standardize fail-safe behavior for shard failures, merge limits, and rerank faults
- [ ] unify diagnostics across retrieval/fusion/utility incident classes
- **Deliverables:** SearchStats with degradation flags (shards_failed, partial_result, rerank_fallback); edge case tests P3-01..P3-08; conformance tests for all 14 components
- **Status:** PENDING

### Phase 4: Test Expansion (Q4 2026)
- [ ] expand focused regressions for overlap, shard-failure, and candidate-limit scenarios
- [ ] extend deterministic stress fixtures for hybrid/distributed workloads
- **Deliverables:** test_search_distributed_merge_stress.cpp; test_search_edge_cases_*.cpp files; CTest labels for categorization
- **Status:** PENDING

### Phase 5: Performance Gatekeeping (Q4 2026, Q1 2027)
- [ ] lock benchmark-backed release gates for search hot paths
- [ ] validate p95/p99 and throughput behavior against release baselines
- **Deliverables:** SRCP-1, SRCP-2, SRCP-3 gates + advanced multimodal/learning-to-rank benchmarks; regression ≤10% baseline; benchmark documentation
- **Status:** PENDING

### Phase 6: Documentation and Acceptance (Q1 2027)
- [ ] Doxygen file headers across all 14 components with maturity metadata
- [ ] Comprehensive PRODUCTION_REQUIREMENTS.md
- [ ] Module FINAL_ACCEPTANCE_CHECKLIST.md
- [ ] Documentation governance sync
- [x] core search module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- **Status:** PENDING

## Production Readiness Checklist

- [x] core search surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [ ] remaining hardening tasks closed for distributed merge/utility edge paths
- [ ] release benchmark stabilization complete

## Known Issues and Limitations

- runtime behavior depends on candidate sizing, shard topology, and utility-stage configuration.
- selected distributed merge and rerank edge scenarios need continued hardening.
- benchmark depth should continue expanding for advanced search workflows.
- Wave B B1 work depends on upstream Wave A deployment and LLM latency prerequisites.

## Wave B (Q1–Q2 2027) Tracking — B1 Self-RAG Search Integration

### Scope
- [x] retrieval-controller decision hooks for selective re-retrieval
- [x] retrieval quality signals for critic feedback (Relevant/Partial/Irrelevant)
- [x] bounded retrieval refinement integration support (max 3 rounds)
- [x] search-path integration support for `InferenceEngineEnhanced` callback loops

### Validation
- [x] unit tests `SELF_RAG-SEARCH-01..08` (covered by SELF_RAG-01..12)
- [x] ALCE retrieval-quality benchmark contribution vs vanilla RAG baseline

### Acceptance Gates
- [ ] precision@k retrieval contribution ≥ 0.85 on golden-doc tests
- [ ] retrieval-path latency overhead remains within Self-RAG budget (≤ 1.5× overall baseline)
- [ ] deterministic fallback behavior under shard/backend partial failures

### Dependencies
- [ ] Wave A deployment complete (Speculative Decoding, DPR, Fairness)
- [ ] LLM inference P95 latency < 200 ms for iterative loops
- [ ] RAG module Self-RAG controller/critic interfaces stabilized

### References
- AI tracker: `../ai/ROADMAP.md`
- RAG tracker: `../rag/ROADMAP.md`
- Shared bibliography: `../../docs/research/ml_enhancements_bibliography.md`
- Issue scope: `https://github.com/makr-code/ThemisDB/issues/5039`

## Planning Traceability

- Wave B dependency planning issue: `#5039`
- Upstream planning context: Wave C `#5040`, Wave A `#5038`

## Breaking Changes

No breaking search contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.

## Evidence and Verification

### Build Infrastructure
- Build target: module_search_* targets registered in tests/search/CMakeLists.txt
- Test framework: GTest with module_search_<test_stem>_focused naming convention
- Compilation model: source files required: hybrid_search.cpp, distributed_hybrid_search.cpp, faceted_search.cpp, query_expander.cpp (per tests/search/CMakeLists.txt:18-23)
- Build status: Verified 2026-08-06 — search module test infrastructure is correctly registered with themis_register_module_focused_test() and TIMEOUT 120s

### Test Evidence
- Focused test suite: tests/search/test_search_analytics.cpp, test_search_future_interfaces.cpp, test_search_highlighter.cpp
- Test labels: search module tests registered with LABELS search for ctest filtering
- Test tier: unit tier (per tests/search/CMakeLists.txt:40)
- Evidence gap: Full build/test run evidence pending RocksDB and fmt dependency resolution (documented 2026-08-06)

### Audit and Documentation Evidence
- AUDIT.md: Module source verification completed; core search surfaces present and documented
- Verified compliance: Source-verifiable behavior claims (pass), Structured forward planning (pass), Historical tracking (pass), Core module docs synchronized (pass)
- Open audit findings: Three medium/low-severity items tracked (SEA-AUD-01, SEA-AUD-02, SEA-AUD-03) — distributed merge hardening, fusion diagnostics, benchmark depth

### Production Readiness Status
- [x] core search surfaces documented and source-verified (verified per AUDIT.md)
- [x] module-level security and failure behavior documented (per SECURITY.md)
- [x] benchmark mapping documented in performance expectations (per PERFORMANCE_EXPECTATIONS.md)
- [ ] remaining hardening tasks closed for distributed merge/utility edge paths (in progress per roadmap priorities)
- [ ] release benchmark stabilization complete (Q3 2026 target, in progress)

### Evidence Justification
- Full build/test run: Currently blocked by transitive dependency (librocksdb-dev); module structure and test registration verified as conformant to module testing policy (2026-08-06)
- Test coverage depth: Audit report indicates focused test presence with unit tier classification; integration and wave-level coverage tracked separately in top-level test integration suite
- Benchmark evidence: Performance expectations documented and benchmarks defined; release gate execution evidence pending Q3 2026 stabilization completion