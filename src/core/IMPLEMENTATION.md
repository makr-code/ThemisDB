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

# Core Implementation Guide

> Exact model backend not verifiable from final document text alone.

## Ownership and entry points

Internal implementations live under `src/core/`; public contracts are in `include/core/`. The central wiring implementation is [`concerns_context.cpp`](concerns/concerns_context.cpp), with interfaces in [`concerns_context.h`](../../include/core/concerns/concerns_context.h). Keep external framework calls inside adapters rather than adding them to consuming modules.

| Concern | Contract / implementation entry |
|---|---|
| Context and configuration | `include/core/concerns/concerns_context.h`, `src/core/concerns/concerns_context.cpp` |
| Adapter registry and runtime plugins | `include/core/concerns/adapter_registry.h`, `src/core/concerns/adapter_registry.cpp` |
| Adapter metadata/signing | `include/core/concerns/adapter_metadata.h`, `adapter_signing.h`, `src/core/concerns/adapter_signing.cpp` |
| Trace context | `include/core/concerns/context_propagation.h`, `src/core/concerns/context_propagation.cpp` |
| Security startup | `include/core/security_initialization.h`, `src/core/security_initialization.cpp` |

## Required invariants

- Add a concern through an interface and wire it at the context factory; avoid direct framework coupling in downstream code.
- Validate configuration before constructing adapters. Unsupported adapter names and missing Redis URLs are errors, not an implicit Redis-to-memory fallback.
- Production mode must not silently use no-op tracing or metrics. `createNoOp()` is for tests/development, not production paths.
- Use `resolve<T>()` shared handles when adapter replacement can occur concurrently. A raw reference returned by an accessor is not an owning lifetime handle; do not retain it across replacement or context shutdown.
- `createCustom()` callers must provide non-null required logger, tracer, metrics, and cache implementations. Optional concerns default to no-op only in overloads that define those defaults.
- Registry adapters must use valid metadata/API versions; callers must handle null results from unregistered `resolve<T>()`.
- Plugin modules execute native code in-process. Use an explicit trusted deployment process and check every `loadFromPlugin()` result.
- Preserve `ContextScope`'s same-thread, stack-bound behavior. Use `ContextPropagation::propagate()` for the provided async handoff; do not assume arbitrary executors inherit thread-local state.

## Change workflow

1. Read the relevant interface and implementation before changing behavior.
2. Update the implementation and the matching focused tests in `tests/core/`.
3. Cover invalid inputs and failure paths, including concurrency or backend failures where applicable.
4. Update [`API.md`](API.md), [`ARCHITECTURE.md`](ARCHITECTURE.md), [`SECURITY.md`](SECURITY.md), or operational docs when a contract, boundary, or recovery action changes.
5. Build and execute the focused target(s) registered by `tests/core/CMakeLists.txt`; report the exact configure/build/test command and result. A registered test is not evidence that it ran.

## Failure and compatibility notes

Factory validation errors throw `std::runtime_error`; null runtime replacements and invalid registry registration throw `std::invalid_argument`. `AdapterRegistry::hotSwap()` can return `false` after the 100 ms drain window while old shared handles remain live. Plugin loader failures return `false`. Preserve these distinct failure modes when adding wrappers or translating errors.

The plugin ABI is versioned independently by `kPluginAbiVersion`; breaking ABI changes require coordinated host/plugin updates. See [`API.md`](API.md), [`MODELING.md`](MODELING.md), and [`MODULE_EVIDENCE.md`](MODULE_EVIDENCE.md).
