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
- **MUST:** Validate the tier registry in the owning application before
  `initialize()`. The current method copies the supplied map and returns true
  without validating it.
- **MUST:** Call `AgeBasedPolicy::isValid()` before `setAgePolicy()` when policy
  validation is required; the coordinator does not call it.
- **MUST NOT:** Treat successful promotion/demotion counters or results as proof
  of data transfer. Current worker code does not invoke tier `get`/`put`.
- **MUST:** Export and monitor `AccessModelMetrics` through the embedding
  application. This module does not provide a Prometheus endpoint; per-key and
  per-tier coordinator queries currently return default metrics.
- **MUST NOT:** Claim bounded queue memory or configure `max_queue_depth` as if
  the module supports it. The task queue is unbounded and has no depth accessor.
  Production use that can generate unbounded event volume must remain blocked
  until the module or its caller provides tested backpressure.
- **MUST:** Restrict logs and redact keys before external sharing; structured
  access-model log entries include keys.

## Ownership and security assumptions

- The application owns authorization, tenant isolation, tier health checks,
  telemetry export, and deployment rollback.
- Cache/storage adapters and their listener pointers must remain valid while
  callbacks can occur.

## Operational Bounds

- Configuration values must be deployment-specific; default values are not production-safe.
- Resource limits and event ingress must match deployment requirements. The
  coordinator queue is currently unbounded.
- Stop event producers/unregister listener pointers before calling `shutdown()`;
  shutdown joins workers but does not drain queued tasks.

## Minimum Production Check (Audit-ready)

- [ ] Thread pool size explicitly configured (not default-only)
- [ ] Age-based policy thresholds explicitly configured
- [ ] Tier map validated by the owning application before initialization
- [ ] Policy validated with `AgeBasedPolicy::isValid()` before assignment
- [ ] Real tier transfer and backend-error behavior verified by adapter tests
- [ ] Bounded event ingress/backpressure and shutdown behavior implemented and tested
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
