<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Search Module Roadmap

## Current Status

v2.4.0 — production. Distributed hybrid search with mTLS + RRF, negative keyword filter, search highlighting, LLM rewriting/reranking, neural sparse retrieval, multi-modal search, cross-lingual search, and analytics are all operational. v2.4.0 adds Phase 5 interfaces: `ConversationalSearch` (multi-turn context), `FederatedSearch` (tenant-isolated indexes), and `SearchResultStream` (streaming pagination).

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
- [x] `ConversationalSearch` multi-turn context-aware search (v2.4.0)
- [x] `FederatedSearch` tenant-isolated indexes with per-tenant weighting (v2.4.0)
- [x] `SearchResultStream` cursor-based streaming pagination (v2.4.0)

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
- [x] Conversational search (multi-turn query context) (`ConversationalSearch`, v2.4.0)
- [x] Federated search across isolated tenant indexes (`FederatedSearch`, v2.4.0)
- [x] Streaming result delivery for large result sets (`SearchResultStream`, v2.4.0)
- [ ] Voice search (speech-to-query) via `MultiModalSearch` (Target: Q3 2026)

### Phase 6 — Documentation & Acceptance ✅
- [x] All 17 headers documented
- [x] AUDIT.md — 0 open stubs

## Production Readiness Checklist

- [x] DistributedHybridSearch tested across 16-shard cluster
- [x] NegativeKeywordFilter validated with TREC adversarial query set
- [x] SearchHighlighter output validated for HTML injection safety
- [x] ConversationalSearch validated with multi-turn session tests
- [x] FederatedSearch validated with multi-tenant isolation tests
- [x] SearchResultStream validated with cursor/pagination tests
- [ ] Voice search integration (Target: Q3 2026)
