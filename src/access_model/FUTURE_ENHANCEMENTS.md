# Access Model Module - Future Enhancements

<!-- Status: current | validated: 2026-09-21 -->
<!-- Links: README.md · ROADMAP.md · PERFORMANCE_EXPECTATIONS.md -->

## Scope

- refinement of promotion/demotion policy expressiveness beyond age-based rules.
- hardening of multi-tier coordinator behavior under concurrent load and degraded tier availability.
- expansion of observability and trace integration for distributed deployments.
- operator-grade diagnostics for tier transition incidents and policy conflicts.

## Design Constraints

- access_model contracts remain backward compatible within major release line.
- `AccessTier` interface must not change in a breaking way without a migration note and CHANGELOG entry.
- coordinator must remain opt-in via feature flag; existing cache/storage APIs must remain unaffected.
- no legacy or compatibility shim paths without explicit human approval.
- trace context propagation must remain non-blocking in hot-path operations.

## Required Interfaces

| Interface | Requirement |
|---|---|
| AccessCoordinator | deterministic promotion/demotion under concurrent event streams and tier permutations |
| AgeBasedPolicy | extensible threshold model allowing per-tier and per-key policy overrides |
| AccessMetrics | stable Prometheus-compatible surface with p50/p95/p99 histograms per transition type |
| EvictionListener / PromotionListener | explicit delivery semantics, bounded callback latency, and failure classification |
| Trace context | non-blocking propagation with correlation-ID flow through entire event chain |

## Implementation Notes

- extend policy model to support:
  - per-key or per-tag promotion/demotion threshold overrides in `age_based_policy.h`.
  - frequency-based policy alongside age-based decisions.
- harden coordinator for partial tier degradation:
  - explicit fail-closed behavior when a target tier is unavailable.
  - bounded retry and fallback path with structured diagnostic output.
- expand trace integration:
  - align `access_model_trace.h` correlation IDs with Wave D distributed tracing spans
    (see `docs/operability/RUNBOOK_ACCESS_MODEL_PROMOTION.md` §Wave D D1 cross-links).
  - OpenTelemetry-compatible span emission for promotion/demotion events.
- improve benchmark coverage:
  - per-tier latency benchmarks for all six tier transition types.
  - memory overhead benchmark under sustained high-frequency promotion events.

## Test Strategy

- unit and integration suites for policy-override and frequency-based paths.
- degraded-tier and fail-closed determinism tests.
- trace propagation correctness under concurrent event storms.
- release-profile benchmark runs for all GATE-ACM-01..06 target gates.
- minimum acceptance expansion (Q4 2026):
  - policy override matrix (per-key, per-tag, frequency vs age interaction)
  - negative-path validation (unavailable tier, malformed event payload, queue overflow)
  - coordinator shutdown determinism (in-flight promotion completion vs abort)

## Performance Targets

- promotion/demotion hot paths remain within GATE-ACM-01..06 release thresholds.
- concurrent coordinator and tier operations remain stable at p95/p99.
- benchmark manifests for all tier transition targets reach no-missing-case status.
- measurable targets:
  - regression threshold: <= 10% vs baseline for mapped access_model benchmark cases
  - L1→L2 promotion p99 <= 50 µs (GATE-ACM-01)
  - cache eviction→storage feedback p99 <= 100 µs (GATE-ACM-02)
  - cold→warm promotion p99 <= 100 ms (GATE-ACM-03)
  - event processing throughput >= 10 K events/sec (GATE-ACM-04)
  - memory overhead <= 50 MB (GATE-ACM-05)
  - policy decision p99 <= 10 µs (GATE-ACM-06)

## Security / Reliability

- coordinator must not silently drop or reorder promotion/demotion events.
- all tier transition decisions remain auditable via structured log records with correlation IDs.
- fail-closed on unavailable tier — no silent fallback to an unsafe promotion path.
- enforce bounded resource behavior: bounded thread pool, bounded event queue depth.
- diagnostics must remain actionable for production incidents and policy conflict resolution.
- reliability gate intent:
  - degraded tier states must remain observable, bounded, and explicitly surfaced to operators.
  - partial tier participation must never silently report success for a promotion/demotion.
