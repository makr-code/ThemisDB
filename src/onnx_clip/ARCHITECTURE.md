> **Status:** current | validated: 2026-10-08

<!-- Status: current | validated: 2026-10-08 -->
<!-- Links: README.md · PRODUCTION_REQUIREMENTS.md · ROADMAP.md · SECURITY.md · FUTURE_ENHANCEMENTS.md -->

# ONNX CLIP Plugin — Architecture Guide

**Version:** 0.3.0
**Last Updated:** 2026-10-08
**Module Path:** `src/onnx_clip/`

---

## 1. Overview

The `onnx_clip` module is a deterministic, source-local embedding backend for CLIP-style image and text analysis. The live implementation is intentionally portable: it validates configuration, bounds the batch fan-in, tracks statistics, exposes health checks, supports optional model-digest verification, and provides hot-swap and mmap hooks without assuming that every environment has a complete native ONNX Runtime provider stack available.

The public interface remains a small `IImageAnalysisBackend` surface. The concrete implementation is held behind `std::shared_ptr<Impl>` inside `ONNXClipPlugin`, which keeps the public API free of provider-specific types and keeps the runtime contract stable across platforms.

---

## 2. Design Principles

- **Deterministic reference behavior** — the default implementation produces stable embeddings for a given input and configuration instead of depending on a live provider runtime.
- **Explicit validation** — `initialize()` and `reloadModel()` validate configuration before activation and fail closed on malformed config or hash mismatches.
- **Bounded request fan-in** — `generateEmbeddingBatch()` processes work in `max_batch_size` sub-batches to keep memory use predictable.
- **Thread safety** — request serialization and hot-swap coordination are handled through the plugin mutex and the in-flight request counter.
- **Best-effort optional acceleration** — optional mmap loading and provider-aware configuration are supported but not required for correctness.

---

## 3. Component Architecture

### 3.1 Component layout

```
┌────────────────────────────────────────────────────────────┐
│ ONNXClipPlugin (public API)                              │
│ - initialize(config, backend)                             │
│ - generateEmbedding(), generateEmbeddingBatch()          │
│ - generateTextEmbedding()                                 │
│ - healthCheck(), warmup(), getStatistics()                │
│ - reloadModel()                                           │
└───────────────────────┬────────────────────────────────────┘
                       │ std::shared_ptr<Impl>
                       ▼
┌────────────────────────────────────────────────────────────┐
│ ONNXClipPlugin::Impl                                       │
│ - ready / backend / model_name / embedding_dim             │
│ - max_batch_size                                           │
│ - request serialization / drain counter                   │
│ - runtime statistics and Prometheus-style counters         │
│ - optional mmap state and hash-verification hooks          │
└───────────────────────┬────────────────────────────────────┘
                       │
                       └───────────────┬────────────────────┐
                                       │
                        Deterministic plugin logic
                        config validation + hashing + batching
```

### 3.2 Interface implementation summary

| Method | Behaviour |
|--------|-----------|
| `initialize(config, backend)` | Validates config, sets active backend, optionally verifies model hash and mmap state |
| `shutdown()` | Clears internal state and resets ready flags |
| `isReady()` | Returns the plugin state after initialization |
| `getBackend()` | Returns the active backend enum |
| `generateEmbedding(image_data, metadata)` | Validates payload, hashes metadata-derived inputs, returns `EmbeddingResult` |
| `generateEmbeddingBatch(images)` | Splits work into bounded sub-batches of `max_batch_size` while preserving order |
| `generateTextEmbedding(text)` | Tokenizes and embeds supplied text payloads |
| `healthCheck()` | Ensures the plugin is initialized and the embedding dimension is positive |
| `getStatistics()` | Exposes request totals, latency, backend, batch size, and counters |
| `warmup()` | Performs a minimal readiness path without assuming provider-specific startup steps |
| `reloadModel(config)` | Validates and swaps a new implementation snapshot while draining in-flight requests |

---

## 4. Runtime pipeline

```text
input payload
  │
  ├─ validate empty / malformed input
  │
  ├─ index metadata + deterministic seed mix
  │
  ├─ bounded sub-batching when batch API is used
  │
  ├─ compute deterministic embedding vector
  │
  ├─ aggregate stats / counters / latency
  │
  └─ return EmbeddingResult{success, error, embedding}
```

The actual plugin performs configuration validation before activation, restricts batch fan-in to `max_batch_size`, and preserves request ordering across chunked processing. It does not depend on a specific provider implementation to remain functional in a portable build.

---

## 5. Backend selection

The current implementation is intentionally explicit: `BackendType::AUTO` resolves to `CPU` in the default portable configuration. Provider strings and enum entries remain present for compatibility, but they are treated as configuration metadata rather than a guarantee that a native GPU or runtime stack is available in every deployment.

---

## 6. Integration points

| Direction | Module | Interface |
|-----------|--------|-----------|
| Implements | `plugins/image_analysis_interface.h` | `IImageAnalysisBackend` |
| Consumes | application or search pipeline | embedding vectors and statistics |
| Registered via | `THEMIS_IMAGE_PLUGIN` macro | dynamic plugin loader |

---

## 7. Threading and concurrency

- Request-level serialization is enforced for all primary embedding APIs through the plugin mutex and per-request guard patterns.
- `generateEmbeddingBatch()` processes chunks of `max_batch_size`; larger inputs are split rather than processed as a single unbounded operation.
- `reloadModel()` uses an in-flight request counter and a drain window so active requests complete on the old snapshot before the new one is published.
- `enable_mmap_loading` is a best-effort optimization and is not treated as a required production dependency.

---

## 8. Error handling

| Scenario | Behaviour |
|----------|-----------|
| invalid config | `initialize()` / `reloadModel()` fails before activation |
| empty image payload | returns `EmbeddingResult{success=false}` with a specific message |
| empty text payload | returns `EmbeddingResult{success=false}` with a specific message |
| model hash mismatch | initialization or reload is rejected when `model.path` and `model.expected_sha256` are both configured |
| plugin not initialized | public API calls fail closed with a structured error result |
| mmap not available | ignored gracefully and normal loading path continues |

---

## 9. Implementation status and follow-up work

### Active source-aligned scope

- [x] `ONNXClipPlugin` public API and lifecycle contract
- [x] config validation, backend resolution, and bounded batch splitting
- [x] text embedding path and deterministic stats collection
- [x] optional model-hash verification bridge
- [x] hot-swap model reload with drain semantics
- [x] mmap best-effort support for supported platforms
- [x] focused tests covering hardening, batching, and plugin behavior

### Remaining follow-up items

- [ ] Validate additional real-world model fixtures when test assets are available
- [ ] Extend runtime benchmarks only where there is measurable production value
- [ ] Keep README, ROADMAP, and security docs synchronized with any future provider-specific expansion

---

## 10. Source evidence used for this contract

- `src/onnx_clip/onnx_clip_plugin.h`
- `src/onnx_clip/onnx_clip_plugin.cpp`
- `tests/onnx_clip/` focused coverage
- `src/onnx_clip/README.md`
- `src/onnx_clip/PRODUCTION_REQUIREMENTS.md`

- **Release (request end):** Allows reloadModel's wait to observe the decrement correctly
- **Timeout-based wait:** Uses `condition_variable::wait_until()` with 30-second deadline

**Drain Algorithm:**
```cpp
// Acquire lock
std::unique_lock<std::mutex> lock(impl_->mutex);

// Wait up to 30 seconds for all requests to complete
auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
bool drain_success = impl_->cv_drain_complete.wait_until(
   lock,
   deadline,
   [this]() { return impl_->in_flight_requests_.load(std::memory_order_acquire) == 0; }
);

// If timeout: return false (old model remains active)
if (!drain_success) return false;

// Otherwise: perform atomic swap
impl_ = std::move(new_impl);  // Old impl destroyed; new impl becomes active
```

**Configuration:**
```json
{
  "model": {
   "name": "clip-vit-large-patch14",
   "embedding_dim": 768,
   "path": "/models/clip-vit-l-14.onnx",
   "expected_sha256": "abc123..."
  },
  "backend": "cuda",
  "max_batch_size": 64
}
```

**Test Coverage (OCP-HS-01..12):**

| Test | Category | Validates |
|------|----------|-----------|
| OCP-HS-01..04 | Basic Scenarios | Reload success, state transitions, sequential reloads, health checks |
| OCP-HS-05..08 | Request Draining | Counter tracking, drain waits for requests, timeout prevention, no request loss |
| OCP-HS-09..12 | Concurrency | Concurrent inference + reload, embedding validity (before/after), race-free operation |

**Performance:**
- Per-request overhead: ~1-2 ns (atomic ops only)
- Idle reload: microseconds
- Under load: depends on in-flight request latency
- Timeout enforcement: 30 seconds maximum
- All 12 tests complete in ~100-150 ms total

---

### 9.2 Memory-Mapped Model Loading (Phase 4)

**New Config Key:** `enable_mmap_loading` (boolean, default: `false`)

**Implementation Strategy:**
```
Traditional Load:                Memory-Mapped Load:
File → Read into heap (copy)     File → mmap() view → ONNX Session
 ↓                                ↓
Peak memory: full model size     Peak memory: metadata only
 ↓                                ↓
Runtime: models are in RAM       Runtime: lazy page faults
                                           (but still in RAM once used)
```

**Platform Support:**
- **Linux:** `mmap(fd, MAP_SHARED | MAP_NORESERVE)` for read-only access
- **Windows:** `CreateFileMapping()` + `MapViewOfFile(PAGE_READONLY)`
- **macOS:** BSD `mmap()` variant
- **Fallback:** Traditional heap loading on unsupported platforms

**Memory Savings (Measured in Phase 4C Tests):**

Test results from Phase 4C (OCP-MM-09..12) using mock models:
- **ViT-B/32 simulation (10 MB):**
  - RSS measurement available via `/proc/self/status` (Linux)
  - Mmap'd loading shows measurable memory efficiency
  - Fallback mechanism verified on unsupported platforms

- **ViT-L/14 simulation (50 MB):**
  - Large model shows greater memory benefit from mmap
  - RSS tracking works across batch operations
  - Memory remains bounded during concurrent inference

**Test Coverage:**
- OCP-MM-01..04: Initialization success/fallback/error handling
- OCP-MM-05..08: Correctness verification (embeddings identical to traditional)
- OCP-MM-09..12: Memory footprint tracking and concurrent safety
- Platform coverage: Linux (primary), Windows/macOS fallback verified

**Key Test Achievements:**
- All 12 tests pass in ~2.5 seconds (sub-timeout execution)
- Concurrent threads (4 concurrent) produce correct embeddings
- Batch inference (8-batch) maintains correctness with mmap
- Text embedding generation works correctly with mmap'd models
- No resource leaks (file descriptors, memory) detected

**Lifecycle:**
```cpp
// In ONNXClipPlugin::Impl
void* mmap_ptr_{nullptr};      // Mapped region pointer
size_t mmap_size_{0};          // Mapped size
int mmap_fd_{-1};              // Linux file descriptor
HANDLE mmap_file_handle_;      // Windows handle

~Impl() {
    // Unmap and close file handles
    if (mmap_ptr_) munmap(mmap_ptr_, mmap_size_);  // Linux
    // or UnmapViewOfFile(mmap_ptr_);  // Windows
}
```

**Configuration:**
```json
{
  "model": {
    "enable_mmap_loading": true,
    "path": "/models/clip-vit-large-patch14.onnx"
  }
}
```

---

## 10. Known Limitations & Future Work

### Current (v0.2.0)
- `generateEmbeddingBatch()` is implemented as sequential single calls; native
  batched ONNX session execution is planned for Phase 5 (post-Q1 2027).
- DirectML backend requires Windows; on Linux `BackendType::DirectML` falls back to CPU.

### Planned (v0.3.0)
- Dynamic model hot-swap (Phase 3): Reload models without server restart
- Memory-mapped model loading (Phase 4): Reduce peak memory for large models
- Native batched inference (Phase 5, optional): True batched ONNX session calls

## Module Dependencies

### Direct Upstream Dependencies (this module uses)
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| ONNX Runtime | external (onnxruntime) | Provides cross-platform model inference via ONNX sessions |
| CUDA / DirectML / TensorRT | external acceleration backends | Optional GPU-accelerated inference; falls back to CPU if unavailable |

### Direct Downstream Consumers (modules that use this module)
| Module | Via | Notes |
|--------|-----|-------|
| server | `include/onnx_clip/` | Server uses ONNX CLIP for image embedding and visual analysis APIs |
| llm | `include/onnx_clip/` | LLM vision pipeline uses CLIP embeddings for multimodal context |

## Integration Points

### Critical Integration: Server Image Analysis (expanded)
**Files:** `src/onnx_clip/onnx_clip_plugin.cpp` ↔ `server/`
**Contract:** Server calls `generateEmbedding()` / `generateEmbeddingBatch()` via the plugin API; batch is currently sequential single calls (native batching planned for Phase 5).
**Thread Safety:** `OnnxClipPlugin` is thread-safe for all public methods via internal mutex.

### Critical Integration: LLM Vision Pipeline (expanded)
**Files:** `src/onnx_clip/` ↔ `llm/`
**Contract:** LLM module requests CLIP embeddings for image inputs before multimodal inference; embedding objects are immutable after return.
**Thread Safety:** Embedding generation is serialised internally; LLM may call concurrently and calls will be queued.

### Critical Integration: ONNX Runtime Backend Selection
**Files:** `src/onnx_clip/onnx_clip_plugin.cpp` ↔ ONNX Runtime external
**Contract:** Backend selection (CUDA → TensorRT → DirectML → CPU) is determined at plugin initialisation from `BackendType` config; DirectML falls back to CPU on non-Windows.
**Thread Safety:** ONNX session is not thread-safe; all calls are serialised through the plugin's internal mutex.
