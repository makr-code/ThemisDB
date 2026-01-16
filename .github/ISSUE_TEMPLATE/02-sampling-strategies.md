---
name: "🎲 Sampling Strategy Implementation"
about: Implement token sampling strategies with llama.cpp sampler API (Phase 1)
title: "[LLM] Implement Token Sampling Strategies (Greedy, Nucleus, Mirostat)"
labels: priority:P0, type:feature, area:llm, effort:medium, phase:1
assignees: ''

---

## 📋 Description

Implement production-ready token sampling strategies using llama.cpp's sampler API. This completes the Strategy Pattern infrastructure for text generation.

**Related Analysis**: `docs/analysis/IMPLEMENTATION_GUIDE.md` §1.4
**Infrastructure Files**:
- `include/llm/sampling_strategy.h`
- `src/llm/sampling_strategy.cpp`

## 🎯 Goals

- [ ] Implement actual llama.cpp sampler API calls
- [ ] Complete greedy, nucleus, and mirostat sampling
- [ ] Verify sampling quality and diversity
- [ ] Ensure deterministic behavior where expected
- [ ] Comprehensive testing and benchmarking

## 📝 Tasks

### 1. GreedySampling Implementation
- [ ] Implement `sample()` with `llama_get_logits_ith()`
- [ ] Select token with highest probability
- [ ] Verify deterministic behavior (same input → same output)
- [ ] Test with various contexts
- [ ] Benchmark performance

**File**: `src/llm/sampling_strategy.cpp`
**Lines**: 12-30
**Expected Performance**: < 0.5ms per token

### 2. NucleusSampling Implementation
- [ ] Implement `sample()` with llama_sampler chain
- [ ] Add `llama_sampler_init_penalties()` for repeat penalty
- [ ] Add `llama_sampler_init_top_k()` for top-k filtering
- [ ] Add `llama_sampler_init_top_p()` for nucleus sampling
- [ ] Add `llama_sampler_init_temp()` for temperature
- [ ] Add `llama_sampler_init_dist()` for distribution sampling
- [ ] Test parameter effects (temperature, top_k, top_p)
- [ ] Verify diversity vs coherence trade-off
- [ ] Benchmark performance

**File**: `src/llm/sampling_strategy.cpp`
**Lines**: 35-75
**Expected Performance**: < 2ms per token

### 3. MirostatSampling Implementation
- [ ] Implement `sample()` with Mirostat v2
- [ ] Initialize with `llama_sampler_init_mirostat_v2()`
- [ ] Test adaptive behavior (mu adjustment)
- [ ] Verify perplexity targeting
- [ ] Compare quality vs other strategies
- [ ] Benchmark performance

**File**: `src/llm/sampling_strategy.cpp`
**Lines**: 80-110
**Expected Performance**: < 2ms per token

### 4. Factory Pattern
- [ ] Update `SamplingStrategyFactory::create()`
- [ ] Add parameter validation
- [ ] Test all strategy types
- [ ] Test unknown strategy fallback
- [ ] Document factory usage

**File**: `src/llm/sampling_strategy.cpp`
**Lines**: 115-140

### 5. Integration with LlamaWrapper
- [ ] Integrate sampling strategies into inference loop
- [ ] Add strategy selection via config
- [ ] Support runtime strategy switching
- [ ] Test with actual text generation
- [ ] Verify output quality

**File**: `src/llm/llama_wrapper.cpp` (to be updated)

### 6. Testing
- [ ] Unit tests for each strategy (`tests/test_sampling_strategy.cpp`)
- [ ] Test determinism (greedy)
- [ ] Test randomness (nucleus, mirostat)
- [ ] Test parameter effects
- [ ] Integration tests with llama.cpp
- [ ] Quality comparison tests
- [ ] Performance benchmarks (`benchmarks/bench_llm_infrastructure.cpp`)

### 7. Quality Metrics
- [ ] Implement perplexity calculation
- [ ] Implement coherence metrics
- [ ] Implement diversity metrics (Self-BLEU)
- [ ] Compare strategies quantitatively
- [ ] Document quality vs performance trade-offs

### 8. Documentation
- [ ] Update code comments
- [ ] Add usage examples for each strategy
- [ ] Document parameter recommendations
- [ ] Document quality vs diversity trade-offs
- [ ] Update `INFRASTRUCTURE_README.md`

## ✅ Acceptance Criteria

- [ ] All TODO comments in sampling_strategy.cpp are resolved
- [ ] Greedy sampling is deterministic
- [ ] Nucleus sampling produces diverse outputs
- [ ] Mirostat sampling adapts to context
- [ ] All strategies work with llama.cpp contexts
- [ ] Factory creates correct strategy types
- [ ] All tests pass (unit, integration, quality)
- [ ] Code coverage > 80%
- [ ] Benchmarks show < 2ms per token
- [ ] Documentation is complete

## 🔗 Dependencies

- llama.cpp sampler API
- `LlamaContextHandle` (from Issue makr-code/ThemisDB#1)
- Test models (TinyLlama-1.1B or similar)

## 📊 Estimated Effort

**Time**: 1-2 weeks (1 FTE)
**Priority**: 🔴 Critical (Phase 1, Week 4-5)

## 🧪 Test Strategy

1. **Unit Tests**: Test each strategy independently
2. **Determinism Tests**: Verify greedy is deterministic
3. **Randomness Tests**: Verify nucleus/mirostat are non-deterministic
4. **Parameter Tests**: Test effect of temperature, top_k, top_p, tau, eta
5. **Quality Tests**: Compare perplexity, coherence, diversity
6. **Integration Tests**: Generate text with each strategy
7. **Performance Tests**: Benchmark sampling speed

## 📚 References

- `docs/analysis/IMPLEMENTATION_GUIDE.md` §1.4 - Sampling strategies
- llama.cpp sampler API: https://github.com/ggerganov/llama.cpp/blob/master/include/llama.h
- Nucleus Sampling paper: https://arxiv.org/abs/1904.09751
- Mirostat paper: https://arxiv.org/abs/2007.14966

## 💡 Implementation Notes

- Use llama.cpp's sampler chain for composable sampling
- Greedy should be fastest (< 0.5ms)
- Nucleus is most commonly used
- Mirostat provides best quality for long-form generation
- Test with multiple model sizes (1B, 7B, 13B)
- Consider caching sampler objects for performance

### Parameter Recommendations

**Greedy**:
- No parameters
- Use for: Tasks requiring determinism

**Nucleus**:
- Temperature: 0.7-0.9 (lower = more focused)
- Top-K: 40-50 (fewer = more focused)
- Top-P: 0.9-0.95 (lower = more focused)
- Repeat Penalty: 1.1-1.2 (higher = less repetition)
- Use for: General text generation

**Mirostat**:
- Tau: 3.0-5.0 (target entropy)
- Eta: 0.1 (learning rate)
- Use for: Long-form coherent text

## 🏁 Definition of Done

- [ ] All tasks completed and checked off
- [ ] All acceptance criteria met
- [ ] Code reviewed and approved
- [ ] Tests pass in CI/CD
- [ ] Benchmarks meet performance targets
- [ ] Quality metrics documented
- [ ] Documentation updated
- [ ] Ready for Phase 2 (Complete Inference)
