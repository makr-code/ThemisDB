# Architecture - Search Module

<!-- Status: current | validated: 2026-08-06 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The search module composes lexical retrieval, vector retrieval, hybrid result fusion, distributed shard merging, and query/result utility layers into a bounded search subsystem.

## Versioned Contracts (Phase 1: Contract Freeze)

### v2.0.0 — HybridSearch API (Frozen)
**Status:** FROZEN | Released 2026-08-06
- Hybrid lexical/vector fusion behavior
- RRF/linear combination fusion modes
- Score normalization with edge-case handling
- Configurable vector distance metric (COSINE, DOT, L2)
- Bounded resource usage via max_k / max_candidates limits
- Optional LLM re-ranking via injected LlmBackend (v1.8.0+)
- Exception Safety: search() is unconditionally noexcept; catches all backend exceptions internally
- Thread Safety: Single instance NOT thread-safe; callers must synchronize or use per-thread instances
- **Error Taxonomy:** Retrieval errors (0x0000-0x0FFF), Fusion errors (0x1000-0x1FFF)

### v2.1.0 — DistributedHybridSearch API (Frozen)
**Status:** FROZEN | Released 2026-08-06
- Cross-shard merge and distributed hybrid behavior
- RRF-based global rank fusion across shards
- Fault-tolerant shard query with graceful degradation
- Per-shard result composition with explicit SearchStats
- Configurable skip_failed_shards behavior
- **Error Taxonomy:** Distributed merge errors (0x2000-0x2FFF)
- Explicit partial_result flag when at least one shard failed
- **Guarantees:** Results from surviving shards remain consistent; failed shards produce empty/degraded results without data corruption

### v2.0.0 — SearchResultStream API (Frozen)
**Status:** FROZEN | Released 2026-08-06
- Streaming result delivery with cursor-based pagination
- Configurable page_size and total_k materialisation
- Timeout support via open_timeout_ms (default 30s)
- Exception safety: open() and nextPage() never throw
- Constructor throws std::invalid_argument on invalid config
- **Error Taxonomy:** Analytics errors (0x4000-0x4FFF)

### v1.0.0 — SearchErrorCode Taxonomy (Frozen)
**Status:** FROZEN | Released 2026-08-06
- Explicit error enumeration for all search failure modes
- 32-bit error codes organized by category
- Conversion function: searchErrorCodeToString()
- Categories:
  - Retrieval errors (0x0000-0x0FFF): backend unavailability, partial results
  - Fusion errors (0x1000-0x1FFF): RRF/normalization failures, empty sets
  - Distributed merge errors (0x2000-0x2FFF): shard failures, merge conflicts, overlap variance
  - Utility errors (0x3000-0x3FFF): expansion, reranking, faceting limit violations
  - Analytics errors (0x4000-0x4FFF): buffer exhaustion, record failures

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

| Contract | Behavior | Error Codes |
|---|---|---|
| retrieval contract | deterministic lexical/vector candidate behavior | 0x0001-0x0007 |
| fusion contract | explicit hybrid/distributed merge semantics | 0x1001-0x1005 |
| distributed contract | fault-tolerant shard composition | 0x2001-0x2008 |
| utility contract | bounded query expansion/reranking/faceting behavior | 0x3001-0x3009 |
| observability contract | explicit analytics and stream result visibility | 0x4001-0x4002 |

## Failure Semantics

- backend candidate deficits surface explicit partial/degraded outcomes (SearchStats::partial_result = true)
- distributed shard failures remain explicit in merge behavior (SearchStats::shards_failed > 0)
- utility-layer failures remain non-silent and diagnosable (error codes logged via THEMIS_ERROR)
- configuration-bound limits are enforced deterministically
- **Fallback behavior:** All components support graceful fallback to partial/empty results rather than throwing

## Sourcecode Verification (Module: search/architecture)

- Verified files:
  - src/search/hybrid_search.cpp (v2.0.0, fixed destructor noexcept)
  - src/search/distributed_hybrid_search.cpp (v2.1.0)
  - src/search/search_result_stream.cpp (v2.0.0, added timeout support)
  - include/search/search_error_codes.h (v1.0.0 NEW)
  - src/search/query_expander.cpp
  - src/search/faceted_search.cpp
  - src/search/llm_reranker.cpp
- Verified architecture claims:
  - retrieval/fusion + distributed merge + utility plane split
  - explicit failure boundaries for candidate, shard, and utility path faults
  - module-local ownership of search behavior composition
  - frozen versioned contracts for all major API surfaces
  - unified error taxonomy with explicit error codes

## Planning Traceability

- Wave B dependency planning issue: `#5039`
- Upstream planning context: Wave C `#5040`, Wave A `#5038`
- Phase 1 completion: 2026-08-06
- Note:
  - Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
  - dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
  - follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`
