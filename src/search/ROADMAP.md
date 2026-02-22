<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Search Module Roadmap

## Current Status
v1.2.0+ – Production-ready hybrid search. BM25 full-text, HNSW vector semantic search, and Reciprocal Rank Fusion (RRF) result merging are all implemented with fuzzy matching and phonetic search.

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

## In Progress 🚧
- [I] Faceted search with dynamic facet counting (Target: Q2 2026) (Issue: #2283)
- [I] Highlight / snippet generation for matched terms (Target: Q3 2026) (Issue: #2457)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] Multi-field boosting (title > body > tags) (Issue: #1971)
- [I] Negative keyword filtering (`NOT` operator) (Issue: #2003)
- [I] Configurable re-ranking with LLM feedback loop (Issue: #2454)
- [I] Search analytics (top queries, zero-result queries) (Issue: #2275)
- [I] Spelling correction suggestions (Issue: #2455)

### Long-term (6-12 months)
- [I] Neural sparse retrieval (SPLADE / BERT-based) (Issue: #2277)
- [I] Cross-lingual semantic search (multilingual embeddings) (Issue: #2278)
- [I] Personalized ranking based on user interaction history (Issue: #2279)
- [I] Distributed search across shards with result merging (Issue: #2280)
- [I] Autocomplete / type-ahead query suggestions (Issue: #2281)

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
- [~] Faceted search with dynamic facet counting
- [~] Highlight / snippet generation for matched terms

### Phase 3: Multi-Field Boosting & Search Analytics (Status: Planned 📋)
- [ ] Multi-field boosting (title > body > tags)
- [ ] Negative keyword filtering (`NOT` operator)
- [ ] Configurable re-ranking with LLM feedback loop
- [ ] Search analytics (top queries, zero-result queries)
- [ ] Spelling correction suggestions

### Phase 4: Neural Retrieval & Distributed Search (Status: Planned 📋)
- [ ] Neural sparse retrieval (SPLADE / BERT-based)
- [ ] Cross-lingual semantic search (multilingual embeddings)
- [ ] Personalized ranking based on user interaction history
- [ ] Distributed search across shards with result merging
- [ ] Autocomplete / type-ahead query suggestions

## Production Readiness Checklist
- [?] Unit tests coverage > 80%
- [?] Integration tests (BM25 correctness, hybrid recall@10)
- [?] Performance benchmarks (QPS, index build time, latency p99)
- [?] Security audit (query injection, resource exhaustion on large datasets)
- [?] Documentation complete
- [?] API stability guaranteed

## Known Issues & Limitations
- Synonym expansion dictionary must be manually provided; no automatic synonym discovery.
- Phonetic search accuracy varies by language; optimized for English.
- Re-ranking with LLM feedback is not yet implemented; planned for a future release.

## Breaking Changes
- HybridSearch API (RRF weights, mode selection) is stable from v1.2.0.
- BM25 default parameters (k1=1.2, b=0.75) may be tuned in v1.5.0 but remain configurable.
