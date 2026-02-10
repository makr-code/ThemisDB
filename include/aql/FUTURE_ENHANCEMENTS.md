# AQL Headers - Future Enhancements

## Planned Features

### Streaming Interface
**Priority:** High  
**Target Version:** v1.7.0

Add streaming API for progressive token generation.

**Current Interface:**
```cpp
Result<std::string> infer(const InferRequest& req);
```

**Proposed Interface:**
```cpp
class TokenStream {
public:
    class Iterator {
    public:
        std::string operator*() const;  // Current token
        Iterator& operator++();          // Next token
        bool operator!=(const Iterator& other) const;
    };
    
    Iterator begin();
    Iterator end();
    void cancel();  // Stop generation
};

Result<TokenStream> inferStreaming(const InferRequest& req);
```

**Usage:**
```cpp
auto stream = handler.inferStreaming(req);
for (const auto& token : stream.value()) {
    std::cout << token << std::flush;  // Progressive output
}
```

---

### Multi-Modal Interface
**Priority:** High  
**Target Version:** v1.7.0

Extend interfaces to support images, audio, and video.

**Proposed Structures:**
```cpp
enum class ModalityType {
    TEXT,
    IMAGE,
    AUDIO,
    VIDEO
};

struct MultiModalInput {
    ModalityType type;
    std::variant<
        std::string,              // Text
        std::vector<uint8_t>,     // Image bytes
        std::filesystem::path     // File path
    > data;
    std::string mime_type;
};

struct MultiModalInferRequest : public InferRequest {
    std::vector<MultiModalInput> inputs;
};
```

**Interface Addition:**
```cpp
class ILLMBackend {
public:
    // Existing
    virtual Result<std::string> infer(const InferRequest& req) = 0;
    
    // New
    virtual Result<std::string> inferMultiModal(const MultiModalInferRequest& req) = 0;
    virtual bool supportsMultiModal() const = 0;
};
```

---

### Agent Framework Interface
**Priority:** Medium  
**Target Version:** v1.8.0

Define interfaces for multi-step reasoning and tool calling.

**Proposed Structures:**
```cpp
struct Tool {
    std::string name;
    std::string description;
    json schema;  // JSON Schema for parameters
    std::function<json(const json&)> executor;
};

struct AgentConfig {
    std::string model_alias;
    std::vector<Tool> tools;
    int max_iterations = 10;
    float temperature = 0.7;
};

struct ReasoningStep {
    std::string thought;
    std::optional<std::string> tool_name;
    std::optional<json> tool_input;
    std::optional<json> tool_output;
    std::string observation;
};

struct AgentResult {
    std::string final_answer;
    std::vector<ReasoningStep> reasoning_trace;
    int iterations_used;
};
```

**Interface:**
```cpp
class IAgent {
public:
    virtual Result<AgentResult> execute(const std::string& task, const json& context) = 0;
    virtual void registerTool(const Tool& tool) = 0;
    virtual std::vector<Tool> getTools() const = 0;
};
```

---

### Batch Interface
**Priority:** Medium  
**Target Version:** v1.7.0

Efficient batch processing of multiple requests.

**Proposed Interface:**
```cpp
struct BatchInferRequest {
    std::vector<InferRequest> requests;
    bool preserve_order = true;
};

struct BatchInferResult {
    std::vector<Result<std::string>> results;
    std::chrono::milliseconds total_duration;
};

class ILLMBackend {
public:
    virtual Result<BatchInferResult> inferBatch(const BatchInferRequest& req) = 0;
};
```

**Benefits:**
- Higher throughput via batching
- Better GPU utilization
- Reduced per-request overhead

---

### Callback Interface
**Priority:** Low  
**Target Version:** v1.8.0

Progress callbacks for long-running operations.

**Proposed:**
```cpp
struct InferProgress {
    int tokens_generated;
    int total_tokens_estimated;
    std::chrono::milliseconds elapsed;
    float tokens_per_second;
};

using ProgressCallback = std::function<void(const InferProgress&)>;

struct InferRequest {
    // Existing fields...
    std::optional<ProgressCallback> on_progress;
    std::optional<std::function<void(const std::string&)>> on_token;  // Per-token callback
};
```

---

### Fine-Tuning Interface
**Priority:** Medium  
**Target Version:** v1.7.0

Define interfaces for in-database model fine-tuning.

**Proposed:**
```cpp
struct TrainingExample {
    std::string instruction;
    std::string response;
    std::optional<std::string> context;
};

struct FineTuneConfig {
    std::string base_model;
    std::vector<TrainingExample> dataset;
    int epochs = 3;
    float learning_rate = 1e-4;
    int lora_rank = 16;
    int lora_alpha = 32;
    float lora_dropout = 0.05;
    std::string output_path;
};

struct FineTuneProgress {
    int current_epoch;
    int total_epochs;
    float loss;
    float learning_rate;
};

class IFineTuner {
public:
    virtual Result<std::string> fineTune(
        const FineTuneConfig& config,
        std::function<void(const FineTuneProgress&)> callback = nullptr
    ) = 0;
    virtual void cancel() = 0;
};
```

---

## Performance Optimizations

### Zero-Copy Data Transfer
**Priority:** High  
**Target Version:** v1.6.0

Eliminate memory copies in data transfer.

**Current:**
```cpp
Result<std::string> infer(const InferRequest& req);
// Returns copied string
```

**Proposed:**
```cpp
Result<std::string_view> inferZeroCopy(const InferRequest& req);
// Returns view into internal buffer
// User must consume before next call
```

---

### Memory-Mapped Model Loading
**Priority:** High  
**Target Version:** v1.6.0

Use mmap for faster model loading.

**Proposed:**
```cpp
struct ModelLoadRequest {
    std::string model_path;
    std::string alias;
    bool use_mmap = true;  // New flag
    // ...
};
```

**Benefits:**
- Faster cold start (no loading time)
- Shared memory across processes
- OS manages paging

---

### Compile-Time Type Safety
**Priority:** Medium  
**Target Version:** v1.7.0

Use C++20 concepts for stronger type checking.

**Proposed:**
```cpp
template<typename T>
concept LLMBackend = requires(T backend, const InferRequest& req) {
    { backend.infer(req) } -> std::same_as<Result<std::string>>;
    { backend.embed(std::string{}) } -> std::same_as<Result<std::vector<float>>>;
};

template<LLMBackend Backend>
class LlmAqlHandler {
    // Enforced at compile time
};
```

---

### Async Interface
**Priority:** High  
**Target Version:** v1.7.0

Non-blocking async operations.

**Proposed:**
```cpp
class IAsyncLLMBackend {
public:
    virtual std::future<Result<std::string>> inferAsync(const InferRequest& req) = 0;
    virtual std::future<Result<std::vector<float>>> embedAsync(const std::string& text) = 0;
};
```

---

## Refactoring Opportunities

### Separate Backend Interface from Handler
**Priority:** High  
**Target Version:** v1.6.0

Decouple LLM backend from AQL handler.

**Current:**
```cpp
// llm_aql_handler.h includes llama.cpp headers
#include <llama.h>
```

**Proposed:**
```cpp
// Pure interface, no implementation headers
class ILLMBackend {
    virtual Result<std::string> infer(const InferRequest&) = 0;
};

// Handler only depends on interface
class LlmAqlHandler {
    std::shared_ptr<ILLMBackend> backend_;
};
```

---

### Request Builder Pattern
**Priority:** Medium  
**Target Version:** v1.7.0

Fluent API for request construction.

**Current:**
```cpp
InferRequest req;
req.prompt = "...";
req.model_alias = "llama-3-8b";
req.max_tokens = 500;
req.temperature = 0.7;
```

**Proposed:**
```cpp
auto req = InferRequest::builder()
    .prompt("...")
    .model("llama-3-8b")
    .maxTokens(500)
    .temperature(0.7)
    .build();
```

---

### Error Codes Enum
**Priority:** Medium  
**Target Version:** v1.7.0

Standardize error codes for better error handling.

**Proposed:**
```cpp
enum class LLMErrorCode {
    SUCCESS = 0,
    MODEL_NOT_FOUND,
    MODEL_LOAD_FAILED,
    INFERENCE_FAILED,
    CONTEXT_LENGTH_EXCEEDED,
    OUT_OF_MEMORY,
    INVALID_REQUEST,
    BACKEND_ERROR
};

struct LLMError {
    LLMErrorCode code;
    std::string message;
    std::optional<std::string> details;
};

template<typename T>
using LLMResult = Result<T, LLMError>;
```

---

### Header-Only Utilities
**Priority:** Low  
**Target Version:** v1.8.0

Move simple utilities to header-only for inlining.

**Candidates:**
- Token counting
- Prompt templating
- Request validation

---

## Known Issues

### Issue #1: Forward Declaration Challenges
**Severity:** Low  
**Reported:** v1.5.0

Complex type dependencies make forward declarations difficult.

**Workaround:** Include full headers

**Fix:** Use PIMPL idiom for complex types

**Planned Fix:** v1.6.0

---

### Issue #2: Template Export Issues
**Severity:** Medium  
**Reported:** v1.5.1

Template methods in headers can cause linker errors.

**Workaround:** Inline all template methods

**Fix:** Explicit template instantiation in .cpp

**Planned Fix:** v1.6.0

---

### Issue #3: ABI Stability
**Severity:** Medium  
**Reported:** v1.5.0

Adding virtual methods breaks ABI compatibility.

**Workaround:** Version interfaces (ILLMBackendV1, ILLMBackendV2)

**Fix:** Use PIMPL for stable ABI

**Planned Fix:** v1.7.0

---

## Research Areas

### Constexpr Interface Validation
**Focus:** Compile-time validation

Use constexpr to validate interfaces at compile time:
```cpp
constexpr bool validateLLMBackend(const ILLMBackend* backend) {
    // Check interface completeness
    return true;
}
```

---

### Reflection-Based Interface Discovery
**Focus:** Runtime interface introspection

Explore C++26 reflection for:
- Automatic serialization
- Dynamic interface discovery
- Binding generation

---

### Coroutine-Based Streaming
**Focus:** Natural streaming with C++20 coroutines

```cpp
Generator<std::string> inferStreaming(const InferRequest& req) {
    for (auto token : generate_tokens()) {
        co_yield token;
    }
}
```

---

## Migration Paths

### v1.5.x → v1.6.x: Backend Interface Abstraction
**Breaking Changes:** LlmAqlHandler constructor

**Old:**
```cpp
LlmAqlHandler(LlamaCppBackend* backend);
```

**New:**
```cpp
LlmAqlHandler(std::shared_ptr<ILLMBackend> backend);
```

**Migration:**
```cpp
// Old
LlamaCppBackend backend;
LlmAqlHandler handler(&backend);

// New
auto backend = std::make_shared<LlamaCppBackend>();
LlmAqlHandler handler(backend);
```

---

### v1.6.x → v1.7.x: Streaming Interface
**Breaking Changes:** None (additive)

**New APIs:**
```cpp
Result<TokenStream> inferStreaming(const InferRequest&);
std::future<Result<std::string>> inferAsync(const InferRequest&);
```

---

### v1.7.x → v1.8.x: Agent Framework
**Breaking Changes:** None (new interfaces)

**New Interfaces:**
```cpp
class IAgent { /* ... */ };
```

---

## Community Contributions Welcome

### High-Impact, Beginner-Friendly
- [ ] Add more Doxygen comments
- [ ] Create usage examples for each interface
- [ ] Document error codes
- [ ] Add type aliases for common patterns

### Medium Complexity
- [ ] Implement streaming interface
- [ ] Add batch inference support
- [ ] Create request builder pattern
- [ ] Implement async interface

### Advanced Topics
- [ ] Design multi-modal interface
- [ ] Implement agent framework
- [ ] Create fine-tuning interface
- [ ] Add coroutine support

**Contribution Guide:** See [CONTRIBUTING.md](../../CONTRIBUTING.md)

---

## Feedback and Discussion

Have ideas for AQL interface improvements?

- 💡 Feature requests: [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- 💬 Design discussions: [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- 📚 Documentation feedback: Label as "documentation"

---

*Last Updated: February 2026*  
*Module Version: v1.5.x*  
*Next Review: v1.6.0 Release*
