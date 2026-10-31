---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Performance Module Trust and Privacy

## Trust boundaries

The performance module executes within the database process and consumes
workload observations, tuning configuration, host capability information, and
caller-provided vectors or query-operator inputs. Its metric exporters produce
operational telemetry for downstream monitoring systems. This module does not
establish caller identity or provide an authorization boundary for exported
metrics; the service and deployment must enforce access control at their
interfaces and monitoring endpoints.

## Data exposure risks

- Query type labels, operation names, hot-table identifiers, workload counts,
  latency, and resource-use data can reveal application activity even when
  query text and values are not recorded.
- High-cardinality or attacker-influenced labels can increase monitoring
  storage and processing load.
- Tuning or model inputs that are malformed, unbounded, or not representative
  can cause resource pressure or unsafe optimization choices.
- Capability-dependent paths expose host-specific behavior and may fail when
  build, hardware, kernel, or permission assumptions do not hold.
- Vector-index graph and metadata files contain identifiers and vector-derived
  data; protect them as application data and preserve their integrity together.

## Controls and operator responsibilities

`PRODUCTION_REQUIREMENTS.md` requires that performance-monitoring data contain
no sensitive user data and that a masking policy be active. It also requires
startup validation for security-relevant configuration and fail-closed
behavior for missing or invalid values. Verify that deployment-specific
configuration and masking are actually active; this module documentation alone
does not demonstrate that every exporter enforces masking.

Keep metric endpoints authenticated and network-restricted at the owning
service boundary. Apply least-privilege access and retention to telemetry and
index files. Validate numeric bounds, dimensions, capability prerequisites,
and resource limits before enabling adaptive or hardware paths. Preserve
explicit errors and fallback evidence rather than treating an unavailable
path as successful acceleration.

The shared `PerfError` values are documented in
[`performance_api_contract.h`](../../include/performance/performance_api_contract.h),
but individual APIs also use exceptions, boolean status, and negative errno
results. Callers must preserve these distinctions and avoid leaking internal
configuration or data through externally visible error messages.

## Incident handling

For suspected privacy leakage, unauthorized telemetry access, malformed
configuration causing resource exhaustion, or integrity issues in vector-index
files, restrict access or disable the affected optional path using the
service's approved change process. Preserve relevant configuration, metrics,
and file-integrity evidence while avoiding copies of secrets or raw user
payloads. Escalate through the project security process; follow
[RUNBOOK.md](RUNBOOK.md) for performance incident diagnostics and
[OPERATIONS.md](OPERATIONS.md) for monitoring and rollback discipline.

Exact model backend not verifiable from final document text alone.
