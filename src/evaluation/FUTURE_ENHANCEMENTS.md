# Evaluation Module - Future Enhancements

<!-- Status: current | validated: 2026-07-29 -->
<!-- Links: README.md · ROADMAP.md · PERFORMANCE_EXPECTATIONS.md · MODULE_EVIDENCE.md -->

## Scope

- evolve the existing EPIC 2 codebase from partial source availability into Phase 3-6 validated production behavior
- extend planner decisions into integrated evaluation, telemetry, and retrieval workflows without weakening explicit fallback semantics
- tie module promotion to measurable build/test/benchmark evidence rather than source presence alone

## Design Constraints

- maintain compatibility across EPIC 1 / EPIC 2 / EPIC 3 dependency boundaries
- keep approximation, planner, and artifact-lifecycle policy behavior explicit and testable
- fail safely when hardware/profile assumptions, shard manifests, or freshness prerequisites are not satisfied
- preserve advisory-only tensor semantics and CPU-only graph-truth finalization
- avoid silent fallback; every downgrade must remain machine-readable, explainable, and benchmark-observable

## Required Interfaces

- `QueryPlanner::selectPath()` decision metadata needs downstream consumers for TensorRAG envelopes, explain output, and routing diagnostics.
- `PlannerObserver` integrations should export per-path latency, fallback, and readiness counters into the evaluation telemetry surface.
- Retrieval-metrics, ablation, approximation, and artifact-lifecycle surfaces need canonical error/policy status outputs for Phase 3 hardening.
- Benchmark and evidence surfaces must stay traceable to `benchmarks/epic2_evaluation/` and `src/evaluation/MODULE_EVIDENCE.md`.

## Implementation Notes

- Keep the five canonical planner paths (`AnnOnly`, `AnnTensorSummary`, `AnnTensorExactGraph`, `DirectExactGraph`, `DistributedSummaryFirstExactOnDemand`) as stable vocabulary.
- Prefer localized hardening of already-shipped contracts over introducing new abstraction layers.
- Derive new explainability or telemetry fields from existing typed outputs before adding duplicated planner state.
- Treat source/test/benchmark presence as insufficient for promotion until executable evidence is captured for the current cycle.

## Test Strategy

- Maintain `tests/epic2_evaluation/` as the module contract gate and keep file/test registration aligned with the local `CMakeLists.txt`.
- Expand focused scenario tests primarily around new Phase 3 error/policy cases and downstream consumer integrations.
- Re-run or refresh benchmark evidence whenever routing policy thresholds, fallback logic, or observer behavior changes.
- Preserve a justified-gap path in `MODULE_EVIDENCE.md` when environment blockers prevent executable validation.

## Performance Targets

- Keep planner decision overhead within the guardrail envelope established by `benchmarks/epic2_evaluation/planner_decision_bench.cc`.
- Establish benchmark-backed baselines for benchmark-matrix, artifact-staleness, and storage-strategy follow-up paths before integration promotion.
- Keep exact-fallback amplification measurable and explainable whenever new routing signals are introduced.
- Preserve distributed summary-first fan-out reduction without weakening graph-verified finalization.

## Security / Reliability

- Category C operations must remain CPU-only with fail-closed behavior.
- Tensor artifacts remain advisory only; no summary-only truth result may be introduced by future integrations.
- Missing or stale distributed manifests must continue to trigger exact fallback instead of degraded truth-bearing output.
- Production promotion must remain blocked until runtime policy errors, executable evidence, and benchmark gates are all explicitly satisfied.
