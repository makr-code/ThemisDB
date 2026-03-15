### Context

This issue implements the roadmap item '`PluginRegistry`: Upgrade Global Mutex to `shared_mutex`' for the plugins domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: `PluginRegistry`: Upgrade Global Mutex to `shared_mutex`

### Goal

Deliver the scoped changes for `PluginRegistry`: Upgrade Global Mutex to `shared_mutex` in src/plugins/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### `PluginRegistry`: Upgrade Global Mutex to `shared_mutex`
**Priority:** Medium
**Target Version:** v1.8.0

`plugin_registry.cpp` uses a single static `std::mutex` (line 50) for all registry operations. All read operations (`getPlugin`, `listPlugins`, `isRegistered`) hold an exclusive lock, serializing concurrent reads from multiple modules.

**Implementation Notes:**
- `[ ]` Replace the static `std::mutex` in `PluginRegistry::getMutex()` with a `std::shared_mutex`.
- `[ ]` Upgrade `getPlugin`, `listPlugins`, `isRegistered` to `std::shared_lock`.
- `[ ]` Keep `registerPlugin`, `unregisterPlugin`, `clear` on `std::unique_lock`.

---


**Priority:** High
**Target Version:** v0.9.0

Replace the current dlopen-based loading with a WASM runtime (Wasmtime or WasmEdge) to provide memory-safe, OS-independent plugin execution. Each plugin runs in its own WASM linear memory; host functions are explicitly allowlisted via a capabilities manifest field.

**Implementation Notes:**
- Add `wasm_plugin_loader.cpp` alongside `plugin_manager.cpp`; select loader via `PluginManifest.runtime` field (`"native"` | `"wasm"`).
- Expose a `WasmHostAPI` header under `include/plugins/` that maps the existing `IPlugin` vtable to WASM import functions.
- Update `plugin_system_edition.cpp` to gate WASM support behind the Enterprise edition flag.
- `plugin_registry.cpp` must verify WASM module hash against manifest `sha256` field before instantiation.

**Performance Targets:**
- WASM plugin cold-start latency: <50 ms per plugin on warm JIT cache.
- Steady-state call overhead vs. native plugin: <3× (Wasmtime near-native tier).

---

### Acceptance Criteria

- [x] Replace the static `std::mutex` in `PluginRegistry::getMutex()` with a `std::shared_mutex`.
- [x] Upgrade `getPlugin`, `listPlugins`, `isRegistered` to `std::shared_lock`.
- [x] Keep `registerPlugin`, `unregisterPlugin`, `clear` on `std::unique_lock`.
- [x] Add `wasm_plugin_loader.cpp` alongside `plugin_manager.cpp`; select loader via `PluginManifest.runtime` field (`"native"` | `"wasm"`).
- [x] Expose a `WasmHostAPI` header under `include/plugins/` that maps the existing `IPlugin` vtable to WASM import functions.
- [x] Update `plugin_system_edition.cpp` to gate WASM support behind the Enterprise edition flag.
- [x] `plugin_registry.cpp` must verify WASM module hash against manifest `sha256` field before instantiation.
- [ ] WASM plugin cold-start latency: <50 ms per plugin on warm JIT cache.
- [ ] Steady-state call overhead vs. native plugin: <3× (Wasmtime near-native tier).

### Relationships

- Roadmap row: #193 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/plugins/FUTURE_ENHANCEMENTS.md#pluginregistry-upgrade-global-mutex-to-shared_mutex
- Source key: roadmap:193:plugins:v1.8.0:pluginregistry-upgrade-global-mutex-to-shared-mutex

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:193:plugins:v1.8.0:pluginregistry-upgrade-global-mutex-to-shared-mutex -->
<!-- roadmap-ref: row=193;module=plugins;target=v1.8.0 -->
<!-- roadmap-detail: src/plugins/FUTURE_ENHANCEMENTS.md#pluginregistry-upgrade-global-mutex-to-shared_mutex -->
