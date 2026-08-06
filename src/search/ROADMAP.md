# Search Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-08-06 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production-capable search runtime exists for hybrid lexical/vector retrieval, distributed shard merge, ranking utilities, and result analytics/stream behavior.

## In Progress

- [~] hardening distributed merge edge behavior under shard failures and overlap variance (Target: Q3 2026)
- [~] improving diagnostics consistency across fusion/rerank utility stages (Target: Q3 2026)
- [~] stabilizing benchmark-backed release guardrails for hybrid/distributed hot paths (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] tighten deterministic behavior under sustained high-concurrency hybrid queries (Target: Q4 2026)
- [ ] expand stress coverage for shard merge failure and k-limit edge paths (Target: Q4 2026)
- [ ] improve operator-facing diagnostics for search degradation triage (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for hybrid and distributed merge paths (Target: Q1 2027)
- [ ] broaden benchmark depth for advanced multimodal and reranking workflows (Target: Q1 2027)
- [ ] harden long-run reliability under sustained search traffic pressure (Target: Q1 2027)
- [~] Wave B B1: Self-RAG retrieval-controller integration for retrieval quality loops (Target: Q1–Q2 2027) — core impl + IEE callback integration + ALCE benchmark done

## Implementation Phases

### Phase 1: Design / API Contract
- [ ] freeze retrieval/fusion/distributed contracts for current major line (Target: Q3 2026)
- [ ] define explicit error taxonomy for search failure classes (Target: Q3 2026)

### Phase 2: Core Implementation
- [ ] complete hardening for hybrid/distributed merge internals (Target: Q4 2026)
- [ ] align utility-layer behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-safe behavior for shard failures, merge limits, and rerank faults (Target: Q4 2026)
- [ ] unify diagnostics across retrieval/fusion/utility incident classes (Target: Q4 2026)

### Phase 4: Tests
- [ ] expand focused regressions for overlap, shard-failure, and candidate-limit scenarios (Target: Q4 2026)
- [ ] extend deterministic stress fixtures for hybrid/distributed workloads (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [ ] lock benchmark-backed release gates for search hot paths (Target: Q4 2026)
- [ ] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core search module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

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