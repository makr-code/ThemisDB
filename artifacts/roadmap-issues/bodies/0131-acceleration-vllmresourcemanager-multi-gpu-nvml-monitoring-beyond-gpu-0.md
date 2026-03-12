### Context

This issue implements the roadmap item 'VLLMResourceManager: Multi-GPU NVML Monitoring (Beyond GPU 0)' for the acceleration domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: VLLMResourceManager: Multi-GPU NVML Monitoring (Beyond GPU 0)

### Goal

Deliver the scoped changes for VLLMResourceManager: Multi-GPU NVML Monitoring (Beyond GPU 0) in src/acceleration/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### VLLMResourceManager: Multi-GPU NVML Monitoring (Beyond GPU 0)
**Priority:** Medium
**Target Version:** v1.8.0

`VLLMResourceManager::initializeNVML()` in `vllm_resource_manager.cpp:178` hard-codes `nvmlDeviceGetHandleByIndex(0, &device)` — it always monitors only the first GPU. In a multi-GPU co-location scenario (4× A100), ThemisDB may be routed to GPU 2 or GPU 3 by the scheduler, but `canUseGPU()` will report GPU 0's utilization, causing incorrect GPU-busy decisions.

**Implementation Notes:**
- `[ ]` Extend `VLLMResourceManager::Config` with a `gpu_device_index` field (default `0`); pass it to `nvmlDeviceGetHandleByIndex(config_.gpu_device_index, &device)` in `initializeNVML()`.
- `[ ]` Alternatively, store a `std::vector<nvmlDevice_t>` for all devices from `0` to `total_gpu_count - 1`; return the maximum utilization across all monitored devices from `queryGPUUtilization()` so that a single busy GPU blocks ThemisDB from scheduling new work on any device.
- `[ ]` Add a `gpu_device_indices` override field to `Config` to allow explicit device pinning (e.g., `{2, 3}` for a 4-GPU node where GPUs 0 and 1 are reserved for vLLM).
- `[ ]` Update `shutdownNVML()` to call `nvmlShutdown()` only after all device handles have been released.
- `[ ]` Test: in a CI environment with a mock NVML shim, verify that `canUseGPU()` returns `false` when the configured device is at 90% utilization but GPU 0 is idle.

---

### Acceptance Criteria

- [ ] Extend `VLLMResourceManager::Config` with a `gpu_device_index` field (default `0`); pass it to `nvmlDeviceGetHandleByIndex(config_.gpu_device_index, &device)` in `initializeNVML()`.
- [ ] Alternatively, store a `std::vector<nvmlDevice_t>` for all devices from `0` to `total_gpu_count - 1`; return the maximum utilization across all monitored devices from `queryGPUUtilization()` so that a single busy GPU blocks ThemisDB from scheduling new work on any device.
- [ ] Add a `gpu_device_indices` override field to `Config` to allow explicit device pinning (e.g., `{2, 3}` for a 4-GPU node where GPUs 0 and 1 are reserved for vLLM).
- [ ] Update `shutdownNVML()` to call `nvmlShutdown()` only after all device handles have been released.
- [ ] Test: in a CI environment with a mock NVML shim, verify that `canUseGPU()` returns `false` when the configured device is at 90% utilization but GPU 0 is idle.

### Relationships

- Roadmap row: #131 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/acceleration/FUTURE_ENHANCEMENTS.md#vllmresourcemanager-multi-gpu-nvml-monitoring-beyond-gpu-0
- Source key: roadmap:131:acceleration:v1.8.0:vllmresourcemanager-multi-gpu-nvml-monitoring-beyond-gpu-0

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:131:acceleration:v1.8.0:vllmresourcemanager-multi-gpu-nvml-monitoring-beyond-gpu-0 -->
<!-- roadmap-ref: row=131;module=acceleration;target=v1.8.0 -->
<!-- roadmap-detail: src/acceleration/FUTURE_ENHANCEMENTS.md#vllmresourcemanager-multi-gpu-nvml-monitoring-beyond-gpu-0 -->
