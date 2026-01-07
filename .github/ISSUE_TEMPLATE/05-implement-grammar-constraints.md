---
name: "🟢 Implement Grammar-Constrained Generation (Phase 3.2)"
about: Guarantee valid JSON/XML/structured output using EBNF grammars
title: "[P2] Implement Grammar-Constrained Generation - Guaranteed Structured Output"
labels: ["priority: medium", "type: feature", "component: llm", "phase-3"]
assignees: []
---

## Priority
🟢 **MEDIUM** - P2 (Phase 3, high value feature)

## Overview

Implement grammar-constrained generation to **guarantee valid structured output** (JSON, XML, custom formats) using EBNF grammars, eliminating parsing errors and retry logic.

**Impact:**
- 95-99% valid outputs (vs 60-70% without constraints)
- No retry logic needed
- Faster generation (only valid tokens sampled)
- Better UX for API integration

## Depends On

- ⚠️ **Blocked by Issue #1** (Fix Compilation)
- 🟢 **Optional:** Issue #2, #3 (Phase 1 & 2 - can proceed in parallel)
- 🟢 **Optional:** Issue #4 (RoPE Scaling - independent)

## Documentation

📄 **Complete implementation guide available:**
`docs/en/llm/GRAMMAR_CONSTRAINED_GENERATION.md` (12.8KB)

## Feature Requirements

### Built-in Grammars

Provide ready-to-use grammars for common formats:

1. **JSON** (strict and relaxed)
2. **XML** (with/without self-closing tags)
3. **CSV** (quoted and unquoted)
4. **Agent Format** (ReAct, thought/action/observation)

### Custom Grammar Support

Allow users to define custom EBNF grammars:

```ebnf
root ::= object
object ::= "{" pair ("," pair)* "}"
pair ::= string ":" value
string ::= "\"" [^"]* "\""
value ::= string | number | object | array
```

## Implementation Tasks

### 1. Grammar Infrastructure (include/llm/grammar.h)

- [ ] Create Grammar class
  ```cpp
  class Grammar {
  public:
      Grammar(const std::string& ebnf_text, const std::string& start_symbol);
      ~Grammar();
      
      bool isValid() const;
      std::string getError() const;
      
      // Internal: llama_grammar handle
      llama_grammar* getHandle() const;
      
  private:
      llama_grammar* grammar_ = nullptr;
      std::string error_;
  };
  ```

- [ ] Add GrammarCache for compiled grammars
  ```cpp
  class GrammarCache {
  public:
      std::shared_ptr<Grammar> get(const std::string& name);
      void put(const std::string& name, std::shared_ptr<Grammar> grammar);
      void clear();
      
  private:
      std::unordered_map<std::string, std::shared_ptr<Grammar>> cache_;
      std::mutex mutex_;
  };
  ```

### 2. Built-in Grammars (src/llm/grammars/)

- [ ] Create grammar files:
  - `json_strict.ebnf` - Strict JSON
  - `json_relaxed.ebnf` - JSON with trailing commas, etc.
  - `xml.ebnf` - XML format
  - `csv.ebnf` - CSV format
  - `react_agent.ebnf` - ReAct agent format

- [ ] Embed grammars in binary (CMake resource compilation)

### 3. LlamaWrapper Integration (src/llm/llama_wrapper.{h,cpp})

- [ ] Add generateConstrained() method
  ```cpp
  std::string generateConstrained(
      const std::string& prompt,
      std::shared_ptr<Grammar> grammar,
      const GenerationParams& params
  );
  ```

- [ ] Implement token sampling with grammar
  ```cpp
  // In generation loop
  llama_sample_grammar(ctx_, &candidates_p, grammar->getHandle());
  llama_token token = llama_sample_token(ctx_, &candidates_p);
  llama_grammar_accept_token(ctx_, grammar->getHandle(), token);
  ```

- [ ] Add configuration support
  ```cpp
  struct GrammarConfig {
      bool enabled = false;
      std::string default_grammar = "json";
      std::string custom_grammars_path = "/grammars/";
      bool cache_grammars = true;
      int max_cached_grammars = 100;
  };
  ```

### 4. Configuration Files

- [ ] Update `config/llm_config.example.yaml`
  ```yaml
  optimizations:
    grammar_constraints:
      enabled: true
      default_grammar: "json"
      custom_grammars_path: "/grammars/"
      cache_grammars: true
  ```

- [ ] Update `config/llm_config.production.yaml`

### 5. HTTP API Integration

- [ ] Add grammar parameter to generate endpoint
  ```cpp
  POST /api/v1/llm/generate
  {
      "prompt": "Generate user profile",
      "grammar": "json",  // Built-in grammar
      "max_tokens": 256
  }
  ```

- [ ] Support custom grammar upload
  ```cpp
  POST /api/v1/llm/generate
  {
      "prompt": "Plan next action",
      "grammar_file": "/grammars/custom.ebnf",
      "max_tokens": 512
  }
  ```

### 6. CLI Interface

- [ ] Add `--grammar` flag
  ```bash
  themisdb llm generate \
    --prompt "Generate JSON user data" \
    --grammar json \
    --model mistral-7b
  ```

- [ ] Add `--grammar-file` flag
  ```bash
  themisdb llm generate \
    --prompt "Plan action" \
    --grammar-file /grammars/agent.ebnf \
    --model mistral-7b
  ```

### 7. Testing

- [ ] Unit tests for Grammar class
- [ ] Test all built-in grammars
- [ ] Test custom grammar loading
- [ ] Test grammar caching
- [ ] Validate output correctness (100 iterations)
- [ ] Performance tests (overhead measurement)

## Expected Performance

### Reliability

| Grammar | Without | With | Improvement |
|---------|---------|------|-------------|
| JSON    | 67%     | 99%  | +48%        |
| XML     | 72%     | 98%  | +36%        |
| Agent   | 45%     | 96%  | +113%       |

### Latency

| Operation | Time | Notes |
|-----------|------|-------|
| Grammar compilation | 10-50ms | One-time, cached |
| Per-token overhead | +5-10% | Slight slowdown |
| End-to-end (with retries) | -22% | Faster due to no retries |

### Memory

| Grammar Type | Memory |
|--------------|--------|
| Simple (JSON) | ~50 KB |
| Complex (Agent) | ~500 KB |
| Cache (100 grammars) | ~10-50 MB |

## Acceptance Criteria

### Functional
- [ ] Built-in grammars work (JSON, XML, CSV, Agent)
- [ ] Custom grammar loading works
- [ ] Grammar caching works
- [ ] Proper error messages for invalid grammars
- [ ] Fallback to unconstrained generation on grammar error

### Quality
- [ ] JSON grammar: 95-99% valid outputs ✅
- [ ] XML grammar: 95-99% valid outputs ✅
- [ ] Custom grammars: Works as defined ✅
- [ ] No false positives (generated text matches grammar)

### Performance
- [ ] Grammar compilation: < 100ms
- [ ] Per-token overhead: < 15%
- [ ] End-to-end faster (no retries): -20 to -30% ✅
- [ ] Memory usage reasonable: < 100 MB for 100 cached grammars

### Documentation
- [ ] API documentation with examples
- [ ] Grammar writing guide (EBNF)
- [ ] Built-in grammar reference
- [ ] Migration guide for existing code

## Deliverables

- [ ] Implementation in `include/llm/grammar.h`, `src/llm/grammar.cpp`
- [ ] Built-in grammar files in `src/llm/grammars/`
- [ ] LlamaWrapper integration
- [ ] HTTP API support
- [ ] CLI support
- [ ] Test suite in `tests/llm/test_grammar.cpp`
- [ ] Benchmark script
- [ ] User documentation
- [ ] Example grammars

## Estimated Effort

**Time:** 2-3 weeks
**Complexity:** Medium (EBNF parsing, token filtering)
**Dependencies:** Issue #1 (compilation)

## Use Cases

1. **API Responses**
   - Guarantee valid JSON for REST APIs
   - No error handling needed

2. **Data Extraction**
   - Extract structured data from text
   - Reliable parsing

3. **Structured Agents**
   - ReAct agents with reliable action parsing
   - Tool-calling agents

4. **Code Generation**
   - Generate valid syntax
   - Function signatures, SQL queries

5. **Form Filling**
   - Extract form data from documents
   - Structured templates

## Related Issues

- Depends on: #1 (Fix Compilation)
- Related: #4 (RoPE Scaling - independent)
- Related: #6 (Vision Support - independent)

## References

- Docs: `docs/en/llm/GRAMMAR_CONSTRAINED_GENERATION.md`
- llama.cpp grammars: https://github.com/ggerganov/llama.cpp/blob/master/grammars/README.md
- EBNF spec: https://www.w3.org/TR/REC-xml/#sec-notation
- Example grammars: https://github.com/ggerganov/llama.cpp/tree/master/grammars

## Success Criteria

| Metric | Target | Status |
|--------|--------|--------|
| JSON Valid Rate | 95-99% | ⏳ |
| XML Valid Rate | 95-99% | ⏳ |
| Agent Valid Rate | 95-99% | ⏳ |
| Latency Overhead | < 15% | ⏳ |
| End-to-End Speedup | 20-30% | ⏳ |

All metrics met = ✅ Feature Complete

## Example Grammar

```ebnf
# ReAct Agent Grammar
root ::= thought "\n" action "\n" observation
thought ::= "Thought: " text
action ::= "Action: " action_name " " json_args
observation ::= "Observation: " text

action_name ::= "search" | "calculate" | "finish"

json_args ::= "{" pair ("," pair)* "}"
pair ::= "\"" [a-z_]+ "\"" ":" value
value ::= "\"" text "\"" | number

text ::= [^\n]*
number ::= [0-9]+
```
