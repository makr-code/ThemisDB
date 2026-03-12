### Context

This issue implements the roadmap item 'BackendRegistry: O(n²) Backend Selection Index' for the acceleration domain. It is sourced from the consolidated roadmap under 🟢 Low Priority — Future (v1.9.0+) and targets milestone v1.9.0.

Primary detail section: BackendRegistry: O(n²) Backend Selection Index

### Goal

Deliver the scoped changes for BackendRegistry: O(n²) Backend Selection Index in src/acceleration/ and complete the linked detail section in a release-ready state for v1.9.0.

### Detailed Scope

### BackendRegistry: O(n²) Backend Selection Index
**Priority:** Low
**Target Version:** v1.9.0

The `selectTyped<T>()` helper in `backend_registry.cpp:223–233` iterates the entire `kFallbackOrder` vector (13 entries) and for each entry scans all registered backends in `backends_`. In the current implementation with ~15 backends this is negligible, but it is called for every query that needs backend selection (`selectVectorBackendFor`, `selectGraphBackendFor`, `selectGeoBackendFor`, `selectMatrixBackendFor`, `getBestVectorBackend`, etc.). More importantly, the nested loop requires O(|kFallbackOrder| × |backends_|) `dynamic_cast` calls per selection.

**Implementation Notes:**
- `[ ]` At the end of `initializeRuntime()`, build a `std::unordered_map<BackendType, IComputeBackend*>` index from `backends_`; replace the nested loop in `selectTyped<T>()` with a single map lookup per priority level.
- `[ ]` Pre-compute and cache `getBestVectorBackend()` / `getBestGraphBackend()` / `getBestGeoBackend()` results into `selectedVectorBackend_` etc. as is already partially done; ensure `getBackend(type)` also uses the map.
- `[ ]` Avoid `dynamic_cast` in the hot path: store typed pointers (`IVectorBackend*`, `IGraphBackend*`, `IGeoBackend*`) alongside the `IComputeBackend*` in a `RegisteredBackend` struct at `registerBackend()` time (one `dynamic_cast` per registration, not per query).

---

### Acceptance Criteria

- [ ] At the end of `initializeRuntime()`, build a `std::unordered_map<BackendType, IComputeBackend*>` index from `backends_`; replace the nested loop in `selectTyped<T>()` with a single map lookup per priority level.
- [ ] Pre-compute and cache `getBestVectorBackend()` / `getBestGraphBackend()` / `getBestGeoBackend()` results into `selectedVectorBackend_` etc. as is already partially done; ensure `getBackend(type)` also uses the map.
- [ ] Avoid `dynamic_cast` in the hot path: store typed pointers (`IVectorBackend*`, `IGraphBackend*`, `IGeoBackend*`) alongside the `IComputeBackend*` in a `RegisteredBackend` struct at `registerBackend()` time (one `dynamic_cast` per registration, not per query).

### Relationships

- Roadmap row: #236 (🟢 Low Priority — Future (v1.9.0+))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/acceleration/FUTURE_ENHANCEMENTS.md#backendregistry-on%C2%B2-backend-selection-index
- Source key: roadmap:236:acceleration:v1.9.0:backendregistry-on-c2-b2-backend-selection-index

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:236:acceleration:v1.9.0:backendregistry-on-c2-b2-backend-selection-index -->
<!-- roadmap-ref: row=236;module=acceleration;target=v1.9.0 -->
<!-- roadmap-detail: src/acceleration/FUTURE_ENHANCEMENTS.md#backendregistry-on%C2%B2-backend-selection-index -->
