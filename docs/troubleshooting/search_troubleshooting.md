# Search Troubleshooting Guide

The `search` module provides full-text and semantic search for ThemisDB, including BM25-based full-text search, hybrid (dense+sparse) search, faceted search, learning-to-rank (LTR), LLM-based query rewriting and reranking, fuzzy matching, and multi-modal search.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `SearchIndex: no index for field` | Full-text index not created | Create search index on the field |
| Hybrid search returns only vector results | BM25 component weight is 0 | Set `search.hybrid.bm25_weight > 0` |
| `LlmQueryRewriter: model not loaded` | LLM module not initialised | Ensure `llm.enabled: true` |
| Facet counts are wrong | Facet cache stale | Flush facet cache; disable caching |
| LTR model not applied | Model not loaded or path wrong | Check `search.ltr.model_path` |
| Fuzzy search returns too many results | Edit distance too high | Reduce `search.fuzzy.max_distance` |
| Semantic search dimension mismatch | Wrong embedding model | Align embedding model with index dimension |
| Reranker latency too high | Reranker model too large | Use smaller reranker; enable result caching |
| `MultiModalSearch: image encoder unavailable` | Vision model not configured | Configure `search.multi_modal.image_encoder` |
| Search autocomplete returns stale suggestions | Suggestion index not updated | Rebuild autocomplete index |

## Common Issues

### Issue 1: Full-Text Index Missing

**Description:** Full-text search on a field returns no results because the search index was not created.

**Symptoms:**
- Error: `SearchIndex: no full-text index on collection=products field=description`
- `SEARCH` queries return empty results

**Cause:** Collection does not have a search (ArangoSearch-compatible) index.

**Solution:**
```bash
# Create a search index
themisdb-admin index create \
  --collection products \
  --type search \
  --fields description,title \
  --analyzers text_en,text_de

# Verify index
themisdb-admin index list --collection products --type search
```
```sql
-- Use SEARCH in AQL
FOR p IN products
  SEARCH ANALYZER(p.description IN TOKENS("fast database", "text_en"), "text_en")
  SORT BM25(p) DESC
  RETURN p
```

---

### Issue 2: Hybrid Search Ignores BM25 Component

**Description:** Hybrid search results look like pure vector search; keyword signals are ignored.

**Symptoms:**
- Results for keyword-heavy queries match only on semantics
- Log: `HybridSearch: bm25_weight=0.0; using vector-only mode`

**Cause:** `bm25_weight` is set to 0; both weights must be non-zero.

**Solution:**
```yaml
search:
  hybrid:
    enabled: true
    vector_weight: 0.6
    bm25_weight: 0.4               # ensure > 0
    rrf_k: 60                      # reciprocal rank fusion constant
    fusion_strategy: rrf            # "rrf" | "linear" | "learned"
```

---

### Issue 3: LLM Query Rewriter Fails to Load

**Description:** Query rewriting is silently skipped because the LLM model is not available.

**Symptoms:**
- Log: `LlmQueryRewriter: LLM orchestrator not initialised – rewriting disabled`
- Search quality degraded; queries not expanded

**Cause:** LLM module disabled or model not downloaded.

**Solution:**
```yaml
llm:
  enabled: true
  model_path: /var/lib/themisdb/models/query-rewriter.Q4_K_M.gguf

search:
  llm_query_rewriter:
    enabled: true
    max_rewrites: 3
    timeout_ms: 2000               # fail fast if LLM is slow
    fallback_to_original: true     # use original query if rewriter fails
```

---

### Issue 4: Facet Counts Incorrect After Recent Writes

**Description:** Faceted search returns outdated counts that do not reflect recent changes.

**Symptoms:**
- Facet count for "category=Electronics" shows 100 but actual count is 150
- Log: `FacetedSearch: serving cached facets (age=3600s)`

**Cause:** Facet cache TTL too long.

**Solution:**
```yaml
search:
  faceted_search:
    enabled: true
    cache:
      enabled: true
      ttl_ms: 30000                # reduce from 3600000
      invalidate_on_write: true
    max_facet_values: 1000
    count_approx_threshold: 100000
```

---

### Issue 5: Learning-to-Rank Model Not Applied

**Description:** LTR reranking is disabled even though the model file exists.

**Symptoms:**
- Log: `LearningToRank: model file not found: /var/lib/themisdb/models/ltr_model.bin`
- Results are ranked only by BM25/vector score

**Cause:** Model path is wrong or feature extraction is misconfigured.

**Solution:**
```yaml
search:
  ltr:
    enabled: true
    model_path: /var/lib/themisdb/models/ltr_model.bin
    model_format: xgboost          # "xgboost" | "lightgbm" | "linear"
    features:
      - bm25_score
      - vector_similarity
      - document_freshness
      - click_count
    top_k_for_rerank: 100          # rerank top 100 results
```

---

### Issue 6: Fuzzy Search Returns Too Many Irrelevant Results

**Description:** Fuzzy matching returns hundreds of loosely-matched documents.

**Symptoms:**
- Search for "python" also returns "rhythm", "phyton", etc.
- User satisfaction with search results is low

**Cause:** Edit distance threshold is too permissive.

**Solution:**
```yaml
search:
  fuzzy:
    enabled: true
    max_distance: 1                # reduce from 2
    prefix_length: 2               # require first 2 chars to match exactly
    transpositions: true
    min_word_length_for_fuzzy: 5   # don't fuzzy-match short words
```

---

### Issue 7: Semantic Search Dimension Mismatch

**Description:** Vector search fails because the query embedding has the wrong dimension.

**Symptoms:**
- Error: `SearchIndex: dimension mismatch: query=768, index=1536`
- Semantic search returns no results

**Cause:** Query is embedded with a different model than was used to build the index.

**Solution:**
```yaml
search:
  embedding_model: text-embedding-3-small   # must match index embedding model
  embedding_dimension: 1536
  validate_dimensions: true        # reject queries with wrong dimension at API level
```
```bash
# Check index embedding dimension
themisdb-admin index info --index products_embedding | grep dimension

# Rebuild index with correct model
themisdb-admin index rebuild \
  --collection products \
  --embedding-model text-embedding-3-small
```

---

### Issue 8: LLM Reranker Adds Excessive Latency

**Description:** Adding LLM-based reranking increases search latency from 50ms to 3s.

**Symptoms:**
- P99 search latency = 3000ms after enabling reranker
- Log: `LlmReranker: reranking 50 results took 2800ms`

**Cause:** Reranker model is too large; reranking too many candidates.

**Solution:**
```yaml
search:
  llm_reranker:
    enabled: true
    model: cross-encoder-ms-marco-MiniLM-L6   # use small cross-encoder
    top_k_to_rerank: 20            # rerank only top 20 (not 50)
    timeout_ms: 500
    cache:
      enabled: true
      ttl_ms: 300000               # cache rerank results for 5 min
    fallback_to_original_rank: true
```

## Diagnostic Commands

```bash
# List search indices
themisdb-admin index list --type search

# Test search query
themisdb-admin search test \
  --query "lightweight database" \
  --collection products \
  --mode hybrid

# Facet cache stats
themisdb-admin search facet-cache-stats

# LTR feature importance
themisdb-admin search ltr-info

# Live search metrics
curl -s http://localhost:9100/metrics | grep themisdb_search

# Tail search logs
journalctl -u themisdb -f | grep -E "search|facet|ltr|rerank|fuzzy|hybrid"
```

## Configuration Reference

```yaml
search:
  full_text:
    enabled: true
    default_analyzer: text_en
  hybrid:
    enabled: true
    vector_weight: 0.6
    bm25_weight: 0.4
    rrf_k: 60
  fuzzy:
    enabled: true
    max_distance: 1
    prefix_length: 2
  faceted_search:
    enabled: true
    cache:
      ttl_ms: 60000
  ltr:
    enabled: false
    model_path: ""
  llm_query_rewriter:
    enabled: false
    timeout_ms: 2000
  llm_reranker:
    enabled: false
    top_k_to_rerank: 20
```

## Known Limitations

- LLM-based query rewriting adds 200–500ms latency per query depending on model size.
- Learning-to-rank requires training data (click logs, explicit ratings); cold start requires BM25 fallback.
- Multi-modal search (image queries) requires a vision encoder model and adds significant latency.
- Facet counts on sharded collections are approximate; exact counts require cross-shard aggregation.

## Related Documentation

- [Search Module ROADMAP](../../src/search/ROADMAP.md)
- [Multi-Vector Search](../de/aql/multi_vector_search.md)
- [RAG Judge LLM Integration](../llm_orchestration/RAG_JUDGE_LLM_INTEGRATION.md)
- [NLP Integration Analysis](../NLP_INTEGRATION_ANALYSIS.md)
