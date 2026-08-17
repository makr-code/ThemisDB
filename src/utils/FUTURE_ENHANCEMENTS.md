# Utils Module - Future Enhancements

<!-- Status: current | validated: 2026-07-18 -->
<!-- Links: README.md · ROADMAP.md · PERFORMANCE_EXPECTATIONS.md -->

## Scope

- hardening and refinement of widely consumed shared helper behavior
- stronger diagnosability and resilience for privacy, audit, crypto, and runtime utility paths
- benchmark-backed release guardrails for selected utility hotspots

## Design Constraints

- shared helper contracts remain backward compatible within the major line.
- failure behavior remains explicit and bounded despite wide consumer fan-out.
- external dependency degradation remains visible rather than hidden by silent fallback.
- performance guardrails focus on real hot paths with benchmark evidence.

## Required Interfaces

| Interface | Requirement |
|---|---|
| observability interfaces | explicit audit, log, and tracing helper behavior |
| privacy interfaces | bounded scan and pseudonymization semantics |
| key interfaces | explicit derivation and lifecycle behavior |
| runtime helper interfaces | reusable support semantics without business-domain ownership |

## Implementation Notes

- deepen failure-path consistency across audit, privacy, and runtime helpers.
- tighten contract clarity for high-fan-out helper APIs.
- broaden integration and stress coverage where utility misuse would have wide blast radius.
- expand benchmark depth only for hot paths with release significance.

### PKI Client Production Signing

- fail-closed guarantee: 100% of pinning-enabled REST signing/verification requests with invalid pin material must be rejected before network success (Target: Q4 2026).
- diagnostics unification: emit structured fields `{path, mode, pinning_enabled, failure_reason}` for REST/local/CSR signing and verify flows (Target: Q4 2026).
- test-path isolation: ensure production presets return `ok=false` for unconfigured signing and disallow test fallback activation outside `THEMIS_TEST_MODE` builds (Target: Q4 2026).

## Test Strategy

- unit and integration suites for audit, privacy, key, compression, and concurrency helpers.
- regressions for overload, fallback, and dependency-loss scenarios.
- deterministic concurrency and stress runs for high-fan-out helper paths.
- release-profile benchmark runs for mapped utility targets.

## Performance Targets

- mapped utility hotspots remain inside regression budgets.
- p95/p99 latency and throughput for selected utility benchmarks remain stable.
- mapped benchmark manifests reach no-missing-case status for release gating.

## Security / Reliability

- preserve explicit failure signaling for privacy, audit, and key helper faults.
- maintain bounded degradation under overload or dependency loss.
- keep diagnostics actionable for shared-helper incidents.
- prevent silent contract drift across widely consumed utilities.