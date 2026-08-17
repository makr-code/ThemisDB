# LLM Module - Developer Guide

<!-- Status: complete | validated: 2026-08-17 -->
<!-- Links: README.md · ARCHITECTURE.md · THREADING.md · CONFIGURATION.md -->

Version: 1.0 (Phase 6)
Last Updated: 2026-08-17
Module Path: src/llm/
Maintainers: LLM Module Team

---

## Table of Contents

1. [Setup & Build](#setup--build)
2. [Architecture Overview](#architecture-overview)
3. [Contributing Guidelines](#contributing-guidelines)
4. [Testing Strategy](#testing-strategy)
5. [Code Style & Standards](#code-style--standards)
6. [Common Tasks](#common-tasks)
7. [Debugging & Profiling](#debugging--profiling)

---

## Setup & Build

### Prerequisites

**Required:**
- C++17 or later compiler (MSVC 2019+, GCC 9+, Clang 12+)
- CMake 3.20+
- NVIDIA CUDA Toolkit 11.0+ (for GPU support)
- Python 3.8+ (for build tools)

**Optional:**
- NVIDIA cuDNN (for optimized kernels)
- ROCm (for AMD GPU support)
- Intel oneAPI (for Intel GPU/CPU optimization)

### Building Locally

**Windows (Recommended):**

```bash
cd ThemisDB
cmake --preset windows-release
cmake --build --preset windows-release --target llm --parallel 16
```

**Linux:**

```bash
cd ThemisDB
cmake --preset linux-release
cmake --build --preset linux-release --target llm --parallel 16
```

**macOS:**

```bash
cd ThemisDB
cmake --preset macos-release
cmake --build --preset macos-release --target llm --parallel 16
```

### Running Tests

**All LLM Tests:**

```bash
ctest --preset windows-release -R llm -V
```

**Specific Test Suite:**

```bash
ctest --preset windows-release -R llm_inference_engine_test -V
ctest --preset windows-release -R llm_model_loader_test -V
ctest --preset windows-release -R llm_threading_safety_test -V
```

**With Thread Sanitizer (TSan):**

```bash
cmake --preset debug-tsan
cmake --build --preset debug-tsan --target llm_tests
ctest --preset debug-tsan -R llm -V
```

---

## Architecture Overview

### Module Structure

```
src/llm/
├── *.cpp                           # Implementation files
├── api/                            # HTTP adapter layer
├── applications/                   # Application-level integrations
├── attention/                      # Attention mechanism implementations
├── lora_framework/                 # LoRA adapter system
├── safety/                         # Safety & policy enforcement
├── security/                       # Security utilities
├── grammars/                       # Grammar files for constrained decoding
├── ARCHITECTURE.md                 # System design
├── README.md                       # Quick start
├── THREADING.md                    # Thread-safety model
├── OPERATIONS.md                   # Operational runbooks
├── CONFIGURATION.md                # Configuration guide
├── API_REFERENCE.md                # API documentation
└── MODULE_GAPS.md                  # Documentation gaps & status

include/llm/
├── embedded_llm.h                  # Main interface
├── llm_client.h                    # Client interface
├── llm_plugin_manager.h            # Plugin management
├── multi_lora_manager.h            # LoRA adapter management
├── llm_response_cache.h            # Response caching
└── ...                             # Other public headers
```

### Key Components

| Component | Files | Responsibility |
|---|---|---|
| **Inference Engine** | `async_inference_engine.cpp`, `inference_engine_enhanced.cpp` | Submit, execute, and manage inference requests |
| **Model Management** | `model_loader.cpp`, `model_router.cpp` | Load, unload, and route models |
| **LoRA Adapters** | `multi_lora_manager.cpp`, `lora_router.cpp` | Manage and hot-swap LoRA adapters |
| **Streaming** | `streaming_handler.cpp`, `openai_compat_adapter.cpp` | Handle token streaming and OpenAI compatibility |
| **GPU Memory** | `gpu_memory_manager.cpp`, `active_vram_allocator.cpp` | Manage GPU memory allocation and defragmentation |
| **Caching** | `llm_response_cache.cpp`, `llm_prefix_cache.cpp` | Cache responses and KV cache prefixes |
| **Safety** | `prompt_policy.cpp`, `llm_security_utils.cpp`, `production_validator.cpp` | Enforce safety policies and validation |
| **Scheduling** | `shared_worker_pool.cpp`, `continuous_batch_scheduler.cpp` | Schedule inference execution |

---

## Contributing Guidelines

### Before Starting

1. **Read Documentation:**
   - [README.md](README.md) — Quick start and API overview
   - [ARCHITECTURE.md](ARCHITECTURE.md) — System design and concurrency model
   - [THREADING.md](THREADING.md) — Thread-safety contracts
   - [OPERATIONS.md](OPERATIONS.md) — Operational patterns

2. **Check ROADMAP.md:**
   - Identify your task in the ROADMAP
   - Understand acceptance criteria and dependencies
   - Check for related Wave/Phase tracking

3. **Understand Coding Standards:**
   - See [Code Style & Standards](#code-style--standards) below
   - Review existing code for conventions

### Workflow

**1. Create a Branch:**

```bash
git checkout develop
git pull origin develop
git checkout -b feature/llm-my-feature
```

**2. Implement Changes:**

```bash
# Edit files in src/llm/ and include/llm/
# Follow coding standards (see below)
# Add Doxygen comments to public APIs
# Add tests for new functionality
```

**3. Test Locally:**

```bash
cmake --build --preset windows-release --target llm
ctest --preset windows-release -R llm -V
```

**4. Run Linting & Formatting:**

```bash
# Format code
clang-format -i src/llm/**/*.cpp include/llm/**/*.h

# Static analysis
cppcheck --enable=all src/llm/ --suppress=missingIncludeSystem

# Thread sanitizer (for threading changes)
cmake --preset debug-tsan && ctest --preset debug-tsan -R llm
```

**5. Commit & Push:**

```bash
git add .
git commit -m "Implement: [Component] Brief description

- Detailed change 1
- Detailed change 2

Fixes #5039 | Related to ROADMAP.md#Wave-B"

git push origin feature/llm-my-feature
```

**6. Create Pull Request:**

- Link to related ROADMAP.md entry
- Add test evidence (pass screenshots)
- Mark for code review

### Code Review Checklist

Before submitting for review, verify:

- [ ] Changes follow coding standards (see below)
- [ ] Public APIs have Doxygen comments
- [ ] Thread-safety is documented (if applicable)
- [ ] All tests pass locally
- [ ] No new warnings or static analysis issues
- [ ] Commit messages are clear and link to ROADMAP
- [ ] Changes don't break existing behavior

---

## Testing Strategy

### Test Categories

| Category | Purpose | Location |
|---|---|---|
| **Unit Tests** | Test individual classes/functions in isolation | `tests/llm/unit/` |
| **Integration Tests** | Test component interactions | `tests/llm/integration/` |
| **Thread Safety Tests** | Test concurrent access patterns | `tests/llm/threading/` |
| **Performance Tests** | Benchmark latency/throughput | `tests/llm/performance/` |
| **E2E Tests** | Full inference pipeline | `tests/llm/e2e/` |

### Writing Tests

**Unit Test Template:**

```cpp
#include <gtest/gtest.h>
#include <llm/embedded_llm.h>

class EmbeddedLLMTest : public ::testing::Test {
protected:
    void SetUp() override {
        llm_.initialize();
    }
    
    void TearDown() override {
        llm_.shutdown();
    }
    
    EmbeddedLLM llm_;
};

TEST_F(EmbeddedLLMTest, LoadModelSucceeds) {
    // Arrange
    auto config = LoadConfig{.gpu_memory_fraction = 0.5f};
    
    // Act
    auto status = llm_.loadModel("test_model.gguf", config);
    
    // Assert
    EXPECT_TRUE(status.ok());
    auto models = llm_.listLoadedModels();
    EXPECT_EQ(models.size(), 1);
}

TEST_F(EmbeddedLLMTest, ConcurrentInferenceSuccess) {
    // Test concurrent requests from multiple threads
    llm_.loadModel("test_model.gguf");
    
    std::vector<std::thread> threads;
    std::atomic<int> successful = 0;
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&]() {
            auto result = llm_.complete("Test prompt");
            if (result.ok()) ++successful;
        });
    }
    
    for (auto& t : threads) t.join();
    EXPECT_EQ(successful, 10);
}
```

### Running Tests with Coverage

```bash
# Build with coverage
cmake --preset coverage-llm
cmake --build --preset coverage-llm --target llm_tests --parallel 16

# Run tests and generate coverage report
ctest --preset coverage-llm -R llm -V
cmake --build --preset coverage-llm --target llm_coverage

# View coverage report
open build/coverage-llm/lcov_html/index.html  # macOS
xdg-open build/coverage-llm/lcov_html/index.html  # Linux
start build/coverage-llm/lcov_html/index.html  # Windows
```

**Target Coverage:** Aim for >80% line coverage, >70% branch coverage.

---

## Code Style & Standards

### C++ Standard & Conventions

**Standard:** C++17 (with C++20 features allowed if backward-compatible)

**Naming Conventions:**

```cpp
// Classes: PascalCase
class EmbeddedLLM { };
class ModelLoader { };

// Functions: camelCase
void loadModel(const std::string& path);
Status initialize();

// Member variables: snake_case with trailing underscore
class MyClass {
private:
    std::mutex mutex_;
    int cache_size_;
};

// Constants: UPPER_SNAKE_CASE
constexpr int32_t MAX_BATCH_SIZE = 1024;
constexpr float DEFAULT_TEMPERATURE = 0.7f;

// Type aliases: same as class (PascalCase)
using ModelHandle = uint64_t;
```

### Doxygen Documentation

**All public APIs must have Doxygen comments:**

```cpp
/// @file llm_plugin_manager.cpp
/// @brief Plugin lifecycle management for LLM backends.
/// Manages registration, loading, and lifecycle of inference backend plugins.

class LLMPluginManager {
public:
    /// @brief Register a new inference plugin.
    /// @param info Plugin metadata (name, version, capabilities).
    /// @return Status::kOk on success; kAlreadyExists if plugin already registered.
    /// @pre Plugin must implement the ILLMPlugin interface.
    /// @post Plugin becomes available for model loading via this manager.
    /// @thread_safety Thread-safe; serialized via plugin_manager_mutex_.
    Status registerPlugin(const PluginInfo& info);
    
    /// @brief Load a model using registered plugins.
    /// @param model_path Path to model file (GGUF format).
    /// @param config Loading configuration (GPU memory fraction, threading, etc.).
    /// @return Status::kOk on success.
    /// @throws kNotFound if model file doesn't exist.
    /// @throws kOutOfMemory if GPU/CPU memory insufficient.
    /// @note Acquires read-write lock on model state; may block concurrent loads.
    Status loadModel(
        const std::string& model_path,
        const LoadConfig& config
    );
};
```

### RAII & Resource Management

**Always use RAII for resource management:**

```cpp
// ✅ GOOD: Uses unique_ptr for automatic cleanup
class ModelLoader {
private:
    std::unique_ptr<GPUBuffer> gpu_buffer_;
    std::unique_ptr<std::ifstream> model_file_;
};

// ❌ WRONG: Manual memory management
class BadLoader {
private:
    GPUBuffer* gpu_buffer;  // Will leak if exception thrown
    ~BadLoader() { delete gpu_buffer; }  // Too late!
};

// ✅ GOOD: Lock guard for mutex
{
    std::lock_guard lock(mutex_);
    // Protected section; automatically unlocked
}

// ✅ GOOD: Custom RAII wrapper
struct GPUMemoryGuard {
    GPUMemoryGuard(size_t size) : ptr(allocateGPU(size)) {}
    ~GPUMemoryGuard() { if (ptr) deallocateGPU(ptr); }
    void* ptr;
};
```

### Error Handling

**Use Status return type for error propagation:**

```cpp
// ✅ GOOD: Explicit error handling
auto status = llm_.loadModel("model.gguf");
if (!status.ok()) {
    LOG(ERROR) << "Load failed: " << status.message();
    return status;  // Propagate to caller
}

// ❌ WRONG: Ignoring errors
llm_.loadModel("model.gguf");  // Error silently ignored!

// ✅ GOOD: Exception for exceptional cases
if (batch_size > MAX_BATCH_SIZE) {
    throw std::invalid_argument("Batch size exceeds maximum");
}

// ✅ GOOD: std::optional for optional results
std::optional<Result<EmbeddingVector>> result = llm_.getEmbedding(text);
if (result) {
    if (result->ok()) {
        use_embedding(result->value());
    }
}
```

### Thread Safety Documentation

**Document thread-safety explicitly:**

```cpp
class ThreadSafeQueue {
public:
    /// @brief Push item to queue.
    /// @param item Item to enqueue.
    /// @thread_safety Thread-safe. Multiple threads can push concurrently.
    void push(Item item);
    
    /// @brief Pop item from queue (blocks if empty).
    /// @param timeout_ms Timeout in milliseconds.
    /// @return Item if available; nullopt if timeout.
    /// @thread_safety Thread-safe for concurrent pops.
    std::optional<Item> pop(int32_t timeout_ms = -1);
};
```

### Memory Model Comments

**For complex memory patterns, add detailed comments:**

```cpp
class KVCacheBuffer {
private:
    // SAFETY: Cache pages are reference-counted. Each in-flight request increments
    // refcount; eviction only removes pages with refcount=0. Protected by
    // atomic CAS loop; no mutex needed for fast-path eviction.
    std::vector<std::atomic<int32_t>> page_refcounts_;
    
    // OWNERSHIP: gpu_buffer_ is owned by this class; deallocated in destructor.
    // Move-only semantics (no copy).
    std::unique_ptr<GPUBuffer> gpu_buffer_;
};
```

---

## Common Tasks

### Adding a New Component

1. **Create source file:**
   ```cpp
   // src/llm/my_new_component.cpp
   #include <llm/my_new_component.h>
   
   namespace themisdb::llm {
   
   MyNewComponent::MyNewComponent() : enabled_(false) {}
   
   Status MyNewComponent::initialize() {
       enabled_ = true;
       return Status::OK();
   }
   
   } // namespace themisdb::llm
   ```

2. **Create header:**
   ```cpp
   // include/llm/my_new_component.h
   #pragma once
   
   namespace themisdb::llm {
   
   /// @brief [One-line description]
   /// [Longer description of purpose and behavior]
   class MyNewComponent {
   public:
       MyNewComponent();
       Status initialize();
       // ... other methods
   };
   
   } // namespace themisdb::llm
   ```

3. **Register in CMakeLists.txt:**
   ```cmake
   # In src/llm/CMakeLists.txt
   add_library(themis_llm
       # ... existing sources
       my_new_component.cpp
   )
   ```

4. **Add tests:**
   ```cpp
   // tests/llm/unit/my_new_component_test.cpp
   TEST_F(MyNewComponentTest, Initialization) { }
   ```

5. **Update MODULE_GAPS.md** with new component coverage status.

### Modifying an Existing Component

1. **Understand current behavior:**
   ```bash
   # Review existing tests
   grep -r "ComponentName" tests/llm/
   
   # Read Doxygen comments
   head -50 include/llm/component_name.h
   ```

2. **Write test for new behavior:**
   ```cpp
   TEST(ComponentName, NewBehavior) {
       // Test should fail initially (TDD approach)
   }
   ```

3. **Implement changes:**
   ```cpp
   // Update src/llm/component_name.cpp
   // Update include/llm/component_name.h
   ```

4. **Verify tests pass:**
   ```bash
   ctest --preset windows-release -R component_name -V
   ```

5. **Check thread-safety** (if concurrent access possible):
   ```bash
   cmake --preset debug-tsan && ctest --preset debug-tsan -R component_name -V
   ```

---

## Debugging & Profiling

### Debugging with GDB (Linux/macOS)

```bash
# Build with debug symbols
cmake --preset debug-llm
cmake --build --preset debug-llm --target llm

# Run under GDB
gdb ./build/debug-llm/tests/llm_test_executable

# GDB commands
(gdb) b ModelLoader::load  # Breakpoint at function
(gdb) run                  # Run program
(gdb) c                    # Continue
(gdb) s                    # Step into
(gdb) n                    # Step over
(gdb) p variable           # Print variable
(gdb) bt                   # Backtrace
```

### Debugging with MSVC (Windows)

```bash
# Open Visual Studio solution
cmake --preset windows-debug
start build/windows-debug/ThemisDB.sln

# Set breakpoints in editor
# Press F5 to debug
# Use Debug menu for step/continue/watch
```

### Performance Profiling

**With VTune (Intel):**

```bash
# Build optimized
cmake --preset release-vtune
cmake --build --preset release-vtune --target llm

# Profile
vtune -collect hotspots -result-dir=vtune_results \
    ./build/release-vtune/tests/llm_benchmark

# View results
vtune-gui vtune_results
```

**With Perf (Linux):**

```bash
# Record
perf record -g ./build/linux-release/tests/llm_benchmark

# View
perf report
```

### Memory Profiling

**With Valgrind (Linux):**

```bash
valgrind --leak-check=full --show-leak-kinds=all \
    ./build/linux-release/tests/llm_test_executable

# Expected: "ERROR SUMMARY: 0 errors"
```

**With Address Sanitizer (ASan):**

```bash
cmake --preset debug-asan
cmake --build --preset debug-asan --target llm_tests
ctest --preset debug-asan -R llm
```

---

## FAQ

### Q: Where do I submit feature requests?

**A:** Add to [ROADMAP.md](ROADMAP.md) or [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md) with clear acceptance criteria. Link in your PR.

### Q: How do I handle thread safety?

**A:** Review [THREADING.md](THREADING.md) first. Use std::lock_guard for simple locks, document acquisition order to prevent deadlocks. Test with ThreadSanitizer.

### Q: What if tests are failing locally?

**A:** 
1. Check you're using correct preset: `cmake --preset windows-release`
2. Clean and rebuild: `cmake --build --preset windows-release --clean-first`
3. Run specific test with verbose output: `ctest --preset windows-release -R test_name -V`
4. Check logs: `tail -50 /var/log/themis-llm.log`

### Q: How do I profile performance?

**A:** Use VTune (Windows/Linux) or Perf (Linux). See [Debugging & Profiling](#debugging--profiling) section.

---

**Last Updated:** 2026-08-17 (Phase 6)
**Status:** PRODUCTION (Wave 5 GA)
**Next Update:** Phase 7 (Q4 2026)
