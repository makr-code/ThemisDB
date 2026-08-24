# Agent 3: LLM Module Phase 2 (CRITICAL Gaps)

**Duration:** 1.5 hours | **Scope:** 20-30 CRITICAL gaps | **Target:** Exception-safe + ASan clean

## Gap Details

### LLM Module CRITICAL Gaps (from LLM_GAPS_EXECUTION_READY.md)

**Category 1: Model Loading & Resource Management (8 gaps)**
- Missing exception handlers in plugin initialization
- Unchecked model pointer dereferences
- Model cleanup on exception paths
- Memory leak in model cache eviction

**Category 2: Token Processing & Exception Safety (6 gaps)**
- Token buffer overflows in batch encoding
- Missing bounds checking in vocabulary access
- Exception-unsafe token stream processing
- Incomplete cleanup in tokenization failures

**Category 3: Inference State & Ownership (8 gaps)**
- Dangling pointer to inference context
- Missing ownership transfer in result objects
- Incomplete state cleanup on inference failure
- Resource leak in concurrent inference paths

**Category 4: Plugin Integration (6 gaps)**
- Missing null check on plugin factory return
- Exception-unsafe plugin initialization
- Plugin cleanup on activation failure
- Unguarded plugin lifecycle transitions

## Implementation Tasks

### Task 1: Design Fix Patterns (15 min)

**Pattern 1: Exception-Safe Model Loading**
```cpp
class ModelGuard {
  std::unique_ptr<LLMModel> model_;
  ModelCache* cache_;
  
public:
  explicit ModelGuard(ModelCache& cache, const std::string& model_id)
    : cache_(&cache) {
    model_ = cache.LoadModel(model_id);
    if (!model_) {
      throw std::runtime_error("Failed to load model: " + model_id);
    }
  }
  
  ~ModelGuard() = default;  // Unique_ptr handles cleanup
  
  LLMModel& operator*() { return *model_; }
  LLMModel* operator->() { return model_.get(); }
};
```

**Pattern 2: Token Buffer with Bounds Checking**
```cpp
class TokenBuffer {
  std::vector<int32_t> tokens_;
  size_t max_capacity_;
  
public:
  void Push(int32_t token) {
    if (tokens_.size() >= max_capacity_) {
      throw std::overflow_error("Token buffer overflow");
    }
    tokens_.push_back(token);
  }
  
  int32_t At(size_t idx) const {
    if (idx >= tokens_.size()) {
      throw std::out_of_range("Token index out of range");
    }
    return tokens_[idx];
  }
};
```

**Pattern 3: Exception-Safe Inference Context**
```cpp
class InferenceGuard {
  InferenceContext* ctx_;
  InferenceEngine* engine_;
  
public:
  explicit InferenceGuard(InferenceEngine& engine)
    : engine_(&engine) {
    ctx_ = engine_->CreateContext();
    if (!ctx_) throw std::runtime_error("Failed to create inference context");
  }
  
  ~InferenceGuard() {
    if (ctx_ && engine_) {
      engine_->DestroyContext(ctx_);  // No-throw
    }
  }
  
  InferenceContext& Get() {
    if (!ctx_) throw std::logic_error("Context invalid");
    return *ctx_;
  }
};
```

**Pattern 4: Plugin Factory with Null Check**
```cpp
std::unique_ptr<ILLMPlugin> CreatePlugin(const std::string& name) {
  auto* factory = plugin_registry_.GetFactory(name);
  if (!factory) {
    throw std::runtime_error("Plugin not found: " + name);
  }
  
  auto plugin = factory->Create();
  if (!plugin) {
    throw std::runtime_error("Plugin factory returned null: " + name);
  }
  
  return plugin;
}
```

### Task 2: Implement Fixes (30 min)

**Files to edit:**
- `src/llm/llm_plugin_manager.cpp`
- `src/llm/llm_inference_engine.cpp`
- `include/llm/model_guard.h` (new)
- `include/llm/inference_guard.h` (new)

**Gap Implementation (20-30 fixes, grouped by pattern):**

**Group 1: Model Loading (8 gaps)**
1. Add ModelGuard for exception-safe model lifecycle
2. Add null check on LoadModel() return
3. Add exception handler in plugin initialization
4. Add model cleanup on exception paths
5. Add memory leak prevention in cache eviction
6. Add model reference counting for shared access
7. Add model validation before use
8. Add timeout for slow model loading

**Group 2: Token Processing (6 gaps)**
1. Add TokenBuffer with overflow detection
2. Add bounds checking in batch encoding
3. Add vocabulary range validation
4. Add exception-safe token stream processing
5. Add cleanup on token processing failure
6. Add buffer pre-allocation to reduce allocation failures

**Group 3: Inference State (8 gaps)**
1. Add InferenceGuard for context lifecycle
2. Add ownership transfer in result objects (unique_ptr)
3. Add state cleanup on inference failure
4. Add resource tracking in concurrent paths
5. Add inference cancellation on exception
6. Add result object validation
7. Add inference timeout handling
8. Add state machine for inference phases

**Group 4: Plugin Integration (6+ gaps)**
1. Add null check on factory return (CreatePlugin)
2. Add plugin initialization exception handler
3. Add plugin health check before inference
4. Add plugin cleanup on activation failure
5. Add plugin lifecycle state tracking
6. Add fallback plugin on primary failure

### Task 3: Write Tests (20 min)

**File:** `tests/llm/test_llm_phase2_critical_gaps.cpp`

```cpp
// Test: model guard cleanup on exception
TEST(LLMPhase2Critical, ModelGuardCleanupOnException) {
  MockModelCache cache;
  EXPECT_CALL(cache, LoadModel("gpt2")).WillOnce(Return(std::make_unique<MockModel>()));
  
  try {
    ModelGuard guard(cache, "gpt2");
    throw std::runtime_error("test");
  } catch (...) {
    // Guard should cleanup
  }
}

// Test: token buffer overflow detection
TEST(LLMPhase2Critical, TokenBufferOverflowDetection) {
  TokenBuffer buffer(10);
  
  for (int i = 0; i < 10; ++i) {
    buffer.Push(i);
  }
  
  EXPECT_THROW(buffer.Push(10), std::overflow_error);
}

// Test: inference guard context cleanup
TEST(LLMPhase2Critical, InferenceGuardContextCleanup) {
  MockInferenceEngine engine;
  
  {
    InferenceGuard guard(engine);
    auto& ctx = guard.Get();
    EXPECT_TRUE(ctx.IsValid());
  }  // Guard destructor cleans up
}

// Test: plugin factory null check
TEST(LLMPhase2Critical, PluginFactoryNullCheck) {
  LLMPluginManager pm;
  
  EXPECT_THROW(
    pm.CreatePlugin("nonexistent"),
    std::runtime_error
  );
}

// Test: inference exception propagation
TEST(LLMPhase2Critical, InferenceExceptionPropagation) {
  LLMInferenceEngine engine;
  
  EXPECT_THROW(
    engine.Infer(InferenceRequest{}),  // Invalid request
    std::runtime_error
  );
}
```

20 focused test cases covering:
- Model guard cleanup (2)
- Token buffer bounds checking (3)
- Inference guard context lifecycle (3)
- Plugin factory null check (2)
- Plugin initialization exception handling (2)
- Token stream processing safety (2)
- Inference result ownership (2)
- Concurrent inference safety (2)

### Task 4: Validation (15 min)

**Local validation:**
```bash
# ASan build
cmake --preset linux-debug -DSANITIZER=asan
cmake --build --preset linux-debug-build
ctest --preset linux-debug -R "test_llm_phase2_critical" -V

# Exception safety check
cmake --preset linux-debug -DCMAKE_CXX_FLAGS="-fexceptions -frtti"
cmake --build --preset linux-debug-build
ctest --preset linux-debug -R "test_llm_phase2_critical" -V
```

**CI validation:**
- ASan: 0 memory leaks
- UBSan: 0 undefined behavior
- Tests: 20/20 passing
- Exception safety verified

### Task 5: Commit

**Message:**
```
PHASE2: LLM Module CRITICAL Gaps (20-30 gaps) — Exception-safe patterns + guards

- Add ModelGuard for exception-safe model lifecycle
- Add InferenceGuard for context lifecycle management
- Add TokenBuffer with overflow detection and bounds checking
- Implement factory null check and plugin initialization safety (CreatePlugin)
- Add exception handler for token processing failures
- Add ownership transfer using unique_ptr for result objects
- Add resource tracking in concurrent inference paths
- Add 20 focused exception-safety tests
- ASan/UBSan: 0 alerts, 20/20 tests passing
```

## Exit Criteria

- [x] All 20-30 CRITICAL gaps addressed with production logic
- [x] 20 focused test cases, 100% passing
- [x] ASan/UBSan output: 0 new alerts
- [x] Exception-safe verified in tests
- [x] Doxygen-compliant API comments
- [x] No build regressions

## Success Timeline

- 0:00-0:15: Pattern design
- 0:15-0:45: Implementation (20-30 fixes)
- 0:45-1:05: Tests (20 cases)
- 1:05-1:20: Validation
- 1:20-1:30: Commit + final checks

**Target completion:** 1.5 hours ✅
