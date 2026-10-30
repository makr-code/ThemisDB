---
Author: "ThemisDB Team"
Created: "2026-09-21"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Access Model Module - Production Requirements

## Purpose and scope

This document states production requirements for the access_model module. A
requirement is not evidence that the implementation already satisfies it.
Current implementation limits are called out explicitly below.

## Document Boundary (Canonical Split)

- **`PRODUCTION_REQUIREMENTS.md` (this file):** mandatory production requirements (MUST/MUST NOT),
  security assumptions, operational bounds.
- **`README.md`:** functional overview, architecture context, API and usage examples.
- **`ROADMAP.md`:** delivery phases, open/completed features, readiness planning.
- **`FUTURE_ENHANCEMENTS.md`:** medium/long-term enhancements and research areas.

## Mandatory Production Requirements

- **MUST:** Coordinator thread pool size explicitly configured; default of 4 workers is not
  sufficient for all deployment profiles — set based on expected event throughput.
- **MUST:** Age-based policy thresholds explicitly configured at startup; default values are
  reference values only and must be tuned for the deployment's tier sizes and access patterns.
- **MUST:** Check the boolean result from `initialize()`; it validates non-null,
  healthy, correctly identified adapters and requires both cache and storage
  tiers. Adapter initialization and lifetime remain the application's duty.
- **MUST:** Handle `std::invalid_argument` from invalid policy/threshold updates.
- **MUST:** Verify production adapter `get`/`put`/`invalidate` behavior and
  reconcile partial demotions. Destination write precedes source invalidation;
  invalidation failure may leave both copies.
- **MUST:** Export and monitor `AccessModelMetrics` through the embedding
  application. This module does not provide a Prometheus endpoint; per-key
  counters retain at most 10,000 touched keys, per-tier counters are retained
  for registered tiers, and aggregate metrics expose current/maximum queue depth.
- **MUST:** Choose an explicit queue limit appropriate to the deployment. The
  pending task queue and demotion-plan registry are bounded; rejected callback
  work is reported through failure counters/history, while `promoteAsync()`
  returns an immediately completed failure future.
- **MUST:** Restrict logs and redact keys before external sharing; structured
  access-model log entries include keys.

## Ownership and security assumptions

- The application owns authorization, tenant isolation, adapter initialization,
  telemetry export, and deployment rollback.
- Cache/storage adapters and their listener pointers must remain valid while
  callbacks can occur.

## Operational Bounds

- Configuration values must be deployment-specific; default values are not production-safe.
- Resource limits and event ingress must match deployment requirements; the
  default queue capacity is a safety bound, not a workload-sizing recommendation.
- Stop event producers/unregister listener pointers before calling `shutdown()`;
  shutdown drains accepted work and can wait indefinitely for a blocked adapter.

## Minimum Production Check (Audit-ready)

- [ ] Thread pool size explicitly configured (not default-only)
- [ ] Age-based policy thresholds explicitly configured
- [ ] `initialize()` result checked and adapter initialization/lifetime verified
- [ ] Invalid policy updates handled as `std::invalid_argument`
- [ ] Real tier transfer, partial-demotion recovery, and backend errors verified by adapter tests
- [ ] Queue limit sized; rejection and shutdown-drain behavior monitored
- [ ] Telemetry exported by the application and compared with backend state
- [ ] Log access restricted and sensitive keys redacted for external reports
- [ ] Listener ownership and shutdown order tested
- [ ] Mapped release benchmarks run on representative hardware with artifacts

Do not mark this module production-ready on the basis of interface or benchmark
scaffolding alone while unmet implementation requirements remain.

## Review / Sourcecode Audit Evidence

### Affected files in scope

- `include/access_model/access_coordinator.h`
- `src/access_model/access_coordinator.cpp`
- `include/access_model/age_based_policy.h`
- `src/access_model/age_based_policy.cpp`
- `include/access_model/access_metrics.h`
- `src/access_model/access_metrics.cpp`
- `include/access_model/access_model_logging.h`
- `src/access_model/access_model_logging.cpp`
- `include/access_model/access_model_trace.h`
- `src/access_model/access_model_trace.cpp`

---

**AI provenance:** Exact model backend not verifiable from final document text alone.
