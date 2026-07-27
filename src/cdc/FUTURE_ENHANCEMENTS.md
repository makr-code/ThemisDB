# CDC Module - Future Enhancements

<!-- Status: current | validated: 2026-07-27 -->
<!-- Links: README.md · ROADMAP.md · PERFORMANCE_EXPECTATIONS.md -->

## Scope

- hardening and refinement of CDC capture/delivery/replay runtime paths
- expansion of deterministic reliability for transport, DLQ, and outbox behavior
- stricter benchmark-backed guardrails for CDC hot paths

## Design Constraints

- CDC contracts remain backward compatible within major release line.
- capture/replay behavior remains deterministic under supported configurations.
- transport degradation paths remain bounded and observable.
- delivery semantics remain auditable through explicit state transitions.

## Required Interfaces

| Interface | Requirement |
|---|---|
| capture/replay interfaces | stable event record/replay semantics |
| delivery interfaces | explicit ack/redelivery and timeout behavior |
| transport interfaces | clear capability and failure classification |
| admin/ops interfaces | reliable control and diagnostics visibility |

## Implementation Notes

- tighten consistency of consumer-group and delivery-tracker failure semantics.
- standardize diagnostics across lag, replay, and redelivery classes.
- expand transport degradation resilience and failover behavior.
- continue replacing proxy-like mappings with dedicated CDC benchmarks.

### Short-term Delivery Plan (Q3-Q4 2026)

- transport/replay hardening:
  - define deterministic degradation matrix for partial transport outages, reconnect churn, and backend timeout combinations.
  - enforce ordered replay + idempotent ack behavior across failover transitions.
- regression expansion:
  - expand focused replay/ack/timeout permutations with explicit DLQ/outbox transition assertions.
  - cover malformed-event and stale-offset paths with fail-closed classification checks.
- operator diagnostics:
  - normalize lag/redelivery/stream-integrity diagnostics into actionable, class-stable outputs.
  - require stream-id and consumer-group context in failure telemetry assertions.

### Mid-term Delivery Plan (Q1 2027)

- benchmark re-baselining:
  - establish CDC-specific release baselines for p95/p99 latency and throughput under release profiles.
  - require stable multi-run variance before baseline promotion.
- dedicated microbench coverage:
  - replace proxy-like mappings with direct microbenchmarks for capture/list/replay/delivery hot paths.
  - close benchmark-manifest missing-case gaps for CDC critical functions.
- sustained consistency hardening:
  - validate multi-tenant and multi-transport determinism under long-running mixed workload and degraded-network conditions.
  - enforce bounded backlog/memory behavior with reproducible replay outcomes.

## Test Strategy

- unit and integration suites across capture, replay, delivery, and admin flows.
- regression scenarios for DLQ/outbox, timeout, and redelivery behavior.
- degraded transport/backend deterministic fault-path tests.
- release-profile benchmark runs for mapped CDC targets.

## Performance Targets

- event record/list/replay hot paths remain within release regression budgets.
- replication lag and WAN lag benchmark paths remain stable at p95/p99.
- benchmark manifests for mapped CDC targets reach no-missing-case status.
- baseline updates require documented variance stability across repeated release-profile runs.

## Security / Reliability

- maintain strict delivery-state and replay contract integrity.
- preserve auditable CDC admin and failure decision paths.
- enforce bounded resource behavior in buffering and transport surfaces.
- keep diagnostics actionable for production CDC incidents.