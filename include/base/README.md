> **Build:** `cmake --preset release && cmake --build build/release`

# Base Module — Public Headers

**Module Path:** `include/base/`
**Implementation:** `../../src/base/`

## Purpose

Public interfaces and declarations for ThemisDB's foundational runtime infrastructure, including module loading, sandboxing, hot reload, dependency handling, and plugin lifecycle management.

## Canonical Module Documentation

`include/base/` contains public header contracts. Canonical module behavior, architecture, and operations docs live in `src/base/`:

- [`../../src/base/README.md`](../../src/base/README.md)
- [`../../src/base/ARCHITECTURE.md`](../../src/base/ARCHITECTURE.md)
- [`../../src/base/ROADMAP.md`](../../src/base/ROADMAP.md)
- [`../../src/base/FUTURE_ENHANCEMENTS.md`](../../src/base/FUTURE_ENHANCEMENTS.md)

## Header Files

| Header | Primary Class / Interface |
|--------|--------------------------|
| `module_loader.h` | `ModuleLoader` — secure module loading and lifecycle |
| `module_sandbox.h` | `ModuleSandbox` — runtime isolation and resource bounds |
| `wasm_plugin_sandbox.h` | `WasmPluginSandbox` — WebAssembly plugin isolation |
| `wasm_runtime_injector.h` | `WasmRuntimeInjector` — WASM runtime registration |
| `hot_reload_manager.h` | `HotReloadManager` — module reload and rollback orchestration |
| `plugin_dependency_graph.h` | `PluginDependencyGraph` — dependency resolution and ordering |
| `remote_registry_client.h` | `RemoteRegistryClient` — remote plugin retrieval |
| `ab_test_manager.h` | `ABTestManager` — module-level traffic split and experiments |

## Usage

```cpp
#include "base/module_loader.h"

auto loader = themis::base::createModuleLoader({
    .trust_verify = true,
    .sandbox_enabled = true
});

auto module = loader->load("path/to/module.so");
module->initialize();
```

For full runtime usage examples (loading, sandboxing, hot reload), see [`../../src/base/README.md`](../../src/base/README.md).

## Key Configuration Surface

Important configuration entry points are declared in:

- `module_loader.h` (`ModuleLoader::Config` for trust and lifecycle)
- `module_sandbox.h` (`ModuleSandbox::Config` for resource limits)
- `wasm_plugin_sandbox.h` (`WasmPluginSandbox::Config` for WASM isolation)
- `hot_reload_manager.h` (reload strategy and rollback configuration)

## Build

```cmake
cmake --preset release && cmake --build build/release --target themis-base
```

## See Also

- [`../../src/base/README.md`](../../src/base/README.md) — implementation details
- [`../../src/plugins/README.md`](../../src/plugins/README.md) — plugin system

## Installation

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
