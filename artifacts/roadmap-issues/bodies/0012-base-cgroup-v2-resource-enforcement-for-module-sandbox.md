### Context

This issue implements the roadmap item 'cgroup v2 Resource Enforcement for Module Sandbox' for the base domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Immediate (≤ v1.4.0) and targets milestone v1.2.0.

Primary detail section: cgroup v2 Resource Enforcement for Module Sandbox

### Goal

Deliver the scoped changes for cgroup v2 Resource Enforcement for Module Sandbox in src/base/ and complete the linked detail section in a release-ready state for v1.2.0.

### Detailed Scope

### cgroup v2 Resource Enforcement for Module Sandbox
**Priority:** High
**Target Version:** v1.2.0

`module_sandbox.cpp` uses `setrlimit(RLIMIT_AS)` and `setrlimit(RLIMIT_CPU)` as a "coarse fallback" (lines 372, 416–417). The source comments explicitly note that real production deployments need cgroup v2. The cgroup path is allocated in `platform_->cgroup_path` (line 238) but cleanup is commented out with "On a real production system, we'd also remove the cgroup" (line 330).

**Implementation Notes:**
- `[ ]` Implement `setupCgroupV2()` in `module_sandbox.cpp`: write `memory.max` and `cpu.max` to `/sys/fs/cgroup/themis/<sandbox_id>/` using the pre-allocated `cgroup_path`.
- `[ ]` Implement `teardownCgroupV2()` to remove the cgroup directory on `stop()` — replace the "would also remove the cgroup" placeholder comment.
- `[ ]` Detect cgroup v2 availability at startup; fall back to `RLIMIT_*` with a `spdlog::warn` when unavailable (container environments without cgroup delegation).
- `[ ]` Add integration test that launches a sandbox plugin allocating > limit bytes and verifies it is killed within 500 ms.

**Performance Targets:**
- Sandbox creation (cgroup v2 setup): ≤ 50 ms per plugin.

---

### Acceptance Criteria

- [ ] Implement `setupCgroupV2()` in `module_sandbox.cpp`: write `memory.max` and `cpu.max` to `/sys/fs/cgroup/themis/<sandbox_id>/` using the pre-allocated `cgroup_path`.
- [ ] Implement `teardownCgroupV2()` to remove the cgroup directory on `stop()` — replace the "would also remove the cgroup" placeholder comment.
- [ ] Detect cgroup v2 availability at startup; fall back to `RLIMIT_*` with a `spdlog::warn` when unavailable (container environments without cgroup delegation).
- [ ] Add integration test that launches a sandbox plugin allocating > limit bytes and verifies it is killed within 500 ms.
- [ ] Sandbox creation (cgroup v2 setup): ≤ 50 ms per plugin.

### Relationships

- Roadmap row: #12 (🟠 High Priority — Immediate (≤ v1.4.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/base/FUTURE_ENHANCEMENTS.md#cgroup-v2-resource-enforcement-for-module-sandbox
- Source key: roadmap:12:base:v1.2.0:cgroup-v2-resource-enforcement-for-module-sandbox

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:12:base:v1.2.0:cgroup-v2-resource-enforcement-for-module-sandbox -->
<!-- roadmap-ref: row=12;module=base;target=v1.2.0 -->
<!-- roadmap-detail: src/base/FUTURE_ENHANCEMENTS.md#cgroup-v2-resource-enforcement-for-module-sandbox -->
