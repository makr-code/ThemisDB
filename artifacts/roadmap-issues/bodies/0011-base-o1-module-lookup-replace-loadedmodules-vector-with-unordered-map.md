### Context

This issue implements the roadmap item 'O(1) Module Lookup — Replace `loadedModules_` Vector with Unordered Map' for the base domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Immediate (≤ v1.4.0) and targets milestone v1.2.0.

Primary detail section: O(1) Module Lookup — Replace `loadedModules_` Vector with Unordered Map

### Goal

Deliver the scoped changes for O(1) Module Lookup — Replace `loadedModules_` Vector with Unordered Map in src/base/ and complete the linked detail section in a release-ready state for v1.2.0.

### Detailed Scope

### O(1) Module Lookup — Replace `loadedModules_` Vector with Unordered Map
**Priority:** High
**Target Version:** v1.2.0

`loadedModules_` in `module_loader.cpp` is a `std::vector<ModuleInfo>`. Every lookup (`isLoaded`, `getModule`, `unload`, `watchdogLoop`) calls `std::find_if` over the entire list — O(n) per operation. With dozens of loaded plugins this is measurable overhead on every query dispatch.

**Implementation Notes:**
- `[ ]` Replace `loadedModules_` (`std::vector`) with `std::unordered_map<std::string, ModuleInfo>` keyed by module name in `module_loader.cpp`.
- `[ ]` Introduce a `shared_mutex` so `getModule` / `isLoaded` (read-only) use `shared_lock` and `load` / `unload` use `unique_lock`, reducing read contention.
- `[ ]` The watchdog loop at line 1752 notes "loadedModules_ has no dedicated mutex in the existing design" — fix this by making the watchdog hold a `shared_lock` when iterating.
- `[ ]` Update `ModuleLoader` unit tests to exercise concurrent `load`/`getModule`/`unload` with TSAN enabled.

**Performance Targets:**
- `getModule(name)` lookup: O(1) average, ≤ 1 µs under contention from 8 concurrent reader threads.

---

### Acceptance Criteria

- [ ] Replace `loadedModules_` (`std::vector`) with `std::unordered_map<std::string, ModuleInfo>` keyed by module name in `module_loader.cpp`.
- [ ] Introduce a `shared_mutex` so `getModule` / `isLoaded` (read-only) use `shared_lock` and `load` / `unload` use `unique_lock`, reducing read contention.
- [ ] The watchdog loop at line 1752 notes "loadedModules_ has no dedicated mutex in the existing design" — fix this by making the watchdog hold a `shared_lock` when iterating.
- [ ] Update `ModuleLoader` unit tests to exercise concurrent `load`/`getModule`/`unload` with TSAN enabled.
- [ ] `getModule(name)` lookup: O(1) average, ≤ 1 µs under contention from 8 concurrent reader threads.

### Relationships

- Roadmap row: #11 (🟠 High Priority — Immediate (≤ v1.4.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/base/FUTURE_ENHANCEMENTS.md#o1-module-lookup--replace-loadedmodules_-vector-with-unordered-map
- Source key: roadmap:11:base:v1.2.0:o1-module-lookup-replace-loadedmodules-vector-with-unordered-map

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:11:base:v1.2.0:o1-module-lookup-replace-loadedmodules-vector-with-unordered-map -->
<!-- roadmap-ref: row=11;module=base;target=v1.2.0 -->
<!-- roadmap-detail: src/base/FUTURE_ENHANCEMENTS.md#o1-module-lookup--replace-loadedmodules_-vector-with-unordered-map -->
