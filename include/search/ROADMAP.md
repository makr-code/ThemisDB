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
  - "src/search/ROADMAP.md"
Related_Issue: "#7378"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/search/ROADMAP.md -->

# Search Module — Public Header Roadmap

**Module Path:** `include/search/`
**Canonical implementation roadmap:** [`../../src/search/ROADMAP.md`](../../src/search/ROADMAP.md)

---

## Overview

Tracks public search API contract stability, retrieval/fusion header coverage, and planned public entry points. Runtime implementation work remains in:

→ [`../../src/search/ROADMAP.md`](../../src/search/ROADMAP.md)

---

## Current Status

All 20 search headers are present and cover hybrid retrieval, distributed shard merge, query shaping, reranking, multimodal search, and analytics/stream result surfaces.

---

## Completed ✅

- [x] retrieval/fusion headers — `hybrid_search.h`, `distributed_hybrid_search.h`, `federated_search.h`, `neural_sparse_retrieval.h`
- [x] query utility headers — `query_expander.h`, `negative_keyword_filter.h`, `fuzzy_matcher.h`, `autocomplete.h`, `faceted_search.h`, `multi_field_search.h`
- [x] ranking/AI headers — `learning_to_rank.h`, `personalized_ranker.h`, `llm_query_rewriter.h`, `llm_reranker.h`, `multi_modal_search.h`
- [x] result/analytics headers — `search_highlighter.h`, `search_result_stream.h`, `search_analytics.h`

---

## In Progress

> The dated items below are historical planning entries, not evidence that the
> work remains active or that the proposed interfaces exist. Check the headers
> and `src/search/ROADMAP.md` before planning implementation.

- [ ] Clarify degraded-shard and partial-result guarantees across distributed/federated search headers (Target: 2026-Q3)
- [ ] Add stronger candidate-limit and rerank-failure guidance to public retrieval utility docs (Target: 2026-Q3)

---

## Planned

> Items below describe proposals only; no public contract should be inferred
> until a corresponding declaration is present in `include/search/`.

- [ ] `search_incident.h` — shared incident/diagnostic DTO for merge and rerank degradation (Target: 2026-Q4)
- [ ] `search_capability_profile.h` — capability summary for retrieval/rerank/multimodal features (Target: 2026-Q4)
- [ ] Document benchmark-backed compatibility notes for hybrid/distributed search hot paths (Target: 2026-Q4)

---

## Breaking Change History

None in v1.x. Public search headers must remain backward compatible within the active major line; contract changes require migration notes and changelog updates.
