# Architecture - Search Module

<!-- Status: current | validated: 2026-09-09 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

Last Updated: 2026-09-09
Module Path: src/search/
Status: PRODUCTION (Wave B GA, 2026-08-17/18; Score 95/100, 0 gaps)

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
- **Phase 2 Enhancements:** SearchStats degradation flags (primary_error_code, fusion_failed, rerank_fallback)

### v2.2.0 — DistributedHybridSearch API (Phase 2 Enhanced)
**Status:** FROZEN | Released 2026-08-06 | Phase 2 Hardening Complete
- Cross-shard merge and distributed hybrid behavior with Phase 2 enhancements
- RRF-based global rank fusion across shards
- Fault-tolerant shard query with graceful degradation
- Per-shard result composition with explicit SearchStats including degradation flags
- Configurable skip_failed_shards behavior
- **Error Taxonomy:** Distributed merge errors (0x2000-0x2FFF)
- Explicit partial_result flag when at least one shard failed
- **Phase 2 Degradation Flags:**
  - merge_underflow: True when result count < k due to insufficient candidates
  - high_overlap_variance: True when high-cardinality overlap detected (>50% of shards)
  - failed_shard_reasons: Vector tracking reason for each failed shard ("timeout", "HTTP 500", etc.)
- **Guarantees:** Results from surviving shards remain consistent; failed shards produce empty/degraded results without data corruption
- **mergeShardResults() Enhancement:** Now detects and tracks merge underflow and overlap variance through SearchStats output parameter

### v2.0.0 — SearchResultStream API (Frozen)
**Status:** FROZEN | Released 2026-08-06
- Streaming result delivery with cursor-based pagination
- Configurable page_size and total_k materialisation
- Timeout support via open_timeout_ms (default 30s)
- Exception safety: open() and nextPage() never throw
- Constructor throws std::invalid_argument on invalid config
- **Error Taxonomy:** Analytics errors (0x4000-0x4FFF)

### v3.0.0 — LayeredRetrievalOrchestrator API (New, Wave B)
**Status:** NEW | Released 2026-08-18 | Wave B Integration Complete
- Four-stage retrieval pipeline: ANN → Tensor → Graph → LLM
- Per-layer timeout enforcement (hard deadline, fire-and-forget on timeout)
- Per-query guardrails bound memory and fan-out (max_layers, max_candidates, max_prompt_chars)
- OpenTelemetry distributed tracing with per-layer spans
- Fail-safe: missing backends or timeouts degrade to FALLBACK, chain continues
- Thread-safe concurrent execute() calls if backends are thread-safe
- Never throws: all errors captured in LayeredRetrievalResult
- **Error Taxonomy:** Layered retrieval routing decisions (EXECUTED, FALLBACK, TIMEOUT_SKIP, GUARDRAIL_SKIP, DISABLED)
- **Phase 4+5 Enhancements:** Real layer integration (not mocks), timeout enforcement, concurrency controls, tracing spans, guardrail enforcement
- **Performance:** p99 ≤ 200ms (ANN+Tensor) to 300ms (full 4-layer with LLM)
- **Memory:** Bounded ~13.5 KB per query (all arrays pre-reserved)
- **Documentation:** See LAYERED_RETRIEVAL_ARCHITECTURE.md and LAYERED_RETRIEVAL_SLA.md for full design and SLA contracts

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
  - src/search/layered_retrieval_orchestrator.cpp (v3.0.0 NEW, Wave B real implementation)
  - include/search/layered_retrieval_orchestrator.h (v3.0.0 NEW, Wave B real implementation)
  - include/search/search_error_codes.h (v1.0.0)
  - src/search/query_expander.cpp
  - src/search/faceted_search.cpp
  - src/search/llm_reranker.cpp
- Verified architecture claims:
  - retrieval/fusion + distributed merge + utility plane split
  - explicit failure boundaries for candidate, shard, and utility path faults
  - module-local ownership of search behavior composition
  - frozen versioned contracts for all major API surfaces
  - unified error taxonomy with explicit error codes
  - **Wave B: 4-layer LayeredRetrievalOrchestrator (ANN→Tensor→Graph→LLM) with real implementations, per-layer timeouts, guardrails, and OpenTelemetry tracing**
  - **Wave B: Documentation: LAYERED_RETRIEVAL_ARCHITECTURE.md, LAYERED_RETRIEVAL_SLA.md**

## Planning Traceability

- Wave B dependency planning issue: `#5039`
- Upstream planning context: Wave C `#5040`, Wave A `#5038`
- Phase 1 completion: 2026-08-06
- Note:
  - Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
  - dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
  - follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`

---

## Module Dependencies

### Direct Upstream Dependencies (this module uses)

| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| index | `include/index/ann_frontdoor.h` (`ANNFrontdoor`) | ANN candidate retrieval for vector search plane |
| index | `include/index/vector_index.h` (`IVectorIndex`) | Direct vector-index access in `HybridSearch` |
| metadata | `include/metadata/` | Metadata filtering applied during candidate generation |
| llm | `include/search/llm_reranker.h` → `LlmBackend` callback injected from `llm` module | Optional LLM re-ranking of fused candidates (`LlmReranker`); injected, not directly included |
| query | `include/query/` (FTS index surface) | Full-text search lexical candidate path |

### Direct Downstream Consumers (modules that use this module)

| Module | Via | Notes |
|--------|-----|-------|
| rag | `src/rag/hybrid_retriever.cpp` | RAG hybrid retrieval delegates to `HybridSearch` |
| api | API query handlers | External search endpoint routing |
| llm | `src/llm/ai_orchestrator.cpp` | Orchestrator triggers layered search for context fetching |

---

## Integration Points

### Critical Integration: HybridSearch v2.0 — Lexical + Vector Fusion (Contract Frozen)
**Files:** `include/search/hybrid_search.h` ↔ `src/search/hybrid_search.cpp`
**Contract:** `HybridSearch::search()` is unconditionally `noexcept`. Score 95/100; 0 open gaps. Supports RRF and linear-combination fusion, configurable distance metrics (COSINE, DOT, L2), bounded by `max_k`/`max_candidates`. Contract frozen at v2.0.0 (2026-08-06).
**Thread Safety:** Single instance NOT thread-safe; callers must synchronise or use per-thread instances.
**Failure Mode:** Backend candidate deficit → `SearchStats::partial_result = true`; fusion errors → `SearchStats::fusion_failed = true`; re-rank fallback → `SearchStats::rerank_fallback = true`.

### Critical Integration: LlmReranker — Injected LLM Backend
**Files:** `include/search/llm_reranker.h` ↔ `src/search/llm_reranker.cpp`
**Contract:** `LlmReranker::LlmBackend` is a `std::function<std::string(const std::string&)>`. No mock fallback — missing or null backend returns `llm_unavailable` and original ranking is preserved. `LLMJudgeIntegration` is equally fail-closed.
**Thread Safety:** `LlmReranker` instance is NOT thread-safe; `LlmBackend` callable must be re-entrant if shared.
**Failure Mode:** Backend null or throws → `rerank_fallback = true`; search result returned without re-ranking.

### Critical Integration: ANNFrontdoor — Stable ABI for Vector Index
**Files:** `include/index/ann_frontdoor.h` ↔ `src/search/hybrid_search.cpp`, `src/search/layered_retrieval_orchestrator.cpp`
**Contract:** `ANNFrontdoor` is the stable public ABI entry-point for ANN queries. Contract frozen at Wave B. `LayeredRetrievalOrchestrator` (v3.0.0) uses it as its ANN layer (first in 4-layer chain: ANN→Tensor→Graph→LLM).
**Thread Safety:** `ANNFrontdoor` must be thread-safe for concurrent `search()` calls; orchestrator does not add additional locking.
**Failure Mode:** ANN layer timeout → `TIMEOUT_SKIP` status; orchestrator continues to next layer.

### Critical Integration: LayeredRetrievalOrchestrator — 4-Layer Pipeline (Wave B)
**Files:** `include/search/layered_retrieval_orchestrator.h` ↔ `src/search/layered_retrieval_orchestrator.cpp`
**Contract:** v3.0.0. Four-stage pipeline: ANN → Tensor → Graph → LLM. Per-layer hard deadline enforcement. All errors captured in `LayeredRetrievalResult`; never throws. OpenTelemetry tracing per layer. Performance: p99 ≤ 200 ms (ANN+Tensor), ≤ 300 ms (full 4-layer with LLM). Memory bounded ~13.5 KB/query.
**Thread Safety:** Thread-safe for concurrent `execute()` calls if backends are individually thread-safe.
**Failure Mode:** Missing backend or timeout → `FALLBACK` or `TIMEOUT_SKIP`; chain continues with remaining layers.

### Critical Integration: DistributedHybridSearch v2.2.0 — Cross-Shard Merge
**Files:** `include/search/distributed_hybrid_search.h` ↔ `src/search/distributed_hybrid_search.cpp`
**Contract:** RRF-based global rank fusion across shards. Configurable `skip_failed_shards`. `mergeShardResults()` detects merge underflow and high-overlap variance via `SearchStats`. Partial results flagged explicitly.
**Thread Safety:** Shard queries run concurrently; result merge is single-threaded per request.
**Failure Mode:** Shard failure → `SearchStats::shards_failed > 0`, `partial_result = true`; surviving shard results remain consistent.

