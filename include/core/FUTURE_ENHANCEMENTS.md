> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/core/FUTURE_ENHANCEMENTS.md -->

# Core Module — Public Header Future Enhancements

**Module Path:** `include/core/`
**Canonical implementation enhancements:** [`../../src/core/FUTURE_ENHANCEMENTS.md`](../../src/core/FUTURE_ENHANCEMENTS.md)

---

## Scope

This document covers planned enhancements to the **public header contract** in `include/core/` — new types, interface additions, deprecation removals, and header-level API improvements. Enhancements that touch both headers and implementation are tracked primarily in the canonical source-level document:

→ [`../../src/core/FUTURE_ENHANCEMENTS.md`](../../src/core/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Headers must remain backward-compatible within a major version.
- `[x]` `#pragma once` guard required on every header.
- `[x]` No implementation code in headers (exception: `constexpr` helpers and header-only utilities).
- `[x]` All factory functions and error-returning methods must be `[[nodiscard]]`.
- `[x]` Bootstrap headers must be includable independently (no hidden include-order dependency).

---

## Required Interfaces (Header Contract)

| Interface | Declared In | Consumer | Status |
|-----------|------------|----------|--------|
| `ConfigValidator::*` validation helpers | `config_validator.h` | Server entrypoint, tests | ✅ Stable |
| `StorageInitializationBuilder::build()` | `storage_initialization.h` | Server entrypoint, tests | ✅ Stable |
| `SecurityLayerBuilder::build()` | `security_initialization.h` | Server entrypoint, tests | ✅ Stable |
| `IndexInitializationBuilder::build()` | `index_initialization.h` | Server entrypoint, tests | ✅ Stable |
| `QueryEngineBuilder::build()` | `query_engine_builder.h` | Server entrypoint, tests | ✅ Stable |
| `IHealthProbeRegistry::checkAll()` | `health_probe.h` | HTTP health endpoint | ✅ Stable |
| `IConfigHotReloader::watch()` / `reload()` | `config_hot_reloader.h` | Server runtime | ✅ Stable |
| `ProductionMode::*` checks | `production_mode.h` | Production server entrypoint | ✅ Stable |

---

## Planned Enhancements

### Short-Term (Q3 2026)

- `bootstrap_orchestrator.h` — unified orchestrator enforcing ordered initialization; wraps individual initializers and propagates errors with structured diagnostics.
- Extend `QueryEngineBuilder` to accept an LLM/LoRA backend factory via `withLLMBackend(ILLMBackendFactory&)`.
- Add `TensorLayerInitializer` header stub for Tensor Mid-Layer wiring (linked to `FUTURE_PLAN.md` Phase 2).

### Medium-Term (Q4 2026)

- `shutdown_coordinator.h` — reverse-order graceful shutdown; ensures WAL flush before index close before security teardown.
- Deprecation marker `[[deprecated("Use bootstrap_orchestrator instead")]]` on direct initializer-builder entry points once orchestrator is stable.

### Long-Term

- Compile-time enforcement that production safety checks run before health readiness is published (static analysis / concept constraint).
- Header-level API versioning annotations for embedder ABI compatibility tracking.
