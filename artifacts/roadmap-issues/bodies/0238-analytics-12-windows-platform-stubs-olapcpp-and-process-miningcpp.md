### Context

This issue implements the roadmap item 'Windows Platform Stubs — `olap.cpp` and `process_mining.cpp`' for the analytics domain. It is sourced from the consolidated roadmap under 🟢 Low Priority — Future (v1.9.0+) and targets milestone v2.0.0.

Primary detail section: 12 · Windows Platform Stubs — `olap.cpp` and `process_mining.cpp`

### Goal

Deliver the scoped changes for Windows Platform Stubs — `olap.cpp` and `process_mining.cpp` in src/analytics/ and complete the linked detail section in a release-ready state for v2.0.0.

### Detailed Scope

### 12 · Windows Platform Stubs — `olap.cpp` and `process_mining.cpp`
**Priority:** Medium
**Target Version:** v2.0.0
**Files:** `src/analytics/olap.cpp` lines 53–100; `src/analytics/process_mining.cpp` lines 24–end

`olap.cpp` (lines 53–100) compiles an entire no-op `OLAPEngine` on `_WIN32`, with every
public method emitting `spdlog::error(…not supported on Windows…)` and returning a default
value.  `process_mining.cpp` (lines 24–end) similarly returns
`Status::Error("Process mining is not supported on Windows builds")` from every method when
`THEMIS_PROCESS_MINING_WINDOWS_STUB` is defined.  Separately, the Arrow-absent stubs for
`exportToParquet()` and `exportCollectionToParquet()` (lines ~1755+) silently return `false`
without any log message or exception.

**Implementation Notes:**
- `[ ]` Audit `OLAPEngine` for Windows-specific blockers (likely POSIX `mmap`, `pread`, or specific SIMD intrinsics); use `#ifdef _WIN32` guards only around the affected primitives rather than replacing the entire class
- `[ ]` Add CMake CI job for Windows (MSVC 2022 + vcpkg) that builds and runs the OLAP unit tests to prevent silent regressions
- `[ ]` `exportToParquet()` / `exportCollectionToParquet()` silent `return false` (lines ~1755–1780) must at minimum emit `spdlog::warn("exportToParquet: Arrow not compiled in – rebuild with -DTHEMIS_HAS_ARROW=ON")` so operators are not silently losing export operations
- `[ ]` `ProcessMining` Windows stub should propagate the `Status::Error` through to the caller's log at `spdlog::error` level rather than silently returning — operators need visibility when a capability is absent
- `[ ]` Track Windows-stub coverage in the file-header `Stubs:` counter and add a CI check that fails if the stub count is > 0 on non-Windows builds

**Performance Targets:**
- Full `OLAPEngine::execute()` on Windows: feature-parity with Linux for non-SIMD code paths

---

### Acceptance Criteria

- [ ] Audit `OLAPEngine` for Windows-specific blockers (likely POSIX `mmap`, `pread`, or specific SIMD intrinsics); use `#ifdef _WIN32` guards only around the affected primitives rather than replacing the entire class
- [ ] Add CMake CI job for Windows (MSVC 2022 + vcpkg) that builds and runs the OLAP unit tests to prevent silent regressions
- [ ] `exportToParquet()` / `exportCollectionToParquet()` silent `return false` (lines ~1755–1780) must at minimum emit `spdlog::warn("exportToParquet: Arrow not compiled in – rebuild with -DTHEMIS_HAS_ARROW=ON")` so operators are not silently losing export operations
- [ ] `ProcessMining` Windows stub should propagate the `Status::Error` through to the caller's log at `spdlog::error` level rather than silently returning — operators need visibility when a capability is absent
- [ ] Track Windows-stub coverage in the file-header `Stubs:` counter and add a CI check that fails if the stub count is > 0 on non-Windows builds
- [ ] Full `OLAPEngine::execute()` on Windows: feature-parity with Linux for non-SIMD code paths

### Relationships

- Roadmap row: #238 (🟢 Low Priority — Future (v1.9.0+))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/analytics/FUTURE_ENHANCEMENTS.md#12--windows-platform-stubs--olapcpp-and-process_miningcpp
- Source key: roadmap:238:analytics:v2.0.0:12-windows-platform-stubs-olapcpp-and-process-miningcpp

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:238:analytics:v2.0.0:12-windows-platform-stubs-olapcpp-and-process-miningcpp -->
<!-- roadmap-ref: row=238;module=analytics;target=v2.0.0 -->
<!-- roadmap-detail: src/analytics/FUTURE_ENHANCEMENTS.md#12--windows-platform-stubs--olapcpp-and-process_miningcpp -->
