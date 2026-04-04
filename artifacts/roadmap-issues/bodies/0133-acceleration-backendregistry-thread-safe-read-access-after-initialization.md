### Context

This issue implements the roadmap item 'BackendRegistry: Thread-Safe Read Access After Initialization' for the acceleration domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: BackendRegistry: Thread-Safe Read Access After Initialization

### Goal

Deliver the scoped changes for BackendRegistry: Thread-Safe Read Access After Initialization in src/acceleration/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### BackendRegistry: Thread-Safe Read Access After Initialization
**Priority:** Medium
**Target Version:** v1.8.0

`BackendRegistry` members `backends_`, `selectedVectorBackend_`, `selectedGraphBackend_`, `selectedGeoBackend_`, and `runtimeInitialized_` in `compute_backend.h:579–587` are plain (non-atomic) pointers and containers with no mutex protection. `initializeRuntime()` writes all of them without holding any lock; `getBestVectorBackend()`, `selectVectorBackendFor()`, and `getSelectedVectorBackend()` read them without a lock. Concurrent calls to `autoDetect()` (which writes `backends_` via `registerBackend()`) and `getBestVectorBackend()` (which iterates `backends_`) are a data race.

**Implementation Notes:**
- `[ ]` Add a `mutable std::shared_mutex registryMutex_` to `BackendRegistry` (declared in `compute_backend.h`); hold an exclusive lock in `registerBackend()`, `shutdownAll()`, and `initializeRuntime()`; hold a shared lock in all `getBackend*()`, `selectBackendFor()`, and `getBestBackend*()` methods.
- `[ ]` Protect `selectedVectorBackend_`, `selectedGraphBackend_`, `selectedGeoBackend_` writes in `initializeRuntime()` and clears in `shutdownAll()` with the exclusive lock.
- `[ ]` Protect `runtimeInitialized_` reads/writes with the shared/exclusive lock; or convert it to `std::atomic<bool>` for a lighter-weight check.
- `[ ]` The `selectTyped<T>()` template function at `backend_registry.cpp:223–233` takes `backends_` by const-ref; callers must hold the shared lock before calling it — document this in a comment.
- `[ ]` Add a thread-safety test (`tests/acceleration/test_backend_registry_thread_safety.cpp`) that spawns 16 threads calling `getBestVectorBackend()` concurrently while a background thread calls `autoDetect()` and verifies no crashes under TSan.

---

### Acceptance Criteria

- [ ] Add a `mutable std::shared_mutex registryMutex_` to `BackendRegistry` (declared in `compute_backend.h`); hold an exclusive lock in `registerBackend()`, `shutdownAll()`, and `initializeRuntime()`; hold a shared lock in all `getBackend*()`, `selectBackendFor()`, and `getBestBackend*()` methods.
- [ ] Protect `selectedVectorBackend_`, `selectedGraphBackend_`, `selectedGeoBackend_` writes in `initializeRuntime()` and clears in `shutdownAll()` with the exclusive lock.
- [ ] Protect `runtimeInitialized_` reads/writes with the shared/exclusive lock; or convert it to `std::atomic<bool>` for a lighter-weight check.
- [ ] The `selectTyped<T>()` template function at `backend_registry.cpp:223–233` takes `backends_` by const-ref; callers must hold the shared lock before calling it — document this in a comment.
- [ ] Add a thread-safety test (`tests/acceleration/test_backend_registry_thread_safety.cpp`) that spawns 16 threads calling `getBestVectorBackend()` concurrently while a background thread calls `autoDetect()` and verifies no crashes under TSan.

### Relationships

- Roadmap row: #133 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/acceleration/FUTURE_ENHANCEMENTS.md#backendregistry-thread-safe-read-access-after-initialization
- Source key: roadmap:133:acceleration:v1.8.0:backendregistry-thread-safe-read-access-after-initialization

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:133:acceleration:v1.8.0:backendregistry-thread-safe-read-access-after-initialization -->
<!-- roadmap-ref: row=133;module=acceleration;target=v1.8.0 -->
<!-- roadmap-detail: src/acceleration/FUTURE_ENHANCEMENTS.md#backendregistry-thread-safe-read-access-after-initialization -->
