# search Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: search
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 91
- Actionable Findings (Critical + High): 59
- Affected Files: 19

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 12 |
| High | 47 |
| Medium | 30 |
| Low | 2 |

## Category Summary

| Category | Count |
|---|---:|
| unordered_container_iter | 16 |
| undefined_conflict_resolution | 13 |
| uninitialized_access | 11 |
| o_n_squared | 8 |
| missing_version_tracking | 6 |
| data_race | 5 |
| string_concat_loop | 5 |
| nested_loop_find | 4 |
| repeated_search | 4 |
| command_injection | 2 |
| fp_exact_comparison | 2 |
| hardcoded_path | 2 |
| missing_latency_metric | 2 |
| module_doc_linkset_drift | 2 |
| range_temporary | 2 |
| generic_catch | 1 |
| manual_cleanup | 1 |
| no_timeout | 1 |
| stale_doc_section_reference | 1 |
| uncaught_exception | 1 |
| unnecessary_copy | 1 |
| unspecified_consistency | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| search/distributed_hybrid_search.cpp | 21 | 6 | 14 | 1 | 0 |
| search/query_expander.cpp | 11 | 0 | 10 | 1 | 0 |
| search/llm_reranker.cpp | 7 | 0 | 5 | 2 | 0 |
| search/cross_lingual_search.cpp | 6 | 2 | 2 | 2 | 0 |
| search/faceted_search.cpp | 6 | 1 | 1 | 4 | 0 |
| search/federated_search.cpp | 5 | 0 | 1 | 4 | 0 |
| search/negative_keyword_filter.cpp | 5 | 1 | 1 | 3 | 0 |
| search/autocomplete.cpp | 4 | 0 | 1 | 3 | 0 |
| search/llm_query_rewriter.cpp | 4 | 1 | 2 | 1 | 0 |
| search/fuzzy_matcher.cpp | 3 | 0 | 1 | 2 | 0 |
| search/hybrid_search.cpp | 3 | 0 | 1 | 2 | 0 |
| search/multi_field_search.cpp | 3 | 0 | 2 | 1 | 0 |
| search/multi_modal_search.cpp | 3 | 0 | 1 | 2 | 0 |
| search/search_highlighter.cpp | 3 | 0 | 3 | 0 | 0 |
| search/search_analytics.cpp | 2 | 0 | 2 | 0 | 0 |
| search/search_result_stream.cpp | 2 | 1 | 0 | 1 | 0 |
| search/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| search/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| search/neural_sparse_retrieval.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### search/distributed_hybrid_search.cpp
Total findings: 21

- Line 48: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (config_.max_concurrent_shards == 0) {
- Line 50: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: "DistributedHybridSearch: Config::max_concurrent_shards must be > 0");
- Line 299: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged.push_back(std::move(r));
- Line 302: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::sort(merged.begin(), merged.end(),
- Line 307: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (merged.size() > config_.k) {
- Line 308: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged.resize(config_.k);
- Line 169: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: shard_results.push_back(futures[i].get());
- Line 233: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // --- Merge ---
- Line 234: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return mergeShardResults(shard_results);
- Line 238: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Cross-Shard RRF Merge
- Line 241: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::vector<HybridSearch::Result> DistributedHybridSearch::mergeShardResults(
- Line 288: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::vector<HybridSearch::Result> merged;
- Line 289: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged.reserve(doc_map.size());
- Line 299: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged.push_back(std::move(r));
- Line 302: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::sort(merged.begin(), merged.end(),
- Line 307: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: if (merged.size() > config_.k) {
- Line 308: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: merged.resize(config_.k);
- Line 311: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: THEMIS_INFO("DistributedHybridSearch: merged {} shards -> {} results",
- Line 312: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: shard_results.size(), merged.size());
- Line 314: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return merged;
- Line 255: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, Accum> doc_map;

### search/query_expander.cpp
Total findings: 11

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3374 [search] Improve spelling c... (2026-03-12)
- Line 46: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (s != key && std::find(entry.begin(), entry.end(), s) == entry.end()) {
- Line 103: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = synonyms_.find(tok);
- Line 109: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (std::find(work_tokens.begin(), work_tokens.end(), syn) == work_tokens.end() &&
- Line 109: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(work_tokens.begin(), work_tokens.end(), syn) == work_tokens.end() &&
- Line 110: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::find(added_synonyms.begin(), added_synonyms.end(), syn) == added_synonyms.end()) {
- Line 110: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::find(added_synonyms.begin(), added_synonyms.end(), syn) == added_synonyms.end()) {
- Line 198: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = word_frequencies_.find(sc.suggestion);
- Line 364: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (std::find(alternatives.begin(), alternatives.end(), alt) == alternatives.end()) {
- Line 364: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(alternatives.begin(), alternatives.end(), alt) == alternatives.end()) {
- Line 373: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::string QueryExpander::relaxQuery(const std::string& query) const {

### search/llm_reranker.cpp
Total findings: 7

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #2648 [search] Configurable re-ra... (2026-03-12)
- Line 31: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: throw std::invalid_argument("LlmReranker: llm_weight must be in [0, 1]");
- Line 37: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: throw std::invalid_argument("LlmReranker: min_score_threshold must be in [0, 1]");
- Line 40: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: throw std::invalid_argument("LlmReranker: temperature must be in [0, 2]");
- Line 157: severity=HIGH; category=command_injection
  Description: sql_injection_concat: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: THEMIS_INFO("LlmReranker::rerank: {} candidates -> {} results (query='{}')",
- Line 246: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Clamp raw value to [0, 10] before normalising

            val = std::max(0.0, std::min(10.0, val));

            scores.push_back(val / 10.0); // normalise to [0, 1]

        } catch (...) {

            // Unparseable line → skip

        }

    }
- Line 246: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### search/cross_lingual_search.cpp
Total findings: 6

- Line 133: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto [st, knn_results] = vec_index_->searchKnn(embedding, config_.candidates);
- Line 176: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: lang = lang_it->second;
- Line 173: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto lang_it = lang_map_.find(doc_id);
- Line 180: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto boost_it = boost_map.find(lang);
- Line 96: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, double> rrf_scores;
- Line 161: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, double> boost_map;

### search/faceted_search.cpp
Total findings: 6

- Line 154: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto [st, matching_pks] = index_->scanKeysEqual(table, facet.field, facet.value);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #2562 [search] Add dynamic facet ... (2026-03-12) | #1335 Search module: v1.4
- Line 120: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: const std::unordered_set<std::string> pk_filter(candidate_pks.begin(), candidate_pks.end());
- Line 151: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> remaining(candidate_pks.begin(), candidate_pks.end());
- Line 160: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: const std::unordered_set<std::string> matching_set(matching_pks.begin(), matching_pks.end());
- Line 161: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> intersected;

### search/federated_search.cpp
Total findings: 5

- Line 89: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (weight == 0.0) {
- Line 81: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::vector<HybridSearch::Result>>
- Line 130: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: const std::unordered_map<std::string,
- Line 144: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::unordered_map<std::string, Accumulator> accum; // key = "tenant_id\ndoc_id"
- Line 144: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, Accumulator> accum; // key = "tenant_id\ndoc_id"

### search/negative_keyword_filter.cpp
Total findings: 5

- Line 126: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto [status, neg_results] = index_->scanFulltext(
- Line 153: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (excluded.find(pk) == excluded.end()) {
- Line 36: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: NegativeKeywordFilter::parseQuery(const std::string& raw_query) {
- Line 82: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (!positive_buf.empty()) positive_buf += ' ';
- Line 119: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> excluded;

### search/autocomplete.cpp
Total findings: 4

- Line 172: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (size_t i = 0; i < std::min(limit, matches.size()); ++i) {
- Line 64: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> seen;
- Line 112: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: true, false, // [prefix, prefix+'\xff')
- Line 137: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto val_opt = entities[0].getFieldAsString(column);

### search/llm_query_rewriter.cpp
Total findings: 4

- Line 233: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto tokenise = [](const std::string& s) -> std::unordered_set<std::string> {
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3590 feat(search): implement Sea... (2026-03-12) | #2582 [search] LLM query
- Line 30: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: throw std::invalid_argument("LlmQueryRewriter: temperature must be in [0, 2]");
- Line 73: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Gap 2):' that was not found in 'src/search/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: FUTURE_ENHANCEMENTS.md §Gap 2):

### search/fuzzy_matcher.cpp
Total findings: 3

- Line 283: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (denom == 0.0) return 1.0;
- Line 161: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: result += static_cast<char>('0' + code);
- Line 196: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: case 'B': if (prev != 'M') result += 'B'; break;

### search/hybrid_search.cpp
Total findings: 3

- Line 269: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = result_map.find(rr.document_id);
- Line 213: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, Result> doc_map;
- Line 265: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, const Result*> result_map;

### search/multi_field_search.cpp
Total findings: 3

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3372 feat(search): Cross-lingual... (2026-03-12) | #3371 feat(search): SPLAD
- Line 170: severity=HIGH; category=command_injection
  Description: sql_injection_concat: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: THEMIS_INFO("MultiFieldBoostedSearch: query='{}' fields={} -> {} results",
- Line 97: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, DocAccumulator> accum;

### search/multi_modal_search.cpp
Total findings: 3

- Line 218: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto it = best_contribution.find(doc_id);
- Line 68: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (!q.embedding_namespace.empty()) mod_label += ":" + q.embedding_namespace;
- Line 69: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!q.embedding_namespace.empty()) mod_label += ":" + q.embedding_namespace;

### search/search_highlighter.cpp
Total findings: 3

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3627 fix(build): register 40+ mi... (2026-03-12) | #3377 [WIP] Add highlight
- Line 108: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: while ((pos = lower_text.find(term, pos)) != std::string::npos) {
- Line 180: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: while ((pos = search_text.find(term, pos)) != std::string::npos) {

### search/search_analytics.cpp
Total findings: 2

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3370 [search] Add dedicated getT... (2026-03-12)
- Line 123: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (size_t i = 0; i < std::min(size_t{20}, freq_vec.size()); ++i) {

### search/search_result_stream.cpp
Total findings: 2

- Line 41: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: void SearchResultStream::open(const std::string& query,
- Line 102: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void SearchResultStream::close() {

### search/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### search/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### search/neural_sparse_retrieval.cpp
Total findings: 1

- Line 224: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, float> accum;

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
