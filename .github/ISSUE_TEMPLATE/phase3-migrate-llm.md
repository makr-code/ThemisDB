---
name: Phase 3 - Migrate LLM Module to Result<T>
about: Migrate LLM integration methods to Result<T>
title: '[Phase 3] Migrate LLM Module to Result<T>'
labels: ['enhancement', 'error-handling', 'phase-3', 'llm', 'ai']
assignees: ''
---

## 📋 Overview

Migrate LLM (Large Language Model) integration methods from legacy error patterns to `Result<T>`.

**Current Status:** ~5% complete  
**Target:** ~12-15 methods  
**Priority:** 🟡 Medium

## 🎯 Goals

- Better error handling for LLM API calls
- Clear error messages for rate limits, timeouts, invalid requests
- Type-safe error propagation from LLM services

## 🔨 Methods to Migrate (~12-15 methods)

### LLM Plugin Management
- [ ] `loadLLMPlugin()` - Load LLM provider plugin
- [ ] `unloadLLMPlugin()` - Unload plugin
- [ ] `getLLMPlugin()` - Retrieve loaded plugin
- [ ] `listLLMPlugins()` - List available providers

### LLM Query/Inference
- [ ] `generateEmbedding()` - Generate vector embedding
- [ ] `generateCompletion()` - Text completion
- [ ] `generateChatResponse()` - Chat interaction
- [ ] `batchGenerate()` - Batch inference

### Model Management
- [ ] `loadModel()` - Load LLM model
- [ ] `unloadModel()` - Unload model
- [ ] `getModelInfo()` - Retrieve model metadata
- [ ] `validateModel()` - Validate model compatibility

### Configuration
- [ ] `configureLLM()` - Configure LLM settings
- [ ] `setAPIKey()` - Set API credentials
- [ ] `testConnection()` - Test LLM service connection

## 📝 Implementation Strategy

### Error Codes to Use
```cpp
ERR_LLM_API_ERROR          // LLM service API error
ERR_LLM_RATE_LIMIT         // Rate limit exceeded
ERR_LLM_TIMEOUT            // Request timeout
ERR_LLM_INVALID_MODEL      // Model not found/invalid
ERR_LLM_AUTHENTICATION     // Auth/API key error
ERR_LLM_QUOTA_EXCEEDED     // Quota limit reached
ERR_API_INVALID_REQUEST    // Invalid request format
ERR_NETWORK_ERROR          // Network communication failed
```

### Migration Pattern

**Before:**
```cpp
std::vector<float> generateEmbedding(const std::string& text) {
    if (text.empty()) {
        return {};  // Empty vector = error
    }
    
    auto response = llm_client_->embed(text);
    if (!response.success) {
        return {};  // No error context
    }
    
    return response.embedding;
}
```

**After:**
```cpp
Result<std::vector<float>> generateEmbedding(const std::string& text) {
    if (text.empty()) {
        return Err<std::vector<float>>(
            ERR_API_INVALID_REQUEST,
            "Cannot generate embedding: text is empty"
        );
    }
    
    auto response = llm_client_->embed(text);
    if (!response.success) {
        return Err<std::vector<float>>(
            ERR_LLM_API_ERROR,
            fmt::format("LLM API error: {} (code: {})", 
                       response.error_message, response.status_code)
        );
    }
    
    return Ok(response.embedding);
}
```

## 📋 Implementation Checklist

### Phase 1: Plugin Management
- [ ] Update `include/llm/llm_plugin_manager.h` signatures
- [ ] Update `src/llm/llm_plugin_manager.cpp` implementations
- [ ] Add LLM-specific error codes if needed
- [ ] Update call sites
- [ ] Update tests

### Phase 2: Inference Methods
- [ ] Migrate embedding generation
- [ ] Migrate completion/chat methods
- [ ] Add timeout and retry logic
- [ ] Handle rate limiting gracefully
- [ ] Update tests

### Phase 3: Model Management
- [ ] Migrate model loading/unloading
- [ ] Migrate model validation
- [ ] Update metadata retrieval
- [ ] Update tests

### Phase 4: Configuration
- [ ] Migrate configuration methods
- [ ] Migrate connection testing
- [ ] Add credential validation
- [ ] Update tests

## 🧪 Testing Requirements

### Unit Tests
```cpp
TEST(LLMModuleTest, EmbeddingWithEmptyText) {
    auto result = llm.generateEmbedding("");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), ERR_API_INVALID_REQUEST);
}

TEST(LLMModuleTest, RateLimitError) {
    // Mock rate limit response
    auto result = llm.generateCompletion(prompt);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), ERR_LLM_RATE_LIMIT);
    EXPECT_THAT(result.error().message(), HasSubstr("retry after"));
}

TEST(LLMModuleTest, TimeoutHandling) {
    auto result = llm.generateCompletion(very_long_prompt);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), ERR_LLM_TIMEOUT);
}
```

### Integration Tests
- [ ] Test with real LLM services (dev keys)
- [ ] Test rate limit handling
- [ ] Test timeout scenarios
- [ ] Test error recovery

### Mock Tests
- [ ] Mock LLM service responses
- [ ] Test various error codes
- [ ] Test retry logic

## 📚 Documentation Updates

- [ ] Update LLM module documentation
- [ ] Document error codes and handling
- [ ] Add troubleshooting guide for LLM errors
- [ ] Document rate limit recommendations

### Error Handling Guidelines

**Rate Limits:**
- Return `ERR_LLM_RATE_LIMIT` with retry-after info
- Implement exponential backoff
- Document rate limit recommendations

**Timeouts:**
- Return `ERR_LLM_TIMEOUT` with timing info
- Allow configurable timeouts
- Document timeout recommendations

**Authentication:**
- Return `ERR_LLM_AUTHENTICATION` for auth errors
- Don't expose API keys in error messages
- Document credential setup

## 🎯 Success Criteria

- [ ] All LLM methods use `Result<T>`
- [ ] Rate limits handled gracefully
- [ ] Timeouts handled properly
- [ ] All tests pass
- [ ] Documentation complete
- [ ] Error messages are actionable

## 📊 Progress Tracking

**Expected Effort:** 1-2 weeks  
**Priority:** Medium (AI features)

### Weekly Goals
- [ ] Week 1: Plugin management + inference (8 methods)
- [ ] Week 2: Model management + config + testing (7 methods)

## 🔗 Related

- **Parent Issue:** #XXX (Error Handling Migration - Master Tracking)
- **Plugin System:** Related to PluginManager migration
- **Documentation:** ERROR_HANDLING_MIGRATION_STATUS.md

## 💡 Notes

- **External Dependencies:** LLM services are external, network-dependent
- **Rate Limits:** Critical to handle gracefully
- **Cost:** LLM API calls cost money - good error handling saves costs
- **Testing:** Use mocks for most tests, real API sparingly
- **Monitoring:** Add metrics for LLM error rates
