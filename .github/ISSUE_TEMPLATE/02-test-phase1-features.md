---
name: "🟡 Test & Validate Phase 1 Features"
about: Validate Phase 1 LLM optimizations (Flash Attention, KV-Cache, Embeddings)
title: "[P1] Test & Validate Phase 1 Features (Flash Attention, KV-Cache Reuse, Embeddings)"
labels: ["priority: high", "type: testing", "component: llm", "phase-1"]
assignees: []
---

## Priority
🟡 **HIGH** - P1 (After compilation fixed)

## Overview

Phase 1 features are **already implemented** but have not been tested due to compilation issues. Once Issue #1 is resolved, we need to validate these features work correctly and deliver the expected performance improvements.

**Phase 1 Features:**
1. ✅ Flash Attention (15-25% speedup)
2. ✅ KV-Cache Reuse (10-20x first-token speedup)
3. ✅ Embeddings Extraction (unified model)

## Depends On

- ⚠️ **Blocked by Issue #1** (Fix Compilation Infrastructure)

## Test Plan

### 1. Flash Attention Testing

#### Functional Tests
- [ ] Model loads with `use_flash_attn: true`
- [ ] Inference produces correct outputs
- [ ] Flash Attention activates (check logs)
- [ ] Fallback works if Flash Attention unavailable

#### Performance Tests
```yaml
# Test configuration
optimizations:
  use_flash_attn: true

# Benchmark
Model: Mistral-7B-Instruct-Q4_K_M
Prompt: "Explain quantum computing in simple terms"
Iterations: 100
```

**Expected Results:**
- [ ] 15-25% faster inference vs baseline
- [ ] 30% less VRAM usage
- [ ] Same output quality (no accuracy loss)

**Baseline (Flash Attention OFF):**
- Tokens/sec: ~42.3
- VRAM usage: ~6.8 GB
- Latency (100 tokens): ~2400ms

**Target (Flash Attention ON):**
- Tokens/sec: 50-53 (18-25% faster)
- VRAM usage: ~4.8 GB (30% reduction)
- Latency (100 tokens): ~1900ms

#### Validation
```bash
# Run benchmark
./scripts/benchmark_flash_attention.sh

# Check results
cat results/flash_attention_benchmark.json
```

---

### 2. KV-Cache Reuse Testing

#### Functional Tests
- [ ] Prefix cache initializes correctly
- [ ] Cache detects repeated system prompts
- [ ] Cache hit/miss logic works
- [ ] Cache eviction (LRU) works correctly
- [ ] Statistics API returns correct metrics

#### Performance Tests
```yaml
# Test configuration
optimizations:
  use_kv_cache_reuse: true
  prefix_cache_config:
    max_cache_size: 100
    similarity_threshold: 0.9
    ttl_seconds: 3600

# Benchmark: RAG workload with repeated system prompt
System Prompt: "You are a helpful AI assistant. Use the context below to answer."
Queries: 100 (same system prompt, different questions)
```

**Expected Results:**
- [ ] 10-20x faster first-token on cache hits
- [ ] 40-60% reduction in total inference time
- [ ] 60-70% cache hit rate
- [ ] Zero accuracy loss

**Baseline (Cache OFF):**
- First-token latency: ~2400ms
- Total inference time: ~3500ms
- Cache hit rate: N/A

**Target (Cache ON):**
- First-token latency: 120-240ms on hit (10-20x faster)
- Total inference time: ~1500ms (60% reduction)
- Cache hit rate: 60-70%

#### Statistics Validation
```cpp
// Check cache statistics
auto stats = llama_wrapper->getPrefixCacheStats();
assert(stats.has_value());
assert(stats->hit_rate >= 0.60);
assert(stats->hit_rate <= 0.75);
```

---

### 3. Embeddings Extraction Testing

#### Functional Tests
- [ ] Model loads with `enable_embeddings: true`
- [ ] Embeddings mode activates (check logs)
- [ ] Embeddings have correct dimensions
- [ ] L2 normalization works
- [ ] Can process multiple texts in batch

#### Performance Tests
```yaml
# Test configuration
optimizations:
  enable_embeddings: true

# Benchmark
Task: Generate embeddings for 1000 sentences
Model: Mistral-7B (embeddings mode)
```

**Expected Results:**
- [ ] Embeddings dimension: 4096 (Mistral-7B)
- [ ] L2 normalized (magnitude ≈ 1.0)
- [ ] Consistent embeddings (same input → same output)
- [ ] Reasonable throughput (>10 sent/sec)

#### Semantic Validation
```python
# Test semantic similarity
text1 = "The cat sits on the mat"
text2 = "A feline rests on the rug"
text3 = "Quantum physics is complex"

emb1 = get_embedding(text1)
emb2 = get_embedding(text2)
emb3 = get_embedding(text3)

sim_12 = cosine_similarity(emb1, emb2)
sim_13 = cosine_similarity(emb1, emb3)

# Similar sentences should have high similarity
assert sim_12 > 0.7  # Similar meaning
assert sim_13 < 0.3  # Different meaning
```

---

## Integration Testing

### Combined Features Test
Test all Phase 1 features together:

```yaml
optimizations:
  use_flash_attn: true
  use_kv_cache_reuse: true
  enable_embeddings: false  # Can't use both modes

# Run RAG workload
- Generate embeddings (embeddings mode)
- Search similar documents
- Generate response (generation mode with Flash + Cache)
```

**Expected:**
- [ ] All features work together
- [ ] No conflicts or errors
- [ ] Combined speedup: 2-3x overall

---

## Acceptance Criteria

### Functional
- [ ] All features activate correctly via config
- [ ] Features produce expected outputs
- [ ] No crashes or memory leaks
- [ ] Proper error handling and fallbacks

### Performance
- [ ] Flash Attention: 15-25% speedup ✅
- [ ] KV-Cache Reuse: 10-20x first-token speedup ✅
- [ ] Embeddings: Functional and semantic correctness ✅
- [ ] Combined: 2-3x improvement ✅

### Quality
- [ ] Zero accuracy loss (outputs match baseline)
- [ ] No quality degradation
- [ ] Stable over extended runs (no memory leaks)

---

## Deliverables

- [ ] Test scripts in `scripts/test_phase1/`
- [ ] Benchmark results in `results/phase1_benchmarks.json`
- [ ] Performance report document
- [ ] Configuration examples for production
- [ ] Update README with test results

---

## Estimated Effort

**Time:** 2-3 days
**Complexity:** Medium (testing existing code)
**Dependencies:** Issue #1 (compilation)

---

## Related Issues

- Depends on: #1 (Fix Compilation)
- Blocks: Production deployment of Phase 1
- Related: #3 (Test Phase 2)

---

## References

- Implementation: PR #XXX (Phase 1 Implementation)
- Docs: `docs/en/llm/FLASH_ATTENTION_IMPLEMENTATION.md`
- Docs: `docs/en/llm/KV_CACHE_REUSE_IMPLEMENTATION.md`
- Docs: `docs/en/llm/EMBEDDINGS_EXTRACTION_IMPLEMENTATION.md`
- Config: `config/llm_config.example.yaml`
- Config: `config/llm_config.production.yaml`

---

## Success Metrics

| Feature | Baseline | Target | Actual | Status |
|---------|----------|--------|--------|--------|
| Flash Attention Speedup | 42.3 tok/s | 50-53 tok/s | TBD | ⏳ |
| Flash Attention Memory | 6.8 GB | 4.8 GB | TBD | ⏳ |
| KV-Cache First-Token | 2400ms | 120-240ms | TBD | ⏳ |
| KV-Cache Hit Rate | N/A | 60-70% | TBD | ⏳ |
| Embeddings Dimension | N/A | 4096 | TBD | ⏳ |

Target: **ALL metrics in expected range** = ✅ Success
