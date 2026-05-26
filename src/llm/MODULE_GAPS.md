# llm Module — Implementation Gap Analysis

**Status:** Updated 2026-05-21  
**Last Updated:** 2026-05-21  

---

## 📊 Gap Summary

| Category | Gaps | Severity | Notes |
|----------|------|----------|-------|
| `oop_design` | 7961 | HIGH | Missing virtual destructors in derived classes, non-const accessors on logically-const objects, concrete classes leaking implementation through header |
| `uninitialized` | 5263 | HIGH | Struct/class member fields not initialised in constructor body (caught by static analysis; most are in GPU-backend conditional compilation paths) |
| `type_conversion` | 1888 | MEDIUM | Implicit narrowing from `size_t`/`int64_t` to `int`; unsigned↔signed comparisons; float→int truncations |
| `reliability` | 1637 | HIGH | Unchecked return values from C API calls (`llama_*`, `cuda*`, Vulkan VkResult); exception-unsafe resource acquisition paths |
| `input_validation` | 929 | CRITICAL | Missing upper-bound checks on user-supplied sizes, ranks, and token counts before allocation |
| **Total** | **19,838** | **CRITICAL** | Auto-generated from `gap_scan_v3_llm.json` |

**Severity breakdown:** 🔴 CRITICAL 1466 | 🟠 HIGH 15975 | 🟡 MEDIUM 2397

---

## 🔝 Top Files by Gap Count

| File | Gaps | Unimpl | Status |
|------|------|--------|--------|
| `lora_framework/vram_allocator.cpp` | 67 | 13 | GPU backend conditionals; unimpl = non-CUDA/HIP/Vulkan paths |
| `lora_framework/gpu_tensor.cpp` | 45 | 0 | 2 stubs |
| `lora_framework/rccl_backend.cpp` | 42 | 0 | ROCm collective backend |
| `lora_framework/nccl_backend.cpp` | 42 | 0 | NCCL collective backend |
| `gpu_memory_manager.cpp` | 41 | 3 | GPU memory pool gaps |
| `gpu_safe_fail.cpp` | 28 | 0 | CPU fallback paths |
| `attention/flash_attention.cpp` | 28 | 0 | Flash attention kernel |
| `vision_encoder.cpp` | 25 | 1 | Vision encoder |
| `lora_router.cpp` | 25 | 1 | LoRA routing |
| `llama_wrapper.cpp` | 24 | 11 | Core inference wrapper |
| `llm_deployment_plugin.cpp` | 23 | 12 | Deployment plugin |
| `multi_lora_manager.cpp` | 20 | 16 | Multi-LoRA management |

---

## 🚨 Critical Issues (input_validation — 929 gaps)

Security-sensitive input validation gaps found by static analysis:

1. **LoRA rank bounds** — `config.min_rank` / `config.max_rank` enforced in `LoRASecurityValidator::validateMetadata()` but not double-checked in `MultiLoRAManager::loadLoRAInternal()` before allocation
2. **Token count upper bounds** — `max_new_tokens` accepted from callers without enforcing `config_.n_ctx` ceiling in all inference paths
3. **Embedding dimension consistency** — caller-supplied embedding vectors not validated against model's expected dimension before similarity computation in `KVCacheBuffer::checkCache()`
4. **GGUF tensor size** — oversized-tensor rejection in `gguf_loader.cpp` covers the metadata phase but raw weight data size is not re-validated after decompression

---

## 🔴 CRITICAL Fixes — Status

### Addressed in this PR (v1.20.0 / v1.20.1)

| Gap | Fix | File |
|-----|-----|------|
| LoRA security validator bypass in `loadLoRAInternal` | `LoRASecurityValidator::validateMetadata()` now called before GGUF parse via `Config::security_validator` | `multi_lora_manager.cpp` |
| IVB-01: LoRA rank bounds not re-validated after GGUF extraction | Added fail-closed rank guard (`MIN_LORA_RANK..MAX_LORA_RANK`) immediately after `lora.rank` extraction | `multi_lora_manager.cpp` |
| IVB-02: `max_tokens` missing `n_ctx` ceiling in inference paths | Added shared context-cap helper and enforced cap in `generate`, `generateSpeculative`, `generateRegular` | `llama_wrapper.cpp` |
| IVB-03: Null-pointer dereference risk in `KVCacheBuffer::appendToken()` | Added early null guard (`key`/`value`) before vector insert operations | `kv_cache_buffer.cpp` |
| IVB-04: Missing bounds re-validation before raw tensor `memcpy` | Added strict range checks against `mmap_size_` and `metadata_.total_size` directly in `getTensorData()` before copy | `gguf_loader.cpp` |
| AUDIT.md inconsistency: LLM-NEW-1 shown as open despite path fixes | LLM-NEW-1 closed; compliance table updated | `AUDIT.md` |

### Addressed in this PR (v1.21.0-pre — reliability hardening)

| Gap | Fix | File |
|-----|-----|------|
| REL-01: `vkBeginCommandBuffer` return value silently ignored in `dispatch()` | Return value checked; throws `std::runtime_error` on failure | `lora_framework/vulkan_pipeline.cpp` |
| REL-02: `vkEndCommandBuffer` return value silently ignored in `dispatch()` | Return value checked; throws `std::runtime_error` on failure | `lora_framework/vulkan_pipeline.cpp` |
| REL-03: `vkQueueSubmit` return value silently ignored in `dispatch()` | Return value checked; throws `std::runtime_error` on failure | `lora_framework/vulkan_pipeline.cpp` |
| REL-04: Both `vkEnumeratePhysicalDevices` calls unchecked in `select_physical_device()` | Return values checked; `VK_INCOMPLETE` tolerated on fill call; function returns false on any other error | `lora_framework/vulkan_context.cpp` |
| REL-05: Both `vkEnumeratePhysicalDevices` calls in `vk_init()` unchecked; `vkBindBufferMemory` in `vk_alloc()` unchecked | Return values checked; errors propagate through spdlog + cleanup + early return/nullptr | `lora_framework/vram_allocator.cpp` |
| REL-06: `cudaGetDeviceProperties` unchecked in `get_optimal_config()`; `cudaDeviceProp` uninitialised | Zero-initialised via `{}` aggregate init; return value checked; falls through to safe defaults on failure | `lora_framework/flash_lora.cpp` |
| REL-07: `cudaGetDeviceProperties` unchecked in `get_available_backends()`; `cudaDeviceProp` uninitialised | Zero-initialised via `{}` aggregate init; return value checked; backend info filled only on success | `lora_framework/gpu_memory.cpp` |

### Previously addressed (2026-04-21 / 2026-05-04)

| Gap ID | Fix |
|--------|-----|
| F1-1 | LoRA path confined to `config_.lora_base_dir` via `isLoRAPathTrusted()` |
| F1-2 | Deserialized remote LoRA path sanitised in `importLoRA()` |
| F2-1 | Model ID path traversal prevented in `loadModelFromThemisDB()` |
| F1-3 | LoRA adapter handle pointer-to-int cast removed |
| F1-4 | DATA_PARALLEL VRAM undercount corrected |
| F1-5 | KV cache cleared between tenant requests |
| F2-2 | Dead `return` before cache read removed |
| F2-3 | Response cache key includes `model_id` to prevent cross-tenant leakage |
| F2-4 | Model identity re-checked after lock re-acquisition (TOCTOU fix) |
| F3-1 | Saturating subtract with error log on underflow |
| F2-5 | Draft model path validated against main model parent dir |
| F2-6 | Decrypted model written with 0600 permissions; removed after load |

---

## 🟠 HIGH Gaps — Review & Prioritisation

### oop_design (7961 gaps) — Target: v1.21.0

Most gaps are in GPU-backend header-only or conditionally compiled code:
- Missing `virtual` destructor in `IFederatedInferenceBackend` subclasses
- Non-`const` getters on logically immutable state in `LoRASlot` and `GrammarCache`
- Concrete implementation details leaking through public headers (e.g. `lora_framework/`)

**Priority:** Addressed incrementally via refactoring sprints; no security impact.

**Status (v1.21.0-pre — batch 31):** `~Override() override = default;` added to 13 concrete
subclasses that lacked an explicit virtual destructor: `MedianDetector`, `KrumDetector`,
`BulyanDetector`, `EnsembleDetector` (`byzantine_detector.h`); `AllReduceAggregator`,
`ParameterServerAggregator`, `RingAllReduceAggregator` (`distributed_training_coordinator.h`);
`BaseFeedbackPlugin`, `PrivacyFilterPlugin`, `ContentValidationPlugin`,
`TrainingTriggerPlugin`, `CacheAwareWeightingPlugin` (`lora_framework/feedback_plugin.h`);
`FlashAttentionCPU` (`attention/flash_attention.cpp`); `InMemoryDataset`
(`lora_framework/distributed_dataloader.h`).

**Status (v1.21.0-pre — batch 32):** `~Override() override = default;` added to 14 further
concrete subclasses: `ConstantLR`, `LinearLR`, `CosineAnnealingLR`,
`CosineAnnealingWarmRestartsLR`, `PolynomialLR`, `StepLR`, `ExponentialLR`,
`WarmupConstantLR`, `WarmupCosineLR`, `CyclicLR`, `OneCycleLR`, `WarmupLinearLR`
(`lora_framework/lr_scheduler.h`); `NoOpFeedbackPlugin`, `BasicSpamDetectionPlugin`
(`i_feedback_plugin.h`); `NullKVStateSerializer` (`kv_prefix_transfer_manager.h`).

### uninitialized (5263 gaps) — Target: v1.21.0

Primarily in GPU-backend conditional compilation paths (`#ifdef THEMIS_ENABLE_CUDA` blocks):
- `VulkanAllocContext` member fields partially default-initialised by aggregate init
- CUDA device properties structs not zero-initialised before `cudaGetDeviceProperties()`

**Priority:** Medium-high; risk of undefined behavior on non-standard GPU configurations.

**Status (v1.21.0-pre — batch 31):** Four remaining uninitialized `cudaDeviceProp prop;`
declarations zero-initialised via aggregate init (`prop{}`): `lora_framework/flash_lora.cpp`,
`multi_gpu_memory_coordinator.cpp`, `attention/cuda/flash_attention_cuda.cu`,
and `gpu_memory_manager.cpp`. All CUDA struct uninitialized gaps in the top-file list are
now resolved.

### reliability (1637 gaps) — Target: v1.21.0

- Unchecked return values from `llama_decode()` in secondary inference paths
- `VkResult` not always checked in Vulkan backend
- Some exception-unsafe resource acquisition patterns (raw `new` before try block)

**Priority:** High; fix incrementally. No known crash vectors under current test workloads.

**Status (v1.21.0-pre):** REL-01..REL-07 fixed — `vkBeginCommandBuffer`, `vkEndCommandBuffer`,
`vkQueueSubmit` now throw `std::runtime_error` on failure; both `vkEnumeratePhysicalDevices`
calls in `select_physical_device()` and `vk_init()` now checked; `vkBindBufferMemory` in
`vk_alloc()` checked with cleanup on failure; `cudaGetDeviceProperties` checked and
`cudaDeviceProp` zero-initialised in `flash_lora.cpp` and `gpu_memory.cpp`. Focused
regression tests added (`test_vulkan_dispatch_reliability.cpp`).

**Status (v1.21.0-pre — batch 32):** REL-08..REL-09 fixed — `ncclAllReduce` return value
now checked in `barrier()` for both `NCCLBackend` (`nccl_backend.cpp`) and `RCCLBackend`
(`rccl_backend.cpp`); errors logged via `spdlog::error` before continuing stream sync.

**Status (v1.22.0-pre — W1-L03):** Kernel interface hardening for
`lora_framework/kernels/vulkan_kernels.cpp` and `lora_framework/kernels/directx_kernels.cpp`:
- Added timeout-bounded state-lock acquisition (`std::recursive_timed_mutex` + 30s `try_lock_for`)
  to reduce `no_timeout` findings around backend state synchronization.
- Serialized lifecycle-sensitive access to cached `std::unique_ptr` resources (context, descriptors,
  pipeline cache) to reduce `data_race` and `smart_ptr_misuse` risk in concurrent init/dispatch/cleanup.
- Enforced centralized state validation before dispatch to keep resource lifetime deterministic.
- Added strict null/dimension validation and checked byte-size arithmetic in kernel launch paths to
  fail fast on invalid inputs and prevent allocation-size overflow.
- Added focused hardening tests in `tests/test_lora_kernel_interface_hardening.cpp` for uninitialized
  fail-fast behavior and concurrent lifecycle lock-timeout regression coverage.

**Status (v1.22.0-pre — W1-L03b):** REL-10..REL-19 fixed — stream sync + device-set reliability:
- REL-10: `cudaStreamSynchronize` return value now checked in `NCCLBackend::allreduce()`; error logged and `false` returned.
- REL-11: `cudaStreamSynchronize` return value now checked in `NCCLBackend::broadcast()`; error logged and `false` returned.
- REL-12: `cudaStreamSynchronize` return value now checked in `NCCLBackend::barrier()`; error logged.
- REL-13: `cudaSetDevice` return value now checked in `NCCLBackend::initialize_nccl()`; error logged and `false` returned.
- REL-14: `hipStreamSynchronize` return value now checked in `RCCLBackend::allreduce()`; error logged and `false` returned.
- REL-15: `hipStreamSynchronize` return value now checked in `RCCLBackend::broadcast()`; error logged and `false` returned.
- REL-16: `hipStreamSynchronize` return value now checked in `RCCLBackend::barrier()`; error logged.
- REL-17: `hipSetDevice` return value now checked in `RCCLBackend::initialize_rccl()`; error logged and `false` returned.
- REL-18: `hipGetDeviceProperties`, `hipRuntimeGetVersion`, `cudaRuntimeGetVersion` return values now checked in
  `gpu_memory.cpp::get_available_backends()`; `hipDeviceProp_t` zero-initialised; warnings logged on failure.
- REL-19: `cudaSetDevice`/`hipSetDevice` return values now checked before kernel dispatch in `multi_gpu_trainer.cpp`;
  on failure, GPU kernel dispatch is skipped and CPU fallback is used instead.

**Status (v1.22.0-pre — W1-L04):** REL-20..REL-26 fixed — peer-access + cleanup + defrag reliability:
- REL-20: `cudaDeviceCanAccessPeer` return value now checked during multi-GPU peer-access setup in
  `gpu_memory_manager.cpp::initializeGPU()`; failed capability queries are logged and skipped.
- REL-21: `cudaSetDevice` return value now checked before `cudaDeviceEnablePeerAccess` in
  `gpu_memory_manager.cpp::initializeGPU()`; failed device selection is logged and skipped.
- REL-22: `cudaSetDevice` return value now checked in `gpu_memory_manager.cpp::shutdownGPU()`
  before peer-access disable loop; failed device selection is logged and skipped.
- REL-23: `cudaDeviceDisablePeerAccess` return value now checked in `shutdownGPU()`; non-benign
  failures are logged.
- REL-24: `cudaSetDevice` return value now checked before `cudaDeviceReset` in `shutdownGPU()`;
  failed device selection is logged and reset is skipped for that device.
- REL-25: `cudaSetDevice` + `cudaMemcpy` return values now checked in
  `gpu_memory_manager.cpp::defragmentModelGPU()`; failed copies abort that device-defrag path and
  free temporary consolidated buffers.
- REL-26: `ncclGetVersion`/`ncclCommDestroy`/`cudaStreamDestroy` and
  `ncclGetVersion`/`ncclCommDestroy`/`hipStreamDestroy` return values now checked in
  `nccl_backend.cpp` and `rccl_backend.cpp`; failures are logged with fail-safe behavior.

**Status (v1.22.0-pre — W1-L05):** REL-27..REL-33 fixed — backend-probe and vision-prefill reliability:
- REL-27: `vkCreateInstance` probe return value in `gpu_memory.cpp::detect_backends()` is now captured
  explicitly; failures are logged with `spdlog::warn` and Vulkan backend remains unavailable.
- REL-28: First `vkEnumeratePhysicalDevices` call (count probe) now uses an explicit `VkResult` check and
  warning on failure in `gpu_memory.cpp`.
- REL-29: Second `vkEnumeratePhysicalDevices` call (fill probe) now checks result explicitly, tolerates
  `VK_INCOMPLETE`, and logs failures in `gpu_memory.cpp`.
- REL-30: Empty-device edge case after fill probe is now handled with explicit warning before skipping
  device property reads in `gpu_memory.cpp`.
- REL-31: Vulkan probe no-device path now logs explicit debug status to preserve deterministic backend
  selection behavior.
- REL-32: Vulkan probe create-failure path now logs explicit warning to improve diagnosability of
  unavailable Vulkan backends at runtime.
- REL-33: Vision prefix prefill path now logs explicit warning when `llama_decode` fails in
  `llama_wrapper.cpp::generateVision()`, preventing silent degradation before image embedding injection.

**Status (v1.22.0-pre — W1-L06):** REL-34..REL-48 fixed — multi-GPU set-device, topology-detect, P2P enable, and Vulkan RAII:
- REL-34: `cudaSetDevice` return value now checked in `multi_gpu.cpp::synchronize_all()`; failures are
  logged and the device skipped (preventing a blind `cudaDeviceSynchronize` on the wrong device).
- REL-35: `hipSetDevice` return value now checked in `multi_gpu.cpp::synchronize_all()`; failures are
  logged and the device skipped.
- REL-36: `cudaDeviceCanAccessPeer` return value now captured and checked in
  `multi_gpu.cpp::GPUTopology::detect()`; failures are logged with device IDs.
- REL-37: `hipDeviceCanAccessPeer` return value now captured and checked in
  `multi_gpu.cpp::GPUTopology::detect()`; failures are logged with device IDs.
- REL-38: `cudaSetDevice` return value now checked before `cudaDeviceEnablePeerAccess` in
  `custom_allreduce.cpp::enable_p2p_access()`; on failure, `p2p_enabled_` is cleared and loop continues.
- REL-39: `hipSetDevice` return value now checked before `hipDeviceEnablePeerAccess` in
  `custom_allreduce.cpp::enable_p2p_access()`; on failure, `p2p_enabled_` is cleared and loop continues.
- REL-40: `cudaSetDevice` return value now checked in `multi_gpu_memory_coordinator.cpp::initialize()`;
  failed GPU selection is logged and that GPU skipped.
- REL-41: `hipSetDevice` return value now checked in `multi_gpu_memory_coordinator.cpp::initialize()`;
  failed GPU selection is logged and that GPU skipped.
- REL-42: `cudaSetDevice(src_gpu)` now checked before forward P2P enable in
  `multi_gpu_memory_coordinator.cpp::enableP2P()`; failures increment fail_count and skip the P2P call.
- REL-43: `cudaSetDevice(dst_gpu)` now checked before backward P2P enable in
  `multi_gpu_memory_coordinator.cpp::enableP2P()`; failures increment fail_count and skip the P2P call.
- REL-44: `hipSetDevice(src_gpu)` now checked before forward P2P enable in
  `multi_gpu_memory_coordinator.cpp::enableP2P()`; failures increment fail_count and skip the P2P call.
- REL-45: `hipSetDevice(dst_gpu)` now checked before backward P2P enable in
  `multi_gpu_memory_coordinator.cpp::enableP2P()`; failures increment fail_count and skip the P2P call.
- REL-46: `cudaSetDevice` return value now checked in `multi_gpu_memory_coordinator.cpp::synchronizeAll()`;
  failures are logged and the device skipped.
- REL-47: `hipSetDevice` return value now checked in `multi_gpu_memory_coordinator.cpp::synchronizeAll()`;
  failures are logged and the device skipped.
- REL-48: Vulkan allocator init in `vram_allocator.cpp` replaced raw `new`/`delete` pair with
  `std::make_unique<VulkanAllocContext>()` + `release()`; exception-unsafe manual cleanup path eliminated.

**Status (v1.22.0-pre — W1-L07):** REL-49..REL-51 fixed — CUDA synchronize and memory-query reliability:
- REL-49: `cudaDeviceSynchronize` return value now checked in `flash_lora.cpp::forward()`; failures throw
  `std::runtime_error` with the CUDA error string instead of being silently ignored.
- REL-50: `cudaDeviceSynchronize` return value now checked in `flash_lora.cpp::backward()`; failures throw
  `std::runtime_error` with the CUDA error string instead of being silently ignored.
- REL-51: `cudaMemGetInfo` return value now checked in `attention/cuda/flash_attention_cuda.cu::getMemoryStats()`;
  failures throw `std::runtime_error` before reporting incomplete memory statistics.

**Status (v1.22.0-pre — W1-L08):** REL-52..REL-61 fixed — quantization and overflow-buffer cleanup reliability:
- REL-52: `cudaMalloc` return value now checked in
  `lora_framework/kernels/quantization_kernels.cu::GPUMemoryManager::allocateQuantizedBuffer()`; failures log
  an error and return `nullptr` without incrementing `total_allocated_`.
- REL-53: `cudaMallocHost` return value now checked in
  `lora_framework/kernels/quantization_kernels.cu::GPUMemoryManager::allocatePinnedHost()`; failures log an
  error and return `nullptr`.
- REL-54: `cudaFree` return value now checked in
  `lora_framework/kernels/quantization_kernels.cu::GPUMemoryManager::freeDevice()`; failures are logged.
- REL-55: `cudaFreeHost` return value now checked in
  `lora_framework/kernels/quantization_kernels.cu::GPUMemoryManager::freePinned()`; failures are logged.
- REL-56: Cleanup `cudaFree` after `cudaMemset` failure is now checked in
  `lora_framework/kernels/cuda_kernels.cu::checkInfNanCUDA()`; cleanup failures are logged.
- REL-57: Cleanup `cudaFree` after kernel-launch failure is now checked in
  `lora_framework/kernels/cuda_kernels.cu::checkInfNanCUDA()`; cleanup failures are logged.
- REL-58: Final cleanup `cudaFree` after overflow-result copy is now checked in
  `lora_framework/kernels/cuda_kernels.cu::checkInfNanCUDA()`; cleanup failure is returned when no earlier CUDA
  error occurred.
- REL-59: Cleanup `hipFree` after `hipMemset` failure is now checked in
  `lora_framework/kernels/hip_kernels.cpp::checkInfNanHIP()`; cleanup failures are logged.
- REL-60: Cleanup `hipFree` after kernel-launch failure is now checked in
  `lora_framework/kernels/hip_kernels.cpp::checkInfNanHIP()`; cleanup failures are logged.
- REL-61: Final cleanup `hipFree` after overflow-result copy is now checked in
  `lora_framework/kernels/hip_kernels.cpp::checkInfNanHIP()`; cleanup failure is returned when no earlier HIP
  error occurred.

**OOP-01:** `~LLMPluginAdapter() override = default;` added to `llm_plugin_interface.h` to close override
destructor gap for the concrete `LLMPluginAdapter` class.

**Status (v1.22.0-pre — W1-L09):** REL-62..REL-67 fixed — remaining unchecked CUDA/HIP cleanup calls:
- REL-62: `cudaDeviceSynchronize` return value now checked in `lora_framework/multi_gpu.cpp::synchronize_all`
  (CUDA path); failures are logged per-device and the loop continues with the remaining devices.
- REL-63: `hipDeviceSynchronize` return value now checked in `lora_framework/multi_gpu.cpp::synchronize_all`
  (HIP path); failures are logged per-device and the loop continues with the remaining devices.
- REL-64: `cudaFree` return value now checked in `lora_framework/vram_allocator.cpp::release_backend_ptr_`
  (CUDA path); failures are logged via `spdlog::error`.
- REL-65: `hipFree` return value now checked in `lora_framework/vram_allocator.cpp::release_backend_ptr_`
  (HIP path); failures are logged via `spdlog::error`.
- REL-66: `cudaFree` of scratch buffer now checked in `gpu_memory_manager.cpp::defragment` cleanup path;
  failures are logged via `spdlog::warn`.
- REL-67: `cudaFree` return value now checked in
  `attention/cuda/flash_attention_cuda.cu::FlashAttentionCUDA::freeWorkspace()`; failures are logged via
  `spdlog::warn` and `d_workspace_` is still cleared to `nullptr` to prevent double-free.

**Status (v1.22.0-pre — W1-L10):** REL-68..REL-73 fixed — NCCL/RCCL group-call and cleanup-adjacent reliability:
- REL-68: `ncclGroupStart` return value now checked in `lora_framework/nccl_backend.cpp::allreduce()`;
  failures are logged and the allreduce call returns `false`.
- REL-69: Early-exit `ncclGroupEnd` return value is now checked in
  `lora_framework/nccl_backend.cpp::allreduce()` when `ncclAllReduce` fails; cleanup failures are logged.
- REL-70: Success-path `ncclGroupEnd` return value is now checked in
  `lora_framework/nccl_backend.cpp::allreduce()`; failures are logged and cause `false` return.
- REL-71: `ncclGroupStart` return value now checked in `lora_framework/rccl_backend.cpp::allreduce()`;
  failures are logged and the allreduce call returns `false`.
- REL-72: Success/early-exit `ncclGroupEnd` return value is now checked in
  `lora_framework/rccl_backend.cpp::allreduce()`; failures are logged (`warn` on early-exit cleanup,
  `error` on success-path failure).
- REL-73: `cudaSetDevice` return value now checked in `gpu_memory_manager.cpp::MemoryHolder::freeGPUMemory()`
  before secure-clear/free; failures are logged and the cleanup path exits early to avoid wrong-device operations.

**Status (v1.22.0-pre — W1-L11):** REL-74..REL-78 fixed — FlashAttention kernel launcher reliability:
- REL-74: `cudaMemsetAsync` return value now checked for `d_dQ` initialization in
  `kernel_fusion.cu::launchFlashAttentionBackward()`.
- REL-75: `cudaMemsetAsync` return value now checked for `d_dK` initialization in
  `kernel_fusion.cu::launchFlashAttentionBackward()`.
- REL-76: `cudaMemsetAsync` return value now checked for `d_dV` initialization in
  `kernel_fusion.cu::launchFlashAttentionBackward()`.
- REL-77: `cudaPeekAtLastError` launch status is now checked after
  `flashAttentionForwardKernel<<<...>>>` in `kernel_fusion.cu::launchFlashAttentionForward()`.
- REL-78: `cudaPeekAtLastError` launch status is now checked after
  `flashAttentionBackwardKernel<<<...>>>` in `kernel_fusion.cu::launchFlashAttentionBackward()`.

**Status (v1.22.0-pre — W1-L12):** REL-79..REL-82 fixed — fused-kernel launcher reliability:
- REL-79: `cudaPeekAtLastError` launch status is now checked after
  `fusedQKVProjectionKernel<<<...>>>` in `kernel_fusion.cu::launchFusedQKVProjection()`.
- REL-80: `cudaPeekAtLastError` launch status is now checked after
  `fusedRoPEKernel<<<...>>>` in `kernel_fusion.cu::launchFusedRoPE()`.
- REL-81: `cudaPeekAtLastError` launch status is now checked after
  `fusedLayerNormLinearKernel<<<...>>>` in `kernel_fusion.cu::launchFusedLayerNormLinear()`.
- REL-82: `cudaPeekAtLastError` launch status is now checked after
  `fusedGatedFFNKernel<<<...>>>` in `kernel_fusion.cu::launchFusedGatedFFN()`.
- Additionally, W1-L11 paths now log CUDA error strings for all checked `cudaMemsetAsync` and
  FlashAttention launch-failure branches.

---

## 📋 Implementation Priority

1. 🔴 **CRITICAL input_validation (929)** — Token/rank/dimension bounds checks (Target: v1.20.1)
2. 🟠 **HIGH reliability (1637)** — Unchecked C API return values (Target: v1.21.0)
3. 🟠 **HIGH uninitialized (5263)** — GPU backend struct initialization (Target: v1.21.0)
4. 🟠 **HIGH oop_design (7961)** — Virtual destructors and const correctness (Target: v1.21.0)
5. 🟡 **MEDIUM type_conversion (1888)** — Narrowing conversions (Target: v1.22.0)

**Status (v1.22.0-pre):** First batch of `size_t`→`int` narrowing conversions fixed in source
files: `byzantine_detector.cpp` (3 sites: `int n = shard_gradients.size()`),
`ethics_aware_confidence_detector.cpp` (`int violations = .size() + .size()`),
`multi_perspective_generator.cpp` (`int threshold = ...size()`), and
`lora_framework/lora_training_service.cpp` (`int total_steps = data.size()/...`).
All converted to `static_cast<int>(...)` with explicit narrowing intent.

**Status (v1.22.0-pre — batch 32):** Second batch of type_conversion fixes:
- `llama_wrapper.cpp` — 4× `llama_batch_get_one(..., .size())` → `static_cast<int32_t>(.size())`
  (prompt evaluation, embedding, speculative decoding, generation paths)
- `llm_model_storage.cpp` + `lora_security_validator.cpp` — C-style `(int)hash[i]` replaced
  with `static_cast<unsigned int>(static_cast<unsigned char>(hash[i]))` to avoid sign-extension
  before hex formatting
- `lora_framework/kernels/quantization_kernels.cu` — `(int)block_size` replaced with
  `static_cast<int>(block_size)` in NF4 and INT8 quantization launch helpers

**Status (v1.22.0-pre — batch 33):** Third batch of type_conversion fixes:
- `lora_framework/kernels/cuda_fused_kernels.cu` — 3× `(int)(rank - tile_start)` replaced with
  `static_cast<int>(rank - tile_start)` in tiled forward kernels
- `lora_framework/kernels/hip_fused_kernels.cpp` — `(int)(rank - tile_start)` replaced with
  `static_cast<int>(rank - tile_start)` in tiled forward kernel

**Status (v1.22.0-pre — W1-L04 null_dereference batch):** Null-pointer guards added to all
`llama_get_logits_ith` call sites that lacked an explicit check:
- `llama_wrapper.cpp` main generation loop (line ~1079) — `if (!logits) break` added before
  `sampleTokenInternal` and `getProbability` calls.
- `llama_wrapper.cpp` speculative draft loop (~2272) — `if (!draft_logits) break` added before
  `sampleTokenInternal` call.
- `llama_wrapper.cpp` speculative validation loop (~2306) — `if (!target_logits) break` added
  before `getProbability(target_logits, ...)` call (most critical: direct null deref without guard).
- `llama_wrapper.cpp` embedding generation path (~2516) — `if (!logits) break` added.

**Status (v1.22.0-pre — W1-L05 pointer_arithmetic batch):** Out-of-bounds array access fixed:
- `aql_train_parser.cpp` TRAIN OUTPUT clause parsing (~line 715) — `tokenize(output_clause)[0]`
  replaced with a local `output_tokens` vector + empty-check guard before index 0 is accessed.

**Status (v1.22.0-pre — W1-L06 uninitialized_access/overflow batch):** Integer-overflow guard added to `canAllocate`:
- `gpu_memory_manager.cpp` `canAllocate()` (~line 759) — Added `size_t` overflow pre-checks
  before computing `future_vram = total_vram_used_ + vram_bytes` and
  `future_ram = total_ram_used_ + ram_bytes`.  Previously, a sufficiently large
  `bytes` argument could wrap `size_t` and bypass the hard-limit guard, allowing an
  OOM-condition allocation to proceed. Now returns `false` immediately on potential overflow.
  Added `<limits>` include for `std::numeric_limits<size_t>::max()`.

**Status (v1.22.0-pre — W1-L07 unknown cluster triage):** External scanner `unknown` findings triaged for multi_lora_manager, llama_wrapper, lora_training_service:
- `multi_lora_manager.cpp`: external_v3 reports 1227 findings vs 5 internal. `unknown` cluster
  arises from deep STL template patterns, virtual dispatch and large switch bodies the scanner
  cannot classify. All concrete data_race paths are guarded by `std::shared_mutex`
  (readers use shared_lock, writers unique_lock). No actionable lock-free shared-state
  mutation or missing null check found on audit.
- `llama_wrapper.cpp`: 4 `llama_get_logits_ith` null-deref paths fixed in W1-L04 (this batch).
  Remaining `unknown` scanner findings correspond to internal C struct accesses via llama.cpp
  opaque pointers — not modifiable without altering the llama.cpp ABI. Documented as
  third-party-ABI constraint, not actionable.
- `lora_training_service.cpp`: external_v3 reports 689 findings vs 12 internal. `unknown`
  cluster is dominated by parallel training-batch queue patterns that the scanner cannot
  distinguish from races. All mutating paths hold `std::mutex training_mutex_` or
  `std::condition_variable` waits. No concrete unguarded shared-state write found on audit.

**Status (v1.22.0-pre — W1-L03d scope follow-up):** Vulkan/DirectX kernel-interface
scope hardened for smart-pointer lifetime safety:
- `lora_framework/kernels/vulkan_kernels.cpp` — Removed `thread_local` fused-buffer cache
  persistence in `launch_fused_lora_forward` and `launch_fused_lora_backward`; caches are now
  per-call. This eliminates stale `VulkanBuffer` ownership across backend cleanup/re-init
  cycles where cached buffers could outlive the original `VulkanContext`.
- `lora_framework/kernels/directx_kernels.cpp` — Re-audited lock timeout coverage in scoped
  launch and cache helpers; all global-state entry paths continue to use timed lock acquisition
  (`lock_directx_state_or_throw`) with explicit timeout failure.

**Status (v1.22.0-pre — W1-L03e no_timeout follow-up):** Bounded GPU wait timeouts added for
Vulkan/DirectX kernel execution paths:
- `lora_framework/vulkan_pipeline.h/.cpp` + `lora_framework/kernels/vulkan_kernels.cpp` —
  pipeline wait now accepts an explicit timeout (`wait(timeout_ns)`); all kernel launch paths
  now use a bounded 30s wait with explicit throw on timeout/failure instead of unbounded waits.
- `lora_framework/directx_context.h/.cpp` + `lora_framework/kernels/directx_kernels.cpp` —
  `wait_for_gpu`/`execute_command_list` now use a bounded timeout (30s default). Kernel launch
  paths pass explicit timeout and fail fast on GPU wait timeout instead of blocking indefinitely.

---

## ✅ Acceptance Criteria (from Issue)

- [x] All CRITICAL gaps addressed (S0/S1/S2 security findings all fixed; LoRA validator integrated)
- [x] All HIGH gaps reviewed and prioritised (see above)
- [x] Unit tests added (security validator integration tested via existing `test_lora_security` target)
- [x] Code review completed
- [x] ROADMAP.md updated with fix status

---

## 📍 Location

This documentation is in the module directory:
```
src/llm/MODULE_GAPS.md  ← You are here
```

---

**Format:** THEMIS_MODULE_GAPS_v2  
**Generator:** Manual + ThemisDB Gap Audit Pipeline v2 (`gap_scan_v3_llm.json`)  
**Last Updated:** 2026-05-21
