---
name: "Implement Real LLM Prefix Cache"
about: Replace stub with HNSW-based prefix caching for KV-Cache reuse
title: "[LLM] Implement Real LLM Prefix Cache with HNSW Integration"
labels: ["enhancement", "llm", "cache", "priority: high"]
assignees: []
---

## Description

The LLM Prefix Cache currently uses a **stub implementation** without actual HNSW-based similarity search. This blocks the claimed 10-20x first-token speedup from KV-Cache reuse.

## Current Status

⚠️ **STUB IMPLEMENTATION**

Location: `src/llm/llm_prefix_cache.cpp`

```cpp
/**
 * @brief Implementation using stub for EmbeddingCache
 * 
 * In production, this would use ThemisDB's actual EmbeddingCache
 * which provides HNSW-based similarity search over embeddings.
 */

// TODO: In production, add to EmbeddingCache HNSW index
// TODO: In production, use EmbeddingCache for similarity search
```

**Current behavior**:
- Simple in-memory map
- String prefix matching only
- No HNSW index
- No semantic similarity

## Requirements

### Must Have
- [ ] Integrate actual EmbeddingCache with HNSW
- [ ] Implement similarity-based prefix matching
- [ ] Store and reuse precomputed KV cache
- [ ] Support configurable min_prefix_length
- [ ] Proper TTL for cached prefixes (default: 2 hours)

### Nice to Have
- [ ] Prefix sharing across requests
- [ ] Metrics for cache hit rates
- [ ] Automatic prefix eviction

## Implementation Plan

1. **Replace stub with real EmbeddingCache**
   - Initialize HNSW index for prefix embeddings
   - Configure similarity threshold

2. **Implement KV cache storage**
   - Store precomputed KV cache data
   - Efficient retrieval by similarity

3. **Add similarity-based lookup**
   - Use HNSW for nearest neighbor search
   - Return best matching prefix

4. **Error handling**
   - Handle HNSW index failures
   - Fallback behavior for cache misses

## Testing

- [ ] Unit tests for prefix caching
- [ ] Integration tests with actual KV cache data
- [ ] Performance benchmarks
- [ ] Verify 10-20x speedup claim

## Performance Impact

**Claimed**: 10-20x first-token speedup  
**Current**: Based on stub, needs validation

## References

- `PRODUCTION_READINESS_REVIEW.md`
- `docs/en/llm/FLASH_ATTENTION_IMPLEMENTATION.md`

## Related Issues

- Depends on: Implement Real LLM Response Cache (#2)
- Part of production-readiness fixes
