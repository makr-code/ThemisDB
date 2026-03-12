### Context

This issue implements the roadmap item 'VLLMResourceManager: OS-Level CPU and RAM Monitoring' for the acceleration domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: VLLMResourceManager: OS-Level CPU and RAM Monitoring

### Goal

Deliver the scoped changes for VLLMResourceManager: OS-Level CPU and RAM Monitoring in src/acceleration/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### VLLMResourceManager: OS-Level CPU and RAM Monitoring
**Priority:** Medium
**Target Version:** v1.8.0

`VLLMResourceManager::getStats()` in `vllm_resource_manager.cpp:134–140` returns `cpu_utilization = 0.0` and `ram_used_mb = 0` unconditionally. Both fields contain inline comments: *"Note: Implement OS-specific CPU monitoring for accurate metrics"* and *"Note: Implement OS-specific memory monitoring for accurate metrics"*. As a result, the `Stats` struct exposed to callers always reports zero CPU and RAM usage, making adaptive throttling and co-location scheduling decisions based on `Stats` unreliable.

**Implementation Notes:**
- `[ ]` Linux CPU monitoring in `VLLMResourceManager::getStats()`: read `/proc/stat` on two successive snapshots (e.g., 100 ms apart) and compute `(total - idle) / total * 100.0`; cache the most recent snapshot to avoid double-reads in rapid successive calls.
- `[ ]` Linux RAM monitoring: parse `/proc/meminfo` fields `MemTotal`, `MemAvailable`; compute `ram_used_mb = (MemTotal - MemAvailable) / 1024`. This is a single read with O(lines) cost and can be done inline.
- `[ ]` Windows CPU monitoring: call `GetSystemTimes()` and delta `IdleTime` / (`KernelTime + UserTime + IdleTime`); cache snapshot for 200 ms.
- `[ ]` Windows RAM monitoring: call `GlobalMemoryStatusEx()` and read `dwMemoryLoad` and `ullTotalPhys - ullAvailPhys`.
- `[ ]` Gate both implementations behind `#ifdef __linux__` / `#ifdef _WIN32` guards; leave `0.0` as the macOS/unknown fallback rather than crashing.
- `[ ]` Add test `tests/acceleration/test_vllm_resource_stats.cpp` asserting `cpu_utilization >= 0.0 && cpu_utilization <= 100.0` and `ram_used_mb > 0` on a live system.

**Performance Targets:**
- Each `getStats()` call must complete in < 2 ms (single `/proc/stat` + `/proc/meminfo` read on Linux).
- CPU snapshot cache TTL 200 ms to balance freshness versus syscall overhead.

---

### Acceptance Criteria

- [ ] Linux CPU monitoring in `VLLMResourceManager::getStats()`: read `/proc/stat` on two successive snapshots (e.g., 100 ms apart) and compute `(total - idle) / total * 100.0`; cache the most recent snapshot to avoid double-reads in rapid successive calls.
- [ ] Linux RAM monitoring: parse `/proc/meminfo` fields `MemTotal`, `MemAvailable`; compute `ram_used_mb = (MemTotal - MemAvailable) / 1024`. This is a single read with O(lines) cost and can be done inline.
- [ ] Windows CPU monitoring: call `GetSystemTimes()` and delta `IdleTime` / (`KernelTime + UserTime + IdleTime`); cache snapshot for 200 ms.
- [ ] Windows RAM monitoring: call `GlobalMemoryStatusEx()` and read `dwMemoryLoad` and `ullTotalPhys - ullAvailPhys`.
- [ ] Gate both implementations behind `#ifdef __linux__` / `#ifdef _WIN32` guards; leave `0.0` as the macOS/unknown fallback rather than crashing.
- [ ] Add test `tests/acceleration/test_vllm_resource_stats.cpp` asserting `cpu_utilization >= 0.0 && cpu_utilization <= 100.0` and `ram_used_mb > 0` on a live system.
- [ ] Each `getStats()` call must complete in < 2 ms (single `/proc/stat` + `/proc/meminfo` read on Linux).
- [ ] CPU snapshot cache TTL 200 ms to balance freshness versus syscall overhead.

### Relationships

- Roadmap row: #130 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/acceleration/FUTURE_ENHANCEMENTS.md#vllmresourcemanager-os-level-cpu-and-ram-monitoring
- Source key: roadmap:130:acceleration:v1.8.0:vllmresourcemanager-os-level-cpu-and-ram-monitoring

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:130:acceleration:v1.8.0:vllmresourcemanager-os-level-cpu-and-ram-monitoring -->
<!-- roadmap-ref: row=130;module=acceleration;target=v1.8.0 -->
<!-- roadmap-detail: src/acceleration/FUTURE_ENHANCEMENTS.md#vllmresourcemanager-os-level-cpu-and-ram-monitoring -->
