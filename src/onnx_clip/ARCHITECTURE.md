> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# ONNX CLIP Plugin — Architecture Guide

**Version:** 0.0.1
**Last Updated:** 2026-04-06
**Module Path:** `src/onnx_clip/`

---

## 1. Overview

The ONNX CLIP plugin wraps OpenAI CLIP models exported to ONNX format using the
ONNX Runtime C++ API. It implements `IImageAnalysisBackend` and exposes a simple
embedding generation API that ThemisDB uses for multi-modal vector similarity search.

The implementation uses the pImpl idiom (`struct Impl` hidden in the `.cpp`) to
keep `Ort::Session`, preprocessing state, and runtime objects completely out of the
public header. This prevents ABI leakage of ONNX Runtime types into caller translation
units.

---

## 2. Design Principles

- **pImpl isolation** — all ONNX Runtime objects (`Ort::Env`, `Ort::Session`,
  `Ort::SessionOptions`) live in `ONNXClipPlugin::Impl`; the header exposes only
  standard types.
- **Thread safety** — `impl_` is protected by a `std::mutex`; `generateEmbedding()`
  and `generateEmbeddingBatch()` serialize access to the ONNX session.
- **Backend AUTO** — at `initialize()` time with `BackendType::AUTO`, the plugin
  probes CUDA availability first, then TensorRT, DirectML, and finally CPU.
- **Warmup** — `warmup()` runs a single inference with a synthetic input to pre-compile
  CUDA/TensorRT kernels before serving live traffic.

---

## 3. Component Architecture

### 3.1 Component Diagram

```
┌──────────────────────────────────────────────────────┐
│               ONNXClipPlugin (public API)            │
│  implements IImageAnalysisBackend                    │
│                                                      │
│  initialize(config, backend) ─ load ONNX model      │
│  generateEmbedding(image_data) ─ single inference   │
│  generateEmbeddingBatch(images) ─ batch inference   │
│  healthCheck()                                       │
│  warmup()                                            │
│  getStatistics()                                     │
└──────────────────────┬───────────────────────────────┘
                       │ std::unique_ptr<Impl>
                       ▼
┌──────────────────────────────────────────────────────┐
│               ONNXClipPlugin::Impl (pImpl)           │
│                                                      │
│  Ort::Env            ─ ONNX Runtime environment     │
│  Ort::Session        ─ loaded CLIP model             │
│  Ort::SessionOptions ─ provider / thread config     │
│  std::mutex          ─ serialises inference calls   │
│  BackendType backend_                                │
│  std::string model_variant_                          │
│  call_count, total_latency_ms (stats)               │
└──────────────────────┬───────────────────────────────┘
                       │
          ┌────────────▼────────────┐
          │  ONNX Runtime C++ API   │
          │  ├─ CPU Execution Prov. │
          │  ├─ CUDA Execution Prov.│
          │  ├─ DirectML Exec. Prov.│
          │  └─ TensorRT Exec. Prov.│
          └─────────────────────────┘
```

### 3.2 Interface Implementation Table

| Method | Behaviour |
|--------|-----------|
| `getInfo()` | Returns `PluginInfo{name="onnx_clip", version="0.0.1", ...}` |
| `initialize(config, backend)` | Creates `Ort::Session`, configures execution provider |
| `shutdown()` | Releases `Ort::Session`; resets stats |
| `isReady()` | Returns `true` if session is loaded and not null |
| `getBackend()` | Returns active `BackendType` |
| `generateEmbedding(image_data, metadata)` | Decodes image → preprocess → infer → return float vector |
| `generateEmbeddingBatch(images)` | Iterates single calls; future: native batched session |
| `healthCheck()` | Runs warmup inference; checks output tensor shape |
| `getStatistics()` | Returns JSON: `{calls, avg_latency_ms, backend, model_variant}` |
| `warmup()` | Runs one inference with a 224×224 zero tensor |

---

## 4. Inference Pipeline

```
image_data (raw bytes)
  │
  ├─ Decode (JPEG / PNG / BMP via OpenCV / stb_image)
  │
  ├─ Resize to 224×224
  │
  ├─ Normalise: subtract ImageNet mean, divide by std
  │     mean = [0.48145466, 0.4578275, 0.40821073]
  │     std  = [0.26862954, 0.26130258, 0.27577711]
  │
  ├─ CHW float32 tensor [1, 3, 224, 224]
  │
  ├─ Ort::Session::Run(input_tensor)
  │
  └─ Output tensor [1, 512] (ViT-B/32) or [1, 768] (ViT-L/14)
        → L2 normalise → std::vector<float>
```

---

## 5. Backend Selection (AUTO)

```
BackendType::AUTO:
  1. Check CUDA device count → if > 0 → CUDA
  2. Check TensorRT availability → if available → TensorRT
  3. Check DirectML (Windows only) → if available → DirectML
  4. Fallback → CPU
```

---

## 6. Integration Points

| Direction | Module | Interface |
|-----------|--------|-----------|
| **Implements** | `plugins/image_analysis_interface.h` | `IImageAnalysisBackend` |
| **Provides to** | `src/server/` vector search handlers | Embedding vectors |
| **Registered via** | `THEMIS_IMAGE_PLUGIN` macro | Dynamic plugin loader |

---

## 7. Threading & Concurrency

- `Ort::Session::Run()` is not thread-safe by default; access serialised via
  `std::mutex` in `Impl`.
- `generateEmbeddingBatch()` holds the lock for the entire batch; consider splitting
  batch into sub-batches for large inputs (planned for v0.1.0).
- `isReady()` and `getBackend()` are lock-free reads of atomic/const members.

---

## 8. Error Handling

| Scenario | Behaviour |
|----------|-----------|
| ONNX model file not found | `initialize()` returns `false`; logs error |
| Image decode failure | `generateEmbedding()` returns `EmbeddingResult{ok=false, error=...}` |
| CUDA not available (CUDA backend) | `initialize()` returns `false` |
| Session Run exception | Caught; `EmbeddingResult{ok=false}` returned |
| Output tensor wrong shape | `healthCheck()` returns `false` |

---

## 9. v0.3.0 Enhancements (In Progress)

### 9.1 Dynamic Model Hot-Swap (Phase 3)

**New Method:** `bool reloadModel(const PluginConfig& new_config)`

**State Machine:**
```
[Ready] ─── reloadModel() ──→ [Loading] ──→ [Validation] ──→ [Activation]
   ↑                                                              ↓
   └──────────────────────────────────────────────────────────[Ready]
   
   On failure: [Loading] → [Error] → (restore old) → [Ready with old]
```

**Key Features:**
- In-flight requests complete with old model before swap
- 30-second timeout for graceful drain of pending requests
- Atomic swap: old model destroyed only after new one ready
- Exception-safe: RAII guards for in-flight counter
- Automatic rollback on new model load failure

**Concurrency Model:**
```cpp
impl_->in_flight_requests_++;  // Track concurrent calls
// ... perform inference ...
impl_->in_flight_requests_--;  // RAII guard ensures decrement
```

**Configuration:**
```json
{
  "model": {
    "name": "clip-vit-large-patch14",
    "path": "/models/clip-vit-l-14.onnx"
  }
}
```

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

**Memory Savings (Estimated):**
- ViT-B/32 (83 MB): ~10-15% reduction (small overhead)
- ViT-L/14 (768 MB): ~30-40% reduction (large model, heap fragmentation saved)

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
