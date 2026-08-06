/**
 * @file PHASE_3_ERROR_HANDLING_GUIDE.md
 * @brief Phase 3: Unified Error Handling and Fail-Safe Behavior Patterns
 * @date 2026-08-06
 * @version 1.0.0
 *
 * This document defines the unified error handling and fail-safe behavior patterns
 * to be implemented across all 14 search module components during Phase 3.
 */

# Phase 3: Unified Error Handling and Fail-Safe Behavior

## Overview

Phase 3 standardizes error handling across all retrieval, fusion, distributed, utility,
and analytics components to ensure:

1. **Explicit Error Tracking**: All failures use search_error_codes.h
2. **Graceful Degradation**: Failures in one layer don't cascade to user-facing API
3. **Transparent Fallback**: Performance optimizations (e.g., reranking) fail silently
4. **Operator Diagnostics**: SearchStats populate with explicit degradation flags

## Error Handling Patterns

### Pattern 1: Retrieval Layer (Lexical/Vector)

**Scope**: hybrid_search.cpp

**Failure Modes**:
- BM25 backend unavailable → SEARCH_ERR_BM25_BACKEND_UNAVAILABLE (0x0002)
- Vector backend unavailable → SEARCH_ERR_VECTOR_BACKEND_UNAVAILABLE (0x0003)
- No results found → SEARCH_ERR_EMPTY_RESULT_SET (0x0004)

**Fail-Safe Behavior**:
```
If BM25 fails but vector succeeds:
  → Return vector results
  → Set stats.partial_result = true
  → Set stats.primary_error_code = SEARCH_ERR_VECTOR_ONLY_AVAILABLE
  → Set stats.bm25_count = 0

If both fail:
  → Return empty result set
  → Set stats.primary_error_code = SEARCH_ERR_BOTH_BACKENDS_UNAVAILABLE
  → Set stats.partial_result = false
```

### Pattern 2: Fusion Layer (RRF/Linear Combination)

**Scope**: hybrid_search.cpp (search() method)

**Failure Modes**:
- RRF fusion normalization fails → SEARCH_ERR_RRF_FUSION_FAILED (0x1000)
- Score normalization overflow → SEARCH_ERR_NORMALIZATION_OVERFLOW (0x1001)
- Empty candidate set → SEARCH_ERR_FUSION_EMPTY_SET (0x1002)

**Fail-Safe Behavior**:
```
If fusion fails but candidates exist:
  → Use base ranking (one backend's order)
  → Set stats.fusion_failed = true
  → Set stats.partial_result = true
  → Set stats.primary_error_code = SEARCH_ERR_RRF_FUSION_FAILED
  → Return ranked results from most-reliable backend

If both backends empty:
  → Return empty result set
  → Set stats.primary_error_code = SEARCH_ERR_EMPTY_RESULT_SET
```

### Pattern 3: Distributed Merge Layer

**Scope**: distributed_hybrid_search.cpp (mergeShardResults())

**Failure Modes**:
- Shard timeout → SEARCH_ERR_SHARD_TIMEOUT (0x2000)
- Partial shard failure → SEARCH_ERR_PARTIAL_SHARD_FAILURE (0x2001)
- Merge underflow → SEARCH_ERR_MERGE_UNDERFLOW (0x2002)
- High-overlap variance → SEARCH_ERR_HIGH_OVERLAP_VARIANCE (0x2003)

**Fail-Safe Behavior**:
```
If some shards fail (with skip_failed_shards=true):
  → Merge results from successful shards only
  → Set stats.partial_result = true
  → Set stats.shards_failed > 0
  → Populate stats.failed_shard_reasons with "shard_id: reason"
  → Set stats.primary_error_code = SEARCH_ERR_PARTIAL_SHARD_FAILURE

If all shards fail:
  → Return empty result set
  → Set stats.shards_failed = stats.shards_queried
  → Set stats.primary_error_code = SEARCH_ERR_SHARD_TIMEOUT or appropriate code

If merge produces < k results:
  → Return all available results (don't pad)
  → Set stats.merge_underflow = true
  → Set stats.primary_error_code = SEARCH_ERR_MERGE_UNDERFLOW

If high overlap detected (>50% of shards):
  → Continue merge as normal
  → Set stats.high_overlap_variance = true
  → Log warning for operator visibility
```

### Pattern 4: Reranking Layer (LLM-based)

**Scope**: hybrid_search.cpp (optional reranking post-fusion)

**Failure Modes**:
- LLM backend unavailable → SEARCH_ERR_RERANKER_UNAVAILABLE (0x1002)
- Reranking timeout → SEARCH_ERR_RERANKER_TIMEOUT (0x1003)
- LLM processing error → SEARCH_ERR_RERANKER_ERROR (0x1004)

**Fail-Safe Behavior**:
```
If reranking fails:
  → Return unre-ranked results (base hybrid order)
  → Set stats.rerank_fallback = true
  → Set stats.partial_result = false  // Fallback is transparent
  → Log warning for operator visibility
  → Do NOT set partial_result (user doesn't see degradation)
```

### Pattern 5: Utility Layer (Expansion, Faceting, etc.)

**Scope**: query_expander.cpp, faceted_search.cpp, llm_reranker.cpp, etc.

**Failure Modes**:
- Query expansion limit exceeded → SEARCH_ERR_EXPANSION_LIMIT_EXCEEDED (0x3000)
- Facet overflow → SEARCH_ERR_FACET_LIMIT_EXCEEDED (0x3001)
- Fuzzy matching timeout → SEARCH_ERR_FUZZY_TIMEOUT (0x3002)

**Fail-Safe Behavior**:
```
If expansion limit exceeded:
  → Use limited expansion set (first N expansions)
  → Set stats.primary_error_code = SEARCH_ERR_EXPANSION_LIMIT_EXCEEDED
  → Continue with partial expansions

If faceting fails:
  → Return results without facet counts
  → Set stats.primary_error_code = SEARCH_ERR_FACET_OPERATION_FAILED
  → Still provide main results
```

### Pattern 6: Analytics/Streaming Layer

**Scope**: search_result_stream.cpp, search_analytics.cpp

**Failure Modes**:
- Stream buffer exhausted → SEARCH_ERR_STREAM_BUFFER_EXHAUSTED (0x4000)
- Stream open timeout → SEARCH_ERR_STREAM_OPEN_TIMEOUT (0x4001)
- Record encoding failure → SEARCH_ERR_RECORD_ENCODING_FAILED (0x4002)

**Fail-Safe Behavior**:
```
If stream open times out:
  → Log warning with elapsed time
  → Return partial stream or error status
  → Caller can retry or use fallback

If buffer exhausted during streaming:
  → Flush current batch and continue
  → Set stats.primary_error_code = SEARCH_ERR_STREAM_BUFFER_EXHAUSTED
```

## SearchStats Field Semantics

### Critical Fields (must always populate)

```cpp
struct SearchStats {
  bool partial_result;           // True if any layer failed but results still returned
  uint32_t primary_error_code;   // Primary failure code (0x0000 = SUCCESS)
  
  // Degradation tracking (set by responsible layer)
  bool fusion_failed;            // Fusion layer fallback applied
  bool rerank_fallback;          // Reranking layer fallback applied
  bool merge_underflow;          // Merge produced < k results
  bool high_overlap_variance;    // High-cardinality overlap detected
  
  // Diagnostic details
  std::vector<std::string> failed_shard_reasons;  // "shard_id: reason" per failure
  
  // Candidate counts (for caller diagnostics)
  size_t bm25_count;             // Raw BM25 candidates before fusion
  size_t vector_count;           // Raw vector candidates before fusion
};
```

### Semantic Rules

1. **partial_result = true** means:
   - At least one retrieval/fusion/distributed layer failed
   - But results were successfully returned from surviving layers
   - User should check primary_error_code for details

2. **primary_error_code = 0x0000** means:
   - All layers succeeded
   - No degradation applied
   - Results are fully authoritative

3. **rerank_fallback = true** implies:
   - partial_result may be false (fallback is transparent)
   - Base ranking used instead of LLM-optimized ranking
   - Performance may be slightly different from intended

4. **merge_underflow = true** means:
   - Merged results < k
   - This is informational; results are still valid
   - Caller may want to adjust k or queue a new search with different strategy

## Implementation Checklist

### Per-Component Phase 3 Tasks

- [ ] **hybrid_search.cpp**
  - [ ] Map all backend failures to SearchErrorCode enum
  - [ ] Populate SearchStats with primary_error_code on each path
  - [ ] Implement fusion_failed fallback when RRF fails
  - [ ] Implement rerank_fallback when LLM unavailable
  - [ ] Add THEMIS_WARN logs for partial_result paths

- [ ] **distributed_hybrid_search.cpp**
  - [ ] Populate SearchStats.failed_shard_reasons in search() method
  - [ ] Detect merge_underflow in mergeShardResults()
  - [ ] Detect high_overlap_variance in mergeShardResults()
  - [ ] Map shard errors to distributed error codes (0x2000-0x2FFF)

- [ ] **query_expander.cpp**
  - [ ] Enforce expansion limit; map overflow to 0x3000
  - [ ] Return partial expansions on limit exceeded
  - [ ] Set primary_error_code in return struct

- [ ] **faceted_search.cpp**
  - [ ] Enforce facet limit; map overflow to 0x3001
  - [ ] Return faceted results without facet counts on failure
  - [ ] Document partial faceting behavior

- [ ] **llm_reranker.cpp**
  - [ ] Catch LLM unavailability; return false from rerank() indicating fallback
  - [ ] Populate error struct with SEARCH_ERR_RERANKER_UNAVAILABLE

- [ ] **search_result_stream.cpp**
  - [ ] Enforce open_timeout_ms in open() method
  - [ ] Return timeout error with elapsed time on exceeded timeout
  - [ ] Implement stream buffer monitoring for exhaustion

- [ ] **Tests**
  - [ ] Add P3-01..P3-08 conformance tests
  - [ ] Add edge case tests for each component
  - [ ] Add stress tests for concurrent failures

## Verification

All Phase 3 implementations must pass:
1. Unit tests P3-01 through P3-08
2. Component-specific edge case tests
3. Build with no warnings
4. Code review for consistency with patterns

## Next Phase (Phase 4)

After Phase 3 completion, Phase 4 will extend test coverage with:
- Stress tests for high concurrency
- Failure injection for chaotic testing
- Performance regression validation
