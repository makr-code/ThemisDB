---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Embedding Cache

Exact model backend not verifiable from final document text alone.

## Purpose and ownership

`EmbeddingCache` reuses a stored query result when a supplied embedding is sufficiently similar to a cached embedding. It does not generate embeddings, own the embedding model, or establish that a near match is equivalent for a particular application. Callers remain responsible for selecting the model/version, dimension, and similarity threshold and for deciding which results are safe to reuse.

The public type is declared in [`include/cache/embedding_cache.h`](../../include/cache/embedding_cache.h) and implemented in [`embedding_cache.cpp`](embedding_cache.cpp).

## Configuration and stored entry

`EmbeddingCache::Config` provides:

- maximum entry count (`max_entries`, default 100,000);
- TTL in seconds (`ttl_seconds`, default 3,600);
- minimum hit similarity (`similarity_threshold`, default 0.95);
- required vector dimension (`embedding_dim`, default 1,536);
- whether to initialize a vector index (`use_vector_index`, default true);
- a cache directory used by the index-backed storage path.

An entry carries query text, an aligned float vector, string metadata, insertion timestamp, access count, and last-hit similarity. The cache is movable but not copyable. `EmbeddingCache` owns its implementation state through a pimpl; callers should not infer persistence guarantees from the temporary-looking default directory. Choose and protect the path according to deployment retention and storage requirements.

## Lookup and store behavior

1. `query(vector)` requires the vector size to equal the configured dimension. A mismatch returns no entry.
2. When the vector index is active, lookup asks it for the nearest item and converts the configured cosine distance to similarity (`1 - distance`). A candidate below the threshold is a miss.
3. When no vector index is active, lookup scans live entries and computes cosine similarity; the best entry must meet the same threshold.
4. Expired candidates do not produce hits. Hits update access/similarity statistics and estimated cost-savings accounting; a miss returns `std::nullopt`.
5. `store(query_text, embedding, metadata)` validates the dimension, inserts the entry, and applies the configured capacity/TTL behavior. `clearExpired()` removes expired entries; `clear()` removes cache contents.

The indexed search requests one nearest neighbor. The fallback scan is used when the index is absent, not a documented guarantee that every index search error is retried by scanning.

## Correctness and evaluation constraints

- Keep model identity and dimension consistent between writes and reads. Equal dimensions alone do not make vectors from different models comparable.
- Calibrate `similarity_threshold` against application-labeled equivalent and non-equivalent queries; do not treat the default as a semantic correctness guarantee.
- Include index-enabled and index-unavailable runs in performance evaluation because the fallback is linear in the number of entries.
- Ensure TTL and explicit invalidation match the freshness requirements of the cached response.
- Avoid placing secrets or sensitive prompt text in metadata unless the storage location, retention, access controls, and backup policy are appropriate.
- The [embedding benchmark](../../benchmarks/cache/bench_embedding_cache_performance.cpp) and cases listed in [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md) are the measurement references; record model, threshold, dimension, corpus, and build/runtime details with results.

## Tests and related docs

Use [`tests/test_embedding_cache.cpp`](../../tests/test_embedding_cache.cpp) for existing behavior coverage. Broader threshold and corpus evaluation is described in [RESEARCH.md](./RESEARCH.md). The [module guide](./MODULE_GUIDE.md) links the embedding cache to adjacent query and LLM integration surfaces.
