# Access Model Module - Production Requirements

<!-- Status: current | validated: 2026-09-21 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · SECURITY.md -->

## Purpose and Scope

This document defines the **canonical production readiness requirements** for the access_model module.
It covers the `AccessCoordinator`, `AgeBasedPolicy`, `AccessMetrics`, and tier-transition infrastructure.

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
- **MUST:** All tier registrations validated at coordinator initialization; missing or misconfigured
  tier references must cause fail-closed startup behavior.
- **MUST NOT:** Disable correlation-ID emission or structured logging in production deployments;
  these are required for incident diagnostics.
- **MUST NOT:** Run coordinator with an unbounded event queue in production; set explicit
  `max_queue_depth` to prevent unbounded memory growth.
- **MUST:** Metrics endpoint (Prometheus-compatible) must be reachable for SLO monitoring.

## Mandatory Security Requirements

- Tier transitions are logged with correlation IDs; audit trail must be preserved.
- Errors in tier-transition paths propagate as explicit error codes; no silent-permit fallback.
- Coordinator must not promote data to a higher-trust tier without a valid promotion event.
- Thread-pool lifecycle (init/shutdown) must be deterministic; no dangling worker threads.

## Operational Bounds

- Configuration values must be deployment-specific; default values are not production-safe.
- Resource limits (thread pool size, queue depth, tier sizes) must match deployment requirements.
- External tier dependencies (cache/storage backends) must be initialized before coordinator
  `initialize()` is called; late-binding tiers are not supported.
- Coordinator `shutdown()` must be called before process exit; in-flight promotions complete
  or are aborted deterministically.

## Minimum Production Check (Audit-ready)

- [ ] Thread pool size explicitly configured (not default-only)
- [ ] Age-based policy thresholds explicitly configured
- [ ] All tier references validated at startup
- [ ] Event queue depth bounded (`max_queue_depth` set)
- [ ] Correlation-ID emission and structured logging active
- [ ] Metrics endpoint reachable for SLO monitoring
- [ ] Coordinator `shutdown()` called on process exit
- [ ] Production mode indicated via `THEMIS_PRODUCTION_MODE` or `THEMIS_ENVIRONMENT`

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
