# Search Module — Architecture Guide

**Version:** 1.0  
**Last Updated:** 2026-02-24  
**Module Path:** `src/search/`

---

## 1. Overview

The Search module provides ThemisDB's full-text and hybrid search capabilities: inverted
index management, BM25 relevance scoring, hybrid vector+keyword fusion via Reciprocal Rank
Fusion (RRF), faceted search, fuzzy matching, query expansion, LLM-based query rewriting,
and LLM re-ranking.

---

## 2. Design Principles

- **Hybrid First** – BM25 lexical search and vector semantic search are combined from the
  start; neither is treated as a secondary path.
- **RRF Fusion** – Reciprocal Rank Fusion merges ranked lists from different search
  strategies without requiring score normalization.
- **LLM Augmentation** – query rewriting and result re-ranking optionally use LLM
  inference to improve recall and relevance.
- **Pluggable Ranking** – Learning-to-Rank (`learning_to_rank.cpp`) enables training
  custom ranking models on click/relevance feedback.
- **Zero-Dependency Fallback** – if LLM is unavailable, search falls back to pure
  BM25+vector without degradation.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `hybrid_search.cpp` | RRF-based fusion of BM25 + vector search |
| `autocomplete.cpp` | Prefix/fuzzy autocompletion |
| `faceted_search.cpp` | Faceted filtering: bucket aggregations over search results |
| `fuzzy_matcher.cpp` | Edit distance and phonetic matching (Soundex, Metaphone) |
| `query_expander.cpp` | Synonym expansion, related-term expansion |
| `llm_query_rewriter.cpp` | LLM-based query rewriting for improved recall |
| `llm_reranker.cpp` | LLM-based re-ranking of top-N results |
| `learning_to_rank.cpp` | Trained ranking model integration |
| `multi_modal_search.cpp` | Text + image search fusion |
| `search_analytics.cpp` | Search analytics: CTR, MRR, NDCG tracking |

*(Inverted index and BM25 scorer are in `src/index/inverted_index.cpp` and `src/query/functions/`)*

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                    Search Query                                  │
│   POST /search { query: "...", filters: {...}, k: 20 }          │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                    HybridSearch                                  │
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │ LlmQueryRewriter (optional): rewrite query for recall      │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
│  ┌─────────────────────────┐  ┌──────────────────────────────┐  │
│  │  BM25 Search             │  │  Vector Search               │  │
│  │  (src/index/inverted)    │  │  (src/index/vector_index)    │  │
│  └─────────────────────────┘  └──────────────────────────────┘  │
│                                                                  │
│  QueryExpander → FuzzyMatcher → FacetedSearch                   │
│                                                                  │
│  Reciprocal Rank Fusion → merged result list                    │
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │ LlmReranker (optional): re-rank top-N with LLM feedback    │ │
│  └────────────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────────┘
```

---

## 4. Data Flow

### 4.1 Hybrid Search

```
HybridSearch(query="database performance tuning", k=10)
    │
    ├─ LlmQueryRewriter (if enabled):
    │       "database performance tuning" →
    │       ["database optimization", "query performance", "index tuning"]
    │
    ├─ BM25Search: tokenize → inverted index lookup → BM25 scores
    │       → top-20 by BM25
    │
    ├─ VectorSearch: embed query → HNSW search
    │       → top-20 by cosine distance
    │
    ├─ QueryExpander: add synonyms ("optimization" → "tuning", "speedup")
    │
    ├─ RRF: merge BM25 list + vector list → unified top-20
    │       score(d, r) = Σ 1/(k + rank_i(d)) for each list i
    │
    ├─ FacetedSearch: aggregate bucket counts per filter field
    │
    ├─ LlmReranker (if enabled):
    │       LLM rates relevance of top-10 → reorder
    │
    └─ return {hits: [...], facets: {...}, total: N}
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Uses** | `src/index/` | `InvertedIndex` for BM25 and `VectorIndexManager` for ANN |
| **Uses** | `src/llm/` | Query rewriting and result re-ranking |
| **Called by** | `src/rag/` | BM25 candidates for hybrid RAG retrieval |
| **Called by** | `src/server/` | Search API endpoints |
| **Uses** | `src/observability/` | Search latency and quality metrics |

---

## 6. Threading & Concurrency Model

- BM25 and vector searches are dispatched concurrently (parallelism within a single query).
- `LlmQueryRewriter` and `LlmReranker` are optional async steps; if they timeout, the
  pipeline continues without them.
- `FacetedSearch` is computed from the result set (no additional I/O).
- `SearchAnalytics` records are appended asynchronously.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| Parallel search | BM25 and vector search run concurrently |
| RRF | O(k log k) merge; no score normalization required |
| Inverted index | O(n) postings traversal; BM25 computed inline |
| HNSW | O(log N) approximate search |

---

## 8. Security Considerations

- Search queries are validated to prevent injection via the query parser.
- Search results are scoped to the authenticated tenant's visible documents.
- LLM re-ranking does not send document content to external services; only query text.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `search.hybrid.enabled` | true | Enable hybrid BM25+vector |
| `search.hybrid.rrf_k` | 60 | RRF constant k |
| `search.hybrid.bm25_weight` | 0.5 | BM25 weight in fusion |
| `search.llm_rewriter.enabled` | false | Enable LLM query rewriting |
| `search.llm_reranker.enabled` | false | Enable LLM re-ranking |
| `search.bm25.k1` | 1.5 | BM25 term frequency saturation |
| `search.bm25.b` | 0.75 | BM25 length normalization |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| Inverted index miss | Return empty BM25 list; continue with vector results |
| LLM query rewriting timeout | Skip rewriting; use original query |
| LLM re-ranking timeout | Return RRF-fused results without re-ranking |
| Vector search unavailable | Fall back to BM25-only results |

---

## 11. Known Limitations & Future Work

- Multi-modal search (text + image) is experimental.
- Learning-to-Rank requires training data and a fitted model; not included out of the box.
- Phonetic search uses Soundex/Metaphone; language-specific stemming is in the utils module.
- Synonym expansion requires a loaded synonym dictionary.

---

## 12. References

- `src/search/README.md` — module overview
- `docs/search_roadmap.md` — roadmap
- `docs/architecture/SEARCH_ARCHITECTURE.md` — search architecture
- `ARCHITECTURE.md` (root) — full system architecture
