---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Evaluation API Contracts

AI provenance: Exact model backend not verifiable from final document text alone.

## Audience and source of truth

This page is an integration summary for developers using the evaluation headers.
The declarations under `src/evaluation/include/` define the API. Build ownership
is split across targets in `src/evaluation/CMakeLists.txt`; the public-looking
header list does not mean every implementation is linked into `themis_evaluation`.

## Metric functions

`retrieval_metrics.h` provides stateless `[[nodiscard]]` metric functions for
retrieval quality, evidence, provenance, compression, LLM answers, distributed
efficiency, and tensor-graph runtime observations. Inputs are supplied by the
caller; these functions do not query indexes, call a judge model, or validate
claims against a live database.

`computeRetrievalQuality(ranked, ground_truth, k, total_candidates)` expects
non-empty ground truth, `1 <= k <= ranked.size()`, and unique IDs in both lists.
It throws `MetricError` with a `MetricErrorKind` on invalid inputs. Relevance is
binary membership in `ground_truth`; `ranked` order determines rank, and score
fields do not replace that ordering. Candidate reduction is zero when no valid
candidate total is supplied.

Other metric functions similarly validate domain constraints (including
non-finite values and probability ranges) and may throw `MetricError`; callers
must handle it rather than treating an invalid metric result as zero. Check the
individual function declaration and implementation for the specific input
preconditions.

## Planner and approximation contracts

`QueryPlanner::selectPath(eligibility, freshness, config)` is `const noexcept`
and returns a `PlannerDecision`, including its selected `ExecutionPath`, any
`FallbackReason`, policy version, and usage flags. It is a decision function,
not an executor. Force-exact leads to direct exact graph; missing distributed
manifests also lead to the exact path. Tensor freshness and resource eligibility
gates affect whether tensor-assisted paths can be selected.

`makeDefaultQueryPlanner(observer)` returns an owning `unique_ptr<QueryPlanner>`.
The optional `PlannerObserver*` is non-owning and must outlive the planner.
`onDecision()` is called after selection; implementations must be `noexcept` and
must not re-enter the planner. The observer must provide its own synchronization
if planner calls may be concurrent.

`ApproximationRuleEngine` checks a requested layer/zone/category against an
`ApproximationPolicy` and returns a `BoundaryCheckResult`. Its `isAllowed()`
helper is true only for `Allow` or `Bypass`; callers must handle `Deny` and
`EscalateToExact` explicitly. Bypass is disabled by default in the policy
structure.

## Matrix, profile, and lifecycle contracts

- `BenchmarkMatrix` records a `BenchmarkResult` by scenario and dimension,
  supports invalidation and lookup, and reports missing cells as `nullopt`.
  The header specifies concurrent reads but requires callers to serialize writes.
- `HardwareProfileRegistry` owns its profile vector and exposes profile lookup,
  validation, activation, and tier-transition checks. Inspect the result's
  `errors` and `warnings`; a returned profile pointer is borrowed from registry
  storage and must not outlive that storage.
- `ArtifactLifecycleManager` computes states from metadata and policy and offers
  value-returning transitions such as `invalidate`, `beginRebuild`, and rebuild
  completion. It does not itself run background rebuild work or persist a
  manifest.
- `MetricCollector` aggregates snapshots and shard observations but is not
  thread-safe. Protect shared instances externally.

## Integration checklist

1. Include the relevant header from `src/evaluation/include/`.
2. Link the target that actually contains the implementation; see
   `src/evaluation/CMakeLists.txt` and the configured target list.
3. Preserve thrown metric errors, planner fallback reasons, and approximation
   decisions at the call site.
4. Do not interpret a benchmark matrix cell, a high metric score, or a planner
   decision as proof of end-to-end retrieval correctness.
5. Add or extend focused tests for invalid input, fallback decisions, and the
   boundary conditions introduced by the integration.

## References

- [Implementation and build mapping](./IMPLEMENTATION.md)
- [Research and evaluation assumptions](./RESEARCH.md)
- [Trust boundaries](./TRUST.md)
- `src/evaluation/include/retrieval_metrics.h`
- `src/evaluation/include/query_planner.h`
- `src/evaluation/include/approximation_rules.h`
- `src/evaluation/include/benchmark_matrix.h`
- `src/evaluation/include/hardware_profile.h`
- `src/evaluation/include/artifact_lifecycle.h`
