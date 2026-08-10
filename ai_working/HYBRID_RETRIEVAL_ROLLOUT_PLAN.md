# Hybrid Retrieval Rollout Plan — Module Readiness Map

**Issue**: makr-code/ThemisDB#5468  
**Status**: ✅ READINESS ASSESSMENT COMPLETE (2026-06-25)  
**Date**: 2026-07-06  
**Author**: Copilot (AI agent — for human review before merge)

---

## 1. Purpose

This document translates the ThemisDB target hybrid retrieval architecture into a staged,
module-readiness-gated delivery plan. It covers all six primary implementation modules:

- `index` — ANN Frontdoor and indexing infrastructure
- `gpu` — CUDA/HIP backend and kernel execution
- `acceleration` — bounded kernel dispatch and fallback orchestration
- `graph` — Graph Truth Layer and CPU-first exact traversal
- `query` — query planning and optimizer integration
- `sharding` — distributed shard coordination and routing

It also covers the `distributed_tensor` module for tensor-update infrastructure rollout (Phases A–D).

Source architecture documents:
- `TARGET_ARCHITECTURE.md` — four-layer hybrid retrieval architecture
- `DISTRIBUTED_TENSOR_SHARDING.md` — tensor artifact distribution model
- `ai_working/KERNEL_CLASSIFICATION_REVIEW.md` — kernel safety classification

---

## 2. Architecture Target Summary

The target architecture is a four-layer hybrid retrieval stack:

```
┌────────────────────────────────┐
│  1. ANN Frontdoor              │  fast semantic candidate retrieval (index)
├────────────────────────────────┤
│  2. Tensor Mid-Layer           │  candidate compression, summaries, routing (distributed_tensor)
├────────────────────────────────┤
│  3. Graph Truth Layer          │  exact validation, provenance, ACL (graph)
├────────────────────────────────┤
│  4. LLM / LoRA Final Layer     │  generation, adapter refinement (rag)
└────────────────────────────────┘
```

**Design invariant**: Graph Truth Layer must remain CPU-first and exact throughout all rollout phases.
Advisory artifacts (tensor summaries, ANN candidates) never replace graph-verified results.

---

## 3. Module Readiness Assessment (Gap-Based, 2026-06-25)

| Module            | Readiness | Gap Count | Critical Gap Categories                        |
|-------------------|-----------|-----------|------------------------------------------------|
| `index`           | 40% 🟡    | 7,712     | Buffer lifecycle (680), concurrency (340), result validation (220) |
| `gpu`             | 35% 🔴    | 2,145     | CUDA error handling (340), resource lifecycle (57 RAII), timeout (95) |
| `acceleration`    | 45% 🟡    | 1,840     | Result validation (320), boundary checks (195), dispatch correctness (145) |
| `graph`           | 60% 🟢    | 5,893     | Error handling (195), thread safety (240), exception propagation (110) |
| `query`           | 55% 🟡    | 4,614     | Return value checks (340), exception handling (180), parallel safety (140) |
| `sharding`        | 35% 🔴    | 7,257     | Thread safety (340+), lock ordering (95), consensus coordination (170) |
| `distributed_tensor` | 30% 🔴 | —        | Runtime not yet implemented; scaffold only |

---

## 4. Rollout Phases (Tensor-Graph Extension)

### Phase A — Exact-First Infrastructure (Ready Now ✅)

**Goal**: Establish a working single-shard exact retrieval baseline. No GPU. No advisory artifacts.

**Entry Criteria**: None (baseline start).

**Deliverables**:

| Component | Module | Status | Notes |
|---|---|---|---|
| CPU-first graph truth (exact) | `graph` | ✅ Ready | 0 real L0 gaps; defensive patterns verified |
| ANN candidate retrieval (CPU fallback) | `index` | ✅ Ready | AnnFrontdoor implemented; CPU fallback enforced |
| Query planning (single-shard) | `query` | 🟡 Near-ready | Error path fixes required first |
| Tensor delta log schema | `distributed_tensor` | 🟡 Design phase | Manifest schema defined; store not wired |
| Manifest schema (advisory-only) | `distributed_tensor` | 🟡 Design phase | Advisory-only artifact policy |
| Snapshot-based rebuild worker | `distributed_tensor` | ❌ Not started | Requires manifest + delta log |
| Exact graph fallback guarantee | `graph` | ✅ Ready | CPU traversal preserved unconditionally |
| Single-shard exact routing | `sharding` | ✅ Ready | Single-shard path stable |

**Blocking gap remediation**:
- `query`: Fix 50% of return-value and exception-handling gaps (340 → 170) — **2-3 weeks**
- `graph`: Harden 50% of error/thread-safety gaps (195+240 → ~220 target) — **2-3 weeks**

**GPU**: Disabled.  
**Distributed sharding**: Disabled.  
**Acceleration**: Disabled.

**Validation gates before Phase B**:
- [ ] Exact graph traversal unit tests pass with error injection
- [ ] ANN Frontdoor single-shard route verified end-to-end
- [ ] Query planner fallback path tested and observable
- [ ] Manifest schema committed with advisory-only policy documented

---

### Phase B — Local Tensor Maintenance + ANN + CPU Validation (Q3 2026 🟡)

**Goal**: Introduce ANN + CPU validation, advisory-only acceleration, and local tensor maintenance.

**Entry Criteria**: Phase A gates passed; `query` and `graph` error hardening ≥ 50% gap reduction.

**Deliverables**:

| Component | Module | Status | Notes |
|---|---|---|---|
| ANN + CPU result validation | `index` | 🟡 Q3 2026 | Requires buffer lifecycle + concurrency fixes |
| Advisory-only kernel dispatch (Category A: distance, TopK) | `acceleration` | 🟡 Q3 2026 | Requires result validation hardening (320 gaps) |
| Patch path for small tensor deltas | `distributed_tensor` | 🟡 Q3 2026 | Bounded delta window |
| Partial refit for bounded windows | `distributed_tensor` | 🟡 Q3 2026 | Window size must be bounded |
| Rebuild fallback for unstable / large update sets | `distributed_tensor` | 🟡 Q3 2026 | Fallback to Phase A snapshot rebuild |
| Planner freshness gates | `distributed_tensor` | 🟡 Q3 2026 | Freshness threshold checks in planner |
| Hybrid query planning (ANN + graph) | `query` | 🟡 Q3 2026 | Thread-safe plan optimization required |
| Vec KNN insert pipeline | `index` | 🟡 Q3 2026 | ThreadSanitizer gate required |

**Blocking gap remediation**:
- `index`: Fix 60% of buffer lifecycle and concurrency gaps — **4-6 weeks**
- `acceleration`: Fix 60% of result validation and boundary-check gaps — **4-6 weeks**
- `query`: Fix thread-safety gaps in parallel plan optimization (140 → 60) — **3-4 weeks**

**GPU**: Not recommended. CUDA error handling gaps (340) remain too high.  
**Distributed sharding**: Not enabled; single-shard scope preserved.

**Kernel safety gate (Category A only)**:
- Distance kernels: L2, Cosine, InnerProduct — advisory output only
- TopK selection: bounded output, CPU validation before downstream use
- CPU fallback enforced for all Category A paths on error, timeout, or validation failure

**Validation gates before Phase C**:
- [ ] ANN + CPU parity tests passing for distance and TopK kernels
- [ ] Advisory-only dispatch verified: no correctness-critical path uses GPU result directly
- [ ] Category A kernel result validation hardened and tested
- [ ] Tensor delta log + partial refit logic implemented with rebuild fallback
- [ ] Planner freshness gates observable via Prometheus metrics
- [ ] ThreadSanitizer gate clean for Vec KNN insert

---

### Phase C — Distributed Tensor Artifact Coordination (Q4 2026 🟠)

**Goal**: Introduce multi-shard exact coordination and summary-first routing with shard summary refresh.

**Entry Criteria**: Phase B gates passed; `sharding` thread-safety and consensus gaps ≥ 70% reduction.

**Deliverables**:

| Component | Module | Status | Notes |
|---|---|---|---|
| Shard summary refresh | `distributed_tensor` | 🔵 Q4 2026 | Requires sharding coordination |
| Summary-first routing | `distributed_tensor` | 🔵 Q4 2026 | Advisory-only; escalation to exact-on-demand |
| Exact-on-demand tensor fetch | `distributed_tensor` | 🔵 Q4 2026 | Replaces summary when accuracy required |
| Multi-shard freshness handling | `distributed_tensor` | 🔵 Q4 2026 | Cross-shard freshness consensus |
| Multi-shard exact routing | `sharding` | ❌ Q3 2026 (after thread-safety) | 340+ thread-safety gaps must be fixed |
| Shard routing correctness | `sharding` | ❌ Q4 2026 | Consensus robustness required |
| Bounded graph kernel acceleration (Category B: Geo, BFS, Dijkstra) | `acceleration` | ⚠️ Q4 2026 | Requires 60% gap reduction + parity tests |

**Blocking gap remediation**:
- `sharding`: Fix 70% of thread-safety and lock-ordering gaps (340+95 → ~130 target) — **8-10 weeks**
- `acceleration`: Implement and pass GPU/CPU parity tests for Geo, BFS, Dijkstra — **4-6 weeks**
- `graph`: Harden Category B kernel fallback paths (BFS frontier cutoff, Dijkstra overflow) — **2-3 weeks**

**GPU**: Conditionally enabled for Category B graph kernels only after:
- Parity tests pass against CPU reference for BFS, Dijkstra, Geo distance/containment
- Frontier size cutoff enforced (max 10K nodes/hop, max 3 hops)
- Edge-weight and overflow validation enforced (Dijkstra non-negative, accumulated cost ≤ FLT_MAX)
- CPU fallback confirmed operational

**Category C (Policy, Provenance, Transactions)**: Permanently CPU-first. No GPU acceleration.

**Validation gates before Phase D**:
- [ ] Multi-shard exact routing tested under shard failure injection
- [ ] Shard summary refresh observable with freshness timestamps
- [ ] Summary-first routing verified to escalate to exact-on-demand correctly
- [ ] Category B kernel GPU/CPU parity tests: all pass
- [ ] Distributed benchmark suite implemented and passing baseline
- [ ] Consensus/lock ordering verified with deadlock detector

---

### Phase D — Optional GPU Acceleration (2027+ 🔴)

**Goal**: Enable bounded GPU acceleration where break-even validated. No unconditional GPU dependency.

**Entry Criteria**: Phase C gates passed; `gpu` CUDA error handling ≥ 85% gap reduction (340 → ~51).

**Deliverables**:

| Component | Module | Status | Notes |
|---|---|---|---|
| CPU/GPU break-even validation | `gpu` + `acceleration` | 🟡 2027+ | CPU baselines shipped; GPU hardware review still pending |
| Bounded GPU refinement (Category A + B) | `gpu` | 🔴 2027+ | Only after break-even proven |
| Full CUDA error handling hardening | `gpu` | 🔴 2027+ | 340 unchecked calls → ≤ 51 |
| Resource exhaustion handling | `gpu` | 🔴 2027+ | RAII violations resolved |
| Graceful CPU fallback paths | `gpu` | 🔴 2027+ | All GPU failures must degrade cleanly |
| HIP/ROCm backend parity | `gpu` | 🔴 2027+ | AMD support after CUDA stable |

**Constraints**:
- No unconditional GPU dependency introduced in any phase
- GPU path is advisory only; never truth-bearing
- Graph Truth Layer stays CPU-first unconditionally

**Validation gates**:
- [~] CPU/GPU break-even benchmarks published and reviewed
- [ ] All 340 unchecked CUDA calls addressed (AddressSanitizer + manual review)
- [ ] GPU resource exhaustion paths tested with injection harness
- [ ] HIP backend feature-parity test matrix passing

---

## 5. Staged Rollout Table

| Phase | Timeline | Modules Active | GPU | Distributed | Advisory Tensors | Key Gate |
|-------|----------|----------------|-----|-------------|------------------|----------|
| A — Exact-First | Q3 2026 (start) | graph, query, index, distributed_tensor (schema) | ❌ | ❌ | ❌ | 50% gap fix in query+graph |
| B — Local Tensor + ANN | Q3 2026 (wk 5–10) | +index (ANN), +acceleration (Cat A), +distributed_tensor (delta) | ❌ | ❌ | Advisory-only | 60% gap fix in index+acceleration |
| C — Distributed Coordination | Q4 2026 | +sharding (multi-shard), +distributed_tensor (summary), +acceleration (Cat B) | Conditional | ✅ | Summary-first | 70% gap fix in sharding |
| D — Optional GPU | 2027+ | All | Optional | ✅ | Full | 85% GPU gap fix |

---

## 6. Dependency Map

```
Phase A (Exact-First)
  ├── graph: exact traversal, CPU-first guarantee ──────────────┐
  ├── query: single-shard planner, error hardening              │
  ├── index: AnnFrontdoor (CPU fallback) ────────────────────── ┤
  └── distributed_tensor: manifest schema, advisory policy       │
                                                                 │
Phase B (Local Tensor + ANN + CPU Validation)                    │
  ├── index: ANN + CPU validation (depends on Phase A index) ───┘
  ├── acceleration: Category A kernels (depends on index)
  ├── query: hybrid planner (depends on Phase A query fixes)
  └── distributed_tensor: delta log, partial refit, rebuild fallback
      └── depends on: index (ANN), query (freshness gates)
                                                                 │
Phase C (Distributed Coordination)                               │
  ├── sharding: multi-shard exact (depends on Phase B sharding hardening)
  ├── distributed_tensor: shard summary refresh (depends on sharding)
  ├── distributed_tensor: summary-first routing (depends on sharding + Phase B tensor)
  └── acceleration: Category B kernels (depends on Phase B acceleration + parity tests)
      └── depends on: graph (fallback), index (candidate set)
                                                                 │
Phase D (Optional GPU)                                           │
  ├── gpu: break-even validation (depends on Phase C benchmarks)
  └── acceleration: bounded GPU refinement (depends on gpu hardening)
```

---

## 7. Risk Analysis by Module

### `index` (40% readiness)

| Risk | Severity | Mitigation |
|---|---|---|
| Buffer lifecycle RAII gaps (680) can cause silent use-after-free in ANN hot path | HIGH | RAII hardening sprint before Phase B; AddressSanitizer gate |
| Concurrent index builder races (340 gaps) | HIGH | ThreadSanitizer clean required before ANN merge |
| Result validation gaps (220) allow invalid ANN candidates to reach downstream | HIGH | Validation gate: output cardinality + range check before tensor layer |
| GPU CUDA backend not ready | MEDIUM | Phase D only; CPU fallback unconditionally enforced |

**Recommended order**: Buffer lifecycle → Concurrency → Result validation → GPU (deferred)

---

### `gpu` (35% readiness)

| Risk | Severity | Mitigation |
|---|---|---|
| 340 unchecked CUDA calls can cause silent failures and undefined behavior | CRITICAL | No GPU ANN until 50% fixed; no GPU at all until 85% fixed |
| Resource exhaustion with no graceful degradation | HIGH | RAII wrapper audit; exhaustion injection test |
| Timeout gaps (95) can block threads indefinitely | HIGH | Kernel SLA: 5-second timeout enforced before any GPU path |
| Exception propagation gaps (120+) | MEDIUM | All GPU errors must propagate to CPU fallback cleanly |

**Recommended order**: Error handling → Timeout gates → RAII resource lifecycle → Exception propagation

---

### `acceleration` (45% readiness)

| Risk | Severity | Mitigation |
|---|---|---|
| Kernel result validation gaps (320) allow correctness-critical output to be silently wrong | HIGH | Mandatory output validation before any advisory use |
| Memory boundary violations (195 gaps) | HIGH | Boundary guards required; ASAN gate before Phase B |
| Dispatch correctness gaps (145) can route wrong inputs to kernels | MEDIUM | Input validation gates (CONSTRAINT_A1–A5 in kernel spec) |
| Category B kernels (Geo, BFS, Dijkstra) not safe without parity tests | HIGH | Parity test suite mandatory before Phase C |

**Recommended order**: Result validation → Boundary checks → Dispatch correctness → Category B parity tests

---

### `graph` (60% readiness)

| Risk | Severity | Mitigation |
|---|---|---|
| Error handling gaps (195) in exact traversal paths | MEDIUM | Error path coverage sprint; exception safety tests |
| Thread safety gaps (240) under concurrent graph access | MEDIUM | Thread-safe access patterns; concurrent test fixtures |
| Exception propagation gaps (110) | LOW | Systematic exception safety review |
| Category C paths (policy, provenance, transactions) must remain CPU-first | CRITICAL | Policy enforced at kernel classification level; no GPU path ever for Category C |

**Note**: Graph module has 0 real L0 gaps; all findings are verified defensive patterns.  
**Recommended order**: Error handling → Thread safety → Exception propagation (Category C: permanently CPU-first)

---

### `query` (55% readiness)

| Risk | Severity | Mitigation |
|---|---|---|
| Return value check gaps (340) can cause silent planner failures | HIGH | Return value audit; error propagation tests |
| Exception handling gaps (180) in optimizer | HIGH | Exception-safe optimizer refactor |
| Thread safety in parallel planning (140 gaps) | HIGH | Required before hybrid planner enables parallel optimization |
| Missing fallback logic in planner | MEDIUM | Fallback-to-exact tested under GPU/ANN unavailability |

**Recommended order**: Return value checks → Exception handling → Thread safety → Fallback logic

---

### `sharding` (35% readiness)

| Risk | Severity | Mitigation |
|---|---|---|
| Cross-shard thread safety gaps (340+) can cause data races | CRITICAL | ThreadSanitizer gate; Phase B single-shard only |
| Lock ordering violations (95) create deadlock risk | CRITICAL | Consistent lock ordering enforcement; deadlock detector |
| Consensus coordination gaps (170) | HIGH | Consensus robustness testing under partition |
| Multi-shard enabled only in Phase C after gate | CRITICAL | Gate enforced; no multi-shard in Phase A or B |

**Recommended order**: Thread safety → Lock ordering → Consensus → Multi-shard (Phase C gate)

---

### `distributed_tensor` (30% readiness)

| Risk | Severity | Mitigation |
|---|---|---|
| Manifest schema not wired to storage — advisory artifacts not tracked | HIGH | Wire manifest store in Phase A |
| Snapshot-based rebuild worker not implemented | HIGH | Required before Phase B partial refit |
| Summary-first routing not implemented | HIGH | Phase C only; requires sharding stable |
| No observability hooks for freshness | MEDIUM | Prometheus metrics required before Phase B |

**Recommended order**: Manifest schema → Delta log → Rebuild worker → Freshness hooks → Shard summary (Phase C)

---

## 8. Recommended Implementation Order

### Q3 2026 — Phase A Entry (Weeks 1–4)

1. **Graph error hardening** (50% gap reduction) — `graph` module
   - Fix error handling in traversal and constraint paths
   - Add exception-safety tests for exact graph layer

2. **Query planner error hardening** (50% return-value + exception gaps) — `query` module
   - Fix return-value check gaps (340 → 170)
   - Add exception-safe optimizer path and fallback tests

3. **Tensor manifest schema + advisory policy** — `distributed_tensor` module
   - Commit manifest schema (ArtifactManifest + ManifestStore)
   - Document advisory-only artifact policy
   - Add observability hooks (Prometheus freshness metrics)

4. **Phase A validation gates** — end of week 4
   - Run exact graph traversal unit tests with error injection
   - Verify ANN Frontdoor single-shard end-to-end
   - Verify planner fallback path observable
   - Tag Phase A complete

---

### Q3 2026 — Phase B Entry (Weeks 5–10)

5. **Index buffer lifecycle + concurrency hardening** — `index` module (weeks 5–7)
   - Fix 60% of buffer lifecycle RAII gaps
   - ThreadSanitizer gate for Vec KNN insert pipeline

6. **Acceleration result validation + boundary hardening** — `acceleration` module (weeks 5–8)
   - Fix result validation gaps (320 → 128)
   - Fix memory boundary violations (195 → 78)
   - Implement Category A constraint enforcement (CONSTRAINT_A1–A5)

7. **Query thread-safety hardening** — `query` module (weeks 6–8)
   - Fix parallel planning thread safety gaps (140 → 56)
   - Enable hybrid planner (ANN + graph) with single-shard scope

8. **Tensor delta log + partial refit + rebuild fallback** — `distributed_tensor` module (weeks 6–9)
   - Implement tensor delta log
   - Implement patch path for small deltas
   - Implement partial refit for bounded windows
   - Wire rebuild fallback for large/unstable update sets
   - Implement planner freshness gates

9. **Phase B validation gates** — end of week 10
   - ANN + CPU parity tests for distance and TopK kernels
   - Category A advisory dispatch verified
   - Tensor delta log tested with rebuild fallback
   - No GPU enabled

---

### Q4 2026 — Phase C Entry

10. **Sharding thread-safety + lock-ordering hardening** — `sharding` module (8–10 weeks)
    - Fix 70% of cross-shard thread safety gaps
    - Enforce consistent lock ordering
    - Consensus coordination robustness testing

11. **Category B kernel parity tests** — `acceleration` module
    - Geo distance/containment: Haversine parity, input/output validation
    - BFS: frontier size cutoff (10K), depth limit (3 hops), CPU parity
    - Dijkstra: edge weight non-negative, overflow guard, CPU parity

12. **Distributed tensor coordination** — `distributed_tensor` module
    - Shard summary refresh with freshness timestamps
    - Summary-first routing with escalation to exact-on-demand
    - Multi-shard freshness consensus

13. **Phase C validation gates**
    - Multi-shard exact routing under shard failure injection
    - Category B kernel GPU/CPU parity: all pass
    - Distributed benchmark suite passing baseline

---

### 2027 — Phase D Entry

14. **GPU break-even validation** — `gpu` + `acceleration` modules
    - Profile CPU vs GPU for Category A + B workloads
    - Enable bounded GPU refinement only where break-even proven
    - Enforce all GPU paths as advisory only

15. **Full CUDA error handling hardening** — `gpu` module
    - 340 unchecked CUDA calls addressed
    - Resource exhaustion paths tested with injection harness
    - HIP/ROCm backend parity

---

## 9. Observability Requirements Before Rollout

The following Prometheus metrics must be wired before enabling each phase in production:

| Phase | Required Metrics |
|---|---|
| A | `graph_exact_traversal_errors_total`, `query_planner_fallback_total`, `ann_frontdoor_route_type` |
| B | `acceleration_kernel_dispatch_advisory_total`, `acceleration_fallback_to_cpu_total`, `tensor_delta_log_entries_total`, `tensor_freshness_age_seconds` |
| C | `sharding_cross_shard_requests_total`, `tensor_shard_summary_refresh_total`, `tensor_exact_on_demand_total`, `category_b_kernel_parity_failures_total` |
| D | `gpu_error_rate`, `gpu_cpu_breakeven_ratio`, `gpu_fallback_total` |

---

## 10. CTest and Benchmark Gates (Mandatory Before Production)

| Phase | Required CTest Gates | Required Benchmark Gates |
|---|---|---|
| A | `test_graph_exact_traversal`, `test_query_planner_fallback`, `test_ann_frontdoor_single_shard` | None (baseline) |
| B | `test_ann_cpu_parity`, `test_acceleration_category_a`, `test_tensor_delta_log`, `test_tensor_rebuild_fallback` | `bench_ann_distance_cpu_vs_flat`, `bench_tensor_partial_refit` |
| C | `test_sharding_multishard_exact`, `test_category_b_parity_geo`, `test_category_b_parity_bfs`, `test_category_b_parity_dijkstra`, `test_tensor_shard_summary` | `bench_multishard_exact`, `bench_category_b_gpu_cpu_parity`, `bench_tensor_summary_first` |
| D | `test_gpu_error_handling`, `test_gpu_resource_exhaustion`, `test_gpu_fallback_all_paths` | `bench_gpu_cpu_breakeven_category_a`, `bench_gpu_cpu_breakeven_category_b` |

---

## 11. Architecture Invariants Preserved Throughout

The following invariants must be preserved in all phases:

1. **Graph Truth Layer is CPU-first and exact in all phases** — no GPU acceleration for Category C kernels (policy, provenance, transactions) ever.
2. **Advisory artifacts never replace graph-verified results** — tensor summaries and ANN candidates are inputs to the planner, not authoritative truth.
3. **All GPU paths require CPU fallback** — timeout (5s SLA), error, or validation failure must degrade to CPU cleanly.
4. **No unconditional GPU dependency** — GPU is always optional and guarded by break-even validation.
5. **Multi-shard only in Phase C+** — Phase A and B are single-shard scope.
6. **Category B kernels require parity tests before production** — GPU/CPU parity must be established empirically, not assumed.

---

## 12. Reference Documents

| Document | Location | Relevance |
|---|---|---|
| Target Architecture | `TARGET_ARCHITECTURE.md` | Four-layer model, ANN artifact classes |
| Distributed Tensor Sharding | `DISTRIBUTED_TENSOR_SHARDING.md` | Tensor artifact model and distribution |
| Kernel Classification | `ai_working/KERNEL_CLASSIFICATION_REVIEW.md` | Category A/B/C kernel safety analysis |
| Module Gap Details | `src/*/MODULE_GAPS.md` | Per-module gap counts and priorities |
| Module Snapshot | `ai_working/MODULE_SNAPSHOT_AGGREGATE_L2.md` | Readiness aggregate |
| Issue #5467 | makr-code/ThemisDB#5467 | Planner design CPU/GPU boundaries |
| Issue #5469 | makr-code/ThemisDB#5469 | Kernel classification framework |
