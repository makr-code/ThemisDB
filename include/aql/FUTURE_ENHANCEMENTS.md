# AQL Headers - Future Enhancements

## Scope

- API-level enhancements to `include/aql/` headers — new AQL command interfaces extending `AQLCommand` base
- Multi-modal input types (`MultiModalInferRequest`, `ModalityType`) with MIME-validated byte ranges
- New built-in function registrations via `FunctionRegistry::registerBuiltin()`
- Parser extension hooks for custom AQL grammar additions (opt-in, backward-compatible)
- Streaming inference interface (`TokenStream`, coroutine-based `Generator<std::string>`)
- Agent framework interface (`IAgent`, `Tool`, `AgentResult`) for multi-step reasoning and tool calling

## Design Constraints

- [ ] All new AQL commands must extend the `AQLCommand` base class; execution bypass is not permitted
- [ ] New built-in functions must be registered via `FunctionRegistry::registerBuiltin()` — no ad-hoc dispatch
- [ ] Parse tree must remain backward-compatible; new grammar extensions use opt-in extension hooks only
- [ ] Async methods must return `std::future<Result<T>>`; per-token callbacks are supplementary only
- [ ] Multi-modal inputs must carry a validated MIME type and byte range to prevent injection attacks
- [ ] ABI stability: new virtual methods added only at end of vtable; breaking changes versioned as `ILLMBackendV2`

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `AQLCommand` (base) | All new AQL command types | Must validate input types before `execute()` |
| `FunctionRegistry::registerBuiltin()` | Built-in function extensions | Single registration point; rejects duplicates |
| `TokenStream` | Streaming inference consumers | Iterator-based; exposes `cancel()` |
| `IAgent` | Agent framework consumers | `execute()` + `registerTool()` contract |
| `IAsyncLLMBackend` | Async AQL handler | Returns `std::future<Result<T>>` |
| `MultiModalInferRequest` | Vision / audio AQL commands | Extends `InferRequest`; MIME-validated |

## Planned Features

### Streaming Interface ✅ SHIPPED (v1.7.0)
**Priority:** High
**Target Version:** v1.7.0 — **Implemented**

Both the SSE explanation streaming and the generic `AQLTokenStream` iterator API are now shipped.

**Shipped interfaces:**
```cpp
// Streams explanation tokens as Server-Sent Events (already shipped)
std::string streamExplainAQLAsSSE(const std::string& aql_query,
                                  SseResponseWriter& writer);
```

**Proposed Extension (generic `TokenStream` — target v1.7.0):**
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

### Multi-Modal Interface ✅ SHIPPED (v1.8.0)
**Priority:** High
**Target Version:** v1.8.0 — **Implemented** (`include/aql/multimodal_infer_request.h`)

**Shipped Structures:**
```cpp
enum class ModalityType { TEXT, IMAGE, AUDIO, VIDEO };

struct MultiModalInput {
    ModalityType type;
    std::variant<std::string, std::vector<uint8_t>, std::filesystem::path> data;
    std::string mime_type;
    std::string label;
    void validate() const;  // throws std::invalid_argument on bad MIME or empty bytes
    static const std::unordered_set<std::string>& imageMimeTypes();
    static const std::unordered_set<std::string>& audioMimeTypes();
    static const std::unordered_set<std::string>& videoMimeTypes();
};

struct MultiModalInferRequest : public llm::InferenceRequest {
    std::vector<MultiModalInput> inputs;
    void addInput(const MultiModalInput&);  // validates before appending
    void validateInputs() const;
    bool hasNonTextInputs() const;
};
```

---

### Agent Framework Interface ✅ SHIPPED (v1.7.0)
**Priority:** Medium
**Target Version:** v1.7.0 — **Implemented** (`include/aql/aql_agent.h`, `src/aql/aql_agent.cpp`)

Interfaces for multi-step reasoning and tool calling are now shipped via `IAgent` / `ReActAgent`.

**Shipped Structures:**
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

### Async Interface ✅ SHIPPED (v1.8.0)
**Priority:** High
**Target Version:** v1.8.0 — **Implemented** (`include/aql/iasync_llm_backend.h`)

**Shipped Interfaces:**
```cpp
class IAsyncLLMBackend {
public:
    virtual ~IAsyncLLMBackend() = default;
    virtual std::future<Result<std::string>>
        inferAsync(const llm::InferenceRequest& req) = 0;
    virtual std::future<Result<std::vector<float>>>
        embedAsync(const std::string& text) = 0;
    virtual bool supportsMultiModal() const { return false; }
};

class ThreadPoolAsyncLLMBackend : public IAsyncLLMBackend {
public:
    explicit ThreadPoolAsyncLLMBackend(std::shared_ptr<llm::ILLMPlugin> plugin);
    // ... wraps plugin->generate() / plugin->embed() via std::async
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

### v1.6.x → v1.7.x: Streaming Interface ✅ Done
**Breaking Changes:** None (additive)

**Shipped APIs (v1.7.0):**
- `AQLTokenStream` (`include/aql/aql_token_stream.h`) – header-only, thread-safe token iterator
- `IAgent` / `ReActAgent` (`include/aql/aql_agent.h`) – ReAct reasoning + tool calling framework

---

### v1.7.x → v1.8.x: Agent Framework ✅ Done
**Breaking Changes:** None (new interfaces)

**Shipped APIs (v1.8.0):**
- `IAsyncLLMBackend` + `ThreadPoolAsyncLLMBackend` (`include/aql/iasync_llm_backend.h`) – non-blocking async inference
- `ModalityType`, `MultiModalInput`, `MultiModalInferRequest` (`include/aql/multimodal_infer_request.h`) – MIME-validated multi-modal requests

---

## Community Contributions Welcome

### High-Impact, Beginner-Friendly
- [ ] Add more Doxygen comments
- [ ] Create usage examples for each interface
- [ ] Document error codes
- [ ] Add type aliases for common patterns

### Medium Complexity
- [x] Implement streaming interface (`include/aql/aql_token_stream.h`, v1.7.0)
- [ ] Add batch inference support
- [ ] Create request builder pattern
- [x] Implement async interface (`include/aql/iasync_llm_backend.h`, v1.8.0)

### Advanced Topics
- [x] Design multi-modal interface (`include/aql/multimodal_infer_request.h`, v1.8.0)
- [x] Implement agent framework (`include/aql/aql_agent.h`, `src/aql/aql_agent.cpp`, v1.7.0)
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

## Test Strategy

- Unit tests for each new `AQLCommand` subclass: verify input type validation fires before `execute()` is reached
- `FunctionRegistry` tests: verify `registerBuiltin()` rejects duplicates and enforces valid function signatures
- `TokenStream` tests: verify iteration, `cancel()` from a separate thread, and partial consumption without resource leaks
- Async interface tests: verify `std::future` resolves correctly under cancellation and deadline timeout scenarios
- Multi-modal input tests: validate MIME type enforcement and rejection of unsupported or malformed payloads
- ABI compatibility tests: compile against v1.5 headers with v1.6+ runtime and assert no linker errors

## Performance Targets

- `FunctionRegistry` function lookup by name: ≤ 100 ns (hash map, no lock contention on hot read path)
- AQL AST node construction: ≤ 1 µs per node for all new command types
- `TokenStream::operator++()` overhead (single token advance): ≤ 500 ns excluding model generation time
- `IAsyncLLMBackend::inferAsync()` dispatch overhead: ≤ 50 µs from call to thread hand-off
- `IAgent::execute()` tool dispatch overhead per step: ≤ 1 ms excluding actual tool execution time
- Batch inference (`BatchInferRequest` of 16 requests): aggregate throughput ≥ 2× single-request rate

## Security / Reliability

- All new AQL commands must validate input types and reject malformed inputs before any execution begins
- No raw SQL pass-through: AQL commands must not allow unescaped string injection into backend queries
- Multi-modal inputs: MIME type and byte range validated before deserialization to prevent buffer over-reads
- Agent tool executors must run in a restricted context; filesystem and network access prohibited by default
- `TokenStream` cancellation must be safe to invoke concurrently from any thread
- Fine-tuning interface must not allow training data to overwrite system prompt or safety guardrails

*Last Updated: April 2026*
*Module Version: v1.5.x*
*Next Review: v1.6.0 Release*
