## 🎯 Objective

Integrate EmbeddedLLM with AQL parser and executor to enable real LLM inference for `LLM_GENERATE()`, `LLM_EMBED()`, `LLM_RAG()`, and `LLM_CHAT()` functions.

## 📊 Current Situation

- ❌ AQL LLM functions return placeholder/stub responses
- ❌ No actual model inference
- ✅ Grammar already defines LLM statements (`aql/AQL_GRAMMAR_EXTENDED_v1.3.1.ebnf`)

## 🔧 Implementation Tasks

### 1. AQL Parser Updates
**File:** `src/query/aql_parser.cpp`

- [ ] Parse `LLM INFER` statements
- [ ] Parse `LLM EMBED` statements  
- [ ] Parse `LLM RAG` statements
- [ ] Parse `LLM CHAT` statements
- [ ] Extract parameters: MODEL, LORA, OPTIONS clauses

### 2. AQL Executor Implementation
**File:** `src/query/aql_executor.cpp` + `src/query/aql_llm_functions.cpp` (new)

```cpp
// Example implementation
std::string executeLLMInfer(const std::string& prompt, const json& options) {
    return THEMIS_LLM_GENERATE(prompt);
}

std::vector<float> executeLLMEmbed(const std::string& text) {
    return THEMIS_LLM_EMBED(text);
}
```

## ✅ Acceptance Criteria

1. ✅ `LLM INFER` returns real model responses (not placeholders)
2. ✅ `LLM EMBED` returns normalized vectors
3. ✅ `LLM RAG` works with document context
4. ✅ OPTIONS clause controls generation parameters
5. ✅ All unit tests pass

## ⏱️ Estimation

**Effort:** 2 days  
**Priority:** P0 (Critical)  
**Team:** Backend

## 🏷️ Labels

`P0-critical` `llm` `aql` `backend` `integration`
