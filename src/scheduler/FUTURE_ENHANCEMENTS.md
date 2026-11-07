---
Author: ThemisDB Team
Created: 2026-10-31
Last Updated: 2026-11-07
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot; OpenAI API assistant
Human_Review: false
Human_Author: ThemisDB Team
Source level: 1
SOT domain: module-behavior
Canonical references: src/scheduler/ROADMAP.md; src/scheduler/ARCHITECTURE.md; src/scheduler/PRODUCTION_REQUIREMENTS.md; src/scheduler/PERFORMANCE_EXPECTATIONS.md
Related issue: #7449
---

Exact model backend not verifiable from final document text alone.

# Scheduler future enhancements

The versioned wave plan, estimates, dependencies, risks, metrics, and validation
criteria are maintained in the canonical [scheduler roadmap](ROADMAP.md). The
items below summarize its scope; they are proposals, not delivery commitments.

## Scope

- **v2.5.0:** replace skipped/simulated coordination and retention evidence,
  establish a production-path benchmark baseline, and obtain security and
  deployment-owner decisions.
- **v3.0.0:** specify delivery/recovery semantics and evaluate durable,
  distributed, and multi-tenant scheduling only if workload evidence requires
  those capabilities.

## Design Constraints

- Keep the shared coordination gate fail-closed; do not infer a delivery
  guarantee from leadership gating alone.
- Do not adopt DRF or other multi-resource policy without measured,
  heterogeneous per-tenant resource demand.
- Do not treat benchmark labels, smoke tests, or header maturity annotations as
  release evidence.
- Any public API/configuration change requires API-owner review and SemVer
  compatibility analysis.

## Required Interfaces

- Use `include/scheduler/scheduler_api_contract.h` and the relevant public
  declarations as the contract source; verify each guarantee against
  `src/scheduler/` implementation.
- Any proposed persistence, fencing, admission, or fairness interface requires
  a reviewed design and consumer/workload requirement before implementation.

## Implementation Notes

- Follow the six implementation phases in [ROADMAP.md](ROADMAP.md); keep
  `v2.5.0` evidence closure separate from `v3.0.0` architecture work.
- Retain the current owner decisions and unverified assumptions in
  [PRODUCTION_REQUIREMENTS.md](PRODUCTION_REQUIREMENTS.md).

## Test Strategy

- Exercise coordinator leadership loss, heartbeat failure, shutdown, recovery,
  and concurrent dispatch through production components.
- Replace simulated retention/burst claims with `TaskResultStore` and
  `HybridRetentionManager` tests.
- Validate benchmark smoke and release thresholds separately; run deterministic
  correctness tests independently from hardware-sensitive timing gates.

## Performance Targets

- Record a reproducible production-path baseline with workload, hardware,
  build settings, sample count, and an owner-approved statistic.
- Agree on a numeric regression threshold only after characterizing measurement
  variance; see [PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md).
- Report saturation queue-wait and per-tenant resource-share metrics before
  selecting a fairness or overload algorithm.

## Security / Reliability

- Require explicit request contexts and operation-limited permissions for
  security-sensitive calls; preserve fail-closed dispatch while coordination is
  unavailable.
- Obtain owner-approved audit/HMAC-key, retention, sandbox, rate-limit, retry,
  timeout, availability, and recovery policies before claiming deployment
  readiness.
- Treat loss/duplicate behavior during restart or failover as an explicit
  contract decision, not an assumed exactly-once guarantee.
