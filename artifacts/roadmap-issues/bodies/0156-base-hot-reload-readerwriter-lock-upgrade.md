### Context

This issue implements the roadmap item 'Hot-Reload Reader/Writer Lock Upgrade' for the base domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.3.0.

Primary detail section: Hot-Reload Reader/Writer Lock Upgrade

### Goal

Deliver the scoped changes for Hot-Reload Reader/Writer Lock Upgrade in src/base/ and complete the linked detail section in a release-ready state for v1.3.0.

### Detailed Scope

### Hot-Reload Reader/Writer Lock Upgrade
**Priority:** Medium
**Target Version:** v1.3.0

`hot_reload_manager.cpp` uses a single `std::mutex` for all operations (lines 55–495). All `getVersion()`, `isLoaded()`, and status queries (read-only operations) contend with `reloadModule()` (write operation), limiting read throughput under concurrent query load.

**Implementation Notes:**
- `[ ]` Replace `std::mutex mutex_` with `std::shared_mutex` in `HotReloadManager`; upgrade `getVersion`, `getCurrentVersion`, `isLoaded`, `getModuleNames` to `std::shared_lock`.
- `[ ]` Keep `reloadModule` and `rollback` on `std::unique_lock`.
- `[ ]` Add TSAN-enabled test with 16 reader threads + 1 reload thread running concurrently.

---

### Acceptance Criteria

- [ ] Replace `std::mutex mutex_` with `std::shared_mutex` in `HotReloadManager`; upgrade `getVersion`, `getCurrentVersion`, `isLoaded`, `getModuleNames` to `std::shared_lock`.
- [ ] Keep `reloadModule` and `rollback` on `std::unique_lock`.
- [ ] Add TSAN-enabled test with 16 reader threads + 1 reload thread running concurrently.

### Relationships

- Roadmap row: #156 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/base/FUTURE_ENHANCEMENTS.md#hot-reload-readerwriter-lock-upgrade
- Source key: roadmap:156:base:v1.3.0:hot-reload-readerwriter-lock-upgrade

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:156:base:v1.3.0:hot-reload-readerwriter-lock-upgrade -->
<!-- roadmap-ref: row=156;module=base;target=v1.3.0 -->
<!-- roadmap-detail: src/base/FUTURE_ENHANCEMENTS.md#hot-reload-readerwriter-lock-upgrade -->
