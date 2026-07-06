# ADR E2-003: Query Planner Routing Model

- Status: Accepted
- Date: 2026-07-06
- Deciders: ThemisDB Architecture / Query Governance
- Supersedes: None (previously a placeholder)
- Related:
  - docs/EPIC2_QUERY_PLANNER.md (design document)
  - docs/adr/adr-e2-005-cross-layer-fallback-confidence-policy.md (fallback / fail-closed)
  - ai_working/KERNEL_CLASSIFICATION_REVIEW.md (kernel category evidence)
  - Issue #5467 (CPU/GPU boundary integration)
  - Issue #5468 (rollout plan)
  - Issue #5469 (kernel eligibility)

## Context

The hybrid query planner must route requests across ANN, tensor-refinement, and
graph-validation stages. Without explicit routing rules, there is a risk that:

1. GPU approximations silently replace CPU exact-truth operations.
2. Stale tensor artifacts propagate advisory results to truth-bearing decisions.
3. Policy, ACL, and provenance operations are accidentally offloaded to GPU paths.
4. Fallback behavior is implicit and non-deterministic.

The architecture review (2026-06-25) and kernel classification (Issue #5469) provide
concrete evidence for the boundaries. The gap analysis (Phase 5, 2026-06-25) quantifies
which modules are currently unsafe for GPU dispatch.

## Decision

The planner adopts five canonical execution paths and three kernel categories with
explicit eligibility gates:

### Kernel Categories

**Category A — Acceleration-Eligible (advisory only)**
ANN distance kernels and TopK selection may run on GPU when the 50 % error-handling
gap fix gate is satisfied. Output is advisory; CPU validation is mandatory.

**Category B — Bounded / Conditional**
Geo distance/containment, graph BFS (≤ 3 hops, ≤ 10 000 frontier), and graph Dijkstra
(≤ 1 000 pairs) may run on GPU when CPU/GPU parity tests pass and input bounds are
satisfied. GPU result is advisory for graph operations; CPU exact-path must confirm
any truth-bearing decision.

**Category C — CPU-Only (never GPU)**
Policy-aware traversal, provenance chain construction, and transaction consistency
verification are CPU-only. No GPU approximation or advisory semantics are permitted.
Runtime checks must prevent accidental GPU dispatch.

### Five Canonical Paths

| Path | Name | Condition |
|------|------|-----------|
| 1 | ANN Only | Non-truth-bearing retrieval, GPU Category A eligible |
| 2 | ANN + Tensor Summary | Fresh tensor artifact (age < max_staleness_ms, residual ≥ 0.95) |
| 3 | ANN + Tensor + Exact Graph | Quality-critical; tensor advisory + graph-verified finalization |
| 4 | Direct Exact Graph | Tensor stale / missing / force_exact; CPU exact traversal |
| 5 | Distributed Summary-First + Exact-On-Demand | Multi-shard; exact loading per shard on confidence trigger |

The planner falls back toward higher-cost paths when any gate fails.

### Tensor Artifact Freshness Gate

Tensor artifacts are advisory only. The planner enforces:
- `artifact_age_ms < max_staleness_ms` (policy default: 5 000 ms)
- `residual_threshold ≥ 0.95`
- `rebuild_in_progress == false`
- `rank_cap` within policy range

Any failed gate forces Path 4 (direct exact graph).

### Fallback Contract

Every fallback carries a machine-readable reason code. Silent fallback is forbidden
(adopted from ADR E2-005). Fallback triggers include: GPU kernel error, timeout
(> 5 s SLA), output parity failure, frontier / pair-count cutoff, tensor staleness,
and Category C sub-path detection.

## Consequences

**Positive**:
- Explicit CPU/GPU boundaries prevent silent approximation of truth-bearing operations.
- Tensor freshness gates ensure stale advisory data cannot reach final answers.
- Five-path model allows structured cost-vs-quality trade-offs at query time.
- Machine-readable reason codes improve incident triage and observability.
- Category C enforcement protects ACL, provenance, and ACID semantics.

**Trade-offs**:
- Module readiness gates (50–60 % gap fix thresholds) delay GPU acceleration until
  Q3 2026 remediation milestones are met.
- Additional planner metadata (path selection, reason codes, freshness signals) adds
  slight overhead to every query decision.
- Strict Category C enforcement means no GPU-assisted policy evaluation even if future
  hardware supports it; an explicit ADR amendment would be required to change this.

## Rejected Options

**Option A: Single-layer GPU-first dispatch**
Rejected because it cannot maintain ACL, provenance, and ACID guarantees for
truth-bearing operations.

**Option B: Advisory GPU for all graph operations**
Rejected because graph truth must remain exact for policy and governance decisions;
an advisory graph layer would undermine the separation between approximate and exact
zones required by EPIC 2.4.

**Option C: Tensor artifacts treated as final truth**
Rejected because tensor indexes are inherently approximate and time-lagged. Treating
them as truth would violate the graph-verified finalization invariant.

## Implementation Notes

Required planner contract surface:
- `src/evaluation/include/query_planner.h` — typed API for path selection,
  freshness check, eligibility signals, and fallback reason codes.
- `src/evaluation/src/query_planner.cc` — implementation (Phase 2).
- `tests/epic2_evaluation/query_planner_test.cc` — deterministic path tests (Phase 4).

Integration order (from ADR E2-005):
1. TensorRAGPipeline decision envelope
2. ANN frontdoor routing diagnostics
3. Graph truth validation result metadata
4. Final-layer resolution diagnostics

## Validation

Minimum acceptance tests:
1. **Path determinism** — same query context and policy produce the same path selection.
2. **Category C enforcement** — ACL / provenance / transaction sub-paths never reach GPU dispatch.
3. **Freshness gate** — stale tensor artifacts force Path 4 with reason code.
4. **Fallback chain** — GPU failure on Path 1/2/3 produces CPU fallback with reason code.
5. **Distributed exact-on-demand** — low shard confidence triggers exact graph load on Path 5.

## Follow-up

- [x] Phase 2 implementation merged: `src/evaluation/src/query_planner.cc` — DefaultQueryPlanner, five paths, all fallback reason codes.
- [x] Phase 4 tests merged: `tests/epic2_evaluation/query_planner_test.cc` — 30+ deterministic path and fallback tests.
- [x] Phase 5 observability merged: `PlannerObserver` interface added to header; timing hooks wired into `selectPath()`; `benchmarks/epic2_evaluation/planner_decision_bench.cc` created for overhead validation.
- [x] Phase 5 / 6 gap remediation milestones: documented in `docs/EPIC2_QUERY_PLANNER.md` Phase 5 checkboxes.
- [ ] Amend this ADR if Category B bounds are widened after parity test coverage improves.
- [ ] Wire `planner_decision_bench` into CI performance regression gating when build environment is ready.
