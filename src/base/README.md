# ThemisDB Base Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The base module provides foundational runtime infrastructure for dynamic module loading, sandboxing, hot reload, dependency handling, and plugin lifecycle support used by higher-level ThemisDB modules.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| module_loader.cpp | secure module loading, trust checks, lifecycle control |
| module_sandbox.cpp | runtime isolation and resource-bounded execution |
| wasm_plugin_sandbox.cpp | wasm plugin isolation and invocation surfaces |
| wasm_runtime_injector.cpp | wasm runtime registration and selection glue |
| hot_reload_manager.cpp | reload and rollback orchestration |
| plugin_dependency_graph.cpp | dependency graph and ordering utilities |
| remote_registry_client.cpp | remote plugin retrieval and validation support |
| ab_test_manager.cpp | module-level traffic split and experiment switching |

## Scope

In scope:
- secure and bounded module runtime infrastructure
- module loading lifecycle and reload behavior
- plugin dependency and registry integration support
- sandbox and wasm execution boundaries

Out of scope:
- module-specific business logic implemented by plugin payloads
- non-base domain APIs outside module runtime foundations
- feature-level policy decisions owned by higher modules

## Runtime Behavior and Limits

- behavior is capability-dependent on enabled build/runtime options.
- sandbox and wasm paths remain bounded by configured limits.
- loader and registry paths fail with structured error states on validation failure.

## Sourcecode Verification (Module: base/readme)

- Verified files:
  - src/base/module_loader.cpp
  - src/base/module_sandbox.cpp
  - src/base/wasm_plugin_sandbox.cpp
  - src/base/wasm_runtime_injector.cpp
  - src/base/hot_reload_manager.cpp
  - src/base/plugin_dependency_graph.cpp
  - src/base/remote_registry_client.cpp
  - src/base/ab_test_manager.cpp
- Verified behavior surfaces:
  - module lifecycle and loader trust surfaces
  - sandbox and wasm runtime boundary paths
  - dependency, registry, and hot-reload infrastructure
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md