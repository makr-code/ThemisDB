# Phase 0.3: llm_aql_handler.cpp Integration

**Objective:** Wire `LLMValidationPipeline` into `llm_aql_handler.cpp::translateNLToAQL()`  
**Duration:** 12 hours  
**Target Date:** 2026-06-22  
**Success Criteria:** All existing tests pass + no circular dependencies

---

## Overview: What Needs to Change

### Current State (BROKEN)

```cpp
std::string LLMAQLHandler::translateNLToAQL(const std::string& nl_query, ...) {
    // ... validation ...
    std::string aql_query = llm_client->invoke(...);  // LLM generates
    
    // ❌ Only string-level validation
    AQLQueryValidator validator;
    auto result = validator.validate(aql_query);
    
    // ❌ No AST parsing
    return aql_query;  // UNVALIDATED returned to user!
}
```

### Required State (FIXED)

```cpp
std::string LLMAQLHandler::translateNLToAQL(const std::string& nl_query, ...) {
    // ... validation ...
    
    // ✅ Use validation pipeline
    auto validation_result = validation_pipeline_->execute(nl_query, schema_context);
    
    if (validation_result.status != LLMValidationStatus::SUCCESS) {
        throw LLMException(LLMErrorCode::INVALID_RESPONSE, 
                           validation_result.error_message);
    }
    
    // ✅ Return ONLY validated AQL
    return validation_result.validated_aql;
}
```

---

## Detailed Task List

### Task 1: Update LLMAQLHandler Header (2 hours)

**File:** `include/aql/llm_aql_handler.h`

**Changes:**

1. **Add includes:**
   ```cpp
   #include "aql/llm_validation_pipeline.h"
   #include "query/aql_parser_service.h"
   ```

2. **Update Config struct:**
   ```cpp
   struct Config {
       // ... existing fields ...
       
       // NEW: Parser service dependency
       std::shared_ptr<query::AQLParserService> parser_service;
       
       // NEW: Validation pipeline config
       LLMValidationPipelineConfig validation_config;
       
       // NEW: Max parse retries (default: 1)
       size_t max_parse_retries = 1;
   };
   ```

3. **Update Impl class:**
   ```cpp
   class LLMAQLHandler::Impl {
       // ... existing fields ...
       
       // NEW: Validation pipeline
       std::shared_ptr<LLMValidationPipeline> validation_pipeline_;
       
       // NEW: Parser service
       std::shared_ptr<query::AQLParserService> parser_service_;
   };
   ```

4. **Update constructor:**
   ```cpp
   LLMAQLHandler(
       const Config& config,
       std::shared_ptr<query::AQLParserService> parser_service  // NEW param
   );
   ```

### Task 2: Implement Refactored translateNLToAQL (3 hours)

**File:** `src/aql/llm_aql_handler.cpp`

**Current implementation (lines 1462-1540):**
- Replace string-level `AQLQueryValidator` with pipeline call
- Remove redundant validation logic
- Add structured error handling

**New implementation pattern:**

```cpp
std::string LLMAQLHandler::translateNLToAQL(
    const std::string& nl_query,
    const std::string& schema_context)
{
    // Input sanitization (keep existing)
    sanitizePromptInput(nl_query, "nl_query", ...);
    sanitizePromptInput(schema_context, "schema_context", ...);
    
    // NEW: Use validation pipeline
    auto validation_result = impl_->validation_pipeline_->execute(
        nl_query,
        schema_context
    );
    
    // Handle result
    switch (validation_result.status) {
        case LLMValidationStatus::SUCCESS:
            return validation_result.validated_aql;
            
        case LLMValidationStatus::EXHAUSTED_RETRIES:
            throw LLMException(LLMErrorCode::INVALID_RESPONSE,
                             validation_result.error_message);
            
        case LLMValidationStatus::REJECTED:
            throw LLMException(LLMErrorCode::INVALID_RESPONSE,
                             validation_result.error_message);
            
        case LLMValidationStatus::LLM_GENERATION_FAILED:
            throw LLMException(LLMErrorCode::LLM_ERROR,
                             validation_result.error_message);
            
        default:
            throw LLMException(LLMErrorCode::INTERNAL_ERROR,
                             "Unknown validation status");
    }
}
```

### Task 3: Update Constructor & Initialization (2 hours)

**File:** `src/aql/llm_aql_handler.cpp`

**Constructor changes:**

```cpp
LLMAQLHandler::LLMAQLHandler(
    const Config& config,
    std::shared_ptr<query::AQLParserService> parser_service)
    : impl_(std::make_unique<Impl>())
{
    if (!parser_service) {
        throw std::invalid_argument("parser_service cannot be null");
    }
    
    impl_->parser_service_ = parser_service;
    
    // Initialize validation pipeline
    impl_->validation_pipeline_ = LLMValidationPipelineFactory::createWithConfig(
        parser_service,
        impl_->llm_client_,  // Existing LLM client
        config.validation_config
    );
    
    // Set up custom strategies if configured
    if (config.enable_custom_feedback) {
        impl_->validation_pipeline_->setFeedbackGenerator(
            [this](const query::ParserDiagnostics& diag) {
                return formatValidationFeedback(diag);
            }
        );
    }
    
    spdlog::info("LLMAQLHandler initialized with parser service and validation pipeline");
}
```

### Task 4: Remove Redundant Validation (2 hours)

**File:** `src/aql/llm_aql_handler.cpp`

**Changes:**

1. **Remove string-level validator:**
   ```cpp
   // DELETE THIS PATTERN:
   AQLQueryValidator aql_validator;
   auto vresult = aql_validator.validate(aql_query);
   if (vresult.hasErrors()) { ... }
   ```

2. **Narrow `aql_query_validator.cpp` scope:**
   - Keep for UI-only syntax highlighting
   - Remove from critical path (translateNLToAQL)
   - Add comment: "Used for IDE/UI purposes only; not for validation"

3. **Remove validation from other methods:**
   - `translateNLToAQLStreaming()` — use pipeline
   - `translateNLToAQLWithExamples()` — use pipeline
   - `translateNLToAQLWithConfidence()` — use pipeline

### Task 5: Update Error Messages (1 hour)

**File:** `src/aql/llm_aql_handler.cpp`

**Changes:**

- Replace generic "validation error" with specific parser diagnostics
- Include line numbers + column positions in error messages
- Add suggestions from parser to user-facing errors

**Example:**

```cpp
// OLD:
throw LLMException(LLMErrorCode::INVALID_RESPONSE, 
                   "Generated AQL failed validation");

// NEW:
std::string error_msg = "Generated AQL parse failed:\n"
                        "  Line " + std::to_string(diag.line_number) + 
                        ": " + diag.error_message;
if (!diag.suggestions.empty()) {
    error_msg += "\nSuggestion: " + diag.suggestions[0];
}
throw LLMException(LLMErrorCode::INVALID_RESPONSE, error_msg);
```

### Task 6: Add Logging (1 hour)

**File:** `src/aql/llm_aql_handler.cpp`

**Add logs at key steps:**

```cpp
// NL query received
spdlog::debug("NL-to-AQL translation starting: nl_query='{}'", nl_query);

// LLM generation
spdlog::debug("LLM generated AQL (attempt N): '{}'", generated_aql);

// Parser validation
spdlog::debug("Parser validation completed: status={}, attempts={}",
              static_cast<int>(result.status), result.attempts_made);

// Success
spdlog::info("NL-to-AQL translation succeeded: generated {} chars",
             result.validated_aql.size());

// Failure with diagnostics
spdlog::warn("NL-to-AQL translation failed: {}", result.error_message);
```

### Task 7: Update Configuration (1 hour)

**File:** `src/aql/llm_aql_handler.cpp`

**Add config option:**

```cpp
struct LLMAQLHandler::Config {
    // NEW: Validation behavior
    bool enable_parser_validation = true;  // Default: enabled
    size_t max_parse_retries = 1;
    uint32_t validation_timeout_ms = 5000;
    
    // NEW: Feedback strategies
    bool enable_custom_feedback = false;
    bool enable_retry_backoff = false;
};
```

### Task 8: Backward Compatibility (2 hours)

**File:** `src/aql/llm_aql_handler.h`

**Ensure no breaking changes:**

1. **Keep existing public methods:**
   - `translateNLToAQL(nl_query, schema_context)`
   - `translateNLToAQLStreaming(...)`
   - `translateNLToAQLWithExamples(...)`

2. **Add deprecation notice (optional):**
   ```cpp
   /// @deprecated Use validateAndTranslateNL() instead
   /// @note This method still works but prefer new pipeline-based version
   std::string translateNLToAQL(...);
   ```

3. **Provide factory with parser:**
   ```cpp
   static std::unique_ptr<LLMAQLHandler> createWithParser(
       const Config& config,
       std::shared_ptr<query::AQLParserService> parser_service
   );
   ```

### Task 9: Verification & Regression Testing (2 hours)

**File:** `tests/aql/test_llm_aql_handler_focused.cpp`

**Verify existing tests:**
- All existing tests should pass unchanged
- New behavior validated by new tests

**Key tests to pass:**
- `TranslateNLToAQLSuccessful` — validates valid AQL is returned
- `TranslateNLToAQLInvalidFails` — validates invalid AQL throws
- `TranslateNLToAQLWithConfidence` — confidence scoring still works
- `TranslateNLToAQLStreaming` — streaming still works
- `TranslateNLToAQLWithExamples` — examples still work

---

## Code Patterns to Use

### Pattern 1: Dependency Injection

```cpp
// ❌ DON'T create parser directly
auto parser = std::make_unique<AQLParser>();

// ✅ DO inject as dependency
std::shared_ptr<query::AQLParserService> parser_service = ...;
impl_->validation_pipeline_ = 
    LLMValidationPipelineFactory::createWithConfig(
        parser_service, llm_client, config
    );
```

### Pattern 2: Result Handling

```cpp
// ❌ DON'T ignore validation status
auto result = pipeline->execute(...);
return result.validated_aql;  // Might be empty!

// ✅ DO check status first
auto result = pipeline->execute(...);
if (result.status != LLMValidationStatus::SUCCESS) {
    throw std::runtime_error(result.error_message);
}
return result.validated_aql;
```

### Pattern 3: Error Propagation

```cpp
// ❌ DON'T lose diagnostic info
try {
    return pipeline->execute(...);
} catch (...) {
    throw LLMException(LLMErrorCode::INVALID_RESPONSE, "Failed");
}

// ✅ DO preserve diagnostics
auto result = pipeline->execute(...);
if (result.status != LLMValidationStatus::SUCCESS) {
    std::string msg = result.parser_diagnostics.error_message;
    if (!result.parser_diagnostics.suggestions.empty()) {
        msg += " Suggestion: " + result.parser_diagnostics.suggestions[0];
    }
    throw LLMException(LLMErrorCode::INVALID_RESPONSE, msg);
}
```

---

## Build & Test Commands

### Compile Check:

```powershell
cd c:\Projects\ThemisDB
cmake --build --preset windows-release --target themis_aql --parallel 16
```

### Run LLM Handler Tests:

```powershell
ctest --preset windows-release -R test_llm_aql_handler_focused -V --output-on-failure
```

### Run All AQL Tests:

```powershell
ctest --preset windows-release -R "test_aql|test_llm" -V --output-on-failure
```

---

## Checklist

- [ ] Update `include/aql/llm_aql_handler.h` with new includes + config
- [ ] Refactor `src/aql/llm_aql_handler.cpp::translateNLToAQL()`
- [ ] Update constructor to accept parser_service
- [ ] Initialize validation_pipeline in Impl
- [ ] Remove redundant AQLQueryValidator calls
- [ ] Update error messages with diagnostics
- [ ] Add logging at key steps
- [ ] Update configuration options
- [ ] Ensure backward compatibility
- [ ] Verify no circular dependencies
- [ ] Run existing unit tests (must all pass)
- [ ] Code review + approval
- [ ] Merge to `develop` branch

---

## Success Criteria

✅ **Code Quality:**
- No compile errors or warnings
- No circular dependencies (verify with CMake)
- Follows SOC/OOP principles

✅ **Functionality:**
- All existing tests pass
- Invalid AQL rejected with parser diagnostics
- Retry logic works (parse fails → LLM retries → succeeds)
- Timeout enforced

✅ **Documentation:**
- API docs updated (Doxygen)
- Inline comments explain validation pipeline
- Error messages clear to end users

✅ **Performance:**
- Parser latency < 5ms p99
- Retry overhead < 1s per attempt
- No resource leaks (concurrent safety)

---

## Risk Mitigation

| Risk | Mitigation |
|------|-----------|
| Circular dependency | Use forward declarations; verify with CMake dep graph |
| Breaking changes | Keep old APIs; add deprecation notices; test backward compat |
| Performance regression | Profile before + after; set performance targets |
| Concurrency issues | Use thread-safe factories; test with concurrent calls |
| Configuration errors | Validate config in constructor; fail fast |

---

## Timeline

- **Phase 0.1:** ✅ 2026-06-18 (Integration Contract)
- **Phase 0.2:** ✅ 2026-06-20 (C++ Interfaces + Tests)
- **Phase 0.3:** 📋 2026-06-22 (This phase - llm_aql_handler.cpp refactor)
- **Phase 0.4:** 📋 2026-06-25 (Integration tests)
- **Phase 0.5:** 📋 2026-06-28 (Documentation)

**Phase 0.3 Kickoff:** After Phase 0.2 unit tests pass + code review approved.
