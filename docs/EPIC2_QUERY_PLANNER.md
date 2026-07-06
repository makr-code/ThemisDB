# EPIC 2.5 Query Planner

<!-- Status: current | cpu-gpu boundaries integrated | validated: 2026-07-06 -->
<!-- Closes: #5467 — CPU/GPU execution-boundary rules integrated into hybrid planner design -->

## Summary

Hybrid query planner contract for ANN, tensor, graph, and distributed retrieval flows.
This document now incorporates CPU/GPU execution-boundary rules, tensor artifact freshness
gates, and the five canonical execution paths established by the architecture review
(2026-06-25) and kernel classification (Issue #5469).

## Scope

- Routing decisions across retrieval stages and deployment topologies
- CPU/GPU execution boundary enforcement per kernel category (A / B / C)
- Tensor artifact freshness and validity gating before refinement steps
- Summary-first versus exact-on-demand decisions across distributed shards
- Fallback, degraded-continue, and fail-closed policies per layer
- Explainability and observability for planner choices

## Planned Repository Surfaces

- `src/evaluation/include/query_planner.h`
- `src/evaluation/src/query_planner.cc`
- `tests/epic2_evaluation/query_planner_test.cc`
- `benchmarks/epic2_evaluation/planner_decision_bench.cc`

---

## CPU / GPU Execution Boundaries

The planner enforces three kernel categories derived from the architecture review and
gap analysis (see `ai_working/KERNEL_CLASSIFICATION_REVIEW.md`):

### Category A — Acceleration-Eligible (advisory only)

These kernels may run on GPU after the required remediation gates are satisfied.
Their output is always advisory; downstream validation is mandatory.

| Kernel | File | GPU Gate | Note |
|--------|------|----------|------|
| L2/Cosine/IP distance | `cuda_backend.cpp` | 50 % error-handling gaps fixed | ANN candidate generation |
| TopK selection | `cuda_backend.cpp` | 50 % error-handling gaps fixed | Bounded output, CPU parity checked |
| Vec KNN insert | `vec_knn.cpp` | 60 % gaps fixed | Tensor refinement (SIMD CPU primary) |
| Tensor-core matmul | `tensor_core_matmul.cpp` | 60 % gaps fixed + parity | Optional GPU, CPU preferred |

**Enforcement rules (Category A)**:
- Input validation on CPU before kernel dispatch.
- Output size verified (numQueries × numVectors for distance; K per query for TopK).
- Kernel must complete within the 5-second SLA; timeout triggers CPU fallback.
- CPU fallback is always available and must never be removed.
- Category A results are never treated as final truth; graph validation is required.

### Category B — Bounded / Conditional (strict gates required)

These kernels may run on GPU only within the specified bounds and after CPU/GPU parity
tests are verified. GPU result is advisory for graph traversal; CPU exact-path must
confirm any truth-bearing decision.

| Kernel | Bound | CPU Fallback Trigger |
|--------|-------|----------------------|
| Geo distance / containment | Valid WGS84 coords, output range validated | Coordinate invalid or accuracy threshold not met |
| Graph BFS | ≤ 3 hops, frontier ≤ 10 000 nodes | Frontier exceeded or parity failed |
| Graph Dijkstra | ≤ 1 000 vertex pairs, non-negative edge weights | Pair count exceeded, overflow, parity failed |

**Enforcement rules (Category B)**:
- All input validation is CPU-side before dispatch.
- GPU result must pass CPU parity check before use.
- Frontier / pair-count cutoffs are hard; exceeding them forces CPU traversal.
- Results are advisory candidates only; ACL, provenance, and transaction decisions
  require CPU exact-traversal.

### Category C — CPU-Only (never GPU)

The following operations are non-delegable. No GPU approximation or advisory semantics
are permitted. Runtime checks must prevent accidental GPU dispatch.

| Operation | Reason |
|-----------|--------|
| Policy-aware graph traversal (ACL enforcement) | Security-critical; GPU cannot evaluate ACL semantics |
| Provenance chain construction (audit trail) | Requires deterministic ordering and completeness |
| Transaction consistency verification (ACID / MVCC) | Truth-bearing; GPU approximations invalidate guarantees |

**Enforcement rules (Category C)**:
- No GPU code paths.
- No advisory semantics.
- No approximations.
- Static analysis and code-review checklists must flag any GPU dispatch attempt.

---

## Five Canonical Execution Paths

The planner selects one of the following paths for each request. Paths are ordered from
most approximate (lowest cost) to most exact (highest cost). The planner falls back to a
higher-cost path whenever a gate is not satisfied.

### Path 1 — ANN Only

**Condition**: retrieval-only query, no tensor artifacts required, no graph validation
needed, result is non-truth-bearing.

```
ANN candidate generation (Category A, GPU-eligible)
  → TopK selection (Category A, GPU-eligible)
  → Return candidates
```

**Planner gates**:
- ANN is enabled for this query context.
- CUDA available and Category A error-handling gate satisfied (50 % gap fix).
- `force_exact` is false.
- No ACL, provenance, or transaction semantics required.

### Path 2 — ANN + Tensor Summary

**Condition**: advisory refinement is acceptable, tensor artifact is fresh and
above-quality threshold, downstream consumer does not require exact truth.

```
ANN candidate generation (Category A)
  → Tensor summary lookup (advisory)
  → Refinement / reranking on CPU (Vec KNN primary)
  → Optional GPU matmul if freshness + parity gates pass (Category A/B)
  → Return refined candidates (advisory)
```

**Planner gates** (all must be true):
- Path 1 gates satisfied.
- Tensor artifact exists and `is_fresh(max_age_ms)` is true.
- `residual_threshold` ≥ configured minimum (default 0.95).
- `rank_cap` within policy bounds.
- `rebuild_in_progress` is false.
- Artifact semantics are advisory; result is never treated as final truth.

### Path 3 — ANN + Tensor Refinement + Exact Graph Validation

**Condition**: quality-critical query where tensor refinement improves candidate
ranking but graph truth must confirm the final answer.

```
ANN candidate generation (Category A)
  → Tensor refinement (CPU-preferred; GPU optional with freshness gate)
  → Exact graph validation on CPU (Category C rules apply to policy/provenance sub-paths)
  → Return graph-verified result
```

**Planner gates**:
- Path 2 gates satisfied OR tensor is stale (exact fallback enforced).
- Graph validation is available and healthy.
- Any ACL, provenance, or transaction sub-path uses CPU only (Category C).
- Result is graph-verified final truth.

### Path 4 — Direct Exact Graph

**Condition**: tensor artifacts are absent, stale, or below quality threshold; or the
query explicitly requires exact truth; or a bounded kernel failed its parity check.

```
Exact graph traversal (CPU)
  → Optional bounded graph kernels (Category B, within stated limits)
  → ACL / provenance / transaction sub-paths (Category C, CPU-only)
  → Return exact result
```

**Planner gates**:
- Tensor freshness gate failed, OR `force_exact` is true, OR tensor artifact missing.
- All Category C sub-paths use CPU only.
- No advisory semantics; result is exact graph truth.

### Path 5 — Distributed Summary-First + Exact-On-Demand

**Condition**: query spans multiple shards; summary-first retrieval is used to reduce
cross-shard traffic; exact graph loading is triggered per shard on demand.

```
Shard-local summary retrieval (per shard, summary-first)
  → Fragment assembly and merge (distributed planner)
  → Exact graph loading per shard on demand (if summary confidence < threshold)
  → Graph-verified final merge
  → Return distributed result
```

**Planner gates**:
- Query is distributed across ≥ 2 shards.
- Shard manifests are available and within freshness bounds.
- Exact-on-demand triggers when shard summary confidence < policy threshold.
- Cross-shard FK validation (via `CrossShardForeignKeyValidator`) is enforced
  during the 2PC prepare phase when graph consistency is required.

---

## Tensor Artifact Freshness Inputs

The planner evaluates the following fields before choosing a tensor-refinement path.
These values are sourced from the artifact manifest (see
`src/distributed_tensor/src/manifest_store.cc`).

| Field | Type | Planner Rule |
|-------|------|--------------|
| `artifact_age_ms` | `uint64_t` | Must be < `max_staleness_ms` (policy, default 5 000 ms) |
| `delta_lag` | `uint64_t` | Large lag indicates index drift; triggers exact fallback above policy threshold |
| `source_seq_start` / `source_seq_end` | `uint64_t` | Must be compatible with current snapshot sequence |
| `residual_threshold` | `double` | Must be ≥ configured minimum (default 0.95); below threshold → exact fallback |
| `rank_cap` | `int` | Must be within policy range; exceeded cap → exact fallback |
| `rebuild_in_progress` | `bool` | If true → exact fallback, no tensor path |

**Staleness decision**:
- `artifact_age_ms ≥ max_staleness_ms` → force Path 4 (exact graph).
- `rebuild_in_progress == true` → force Path 4.
- `residual_threshold < 0.95` → force Path 4.
- All other gates passed → tensor path (Path 2 or 3) is eligible.

**Tensor artifacts are never final truth**. Even when all freshness gates pass,
tensor results must be treated as advisory. Truth-bearing decisions (policy, ACL,
provenance, audit) always require exact graph validation.

---

## Execution Eligibility and Cost Model Inputs

The planner collects the following signals to choose a path and hardware tier:

| Signal | Source | Impact |
|--------|--------|--------|
| `cuda_available` | Hardware profile | Enables Category A/B GPU paths |
| `ann_enabled` | Query context | Enables ANN candidate generation |
| `gpu_error_handling_gate` | Module readiness (50 % gap fix) | Unlocks Category A ANN |
| `gpu_parity_validated` | Kernel test suite | Unlocks Category B bounded kernels |
| `force_exact` | Query flag / policy | Overrides to Path 4 |
| `force_cpu` | Query flag / policy | Disables all GPU paths |
| `artifact_freshness` | Manifest store | Governs Path 2 / 3 eligibility |
| `module_gap_thresholds` | Gap analysis runtime | Blocks paths when gap severity is above limit |

**Module readiness blockers** (from gap analysis, 2026-06-25):

| Module | Critical gaps | Blocked planner feature | Remediation ETA |
|--------|---------------|------------------------|-----------------|
| GPU error handling | ~340 | Category A ANN (GPU path) | Q3 2026 |
| Acceleration kernel validation | ~320 | Category B bounded kernels | Q3 2026 |
| Query thread safety | ~140 | Parallel plan optimization | Q3 2026 |
| Query exception handling | ~180 | Fallback logic safety | Q3 2026 |
| Index buffer safety | ~680 | ANN candidate reliability | Q3 2026 |

Until these gaps are remediated to the required thresholds, the planner MUST treat the
corresponding path as ineligible and fall back to the next safer path.

---

## Provenance, ACL, and Policy Blockers

Any query carrying the following attributes is blocked from GPU offload regardless of
hardware availability or freshness state:

- **ACL-gated traversal**: requires CPU exact evaluation of policy semantics.
- **Provenance assembly**: requires deterministic CPU ordering.
- **Transaction verification**: requires CPU ACID / MVCC guarantees.
- **Audit trail construction**: must be reconstructable; GPU parallelism breaks completeness.

The planner must refuse to route these operations to any GPU path. Violation is
fail-closed (request rejected, not silently downgraded).

---

## Fallback and Recovery Rules

| Trigger | Fallback Action |
|---------|----------------|
| GPU kernel error | Immediate CPU fallback; log with reason code |
| Kernel timeout (> 5 s SLA) | CPU fallback; decrement GPU confidence score |
| Output validation failure (size / range / parity) | CPU fallback; log with reason code |
| BFS frontier > 10 000 nodes | CPU exact BFS; not an error |
| Dijkstra pair count > 1 000 | CPU exact Dijkstra; not an error |
| Tensor artifact stale / missing | Path 4 (exact graph); log artifact age |
| Tensor residual below threshold | Path 4; log residual value |
| Shard summary confidence below threshold | Trigger exact-on-demand for that shard |
| Module gap threshold exceeded at runtime | Block path; downgrade to CPU-only tier |
| ACL / provenance / transaction sub-path detected | Force Category C (CPU-only) |

All fallback decisions must carry a machine-readable reason code. Silent fallback is
forbidden (see ADR E2-005).

---

## Seven-Phase Roadmap

### Phase 1: Design / API contract
- [x] Freeze planner inputs, outputs, and explanation schema (this document + `query_planner.h`)
- [x] Document CPU/GPU execution boundaries (Category A / B / C) with kernel classification
- [x] Define five canonical execution paths with eligibility gates
- [x] Document tensor artifact freshness inputs and staleness rules
- [x] Align with ADR E2-003 (routing model), ADR E2-005 (fallback / fail-closed policy)
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.5 Query Planner`.

### Phase 2: Core implementation
- [x] Implement `DefaultQueryPlanner::selectPath()` using the five canonical paths above (`src/evaluation/src/query_planner.cc`)
- [x] `TensorArtifactFreshness::isFresh()` and `staleness_reason()` are inline in the header (Phase 1)
- [x] `ExecutionEligibility::isGpuEligible()` enforces Category A / B / C boundaries (Phase 1 + Phase 2)
- [x] Distributed Path 5 gates (`distributed_multi_shard`, `shard_manifests_available`) wired into `selectPath()`
- [x] `makeDefaultQueryPlanner()` factory function declared in header and defined in `.cc`
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.5 Query Planner`.

### Phase 3: Error handling and edge cases
- [x] Fail-closed behavior implemented: `force_exact` always overrides to Path 4 with `FallbackReason::ForceExact`
- [x] Category C enforcement: `isGpuEligible(KernelCategory::C)` always returns false
- [x] Every fallback carries a non-None `FallbackReason` from the defined trigger table
- [x] Module gap threshold blocker: `index_buffer_safety_ok == false` → `ModuleGapThreshold` fallback
- [x] Partial shard loss (missing manifests): `ShardManifestMissing` → Path 4 fallback
- [x] Stale / low-quality tensor artifacts map to specific reason codes (TensorArtifactStale, TensorResidualLow, TensorRebuildInProgress, TensorRankCapExceeded)
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.5 Query Planner`.

### Phase 4: Tests
- [x] Unit tests: deterministic path selection for all five paths (`tests/epic2_evaluation/query_planner_test.cc`)
- [x] Unit tests: tensor freshness gate — all boundary conditions (age, residual, rebuild, delta_lag, rank_cap)
- [x] Unit tests: Category C fail-closed enforcement (no GPU dispatch)
- [x] Integration tests: fallback chain from Path 1 → Path 4 on stale artifact / GPU disabled
- [x] Integration tests: Path 5 distributed exact-on-demand trigger + missing manifest fallback
- [x] Tests: force_exact, force_cpu overrides, policy version metadata in decision
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.5 Query Planner`.

### Phase 5: Performance and hardening
- [ ] Add module gap threshold monitoring at planner initialization
- [ ] Expose path selection decisions via observability hooks (latency, fallback rate)
- [ ] Benchmark planner overhead vs. retrieval latency for all five paths
- [ ] Keep planner docs synchronized with approximation and distributed retrieval work
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.5 Query Planner`.

### Phase 6: Documentation and acceptance
- [ ] Update ADR E2-003 with rationale and trade-offs after Phase 2 implementation
- [ ] Verify acceptance criteria (see below) before marking EPIC 2.5 complete
- [ ] Keep planner docs synchronized with approximation and distributed retrieval work
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.5 Query Planner`.

### Phase 7: Integration
- [ ] Wire the planned files into the nearest CMake and cross-epic integration checkpoints for `EPIC 2.5 Query Planner`.
- [ ] Integrate planner decisions with `TensorRAGPipeline` decision envelope (ADR E2-005)
- [ ] Integrate ANN frontdoor routing diagnostics
- [ ] Integrate graph truth validation result metadata

## Acceptance Criteria

- [x] Planner design explicitly models CPU/GPU boundaries (Category A / B / C)
- [x] Exact graph truth is protected — Category C operations are CPU-only with no exceptions
- [x] Summary-first (Path 5) and exact-on-demand decisions are documented with eligibility gates
- [x] Fallback behavior is explicit — every trigger has a defined action and reason code
- [x] Tensor artifacts are never treated as final truth — advisory semantics documented
- [x] Stale or invalid tensor artifacts trigger exact graph fallback (Path 4)
- [x] Planner can distinguish retrieval paths (1–3) from artifact-maintenance paths (4–5)
- [x] `src/evaluation/include/query_planner.h` contract header provides typed API for the above
- [x] Tests cover all five paths and all fallback triggers (`tests/epic2_evaluation/query_planner_test.cc`)

## Acceptance Signals

- The planned repository surfaces remain stable enough for issue creation and ownership.
- The document names the dependencies, failure modes, and validation hooks needed before code lands.
- Tests and benchmarks have reserved file names before implementation starts.

## References

- `docs/EPIC1_ARCHITECTURE.md`
- `docs/EPIC2_APPROXIMATION_GOVERNANCE.md`
- `docs/EPIC3_DISTRIBUTED_RETRIEVAL.md`
- `docs/adr/adr-e2-003-query-planner-routing-model.md` — routing model decision record
- `docs/adr/adr-e2-005-cross-layer-fallback-confidence-policy.md` — fallback / fail-closed policy
- `ai_working/KERNEL_CLASSIFICATION_REVIEW.md` — kernel category details and gap counts
- `src/evaluation/include/query_planner.h` — typed planner contract
- Issue #5467 — CPU/GPU boundary integration (this document)
- Issue #5468 — Rollout plan with Phase A / B remediation gates
- Issue #5469 — Kernel eligibility classification
