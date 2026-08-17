# LLM Module - Architecture Guide

<!-- Status: enhanced | validated: 2026-08-17 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · THREADING.md · OPERATIONS.md -->

Version: 2.0 (Phase 6 Enhanced)
Last Updated: 2026-08-17
Module Path: src/llm/
Status: PRODUCTION (Wave 5 GA)

---

## 1. Overview

The LLM module provides inference execution, routing, model and adapter lifecycle control, streaming, policy enforcement, and runtime safety surfaces for ThemisDB AI capabilities. The module is designed for production use with robust concurrency controls, fail-closed behavior patterns, and comprehensive observability.

**Key Properties:**
- **Thread-Safe:** Yes (see [THREADING.md](THREADING.md) for detailed model)
- **Async-First:** Yes (non-blocking inference paths throughout)
- **Fail-Closed:** Yes (safe degradation on resource exhaustion, timeout, or backend failure)
- **GPU/CPU:** Both (automatic fallback; configurable routing)
- **Distributed:** Yes (federated inference via deployment plugins)

---

## 2. Architecture Surfaces

| Surface | Source files | Primary Role |
|---|---|---|
| **Core Inference Engines** | `async_inference_engine.cpp`, `inference_engine_enhanced.cpp` | Asynchronous request submission, routing, and execution coordination |
| **Scheduling & Queueing** | `shared_worker_pool.cpp`, `continuous_batch_scheduler.cpp` | Worker pool management and batch request scheduling |
| **Routing & Orchestration** | `model_router.cpp`, `ai_orchestrator.cpp` | Rule-based model/worker selection and multi-request orchestration |
| **Model & Plugin Lifecycle** | `llm_plugin_manager.cpp`, `model_loader.cpp`, `model_downloader.cpp` | Load, unload, switch, and manage inference backends and model files |
| **Adapter & LoRA Lifecycle** | `multi_lora_manager.cpp`, `adapter_load_balancer.cpp`, `lora_router.cpp` | Load, hot-swap, and unload LoRA adapter chains |
| **Streaming & Response Shaping** | `streaming_handler.cpp`, `openai_compat_adapter.cpp` | Token streaming, callback framing, and OpenAI protocol adaptation |
| **Policy & Safety Controls** | `prompt_policy.cpp`, `llm_security_utils.cpp`, `production_validator.cpp` | Prompt validation, safety guardrails, and production readiness checks |
| **Caching & Resource Controls** | `llm_response_cache.cpp`, `kv_cache_buffer.cpp`, `token_quota_manager.cpp` | KV cache management, response caching, and token quota enforcement |
| **GPU Memory & Acceleration** | `gpu_memory_manager.cpp`, `active_vram_allocator.cpp`, `kernel_fusion.cu` | GPU memory allocation, defragmentation, and fused kernel compilation |

---

## 3. Runtime Control Flow

```
Request Entry
    ↓
[1] Policy/Guard Checks (prompt_policy, llm_security_utils)
    ↓ (pass → continue; fail → reject early)
[2] Routing & Model Selection (model_router, ai_orchestrator)
    ↓
[3] Adapter/Plugin Selection (lora_router, llm_plugin_manager)
    ↓
[4] Queue & Scheduler (shared_worker_pool, continuous_batch_scheduler)
    ↓
[5] Backend Inference (inference_engine, llamacpp/plugin)
    ↓ (with streaming callbacks, KV cache, resource limits)
[6] Response Framing & Emit (streaming_handler, openai_compat_adapter)
    ↓
[7] Cache Store (llm_response_cache, llm_prefix_cache)
    ↓
Response Completion
```

**Error Flow:**
- Policy rejection → immediate HTTP 400/403 response
- Model not loaded → attempt load; if fails → HTTP 503 with retry hint
- VRAM exhausted → evict LRU model, retry; if still fails → HTTP 429 (backpressure)
- Inference timeout → cancel in-flight request, return HTTP 408 (or incomplete stream)
- Backend unavailable → fail-closed (CPU fallback if available; else HTTP 503)

---

## 4. Integration Boundaries

| Direction | Integration | Contract |
|---|---|---|
| **Used by** | API layer (`api/llm_handler.h`), orchestration features, ingestion pipeline (`ingestion/llm_adapter.h`), RAG systems | Async request submission, streaming callbacks, lifecycle hooks |
| **Uses** | Backend plugins (llama.cpp, VLLM, etc.), GPU drivers, RocksDB (state store), file system (models), optional hardware accelerators | Pluggable inference, storage abstraction, model loading |
| **Exposes** | Core inference APIs, routing hooks, adapter lifecycle hooks, streaming callbacks, policy guards | Public headers in `include/llm/` |

---

## 5. Concurrency Model

**Thread-Safety Guarantee:** High.

The module uses a layered concurrency model with explicit synchronization at key points:

1. **Shared Worker Pool** (`shared_worker_pool.cpp`)
   - Thread-safe work queue with FIFO scheduling
   - Multiple worker threads execute inference tasks in parallel
   - Coordination via lock-free queue or mutex-protected deque

2. **Model Lifecycle** (`llm_plugin_manager.cpp`)
   - Single-writer, multiple-reader semantics for model load/unload
   - Protected by `model_load_mutex_` (read-write lock)
   - In-flight requests hold read locks; loader holds write lock

3. **Adapter Lifecycle** (`multi_lora_manager.cpp`)
   - LoRA hot-swap protected by `adapter_lifecycle_mutex_`
   - Prevents concurrent load/switch/unload on same adapter chain
   - Readers proceed under read lock; writers acquire exclusive lock

4. **Response Cache** (`llm_response_cache.cpp`, `llm_prefix_cache.cpp`)
   - Atomic operations on cache keys and reference counts
   - Lock-free or RCU (read-copy-update) patterns where possible
   - RocksDB backend provides transactional semantics for persistent cache

5. **Streaming Callbacks**
   - Per-request token buffer isolation (no shared state)
   - Caller responsible for thread-safe accumulation of streamed tokens

6. **GPU Memory** (`gpu_memory_manager.cpp`, `active_vram_allocator.cpp`)
   - Mutex-protected allocation/deallocation
   - No concurrent access to the same GPU buffer

**For detailed thread-safety contracts, see [THREADING.md](THREADING.md).**

---

## 6. Resource Management

### Memory Model

- **Heap:** Standard C++ dynamic allocation via `std::unique_ptr`, `std::shared_ptr`
- **GPU Memory:** Managed by `gpu_memory_manager.cpp` with quota enforcement and defragmentation
- **KV Cache:** Managed by `kv_cache_buffer.cpp` with page-based allocation and LRU eviction
- **Response Cache:** Managed by `llm_response_cache.cpp` with configurable TTL and size limits
- **Model Cache:** Managed by `model_loader.cpp` with size quotas and LRU unload policy

### Cleanup & Shutdown

- Models unloaded in reverse load order during module shutdown
- Adapters unloaded and cache cleared before module cleanup
- GPU memory defragmented before final cleanup
- All pending inference requests are cancelled with `CancellationToken`

---

## 7. Error Handling & Fail-Closed Behavior

**Fail-Closed Principle:** The module defaults to safe degradation and refuses unsafe operation.

| Scenario | Behavior | Status Code |
|---|---|---|
| **Policy Violation** | Reject request; log violation; audit trail | `Status::kPolicyViolation` |
| **Model Not Found** | Return 404 if not loadable; suggest alternatives | `Status::kNotFound` |
| **Model Load Failure** | Reject new inference; do not proceed; audit log | `Status::kModelLoadFailed` |
| **VRAM Exhaustion** | Evict LRU models; retry; if persistent → backpressure | `Status::kOutOfMemory` |
| **Request Timeout** | Cancel in-flight work; return `kDeadlineExceeded` | `Status::kDeadlineExceeded` |
| **Stream Abort** | Stop token generation; cleanup in-flight resources; signal caller | `Status::kCancelled` |
| **Backend Unavailable** | Attempt CPU inference fallback; fail fast if unavailable | `Status::kUnavailable` |
| **Plugin Load Failure** | Skip plugin; try next available backend; log error; do not crash | `Status::kFailedPrecondition` |

**Key Guarantee:** No silent failures. All errors are explicitly surfaced to the caller or logged for observability.

---

## 8. Performance Expectations & Constraints

### Latency

- **First-token latency (cached model):** ~50–200 ms (GPU) / ~200–500 ms (CPU)
- **Streaming throughput:** 50–100 tokens/sec (typical GPU, varies by model size)
- **KV cache page allocation:** ~5–10 μs (lock-free)
- **Model switch latency:** ~500 ms–2s (load from disk/network)

### Throughput

- **Concurrent requests:** Depends on model size and GPU memory; typically 10–100 concurrent requests on a single GPU
- **Batch scheduling:** Continuous batching up to configurable limit (default: 1024 tokens/batch)
- **Worker threads:** Configurable; default: `std::thread::hardware_concurrency()`

### Resource Constraints

- **GPU Memory:** Configurable quota per model; default: 80% of available VRAM
- **Response Cache:** Configurable max size; default: 1 GB
- **Token Quota:** Configurable per-request and per-model; default: unlimited
- **Timeout:** Configurable per-request; default: 300 seconds

---

## 9. Known Limits & Future Work

### Current Limitations

- **Distributed Inference:** Requires deployment wiring (not default-on); federated paths tested in Wave B
- **Multi-GPU Coordination:** Basic support via `multi_gpu_memory_coordinator.cpp`; advanced NCCL/collective ops in Wave C
- **Mixed-Precision Inference:** Supported but not auto-tuned; manual quantization pipeline
- **Long-Context Handling:** Up to model limit (~100k tokens); efficient paging in Wave C

### Wave C Enhancements (Future)

- Speculative decoding integration for up to 2x speedup
- Distributed end-to-end inference optimization
- Advanced GPU memory fragmentation recovery
- Persistent embedding cache (RocksDB integration)

---

## 10. Sourcecode Verification (Module: llm/architecture — Phase 6)

### Verified Files

**Core Inference:**
- ✅ `src/llm/async_inference_engine.cpp` (Phase 5+)
- ✅ `src/llm/inference_engine_enhanced.cpp` (Phase 5+)
- ✅ `src/llm/shared_worker_pool.cpp` (Phase 5+)
- ✅ `src/llm/continuous_batch_scheduler.cpp` (Phase 5+)

**Routing & Orchestration:**
- ✅ `src/llm/model_router.cpp` (Phase 5+)
- ✅ `src/llm/ai_orchestrator.cpp` (Phase 5+)
- ✅ `src/llm/llm_plugin_manager.cpp` (Phase 5+)

**Adapter & LoRA:**
- ✅ `src/llm/multi_lora_manager.cpp` (Phase 5+)
- ✅ `src/llm/adapter_load_balancer.cpp` (Phase 5+)
- ✅ `src/llm/lora_router.cpp` (Phase 5+)

**Streaming & Safety:**
- ✅ `src/llm/streaming_handler.cpp` (Phase 5+)
- ✅ `src/llm/openai_compat_adapter.cpp` (Phase 5+)
- ✅ `src/llm/prompt_policy.cpp` (Phase 5+)
- ✅ `src/llm/llm_security_utils.cpp` (Phase 5+)
- ✅ `src/llm/production_validator.cpp` (Phase 5+)

**Resource Management:**
- ✅ `src/llm/token_quota_manager.cpp` (Phase 5+)
- ✅ `src/llm/llm_response_cache.cpp` (Phase 5+)
- ✅ `src/llm/kv_cache_buffer.cpp` (Phase 5+)
- ✅ `src/llm/gpu_memory_manager.cpp` (Phase 6)
- ✅ `src/llm/active_vram_allocator.cpp` (Phase 6)

### Verified Interfaces & Behavior

- ✅ Request submit/schedule/execute flow
- ✅ Routing and lifecycle integration points
- ✅ Streaming and policy guard control surfaces
- ✅ Thread-safety contracts (Phase 6: documented in THREADING.md)
- ✅ Fail-closed error handling patterns (Phase 6: documented in OPERATIONS.md)
- ✅ Resource cleanup and shutdown sequencing

### Quality Gate Status

| Gate | Status | Evidence |
|---|---|---|
| **Documentation** | ✅ PHASE 6 COMPLETE | ARCHITECTURE.md, README.md, THREADING.md, OPERATIONS.md, API_REFERENCE.md, DEVELOPER_GUIDE.md |
| **Thread Safety** | ✅ DOCUMENTED | THREADING.md with detailed contracts and synchronization primitives |
| **Fail-Closed** | ✅ DOCUMENTED | OPERATIONS.md with error handling runbooks and recovery procedures |
| **Build & Tests** | ✅ VERIFIED | CI green on develop; Wave 5 acceptance gates passed |
| **Inline Docs** | ✅ ONGOING | @file headers added to all .cpp files; function-level docs for public APIs |

### Known Issues & Tracking

- Wave B tracking issue: https://github.com/makr-code/ThemisDB/issues/5039
- Dependent Wave A issue: https://github.com/makr-code/ThemisDB/issues/5038
- Follow-on Wave C issue: https://github.com/makr-code/ThemisDB/issues/5040
- Phase 6 Documentation Enhancement: Tracked in this MODULE_GAPS.md remediation

---

## 11. Related Documentation

- **README.md:** Quick start, API overview, configuration
- **THREADING.md:** Detailed thread-safety model and synchronization contracts
- **OPERATIONS.md:** Operational runbooks, debugging, performance tuning
- **API_REFERENCE.md:** Complete API documentation and usage patterns
- **DEVELOPER_GUIDE.md:** Contributing guide, testing strategy, build procedures
- **CONFIGURATION.md:** Configuration options, environment variables, tuning guide
- **ROADMAP.md:** Feature pipeline and release planning
- **FUTURE_ENHANCEMENTS.md:** Planned enhancements and Wave C work

