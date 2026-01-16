---
name: Feature - Grammar-Constrained Sampling
about: Implement grammar-constrained sampling for structured outputs
title: '[Feature] Grammar-Constrained Sampling Support'
labels: ['feature', 'llm', 'priority-low', 'phase-3']
assignees: ''
---

## 📋 Overview

Implement grammar-constrained sampling to enforce structured outputs (JSON, YAML, custom formats). Currently marked as TODO pending stable llama.cpp API.

**Related Documentation**: 
- `docs/analysis/LLM_LORA_SYSTEM_ANALYSIS.md` - Advanced features
- `src/llm/llama_wrapper.cpp` - Current sampling implementation

**Current Status**: Waiting for llama.cpp API stabilization (lines 1104, 1131 in llama_wrapper.cpp)

## 🎯 Goals

Enable structured output generation with grammar constraints, ensuring LLM responses conform to predefined formats (JSON schemas, YAML, custom grammars).

## 📊 Current Status

**Completion**: 30% (Grammar loading/caching works, sampling integration pending)

### ✅ Already Implemented
- Grammar loading from GBNF files
- Grammar caching (Phase 3.2)
- Grammar validation
- Pre-compiled grammar support

### ❌ Missing Implementation
- `llama_grammar_sample()` integration (API not yet stable)
- `llama_grammar_accept()` integration (API not yet stable)
- Grammar-constrained token sampling
- Grammar state management
- Token acceptance/rejection based on grammar

## 📝 Detailed Requirements

### 1. llama.cpp Grammar API Integration

**Priority**: 🟢 Low (Waiting for stable API)  
**Effort**: 3-5 days (once API is stable)

**Implementation Tasks**:
- [ ] Monitor llama.cpp releases for stable grammar API
- [ ] Integrate `llama_grammar_sample()` when available
- [ ] Integrate `llama_grammar_accept()` when available
- [ ] Handle grammar state transitions
- [ ] Support grammar reset

**Files to Modify**:
- `src/llm/llama_wrapper.cpp` - Update sampling methods (lines 1104, 1131)
- `src/llm/sampling_strategy.cpp` - Add grammar-aware sampling
- `include/llm/grammar.h` - Update grammar interface

**Code Example** (when API is stable):
```cpp
// Current TODO markers in code:
// Line 1104: // TODO: llama_grammar_sample not yet available in stable llama.cpp
// Line 1131: // TODO: llama_grammar_accept not yet available in stable llama.cpp

// Future implementation:
llama_token LlamaWrapper::sampleWithGrammar(
    llama_context* ctx,
    llama_grammar* grammar,
    const std::vector<llama_token>& candidates
) {
    // Once API is stable, replace with:
    llama_token token = llama_grammar_sample(
        ctx,
        grammar,
        candidates.data(),
        candidates.size()
    );
    
    // Update grammar state
    llama_grammar_accept(grammar, token);
    
    return token;
}
```

### 2. Grammar-Aware Sampling Strategy

**Priority**: 🟢 Low  
**Effort**: 2-3 days

**Implementation Tasks**:
- [ ] Create `GrammarSamplingStrategy` class
- [ ] Filter tokens based on grammar rules
- [ ] Maintain grammar state across tokens
- [ ] Support grammar composition
- [ ] Handle grammar errors gracefully

**Code Example**:
```cpp
class GrammarSamplingStrategy : public SamplingStrategy {
public:
    GrammarSamplingStrategy(
        std::shared_ptr<Grammar> grammar,
        std::unique_ptr<SamplingStrategy> base_strategy
    ) : grammar_(grammar), base_strategy_(std::move(base_strategy)) {}
    
    llama_token sample(
        llama_context* ctx,
        const std::vector<float>& logits
    ) override {
        // Get allowed tokens from grammar
        auto allowed_tokens = grammar_->getAllowedTokens();
        
        // Filter logits based on grammar
        std::vector<float> filtered_logits = logits;
        for (size_t i = 0; i < filtered_logits.size(); ++i) {
            if (allowed_tokens.count(i) == 0) {
                filtered_logits[i] = -INFINITY; // Reject token
            }
        }
        
        // Sample from filtered distribution
        llama_token token = base_strategy_->sample(ctx, filtered_logits);
        
        // Update grammar state
        grammar_->acceptToken(token);
        
        return token;
    }
    
private:
    std::shared_ptr<Grammar> grammar_;
    std::unique_ptr<SamplingStrategy> base_strategy_;
};
```

### 3. JSON Schema Support

**Priority**: 🟢 Medium  
**Effort**: 3-4 days

**Implementation Tasks**:
- [ ] Convert JSON schemas to GBNF grammars
- [ ] Support common JSON schema features
- [ ] Validate generated JSON against schema
- [ ] Handle nested objects and arrays
- [ ] Support optional fields

**Code Example**:
```cpp
std::string convertJSONSchemaToGBNF(const json& schema) {
    std::stringstream gbnf;
    
    gbnf << "root ::= object\n";
    gbnf << "object ::= \"{\" ";
    
    bool first = true;
    for (const auto& [key, value] : schema["properties"].items()) {
        if (!first) gbnf << "\",\" ";
        gbnf << "\"\\\"" << key << "\\\":\" ";
        
        std::string type = value["type"];
        if (type == "string") {
            gbnf << "string";
        } else if (type == "number") {
            gbnf << "number";
        } else if (type == "boolean") {
            gbnf << "boolean";
        } else if (type == "array") {
            gbnf << "array";
        } else if (type == "object") {
            gbnf << "object";
        }
        
        first = false;
    }
    
    gbnf << " \"}\"\n";
    gbnf << "string ::= \"\\\"\" [^\"\\]* \"\\\"\"\n";
    gbnf << "number ::= \"-\"? [0-9]+ (\".\" [0-9]+)?\n";
    gbnf << "boolean ::= \"true\" | \"false\"\n";
    gbnf << "array ::= \"[\" (value (\",\" value)*)? \"]\"\n";
    
    return gbnf.str();
}
```

### 4. Custom Grammar Support

**Priority**: 🟢 Low  
**Effort**: 2-3 days

**Implementation Tasks**:
- [ ] Support GBNF grammar files
- [ ] Grammar validation and compilation
- [ ] Runtime grammar loading
- [ ] Grammar debugging tools
- [ ] Grammar examples and templates

**Example Grammars**:

**JSON Grammar (GBNF)**:
```gbnf
root ::= object
object ::= "{" pair ("," pair)* "}"
pair ::= string ":" value
value ::= string | number | boolean | null | array | object
string ::= "\"" [^"]* "\""
number ::= "-"? [0-9]+ ("." [0-9]+)?
boolean ::= "true" | "false"
null ::= "null"
array ::= "[" (value ("," value)*)? "]"
```

**YAML Grammar (GBNF)**:
```gbnf
root ::= document
document ::= (key ":" value "\n")*
key ::= [a-zA-Z_][a-zA-Z0-9_]*
value ::= string | number | boolean | list
string ::= "\"" [^"]* "\"" | [^\n]+
number ::= "-"? [0-9]+ ("." [0-9]+)?
boolean ::= "true" | "false" | "yes" | "no"
list ::= "\n" ("- " value "\n")+
```

### 5. Grammar State Management

**Priority**: 🟢 Medium  
**Effort**: 2 days

**Implementation Tasks**:
- [ ] Track grammar state across tokens
- [ ] Support state save/restore
- [ ] Handle grammar reset on completion
- [ ] Support multi-turn conversations with grammar
- [ ] Add grammar state visualization (debug)

## ✅ Acceptance Criteria

### Functional Requirements
- [ ] Grammar-constrained sampling works
- [ ] JSON outputs conform to schemas
- [ ] Custom grammars are supported
- [ ] Grammar state is maintained correctly
- [ ] All tests pass

### Performance Requirements
- [ ] Grammar filtering overhead < 2ms per token
- [ ] Grammar state updates < 1ms per token
- [ ] No memory leaks in grammar handling

### Compatibility Requirements
- [ ] Works with stable llama.cpp API
- [ ] Backward compatible with non-grammar sampling
- [ ] Supports all existing sampling strategies

## 🔗 Dependencies

**llama.cpp**: Waiting for stable grammar API  
**Grammar Cache**: Already implemented (Phase 3.2)  
**Sampling Strategies**: Already implemented

## 📈 Implementation Plan

### Prerequisites
- [ ] Monitor llama.cpp releases for stable grammar API
- [ ] Test grammar API in development branch

### Phase 1 (Once API is stable)
- [ ] Day 1-2: llama.cpp grammar API integration
- [ ] Day 3: Grammar-aware sampling strategy
- [ ] Day 4-5: JSON schema support

### Phase 2
- [ ] Day 1-2: Custom grammar support
- [ ] Day 3: Grammar state management
- [ ] Day 4-5: Tests and documentation

## 🔍 Testing Strategy

### Unit Tests
```bash
./build/tests/test_grammar --gtest_filter="*Sampling*"
./build/tests/test_grammar --gtest_filter="*JSON*"
```

### Integration Tests
```cpp
TEST(GrammarSampling, JSONOutput) {
    // Define JSON schema
    json schema = {
        {"type", "object"},
        {"properties", {
            {"name", {{"type", "string"}}},
            {"age", {{"type", "number"}}}
        }}
    };
    
    // Generate with grammar
    std::string prompt = "Generate a person: ";
    std::string output = llama->generateWithGrammar(prompt, schema);
    
    // Parse and validate
    json result = json::parse(output);
    EXPECT_TRUE(result.contains("name"));
    EXPECT_TRUE(result.contains("age"));
    EXPECT_TRUE(result["name"].is_string());
    EXPECT_TRUE(result["age"].is_number());
}
```

## 📚 References

- [llama.cpp Grammar Guide](https://github.com/ggerganov/llama.cpp/blob/master/grammars/README.md)
- [GBNF Format](https://github.com/ggerganov/llama.cpp/tree/master/grammars)
- [JSON Schema](https://json-schema.org/)

## 🏁 Definition of Done

- [ ] llama.cpp stable grammar API available
- [ ] All implementation tasks complete
- [ ] All acceptance criteria met
- [ ] All tests passing
- [ ] Code review completed
- [ ] Documentation updated
- [ ] Examples provided

## 📝 Notes

**Priority**: LOW - Advanced feature, not critical for MVP  
**Blocked By**: llama.cpp stable grammar API  
**Target Version**: v1.4.0 or later  
**Estimated Completion**: 1-2 weeks (once API is stable)

**Current TODOs in Code**:
- Line 1104: `src/llm/llama_wrapper.cpp`
- Line 1131: `src/llm/llama_wrapper.cpp`
