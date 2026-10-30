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

# Core Module Guide

> Exact model backend not verifiable from final document text alone.

## Who should use Core?

Use Core when a module needs shared logging, tracing, metrics, cache, secrets, feature-flag, audit, or circuit-breaker behavior. Core supplies interface-based contracts and a `ConcernsContext`; it does not own application protocols, persistence, query execution, embeddings, or model inference. See [`EMBEDDING.md`](EMBEDDING.md) for the explicit feature boundary.

## Integrating a consumer

1. Include `core/concerns/concerns_context.h` and accept/share a `std::shared_ptr<ConcernsContext>` at the subsystem boundary.
2. Construct the context once during application startup using `ConcernsContext::create(config)` after configuring the required adapters.
3. Pass the context to consumers rather than constructing framework-specific loggers or tracers in each module.
4. Use interface operations via `context->logger()`, `tracer()`, `metrics()`, and other accessors. Use `resolve<T>()` when a replaceable owning handle is needed.
5. Propagate trace context explicitly across asynchronous boundaries. `ContextScope` is thread-local and restores the previous context at scope exit; `ContextPropagation::propagate()` creates the supported `std::future` handoff.
6. On shutdown, call `flush()` and `shutdown()` as appropriate before releasing the owning context.

## Environment and configuration

For development and tests, no-op adapters are available. For production, set the production-mode environment deliberately and configure a non-noop tracing adapter plus Prometheus metrics; otherwise construction fails. The security layer also requires an accepted production key provider and JWT validation configuration. See [`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md) before deployment.

Do not assume `createCustom()` checks all required pointers: supply valid logger, tracer, metrics, and cache implementations. For configuration-driven setup, use the accepted values in [`API.md`](API.md) and inspect `ConfigValidator` when changing them.

## Testing integration

Use no-op implementations or test doubles to isolate a consumer's behavior. Assert both the consumer result and calls made to injected interfaces. For context/registry changes, add focused tests under `tests/core/`; targets are generated from each `test_*.cpp` stem by [`tests/core/CMakeLists.txt`](../../tests/core/CMakeLists.txt).

## Common integration errors

- Startup throws: inspect the formatted validation error and compare each adapter name with `ConfigValidator`.
- Production mode violation: enable required production adapters; do not bypass the guard by using `createNoOp()`.
- Unregistered extension resolves empty: register the interface type before resolution and check the returned handle.
- Trace disappears in async work: propagate it explicitly; thread-local state is not inherited by arbitrary threads.
- Credentials appear in diagnostics: remove secret material from caller-provided log/trace fields and rotate exposed credentials.

Related engineering references: [`ARCHITECTURE.md`](ARCHITECTURE.md), [`IMPLEMENTATION.md`](IMPLEMENTATION.md), [`RUNBOOK.md`](RUNBOOK.md), and [`OPERATIONS.md`](OPERATIONS.md).
