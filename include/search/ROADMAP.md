<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Search Module Roadmap

## Current Status

v2.3.0 — production. Distributed hybrid search with mTLS + RRF, negative keyword filter, search highlighting, LLM rewriting/reranking, neural sparse retrieval, multi-modal search, cross-lingual search, and analytics are all operational.

## Completed

- [x] `Autocomplete`, `FuzzyMatcher`, `FacetedSearch`
- [x] `HybridSearch` BM25 + vector single-node
- [x] `MultiFieldSearch`, `QueryExpander`
- [x] `NeuralSparseRetrieval` SPLADE-style
- [x] `MultiModalSearch` text + image + audio
- [x] `LlmQueryRewriter` and `LlmReranker`
- [x] `LearningToRank` LTR model training
- [x] `PersonalizedRanker` user-preference ranking
- [x] `CrossLingualSearch` cross-language support
- [x] `SearchAnalytics` CTR and zero-result tracking
- [x] `SearchHighlighter` inline spans and contextual snippets
- [x] `NegativeKeywordFilter` NOT/minus-prefix
- [x] `DistributedHybridSearch` mTLS + RRF with `SearchStats`

## Implementation Phases

### Phase 1 — Core Search ✅
- [x] BM25 inverted index, fuzzy matching, autocomplete
- [x] Faceted search, multi-field search

### Phase 2 — Hybrid & Neural ✅
- [x] HybridSearch BM25 + vector
- [x] NeuralSparseRetrieval SPLADE
- [x] QueryExpander synonym + semantic

### Phase 3 — LLM & Multi-Modal ✅
- [x] LlmQueryRewriter + LlmReranker
- [x] MultiModalSearch
- [x] LearningToRank, PersonalizedRanker

### Phase 4 — Distribution & Operators ✅
- [x] DistributedHybridSearch mTLS + RRF
- [x] NegativeKeywordFilter NOT/minus
- [x] SearchHighlighter spans + snippets

### Phase 5 — Future Enhancements (Planned)
- [ ] Voice search (speech-to-query) via `MultiModalSearch` (Target: Q3 2026)
- [ ] Federated search across isolated tenant indexes (Target: Q4 2026)
- [ ] Conversational search (multi-turn query context) (Target: Q4 2026)

### Phase 6 — Documentation & Acceptance ✅
- [x] All 17 headers documented
- [x] AUDIT.md — 0 open stubs

## Production Readiness Checklist

- [x] DistributedHybridSearch tested across 16-shard cluster
- [x] NegativeKeywordFilter validated with TREC adversarial query set
- [x] SearchHighlighter output validated for HTML injection safety
- [ ] Voice search integration (Target: Q3 2026)
