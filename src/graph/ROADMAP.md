# Graph Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-20 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · MODULE_GAPS.md · ../../NEXT_PHASE_IMPLEMENTATION_PLAN.md -->

## L0 Verification Status (2026-06-25) — **0 REAL GAPS**

**Status**: All 9 L0 findings verified and reclassified as defensive patterns

| Finding Type | Count | Classification | Severity (L0.5) | Status |
|---|---|---|---|---|
| Guarded Precondition Checks | 8 | GUARDED_STUB | INFO | Production-ready |
| False Positive (non-existent file) | 1 | FALSE_POSITIVE | IGNORE | Scanner artifact |
| **TOTAL VERIFIED GAPS** | **0** | **—** | **—** | **✅ READY** |

**Pattern Examples**:
- `explain_plan::toDot()` line 68: Empty plan → empty DOT (correct semantics)
- `ontology_manager::parseString()` line 73: Parse error → empty string (documented behavior)
- `rotate_completion::entityEmbedding()` line 95: Untrained model → empty vector (defensive guard)

**L0.5 Analysis**: All findings follow standard defensive programming patterns with semantic correctness. No implementation blockers.

**Reference**: `ai_working/gap_scanner_verified_graph.json` (timestamp: 2026-06-25T14:45:00)

---

## Current Status

Production graph runtime exists across query planning, constraint-aware traversal, rewrite/explain support, parallel and distributed execution, and reasoning-oriented graph capabilities.

**Hybrid Retrieval Rollout Readiness**: 60% 🟢 (issue #5468).
- L0 gap verification: 0 real gaps — all 9 findings verified as defensive patterns (2026-06-25).
- Phase A (exact-first, CPU-first graph truth): ✅ Ready. Graph truth is exact and CPU-first in all phases.
- Phase B (graph validation): ✅ Ready with minor error-path hardening.
- Phase C (hybrid planning): ✅ Ready for exact graph layer; Category B kernels conditional.
- **Invariant**: Category C kernels (policy, provenance, transactions) are CPU-first permanently.
  GPU acceleration is never permitted on policy enforcement or transaction paths.
- Rollout risk detail: `ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md §7`

## In Progress

- [~] **Phase 1 API Contract Freeze** (2026-08-01): ✅ COMPLETE
  - [x] Error taxonomy frozen (44 error codes, 3 categories: DENIAL/FALLBACK/REASONING_CONFLICT)
  - [x] API contracts documented for all 4 major component layers
  - [x] Error context diagnostics framework implemented
  - [x] Phase 1 validation test suite (test_graph_error_taxonomy_phase1.cpp)
  - Deliverables: `graph_error_taxonomy.h/cpp`, `GRAPH_API_CONTRACTS_PHASE1.md`

- [~] **Phase 2-6 Tasks** (Target: Q3-Q4 2026)
  - [~] hardening GPU/distributed traversal parity and fallback determinism in mixed-capability environments (Target: Q4 2026)
  - [~] benchmark stabilization for optimizer, traversal, and tensor-fingerprint graph hot paths (Target: Q4 2026) — Benchmark framework structure in development (2026-08-07)
  - [~] diagnostics consistency for constraint denial, fallback, and reasoning conflict incidents (Target: Q4 2026)
  - [~] hybrid retrieval rollout Phase A entry: error path and thread-safety hardening (Target: Q3-Q4 2026)

## Planned Features

### Hybrid Retrieval Rollout Gates (issue #5468)
- [~] Phase A gate: fix 50% of error-handling gaps (195 → ~98) in exact traversal paths (Target: Q3 2026)
- [~] Phase A gate: fix 50% of thread-safety gaps (240 → ~120) under concurrent access — **GraphLRUPlanCache complete** (std::lock_guard on all put/get/erase/clear/size paths, 2026-08-10); ontology manager, knowledge graph reasoner, tensor fingerprint, and scheduled edge refresh protections in progress (Target: Q3 2026)
- [x] Phase A ctest gate: `test_graph_exact_traversal` with error injection (tests/graph/test_graph_exact_traversal.cpp: BFS + Dijkstra failure injection present)
- [ ] Phase C gate: Category B kernel fallback paths hardened (BFS frontier cutoff, Dijkstra overflow) (Target: Q4 2026)
- [ ] Permanent invariant: `ann_frontdoor_route_type` metric confirms no GPU path for Category C (ongoing)

### Short-term (3-6 months)
- [ ] tighten deterministic behavior for high fan-out constrained traversals under mixed query shapes (Target: Q4 2026)
- [ ] extend stress coverage for long-running parallel/distributed graph query workloads (Target: Q4 2026)
- [ ] improve operator-facing diagnostics for explain/rewrite and fallback incidents (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for optimizer and traversal pathways (Target: Q1 2027)
- [ ] broaden benchmark depth for semantic reasoning and incremental refresh workflows (Target: Q1 2027)
- [ ] harden reliability under sustained multi-tenant graph execution pressure (Target: Q1 2027)
- [~] Wave B B2: RotatE link-prediction integration with `KnowledgeGraphReasoner` (Target: Q1–Q2 2027) — core impl + KGC-01..15 tests done

### Distributed Maturity Phase 3 — Track 2 Items (Q3–Q4 2026)

These items are part of the next-phase **Track 2: Distributed Systems Maturity — 3.3 Graph** plan
(see `ROADMAP.md §Track 2`). Hard gate per item: deterministic under-load benchmark + `release_critical` CI green.

- [ ] **Cross-shard graph query execution**: extend the distributed graph orchestrator to execute
  traversal queries that span multiple shards without requiring full graph materialization on the
  coordinator; partial traversal results merged at the coordinator level (Target: Q4 2026)
  - Inputs: traversal start vertex (may reside on any shard); traversal depth limit; result merge strategy
  - Acceptance: correct traversal result for 4-shard graph with 1M edges total; throughput ≥ 50%
    of single-shard baseline for depth-3 BFS; `release_critical` green
- [ ] **Distributed Betweenness Centrality (BC)**: implement parallel Brandes BC algorithm with
  work distributed across available nodes; coordinator collects and merges partial sigma/delta
  accumulators; support approximation mode for large graphs (Target: Q4 2026)
  - Inputs: graph name, sample fraction (approximation mode), parallelism hint
  - Acceptance: exact BC matches NetworkX reference for graphs ≤ 10K nodes; approximate BC
    within 5% error for graphs ≤ 1M nodes; runtime ≤ 60 s for 1M-node graph on 4 nodes

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze planning/traversal/semantic/tensor utility contracts for active major line (Target: Q3 2026) — **COMPLETE 2026-08-01**
  - Frozen API contracts documented in `GRAPH_API_CONTRACTS_PHASE1.md`
  - All 10 major components have explicit preconditions, postconditions, thread-safety contracts
  - Breaking change policy established (requires full team review + deprecation period)
- [x] define explicit error taxonomy for denial, fallback, and reasoning conflict classes (Target: Q3 2026) — **COMPLETE 2026-08-01**
  - 44 error codes across 7 component categories (Optimizer, Traversal, Reasoning, Tensor, Distributed, Resource, Generic)
  - Category semantics frozen: DENIAL (fix preconditions), FALLBACK (retry/CPU), REASONING_CONFLICT (operator intervention)
  - Error description lookup, diagnostics, and validation helpers implemented in `graph_error_taxonomy.h/cpp`

### Phase 2: Core Implementation
- [ ] complete hardening for optimizer, traversal, and distributed orchestration internals (Target: Q4 2026)
- [ ] align semantic/reasoning and tensor utility behavior with bounded runtime contracts (Target: Q4 2026)
- [ ] stage query-optimizer work before cache/resource/scheduling follow-on items in the current Phase 3 execution order (Target: Q3 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-safe behavior for invalid constraints and degraded acceleration routes (Target: Q4 2026)
- [ ] unify diagnostics across denial, fallback, and semantic conflict incidents (Target: Q4 2026)

### Phase 4: Tests
- [ ] expand focused regressions for mixed constraint/distribution/acceleration edge scenarios (Target: Q4 2026)
- [ ] extend deterministic stress fixtures for high fan-out and long-path workloads (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for graph hot paths (Target: Q4 2026) — GATE-GRG-01..06 benchmarks delivered (2026-08-07)
- [~] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026) — Benchmark execution in progress (2026-08-07)

### Phase 6: Documentation and Acceptance
- [x] core graph module docs aligned to source-verifiable behavior (2026-08-07)
- [x] roadmap/future planning separated from historical changelog entries (2026-08-07)
- [x] error taxonomy frozen (44 error codes) in graph_error_taxonomy.h (Phase 1, 2026-08-07)
- [x] API contracts documented in GRAPH_API_CONTRACTS_PHASE1.md (Phase 1, 2026-08-07)

## Production Readiness Checklist

- [x] core graph surfaces documented and source-verified (2026-08-07)
- [x] module-level security and failure behavior documented (Phase 1: error taxonomy in graph_error_taxonomy.h, 2026-08-07)
- [x] benchmark mapping documented in performance expectations (Phase 5: GATE-GRG-01..06 benchmarks, 2026-08-07)
- [~] remaining hardening tasks in progress for distributed/GPU/semantic edge paths (target Q4 2026)
- [~] release benchmark stabilization in progress (Phase 5, benchmark framework ready 2026-08-07)

## Known Issues and Limitations

- runtime behavior depends on graph shape, constraints, and enabled acceleration capabilities.
- advanced distributed/GPU and semantic reasoning edge scenarios need continued hardening.
- benchmark breadth should continue expanding for specialized graph workflows.

## Wave B (Q1–Q2 2027) Tracking — B2 Knowledge Graph Completion (RotatE)

### Scope
- [x] RotatE embedding model (relation-as-rotation) implementation
- [x] Triple loss with negative sampling
- [x] Link-prediction head for ranked completion results
- [x] Integration with `KnowledgeGraphReasoner`

### Validation
- [x] Unit tests `KGC-01..15`
- [ ] Benchmark vs TransE baseline (FB15k-237)

### Acceptance Gates
- [ ] MRR ≥ 0.35, Hits@10 ≥ 0.55 on FB15k-237
- [ ] Inference latency ≤ 50 ms for top-20 predictions
- [ ] Zero backward compatibility breaks

### Dependencies
- [ ] `KnowledgeGraphReasoner` stability and benchmark baseline complete
- [ ] Wave A deployment complete (Speculative Decoding, DPR, Fairness)

### References
- Detail tracker: `../ai/FUTURE_ENHANCEMENTS.md`
- Shared bibliography: `../../docs/research/ml_enhancements_bibliography.md`
- Issue scope: `https://github.com/makr-code/ThemisDB/issues/5039`

## Breaking Changes

No breaking graph contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.
