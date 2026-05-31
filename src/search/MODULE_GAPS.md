# search Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: search
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 298
- Actionable Findings (Critical + High): 134
- Affected Files: 20

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 19 |
| High | 115 |
| Medium | 164 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| container | 91 |
| performance_patterns | 76 |
| reliability | 69 |
| distributed_consistency | 20 |
| determinism | 18 |
| performance | 8 |
| concurrency | 6 |
| raii | 3 |
| llm_ai_safety | 2 |
| observability | 2 |
| platform | 2 |
| security | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/search/query_expander.cpp | 39 | 1 | 17 | 21 | 0 |
| src/search/distributed_hybrid_search.cpp | 34 | 6 | 18 | 10 | 0 |
| src/search/hybrid_search.cpp | 27 | 1 | 12 | 14 | 0 |
| src/search/llm_reranker.cpp | 21 | 1 | 11 | 9 | 0 |
| src/search/autocomplete.cpp | 20 | 1 | 4 | 15 | 0 |
| src/search/search_highlighter.cpp | 19 | 0 | 3 | 16 | 0 |
| src/search/cross_lingual_search.cpp | 16 | 2 | 7 | 7 | 0 |
| src/search/multi_modal_search.cpp | 14 | 0 | 4 | 10 | 0 |
| src/search/faceted_search.cpp | 13 | 1 | 1 | 11 | 0 |
| src/search/federated_search.cpp | 13 | 0 | 5 | 8 | 0 |
| src/search/llm_query_rewriter.cpp | 12 | 1 | 4 | 7 | 0 |
| src/search/neural_sparse_retrieval.cpp | 12 | 3 | 3 | 6 | 0 |
| src/search/negative_keyword_filter.cpp | 11 | 1 | 1 | 9 | 0 |
| src/search/search_analytics.cpp | 10 | 0 | 6 | 4 | 0 |
| src/search/search_result_stream.cpp | 9 | 1 | 4 | 4 | 0 |
| src/search/fuzzy_matcher.cpp | 8 | 0 | 3 | 5 | 0 |
| src/search/multi_field_search.cpp | 7 | 0 | 3 | 4 | 0 |
| src/search/learning_to_rank.cpp | 5 | 0 | 3 | 2 | 0 |
| src/search/conversational_search.cpp | 4 | 0 | 2 | 2 | 0 |
| src/search/personalized_ranker.cpp | 4 | 0 | 4 | 0 | 0 |

## Full Scanner Findings

### src/search/query_expander.cpp
Total findings: 39

- Line 106: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = synonyms_.find(tok);
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #3374 [search] Improve spelling correction suggestions with frequency-wei... (2026-03-12T07:07
- Line 28: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("QueryExpander: max_edit_distance must be >= 0");
- Line 31: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("QueryExpander: max_expansions must be > 0");
- Line 48: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (s != key && std::find(entry.begin(), entry.end(), s) == entry.end()) {
- Line 105: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = synonyms_.find(tok);
- Line 105: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = synonyms_.find(tok);
- Line 105: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = synonyms_.find(tok);
- Line 111: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (std::find(work_tokens.begin(), work_tokens.end(), syn) == work_tokens.end() &&
  Confidence: band=very_high; score=0.9
- Line 111: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(work_tokens.begin(), work_tokens.end(), syn) == work_tokens.end() &&
- Line 111: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(work_tokens.begin(), work_tokens.end(), syn) == work_tokens.end() &&
- Line 112: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: std::find(added_synonyms.begin(), added_synonyms.end(), syn) == added_synonyms.end()) {
  Confidence: band=very_high; score=0.9
- Line 112: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: std::find(added_synonyms.begin(), added_synonyms.end(), syn) == added_synonyms.end()) {
- Line 112: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: std::find(added_synonyms.begin(), added_synonyms.end(), syn) == added_synonyms.end()) {
- Line 200: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = word_frequencies_.find(sc.suggestion);
- Line 366: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (std::find(alternatives.begin(), alternatives.end(), alt) == alternatives.end()) {
  Confidence: band=very_high; score=0.9
- Line 366: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(alternatives.begin(), alternatives.end(), alt) == alternatives.end()) {
- Line 366: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(alternatives.begin(), alternatives.end(), alt) == alternatives.end()) {
- Line 48: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entry.push_back(s);
  Confidence: band=high; score=0.74
- Line 49: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entry.push_back(s);
- Line 88: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: corrected_tokens.push_back(c);
  Confidence: band=high; score=0.74
- Line 89: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: corrected_tokens.push_back(c);
- Line 112: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: added_synonyms.push_back(syn);
  Confidence: band=high; score=0.74
- Line 112: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: added_synonyms.push_back(syn);
  Confidence: band=high; score=0.74
- Line 112: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: added_synonyms.push_back(syn);
  Confidence: band=high; score=0.74
- Line 113: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: added_synonyms.push_back(syn);
- Line 124: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.all_terms.push_back(syn);
  Confidence: band=high; score=0.74
- Line 125: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.all_terms.push_back(syn);
- Line 189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back(sc);
  Confidence: band=high; score=0.74
- Line 190: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: candidates.push_back(sc);
- Line 306: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(sc);
  Confidence: band=high; score=0.74
- Line 307: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(sc);
- Line 366: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: alternatives.push_back(alt);
  Confidence: band=high; score=0.74
- Line 366: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: alternatives.push_back(alt);
  Confidence: band=high; score=0.74
- Line 366: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: alternatives.push_back(alt);
  Confidence: band=high; score=0.74
- Line 367: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: alternatives.push_back(alt);
- Line 375: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string QueryExpander::relaxQuery(const std::string& query) const {
  Confidence: band=high; score=0.74
- Line 405: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(clean);
  Confidence: band=high; score=0.74
- Line 406: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(clean);

### src/search/distributed_hybrid_search.cpp
Total findings: 34

- Line 50: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (config_.max_concurrent_shards == 0) {
  Confidence: band=very_high; score=0.99
- Line 52: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: "DistributedHybridSearch: Config::max_concurrent_shards must be > 0");
  Confidence: band=very_high; score=0.99
- Line 301: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged.push_back(std::move(r));
  Confidence: band=very_high; score=0.99
- Line 304: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::sort(merged.begin(), merged.end(),
  Confidence: band=very_high; score=0.99
- Line 309: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (merged.size() > config_.k) {
  Confidence: band=very_high; score=0.99
- Line 310: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged.resize(config_.k);
  Confidence: band=very_high; score=0.99
- Line 43: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 47: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 51: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 55: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 171: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: shard_results.push_back(futures[i].get());
  Confidence: band=very_high; score=0.9
- Line 235: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // --- Merge ---
  Confidence: band=very_high; score=0.9
- Line 236: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return mergeShardResults(shard_results);
  Confidence: band=very_high; score=0.9
- Line 240: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Cross-Shard RRF Merge
  Confidence: band=very_high; score=0.9
- Line 243: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<HybridSearch::Result> DistributedHybridSearch::mergeShardResults(
  Confidence: band=very_high; score=0.9
- Line 290: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<HybridSearch::Result> merged;
  Confidence: band=very_high; score=0.9
- Line 291: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.reserve(doc_map.size());
  Confidence: band=very_high; score=0.9
- Line 301: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.push_back(std::move(r));
  Confidence: band=very_high; score=0.9
- Line 304: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::sort(merged.begin(), merged.end(),
  Confidence: band=very_high; score=0.9
- Line 309: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: if (merged.size() > config_.k) {
  Confidence: band=very_high; score=0.9
- Line 310: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.resize(config_.k);
  Confidence: band=very_high; score=0.9
- Line 313: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: THEMIS_INFO("DistributedHybridSearch: merged {} shards -> {} results",
  Confidence: band=very_high; score=0.9
- Line 314: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: shard_results.size(), merged.size());
  Confidence: band=very_high; score=0.9
- Line 316: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return merged;
  Confidence: band=very_high; score=0.9
- Line 135: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: remote_shards.push_back(shard);
- Line 153: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(
  Confidence: band=high; score=0.74
- Line 154: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(std::async(
- Line 170: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_results.push_back(futures[i].get());
  Confidence: band=high; score=0.74
- Line 170: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_results.push_back(futures[i].get());
  Confidence: band=high; score=0.74
- Line 171: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: shard_results.push_back(futures[i].get());
- Line 180: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: shard_results.push_back(std::move(timeout_result));
- Line 257: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, Accum> doc_map;
  Confidence: band=medium; score=0.66
- Line 300: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 301: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged.push_back(std::move(r));

### src/search/hybrid_search.cpp
Total findings: 27

- Line 272: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = result_map.find(rr.document_id);
- Line 52: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("HybridSearch: Config::k must be > 0");
- Line 55: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("HybridSearch: Config::max_k must be > 0");
- Line 58: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("HybridSearch: Config::k exceeds max_k");
- Line 61: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("HybridSearch: Config::max_candidates must be > 0");
- Line 64: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("HybridSearch: Config::k_bm25 exceeds max_candidates");
- Line 67: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("HybridSearch: Config::k_vector exceeds max_candidates");
- Line 70: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("HybridSearch: Config::rrf_k must be > 0");
- Line 73: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("HybridSearch: weights must be non-negative");
- Line 76: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("HybridSearch: Config::default_table must not be empty");
- Line 79: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("HybridSearch: Config::default_column must not be empty");
- Line 271: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = result_map.find(rr.document_id);
- Line 271: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = result_map.find(rr.document_id);
- Line 133: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: bm25_results.push_back(r);
  Confidence: band=high; score=0.74
- Line 134: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: bm25_results.push_back(r);
- Line 164: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vector_results.push_back(r);
  Confidence: band=high; score=0.74
- Line 165: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vector_results.push_back(r);
- Line 215: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, Result> doc_map;
  Confidence: band=medium; score=0.66
- Line 233: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fused.push_back(result);
- Line 258: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: candidates.push_back(std::move(c));
- Line 267: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, const Result*> result_map;
  Confidence: band=medium; score=0.66
- Line 275: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: reranked_results.push_back(std::move(out));
  Confidence: band=high; score=0.74
- Line 275: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: reranked_results.push_back(std::move(out));
  Confidence: band=high; score=0.74
- Line 276: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: reranked_results.push_back(std::move(out));
- Line 327: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fused_results.push_back(result);
  Confidence: band=high; score=0.74
- Line 327: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fused_results.push_back(result);
  Confidence: band=high; score=0.74
- Line 328: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fused_results.push_back(result);

### src/search/llm_reranker.cpp
Total findings: 21

- Line 86: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Sort by final_score descending for consistent ordering regardless of input order
  Confidence: band=very_high; score=0.99
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #2648 [search] Configurable re-ranking with LLM feedback loop (Phase 3) (2026-03-12T05:53:53Z)
- Line 30: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("LlmReranker: batch_size must be > 0");
- Line 33: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("LlmReranker: llm_weight must be in [0, 1]");
- Line 33: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: throw std::invalid_argument("LlmReranker: llm_weight must be in [0, 1]");
- Line 36: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("LlmReranker: max_snippet_length must be > 0");
- Line 39: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("LlmReranker: min_score_threshold must be in [0, 1]");
- Line 39: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: throw std::invalid_argument("LlmReranker: min_score_threshold must be in [0, 1]");
- Line 42: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("LlmReranker: temperature must be in [0, 2]");
- Line 42: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: throw std::invalid_argument("LlmReranker: temperature must be in [0, 2]");
- Line 86: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Sort by final_score descending for consistent ordering regardless of input order
  Confidence: band=very_high; score=0.9
- Line 247: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: scores.push_back(val / 10.0); // normalise to [0, 1]
- Line 84: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(r);
- Line 116: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 138: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(r);
  Confidence: band=high; score=0.74
- Line 139: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(r);
- Line 181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: events.push_back(ev);
  Confidence: band=high; score=0.74
- Line 182: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: events.push_back(ev);
- Line 247: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: scores.push_back(val / 10.0); // normalise to [0, 1]
- Line 248: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 255: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: scores.push_back(0.0);

### src/search/autocomplete.cpp
Total findings: 20

- Line 7: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: * PR: #1335 Search module: v1.4.0 hardening + v1.5.0 feature set (7 new compone... (2026-03-11T21:30
- Line 34: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AutocompleteEngine: max_suggestions must be > 0");
- Line 37: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AutocompleteEngine: min_prefix_length must be > 0");
- Line 90: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: THEMIS_DEBUG("AutocompleteEngine::suggest('{}') -> {} suggestions", prefix, combined.size());
- Line 174: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(limit, matches.size()); ++i) {
- Line 56: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto& s : popular) combined.push_back(std::move(s));
- Line 60: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (auto& s : prefix_sug) combined.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 61: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto& s : prefix_sug) combined.push_back(std::move(s));
- Line 66: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 74: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: deduped.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 74: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: deduped.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 75: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: deduped.push_back(std::move(s));
- Line 114: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: true, false, // [prefix, prefix+'\xff')
- Line 135: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: suggestions.push_back(s);
  Confidence: band=high; score=0.74
- Line 136: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: suggestions.push_back(s);
- Line 139: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto val_opt = entities[0].getFieldAsString(column);
  Confidence: band=high; score=0.74
- Line 144: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: suggestions.push_back(std::move(s));
- Line 166: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matches.emplace_back(query, count);
  Confidence: band=high; score=0.74
- Line 178: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: suggestions.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 179: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: suggestions.push_back(std::move(s));

### src/search/search_highlighter.cpp
Total findings: 19

- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #3377 [WIP] Add highlight functionality for matched search terms (2026-03-12T07:07:43Z)
- Line 110: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: while ((pos = lower_text.find(term, pos)) != std::string::npos) {
- Line 182: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: while ((pos = search_text.find(term, pos)) != std::string::npos) {
- Line 44: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(std::move(current));
  Confidence: band=high; score=0.74
- Line 45: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(std::move(current));
- Line 57: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(std::move(current));
- Line 111: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matches.push_back({pos, pos + term.size(), ti});
  Confidence: band=high; score=0.74
- Line 111: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matches.push_back({pos, pos + term.size(), ti});
  Confidence: band=high; score=0.74
- Line 112: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: matches.push_back({pos, pos + term.size(), ti});
- Line 183: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ranges.emplace_back(pos, pos + term.size());
  Confidence: band=high; score=0.74
- Line 183: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ranges.emplace_back(pos, pos + term.size());
  Confidence: band=high; score=0.74
- Line 197: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged.push_back(ranges.front());
- Line 202: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(ranges[i]);
  Confidence: band=high; score=0.74
- Line 203: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged.push_back(ranges[i]);
- Line 220: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 252: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: lower_terms.push_back(std::move(lt));
  Confidence: band=high; score=0.74
- Line 252: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: lower_terms.push_back(std::move(lt));
  Confidence: band=high; score=0.74
- Line 253: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: lower_terms.push_back(std::move(lt));
- Line 286: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/search/cross_lingual_search.cpp
Total findings: 16

- Line 135: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto [st, knn_results] = vec_index_->searchKnn(embedding, config_.candidates);
- Line 178: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: lang = lang_it->second;
- Line 31: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("CrossLingualSearch: k must be > 0");
- Line 34: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("CrossLingualSearch: candidates must be > 0");
- Line 37: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("CrossLingualSearch: rrf_k must be > 0");
- Line 175: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto lang_it = lang_map_.find(doc_id);
- Line 175: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto lang_it = lang_map_.find(doc_id);
- Line 182: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto boost_it = boost_map.find(lang);
- Line 182: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto boost_it = boost_map.find(lang);
- Line 89: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ranked_lists.push_back(std::move(list));
  Confidence: band=high; score=0.74
- Line 90: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ranked_lists.push_back(std::move(list));
- Line 91: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: weights.push_back(q.weight > 0.0 ? q.weight : 1.0);
- Line 98: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> rrf_scores;
  Confidence: band=medium; score=0.66
- Line 111: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.emplace_back(doc_id, score);
  Confidence: band=high; score=0.74
- Line 145: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.emplace_back(r.pk, sim);
  Confidence: band=high; score=0.74
- Line 163: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> boost_map;
  Confidence: band=medium; score=0.66

### src/search/multi_modal_search.cpp
Total findings: 14

- Line 33: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("MultiModalSearch: k must be > 0");
- Line 36: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("MultiModalSearch: rrf_k must be > 0");
- Line 39: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("MultiModalSearch: candidates_per_modal must be > 0");
- Line 220: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = best_contribution.find(doc_id);
  Confidence: band=very_high; score=0.9
- Line 60: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: all_lists.push_back(std::move(results));
  Confidence: band=high; score=0.74
- Line 61: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: all_lists.push_back(std::move(results));
- Line 62: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: weights.push_back(q.weight > 0.0 ? q.weight : 1.0);
- Line 70: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!q.embedding_namespace.empty()) mod_label += ":" + q.embedding_namespace;
  Confidence: band=high; score=0.74
- Line 71: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!q.embedding_namespace.empty()) mod_label += ":" + q.embedding_namespace;
- Line 72: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: modality_names.push_back(std::move(mod_label));
- Line 150: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(r.pk, r.score);
  Confidence: band=high; score=0.74
- Line 181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(r.pk, sim);
  Confidence: band=high; score=0.74
- Line 235: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 236: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(r));

### src/search/faceted_search.cpp
Total findings: 13

- Line 156: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto [st, matching_pks] = index_->scanKeysEqual(table, facet.field, facet.value);
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #2562 [search] Add dynamic facet counting to FacetedSearch (2026-03-12T05:51:33Z)
- Line 52: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: table, column, scan_status.message);
- Line 99: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: facets.push_back(std::move(facet));
  Confidence: band=high; score=0.74
- Line 100: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: facets.push_back(std::move(facet));
- Line 122: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string> pk_filter(candidate_pks.begin(), candidate_pks.end());
  Confidence: band=medium; score=0.66
- Line 153: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> remaining(candidate_pks.begin(), candidate_pks.end());
  Confidence: band=medium; score=0.66
- Line 162: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string> matching_set(matching_pks.begin(), matching_pks.end());
  Confidence: band=medium; score=0.66
- Line 163: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> intersected;
  Confidence: band=medium; score=0.66
- Line 191: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: columns.push_back(stats.column);
  Confidence: band=high; score=0.74
- Line 192: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: columns.push_back(stats.column);
- Line 212: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: facets.push_back(std::move(facet));
  Confidence: band=high; score=0.74
- Line 213: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: facets.push_back(std::move(facet));

### src/search/federated_search.cpp
Total findings: 13

- Line 28: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("FederatedSearch: k must be > 0");
- Line 31: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("FederatedSearch: rrf_k must be > 0");
- Line 66: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("FederatedSearch: k must be > 0");
- Line 69: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("FederatedSearch: rrf_k must be > 0");
- Line 91: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (weight == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 83: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<HybridSearch::Result>>
  Confidence: band=medium; score=0.66
- Line 92: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (tenant_stats) tenant_stats->push_back(stats);
  Confidence: band=high; score=0.74
- Line 116: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 132: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string,
  Confidence: band=medium; score=0.66
- Line 146: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, Accumulator> accum; // key = "tenant_id\ndoc_id"
  Confidence: band=medium; score=0.66
- Line 146: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::unordered_map<std::string, Accumulator> accum; // key = "tenant_id\ndoc_id"
- Line 173: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(std::move(res));
  Confidence: band=high; score=0.74
- Line 174: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged.push_back(std::move(res));

### src/search/llm_query_rewriter.cpp
Total findings: 12

- Line 235: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto tokenise = [](const std::string& s) -> std::unordered_set<std::string> {
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #2559 [search] LLM-based query rewriting for improved recall + MINIMAL bu... (2026-03-12T05:51
- Line 29: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("LlmQueryRewriter: num_rewrites must be > 0");
- Line 32: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("LlmQueryRewriter: temperature must be in [0, 2]");
- Line 32: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: throw std::invalid_argument("LlmQueryRewriter: temperature must be in [0, 2]");
- Line 105: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rewrites.push_back(line);
  Confidence: band=high; score=0.74
- Line 214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rewrites.push_back(line);
  Confidence: band=high; score=0.74
- Line 215: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rewrites.push_back(line);
- Line 221: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rewrites.push_back(original);
- Line 273: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: kept.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 274: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: kept.push_back(std::move(r));

### src/search/neural_sparse_retrieval.cpp
Total findings: 12

- Line 99: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = inverted_index_.find(term);
- Line 123: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator fwd_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto fwd_it = forward_index_.find(doc_id);
- Line 161: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator fwd_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto fwd_it = forward_index_.find(doc_id);
- Line 29: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("NeuralSparseRetrieval: k must be > 0");
- Line 32: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 36: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 90: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: inverted_index_[term].emplace_back(doc_id, weight);
  Confidence: band=high; score=0.74
- Line 90: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: inverted_index_[term].emplace_back(doc_id, weight);
  Confidence: band=high; score=0.74
- Line 226: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, float> accum;
  Confidence: band=medium; score=0.66
- Line 248: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 249: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(r));
- Line 289: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/search/negative_keyword_filter.cpp
Total findings: 11

- Line 128: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto [status, neg_results] = index_->scanFulltext(
- Line 155: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (excluded.find(pk) == excluded.end()) {
- Line 38: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: NegativeKeywordFilter::parseQuery(const std::string& raw_query) {
  Confidence: band=high; score=0.74
- Line 66: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.negative_terms.push_back(std::move(neg));
  Confidence: band=high; score=0.74
- Line 67: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.negative_terms.push_back(std::move(neg));
- Line 79: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.negative_terms.push_back(std::move(neg));
- Line 84: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!positive_buf.empty()) positive_buf += ' ';
  Confidence: band=high; score=0.74
- Line 121: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> excluded;
  Confidence: band=medium; score=0.66
- Line 137: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: term, status.message);
- Line 156: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filtered.push_back(pk);
  Confidence: band=high; score=0.74
- Line 157: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: filtered.push_back(pk);

### src/search/search_analytics.cpp
Total findings: 10

- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #3370 [search] Add dedicated getTopQueries() to SearchAnalytics (2026-03-12T07:07:28Z)
- Line 26: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("SearchAnalytics: max_events must be > 0");
- Line 65: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = events_.rbegin(); it != events_.rend(); ++it) {
  Confidence: band=very_high; score=0.9
- Line 79: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = events_.rbegin(); it != events_.rend() && result.size() < n; ++it) {
  Confidence: band=very_high; score=0.9
- Line 125: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < std::min(size_t{20}, freq_vec.size()); ++i) {
  Confidence: band=very_high; score=0.9
- Line 125: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(size_t{20}, freq_vec.size()); ++i) {
- Line 67: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(*it);
- Line 80: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(*it);
- Line 96: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: latencies.push_back(ev.latency_ms);
  Confidence: band=high; score=0.74
- Line 97: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: latencies.push_back(ev.latency_ms);

### src/search/search_result_stream.cpp
Total findings: 9

- Line 43: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: void SearchResultStream::open(const std::string& query,
- Line 30: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 34: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 137: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 141: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 45: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 62: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 104: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void SearchResultStream::close() {
- Line 123: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/search/fuzzy_matcher.cpp
Total findings: 8

- Line 34: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("FuzzyMatcher: max_distance must be >= 0");
- Line 37: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("FuzzyMatcher: ngram_size must be > 0");
- Line 285: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (denom == 0.0) return 1.0;
  Confidence: band=very_high; score=0.9
- Line 163: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += static_cast<char>('0' + code);
  Confidence: band=high; score=0.74
- Line 163: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += static_cast<char>('0' + code);
  Confidence: band=high; score=0.74
- Line 163: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += static_cast<char>('0' + code);
  Confidence: band=high; score=0.74
- Line 198: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case 'B': if (prev != 'M') result += 'B'; break;
  Confidence: band=high; score=0.74
- Line 198: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case 'B': if (prev != 'M') result += 'B'; break;
  Confidence: band=high; score=0.74

### src/search/multi_field_search.cpp
Total findings: 7

- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #3141 [search] Multi-field boosting: title > body > tags (v1.9.0) (2026-03-12T06:23:45Z)
- Line 32: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("MultiFieldBoostedSearch: k must be > 0");
- Line 35: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 99: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, DocAccumulator> accum;
  Confidence: band=medium; score=0.66
- Line 128: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: field.column, status.message);
- Line 158: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 159: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(r));

### src/search/learning_to_rank.cpp
Total findings: 5

- Line 26: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("LearningToRank: learning_rate must be > 0");
- Line 29: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("LearningToRank: max_click_buffer must be > 0");
- Line 32: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("LearningToRank: regularization must be >= 0");
- Line 94: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: clicks_.push_back(event);
  Confidence: band=high; score=0.74
- Line 95: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: clicks_.push_back(event);

### src/search/conversational_search.cpp
Total findings: 4

- Line 30: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 110: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 54: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 67: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: history_.push_back(std::move(turn));

### src/search/personalized_ranker.cpp
Total findings: 4

- Line 26: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("PersonalizedRanker: decay_rate must be >= 0");
- Line 29: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 33: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("PersonalizedRanker: boost_weight must be >= 0");
- Line 150: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mu_);

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
