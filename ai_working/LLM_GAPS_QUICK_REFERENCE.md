# LLM Module Gaps - Quick Reference Guide

**Purpose:** Quick lookup for gap types, patterns, and implementation approaches  
**Created:** 2026-08-15  

---

## Gap Category Reference

### 1. Thread-Safety Gaps

| Pattern | Indicator | Fix Pattern | Priority |
|---------|-----------|-------------|----------|
| **data_race** | Unsynchronized access to shared state | Add mutex/atomic/lock_guard | CRITICAL |
| **circular_lock_ordering** | Multiple threads acquire locks in different orders | Use consistent lock ordering; document order | HIGH |
| **missing_sync_threads** | Thread access to shared data without synchronization | Add synchronization primitive (mutex/atomic) | HIGH |
| **missing_volatile** | Primitives shared without volatile/atomic | Use std::atomic for simple types | HIGH |
| **double_lock** | Attempting to acquire same lock twice | Use reader-writer lock or redesign | HIGH |

**File Examples:** `async_inference_engine.cpp`, `shared_worker_pool.cpp`, `multi_lora_manager.cpp`

### 2. Resource Management Gaps

| Pattern | Indicator | Fix Pattern | Priority |
|---------|-----------|-------------|----------|
| **resource_leaked_in_exception** | Resource acquired but not released on exception | Use RAII (unique_ptr, shared_ptr, guard classes) | CRITICAL |
| **db_connection_leak** | DB connection not closed in all paths | Wrap in RAII; use connection pooling | HIGH |
| **gpu_memory_leak** | GPU memory allocated but not freed | Wrap in CUDA memory guard; use RAII | HIGH |
| **manual_cleanup** | Manual delete/close outside RAII | Replace with smart pointers; use RAII guards | HIGH |
| **delete_no_nullptr** | delete followed by inconsistent null checks | Use smart pointers; eliminate raw new/delete | MEDIUM |

**File Examples:** `model_loader.cpp`, `llm_model_storage.cpp`, `wiki_index_store.cpp`

### 3. Memory Safety Gaps

| Pattern | Indicator | Fix Pattern | Priority |
|---------|-----------|-------------|----------|
| **null_dereference** | Pointer dereferenced without null check | Add null guard before dereference | HIGH |
| **pointer_arithmetic_unbounded** | Array/pointer access without bounds check | Add bounds validation; use std::span/std::vector | HIGH |
| **uninitialized_access** | Variable used before initialization | Initialize in constructor/declaration | HIGH |
| **unchecked_array_index** | Array access without bounds validation | Add range check; use safe access (at() vs []) | HIGH |
| **use_after_free_gpu** | GPU memory accessed after deallocation | Track lifetime; use RAII; mark freed regions | HIGH |

**File Examples:** `kv_cache_buffer.cpp`, `paged_kv_cache_manager.cpp`, `wiki_index_store.cpp`

### 4. Performance Gaps

| Pattern | Indicator | Fix Pattern | Priority |
|---------|-----------|-------------|----------|
| **copy_overhead** | Unnecessary object copies (parameters, return values) | Use const-ref parameters; move return values | HIGH |
| **string_concat_loop** | String concatenation in loop (O(n²)) | Use std::ostringstream or vector + join | HIGH |
| **o_n_squared** | Nested loops with quadratic complexity | Refactor to linear/logarithmic; use data structures | MEDIUM |
| **lock_contention** | Frequently contested lock | Reduce critical section; use read-write lock; shard state | MEDIUM |
| **repeated_lookup** | Same key lookup multiple times | Cache result; extract before loop | MEDIUM |

**File Examples:** `streaming_handler.cpp`, `prompt_evaluator.cpp`, `ai_orchestrator.cpp`

### 5. Error Handling Gaps

| Pattern | Indicator | Fix Pattern | Priority |
|---------|-----------|-------------|----------|
| **no_retry_logic** | Transient failures not retried | Add exponential backoff retry loop | HIGH |
| **uncaught_exception** | Exception thrown but not caught | Add try-catch with appropriate handler | HIGH |
| **generic_catch** | `catch (...)` without specific type | Catch specific exception types; log + handle | MEDIUM |
| **silent_error_swallow** | Exception caught but not logged/handled | Log error with context; handle or re-throw | MEDIUM |
| **exception_in_destructor** | Destructor throws exception (violates noexcept) | Suppress exceptions in destructor; mark noexcept | MEDIUM |

**File Examples:** `model_downloader.cpp`, `inference_engine_enhanced.cpp`, `llm_plugin_manager.cpp`

### 6. Input Validation & Security Gaps

| Pattern | Indicator | Fix Pattern | Priority |
|---------|-----------|-------------|----------|
| **unvalidated_llm_output** | LLM output used without validation | Validate structure, size, content; sanitize | HIGH |
| **prompt_injection** | User input directly used in prompts | Escape/sanitize user input; use templates | HIGH |
| **unsanitized_llm_input** | LLM input not validated | Validate size, format, token count | HIGH |
| **command_injection** | Shell commands constructed from untrusted input | Use exec arrays; avoid shell interpretation | HIGH |
| **sql_injection** | SQL queries constructed from untrusted input | Use parameterized queries; prepared statements | HIGH |

**File Examples:** `prompt_policy.cpp`, `prompt_optimizer.cpp`, `llm_security_utils.cpp`

### 7. Code Quality Gaps

| Pattern | Indicator | Fix Pattern | Priority |
|---------|-----------|-------------|----------|
| **hardcoded_path** | Filesystem paths hardcoded in code | Use configuration; parameterize paths | MEDIUM |
| **hardcoded_output** | Hardcoded strings, values, or outputs | Extract to constants/config; make configurable | MEDIUM |
| **todo_as_productionlogic** | TODO/FIXME in production code path | Implement properly; defer to Phase N+1 if necessary | MEDIUM |
| **legacy_or_compat_path** | Legacy/compatibility fallback code | Document purpose/sunset; mark with LEGACY comment | MEDIUM |
| **missing_noexcept_on_move** | Move constructor/assignment not marked noexcept | Add noexcept if exception-safe; document if not | MEDIUM |

**File Examples:** `model_downloader.cpp`, `gguf_loader.cpp`, various adapter files

---

## Common Implementation Patterns

### Pattern A: From Manual Resource Management to RAII

**BEFORE:**
```cpp
void loadModels() {
    std::vector<Resource*> resources;
    for (const auto& path : paths) {
        Resource* r = allocateResource(path);
        resources.push_back(r);
    }
    
    // If exception here, resources leak
    processResources(resources);
    
    // Manual cleanup
    for (auto r : resources) {
        delete r;
    }
}
```

**AFTER:**
```cpp
void loadModels() {
    std::vector<std::unique_ptr<Resource>> resources;
    for (const auto& path : paths) {
        resources.push_back(std::make_unique<Resource>(path));
        // Automatic cleanup on exception via destructor
    }
    
    processResources(resources);  // Automatic cleanup at scope end
}
```

### Pattern B: From Data Race to Thread-Safe Access

**BEFORE:**
```cpp
class InferenceEngine {
    std::vector<Model> models_;  // Shared, no sync
    
    void addModel(const Model& m) {
        models_.push_back(m);  // RACE!
    }
};
```

**AFTER:**
```cpp
class InferenceEngine {
    std::vector<Model> models_;
    mutable std::mutex models_mutex_;
    
    void addModel(const Model& m) {
        std::lock_guard<std::mutex> lock(models_mutex_);
        models_.push_back(m);  // THREAD-SAFE
    }
};
```

### Pattern C: From Unbounded Pointer Access to Safe Access

**BEFORE:**
```cpp
float* data = cache.data();
for (size_t i = 0; i < num_elements; ++i) {
    data[i] = compute(i);  // UNSAFE: no bounds check
}
```

**AFTER:**
```cpp
std::span<float> data = cache.span();
if (num_elements > data.size()) {
    throw std::out_of_range("Cache too small");
}
for (size_t i = 0; i < num_elements; ++i) {
    data[i] = compute(i);  // SAFE
}
```

### Pattern D: From String Concat Loop to Efficient Building

**BEFORE:**
```cpp
std::string result;
for (const auto& token : tokens) {
    result += token.text;  // O(n²) — repeated reallocation!
}
return result;
```

**AFTER:**
```cpp
std::ostringstream result;
for (const auto& token : tokens) {
    result << token.text;  // O(n) — efficient streaming
}
return result.str();
```

### Pattern E: From Transient Failure to Retry with Backoff

**BEFORE:**
```cpp
Result infer() {
    return plugin_->invoke(request);  // Fails on transient error
}
```

**AFTER:**
```cpp
Result infer() {
    constexpr int max_retries = 3;
    constexpr int base_delay_ms = 100;
    
    for (int attempt = 0; attempt < max_retries; ++attempt) {
        try {
            return plugin_->invoke(request);
        } catch (const TransientError& e) {
            if (attempt == max_retries - 1) throw;
            int delay = base_delay_ms * (1 << attempt);  // Exponential backoff
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        }
    }
}
```

---

## Gap Fix Workflow Checklist

For each gap fix:

1. **Understand the Gap**
   - [ ] Read gap-verifier classification + rationale
   - [ ] Locate source line in file
   - [ ] Understand context (function, class, module)

2. **Review Current Code**
   - [ ] Examine surrounding code (±10 lines)
   - [ ] Identify related patterns (similar gaps nearby?)
   - [ ] Check if similar code exists elsewhere (duplicate issue?)

3. **Design Fix**
   - [ ] Select fix pattern from this guide
   - [ ] Verify fix doesn't break related code
   - [ ] Consider exception safety (if applicable)
   - [ ] Consider thread safety (if applicable)

4. **Implement Fix**
   - [ ] Apply code change
   - [ ] Add comments explaining fix (if non-obvious)
   - [ ] Update Doxygen docs (if public API changed)

5. **Validate Fix**
   - [ ] Build: `cmake --build --preset windows-release`
   - [ ] Compile: No warnings/errors
   - [ ] Test: `ctest ... -k "llm"` (module tests PASS)
   - [ ] Sanitizers: Run under ASan/UBSan/TSan (if available)

6. **Document Fix**
   - [ ] Update commit message with gap ID + pattern
   - [ ] Link to MODULE_GAPS.md finding
   - [ ] Note any related cleanup or follow-up

---

## Key Resources

| Document | Purpose | Link |
|----------|---------|------|
| MODULE_GAPS.md | Raw gap findings | `src/llm/MODULE_GAPS.md` |
| gap_verifier_report_llm.md | Verified + classified gaps | `ai_working/gap_verifier_report_llm.md` (pending) |
| gap_scanner_verified_llm.json | Structured findings | `ai_working/gap_scanner_verified_llm.json` (pending) |
| LLM_GAPS_IMPLEMENTATION_PLAN.md | Detailed implementation strategy | `ai_working/LLM_GAPS_IMPLEMENTATION_PLAN.md` |
| LLM_GAPS_IMPLEMENTATION_CHECKLIST.md | Step-by-step checklist | `ai_working/LLM_GAPS_IMPLEMENTATION_CHECKLIST.md` |
| ROADMAP.md | Module timeline + phases | `src/llm/ROADMAP.md` |
| ARCHITECTURE.md | Module structure + surfaces | `src/llm/ARCHITECTURE.md` |

---

**Quick Tips:**
- Always use `std::unique_ptr` / `std::shared_ptr` instead of raw `new` / `delete`
- Mark move constructors/assignment as `noexcept` when possible
- Use `const &` for large parameter types; use move for return values
- Use `std::lock_guard` / `std::unique_lock` for automatic mutex management
- Use `std::atomic<T>` for simple shared primitives
- Use `std::span<T>` for bounds-checked array access
- Log errors before throwing/returning error codes
- Test under ASan/UBSan/TSan for memory and thread safety

