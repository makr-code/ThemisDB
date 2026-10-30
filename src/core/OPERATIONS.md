---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Core Operations

> Exact model backend not verifiable from final document text alone.

## Operational ownership

Core is an in-process library, not a standalone service. The consuming service owns process startup/shutdown, health endpoint publication, external exporter/Redis endpoints, deployment rollout, and alert delivery. Core provides `flush()`, `shutdown()`, `healthCheck()`, and `readinessCheck()` on `ConcernsContext`.

## Startup controls

- Set `THEMIS_PRODUCTION_MODE` or `THEMIS_ENVIRONMENT` explicitly in production; see [`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md) for accepted values.
- Configure production tracing (`otel`, `jaeger`, or `zipkin`) and metrics (`prometheus`); a no-op configuration is rejected in production.
- Configure security key provider and JWT validation before startup. Do not use `LOCAL` in production.
- Keep cache limits and metric cardinality bounded. Current defaults include a 1000 cardinality limit and in-memory cache settings from `ConcernsContext::Config`; tune based on actual workload and memory measurements.
- Run a startup smoke test after adapter/configuration changes and verify each configured external dependency from the deployed service.

## Signals and diagnosis

| Signal | Interpretation / action |
|---|---|
| Context creation exception | Config or production policy rejected; use the formatted validation error and check `ConfigValidator`. |
| `healthCheck()` concern reports unhealthy | Diagnose that adapter's own dependency and lifecycle; readiness currently mirrors health. |
| Plugin loader error on standard error | Path, digest, library load, ABI symbol, or init failure; see [`RUNBOOK.md`](RUNBOOK.md). |
| Registry hot-swap returns `false` | Existing references did not drain before 100 ms; investigate long-lived `resolve<T>()` handles. |
| Cache miss or stale value across nodes | In-memory cache is process-local; verify selected backend and invalidation design rather than assuming cluster coherence. |
| Missing trace in async work | Propagate with `ContextPropagation`; thread-local state alone does not cross arbitrary thread boundaries. |

The core API does not promise a universal metric-name inventory or publish its health results over HTTP. Use the actual adapter and server instrumentation as the operational source for exported names and endpoints; do not infer them from this guide.

## Lifecycle and recovery

1. During graceful shutdown, stop accepting new service work first.
2. Call context `flush()` to flush concern buffers, then `shutdown()` to invoke adapter lifecycle hooks; follow the service's normal ownership order.
3. For a failed configuration or adapter rollout, restore the last known-good deployment configuration/package and restart through the service's supported rollback mechanism.
4. For plugin library changes, stop the process before replacing a library file: the registry keeps a library handle alive until it is destroyed.
5. Re-run the startup smoke test and inspect health/readiness before returning traffic.

No core-level automated recovery or dynamic rollback guarantee is asserted. External dependency restart/failover and service traffic management belong to deployment operations.

## Escalation and evidence

Escalate repeated production-policy failures, unexplained health failures, plugin load/signature failures, cache consistency incidents, or suspected credential exposure to the owning service/module maintainers; suspected security incidents follow the project process in [`TRUST.md`](TRUST.md). Preserve redacted configuration, build revision, adapter versions, timestamps, exception text, and relevant service logs.
