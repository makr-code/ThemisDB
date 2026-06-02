# search Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: search
- Generated: 2026-06-02 12:40:51
- Status: Critical Findings Present
- Total Findings: 174
- Actionable Findings (Critical + High): 68
- Affected Files: 19

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 13 |
| High | 55 |
| Medium | 106 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 73 |
| container | 29 |
| distributed_consistency | 20 |
| determinism | 18 |
| reliability | 14 |
| performance | 8 |
| concurrency | 5 |
| llm_ai_safety | 2 |
| observability | 2 |
| platform | 2 |
| raii | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/search/query_expander.cpp | 28 | 0 | 15 | 13 | 0 |
| src/search/distributed_hybrid_search.cpp | 26 | 6 | 14 | 6 | 0 |
| src/search/search_highlighter.cpp | 13 | 0 | 3 | 10 | 0 |
| src/search/llm_reranker.cpp | 12 | 1 | 6 | 5 | 0 |
| src/search/cross_lingual_search.cpp | 11 | 2 | 4 | 5 | 0 |
| src/search/autocomplete.cpp | 10 | 0 | 1 | 9 | 0 |
| src/search/faceted_search.cpp | 10 | 1 | 1 | 8 | 0 |
| src/search/hybrid_search.cpp | 10 | 0 | 2 | 8 | 0 |
| src/search/federated_search.cpp | 8 | 0 | 1 | 7 | 0 |
| src/search/negative_keyword_filter.cpp | 8 | 1 | 1 | 6 | 0 |
| src/search/llm_query_rewriter.cpp | 7 | 1 | 2 | 4 | 0 |
| src/search/multi_modal_search.cpp | 7 | 0 | 1 | 6 | 0 |
| src/search/fuzzy_matcher.cpp | 6 | 0 | 1 | 5 | 0 |
| src/search/neural_sparse_retrieval.cpp | 5 | 0 | 0 | 5 | 0 |
| src/search/multi_field_search.cpp | 4 | 0 | 1 | 3 | 0 |
| src/search/search_result_stream.cpp | 4 | 1 | 0 | 3 | 0 |
| src/search/search_analytics.cpp | 3 | 0 | 2 | 1 | 0 |
| src/search/conversational_search.cpp | 1 | 0 | 0 | 1 | 0 |
| src/search/learning_to_rank.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### src/search/query_expander.cpp
Total findings: 28

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3374 [search] Improve spelling c... (2026-03-12)
- Line 46: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (s != key && std::find(entry.begin(), entry.end(), s) == entry.end()) {
- Line 103: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = synonyms_.find(tok);
- Line 103: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = synonyms_.find(tok);
- Line 103: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = synonyms_.find(tok);
- Line 109: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (std::find(work_tokens.begin(), work_tokens.end(), syn) == work_tokens.end() &&
  Confidence: band=very_high; score=0.9
- Line 109: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(work_tokens.begin(), work_tokens.end(), syn) == work_tokens.end() &&
- Line 109: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(work_tokens.begin(), work_tokens.end(), syn) == work_tokens.end() &&
- Line 110: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: std::find(added_synonyms.begin(), added_synonyms.end(), syn) == added_synonyms.end()) {
  Confidence: band=very_high; score=0.9
- Line 110: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: std::find(added_synonyms.begin(), added_synonyms.end(), syn) == added_synonyms.end()) {
- Line 110: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: std::find(added_synonyms.begin(), added_synonyms.end(), syn) == added_synonyms.end()) {
- Line 198: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = word_frequencies_.find(sc.suggestion);
- Line 364: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (std::find(alternatives.begin(), alternatives.end(), alt) == alternatives.end()) {
  Confidence: band=very_high; score=0.9
- Line 364: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(alternatives.begin(), alternatives.end(), alt) == alternatives.end()) {
- Line 364: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(alternatives.begin(), alternatives.end(), alt) == alternatives.end()) {
- Line 46: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entry.push_back(s);
  Confidence: band=high; score=0.74
- Line 86: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: corrected_tokens.push_back(c);
  Confidence: band=high; score=0.74
- Line 110: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: added_synonyms.push_back(syn);
  Confidence: band=high; score=0.74
- Line 110: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: added_synonyms.push_back(syn);
  Confidence: band=high; score=0.74
- Line 110: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: added_synonyms.push_back(syn);
  Confidence: band=high; score=0.74
- Line 122: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.all_terms.push_back(syn);
  Confidence: band=high; score=0.74
- Line 187: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back(sc);
  Confidence: band=high; score=0.74
- Line 304: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(sc);
  Confidence: band=high; score=0.74
- Line 364: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: alternatives.push_back(alt);
  Confidence: band=high; score=0.74
- Line 364: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: alternatives.push_back(alt);
  Confidence: band=high; score=0.74
- Line 364: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: alternatives.push_back(alt);
  Confidence: band=high; score=0.74
- Line 373: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string QueryExpander::relaxQuery(const std::string& query) const {
  Confidence: band=high; score=0.74
- Line 403: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(clean);
  Confidence: band=high; score=0.74

### src/search/distributed_hybrid_search.cpp
Total findings: 26

- Line 48: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (config_.max_concurrent_shards == 0) {
  Confidence: band=very_high; score=0.99
- Line 50: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: "DistributedHybridSearch: Config::max_concurrent_shards must be > 0");
  Confidence: band=very_high; score=0.99
- Line 299: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged.push_back(std::move(r));
  Confidence: band=very_high; score=0.99
- Line 302: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::sort(merged.begin(), merged.end(),
  Confidence: band=very_high; score=0.99
- Line 307: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: if (merged.size() > config_.k) {
  Confidence: band=very_high; score=0.99
- Line 308: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged.resize(config_.k);
  Confidence: band=very_high; score=0.99
- Line 169: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: shard_results.push_back(futures[i].get());
  Confidence: band=very_high; score=0.9
- Line 233: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // --- Merge ---
  Confidence: band=very_high; score=0.9
- Line 234: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return mergeShardResults(shard_results);
  Confidence: band=very_high; score=0.9
- Line 238: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Cross-Shard RRF Merge
  Confidence: band=very_high; score=0.9
- Line 241: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<HybridSearch::Result> DistributedHybridSearch::mergeShardResults(
  Confidence: band=very_high; score=0.9
- Line 288: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<HybridSearch::Result> merged;
  Confidence: band=very_high; score=0.9
- Line 289: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.reserve(doc_map.size());
  Confidence: band=very_high; score=0.9
- Line 299: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.push_back(std::move(r));
  Confidence: band=very_high; score=0.9
- Line 302: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::sort(merged.begin(), merged.end(),
  Confidence: band=very_high; score=0.9
- Line 307: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: if (merged.size() > config_.k) {
  Confidence: band=very_high; score=0.9
- Line 308: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.resize(config_.k);
  Confidence: band=very_high; score=0.9
- Line 311: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: THEMIS_INFO("DistributedHybridSearch: merged {} shards -> {} results",
  Confidence: band=very_high; score=0.9
- Line 312: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: shard_results.size(), merged.size());
  Confidence: band=very_high; score=0.9
- Line 314: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return merged;
  Confidence: band=very_high; score=0.9
- Line 151: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(
  Confidence: band=high; score=0.74
- Line 168: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_results.push_back(futures[i].get());
  Confidence: band=high; score=0.74
- Line 168: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_results.push_back(futures[i].get());
  Confidence: band=high; score=0.74
- Line 169: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: shard_results.push_back(futures[i].get());
- Line 255: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, Accum> doc_map;
  Confidence: band=medium; score=0.66
- Line 298: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(std::move(r));
  Confidence: band=high; score=0.74

### src/search/search_highlighter.cpp
Total findings: 13

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3627 fix(build): register 40+ mi... (2026-03-12) | #3377 [WIP] Add highlight
- Line 108: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: while ((pos = lower_text.find(term, pos)) != std::string::npos) {
- Line 180: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: while ((pos = search_text.find(term, pos)) != std::string::npos) {
- Line 42: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(std::move(current));
  Confidence: band=high; score=0.74
- Line 109: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matches.push_back({pos, pos + term.size(), ti});
  Confidence: band=high; score=0.74
- Line 109: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matches.push_back({pos, pos + term.size(), ti});
  Confidence: band=high; score=0.74
- Line 181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ranges.emplace_back(pos, pos + term.size());
  Confidence: band=high; score=0.74
- Line 181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ranges.emplace_back(pos, pos + term.size());
  Confidence: band=high; score=0.74
- Line 200: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(ranges[i]);
  Confidence: band=high; score=0.74
- Line 218: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 250: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: lower_terms.push_back(std::move(lt));
  Confidence: band=high; score=0.74
- Line 250: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: lower_terms.push_back(std::move(lt));
  Confidence: band=high; score=0.74
- Line 284: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/search/llm_reranker.cpp
Total findings: 12

- Line 84: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Sort by final_score descending for consistent ordering regardless of input order
  Confidence: band=very_high; score=0.99
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #2648 [search] Configurable re-ra... (2026-03-12)
- Line 31: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: throw std::invalid_argument("LlmReranker: llm_weight must be in [0, 1]");
- Line 37: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: throw std::invalid_argument("LlmReranker: min_score_threshold must be in [0, 1]");
- Line 40: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: throw std::invalid_argument("LlmReranker: temperature must be in [0, 2]");
- Line 84: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Sort by final_score descending for consistent ordering regardless of input order
  Confidence: band=very_high; score=0.9
- Line 245: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: scores.push_back(val / 10.0); // normalise to [0, 1]
- Line 114: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 136: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(r);
  Confidence: band=high; score=0.74
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: events.push_back(ev);
  Confidence: band=high; score=0.74
- Line 245: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: scores.push_back(val / 10.0); // normalise to [0, 1]
- Line 246: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/search/cross_lingual_search.cpp
Total findings: 11

- Line 133: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto [st, knn_results] = vec_index_->searchKnn(embedding, config_.candidates);
- Line 176: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: lang = lang_it->second;
- Line 173: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto lang_it = lang_map_.find(doc_id);
- Line 173: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto lang_it = lang_map_.find(doc_id);
- Line 180: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto boost_it = boost_map.find(lang);
- Line 180: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto boost_it = boost_map.find(lang);
- Line 87: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ranked_lists.push_back(std::move(list));
  Confidence: band=high; score=0.74
- Line 96: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> rrf_scores;
  Confidence: band=medium; score=0.66
- Line 109: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.emplace_back(doc_id, score);
  Confidence: band=high; score=0.74
- Line 143: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.emplace_back(r.pk, sim);
  Confidence: band=high; score=0.74
- Line 161: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> boost_map;
  Confidence: band=medium; score=0.66

### src/search/autocomplete.cpp
Total findings: 10

- Line 172: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(limit, matches.size()); ++i) {
- Line 58: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (auto& s : prefix_sug) combined.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 64: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 72: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: deduped.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 72: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: deduped.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 112: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: true, false, // [prefix, prefix+'\xff')
- Line 133: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: suggestions.push_back(s);
  Confidence: band=high; score=0.74
- Line 137: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto val_opt = entities[0].getFieldAsString(column);
  Confidence: band=high; score=0.74
- Line 164: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matches.emplace_back(query, count);
  Confidence: band=high; score=0.74
- Line 176: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: suggestions.push_back(std::move(s));
  Confidence: band=high; score=0.74

### src/search/faceted_search.cpp
Total findings: 10

- Line 154: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto [st, matching_pks] = index_->scanKeysEqual(table, facet.field, facet.value);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #2562 [search] Add dynamic facet ... (2026-03-12) | #1335 Search module: v1.4
- Line 50: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: table, column, scan_status.message);
- Line 97: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: facets.push_back(std::move(facet));
  Confidence: band=high; score=0.74
- Line 120: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string> pk_filter(candidate_pks.begin(), candidate_pks.end());
  Confidence: band=medium; score=0.66
- Line 151: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> remaining(candidate_pks.begin(), candidate_pks.end());
  Confidence: band=medium; score=0.66
- Line 160: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string> matching_set(matching_pks.begin(), matching_pks.end());
  Confidence: band=medium; score=0.66
- Line 161: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> intersected;
  Confidence: band=medium; score=0.66
- Line 189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: columns.push_back(stats.column);
  Confidence: band=high; score=0.74
- Line 210: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: facets.push_back(std::move(facet));
  Confidence: band=high; score=0.74

### src/search/hybrid_search.cpp
Total findings: 10

- Line 269: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = result_map.find(rr.document_id);
- Line 269: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = result_map.find(rr.document_id);
- Line 131: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: bm25_results.push_back(r);
  Confidence: band=high; score=0.74
- Line 162: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vector_results.push_back(r);
  Confidence: band=high; score=0.74
- Line 213: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, Result> doc_map;
  Confidence: band=medium; score=0.66
- Line 265: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, const Result*> result_map;
  Confidence: band=medium; score=0.66
- Line 273: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: reranked_results.push_back(std::move(out));
  Confidence: band=high; score=0.74
- Line 273: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: reranked_results.push_back(std::move(out));
  Confidence: band=high; score=0.74
- Line 325: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fused_results.push_back(result);
  Confidence: band=high; score=0.74
- Line 325: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fused_results.push_back(result);
  Confidence: band=high; score=0.74

### src/search/federated_search.cpp
Total findings: 8

- Line 89: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (weight == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 81: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<HybridSearch::Result>>
  Confidence: band=medium; score=0.66
- Line 90: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (tenant_stats) tenant_stats->push_back(stats);
  Confidence: band=high; score=0.74
- Line 114: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 130: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string,
  Confidence: band=medium; score=0.66
- Line 144: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, Accumulator> accum; // key = "tenant_id\ndoc_id"
  Confidence: band=medium; score=0.66
- Line 144: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::unordered_map<std::string, Accumulator> accum; // key = "tenant_id\ndoc_id"
- Line 171: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(std::move(res));
  Confidence: band=high; score=0.74

### src/search/negative_keyword_filter.cpp
Total findings: 8

- Line 126: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto [status, neg_results] = index_->scanFulltext(
- Line 153: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (excluded.find(pk) == excluded.end()) {
- Line 36: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: NegativeKeywordFilter::parseQuery(const std::string& raw_query) {
  Confidence: band=high; score=0.74
- Line 64: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.negative_terms.push_back(std::move(neg));
  Confidence: band=high; score=0.74
- Line 82: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!positive_buf.empty()) positive_buf += ' ';
  Confidence: band=high; score=0.74
- Line 119: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> excluded;
  Confidence: band=medium; score=0.66
- Line 135: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: term, status.message);
- Line 154: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filtered.push_back(pk);
  Confidence: band=high; score=0.74

### src/search/llm_query_rewriter.cpp
Total findings: 7

- Line 233: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto tokenise = [](const std::string& s) -> std::unordered_set<std::string> {
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3590 feat(search): implement Sea... (2026-03-12) | #2582 [search] LLM query
- Line 30: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: throw std::invalid_argument("LlmQueryRewriter: temperature must be in [0, 2]");
- Line 103: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 212: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rewrites.push_back(line);
  Confidence: band=high; score=0.74
- Line 212: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rewrites.push_back(line);
  Confidence: band=high; score=0.74
- Line 271: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: kept.push_back(std::move(r));
  Confidence: band=high; score=0.74

### src/search/multi_modal_search.cpp
Total findings: 7

- Line 218: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = best_contribution.find(doc_id);
  Confidence: band=very_high; score=0.9
- Line 58: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: all_lists.push_back(std::move(results));
  Confidence: band=high; score=0.74
- Line 68: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!q.embedding_namespace.empty()) mod_label += ":" + q.embedding_namespace;
  Confidence: band=high; score=0.74
- Line 69: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!q.embedding_namespace.empty()) mod_label += ":" + q.embedding_namespace;
- Line 148: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(r.pk, r.score);
  Confidence: band=high; score=0.74
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(r.pk, sim);
  Confidence: band=high; score=0.74
- Line 233: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(r));
  Confidence: band=high; score=0.74

### src/search/fuzzy_matcher.cpp
Total findings: 6

- Line 283: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (denom == 0.0) return 1.0;
  Confidence: band=very_high; score=0.9
- Line 161: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += static_cast<char>('0' + code);
  Confidence: band=high; score=0.74
- Line 161: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += static_cast<char>('0' + code);
  Confidence: band=high; score=0.74
- Line 161: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += static_cast<char>('0' + code);
  Confidence: band=high; score=0.74
- Line 196: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case 'B': if (prev != 'M') result += 'B'; break;
  Confidence: band=high; score=0.74
- Line 196: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case 'B': if (prev != 'M') result += 'B'; break;
  Confidence: band=high; score=0.74

### src/search/neural_sparse_retrieval.cpp
Total findings: 5

- Line 88: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: inverted_index_[term].emplace_back(doc_id, weight);
  Confidence: band=high; score=0.74
- Line 88: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: inverted_index_[term].emplace_back(doc_id, weight);
  Confidence: band=high; score=0.74
- Line 224: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, float> accum;
  Confidence: band=medium; score=0.66
- Line 246: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 287: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/search/multi_field_search.cpp
Total findings: 4

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3372 feat(search): Cross-lingual... (2026-03-12) | #3371 feat(search): SPLAD
- Line 97: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, DocAccumulator> accum;
  Confidence: band=medium; score=0.66
- Line 126: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: field.column, status.message);
- Line 156: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(r));
  Confidence: band=high; score=0.74

### src/search/search_result_stream.cpp
Total findings: 4

- Line 41: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: void SearchResultStream::open(const std::string& query,
- Line 60: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 102: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void SearchResultStream::close() {
- Line 121: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/search/search_analytics.cpp
Total findings: 3

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3370 [search] Add dedicated getT... (2026-03-12)
- Line 123: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(size_t{20}, freq_vec.size()); ++i) {
- Line 94: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: latencies.push_back(ev.latency_ms);
  Confidence: band=high; score=0.74

### src/search/conversational_search.cpp
Total findings: 1

- Line 52: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/search/learning_to_rank.cpp
Total findings: 1

- Line 92: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: clicks_.push_back(event);
  Confidence: band=high; score=0.74

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
