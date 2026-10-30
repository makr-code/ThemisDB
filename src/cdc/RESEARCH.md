---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source level: "level1"
SOT domain: "module-behavior"
Canonical source references: "src/cdc/ROADMAP.md; src/cdc/FUTURE_ENHANCEMENTS.md; include/cdc/cdc_delivery_contract.h; tests/cdc/; benchmarks/cdc/"
Related milestone: "DOC-WEEKLY-2026-44"
---

# CDC research and engineering questions

## Scope

This page records research directions that are explicitly present in CDC's
module roadmap and future-enhancement plan. It is not a literature review and
does not claim that a particular algorithm or external research result is
implemented. The delivery contract in
[`include/cdc/cdc_delivery_contract.h`](../../include/cdc/cdc_delivery_contract.h)
and current implementation/tests remain authoritative for behavior.

## Open engineering questions

The module roadmap identifies these areas for further validation:

1. **Transport degradation and replay consistency.** The planned fault matrix
   covers partial transport outages, backend timeouts, reconnect churn, ordered
   replay, and acknowledgement behavior across failover. See
   [`ROADMAP.md`](./ROADMAP.md) and
   [`FUTURE_ENHANCEMENTS.md`](./FUTURE_ENHANCEMENTS.md).
2. **Delivery and recovery state transitions.** Expand regression coverage for
   acknowledgement timeouts, redelivery, stale offsets, and DLQ/outbox
   transitions. The delivery contract specifies at-least-once delivery and
   canonical failure classes; it does not make the planned hardening complete.
3. **Operational diagnostics.** Improve actionable, contextual reporting for
   consumer lag, redelivery, transport failures, and outbox backlog. Existing
   diagnostic APIs and tests are in
   [`cdc_admin.h`](../../include/cdc/cdc_admin.h) and
   [`test_cdc_diagnostics_hardening.cpp`](../../tests/cdc/test_cdc_diagnostics_hardening.cpp).
4. **Performance evidence.** The roadmap calls for CDC-specific p95/p99
   baselines, repeated-run stability, and direct microbenchmarks of capture,
   listing, replay, and delivery. Existing CDC benchmarks are under
   [`benchmarks/cdc/`](../../benchmarks/cdc/); a planned target is not a
   measured result.
5. **Sustained multi-tenant and multi-transport behavior.** Future work calls
   for soak and degraded-network validation with bounded backlog and
   reproducible replay outcomes. Do not infer this validation from unit tests
   or the presence of tenant/transport classes.

## Evidence and validation approach

- Treat the normative contract and public headers as the behavior baseline.
- Use focused tests such as
  [`test_cdc_transport_degradation.cpp`](../../tests/cdc/test_cdc_transport_degradation.cpp),
  [`test_cdc_replay_ack_hardening.cpp`](../../tests/cdc/test_cdc_replay_ack_hardening.cpp),
  and [`test_cdc_diagnostics_hardening.cpp`](../../tests/cdc/test_cdc_diagnostics_hardening.cpp)
  for deterministic regressions.
- Use the dedicated CDC benchmark sources for measured performance work, and
  report environment and run variability before changing a baseline.
- Keep unresolved questions in the module roadmap/future-enhancement plan.
  Do not convert a planned validation into a claim of completed capability.

## Source references

- [CDC delivery contract](../../include/cdc/cdc_delivery_contract.h)
- [CDC module roadmap](./ROADMAP.md)
- [CDC future enhancements](./FUTURE_ENHANCEMENTS.md)
- [CDC delivery-gate benchmarks](../../benchmarks/cdc/bench_cdc_delivery_gates.cpp)

**Exact model backend not verifiable from final document text alone.**
