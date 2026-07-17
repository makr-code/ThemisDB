# Evaluation Module - Future Enhancements

<!-- Status: current | validated: 2026-07-17 -->
<!-- Links: README.md · ROADMAP.md · PERFORMANCE_EXPECTATIONS.md -->

## Scope

- extend the shipped hybrid query planner into fully integrated evaluation and retrieval workflows
- keep planner policy, observability, and distributed retrieval behavior explicit and testable
- tie future planner promotion to measurable latency, fallback-rate, and correctness thresholds

## Design Constraints

- maintain compatibility across EPIC 1/2/3 dependency boundaries
- keep approximation and policy behavior explicit and testable
- fail safely when hardware/profile assumptions are not satisfied
- preserve advisory-only tensor semantics and CPU-only graph-truth finalization
- avoid silent fallback; every downgrade must remain machine-readable and explainable

## Required Interfaces

- `QueryPlanner::selectPath()` decision metadata needs downstream consumers for TensorRAG envelopes and explain/trace output.
- `PlannerObserver` integrations should export per-path latency, fallback, and module-gap counters into the evaluation telemetry surface.
- Distributed Path 5 follow-up work must preserve compatibility with shard-manifest freshness and exact-on-demand fragment loading contracts.

## Implementation Notes

- Keep the five canonical paths (`AnnOnly`, `AnnTensorSummary`, `AnnTensorExactGraph`, `DirectExactGraph`, `DistributedSummaryFirstExactOnDemand`) as the stable planner vocabulary.
- Prefer localized integration work over planner-core rewrites; the current planner already owns freshness gates, fallback reasons, and distributed-path selection.
- Any new downstream explainability field must be derived from existing typed planner outputs before introducing new planner-state duplication.

## Test Strategy

- Maintain the standalone `tests/epic2_evaluation/query_planner_test.cc` regression suite as the planner contract gate.
- Add focused scenario tests only for newly integrated downstream consumers (TensorRAG envelope, ANN frontdoor diagnostics, graph-validation metadata).
- Re-run planner benchmarks when changing routing policy thresholds or observer behavior.

## Performance Targets

- Keep planner decision overhead within the existing benchmark guardrail envelope in `benchmarks/epic2_evaluation/planner_decision_bench.cc`.
- Keep exact-fallback amplification measurable and explainable when new routing signals are introduced.
- Preserve distributed summary-first fan-out reduction without weakening graph-verified finalization.

## Security / Reliability

- Category C operations must remain CPU-only with fail-closed behavior.
- Tensor artifacts remain advisory only; no summary-only truth result may be introduced by future integrations.
- Missing or stale distributed manifests must continue to trigger exact fallback instead of degraded truth-bearing output.
