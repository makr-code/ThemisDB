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

# Core Runtime Model

> Exact model backend not verifiable from final document text alone.

## Purpose and boundary

This is the engineering model of Core's configuration and adapter lifecycle, for developers reasoning about invariants and test coverage. Core contains no trained or predictive model: its behavior is selected by configuration and implemented by adapters and stateful utilities.

The public contracts are in [`ConcernsContext`](../../include/core/concerns/concerns_context.h), [`AdapterRegistry`](../../include/core/concerns/adapter_registry.h), [`ICircuitBreaker`](../../include/core/concerns/i_circuit_breaker.h), and [`ProductionMode`](../../include/core/production_mode.h).

## Context construction

`ConcernsContext::create(config)` validates log, tracing, cache, and adapter settings before constructing the selected implementations. Empty tracer and metrics adapter names are resolved from their enable flags (`otel`/`prometheus` when enabled, `noop` otherwise); an explicit adapter name takes precedence over the flag. In production mode, the resulting tracer must be `otel`, `jaeger`, or `zipkin`, and metrics must be `prometheus`; otherwise construction throws.

The resulting context owns logger, tracer, metrics, cache, circuit breaker, secrets, feature flags, audit log, and an `AdapterRegistry`. Its factory-produced concern pointers are shared-owned. Legacy accessors return references; `resolve<T>()` returns owning shared handles for built-in concern interfaces, while unregistered extension types resolve through the registry and may return an empty pointer.

## Adapter registry lifecycle

The registry maps C++ interface types to shared adapter handles:

1. Registration rejects an empty ID, null adapter, API version below `kCurrentApiVersion`, or metadata rejected by an optional validator.
2. Resolution takes a shared lock and returns a shared handle, or null when the type is absent.
3. `hotSwap<T>` installs the new handle under an exclusive lock, releases the lock, then waits for existing references to the old handle for up to `kHotSwapTimeoutMs` (100 ms).
4. A drain timeout returns `false` and writes a warning to standard error. It does not cancel in-flight calls.

Plugin library handles are retained by the registry and released on registry destruction. Plugin init or load failures return `false`; callers must check the result.

## Failure states

| Condition | Observable behavior |
|---|---|
| Invalid config in `create(config)` | Throws `std::runtime_error` with validation details |
| Production mode with no-op tracing or metrics | Throws `std::runtime_error` |
| Null adapter passed to `replace*` or `hotSwap` | `std::invalid_argument` for the documented replacement/registry paths |
| Registry type not present | `resolve<T>()` returns an empty `shared_ptr` |
| Plugin missing entry point, failing ABI init, or rejected digest | `loadFromPlugin()` returns `false` and logs the cause to `std::cerr` |
| Circuit breaker denies a request | `call(fn, fallback)` executes the fallback; a thrown function records failure and rethrows |

`ContextPropagation` stores the current context in thread-local state. `propagate(fn)` captures a child context and installs it in the launched async task; context does not implicitly cross arbitrary thread or executor boundaries.

## Validation implications

Tests should cover transitions and observable outcomes, not just constructed object types: production and development configuration, validation rejection, absent registry types, replacement with live handles and drain timeout, plugin load failures, circuit-breaker state changes, and context restoration after scoped/async work. See [`RESEARCH.md`](RESEARCH.md) for a reproducible evaluation method and [`IMPLEMENTATION.md`](IMPLEMENTATION.md) for change invariants.
