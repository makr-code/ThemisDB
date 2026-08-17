# LLM Module — Developer Guide & Contribution Standards

**Status**: Active Development (Phase 6 Completed, Wave A in progress)  
**Last Updated**: 2026-08-17  
**Maintainer**: ThemisDB LLM Team

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Module Architecture](#module-architecture)
3. [Development Workflow](#development-workflow)
4. [Code Standards](#code-standards)
5. [Testing Strategy](#testing-strategy)
6. [Performance & Benchmarking](#performance--benchmarking)
7. [Troubleshooting](#troubleshooting)
8. [Release Process](#release-process)

---

## Quick Start

### Build the LLM Module

```bash
cd /path/to/ThemisDB

# Configure with LLM support
cmake --preset windows-release

# Build LLM module only
cmake --build --preset windows-release --target llm -j16

# Build with sanitizers (recommended for development)
cmake --preset windows-release -DCMAKE_CXX_FLAGS="-fsanitize=address -g"
cmake --build --preset windows-release -j16
```

### Run Tests

```bash
# Run all LLM tests
ctest --preset windows-release -L llm -V

# Run specific focused tests
ctest --preset windows-release -R "llm.*focused" -V

# Run with sanitizers
ASAN_OPTIONS=halt_on_error=1 ctest --preset windows-release -L llm -V
```

### Debug a Test

```bash
# Build test executable
cmake --build --preset windows-release --target test_llm_api_contract_hardening_focused

# Run with debugger
gdb ./build/windows-release/tests/llm/test_llm_api_contract_hardening_focused
(gdb) run
```

---

## Module Architecture

### High-Level Structure

```
src/llm/
├── Core Inference Runtime
│   ├── embedded_llm.cpp          — Primary inference interface
│   ├── async_inference_engine.cpp — Async request handling
│   ├── inference_engine_enhanced.cpp — Multi-model orchestration
│   └── shared_worker_pool.cpp    — Worker thread management
├── Model & Adapter Lifecycle
│   ├── model_loader.cpp           — Model load/unload
│   ├── model_router.cpp           — Model routing logic
│   ├── multi_lora_manager.cpp     — LoRA adapter management
│   └── llm_plugin_manager.cpp     — Plugin backend lifecycle
├── Memory & Performance
│   ├── paged_kv_cache.cpp        — KV cache management
│   ├── gpu_memory_manager.cpp    — GPU memory allocation
│   ├── adaptive_vram_allocator.cpp — Dynamic VRAM management
│   └── token_quota_manager.cpp   — Token quota enforcement
├── Safety & Policy
│   ├── prompt_policy.cpp         — Prompt safety validation
│   ├── production_validator.cpp  — Production safety checks
│   └── ethics_aware_confidence_detector.cpp — Ethics guardrails
└── Operations & Diagnostics
    ├── streaming_handler.cpp     — Stream output handling
    ├── llm_response_cache.cpp    — Response caching
    └── llm_model_audit_logger.cpp — Audit logging

include/llm/ — Public header files (match src/ structure)
tests/llm/ — Test files (match src/ structure)
```

### Key Interfaces

```cpp
// Main inference interface
class EmbeddedLLM {
  Status loadModel(const std::string& path, const LoadConfig& cfg);
  Result<CompletionResponse> complete(const std::string& prompt, const CompletionConfig& cfg);
  void streamComplete(const std::string& prompt, const CompletionConfig& cfg,
                      std::function<void(const StreamToken&)> callback);
  Result<std::vector<float>> embed(const std::string& text, const EmbedConfig& cfg);
  Status unloadModel(const std::string& model_name);
};

// Routing & model selection
class ModelRouter {
  Model* selectModel(const std::string& prompt);
  void addRoute(const ModelRoute& route);
};

// LoRA adapter management
class MultiLoraManager {
  AdapterId loadAdapter(const std::string& path, const AdapterConfig& cfg);
  Status switchAdapter(AdapterId from, AdapterId to);
  void unloadAdapter(AdapterId id);
};
```

---

## Development Workflow

### Creating a New Feature

1. **Plan**: Create an issue or design doc
2. **Branch**: `git checkout -b feature/my-feature`
3. **Implement**: Add code following code standards (§4)
4. **Test**: Add tests with >= 80% coverage
5. **Document**: Update Doxygen headers and README
6. **PR**: Submit PR with clear description
7. **Review**: Address feedback from code review
8. **Merge**: Rebase and merge to develop

### Example: Add a new inference mode

```cpp
// 1. Define new inference mode in header
// include/llm/inference_modes.h
enum class InferenceMode {
  kStandard,
  kDraft,      // NEW
  kSpeculative // NEW
};

// 2. Implement in source
// src/llm/inference_engine_enhanced.cpp
Result<CompletionResponse> InferenceEngine::completeDraft(...) {
  // Implementation
}

// 3. Add tests
// tests/llm/test_inference_modes_focused.cpp
TEST(InferenceModes, DraftModeProducesTokens) {
  // Test
}

// 4. Document
/// @brief Draft-and-verify inference mode for speculation.
/// @param prompt User prompt
/// @return Completion response with draft tokens
/// @thread_safety Thread-safe for concurrent calls
Result<CompletionResponse> completeDraft(const std::string& prompt);

// 5. Update README
// src/llm/README.md - Add section on draft mode usage
```

### Code Review Checklist

**Before Submitting PR:**
- [ ] Code compiles: `cmake --build --preset windows-release`
- [ ] No new warnings: `-Wall -Wextra -pedantic`
- [ ] Tests pass: `ctest --preset windows-release -L llm`
- [ ] Sanitizers pass: ASan/TSan/UBSan clean
- [ ] Documentation updated: Doxygen headers, README
- [ ] Performance baseline: Benchmarks within ±10%
- [ ] Thread-safety: Proper mutex/atomic usage

**During Review:**
- Reviewers check functional correctness
- Security review for externally-reachable APIs
- Performance impact assessment
- Design consistency with architecture

---

## Code Standards

### Naming Conventions

```cpp
// Classes: PascalCase
class InferenceEngine {};

// Functions: camelCase
void submitRequest();
Status loadModel();

// Member variables: snake_case_with_trailing_underscore
int token_count_;
std::mutex request_mutex_;

// Constants: UPPER_SNAKE_CASE
constexpr int kMaxBatchSize = 1024;
constexpr float kDefaultTemperature = 0.7f;

// Enums: PascalCase for enum, UPPER_SNAKE_CASE for values
enum class InferenceMode {
  kStandard,
  kStreaming,
  kDraft
};
```

### Documentation Template

```cpp
/// @file inference_engine_enhanced.cpp
/// @brief Multi-model orchestration and enhanced inference controls.
/// @details Handles model routing, streaming callbacks, and policy enforcement
///          for inference requests. Thread-safe via internal synchronization.
/// @maturity PRODUCTION
/// @ingroup llm

/// @brief Submit an inference request asynchronously.
/// @param prompt User input prompt
/// @param config Configuration (model, timeout, temperature, etc.)
/// @param callback Invoked with result when inference completes
/// @return RequestHandle for tracking/cancellation
/// @throws std::invalid_argument if prompt is empty
/// @throws std::runtime_error if no models are loaded
/// @thread_safety Thread-safe. Callback invoked on internal worker thread.
/// @exception_safety Strong: request submitted or exception thrown, no side effects
/// @see complete(), streamComplete(), cancelRequest()
RequestHandle submitRequest(const std::string& prompt,
                             const CompletionConfig& config,
                             std::function<void(const Result<CompletionResponse>&)> callback);

/// @brief Execute cancellation of in-flight request.
/// @param handle RequestHandle from submitRequest()
/// @return Status::OK if cancelled, Status::kNotFound if already completed
/// @thread_safety Thread-safe for concurrent cancellations
/// @note Cancellation is asynchronous; callback may still be invoked
void cancelRequest(RequestHandle handle);
```

### RAII Principles

Always use smart pointers and RAII wrappers:

```cpp
// ❌ WRONG - Manual memory management
uint8_t* data = new uint8_t[1024];
process(data);
delete[] data;  // Leaks on exception!

// ✅ RIGHT - RAII wrapper
auto data = std::make_unique<uint8_t[]>(1024);
process(data.get());  // Automatic cleanup

// ✅ ALSO RIGHT - Custom RAII for GPU
class CudaBuffer {
 public:
  CudaBuffer(size_t size) {
    CUDA_CHECK(cudaMalloc(&ptr_, size));
  }
  ~CudaBuffer() { if (ptr_) cudaFree(ptr_); }
  
  uint8_t* get() { return ptr_; }
  CudaBuffer(CudaBuffer&& other) noexcept : ptr_(other.release()) {}
  
 private:
  uint8_t* ptr_ = nullptr;
  uint8_t* release() {
    uint8_t* p = ptr_;
    ptr_ = nullptr;
    return p;
  }
};
```

### Thread-Safety Annotations

```cpp
class ModelCache {
 private:
  /// @thread_safety Protected by models_mutex_
  mutable std::mutex models_mutex_;
  std::map<std::string, std::unique_ptr<Model>> loaded_models_;
  
  /// @thread_safety Atomic operations only
  std::atomic<uint64_t> total_loads_{0};

 public:
  /// @thread_safety Thread-safe. May block on cache update.
  Model* getModel(const std::string& name) {
    std::lock_guard<std::mutex> lock(models_mutex_);
    // Access loaded_models_ under lock
  }
};
```

---

## Testing Strategy

### Test Organization

```cpp
// File: tests/llm/test_inference_modes_focused.cpp
// Test pattern: MODULE_TYPE_CATEGORY_focused

#include <gtest/gtest.h>
#include <themis/testing/module_test_registry.h>

namespace themis::llm::test {

class InferenceModesTest : public ::testing::Test {
 protected:
  void SetUp() override {
    engine_.initialize();
    engine_.loadModel("test-model.gguf");
  }
  
  EmbeddedLLM engine_;
};

// Test case format: <FEATURE>_<SCENARIO>
TEST_F(InferenceModesTest, StandardModeCompletes) {
  auto result = engine_.complete("Hello", CompletionConfig{
      .mode = InferenceMode::kStandard
  });
  EXPECT_TRUE(result.ok());
  EXPECT_FALSE(result.value().text.empty());
}

TEST_F(InferenceModesTest, DraftModeWithinTimeConstraint) {
  auto start = std::chrono::high_resolution_clock::now();
  auto result = engine_.complete("Hello", CompletionConfig{
      .mode = InferenceMode::kDraft,
      .timeout_ms = 5000
  });
  auto duration = std::chrono::high_resolution_clock::now() - start;
  
  EXPECT_TRUE(result.ok());
  EXPECT_LT(duration, std::chrono::milliseconds(5000));
}

// Register test with focus label
THEMIS_REGISTER_MODULE_FOCUSED_TEST(
    InferenceModesTest,
    {.module = "llm", .category = "inference", .tier = "unit"});

}  // namespace themis::llm::test
```

### Test Coverage Requirements

- **Public APIs**: >= 90% line coverage
- **Error paths**: All error returns covered
- **Thread-safe paths**: Concurrency tests for >=1 level
- **Exception paths**: Try/catch blocks tested
- **Performance paths**: Benchmarks with gate verification

### Running Tests

```bash
# Unit tests only
ctest --preset windows-release -R "llm.*" -LE integration -V

# Integration tests
ctest --preset windows-release -R "llm.*" -L integration -V

# Focused tests only (release-critical)
ctest --preset windows-release -L "release_critical;llm" -V

# With sanitizers
ASAN_OPTIONS=halt_on_error=1:detect_leaks=1 ctest --preset windows-release -L llm -V
TSAN_OPTIONS=halt_on_error=1 ctest --preset windows-release -L llm -V
```

---

## Performance & Benchmarking

### Adding a Benchmark

```cpp
// File: benchmarks/llm/bench_llm_hotpaths.cpp
#include <benchmark/benchmark.h>

static void BenchInferenceLatency(benchmark::State& state) {
  EmbeddedLLM engine;
  engine.initialize();
  engine.loadModel("llama2-7b.gguf");
  
  for (auto _ : state) {
    auto result = engine.complete(
        "What is 2+2?",
        CompletionConfig{.max_tokens = 10}
    );
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BenchInferenceLatency)->Name("LLM-LATENCY-01");

static void BenchModelLoadTime(benchmark::State& state) {
  EmbeddedLLM engine;
  engine.initialize();
  
  for (auto _ : state) {
    engine.loadModel("llama2-7b.gguf");
    engine.unloadModel("llama2-7b");
  }
}
BENCHMARK(BenchModelLoadTime)->Name("LLM-LOAD-02");
```

### Performance Gates (GATE-LLM-01..08)

| Gate | Target | Threshold | Consequence |
|------|--------|-----------|------------|
| GATE-LLM-01 | Inference latency (p50) | < baseline +10% | FAIL if exceeded |
| GATE-LLM-02 | Inference latency (p99) | < baseline +15% | FAIL if exceeded |
| GATE-LLM-03 | Model load time | < baseline +20% | WARN if exceeded |
| GATE-LLM-04 | Adapter switch overhead | < 100ms | FAIL if exceeded |
| GATE-LLM-05 | Memory allocation peak | < baseline +5% | WARN if exceeded |
| GATE-LLM-06 | Token throughput | > baseline -5% | FAIL if exceeded |
| GATE-LLM-07 | Cache hit rate | > 80% (if enabled) | WARN if below |
| GATE-LLM-08 | Query latency (p95) | < 200ms (wiki RAG) | FAIL if exceeded |

### Running Benchmarks

```bash
# Run all LLM benchmarks
ctest --preset windows-release -L benchmark -R "llm" -V

# Run specific benchmark
./build/windows-release/benchmarks/llm_hotpaths \
  --benchmark_filter="LLM-LATENCY-01" \
  --benchmark_repetitions=5 \
  --benchmark_report_aggregates_only=true
```

---

## Troubleshooting

### Common Build Issues

**Issue**: "Missing CUDA toolkit"
```bash
# Solution: Set CUDA path
cmake --preset windows-release -DCUDA_TOOLKIT_ROOT_DIR=/path/to/cuda
```

**Issue**: "ThreadSanitizer: data race"
```cpp
// Solution: Add proper synchronization
std::mutex state_mutex_;
std::vector<T> shared_state_;  // Protected by state_mutex_

{
  std::lock_guard<std::mutex> lock(state_mutex_);
  shared_state_.push_back(item);
}
```

**Issue**: "AddressSanitizer: heap-use-after-free"
```cpp
// Solution: Use smart pointers
// ❌ WRONG
Model* model = new Model();
delete model;
model->inference();  // Use-after-free!

// ✅ RIGHT
auto model = std::make_unique<Model>();
model->inference();  // OK, pointer still valid
// Auto-deleted when model goes out of scope
```

### Common Runtime Issues

**Issue**: "Model load failed: CUDA out of memory"
```bash
# Solution: Reduce GPU memory fraction
export THEMIS_LLM_GPU_MEMORY_FRACTION=0.6
# Or enable CPU fallback
export THEMIS_LLM_FALLBACK_TO_CPU=true
```

**Issue**: "Request timeout after 300000ms"
```cpp
// Solution: Increase timeout
auto result = engine.complete(prompt, CompletionConfig{
    .timeout_ms = 600000  // 10 minutes
});
```

---

## Release Process

### Pre-Release Checklist

- [ ] All tests pass: `ctest --preset windows-release --output-on-failure`
- [ ] Sanitizers clean: ASan/TSan/UBSan
- [ ] Benchmarks pass: `GATE-LLM-01..08` all green
- [ ] Code review approved
- [ ] Documentation updated: README, ROADMAP, CHANGELOG
- [ ] CHANGELOG.md entries added with version/date

### Release Steps

1. **Tag**: `git tag -a v2.x.x -m "Release v2.x.x"`
2. **Build**: `cmake --build --preset release -j16`
3. **Package**: `cpack -G DEB -G RPM` (Linux) or `.msi` (Windows)
4. **Publish**: Upload to release repository
5. **Announce**: Update CHANGELOG, post release notes

---

## Getting Help

- **API Documentation**: See `include/llm/llm_api_contract.h`
- **Architecture**: See `ARCHITECTURE.md`
- **Troubleshooting**: See [Quick Troubleshooting](#troubleshooting) above
- **Module Status**: See `ROADMAP.md`
- **Gaps/Issues**: See `MODULE_GAPS.md`

---

**Last Updated**: 2026-08-17  
**Version**: 1.0 (Phase 6 Complete)  
**Next Review**: Q4 2026 (Wave B release)
