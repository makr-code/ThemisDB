---
name: "🟢 Implement RoPE Scaling (Phase 3.1)"
about: Extend context window from 4K to 32K tokens
title: "[P2] Implement RoPE Scaling - Extended Context (4K → 32K tokens)"
labels: ["priority: medium", "type: feature", "component: llm", "phase-3"]
assignees: []
---

## Priority
🟢 **MEDIUM** - P2 (Phase 3, easiest feature)

## Overview

Implement RoPE (Rotary Position Embedding) Scaling to extend context window from **4K → 32K tokens** (8x increase), enabling long document processing, code repository analysis, and extended conversations.

**Why First in Phase 3:**
- Easiest to implement (1-2 weeks)
- llama.cpp already supports RoPE scaling
- Mostly configuration changes
- High value for long-context use cases

## Depends On

- ⚠️ **Blocked by Issue #1** (Fix Compilation)
- 🟢 **Optional:** Issue #2, #3 (Phase 1 & 2 testing - can proceed in parallel)

## Documentation

📄 **Complete implementation guide available:**
`docs/en/llm/ROPE_SCALING_IMPLEMENTATION.md` (11.7KB)

## Feature Requirements

### Scaling Methods

Implement support for all 4 RoPE scaling methods:

1. **Linear Scaling** (simplest)
   - Good for 2-4x scaling
   - Fast, but quality degrades at high factors

2. **NTK-Aware Scaling** (better)
   - Good for 4x scaling
   - Better quality than linear

3. **YaRN Scaling** (best - recommended)
   - Best quality at high factors (8-16x)
   - Minimal perplexity increase (+7.5% at 8x)

4. **Dynamic Scaling** (automatic)
   - Adapts to input length
   - Good balance of flexibility and quality

### Configuration Interface

```yaml
llm_plugins:
  llamacpp:
    context:
      rope_scaling:
        enabled: true
        method: "yarn"  # linear, ntk, yarn, dynamic
        max_context: 32768
        original_context: 4096
        
        # YaRN-specific parameters
        yarn:
          ext_factor: 1.0
          attn_factor: 1.0
          beta_fast: 32.0
          beta_slow: 1.0
```

## Implementation Tasks

### 1. Configuration (src/llm/llama_wrapper.h)

- [ ] Add RoPE scaling config struct
  ```cpp
  struct RopeScalingConfig {
      bool enabled = false;
      RopeScalingMethod method = RopeScalingMethod::YARN;
      int max_context = 32768;
      int original_context = 4096;
      
      // YaRN parameters
      float yarn_ext_factor = 1.0f;
      float yarn_attn_factor = 1.0f;
      float yarn_beta_fast = 32.0f;
      float yarn_beta_slow = 1.0f;
  };
  ```

- [ ] Add to LlamaWrapper::Config
- [ ] Add enum for scaling methods

### 2. Context Initialization (src/llm/llama_wrapper.cpp)

- [ ] Implement RoPE scaling in initializeContext()
  ```cpp
  if (config_.rope_scaling.enabled) {
      switch (config_.rope_scaling.method) {
          case RopeScalingMethod::LINEAR:
              ctx_params.rope_scaling_type = LLAMA_ROPE_SCALING_LINEAR;
              ctx_params.rope_freq_scale = 
                  static_cast<float>(config_.original_context) / 
                  config_.max_context;
              break;
          
          case RopeScalingMethod::YARN:
              ctx_params.rope_scaling_type = LLAMA_ROPE_SCALING_YARN;
              // ... set YaRN parameters
              break;
          
          // ... other methods
      }
  }
  ```

- [ ] Add validation for context sizes
- [ ] Add VRAM estimation
- [ ] Add warnings for insufficient memory

### 3. Configuration Files

- [ ] Update `config/llm_config.example.yaml`
  ```yaml
  rope_scaling:
    enabled: false  # Disabled by default
    method: "yarn"
    max_context: 32768
  ```

- [ ] Update `config/llm_config.production.yaml`
  ```yaml
  rope_scaling:
    enabled: true  # Enable for production long-context
    method: "yarn"  # Best quality
    max_context: 32768
  ```

### 4. API Integration

- [ ] Add per-request context override
  ```cpp
  InferenceRequest {
      std::string prompt;
      std::optional<int> max_context;  // Override config
      // ...
  };
  ```

- [ ] HTTP API support
  ```json
  POST /api/v1/llm/generate
  {
      "prompt": "...",
      "max_context": 16384  // Request-specific override
  }
  ```

### 5. Monitoring & Metrics

- [ ] Add metrics for context usage
  ```cpp
  struct ContextMetrics {
      int context_size;
      int tokens_used;
      float utilization;  // tokens_used / context_size
      std::string scaling_method;
  };
  ```

- [ ] Log context configuration
  ```
  [INFO] RoPE scaling enabled: YARN (4096 → 32768 tokens)
  [INFO] Estimated VRAM: 14.1 GB
  ```

### 6. Testing

- [ ] Unit tests for configuration parsing
- [ ] Integration tests for each scaling method
- [ ] Performance tests (prefill time, memory usage)
- [ ] Quality tests (perplexity measurement)

## Expected Performance

### Memory Usage

| Context | KV Cache | Total VRAM |
|---------|----------|------------|
| 4K      | 1.2 GB   | 5.7 GB     |
| 8K      | 2.4 GB   | 6.9 GB     |
| 16K     | 4.8 GB   | 9.3 GB     |
| 32K     | 9.6 GB   | 14.1 GB    |

### Latency Impact

| Context | Prefill Time | Generation Speed |
|---------|--------------|------------------|
| 4K      | 800ms        | 25ms/token       |
| 32K     | 3200ms       | 27ms/token       |

### Quality (Perplexity)

| Method  | 8K     | 16K    | 32K    |
|---------|--------|--------|--------|
| Baseline| 6.12   | 6.12   | 6.12   |
| Linear  | 6.31   | 6.89   | 8.21   |
| NTK     | 6.19   | 6.47   | 7.03   |
| **YaRN**| **6.15**| **6.29**| **6.58** |

## Acceptance Criteria

### Functional
- [ ] All 4 scaling methods work
- [ ] Configuration loads correctly
- [ ] Context sizes validate properly
- [ ] Proper error handling for OOM
- [ ] Fallback to smaller context if needed

### Performance
- [ ] 32K context with YaRN: perplexity < 7.0 ✅
- [ ] Memory usage within 5% of estimate ✅
- [ ] Prefill scales linearly with context ✅

### Documentation
- [ ] Configuration guide updated
- [ ] API documentation updated
- [ ] Performance benchmarks documented
- [ ] Migration guide for existing configs

## Deliverables

- [ ] Implementation in `src/llm/llama_wrapper.{h,cpp}`
- [ ] Updated configuration files
- [ ] Test suite in `tests/llm/test_rope_scaling.cpp`
- [ ] Benchmark script in `scripts/benchmark_rope_scaling.sh`
- [ ] Performance report
- [ ] User documentation

## Estimated Effort

**Time:** 1-2 weeks
**Complexity:** Low-Medium (llama.cpp already supports, mostly config)
**Dependencies:** Issue #1 (compilation)

## Use Cases Enabled

1. **Long Document Q&A**
   - Process entire research papers (20K tokens)
   - Book chapters (25K tokens)

2. **Code Repository Analysis**
   - Analyze entire modules (15K tokens)
   - Multiple files in context

3. **Extended Conversations**
   - Long conversation history (15K tokens)
   - No loss of context in multi-turn dialogs

## Related Issues

- Depends on: #1 (Fix Compilation)
- Related: #5 (Grammar Constraints)
- Related: #6 (Vision Support)

## References

- Docs: `docs/en/llm/ROPE_SCALING_IMPLEMENTATION.md`
- RoPE paper: https://arxiv.org/abs/2104.09864
- YaRN paper: https://arxiv.org/abs/2309.00071
- llama.cpp RoPE: https://github.com/ggerganov/llama.cpp/pull/2268

## Success Criteria

| Metric | Target | Status |
|--------|--------|--------|
| YaRN 32K Perplexity | < 6.7 | ⏳ |
| Memory Usage (32K) | 14-15 GB | ⏳ |
| Prefill Time (32K) | 3-4 seconds | ⏳ |
| Generation Speed | ~27ms/token | ⏳ |

All metrics met = ✅ Feature Complete
