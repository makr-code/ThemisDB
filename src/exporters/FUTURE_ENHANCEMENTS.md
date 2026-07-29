# Exporters Module - Future Enhancements

<!-- Status: current | validated: 2026-07-29 -->
<!-- Links: README.md · ROADMAP.md · PERFORMANCE_EXPECTATIONS.md -->

## Scope

- hardening and refinement of export format/orchestration/safety runtime behavior
- expansion of deterministic reliability under stream/delta/join stress
- stricter benchmark-backed guardrails for exporter hot paths
- issue #5644 sync revalidated this focus set against `ROADMAP.md` priorities

### Roadmap-aligned horizon priorities

#### Short-term (3-6 months)
- tighten deterministic behavior for mixed-format and mixed-policy export permutations (Target: Q4 2026)
- expand regressions for join/stream/incremental checkpoint edge scenarios (Target: Q4 2026)
- improve operator-facing observability for hub upload and redaction incidents (Target: Q4 2026)

#### Mid-term (6-12 months)
- re-baseline p95/p99 and throughput envelopes for major exporter paths (Target: Q1 2027)
- broaden benchmark depth for join/predicate and template-heavy workflows (Target: Q1 2027)
- harden long-running reliability under sustained large-export workloads (Target: Q1 2027)

## Design Constraints

- exporter contracts remain backward compatible within major release line.
- policy/filter/PII/encryption guards remain explicit and fail closed.
- stream/incremental checkpoint semantics remain bounded and observable.
- export execution and upload transitions remain auditable.

## Required Interfaces

| Interface | Requirement |
|---|---|
| format interfaces | deterministic output behavior across supported formats |
| orchestration interfaces | stable stream/incremental/join contracts |
| safety interfaces | explicit authorization/filter/privacy protections |
| operations interfaces | actionable metrics and integration diagnostics |

## Implementation Notes

- complete hardening for format pipelines and orchestration internals.
- align security and governance behavior to bounded runtime contracts.
- standardize fail-closed behavior for unauthorized and unsafe export scenarios.
- unify diagnostics across stream/incremental/join and hub upload failure classes.

### 2026-07-29 hardening delivery snapshot
- Filter-parity was extended to incremental and join exporters using `ExportOptions::filter_expression` (merged-record evaluation for join paths).
- Incremental watermark progression now blocks updates on partial scans (size/error stop) to keep delta semantics fail-safe under non-monotonic input order.
- Join export now fails closed when right-side state was not initialized via `setRightCollection()`.
- Streaming encryption path now enforces deterministic precedence (`encryption_config` over legacy `encryption`) to avoid ambiguous/double-encryption behavior.

## Test Strategy

- unit and integration suites for format, stream, incremental, join, and upload paths.
- regressions for policy denials, predicate errors, and checkpoint recovery cases.
- deterministic stress runs for large export and redaction-heavy scenarios.
- release-profile benchmark runs for mapped exporters targets.

## Performance Targets

- major export hot paths remain within regression budgets.
- stream and incremental workflows remain stable at p95/p99 envelopes.
- benchmark manifests for mapped exporters targets reach no-missing-case status.

## Security / Reliability

- maintain strict authorization and privacy gates before export egress.
- preserve explicit failure signaling for unsafe or degraded export paths.
- enforce bounded memory/checkpoint behavior under pressure.
- keep diagnostics actionable for production export incidents.