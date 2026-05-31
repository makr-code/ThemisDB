# CDC Module - Future Enhancements

<!-- Status: current | validated: 2026-05-31 -->
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

## Test Strategy

- unit and integration suites across capture, replay, delivery, and admin flows.
- regression scenarios for DLQ/outbox, timeout, and redelivery behavior.
- degraded transport/backend deterministic fault-path tests.
- release-profile benchmark runs for mapped CDC targets.

## Performance Targets

- event record/list/replay hot paths remain within release regression budgets.
- replication lag and WAN lag benchmark paths remain stable at p95/p99.
- benchmark manifests for mapped CDC targets reach no-missing-case status.

## Security / Reliability

- maintain strict delivery-state and replay contract integrity.
- preserve auditable CDC admin and failure decision paths.
- enforce bounded resource behavior in buffering and transport surfaces.
- keep diagnostics actionable for production CDC incidents.