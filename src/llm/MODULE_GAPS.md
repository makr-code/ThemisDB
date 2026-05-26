# llm Module — Implementation Gap Analysis

**Status:** Updated 2026-05-26 (W1-L06 delta applied)
**Last Updated:** 2026-05-26  

---

## 📊 Gap Summary

| Category | Gaps | Severity | Notes |
|----------|------|----------|-------|
| `oop_design` | ~~7961~~ **7960** | HIGH | OOP-01 fixed (W1-L04): `LLMPluginAdapter` missing override dtor; remaining: concrete class leaks, non-const accessors |
| `uninitialized` | 5263 | HIGH | Struct/class member fields not initialised in constructor body (caught by static analysis; most are in GPU-backend conditional compilation paths) |
| `type_conversion` | 1888 | MEDIUM | Implicit narrowing from `size_t`/`int64_t` to `int`; unsigned↔signed comparisons; float→int truncations |
| `reliability` | ~~1637~~ **1598** | HIGH | 17 unchecked GPU API calls fixed in W1-L04 + 7 secondary-path checks in W1-L05 + 15 multi-GPU reliability fixes in W1-L06; remaining: Vulkan VkResult, llama_* |
| `input_validation` | 929 | CRITICAL | Missing upper-bound checks on user-supplied sizes, ranks, and token counts before allocation |
| `data_race` | ~~7~~ **0** | CRITICAL | ✅ **Resolved in W1-L02 + W1-L03** — See delta tables below |
| `iterator_invalidation` | ~~1~~ **0** | HIGH | ✅ **Resolved in W1-L02** — `loss_history_` push_back race |
| `no_timeout` | ~~2~~ **0** | HIGH | ✅ **Resolved in W1-L02 + W1-L03** — stopTraining polling; GPU fence INFINITE waits |
| `smart_ptr_misuse` | ~~1~~ **0** | HIGH | ✅ **Resolved in W1-L03** — raw pointer escape from unique_ptr under lock |
| **Total** | **19,788** | **CRITICAL** | W1-L02+W1-L03+W1-L04+W1-L05+W1-L06 reduced 53 findings |

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

### Addressed in W1-L02 (2026-05-26 — critical parallel path hardening)

**Scope file:** `lora_framework/lora_training_service.cpp`

| Gap ID | Category | Finding | Fix | Impact |
|--------|----------|---------|-----|--------|
| W1-L02-DR-01 | `data_race` | TOCTOU race: `is_training_.load()` check + manual `store(true)` allowed two concurrent callers to both pass before either set the flag | Replaced with `is_training_.compare_exchange_strong()` — atomic check-and-set in a single operation | Eliminates double-start race |
| W1-L02-DR-02 | `data_race` | `current_metrics_` fields written by training thread while `getMetrics()` can read from another thread (no synchronisation) | Added `metrics_mutex_`; all writes inside the training loop and all reads in `getMetrics()` now hold the lock | Eliminates TSan-detectable race on `TrainingMetrics` |
| W1-L02-DR-03 | `data_race` | `training_callback_` written via `registerCallback()` and read inside the training loop without any lock | `registerCallback()` now acquires `metrics_mutex_`; callback copy + invocation inside the loop uses a lock-guarded snapshot | Eliminates callback pointer race |
| W1-L02-DR-04 | `data_race` | `config_.target_modules` mutated inside `trainOnTheFly()` (Phi-3 detection branch) while external thread can call `getTrainingConfig()` | `trainOnTheFly()` takes a full `Config` snapshot under `config_mutex_` at entry and operates on `local_config`; shared `config_` is never mutated inside the training loop | Eliminates config mutation race |
| W1-L02-DR-05 | `data_race` | `setTrainingConfig()` / `getTrainingConfig()` / `setHyperparameters()` / `getHyperparameters()` accessed `config_` without a lock | Added `config_mutex_` guards to all four methods | Consistent external config access |
| W1-L02-DR-06 | `data_race` | `saveCheckpoint()` read `current_metrics_` and `loss_history_` without a lock | Snapshot taken under `metrics_mutex_` before file I/O | Consistent checkpoint state |
| W1-L02-DR-07 | `data_race` | GPU `trainWithQuantization()` callback wrote to `impl_->current_metrics_` directly without a lock | Callback now acquires `metrics_mutex_` for the write and invokes the user callback outside the lock | Consistent GPU training metrics |
| W1-L02-II-01 | `iterator_invalidation` | `loss_history_.push_back()` (training thread) raced with `saveCheckpoint()`'s vector copy; a concurrent reallocation could invalidate the checkpoint's iterator | Both the `push_back()` and the checkpoint copy are now guarded by `metrics_mutex_` | Eliminates concurrent vector reallocation hazard |
| W1-L02-NT-01 | `no_timeout` | `stopTraining()` busy-polled with 100 ms sleeps; after the 30 s timeout it logged "stopped" even though `is_training_` was still `true` | Replaced spin-loop with `std::condition_variable::wait_for`; on timeout `is_training_` is force-cleared to prevent permanent stuck state | Efficient wait + reliable post-timeout cleanup |
| W1-L02-NT-02 | `no_timeout` | `is_training_.store(false)` inside the training thread was not guaranteed to be visible to `stopTraining()`'s wait predicate without a fence | Store now happens under `stop_mutex_` (the same mutex the CV waits on), followed by `stop_cv_.notify_all()` | Correct happens-before relationship |

**Focused tests added:** `ConcurrentGetMetricsIsRaceFree`, `StopTrainingClearsIsTraining`, `ConcurrentRegisterCallbackIsRaceFree`, `ConcurrentTrainCallsAreSerialised`

---

### Addressed in W1-L03 (2026-05-26 — Vulkan/DirectX kernel hardening)

**Scope files:** `lora_framework/kernels/vulkan_kernels.cpp`, `lora_framework/kernels/directx_kernels.cpp`  
**Support files fixed:** `lora_framework/vulkan_pipeline.cpp`, `lora_framework/directx_context.cpp`

| Gap ID | Category | Finding | Fix | Impact |
|--------|----------|---------|-----|--------|
| W1-L03-DR-01 | `data_race` | All 11 `launch_*` functions in `vulkan_kernels.cpp` checked `g_vulkan_state.initialized` and accessed `g_vulkan_state.context` without holding `g_vulkan_state.mutex`; TOCTOU race with `cleanup_vulkan_lora()` | Added `get_pipeline_locked()` internal helper (caller holds lock); each launch function acquires `g_vulkan_state.mutex` for its entire body | Eliminates TOCTOU race between launch and cleanup |
| W1-L03-DR-02 | `data_race` | `launch_fused_lora_forward` and `launch_fused_lora_backward` accessed `g_vulkan_state.context.get()` (for thread-local buffer cache `ensure()`) without holding the mutex; missing `initialized` guard | Both fused functions now hold `g_vulkan_state.mutex` for their full body; initialized + context null-checks added | Eliminates context-pointer race on fused kernel path |
| W1-L03-DR-03 | `data_race` | `DirectXState` had no `std::mutex`; all of `initialize_directx_lora()`, `cleanup_directx_lora()`, `get_or_load_shader()`, `get_or_create_pipeline()`, and every `launch_*` function accessed shared state unsynchronised | Added `std::mutex mutex` to `DirectXState`; introduced `get_or_load_shader_locked()` + `get_or_create_pipeline_locked()` internal helpers; all public functions acquire the mutex for their full body | Eliminates all D3D12 command-list recording data races |
| W1-L03-SP-01 | `smart_ptr_misuse` | `get_pipeline()` extracted a raw `VulkanComputePipeline*` from a `unique_ptr` under lock, then returned it; callers used the raw pointer after the lock was released — dangling pointer if `cleanup_vulkan_lora()` ran concurrently | Lock now covers the full dispatch body; `get_pipeline_locked()` is only called under the already-held mutex | Raw pointer lifetime is fully covered by the lock scope |
| W1-L03-SP-02 | `smart_ptr_misuse` | Same pattern in `get_or_create_pipeline()` for DirectX: raw `DirectXPipeline*` returned to callers outside the (non-existent) lock | Addressed by W1-L03-DR-03: entire dispatch body now holds `DirectXState::mutex` | Consistent with Vulkan fix |
| W1-L03-NT-01 | `no_timeout` | `VulkanComputePipeline::wait()` called `context_->wait_for_fence(fence_)` with default `UINT64_MAX` timeout — 585-year wait on GPU hang | `wait()` now uses a 30-second timeout (`30'000'000'000` ns) and throws `std::runtime_error` on `VK_TIMEOUT` | Process terminates cleanly on GPU hang instead of blocking forever |
| W1-L03-NT-02 | `no_timeout` | `DirectXContext::wait_for_gpu()` called `WaitForSingleObject(fence_event_, INFINITE)` — blocks the thread permanently on GPU hang | Replaced with `WaitForSingleObject(fence_event_, 30'000)` (30 s); throws on `WAIT_TIMEOUT` or `WAIT_FAILED` | Consistent timeout semantics with Vulkan path |
| W1-L03-TC-01 | `type_conversion` | `launch_scalar_multiply_shader` and `launch_lora_grad_A/B_shader` used `*reinterpret_cast<const uint32_t*>(&scalar)` to bit-cast `float` — strict-aliasing UB | Replaced with the existing `float_to_bits()` helper (uses `std::memcpy` internally) | Eliminates undefined behaviour in push-constant encoding |
| W1-L03-IV-01 | `input_validation` | `launch_embedding_lookup_shader` in DirectX path had no null-pointer or zero-dimension guards before accessing `g_directx_state` | Added identical null + dimension guards (matching Vulkan path) before mutex acquisition | Consistent defensive input validation |
| W1-L03-IV-02 | `input_validation` | `launch_sequence_mean_shader` in DirectX path had no null-pointer or zero-dimension guards | Same fix as W1-L03-IV-01 | Consistent defensive input validation |

**Focused tests added:** `test_w1l03_kernel_hardening.cpp` — 15 tests covering uninitialized-call throws, null-pointer guards, zero-dimension rejection, concurrent uninitialized call safety (8-thread), and non-Windows stub behaviour.

---

### Addressed in W1-L04 (2026-05-26 — unchecked CUDA/HIP API return values + OOP-01)

**Scope files:** `lora_framework/nccl_backend.cpp`, `lora_framework/rccl_backend.cpp`, `gpu_memory_manager.cpp`, `lora_framework/multi_gpu_trainer.cpp`, `llm_plugin_interface.h`

| Gap ID | Category | Finding | Fix | Impact |
|--------|----------|---------|-----|--------|
| W1-L04-REL-10 | `reliability` | `cudaStreamSynchronize()` return value silently ignored after NCCL `allreduce` in `NCCLBackend::synchronize()` — stream errors invisible | Return value checked; logs error and returns `false` on failure | CUDA stream errors propagate to callers |
| W1-L04-REL-11 | `reliability` | `cudaStreamSynchronize()` return value silently ignored after NCCL `broadcast` in `NCCLBackend::broadcast()` | Return value checked; returns `false` on failure | Consistent with allreduce path |
| W1-L04-REL-12 | `reliability` | `cudaStreamSynchronize()` return value silently ignored in `NCCLBackend::barrier()` — barrier completeness unverified | Return value checked; logs error on failure (barrier cannot propagate a bool; error is surfaced via log) | GPU barrier hang/error now observable |
| W1-L04-REL-13 | `reliability` | `cudaSetDevice()` silently ignored in `NCCLBackend::initialize_nccl()` — subsequent NCCL operations run on wrong device if set fails | Return value checked; logs error and returns `false` | Prevents NCCL init on incorrect device |
| W1-L04-REL-14 | `reliability` | `hipStreamSynchronize()` return value silently ignored after RCCL `allreduce` in `RCCLBackend::synchronize()` | Return value checked; logs error and returns `false` on failure | ROCm stream errors propagate to callers |
| W1-L04-REL-15 | `reliability` | `hipStreamSynchronize()` return value silently ignored after RCCL `broadcast` in `RCCLBackend::broadcast()` | Return value checked; returns `false` on failure | Consistent with allreduce path |
| W1-L04-REL-16 | `reliability` | `hipStreamSynchronize()` return value silently ignored in `RCCLBackend::barrier()` | Return value checked; logs error on failure | ROCm barrier hang/error now observable |
| W1-L04-REL-17 | `reliability` | `hipSetDevice()` silently ignored in `RCCLBackend::initialize_rccl()` | Return value checked; logs error and returns `false` | Prevents RCCL init on incorrect device |
| W1-L04-REL-18 | `reliability` | `cudaSetDevice()` silently ignored in `GPUAllocation::freeGPUMemory()` destructor — secure clear runs on wrong device if fails | Return value checked; logs error and continues (destructor context: cannot throw; attempts clear on current device) | GPU secure clear error now observable; no UB |
| W1-L04-REL-19 | `reliability` | `cudaDeviceCanAccessPeer()` return value ignored in `GPUMemoryManager::initializeGPU()` P2P setup | Return value checked; logs warning and skips peer pair on failure | Prevents P2P enable attempt after API error |
| W1-L04-REL-20 | `reliability` | `cudaSetDevice()` silently ignored in P2P enable loop in `initializeGPU()` | Return value checked; logs warning and `continue`s to next pair | Prevents P2P enable attempt on wrong device |
| W1-L04-REL-21 | `reliability` | `cudaSetDevice()` silently ignored in `shutdownGPU()` peer-disable loop | Return value checked; logs warning and `continue`s | Prevents disablePeerAccess attempt on wrong device |
| W1-L04-REL-22 | `reliability` | `cudaSetDevice()` silently ignored in `shutdownGPU()` device-reset loop | Return value checked; logs warning and `continue`s | Prevents `cudaDeviceReset` attempt on wrong device |
| W1-L04-REL-23 | `reliability` | `cudaSetDevice()` silently ignored in memory defrag loop in `GPUMemoryManager::defragGPUMemory()` | Return value checked; logs warning and `continue`s to next device | Prevents malformed `cudaMalloc` on wrong device |
| W1-L04-REL-24 | `reliability` | `cudaMemcpy()` DeviceToDevice during defrag silently ignored — consolidation may silently corrupt layout | Return value checked; frees new allocation, logs error, and skips device on failure | Eliminates silent defrag corruption |
| W1-L04-REL-25 | `reliability` | `cudaSetDevice()` silently ignored in SGD update loop in `MultiGPUTrainer::updateParameters()` — kernel dispatched to wrong device on failure | Return value checked; logs warning and falls through to CPU fallback via `else` branch | Consistent with per-param CPU fallback logic |
| W1-L04-REL-26 | `reliability` | `hipSetDevice()` silently ignored in SGD update loop in `MultiGPUTrainer::updateParameters()` | Return value checked; logs warning and falls through to CPU fallback via `else` branch | ROCm path consistent with CUDA fix |
| W1-L04-OOP-01 | `oop_design` | `LLMPluginAdapter` derived from `plugins::IThemisPlugin` (which has `virtual ~IThemisPlugin()`) but declared no `~LLMPluginAdapter() override` — deleting through base pointer leaks the contained `unique_ptr<ILLMPlugin>` | Added `~LLMPluginAdapter() override = default;` | Correct virtual destructor chain; unique_ptr releases the owned plugin |

**Files modified:** `nccl_backend.cpp`, `rccl_backend.cpp`, `gpu_memory_manager.cpp`, `multi_gpu_trainer.cpp`, `llm_plugin_interface.h`

---

### Addressed in W1-L05 (2026-05-26 — secondary-path reliability checks)

**Scope files:** `lora_framework/gpu_memory.cpp`, `llama_wrapper.cpp`

| Gap ID | Category | Finding | Fix | Impact |
|--------|----------|---------|-----|--------|
| W1-L05-REL-27 | `reliability` | `cudaRuntimeGetVersion()` return value ignored in backend detection (`GPUMemoryManager::detect_backends`) | Return value checked; warns on failure, version string set only on success | Avoids reporting invalid CUDA runtime version |
| W1-L05-REL-28 | `reliability` | `hipGetDeviceProperties()` unchecked and `hipDeviceProp_t` uninitialised in backend detection | Zero-initialised `hipDeviceProp_t prop{}`; return value checked with warning path | Prevents reading undefined HIP device fields |
| W1-L05-REL-29 | `reliability` | `hipRuntimeGetVersion()` return value ignored in backend detection | Return value checked; warns on failure, version string set only on success | Avoids reporting invalid HIP runtime version |
| W1-L05-REL-30 | `reliability` | Vulkan backend probe ignored non-success `vkCreateInstance` result | Captured `VkResult`; explicit warning on failure | Vulkan probe failures are now diagnosable |
| W1-L05-REL-31 | `reliability` | Vulkan backend probe ignored error details when `vkEnumeratePhysicalDevices(..., nullptr)` failed | Captured count-call `VkResult`; warning on non-success | Improves triage for Vulkan enumeration failures |
| W1-L05-REL-32 | `reliability` | Vulkan backend probe ignored error details when `vkEnumeratePhysicalDevices(..., data)` failed | Captured fill-call `VkResult`; warning on non-success | Improves triage for Vulkan device-list retrieval failures |
| W1-L05-REL-33 | `reliability` | Vision prefill path in `generateVision()` silently ignored `llama_decode` failures (`if (...) == 0` only) | Captured decode result; warning emitted on failure before continuing | Secondary decode failures are now observable in logs |

**Files modified:** `lora_framework/gpu_memory.cpp`, `llama_wrapper.cpp`

---

### Addressed in W1-L06 (2026-05-26 — multi-GPU reliability + RAII hardening)

**Scope files:** `lora_framework/multi_gpu.cpp`, `lora_framework/custom_allreduce.cpp`, `multi_gpu_memory_coordinator.cpp`, `lora_framework/vram_allocator.cpp`

| Gap ID | Category | Finding | Fix | Impact |
|--------|----------|---------|-----|--------|
| W1-L06-REL-34 | `reliability` | `cudaSetDevice()` ignored in `MultiGPUContext::synchronize_all()` | Return value checked; logs warning and skips sync on failure | Prevents sync on wrong CUDA device |
| W1-L06-REL-35 | `reliability` | `cudaDeviceSynchronize()` ignored in `MultiGPUContext::synchronize_all()` | Return value checked; logs warning on failure | Makes cross-device sync failures observable |
| W1-L06-REL-36 | `reliability` | `hipSetDevice()` ignored in `MultiGPUContext::synchronize_all()` | Return value checked; logs warning and skips sync on failure | Prevents sync on wrong HIP device |
| W1-L06-REL-37 | `reliability` | `hipDeviceSynchronize()` ignored in `MultiGPUContext::synchronize_all()` | Return value checked; logs warning on failure | Makes HIP sync failures observable |
| W1-L06-REL-38 | `reliability` | `cudaDeviceCanAccessPeer()` ignored in CUDA topology detection | Return value checked; logs warning and falls back to conservative bandwidth | Prevents false P2P topology assumptions |
| W1-L06-REL-39 | `reliability` | `hipDeviceCanAccessPeer()` ignored in HIP topology detection | Return value checked; logs warning and falls back to conservative bandwidth | Prevents false HIP P2P topology assumptions |
| W1-L06-REL-40 | `reliability` | `cudaDeviceCanAccessPeer()` / `cudaSetDevice()` ignored during CUDA P2P setup in `CustomAllReduce` | Return values checked before enabling peer access | Prevents P2P enable on the wrong CUDA device |
| W1-L06-REL-41 | `reliability` | `hipDeviceCanAccessPeer()` / `hipSetDevice()` ignored during HIP P2P setup in `CustomAllReduce` | Return values checked before enabling peer access | Prevents P2P enable on the wrong HIP device |
| W1-L06-REL-42 | `reliability` | `cudaSetDevice()` ignored during CUDA GPU discovery in `MultiGPUMemoryCoordinator::initialize()` | Return value checked; failed devices skipped | Avoids querying VRAM on the wrong CUDA device |
| W1-L06-REL-43 | `reliability` | `hipSetDevice()` ignored during HIP GPU discovery in `MultiGPUMemoryCoordinator::initialize()` | Return value checked; failed devices skipped | Avoids querying VRAM on the wrong HIP device |
| W1-L06-REL-44 | `reliability` | `cudaSetDevice()` ignored before CUDA P2P enable in `MultiGPUMemoryCoordinator::enableP2PAccess()` | Return value checked; failed direction counted and skipped | Prevents malformed CUDA P2P enable attempts |
| W1-L06-REL-45 | `reliability` | `hipSetDevice()` ignored before HIP P2P enable in `MultiGPUMemoryCoordinator::enableP2PAccess()` | Return value checked; failed direction counted and skipped | Prevents malformed HIP P2P enable attempts |
| W1-L06-REL-46 | `reliability` | `cudaSetDevice()` ignored before `cudaDeviceSynchronize()` in `MultiGPUMemoryCoordinator::synchronizeAll()` | Return value checked; failed device skipped | Prevents synchronizing the wrong CUDA device |
| W1-L06-REL-47 | `reliability` | `hipSetDevice()` ignored before `hipDeviceSynchronize()` in `MultiGPUMemoryCoordinator::synchronizeAll()` | Return value checked; failed device skipped | Prevents synchronizing the wrong HIP device |
| W1-L06-REL-48 | `reliability` | Vulkan backend init in `VRAMAllocator::initialize_backend()` used raw `new VulkanAllocContext()` with manual delete on failure | Replaced with `std::make_unique` + `release()` handoff after successful init | Eliminates exception-unsafe raw allocation pattern in backend setup |

**Files modified:** `lora_framework/multi_gpu.cpp`, `lora_framework/custom_allreduce.cpp`, `multi_gpu_memory_coordinator.cpp`, `lora_framework/vram_allocator.cpp`

---

### Addressed in W1-L07 (2026-05-26 — CUDA sync/memory-info reliability hardening)

**Scope files:** `lora_framework/flash_lora.cpp`, `attention/cuda/flash_attention_cuda.cu`

| Gap ID | Category | Finding | Fix | Impact |
|--------|----------|---------|-----|--------|
| W1-L07-REL-49 | `reliability` | `cudaDeviceSynchronize()` return value silently ignored at end of FlashLoRA forward pass | Captured return value and throw `std::runtime_error` on failure with CUDA error string | Prevents silent forward-path kernel/sync failures |
| W1-L07-REL-50 | `reliability` | `cudaDeviceSynchronize()` return value silently ignored at end of FlashLoRA backward pass | Captured return value and throw `std::runtime_error` on failure with CUDA error string | Prevents silent backward-path kernel/sync failures |
| W1-L07-REL-51 | `reliability` | `cudaMemGetInfo()` return value silently ignored in `FlashAttentionCUDA::getMemoryStats()` | Captured return value and throw `std::runtime_error` on failure with CUDA error string | Prevents silent GPU memory-telemetry failures |

**Files modified:** `lora_framework/flash_lora.cpp`, `attention/cuda/flash_attention_cuda.cu`

---

### Addressed in W1-L08 (2026-05-26 — CUDA/HIP cleanup + allocator reliability hardening)

**Scope files:** `lora_framework/kernels/cuda_kernels.cu`, `lora_framework/kernels/hip_kernels.cpp`, `lora_framework/kernels/quantization_kernels.cu`, `llm/lora_framework/quantization_kernels.h`

| Gap ID | Category | Finding | Fix | Impact |
|--------|----------|---------|-----|--------|
| W1-L08-REL-52 | `reliability` | `cudaFree()` result ignored during `launch_check_inf_nan_kernel()` cleanup-after-`cudaMemset` failure | Captured `cudaFree` result and return cleanup error when free fails | Prevents silent device-memory cleanup failure masking original path |
| W1-L08-REL-53 | `reliability` | `cudaFree()` result ignored during `launch_check_inf_nan_kernel()` cleanup-after-kernel-launch failure | Captured `cudaFree` result and return cleanup error when free fails | Prevents silent cleanup failure on kernel launch error path |
| W1-L08-REL-54 | `reliability` | Final `cudaFree()` result ignored in `launch_check_inf_nan_kernel()` success/transfer path | Captured final `cudaFree` result and return error when free fails | Prevents silent leak/cleanup failures in overflow-check helper |
| W1-L08-REL-55 | `reliability` | `hipFree()` result ignored during `launch_check_inf_nan_kernel()` cleanup-after-`hipMemset` failure | Captured `hipFree` result and return cleanup error when free fails | Prevents silent HIP cleanup failure masking original path |
| W1-L08-REL-56 | `reliability` | `hipFree()` result ignored during `launch_check_inf_nan_kernel()` cleanup-after-kernel-launch failure | Captured `hipFree` result and return cleanup error when free fails | Prevents silent HIP cleanup failure on kernel launch error path |
| W1-L08-REL-57 | `reliability` | Final `hipFree()` result ignored in HIP `launch_check_inf_nan_kernel()` success/transfer path | Captured final `hipFree` result and return error when free fails | Prevents silent HIP leak/cleanup failures in overflow-check helper |
| W1-L08-REL-58 | `reliability` | `cudaMalloc()` result ignored in `GPUMemoryManager::allocateQuantizedBuffer()` | Captured result and return `nullptr` on failure; `total_allocated_` updated only on success | Prevents false allocation accounting and null-unsafe caller assumptions |
| W1-L08-REL-59 | `reliability` | `cudaMallocHost()` result ignored in `GPUMemoryManager::allocatePinnedHost()` | Captured result and return `nullptr` on failure | Prevents silent pinned-host allocation failures |
| W1-L08-REL-60 | `reliability` | `cudaFree()` result ignored in `GPUMemoryManager::freeDevice()` | Captured result and explicitly short-circuit on free failure | Prevents silent free failure in device cleanup path |
| W1-L08-REL-61 | `reliability` | `cudaFreeHost()` result ignored in `GPUMemoryManager::freePinned()` | Captured result and explicitly short-circuit on free failure | Prevents silent pinned-memory free failure |

**Files modified:** `lora_framework/kernels/cuda_kernels.cu`, `lora_framework/kernels/hip_kernels.cpp`, `lora_framework/kernels/quantization_kernels.cu`, `llm/lora_framework/quantization_kernels.h`

---

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

**Status (v1.21.0-pre — batch 33):** REL-49..REL-51 fixed — both trailing
`cudaDeviceSynchronize()` calls in `FlashLoRA::{forward,backward}` now checked and throw
on failure (`flash_lora.cpp`); `cudaMemGetInfo()` in
`FlashAttentionCUDA::getMemoryStats()` now checked and throws on failure
(`attention/cuda/flash_attention_cuda.cu`).

**Status (v1.21.0-pre — batch 34):** REL-52..REL-61 fixed — overflow-flag cleanup
paths now validate `cudaFree`/`hipFree` return values in CUDA/HIP kernel helpers
(`lora_framework/kernels/cuda_kernels.cu`, `lora_framework/kernels/hip_kernels.cpp`);
`GPUMemoryManager` quantization allocators now validate `cudaMalloc`/`cudaMallocHost`
and explicit free paths validate `cudaFree`/`cudaFreeHost`, with allocation docs updated
to state `nullptr` on failure (`lora_framework/kernels/quantization_kernels.cu`,
`include/llm/lora_framework/quantization_kernels.h`).

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
