# ThemisDB LLM Module

<!-- Status: enhanced | validated: 2026-08-17 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · THREADING.md · OPERATIONS.md · API_REFERENCE.md · CONFIGURATION.md -->

**Module Purpose:** Production-grade inference runtime, routing, model/adapter lifecycle management, and LLM-oriented orchestration surfaces used by ThemisDB AI features.

**Status:** PRODUCTION (Wave 5 GA) | **Thread-Safe:** Yes | **Fail-Closed:** Yes

Model baseline selection policy (scoring + hard gates) is maintained in `src/llm/VALIDATION_AND_TEST_CHECKLIST.md` under "Baseline Model Evaluation Matrix (Release Candidate)".

---

## Quick Start Guide

### 1. Basic Inference (Embedded LLM)

```cpp
#include <llm/embedded_llm.h>
using namespace themisdb::llm;

// Initialize embedded LLM with default settings
EmbeddedLLM llm;
llm.initialize();

// Load a model
auto status = llm.loadModel("path/to/model.gguf", LoadConfig{
    .gpu_memory_fraction = 0.8f,
    .fallback_to_cpu = true
});
if (!status.ok()) {
    LOG(ERROR) << "Failed to load model: " << status.message();
    return;
}

// Run inference
std::string prompt = "What is the capital of France?";
auto result = llm.complete(prompt, CompletionConfig{
    .max_tokens = 512,
    .temperature = 0.7f,
    .timeout_ms = 30000
});

if (result.ok()) {
    std::cout << "Response: " << result.value().text << std::endl;
} else {
    LOG(ERROR) << "Inference failed: " << result.status().message();
}

// Cleanup
llm.shutdown();
```

### 2. Streaming Inference

```cpp
// Request with streaming callback
llm.streamComplete(prompt, CompletionConfig{
    .max_tokens = 512
}, [](const StreamToken& token) {
    // Called for each token
    std::cout << token.text;
    std::cout.flush();
});
```

### 3. Model Routing

```cpp
// Define routing rules
ModelRouter router;
router.addRoute(ModelRoute{
    .condition = "prompt.length > 1000",
    .target_model = "large-model",
    .fallback = "medium-model"
});

// Query with routing
auto routed_model = router.selectModel(prompt);
auto result = llm.complete(prompt, CompletionConfig{
    .model = routed_model
});
```

### 4. LoRA Adapter Management

```cpp
// Load base model
llm.loadModel("base-model.gguf");

// Load and apply LoRA adapter
MultiLoraManager lora_mgr;
auto adapter_id = lora_mgr.loadAdapter(
    "finetuned-adapter.lora",
    AdapterConfig{.merge_into_model = false}
);

// Run inference with adapter
auto result = llm.complete(prompt, CompletionConfig{
    .active_adapters = {adapter_id}
});

// Hot-swap adapters (no model reload)
auto new_adapter_id = lora_mgr.switchAdapter(adapter_id, "other-adapter.lora");
```

---

## Relevant Interfaces

| Interface / File | Role | Key Operations |
|---|---|---|
| **`async_inference_engine.cpp`** | Asynchronous inference submission and completion handling | Submit, poll, cancel requests |
| **`inference_engine_enhanced.cpp`** | Multi-model orchestration and enhanced runtime controls | Multi-model routing, latency tracking |
| **`shared_worker_pool.cpp`** | Shared worker scheduling for LLM execution paths | Work dispatch, thread pool management |
| **`model_router.cpp`** | Rule-based model routing and selection | Route conditions, failover logic |
| **`llm_plugin_manager.cpp`** | Plugin/backend lifecycle control | Load, unload, switch backends |
| **`multi_lora_manager.cpp`** | LoRA adapter load/switch/unload lifecycle | Load, hot-swap, unload adapters |
| **`streaming_handler.cpp`** | Streaming token framing and callback paths | Token buffering, stream callbacks |
| **`prompt_policy.cpp`** | Prompt safety/policy enforcement helpers | Validation, filtering, guardrails |
| **`token_quota_manager.cpp`** | Per-model/per-request quota enforcement | Token budgets, rate limiting |
| **`production_validator.cpp`** | Runtime validation and production-safety checks | Safety gates, pre-flight checks |

---

## API Overview

### Core Inference APIs

#### `EmbeddedLLM` (Primary Interface)

```cpp
class EmbeddedLLM {
public:
    // Lifecycle
    void initialize();
    void shutdown();
    
    // Model Management
    Status loadModel(const std::string& path, const LoadConfig& cfg);
    Status unloadModel(const std::string& model_name);
    std::vector<std::string> listLoadedModels() const;
    
    // Synchronous Inference
    Result<CompletionResponse> complete(
        const std::string& prompt,
        const CompletionConfig& cfg
    );
    
    // Asynchronous Inference
    RequestHandle submitComplete(
        const std::string& prompt,
        const CompletionConfig& cfg,
        std::function<void(const Result<CompletionResponse>&)> callback
    );
    
    // Streaming Inference
    void streamComplete(
        const std::string& prompt,
        const CompletionConfig& cfg,
        std::function<void(const StreamToken&)> on_token
    );
    
    // Embeddings
    Result<std::vector<float>> embed(
        const std::string& text,
        const EmbedConfig& cfg
    );
};
```

#### Configuration Types

```cpp
struct CompletionConfig {
    std::string model;              // Model to use (default: first loaded)
    int32_t max_tokens = 512;       // Max output tokens
    float temperature = 0.7f;       // Sampling temperature
    float top_p = 0.9f;             // Nucleus sampling parameter
    int32_t top_k = 40;             // Top-k sampling
    int64_t timeout_ms = 300000;    // Request timeout (5 min default)
    std::vector<std::string> active_adapters;  // LoRA adapters to apply
    bool use_cache = true;          // Use KV cache
    std::string system_prompt;      // System context
};

struct LoadConfig {
    float gpu_memory_fraction = 0.8f;  // GPU memory allocation
    bool fallback_to_cpu = true;       // Allow CPU fallback
    int32_t num_threads = -1;          // Worker threads (-1 = auto)
    int32_t batch_size = 1024;         // Batching window (tokens)
};
```

### Policy & Safety APIs

```cpp
// Prompt policy enforcement
struct PromptPolicy {
    static Status validatePrompt(
        const std::string& prompt,
        const PolicyConfig& cfg
    );
    
    static std::string filterPrompt(
        const std::string& prompt,
        const FilterConfig& cfg
    );
};

// Production validator
struct ProductionValidator {
    static Status preFlightCheck(
        const CompletionConfig& cfg,
        const SystemState& state
    );
    
    static Status postFlightValidation(
        const CompletionResponse& response
    );
};
```

### Resource Management APIs

```cpp
// Token quota enforcement
class TokenQuotaManager {
    Status allocate(const std::string& model, int32_t tokens);
    void release(const std::string& model, int32_t tokens);
    Status checkQuota(const std::string& model, int32_t required_tokens);
};

// Response caching
class LLMResponseCache {
    Result<CompletionResponse> lookup(const std::string& cache_key);
    void store(const std::string& cache_key, const CompletionResponse& resp);
    void evictLRU();
};

// GPU memory management
class GPUMemoryManager {
    Result<GPUBuffer> allocate(size_t bytes);
    void deallocate(GPUBuffer buffer);
    Status defragment();
    GPUMemoryStats getStats();
};
```

---

## Scope

**In Scope:**
- ✅ Inference runtime and orchestration
- ✅ Model and adapter lifecycle operations
- ✅ Routing and scheduling for LLM execution
- ✅ Streaming output and prompt/policy controls
- ✅ Runtime safety, quota, and observability helpers
- ✅ GPU memory management and acceleration

**Out of Scope:**
- ❌ Persistence internals and storage engine behavior (see `storage/` module)
- ❌ HTTP gateway implementation details outside LLM runtime adapters
- ❌ Non-LLM domain modules unrelated to inference/orchestration
- ❌ Third-party backend implementation (llama.cpp, VLLM, etc. are pluggable)

---

## Configuration Guide

### Environment Variables

```bash
# GPU Configuration
export THEMIS_LLM_GPU_MEMORY_FRACTION=0.8        # GPU memory allocation %
export THEMIS_LLM_FALLBACK_TO_CPU=true            # Allow CPU fallback
export THEMIS_LLM_DEVICE_ID=0                     # GPU device index

# Inference Configuration
export THEMIS_LLM_MAX_BATCH_SIZE=1024            # Token batching window
export THEMIS_LLM_NUM_WORKER_THREADS=-1          # Worker threads (-1=auto)
export THEMIS_LLM_DEFAULT_TIMEOUT_MS=300000      # Default timeout (5 min)

# Cache Configuration
export THEMIS_LLM_RESPONSE_CACHE_SIZE=1GB        # Response cache max size
export THEMIS_LLM_KV_CACHE_PAGES=8192            # KV cache page count
export THEMIS_LLM_CACHE_EVICTION=lru              # Eviction policy

# Model Management
export THEMIS_LLM_MODEL_PATH=/path/to/models     # Model directory
export THEMIS_LLM_PRELOAD_MODELS="model1,model2" # Auto-load on init
export THEMIS_LLM_MODEL_QUOTA_MB=32000           # Max total model size

# Logging & Observability
export THEMIS_LLM_LOG_LEVEL=INFO                 # Logging level
export THEMIS_LLM_ENABLE_METRICS=true            # Prometheus metrics
export THEMIS_LLM_ENABLE_TRACING=true            # Distributed tracing
```

### Programmatic Configuration

```cpp
// Via initialization file
ConfigLoader loader("llm_config.yaml");
auto config = loader.loadConfig();
llm.initialize(config);

// Example config.yaml
inference:
  max_batch_size: 1024
  num_workers: 8
  default_timeout_ms: 300000

gpu:
  enabled: true
  memory_fraction: 0.8
  fallback_to_cpu: true

cache:
  response_cache_size_mb: 1024
  kv_cache_pages: 8192
  eviction_policy: lru

models:
  default_model: "llama2-7b"
  preload:
    - "llama2-7b"
    - "mistral-7b"
```

---

## Thread-Safety Model (Tier 1 Requirement)

**Concurrency Guarantees:**
- **Query API:** Thread-safe for concurrent queries on distinct models via `EmbeddedLLM` thread-local isolation
- **Model Loading:** Single writer, multiple readers protected by `model_load_mutex_` (read-write lock)
- **Adapter Management:** LoRA adapter load/switch/unload protected by `adapter_lifecycle_mutex_`
- **Embedding Cache:** Atomic operations on `query_embed_cache_` (concurrent readers + persistent RocksDB backend)
- **Streaming Output:** Per-session callback isolation; no shared state between concurrent streams
- **Plugin Lifecycle:** Thread-safe registration via `plugin_manager_mutex_`

**Known Thread-Safety Invariants:**
- Multi-model query paths isolate state via separate `EmbeddedLLM*` instances per model
- Cache eviction under concurrent reads is protected by `query_embed_mutex_` and `atomic<dim_probed_>`
- Adapter hot-swap races prevented by `SubagentLifecycleManager` state machine (Phase B delivered)

**For Detailed Contracts:** See [THREADING.md](THREADING.md)

---

## Fail-Closed Behavior & Error Handling

**Design Principle:** Safe degradation and explicit error signaling.

| Scenario | Response | Recovery |
|---|---|---|
| **Policy Violation** | Reject immediately with `Status::kPolicyViolation` | User must modify prompt |
| **Model Load Failure** | Return `kModelLoadFailed`; do not proceed to inference | Fallback to alternative model or fail |
| **VRAM Exhaustion** | Trigger model eviction; retry; backpressure if persistent | Queue request or reject with `kOutOfMemory` |
| **Stream Abort** | Signal cancellation; cleanup in-flight tokens; return `kCancelled` | Caller should retry or fail gracefully |
| **Request Timeout** | Enforce per-request timeout via `CancellationToken`; return `kDeadlineExceeded` | Caller must handle deadline and retry with exponential backoff |
| **Backend Unavailable** | Fall back to CPU inference path; fail fast if CPU unavailable | Return `kUnavailable` with retry hint in response metadata |
| **Plugin Load Failure** | Skip plugin; try next available backend; log error, do not crash | Graceful degradation to next viable backend |

**For Operational Runbooks:** See [OPERATIONS.md](OPERATIONS.md)

---

## Quick Troubleshooting

### "Model Load Failed"
- **Cause:** Model file not found, corrupted, or incompatible format
- **Solution:** Verify file path, check file integrity, ensure GGUF format compatibility
- **See:** [OPERATIONS.md § Model Loading](OPERATIONS.md#model-loading-runbook)

### "CUDA Out of Memory"
- **Cause:** GPU memory exhausted by model or inference
- **Solution:** Reduce model size, increase GPU memory quota, enable CPU fallback
- **See:** [CONFIGURATION.md § GPU Memory Tuning](CONFIGURATION.md#gpu-memory-tuning)

### "Inference Timeout"
- **Cause:** Request exceeded configured timeout (default 5 min)
- **Solution:** Increase timeout, reduce batch size, use faster model
- **See:** [OPERATIONS.md § Performance Tuning](OPERATIONS.md#performance-tuning)

### "Thread Safety Warnings"
- **Cause:** Concurrent access without synchronization
- **Solution:** Use separate `EmbeddedLLM` instance per thread or add explicit synchronization
- **See:** [THREADING.md § Concurrency Patterns](THREADING.md#concurrency-patterns)

### "Streaming Callbacks Not Firing"
- **Cause:** Callback not registered or stream cancelled
- **Solution:** Register callback before submission; check cancellation token
- **See:** [API_REFERENCE.md § Streaming](API_REFERENCE.md#streaming-inference)

---

## Known Limitations

### Current

- ✓ Some advanced distributed/federated paths require deployment wiring (not default-on)
- ✓ Runtime behavior can vary by selected backend and available acceleration stack
- ✓ Benchmark coverage is broad but still evolving for all cross-node production scenarios
- ✓ Multi-GPU coordination via basic `multi_gpu_memory_coordinator.cpp` (advanced NCCL in Wave C)

### Planned Enhancements (Wave C)

- ☐ Speculative decoding integration (~2x speedup)
- ☐ Distributed end-to-end inference optimization
- ☐ Advanced GPU memory fragmentation recovery
- ☐ Persistent embedding cache (RocksDB integration)

---

## Sourcecode Verification (Module: llm/readme — Phase 6)

**Verified Files:**
- ✅ `src/llm/async_inference_engine.cpp`
- ✅ `src/llm/inference_engine_enhanced.cpp`
- ✅ `src/llm/shared_worker_pool.cpp`
- ✅ `src/llm/model_router.cpp`
- ✅ `src/llm/llm_plugin_manager.cpp`
- ✅ `src/llm/multi_lora_manager.cpp`
- ✅ `src/llm/streaming_handler.cpp`
- ✅ `src/llm/prompt_policy.cpp`
- ✅ `src/llm/token_quota_manager.cpp`
- ✅ `src/llm/production_validator.cpp`

**Verified Behavior Surfaces:**
- ✅ Request submission, scheduling, routing, and streaming
- ✅ Plugin/adapter lifecycle behavior
- ✅ Policy/quota/validation control surfaces
- ✅ Thread-safety guarantees (Phase 6: documented in THREADING.md)
- ✅ Fail-closed error handling patterns (Phase 6: documented in OPERATIONS.md)

**Quality Gates (Phase 6):**
| Gate | Status | Evidence |
|---|---|---|
| **API Documentation** | ✅ COMPLETE | API_REFERENCE.md with examples and contracts |
| **Configuration Guide** | ✅ COMPLETE | CONFIGURATION.md with all options and tuning |
| **Operations Runbooks** | ✅ COMPLETE | OPERATIONS.md with debugging and recovery procedures |
| **Thread-Safety** | ✅ COMPLETE | THREADING.md with detailed synchronization model |
| **Quick Start** | ✅ COMPLETE | This README with working code examples |
| **Developer Guide** | ✅ COMPLETE | DEVELOPER_GUIDE.md for contributors |

---

## Related Documentation

| Document | Purpose |
|---|---|
| [ARCHITECTURE.md](ARCHITECTURE.md) | System design, concurrency model, resource management, integration boundaries |
| [THREADING.md](THREADING.md) | Detailed thread-safety contracts and synchronization primitives |
| [OPERATIONS.md](OPERATIONS.md) | Operational runbooks, debugging guide, performance tuning, error recovery |
| [API_REFERENCE.md](API_REFERENCE.md) | Complete API documentation, usage patterns, code examples |
| [CONFIGURATION.md](CONFIGURATION.md) | Configuration options, environment variables, tuning guide |
| [DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md) | Contributing guide, testing strategy, build procedures |
| [ROADMAP.md](ROADMAP.md) | Feature pipeline, release planning, milestones |
| [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md) | Planned enhancements and Wave C work |
| [CHANGELOG.md](CHANGELOG.md) | Historical implementation record and version history |

---

## Installation & Build

This module is built as part of ThemisDB.

**Build LLM Module Only:**
```bash
cd ThemisDB
cmake --preset windows-release  # or your preset
cmake --build --preset windows-release --target llm --parallel 16
```

**Run LLM Tests:**
```bash
ctest --preset windows-release -R llm -V
```

**Full Details:** See [DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md)

---

**Last Updated:** 2026-08-17 (Phase 6 Documentation Enhancement)
**Status:** PRODUCTION (Wave 5 GA) | **Maintainer:** LLM Module Team
