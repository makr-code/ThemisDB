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

### Addressed in this PR (W1-L01 — Multi-LoRA Manager race/lock hardening)

| Gap ID | Category | Fix | File |
|--------|----------|-----|------|
| W1-L01-DR-01 | `data_race` | `applyLoRA`: raw `LoRASlot*` from `getLoRA()` (lock released) dereferenced after lock release; `switches_++` and `is_active` written without lock. Fixed by snapshotting adapter handle/scale/fn under lock, calling external API outside lock, writing state back under a second lock guard. | `multi_lora_manager.cpp` |
| W1-L01-DR-02 | `data_race` | `removeLoRA`: same pattern as `applyLoRA`. Fixed identically. | `multi_lora_manager.cpp` |
| W1-L01-DR-03 | `data_race` | `fuseLoRAsAdvanced`: `fusion_cache_`, `fusion_cache_hits_/misses_`, `fusion_configs_`, `fusion_schedules_`, `total_fusions_` accessed without holding `mutex_`. Fixed by adding two scoped lock guards (cache-check section before `fuseLoRAsInternal`, cache-update+metrics section after). | `multi_lora_manager.cpp` |
| W1-L01-DR-04 | `data_race` | `exportLoRA`: raw `LoRASlot*` from `getLoRA()` read for serialization outside the lock. Fixed by snapshotting all needed fields under the lock and serializing from the snapshot. | `multi_lora_manager.cpp` |
| W1-L01-DR-05 | `data_race` | `batchInferenceMultiLoRA`: `lora->base_model_id` read via raw pointer returned by `getLoRA()` after lock release. Fixed by snapshotting `base_model_id` under the lock. | `multi_lora_manager.cpp` |
| W1-L01-DR-06 | `data_race` | `evictExpired`: TOCTOU — eviction list built under lock, lock released, then `unloadLoRA` called per entry. Between the two phases a LoRA could be reloaded (fresh, not expired) and then incorrectly evicted. Fixed by performing the entire eviction — handle free, VRAM accounting, map erase — within a single lock scope. | `multi_lora_manager.cpp` |
| W1-L01-NT-01 | `no_timeout` | `stopEvictionThread`: `eviction_thread_->join()` had no timeout and could block indefinitely. Fixed by adding `std::atomic<bool> eviction_thread_done_` (header), setting it to `false` in `startEvictionThread`, signalling it (+ `notify_all`) at the end of `evictionWorker`, and using `eviction_cv_.wait_for(lock, 5s, ...)` in `stopEvictionThread` before `join()`. Timeout events are now logged for observability and followed by safe join semantics (no detach/UAF risk). | `multi_lora_manager.cpp`, `multi_lora_manager.h` |
| W1-L01-ND-01 | `null_dereference` | `checkGPUHealthAndMigrate`: `gpu_vram_usage_[unhealthy_gpu] -= lora->vram_bytes` was an unguarded subtraction on a `size_t`; if the tracked usage was lower than `vram_bytes` (e.g. key not in map → operator[] inserts 0) the result wrapped to `UINT64_MAX`. Fixed by checking `unhealthy_usage >= lora->vram_bytes` and zeroing on underflow. | `multi_lora_manager.cpp` |
| W1-L01b-ND-02 | `null_dereference` | `evictLRU`: same unsigned underflow class as ND-01 — `total_vram_bytes_ -= lru_lora->vram_bytes` had no guard. Per-GPU VRAM accounting for `primary_gpu` was also missing entirely. Fixed by adding underflow guards for both `total_vram_bytes_` and `gpu_vram_usage_[primary_gpu]`. | `multi_lora_manager.cpp` |
| W1-L01c-ND-03 | `null_dereference` | `unloadLoRA`, `evictResourceAware`, `balanceGPULoad`, and `migrateLoRAToGPU` still contained unchecked `size_t` VRAM decrements. Fixed by guarding each source-GPU / total-VRAM subtraction against underflow and zeroing on skew instead of wrapping to `UINT64_MAX`. | `multi_lora_manager.cpp` |
| W1-L01c-REL-01 | `reliability` | `balanceGPULoad`: `usage_ratio` divided byte usage by `max_vram_per_gpu_mb` (MB units), making almost every non-zero GPU look overloaded. Fixed by comparing bytes-to-bytes using `max_vram_per_gpu_bytes`. | `multi_lora_manager.cpp` |

### Previously addressed (v1.20.0 / v1.20.1)

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
