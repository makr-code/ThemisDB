# LLM Module - API Reference

<!-- Status: complete | validated: 2026-08-17 -->
<!-- Links: README.md · ARCHITECTURE.md · THREADING.md -->

Version: 1.0 (Phase 6)
Last Updated: 2026-08-17
Module Path: src/llm/ & include/llm/

---

## Core Interfaces

### EmbeddedLLM (Primary Interface)

**Header:** `include/llm/embedded_llm.h`

**Purpose:** Main inference engine providing synchronous, asynchronous, and streaming inference.

#### Lifecycle Methods

```cpp
class EmbeddedLLM {
public:
    /// @brief Initialize the LLM module with default or custom configuration.
    /// @param config Optional configuration; uses environment/file if not provided.
    /// @return Status::kOk on success; error status otherwise.
    Status initialize(const std::optional<LLMConfig>& config = std::nullopt);
    
    /// @brief Gracefully shutdown the module.
    /// - Cancels pending requests
    /// - Unloads all models
    /// - Deallocates GPU memory
    /// - Flushes caches
    Status shutdown();
    
    /// @brief Check if module is initialized.
    bool isInitialized() const;
};
```

#### Model Management Methods

```cpp
/// @brief Load a model from file.
/// @param model_path Path to GGUF model file.
/// @param config Load configuration (GPU fraction, threads, etc.).
/// @return Status::kOk on success.
/// @throws Status::kNotFound if file doesn't exist.
/// @throws Status::kOutOfMemory if insufficient GPU/CPU memory.
Status loadModel(
    const std::string& model_path,
    const LoadConfig& config = LoadConfig{}
);

/// @brief Load model asynchronously.
/// @param model_path Path to GGUF model file.
/// @param config Load configuration.
/// @param callback Invoked when load completes or fails.
/// @return Handle for monitoring load progress.
LoadHandle loadModelAsync(
    const std::string& model_path,
    const LoadConfig& config,
    std::function<void(Status)> callback
);

/// @brief Unload a model, freeing GPU/CPU memory.
/// @param model_name Name/path of model to unload.
/// @return Status::kOk on success.
/// @note Waits for active queries to complete before unloading.
Status unloadModel(const std::string& model_name);

/// @brief Hot-swap to a different model without reloading adapters.
/// @param model_name Name of currently loaded model.
/// @param new_model_path Path to new model.
/// @return Status::kOk on success.
Status switchModel(
    const std::string& model_name,
    const std::string& new_model_path
);

/// @brief List all currently loaded models.
std::vector<ModelInfo> listLoadedModels() const;

/// @brief Get memory usage for a loaded model.
Result<ModelMemoryStats> getModelMemoryStats(
    const std::string& model_name
) const;
```

#### Synchronous Inference

```cpp
/// @brief Execute inference synchronously (blocks until complete).
/// @param prompt Input prompt/query.
/// @param config Inference configuration (timeout, batch size, etc.).
/// @return CompletionResponse with generated text.
/// @throws Status::kDeadlineExceeded if exceeds timeout.
/// @throws Status::kPolicyViolation if prompt violates safety policy.
Result<CompletionResponse> complete(
    const std::string& prompt,
    const CompletionConfig& config = CompletionConfig{}
);

/// @brief Execute inference with structured output (JSON schema).
/// @param prompt Input prompt.
/// @param schema JSON schema for output validation.
/// @param config Inference configuration.
/// @return CompletionResponse with JSON output matching schema.
Result<CompletionResponse> completeWithSchema(
    const std::string& prompt,
    const std::string& json_schema,
    const CompletionConfig& config = CompletionConfig{}
);
```

#### Asynchronous Inference

```cpp
/// @brief Submit inference request asynchronously.
/// @param prompt Input prompt.
/// @param config Inference configuration.
/// @return Handle for polling/cancelling the request.
RequestHandle submit(
    const std::string& prompt,
    const CompletionConfig& config = CompletionConfig{}
);

/// @brief Poll result of asynchronous inference request.
/// @param handle Request handle from submit().
/// @param block_until_ready If true, blocks until ready; else non-blocking.
/// @return Result if ready; nullopt if still executing.
std::optional<Result<CompletionResponse>> poll(
    RequestHandle handle,
    bool block_until_ready = false
);

/// @brief Cancel an in-flight inference request.
/// @param handle Request handle from submit().
/// @return Status::kOk on success.
Status cancel(RequestHandle handle);

/// @brief Wait for request completion with timeout.
/// @param handle Request handle.
/// @param timeout_ms Maximum time to wait in milliseconds.
/// @return Result if completed; error if timeout.
Result<CompletionResponse> wait(
    RequestHandle handle,
    int64_t timeout_ms = 300000
);
```

#### Streaming Inference

```cpp
/// @brief Stream tokens as they're generated.
/// @param prompt Input prompt.
/// @param config Inference configuration.
/// @param on_token Callback invoked for each token.
/// @return Status::kOk on success.
/// @note Callbacks run on worker thread; caller responsible for thread safety.
Status streamComplete(
    const std::string& prompt,
    const CompletionConfig& config,
    std::function<void(const StreamToken&)> on_token
);

/// @brief Stream with error handling callback.
/// @param prompt Input prompt.
/// @param config Inference configuration.
/// @param on_token Token callback.
/// @param on_error Error callback (invoked if inference fails).
/// @param on_complete Completion callback (invoked when stream ends).
Status streamCompleteWithCallbacks(
    const std::string& prompt,
    const CompletionConfig& config,
    std::function<void(const StreamToken&)> on_token,
    std::function<void(Status)> on_error,
    std::function<void()> on_complete
);
```

#### Embedding & RAG

```cpp
/// @brief Generate embedding for text.
/// @param text Input text.
/// @param config Embedding configuration.
/// @return Vector of embeddings.
Result<std::vector<float>> embed(
    const std::string& text,
    const EmbedConfig& config = EmbedConfig{}
);

/// @brief Generate embeddings for batch of texts.
/// @param texts Vector of input texts.
/// @param config Embedding configuration.
/// @return Vector of embedding vectors.
Result<std::vector<std::vector<float>>> embedBatch(
    const std::vector<std::string>& texts,
    const EmbedConfig& config = EmbedConfig{}
);
```

#### Query & Monitoring

```cpp
/// @brief Check if GPU is available and healthy.
bool isGPUHealthy() const;

/// @brief Get current system state (GPU memory, CPU load, etc.).
SystemState getSystemState() const;

/// @brief Get memory usage statistics.
MemoryStats getMemoryStats() const;

/// @brief Get inference metrics (throughput, latency, etc.).
InferenceMetrics getMetrics() const;

/// @brief Get state of a pending request.
RequestState getRequestState(RequestHandle handle) const;

/// @brief Get number of active queries for a model.
int32_t getActiveQueryCount(const std::string& model_name) const;
```

---

## Configuration Types

### LLMConfig

```cpp
struct LLMConfig {
    // GPU Configuration
    bool gpu_enabled = true;
    int32_t gpu_device_id = -1;           // -1 = auto-select
    float gpu_memory_fraction = 0.8f;     // 0.0-1.0
    bool fallback_to_cpu = true;
    
    // Inference
    std::string default_model;
    int32_t num_workers = -1;             // -1 = auto (CPU core count)
    int32_t batch_size = 1024;
    int32_t batch_timeout_ms = 100;
    int64_t default_timeout_ms = 300000;  // 5 min
    int32_t max_concurrent_requests = -1; // -1 = unlimited
    
    // Cache
    int32_t response_cache_size_mb = 1024;
    bool kv_cache_offload_to_cpu = false;
    
    // Logging
    LogLevel log_level = LogLevel::kInfo;
    bool metrics_enabled = true;
};
```

### CompletionConfig

```cpp
struct CompletionConfig {
    std::string model;                     // Model name
    int32_t max_tokens = 512;
    float temperature = 0.7f;              // 0.0-2.0
    float top_p = 0.9f;                    // nucleus sampling
    int32_t top_k = 40;                    // top-k sampling
    bool do_sample = true;
    int32_t num_beams = 1;                 // 1 = greedy
    int64_t timeout_ms = 300000;
    std::vector<std::string> active_adapters;
    bool use_cache = true;
    std::string system_prompt;             // System context
};
```

---

## Status Codes

```cpp
enum class StatusCode {
    kOk,
    kCancelled,
    kInvalidArgument,
    kNotFound,
    kAlreadyExists,
    kPermissionDenied,
    kResourceExhausted,
    kFailedPrecondition,
    kAborted,
    kOutOfRange,
    kUnimplemented,
    kInternal,
    kUnavailable,
    kDataLoss,
    kDeadlineExceeded,
    kUnauthenticated
};
```

---

## Response Types

### CompletionResponse

```cpp
struct CompletionResponse {
    std::string text;                      // Generated text
    int32_t token_count;                   // Tokens generated
    int64_t latency_ms;                    // Total latency
    bool is_complete;                      // Fully complete?
    std::string model;                     // Model used
    CompletionStats stats;                 // Detailed statistics
};
```

### StreamToken

```cpp
struct StreamToken {
    std::string text;                      // Token text
    int32_t token_id;                      // Token ID
    float logprob;                         // Log probability
    bool is_final;                         // Is this the last token?
};
```

---

## Usage Examples

### Example 1: Simple Inference

```cpp
#include <llm/embedded_llm.h>

int main() {
    EmbeddedLLM llm;
    llm.initialize();
    llm.loadModel("model.gguf");
    
    auto result = llm.complete("What is 2+2?");
    if (result.ok()) {
        std::cout << result.value().text << std::endl;
    }
    
    llm.shutdown();
}
```

### Example 2: Streaming with Error Handling

```cpp
auto status = llm.streamCompleteWithCallbacks(
    "Tell me a story",
    CompletionConfig{.max_tokens = 500},
    [](const StreamToken& token) {
        // On each token
        std::cout << token.text;
        std::cout.flush();
    },
    [](Status error) {
        // On error
        LOG(ERROR) << "Stream failed: " << error.message();
    },
    []() {
        // On complete
        std::cout << "\n[DONE]" << std::endl;
    }
);
```

### Example 3: Asynchronous with Polling

```cpp
auto handle = llm.submit("Generate code snippet", config);

// Do other work while inference runs
while (true) {
    auto result = llm.poll(handle, false);  // Non-blocking
    if (result) {
        std::cout << result.value().value().text << std::endl;
        break;
    }
    std::cout << "Still processing..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
```

### Example 4: Batch Processing

```cpp
std::vector<std::string> prompts = {"Q1", "Q2", "Q3"};
std::vector<RequestHandle> handles;

// Submit all
for (const auto& prompt : prompts) {
    handles.push_back(llm.submit(prompt));
}

// Collect results
std::vector<std::string> results;
for (auto handle : handles) {
    auto result = llm.wait(handle);
    if (result.ok()) {
        results.push_back(result.value().text);
    }
}
```

---

**Last Updated:** 2026-08-17 (Phase 6)
**Status:** PRODUCTION (Wave 5 GA)
