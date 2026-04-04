<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Search Module — Architecture Guide

## Overview

The search module provides full-text, hybrid, and neural search capabilities for ThemisDB: autocomplete, fuzzy matching, faceted search, multi-field and multi-modal search, cross-lingual search, neural sparse retrieval, LLM-based query rewriting and reranking, personalized ranking, learning-to-rank, distributed hybrid search with mTLS and RRF merging, negative keyword filtering, and search analytics.

## Design Principles

- **Distributed by design** — `DistributedHybridSearch` merges results from multiple nodes with Reciprocal Rank Fusion (RRF); mTLS enforced on all node-to-node connections.
- **LLM-augmented** — `LlmQueryRewriter` and `LlmReranker` use LLM APIs to improve query intent and result relevance.
- **Negative keywords** — `NegativeKeywordFilter` supports NOT operator and minus-prefix (`-keyword`) syntax.
- **Highlighted snippets** — `SearchHighlighter` generates highlight spans and contextual snippets from matched documents.
- **Analytics-driven** — `SearchAnalytics` tracks query patterns, click-through, and zero-result rates.

## Interface Inventory

| Header | Classes / Interfaces | Purpose |
|---|---|---|
| `autocomplete.h` | `Autocomplete` | Real-time prefix autocomplete with ranking |
| `cross_lingual_search.h` | `CrossLingualSearch` | Cross-language query translation and search |
| `distributed_hybrid_search.h` | `DistributedHybridSearch`, `SearchStats` | Multi-node hybrid search with mTLS; RRF merging; `shards_failed` / `shards_queried` stats |
| `faceted_search.h` | `FacetedSearch` | Facet extraction and filtering |
| `fuzzy_matcher.h` | `FuzzyMatcher` | Edit-distance fuzzy term matching |
| `hybrid_search.h` | `HybridSearch` | Single-node BM25 + vector hybrid search |
| `learning_to_rank.h` | `LearningToRank` | LTR model training and scoring |
| `llm_query_rewriter.h` | `LlmQueryRewriter` | LLM-based query intent rewriting |
| `llm_reranker.h` | `LlmReranker` | LLM-based result reranking |
| `multi_field_search.h` | `MultiFieldSearch` | Weighted multi-field search |
| `multi_modal_search.h` | `MultiModalSearch` | Text + image + audio search |
| `negative_keyword_filter.h` | `NegativeKeywordFilter` | NOT operator and minus-prefix exclusion filter |
| `neural_sparse_retrieval.h` | `NeuralSparseRetrieval` | Neural sparse (SPLADE-style) retrieval |
| `personalized_ranker.h` | `PersonalizedRanker` | User-preference-based result ranking |
| `query_expander.h` | `QueryExpander` | Synonym and semantic query expansion |
| `search_analytics.h` | `SearchAnalytics` | Query patterns, CTR, zero-result tracking |
| `search_highlighter.h` | `SearchHighlighter` | Highlight spans and snippet generation |

## Integration Points

| Integrates With | Via | Notes |
|---|---|---|
| `index` | `HybridSearch`, `NeuralSparseRetrieval` | Inverted index and vector index access |
| `llm` | `LlmQueryRewriter`, `LlmReranker` | LLM provider calls |
| `observability` | `SearchAnalytics` | Query metrics and zero-result alerting |
| `network` | `DistributedHybridSearch` | mTLS inter-node connections |
| `rag` | `HybridSearch`, `QueryExpander` | Retrieval for RAG pipelines |

## Implementation

Implementation in `../../src/search/`.
