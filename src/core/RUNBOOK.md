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

# Core Incident Runbook

> Exact model backend not verifiable from final document text alone.

## Scope and safety

Use this runbook for failures during Core concern construction, adapter loading/replacement, or security-layer initialization. Core has no independent server control plane; restart, endpoint exposure, and deployment rollback are owned by the consuming service. Do not put secret values in commands, tickets, logs, or diagnostics.

## Startup rejected by validation or production policy

**Symptoms:** startup throws `Invalid ... configuration` or `Production mode violation`.

1. Capture the exception message with secret values redacted.
2. Check whether `THEMIS_PRODUCTION_MODE` is one of `1`, `true`, `True`, `TRUE`, `yes`, `Yes`, `on`, `On`, or `THEMIS_ENVIRONMENT` is `production`/`prod`.
3. Compare configured adapter names and fields against [`API.md`](API.md) and `include/core/config_validator.h`.
4. In production, configure tracer `otel`, `jaeger`, or `zipkin`, metrics `prometheus`, and the required security provider/JWT settings. Do not change the environment to development as a production workaround.
5. Correct the deployment configuration, validate it in a non-production environment, then roll out through the service's normal deployment process.

## Plugin load returns false

`AdapterRegistry::loadFromPlugin()` reports load errors to standard error and returns `false`. Check, in order:

1. The configured path is non-empty and points to an existing library readable by the service.
2. The library exports `themis_plugin_register` and was built for the supported host ABI (`kPluginAbiVersion`).
3. Its init function returns zero and registers the expected adapter.
4. If `kRequireSignature` is enabled, the adjacent `<library-path>.sig` file contains the expected SHA-256 hex digest for the library.

Keep the last known-good plugin package and configuration available for rollback. If a library was loaded into a running process, stop that process before replacing files; the registry retains loaded library handles until destruction.

## Adapter swap drain timeout

**Signal:** `hotSwap()` returns `false` and reports a drain timeout after its 100 ms window.

1. Do not assume in-flight calls were cancelled or that the old adapter has been destroyed.
2. Check callers holding shared handles returned by `resolve<T>()`; ensure they release handles after completing operations.
3. Inspect adapter-specific flush/shutdown and external-backend health.
4. If the new adapter is unhealthy, use the service's deployment/configuration rollback procedure. Core does not provide an atomic rollback API for registry hot-swap.

## Core readiness or health degrades

`ConcernsContext::healthCheck()` aggregates concern probes; current `readinessCheck()` delegates to it. Identify which concern reports unhealthy, then inspect the owning adapter's endpoint, credentials, connectivity, and lifecycle logs. HTTP health endpoints and alert routing are owned by the server/deployment configuration.

## Security-layer build failure

For production, `SecurityLayerBuilder::build()` rejects a missing/local key provider and missing or invalid JWT configuration. For HSM, verify `THEMIS_HSM_ENABLED=1`, a regular PKCS#11 library path, valid numeric slot (within `uint32_t`), and device availability. For Vault, validate endpoint/TLS and token injection without printing the token. Do not downgrade provider type or JWT validation to restore service.

## Incident closeout

Record timestamp, affected concern, redacted error, adapter/config revision, impact, recovery action, and whether credentials or plugin artifacts may have been exposed. Escalate suspected secret disclosure or untrusted plugin execution using the project security reporting process in [`SECURITY.md`](../../SECURITY.md). See [`OPERATIONS.md`](OPERATIONS.md) for steady-state checks and rollback boundaries.
