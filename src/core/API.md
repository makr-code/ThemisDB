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

# Core API Contract

> Exact model backend not verifiable from final document text alone.

## Audience and scope

This page summarizes the public integration surface in `include/core/`. Header declarations and Doxygen comments are the API source of truth. This is not a list of concrete adapter-specific options; consult each adapter header and [`include/core/README.md`](../../include/core/README.md).

## Main contracts

| API | Contract and edge behavior |
|---|---|
| `ConcernsContext::create()` / `create(config)` | Creates the configured concern set. Invalid settings throw `std::runtime_error`. Empty tracer/metrics adapter names are derived from their enable flags. In production mode, no-op tracing or metrics causes construction to fail. |
| `ConcernsContext::createCustom(...)` | Transfers supplied adapters into a shared context. Required base adapters must be non-null. Overloads may default omitted circuit breaker, secrets, feature flags, or audit log to no-op implementations. |
| `ConcernsContext::createNoOp()` | Creates a no-op context for test/development use; production mode rejects it. |
| Accessors (`logger()`, `tracer()`, `metrics()`, `cache()`, `secrets()`, `circuitBreaker()`, `featureFlags()`, `auditLog()`) | Return references owned by the context. Do not retain references across replacement or context destruction. |
| `resolve<T>()` | Returns a shared handle for built-in concern interfaces. Other types are looked up in `AdapterRegistry`; an unregistered type returns an empty `shared_ptr`. |
| `replaceLogger/Tracer/Metrics/Cache/Secrets/FeatureFlags/AuditLog(...)` | Replaces one owned concern. Null arguments are rejected with `std::invalid_argument`; use owning handles for work that may overlap a replacement. |
| `AdapterRegistry::registerAdapter<T>(id, adapter, validator, metadata)` | Rejects an empty ID, null pointer, API version below `kCurrentApiVersion`, or validator rejection with `std::invalid_argument`. Metadata ID is filled from `id` if empty. |
| `AdapterRegistry::hotSwap<T>(adapter, metadata)` | Rejects null. Installs the new adapter and waits up to 100 ms for references to the old adapter; returns `false` on drain timeout. |
| `AdapterRegistry::loadFromPlugin(path, adapter_id)` | Loads a shared library exporting `themis_plugin_register`; returns `false` for load, entry-point, init, or required digest failures. |
| `healthCheck()` / `readinessCheck()` | Aggregate per-concern probes. In current code readiness delegates to health; HTTP exposure belongs to the server module. |
| `flush()` / `shutdown()` | Dispatch lifecycle hooks to owned concerns; invoke during orderly service shutdown according to application lifecycle. |

## Configuration contract

`ConcernsContext::Config` defines the supported adapter names, defaults, and values. `ConfigValidator` is authoritative for allowed values. Current categories are logger (`spdlog`, `noop`), tracer (`otel`, `jaeger`, `zipkin`, `noop`, or empty for auto), metrics (`prometheus`, `noop`, or empty for auto), cache (`inmemory`, `redis`, `noop`), circuit breaker (`default`, `noop`), feature flags (`inmemory`, `noop`), audit (`inmemory`, `noop`), and secrets (`noop`, `inmemory`, `env`). Redis requires a non-empty URL. In production, explicit adapter names override enable booleans but must still satisfy production policy.

`ProductionMode::isEnabled()` activates on the accepted values documented in [`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md). `SecurityLayerBuilder::build()` additionally requires a configured non-local key provider and valid JWT settings in production.

## Caller obligations

- Keep secrets out of logs, traces, metric labels, and exception text.
- End spans or use `ScopedSpan`; verify `isValid()` where the adapter contract requires it.
- Treat plugin code as native code running with host privileges. The plugin ABI is C-linkage function `themis_plugin_register` with host ABI version `kPluginAbiVersion`.
- Use `AdapterTrustPolicy::kRequireSignature` where required, while recognizing that a co-located SHA-256 digest is not asymmetric publisher authentication.

See [`IMPLEMENTATION.md`](IMPLEMENTATION.md) for lifecycle invariants and [`TRUST.md`](TRUST.md) for risk boundaries.
