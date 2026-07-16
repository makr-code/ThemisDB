### Context

This issue implements the roadmap item 'BackendRegistry: Replace `std::cout` with Structured Logger' for the acceleration domain. It is sourced from the consolidated roadmap under 🟢 Low Priority — Future (v1.9.0+) and targets milestone v1.8.0.

Primary detail section: BackendRegistry: Replace `std::cout` with Structured Logger

### Goal

Deliver the scoped changes for BackendRegistry: Replace `std::cout` with Structured Logger in src/acceleration/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### BackendRegistry: Replace `std::cout` with Structured Logger
**Priority:** Low
**Target Version:** v1.8.0

`backend_registry.cpp` uses `std::cout` for all diagnostic output (lines 136, 143, 167, 311, 335, 340, 359, 417, 438, 442) despite the codebase providing a structured logger via `utils/logger.h` (`THEMIS_INFO`, `THEMIS_WARN`, `THEMIS_ERROR`, `THEMIS_DEBUG` macros). The inconsistency means backend-selection events are invisible when the calling application redirects or suppresses `std::cout`, and they cannot be structured-logged (JSON, syslog) by the logging framework.

**Implementation Notes:**
- `[ ]` Replace all `std::cout << "Registered backend: ..."` (line 136) with `THEMIS_INFO("Registered backend: {} (type={})", backend->name(), static_cast<int>(backend->type()))`.
- `[ ]` Replace all `std::cout <<` calls in `autoDetect()`, `initializeRuntime()`, `shutdownAll()`, `loadPlugins()`, `loadPlugin()` with the appropriate severity-level macro (`THEMIS_INFO` for status, `THEMIS_WARN` for degraded paths, `THEMIS_DEBUG` for verbose capability dumps).
- `[ ]` The `logSelection` lambda in `initializeRuntime()` (line 435) already uses `std::cout`; convert it to `THEMIS_INFO` / `THEMIS_WARN`.
- `[ ]` Ensure `utils/logger.h` is already included in `backend_registry.cpp` (it is used for `THEMIS_ERROR` on line 180 but `#include "utils/logger.h"` is already present).

---

### Acceptance Criteria

- [ ] Replace all `std::cout << "Registered backend: ..."` (line 136) with `THEMIS_INFO("Registered backend: {} (type={})", backend->name(), static_cast<int>(backend->type()))`.
- [ ] Replace all `std::cout <<` calls in `autoDetect()`, `initializeRuntime()`, `shutdownAll()`, `loadPlugins()`, `loadPlugin()` with the appropriate severity-level macro (`THEMIS_INFO` for status, `THEMIS_WARN` for degraded paths, `THEMIS_DEBUG` for verbose capability dumps).
- [ ] The `logSelection` lambda in `initializeRuntime()` (line 435) already uses `std::cout`; convert it to `THEMIS_INFO` / `THEMIS_WARN`.
- [ ] Ensure `utils/logger.h` is already included in `backend_registry.cpp` (it is used for `THEMIS_ERROR` on line 180 but `#include "utils/logger.h"` is already present).

### Relationships

- Roadmap row: #237 (🟢 Low Priority — Future (v1.9.0+))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/acceleration/FUTURE_ENHANCEMENTS.md#backendregistry-replace-stdcout-with-structured-logger
- Source key: roadmap:237:acceleration:v1.8.0:backendregistry-replace-stdcout-with-structured-logger

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:237:acceleration:v1.8.0:backendregistry-replace-stdcout-with-structured-logger -->
<!-- roadmap-ref: row=237;module=acceleration;target=v1.8.0 -->
<!-- roadmap-detail: src/acceleration/FUTURE_ENHANCEMENTS.md#backendregistry-replace-stdcout-with-structured-logger -->
