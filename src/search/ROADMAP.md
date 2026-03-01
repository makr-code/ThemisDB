<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Search Module Roadmap

## Current Status
v1.5.0 – Production-ready hybrid search. The core engine (BM25, HNSW vector, RRF fusion, fuzzy matching, phonetic search, query expansion) has been production-ready since v1.2.0. v1.5.0 adds 7 new components: `QueryExpander`, `FuzzyMatcher`, `FacetedSearch`, `SearchAnalytics`, `AutocompleteEngine`, `LearningToRank`, and `MultiModalSearch`.

## Completed ✅
- [x] HybridSearch – RRF-based fusion of BM25 and vector results
- [x] BM25 ranking (configurable k1 and b parameters)
- [x] Term frequency and document frequency analysis
- [x] Field-length normalization
- [x] HNSW semantic vector indexing (Cosine, Dot Product, L2)
- [x] Reciprocal Rank Fusion (RRF) with configurable weights
- [x] Score normalization across search types
- [x] Fuzzy matching (edit distance)
- [x] Phonetic search (Soundex, Metaphone)
- [x] Stemming and stop-word filtering
- [x] Synonym expansion / query expansion
- [x] Phrase search (exact phrase matching)
- [x] Text-only, vector-only, and hybrid search modes
- [x] High recall@10 (85%+) with hybrid search
- [x] QueryParser – natural language query parsing
- [x] ResultRanker – configurable score aggregation
- [x] LLM-based query rewriting for improved recall (`LlmQueryRewriter`)
- [x] `QueryExpander` – synonym expansion, Levenshtein spelling correction, zero-result relaxation (v1.5.0)
- [x] `FuzzyMatcher` – Levenshtein, Soundex, Metaphone, N-gram (Dice) similarity (v1.5.0)
- [x] `FacetedSearch` – per-field value-count facets, numeric range buckets, drill-down filtering (v1.5.0)
- [x] `SearchAnalytics` – thread-safe query log; avg/p95/p99 latency, zero-result rate, top-20 queries (v1.5.0) (Issue: #2275)
- [x] `AutocompleteEngine` – prefix-index + popular-query suggestions, score-ranked output (v1.5.0) (Issue: #2281)
- [x] `LearningToRank` – linear re-ranker over 6-dimensional feature vector; online pairwise gradient-descent training (v1.5.0)
- [x] `MultiModalSearch` – TEXT/IMAGE/AUDIO/CUSTOM modalities; weighted RRF fusion (v1.5.0)
- [x] Faceted search with dynamic facet counting (`FacetedSearch`, v1.5.0) (Issue: #2283)
- [x] Configurable re-ranking with LLM feedback loop (Issue: #2454)
- [x] Spelling correction suggestions (Issue: #2455)

## In Progress 🚧
- [I] Highlight / snippet generation for matched terms (Target: Q3 2026) (Issue: #2457)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [x] Multi-field boosting (title > body > tags) (`MultiFieldBoostedSearch`, v1.9.0) (Issue: #1971)
- [I] Negative keyword filtering (`NOT` operator) (Issue: #2003)

### Long-term (6-12 months)
- [I] Neural sparse retrieval (SPLADE / BERT-based) (Issue: #2277)
- [x] Cross-lingual semantic search (multilingual embeddings) (Issue: #2278)
- [I] Personalized ranking based on user interaction history (Issue: #2279)
- [I] Distributed search across shards with result merging (Issue: #2280)

## Implementation Phases

### Phase 1: Hybrid Search & BM25 Engine (Status: Completed ✅)
- [x] `HybridSearch` – Reciprocal Rank Fusion (RRF) merging of BM25 and vector results
- [x] BM25 ranking with configurable k1 and b parameters
- [x] Term frequency and document frequency analysis with field-length normalization
- [x] HNSW semantic vector indexing (Cosine, Dot Product, L2)
- [x] Score normalization across search types
- [x] Fuzzy matching (edit distance), phonetic search (Soundex, Metaphone)
- [x] Stemming, stop-word filtering, synonym expansion, and phrase search
- [x] Text-only, vector-only, and hybrid search modes with 85%+ recall@10
- [x] `QueryParser` – natural language query parsing
- [x] `ResultRanker` – configurable score aggregation

### Phase 2: LLM Query Rewriting & Faceted Search (Status: In Progress 🚧)
- [x] LLM-based query rewriting for improved recall (`LlmQueryRewriter`)
- [x] Faceted search with dynamic facet counting
- [~] Highlight / snippet generation for matched terms

### Phase 3: Multi-Field Boosting & Search Analytics (Status: Planned 📋)
- [x] Multi-field boosting (title > body > tags) (`MultiFieldBoostedSearch`, v1.9.0)
- [ ] Negative keyword filtering (`NOT` operator)
- [x] Configurable re-ranking with LLM feedback loop
- [x] Search analytics (top queries, zero-result queries) (`SearchAnalytics`, v1.5.0)
- [x] Spelling correction suggestions

### Phase 4: Neural Retrieval & Distributed Search (Status: Planned 📋)
- [ ] Neural sparse retrieval (SPLADE / BERT-based)
- [x] Cross-lingual semantic search (multilingual embeddings)
- [ ] Personalized ranking based on user interaction history
- [ ] Distributed search across shards with result merging
- [x] Autocomplete / type-ahead query suggestions (`AutocompleteEngine`, v1.5.0)

## Production Readiness Checklist
- [x] Unit tests coverage > 80% (LlmReranker: 29 tests; LearningToRank: 20 tests; HybridSearch: 30+ tests; v1.5.0 components: 162+ tests)
- [x] Integration tests (BM25 correctness, hybrid recall@10, LlmReranker with real indices)
- [?] Performance benchmarks (QPS, index build time, latency p99)
- [?] Security audit (query injection, resource exhaustion on large datasets)
- [x] Documentation complete (FUTURE_ENHANCEMENTS.md, class docblocks, ARCHITECTURE.md, ROADMAP.md)
- [x] API stability guaranteed (HybridSearch v1.2.0+, setReranker() v1.8.0)

## Known Issues & Limitations
- Synonym expansion dictionary must be manually provided; no automatic synonym discovery.
- Phonetic search accuracy varies by language; optimized for English.

## Breaking Changes
- HybridSearch API (RRF weights, mode selection) is stable from v1.2.0.
- BM25 default parameters (k1=1.2, b=0.75) remain unchanged in v1.5.0 and are configurable.
