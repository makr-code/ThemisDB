---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-11-09
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
Source level: level1
SOT domain: module-behavior
Canonical source references:
  - ../../ROADMAP.md
  - ../../include/graph/
  - ../../src/graph/
  - ../../tests/graph/
Related issue: #7424 (research and wave-plan proposal)
AI_Model_Note: Exact model backend not verifiable from final document text alone.
---

# Graph Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Historical status snapshot validated: 2026-07-20; current claims require revalidation. -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · MODULE_GAPS.md · ../../NEXT_PHASE_IMPLEMENTATION_PLAN.md -->

> **Status notice:** Checkboxes, scanner counts, maturity/readiness statements, and targets in
> this accumulated roadmap are historical planning records, not current validation. The root
> [`ROADMAP.md`](../../ROADMAP.md) is the repository maturity aggregate. `ai_working/` reports
> referenced below are not current source-of-truth evidence.

## Current source inventory (2026-10-31)

Current code includes optimizer, path constraints, local/parallel and device-aware traversal,
registered shard executors, ontology/reasoning, RotatE, and graph utility sources. The focused
test and benchmark source inventories are registered through their local CMake files. File
presence does not establish feature parity, production readiness, or passing tests.

## L0 Verification Status (2026-06-25) — **0 REAL GAPS**

> **Historical snapshot:** the following values and classifications reproduce the older scanner
> report; they were not independently revalidated for this documentation update and are not
> current status.

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

**Historical reference recorded by the snapshot**: the archived copy of
[`gap_scanner_verified_graph.json`](../../docs/ARCHIVED/ai-working-history/gap_scanner_verified_graph.json)
(timestamp: 2026-06-25T14:45:00). The original `ai_working/` path is no longer active; this
archived scan is historical only and does not establish current readiness.

---

## Current Status

> **Historical status narrative:** the maturity/readiness statements in this section are not
> current source verification.

The earlier roadmap described graph runtime across query planning, constraint-aware traversal,
rewrite/explain support, parallel/distributed execution, and reasoning capabilities. This
historical summary is not a current readiness assessment.

**Hybrid Retrieval Rollout Readiness**: 60% 🟢 (issue #5468).
- L0 gap verification: 0 real gaps — all 9 findings verified as defensive patterns (2026-06-25).
- Phase A (exact-first, CPU-first graph truth): ✅ Ready. Graph truth is exact and CPU-first in all phases.
- Phase B (graph validation): ✅ Ready with minor error-path hardening.
- Phase C (hybrid planning): ✅ Ready for exact graph layer; Category B kernels conditional.
- **Invariant**: Category C kernels (policy, provenance, transactions) are CPU-first permanently.
  GPU acceleration is never permitted on policy enforcement or transaction paths.
- Historical rollout-risk reference: the archived
  [Hybrid Retrieval Rollout Plan](../../docs/ARCHIVED/ai-working-history/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md),
  §7. The plan is a historical snapshot, not current rollout or readiness evidence.

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
- [x] Phase C gate: Category B kernel fallback paths hardened (BFS frontier cutoff, Dijkstra overflow) (Target: Q4 2026)
- [x] Permanent invariant: `ann_frontdoor_route_type` metric confirms no GPU path for Category C (ongoing)

### Short-term (3-6 months)
- [x] tighten deterministic behavior for high fan-out constrained traversals under mixed query shapes (Target: Q4 2026)
- [x] extend stress coverage for long-running parallel/distributed graph query workloads (Target: Q4 2026)
- [x] improve operator-facing diagnostics for explain/rewrite and fallback incidents (Target: Q4 2026)

### Mid-term (6-12 months)
- [~] re-baseline p95/p99 envelopes for optimizer and traversal pathways (Target: Q1 2027)
- [~] broaden benchmark depth for semantic reasoning and incremental refresh workflows (Target: Q1 2027)
- [~] harden reliability under sustained multi-tenant graph execution pressure (Target: Q1 2027)
- [x] RotatE link-prediction implementation and reasoner-injection surface exist in current source;
  the old target date is historical. The root roadmap now distinguishes the acceptance-test floor
  from the higher MRR/Hits@10 targets, which remain unverified pending a separately recorded
  evaluation.

### Distributed Maturity Phase 3 — Track 2 Items (Q3–Q4 2026)

These items are part of the next-phase **Track 2: Distributed Systems Maturity — 3.3 Graph** plan
(see `ROADMAP.md §Track 2`). Hard gate per item: deterministic under-load benchmark + `release_critical` CI green.

- [~] **Cross-shard graph query execution**: extend the distributed graph orchestrator to execute
  traversal queries that span multiple shards without requiring full graph materialization on the
  coordinator; partial traversal results merged at the coordinator level (Target: Q4 2026)
  - Inputs: traversal start vertex (may reside on any shard); traversal depth limit; result merge strategy
  - Acceptance: correct traversal result for 4-shard graph with 1M edges total; throughput ≥ 50%
    of single-shard baseline for depth-3 BFS; `release_critical` green
- [~] **Distributed Betweenness Centrality (BC)**: implement parallel Brandes BC algorithm with
  work distributed across available nodes; coordinator collects and merges partial sigma/delta
  accumulators; support approximation mode for large graphs (Target: Q4 2026)
  - Inputs: graph name, sample fraction (approximation mode), parallelism hint
  - Acceptance: exact BC matches NetworkX reference for graphs ≤ 10K nodes; approximate BC
    within 5% error for graphs ≤ 1M nodes; runtime ≤ 60 s for 1M-node graph on 4 nodes

## Proposed Release Roadmap: v2.5.0 and v3.0.0 (Issue #7424)

> **Status:** Research-backed planning proposal, not a release commitment or evidence of
> implementation. The root roadmap currently identifies v2.4.0-alpha; `VERSIONING.md` names
> v2.5.0 as the next planned MINOR. No v3.0.0 graph commitment was found. Priorities and wave
> order below require normal release and human approval.

### Current module assessment (source-backed, 2026-11-06)

| Area | Observed capability | Gap / interpretation |
|---|---|---|
| Performance and scale | Planner, local/parallel traversal, GPU-assisted CSR traversal, distributed orchestration, and benchmark sources exist. | The dedicated gate benchmark uses a 64-node synthetic graph and prints threshold warnings without failing its process; no representative graph benchmark run or hardware baseline is recorded here. Treat thresholds as proposals, not results. (`include/graph/gpu_traversal.h`; `benchmarks/graph/bench_graph_dedicated_gates.cpp`; `benchmarks/graph/README.md`) |
| Use and query behavior | Constraint, explain/rewrite, ontology/reasoning, and traversal surfaces are represented in headers, source, and focused tests. | The module inventory does not establish end-to-end application workload coverage or a GQL conformance claim. Evaluate query patterns and caller integration separately. (`include/graph/`; `tests/graph/`) |
| Security and resilience | Path constraints expose depth/result bounds in selected APIs; error taxonomy and injected distributed failures have focused coverage. | Graph constraints do not authenticate or authorize callers. The distributed fixture duplicates the same small graph on two stores; high-cardinality stress uses an in-process adjacency-list implementation. These do not prove partitioned-cluster correctness, tenant isolation, or fault recovery. (`src/graph/SECURITY.md`; `tests/graph/test_graph_distributed.cpp`; `tests/graph/test_graph_highcardinality_stress.cpp`) |
| Governance and maintainability | Public contracts, focused test registration, and benchmark mapping documents exist. | Benchmark result manifests, representative hardware evidence, and full query-to-measurement traceability are needed before source thresholds become release gates. CMake discovery/registration is not evidence that tests or benchmarks ran. (`tests/graph/CMakeLists.txt`; `benchmarks/graph/README.md`; `src/graph/PERFORMANCE_EXPECTATIONS.md`) |
| Architecture and operations | The module has separate planning, traversal, semantic, and tensor utility paths, plus CPU fallback and distributed execution interfaces. | Cross-shard traversal, partial-capability behavior, and sustained multi-tenant operation need partitioned data, failure injection, and operator-facing evidence. Graph-level authorization remains an integration responsibility. (`src/graph/ARCHITECTURE.md`; `src/graph/OPERATIONS.md`) |

**Maturity conclusion:** substantial feature breadth with incomplete representative performance,
distributed resilience, and operational evidence. No numeric maturity score or production-ready
claim is inferred from declarations, test registration, historical scanner reports, or thresholds.

### v2.5.0 — compatible hardening and evidence

Scope: preserve existing public contracts; close measured correctness, bounded-execution, and
benchmark-evidence gaps before proposing new graph capabilities.

#### Wave 1 — Reproducible performance baseline (P0)
- **Goal:** establish representative, repeatable baselines for graph read and update workloads.
- **Tasks:**
  - [ ] Map optimizer, traversal, constrained traversal, incremental updates, and distributed paths to benchmark cases (Target: v2.5.0).
  - [ ] Extend the current 64-node fixture with documented graph shapes/scales and an LDBC SNB Interactive-inspired neighborhood-read/update workload (Target: v2.5.0).
  - [ ] Make threshold failures machine-detectable; publish hardware, compiler, configuration, workload version, per-query latency, and run manifest (Target: v2.5.0).
- **Dependencies:** benchmark owner; benchmark-enabled build with Google Benchmark/Python; agreed
  hardware and dataset policy. LDBC is an evaluation reference, not a claim of benchmark
  certification or current compatibility.
- **Risk / trade-off:** small in-process microbenchmarks are cheap and stable but do not predict
  storage-backed or multi-node behavior; public datasets and hardware can add setup cost.
- **Success metrics:** 100% of mapped release cases present in the run manifest; report p50/p95/p99,
  throughput, errors/timeouts, and graph size for every workload; no >10% regression against a
  captured, same-hardware release baseline without an approved exception.
- **Validation:** run focused graph tests and release-profile benchmarks; verify repeatability across
  at least 3 runs and preserve raw output/configuration; require the graph `release_critical` lane
  before a gate is marked passed.
- **Estimate:** 3–5 engineering-weeks.

#### Wave 2 — Bounded execution, security boundaries, and diagnostics (P0)
- **Goal:** make resource limits and failure outcomes explicit across supported graph execution
  routes without moving authorization ownership into the graph module.
- **Tasks:**
  - [ ] Inventory API-specific depth, result, time, cancellation, and fan-out controls; close missing/unbounded paths where supported by existing contracts (Target: v2.5.0).
  - [ ] Test invalid/oversized inputs, high fan-out, timeouts, partial GPU/distributed capability, and deterministic error/fallback reporting (Target: v2.5.0).
  - [ ] Document that caller authentication, tenant isolation, and authorization are enforced by owning server/query integrations (Target: v2.5.0).
- **Dependencies:** Wave 1 workload and SLO evidence; server/query owners for caller and tenant
  policy; existing error taxonomy and path-constraint contracts.
- **Risk / trade-off:** tighter defaults can truncate legitimate traversals or change latency;
  prefer explicit limits and observable outcomes over silent partial results or arbitrary defaults.
- **Success metrics:** 100% of public execution paths have documented limit/failure semantics;
  adversarial-limit tests show bounded result/work behavior; all injected fallback/denial cases are
  distinguishable; all negative cross-tenant integration cases are denied at the owning boundary.
- **Validation:** focused constraint, exact-traversal, GPU-fallback, and distributed error tests;
  integration tests through the owning authorization boundary; run sanitizer/concurrency checks
  where supported. Security sign-off is required for authorization-boundary changes.
- **Estimate:** 4–6 engineering-weeks.

#### Wave 3 — Partitioned cross-shard correctness and recovery (P1)
- **Goal:** validate distributed graph behavior on genuinely partitioned data and node failures.
- **Tasks:**
  - [ ] Replace/extend duplicated-shard fixtures with deterministic partitioned graph fixtures and compare cross-shard BFS and shortest-path results to a single-node oracle (Target: v2.5.0).
  - [ ] Inject unhealthy, throwing, delayed, and failed shards; measure coordinator fan-out/merge costs and recovery (Target: v2.5.0).
  - [ ] Validate the existing four-shard/1M-edge depth-3 BFS acceptance target on a declared testbed before proposing it as a release gate (Target: v2.5.0).
- **Dependencies:** Wave 1 baseline, Wave 2 bounded timeout/error contracts, and a multi-node or
  representative test environment.
- **Risk / trade-off:** cross-shard coordination and replication improve reachability but add network
  and merge overhead; target hardware and partition skew materially affect throughput.
- **Success metrics:** exact result parity with the oracle for deterministic fixtures; for the
  existing proposed scale gate, depth-3 BFS throughput ≥50% of the same-hardware single-shard
  baseline; record p95/p99, timeout/error rate, shard fan-out, and recovery outcome; `release_critical`
  green. These are acceptance targets, not current results.
- **Validation:** unit/property-style partition and merge tests, fault-injection integration tests,
  repeatable under-load benchmark, and review of the run manifest and failure history.
- **Estimate:** 6–10 engineering-weeks.

### v3.0.0 — strategic capability and contract evolution

Scope: only after v2.5 evidence, consider deliberate API/architecture evolution. Breaking changes
require an approved design, migration notes, and compatibility/deprecation policy; this plan does
not commit the project to full GQL support or a particular storage representation.

#### Wave 4 — Graph query contract and GQL interoperability (P1)
- **Goal:** decide which interoperable property-graph query contracts ThemisDB should support.
- **Tasks:**
  - [ ] Map current graph/query APIs to ISO/IEC 39075:2024 and Technical Corrigendum 1; create a supported-feature/gap matrix and conformance fixtures (Target: v3.0.0).
  - [ ] Evaluate parameter handling, error semantics, and migration impact before proposing implementation or API changes (Target: v3.0.0).
- **Dependencies:** v2.5 API inventory, query/server owners, and an approved scope decision.
- **Risk / trade-off:** standard alignment can improve portability but may require semantic changes,
  parser work, and compatibility costs; the standard does not prescribe a storage engine or
  performance target.
- **Success metrics:** every declared supported syntax/semantic feature has a conformance test and
  documented unsupported-feature behavior; zero undocumented behavior changes across the migration
  matrix.
- **Validation:** independent conformance review, negative/invalid-query tests, compatibility tests,
  and migration review. Do not claim GQL conformance until the selected tests and scope are approved.
- **Estimate:** 6–8 engineering-weeks for evaluation and a scoped first increment; full standard
  coverage is unestimated until the gap matrix is approved.

#### Wave 5 — Measured sparse analytics and representation choices (P1)
- **Goal:** determine whether specialized sparse graph kernels or representations improve selected
  analytical workloads without compromising dynamic property-graph behavior.
- **Tasks:**
  - [ ] Compare current paths with CSR/semiring formulations informed by GraphBLAS; characterize graph density, mutation rate, memory use, ID/offset limits, CPU/GPU parity, and rebuild/update cost (Target: v3.0.0).
  - [ ] Adopt only measured workload-specific improvements after architecture approval (Target: v3.0.0).
- **Dependencies:** Wave 1 baselines, Wave 3 distributed correctness, and architecture approval for
  any new backend or persisted format.
- **Risk / trade-off:** sparse matrix methods can accelerate suitable analytics but may add memory,
  conversion, update, and portability costs; CSR is not assumed to be universally faster.
- **Success metrics:** exact result parity for exact algorithms; documented memory and p50/p95/p99
  latency/throughput comparison on at least two graph shapes and two scales; accept a backend only
  when it improves a predeclared target without regression beyond the Wave 1 budget.
- **Validation:** oracle parity, update/rebuild tests, CPU/GPU differential tests, sanitizer runs,
  and benchmarks with configuration and raw results retained.
- **Estimate:** 8–12 engineering-weeks for evaluation and one approved workload-specific increment.

#### Wave 6 — Distributed analytics, fault tolerance, and operational governance (P1)
- **Goal:** establish correctness and operability for selected multi-node graph analytics under
  concurrent load and failures.
- **Tasks:**
  - [ ] Implement and evaluate only approved distributed analytics, including the proposed Brandes betweenness-centrality track (Target: v3.0.0).
  - [ ] Define consistency and recovery expectations; inject node/shard loss during reads and updates (Target: v3.0.0).
  - [ ] Document SLOs, alerts, runbooks, evidence retention, and responsibilities at graph/server/storage boundaries (Target: v3.0.0).
- **Dependencies:** Waves 1–3; agreed consistency model, workload, cluster, and human security/
  operations sign-off.
- **Risk / trade-off:** approximate analytics trade accuracy for cost; stronger consistency and
  recovery guarantees can reduce availability or throughput. These must be explicit per workload.
- **Success metrics:** for the existing proposed BC target, exact results match a reference on
  graphs ≤10K nodes; approximate error ≤5% on ≤1M nodes and runtime ≤60 s on a 4-node testbed,
  subject to re-baselining; fault tests record zero unexplained lost/duplicate results and bounded,
  observable recovery against an approved SLO.
- **Validation:** deterministic reference comparisons, fault-injected histories with consistency
  checks (Jepsen-inspired methodology), sustained-load benchmarks, security/governance review, and
  operator runbook exercise. No independent Jepsen result is implied.
- **Estimate:** 10–16 engineering-weeks after the cluster and consistency requirements are approved.

### Research basis, trade-offs, and open assumptions

| Research / technical basis | Roadmap implication and limitation |
|---|---|
| [LDBC SNB Interactive workload](https://ldbcouncil.org/benchmarks/snb/interactive/) and [SNB specification](https://ldbcouncil.org/ldbc_snb_docs/ldbc-snb-specification.pdf) model neighborhood reads with continuous inserts and report throughput by scale. | Use its workload concepts for reproducible query/update coverage; an internal adaptation is not an audited LDBC result. |
| [SPACE: Cardinality Estimation for Path Queries (ACM, 2025)](https://doi.org/10.1145/3725355) motivates measuring estimated versus actual cardinality and intermediate result growth. | Include per-query plan/estimate diagnostics before tuning cost-based decisions; no optimizer gain is assumed. |
| [ISO/IEC 39075:2024](https://www.iso.org/standard/76120.html) and [Technical Corrigendum 1 (2026)](https://www.iso.org/standard/93701.html) define the GQL contract and correction. | Scope a conformance/gap evaluation; this does not imply ThemisDB currently implements GQL. |
| [GraphBLAS C API v2.1](https://graphblas.org/docs/GraphBLAS_API_C_v2.1.0.pdf) describes sparse algebra for graph computations; [NVIDIA CSR documentation](https://docs.nvidia.com/cuda/archive/12.4.0/cusparse/storage-formats.html) describes one sparse representation. | Evaluate semiring/CSR approaches only against actual graph shapes, mutation patterns, and CPU/GPU parity; neither source establishes universal database superiority. |
| [Jepsen methodology](https://github.com/jepsen-io/jepsen) combines fault injection with checking operation histories against consistency models. | Use fault histories and explicit consistency checks for distributed validation; no Jepsen run or compliance claim is made. |

**Open assumptions / knowledge gaps:** no representative graph benchmark results, workload mix,
supported graph scale, latency/availability SLO, deployment topology, or v3 release commitment was
verified in this task. The four-shard throughput and distributed BC figures above are existing
roadmap acceptance proposals, not measured outcomes. Estimate ranges are engineering effort, not
calendar promises. Revisit all numeric targets after Wave 1 baseline capture.

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
- [x] complete hardening for optimizer, traversal, and distributed orchestration internals (Target: Q4 2026)
- [x] align semantic/reasoning and tensor utility behavior with bounded runtime contracts (Target: Q4 2026)
- [x] stage query-optimizer work before cache/resource/scheduling follow-on items in the current Phase 3 execution order (Target: Q3 2026)

### Phase 3: Error Handling and Edge Cases
- [x] standardize fail-safe behavior for invalid constraints and degraded acceleration routes (Target: Q4 2026)
- [x] unify diagnostics across denial, fallback, and semantic conflict incidents (Target: Q4 2026)

### Phase 4: Tests
- [x] expand focused regressions for mixed constraint/distribution/acceleration edge scenarios (Target: Q4 2026)
- [x] extend deterministic stress fixtures for high fan-out and long-path workloads (Target: Q4 2026)

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

> **Historical plan:** the dates and completion checkboxes below are not current validation.

### Scope
- [x] RotatE embedding model (relation-as-rotation) implementation
- [x] Triple loss with negative sampling
- [x] Link-prediction head for ranked completion results
- [x] Integration with `KnowledgeGraphReasoner`

### Validation
- [x] Unit tests `KGC-01..15`
- [x] Benchmark vs TransE baseline

### Acceptance Gates
- Historical quality targets: MRR ≥ 0.35 and Hits@10 ≥ 0.55. The current acceptance test asserts
  MRR ≥ 0.10, Hits@10 ≥ 0.40, relative-to-TransE bounds, and p99 top-20 latency < 50 ms; these
  thresholds are not captured outcomes. The root roadmap now records this distinction; the higher
  targets remain unverified until a separate evaluation is recorded.
- [x] Zero backward compatibility breaks

### Dependencies
- [~] `KnowledgeGraphReasoner` stability and benchmark baseline complete
- [~] Wave A deployment complete (Speculative Decoding, DPR, Fairness)

### References
- Detail tracker: `../ai/FUTURE_ENHANCEMENTS.md`
- Shared bibliography: `../../docs/research/ml_enhancements_bibliography.md`
- Issue scope: `https://github.com/makr-code/ThemisDB/issues/5039`

## Breaking Changes

No breaking graph contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.
