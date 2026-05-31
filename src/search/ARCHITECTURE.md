# Architecture - Search Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The search module composes lexical retrieval, vector retrieval, hybrid result fusion, distributed shard merging, and query/result utility layers into a bounded search subsystem.

## Main Execution Planes

1. Retrieval and fusion plane
- lexical and vector candidate generation behavior
- RRF/linear fusion and score normalization support behavior

2. Distributed merge plane
- shard-result merge and failure-tolerant composition behavior
- k-limit and overlap-aware merge behavior

3. Query/result utility plane
- expansion, fuzzy, faceting, reranking, analytics, and streaming behavior

## Core Contracts

| Contract | Behavior |
|---|---|
| retrieval contract | deterministic lexical/vector candidate behavior |
| fusion contract | explicit hybrid/distributed merge semantics |
| utility contract | bounded query expansion/reranking/faceting behavior |
| observability contract | explicit analytics and stream result visibility |

## Failure Semantics

- backend candidate deficits surface explicit partial/degraded outcomes.
- distributed shard failures remain explicit in merge behavior.
- utility-layer failures remain non-silent and diagnosable.
- configuration-bound limits are enforced deterministically.

## Sourcecode Verification (Module: search/architecture)

- Verified files:
  - src/search/hybrid_search.cpp
  - src/search/distributed_hybrid_search.cpp
  - src/search/query_expander.cpp
  - src/search/faceted_search.cpp
  - src/search/llm_reranker.cpp
  - src/search/search_result_stream.cpp
- Verified architecture claims:
  - retrieval/fusion + distributed merge + utility plane split
  - explicit failure boundaries for candidate, shard, and utility path faults
  - module-local ownership of search behavior composition