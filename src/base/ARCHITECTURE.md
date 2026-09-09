# Architecture - Base Module

<!-- Status: current | validated: 2026-09-09 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The base module is the runtime foundation for module extensibility. It composes loading, validation, dependency resolution, isolation, and reload behavior into a single platform-facing layer consumed by other modules.

## Main Execution Planes

1. Loader and trust plane
- dynamic module load/unload lifecycle
- trust, integrity, and policy gating before activation

2. Dependency and registry plane
- dependency graph and compatibility ordering
- remote registry retrieval and validation path

3. Isolation and runtime plane
- module sandbox boundaries and limits
- wasm isolation and runtime injection hooks

4. Reload and operations plane
- hot reload with rollback semantics
- observability and runtime status surfaces

## Core Contracts

| Contract | Behavior |
|---|---|
| loader interfaces | validate and activate/deactivate modules safely |
| dependency interfaces | express and resolve load order constraints |
| sandbox interfaces | enforce bounded execution policies |
| reload interfaces | coordinate swap/rollback without full restart |

## Failure Semantics

- invalid or untrusted module artifacts fail closed during load.
- dependency/compatibility conflicts return structured resolution failures.
- sandbox/runtime setup failures block unsafe execution paths.

## Sourcecode Verification (Module: base/architecture)

- Verified files:
  - src/base/module_loader.cpp
  - src/base/plugin_dependency_graph.cpp
  - src/base/module_sandbox.cpp
  - src/base/wasm_plugin_sandbox.cpp
  - src/base/wasm_runtime_injector.cpp
  - src/base/hot_reload_manager.cpp
  - src/base/remote_registry_client.cpp
- Verified architecture claims:
  - explicit loader, dependency, isolation, and reload planes
  - bounded failure behavior around trust and runtime activation
  - dedicated base-layer composition for extensible modules

## Module Dependencies

### Direct Upstream Dependencies (this module uses)
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| utils | `include/utils/` | Logging, audit, and thread helpers consumed during load/reload lifecycle |

### Direct Downstream Consumers (modules that use this module)
| Module | Via | Notes |
|--------|-----|-------|
| server | `include/base/module_loader.h` | Server uses base to dynamically load/unload extension modules at runtime |
| plugins | `include/base/` (ModuleLoader) | Plugin manager delegates module sandboxing and reload to the base layer |

## Integration Points

### Critical Integration: Module Load / Unload Lifecycle
**Files:** `src/base/module_loader.cpp`, `src/base/hot_reload_manager.cpp` ↔ `server/` and `plugins/`
**Contract:** Callers (server, plugins) invoke load/unload/reload through the ModuleLoader API; base module owns trust, integrity, and sandbox decisions exclusively.
**Thread Safety:** Load/unload operations are serialised under an internal lifecycle mutex; hot-reload acquires the same lock for atomic swap.

### Critical Integration: Wasm Sandbox Isolation
**Files:** `src/base/wasm_plugin_sandbox.cpp`, `src/base/wasm_runtime_injector.cpp`
**Contract:** Wasm plugins execute inside isolated sandbox instances; host callbacks injected via `WasmRuntimeInjector` must be side-effect-free.
**Thread Safety:** Each sandbox instance is single-threaded; host callback invocations must not re-enter the sandbox.