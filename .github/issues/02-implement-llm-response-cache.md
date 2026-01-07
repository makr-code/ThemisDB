---
name: "Implement Real LLM Response Cache"
about: Replace stub implementation with production-ready semantic cache
title: "[LLM] Implement Real LLM Response Cache with Semantic Similarity"
labels: ["enhancement", "llm", "cache", "priority: high"]
assignees: []
---

## Description

The current LLM Response Cache uses a **stub implementation** with simple string matching instead of semantic similarity. This needs to be replaced with a production-ready implementation using ThemisDB's actual SemanticCache/EmbeddingCache with HNSW indexing.

## Current Status

⚠️ **STUB IMPLEMENTATION**

Location: `src/llm/llm_response_cache.cpp`

```cpp
// For now, using in-memory map as stub
// TODO: v1.3.0 - Initialize actual SemanticCache here
// TODO: v1.3.0 - Generate actual embedding using EmbeddingCache
// TODO: v1.3.0 - Use actual embedding similarity (cosine similarity)
```

**Current behavior**: 
- Uses simple `std::unordered_map` for storage
- String-based matching only
- No semantic similarity
- No HNSW index integration

## Requirements

### Must Have
- [ ] Integrate ThemisDB's actual EmbeddingCache
- [ ] Implement HNSW-based similarity search
- [ ] Use cosine similarity for semantic matching
- [ ] Generate embeddings for prompts using actual embedding model
- [ ] Support configurable similarity threshold (default: 0.95)
- [ ] Proper TTL and eviction policies

### Nice to Have
- [ ] Metrics for cache hit/miss rates
- [ ] Support for multi-level caching
- [ ] Batch embedding generation

## Implementation Plan

1. **Replace stub cache initialization**
   - Initialize actual SemanticCache in constructor
   - Configure HNSW parameters

2. **Implement embedding generation**
   - Use EmbeddingCache for prompt embedding
   - Support for different embedding models

3. **Replace string matching with similarity search**
   - Implement HNSW similarity search
   - Use cosine similarity threshold

4. **Add proper error handling**
   - Handle embedding generation failures
   - Handle cache misses gracefully

## Testing

- [ ] Unit tests for cache operations
- [ ] Integration tests with actual embeddings
- [ ] Performance benchmarks vs stub
- [ ] Verify claimed 75x speedup with real implementation

## Performance Impact

**Claimed**: 75x speedup with semantic caching  
**Current**: Based on stub, needs validation with real implementation

## References

- `PRODUCTION_READINESS_REVIEW.md` - Stub analysis
- `docs/en/llm/CACHE_IMPLEMENTATION.md` - Cache architecture
- ThemisDB EmbeddingCache implementation

## Related Issues

- Part of production-readiness fixes for LLM features
- Blocks accurate performance benchmarking
