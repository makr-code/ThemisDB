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

## In Progress 🚧
- [ ] LLM-based query rewriting for improved recall (Target: Q2 2026)
- [ ] Faceted search with dynamic facet counting (Target: Q2 2026)
- [ ] Highlight / snippet generation for matched terms (Target: Q3 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] Multi-field boosting (title > body > tags)
- [ ] Negative keyword filtering (`NOT` operator)
- [ ] Configurable re-ranking with LLM feedback loop
- [ ] Search analytics (top queries, zero-result queries)
- [ ] Spelling correction suggestions

### Long-term (6-12 months)
- [ ] Neural sparse retrieval (SPLADE / BERT-based)
- [ ] Cross-lingual semantic search (multilingual embeddings)
- [ ] Personalized ranking based on user interaction history
- [ ] Distributed search across shards with result merging
- [ ] Autocomplete / type-ahead query suggestions

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [ ] Integration tests (BM25 correctness, hybrid recall@10)
- [ ] Performance benchmarks (QPS, index build time, latency p99)
- [ ] Security audit (query injection, resource exhaustion on large datasets)
- [ ] Documentation complete
- [ ] API stability guaranteed

## Known Issues & Limitations
- Synonym expansion dictionary must be manually provided; no automatic synonym discovery.
- Phonetic search accuracy varies by language; optimized for English.
- Re-ranking with LLM feedback is not yet implemented; planned for a future release.

## Breaking Changes
- HybridSearch API (RRF weights, mode selection) is stable from v1.2.0.
- BM25 default parameters (k1=1.2, b=0.75) may be tuned in v1.5.0 but remain configurable.
