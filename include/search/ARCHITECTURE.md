---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "unknown"
Source_Level: "level1"
SOT_Domain: "api-contract"
Canonical_References:
  - "include/search/*.h"
  - "src/search/ARCHITECTURE.md"
  - "cmake/ModularBuild.cmake"
Related_Issue: "#7378"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

<!-- Status: current | validated: 2026-08-06 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/search/ARCHITECTURE.md -->

# Search Module — Public Header Architecture

**Module Path:** `include/search/`
**Implementation:** `../../src/search/`
**Canonical architecture doc:** [`../../src/search/ARCHITECTURE.md`](../../src/search/ARCHITECTURE.md)

---

## 1. Overview

`include/search/` defines the **public retrieval, fusion, and search-utility contract** for ThemisDB. The 20 headers cover lexical/vector and hybrid retrieval, distributed shard merge, faceting and fuzzy matching, query expansion, LLM-assisted rewrite/rerank flows, multimodal search, and analytics/streaming helpers.

For runtime composition details — hybrid merge, reranking, and distributed search internals — see:
→ [`../../src/search/ARCHITECTURE.md`](../../src/search/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Retrieval and Fusion

| Header | Public Type | Purpose |
|--------|------------|---------|
| `hybrid_search.h`, `distributed_hybrid_search.h` | Hybrid search types | Lexical/vector fusion and shard merge |
| `federated_search.h` | `FederatedSearch` | Cross-backend search orchestration |
| `cross_lingual_search.h`, `conversational_search.h` | Search adaptation types | Cross-language and conversational search |
| `neural_sparse_retrieval.h` | `NeuralSparseRetrieval` | Sparse-neural retrieval contract |

### 2.2 Query and Ranking Utilities

| Header | Public Type | Purpose |
|--------|------------|---------|
| `query_expander.h`, `negative_keyword_filter.h` | Query-shaping types | Expansion and filtering |
| `fuzzy_matcher.h`, `autocomplete.h`, `faceted_search.h`, `multi_field_search.h` | Utility types | Fuzzy, prefix, facet, and field-weighted search |
| `learning_to_rank.h`, `personalized_ranker.h` | Ranking types | Trainable and personalized ranking |

### 2.3 AI and Multimodal Search

| Header | Public Type | Purpose |
|--------|------------|---------|
| `llm_query_rewriter.h`, `llm_reranker.h` | LLM-assisted types | Rewrite and rerank |
| `multi_modal_search.h` | `MultiModalSearch` | Multi-modal retrieval |

### 2.4 Result Analytics and Streaming

| Header | Public Type | Purpose |
|--------|------------|---------|
| `search_highlighter.h`, `search_result_stream.h` | Result-presentation types | Highlighting and streaming cursors |
| `search_analytics.h` | `SearchAnalytics` | Search metrics and behavior analysis |

---

## 3. Namespace Layout

| Namespace | Declared types |
|---|---|
| `themis` | Most public search APIs, including `HybridSearch` and the utilities listed above |
| `themis::search` | `LayeredRetrievalOrchestrator` and its layer/result/config types |

This is not a promise that all search headers use a common namespace; the
individual declarations in each public header are authoritative.

---

## 4. Public Contract Notes

- Retrieval and fusion headers remain public because API, query, and application layers compose search behavior directly.
- Distributed and federated search contracts expose explicit degraded/partial-result behavior for multi-shard and multi-backend use cases.
- Query-shaping, ranking, and multimodal headers stay public so deployers can customize retrieval pipelines without replacing core search orchestration.
- Analytics and streaming surfaces provide operational visibility and incremental result delivery to higher layers.

The header directory is a contract surface, not evidence that each component is
compiled into every executable. Check the target's active source list in
`cmake/ModularBuild.cmake` or its owning CMake target before inferring linkage.
