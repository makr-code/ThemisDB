# graph Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: graph
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 519
- Actionable Findings (Critical + High): 197
- Affected Files: 13

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 48 |
| High | 149 |
| Medium | 322 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 154 |
| container | 151 |
| determinism | 34 |
| reliability | 31 |
| performance | 29 |
| exception_safety | 27 |
| memory | 18 |
| security | 17 |
| concurrency | 14 |
| distributed_consistency | 11 |
| platform | 7 |
| raii | 6 |
| legacy_duplication | 5 |
| observability | 5 |
| uninitialized | 4 |
| type_conversion | 3 |
| llm_ai_safety | 2 |
| audit_logging | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/graph/graph_query_optimizer.cpp | 115 | 10 | 20 | 81 | 4 |
| src/graph/tensor_deduplication_manager.cpp | 72 | 5 | 40 | 27 | 0 |
| src/graph/tensor_fingerprint_graph.cpp | 66 | 9 | 18 | 39 | 0 |
| src/graph/scheduled_edge_refresh.cpp | 48 | 0 | 26 | 22 | 0 |
| src/graph/parallel_traversal.cpp | 38 | 1 | 6 | 31 | 0 |
| src/graph/gpu_traversal.cpp | 37 | 1 | 7 | 29 | 0 |
| src/graph/distributed_graph.cpp | 35 | 3 | 12 | 20 | 0 |
| src/graph/knowledge_graph_reasoner.cpp | 35 | 7 | 8 | 20 | 0 |
| src/graph/graph_query_rewriter.cpp | 29 | 5 | 8 | 16 | 0 |
| src/graph/ontology_manager.cpp | 22 | 6 | 0 | 16 | 0 |
| src/graph/path_constraints.cpp | 9 | 1 | 3 | 5 | 0 |
| src/graph/graph_watermark.cpp | 8 | 0 | 1 | 7 | 0 |
| src/graph/explain_plan.cpp | 5 | 0 | 0 | 5 | 0 |

## Full Scanner Findings

### src/graph/graph_query_optimizer.cpp
Total findings: 115

- Line 1276: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator nit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto nit = dist.find(adj.targetPk);
- Line 1302: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator bit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto bit = buckets.find(old_idx);
- Line 1332: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = dist.find(adj.targetPk);
- Line 1340: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator bit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto bit = buckets.find(old_idx);
- Line 1359: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator dit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto dit = dist.find(target);
- Line 1366: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator pit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto pit = parent.find(cur);
- Line 1611: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = forward_parents.find(current);
- Line 1627: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = backward_parents.find(current);
- Line 2080: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = plan_cache_.find(key);
- Line 2754: severity=CRITICAL; category=missing_dtor
  Description: Class PendingCallback allocates resources but has no destructor
  Remediation: Add explicit destructor: ~PendingCallback() { /* cleanup */ }
  Context: class/struct PendingCallback
- Line 0: severity=HIGH; category=uncategorized
  Context: ['            };', '', '            const size_t chunk_size = (current_frontier.size() + effective_threads - 1) / effective_threads;', '            std::vector<std::future<ChunkResult>> futures;', '            std::atomic<bool> any_error{false};']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 862: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(constraints.forbidden_vertices.begin(),
- Line 862: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(constraints.forbidden_vertices.begin(),
- Line 926: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(constraints.forbidden_vertices.begin(),
- Line 926: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(constraints.forbidden_vertices.begin(),
- Line 1264: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto dit = dist.find(v);
- Line 1265: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto dit = dist.find(v);
  Confidence: band=very_high; score=0.9
- Line 1275: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto nit = dist.find(adj.targetPk);
- Line 1276: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto nit = dist.find(adj.targetPk);
  Confidence: band=very_high; score=0.9
- Line 1293: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = dist.find(r.vertex);
  Confidence: band=very_high; score=0.9
- Line 1302: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto bit = buckets.find(old_idx);
  Confidence: band=very_high; score=0.9
- Line 1332: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = dist.find(adj.targetPk);
  Confidence: band=very_high; score=0.9
- Line 1340: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto bit = buckets.find(old_idx);
  Confidence: band=very_high; score=0.9
- Line 1839: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(constraints.forbidden_vertices.begin(),
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 72: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) hint += ", ";
  Confidence: band=high; score=0.74
- Line 72: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) hint += ", ";
  Confidence: band=high; score=0.74
- Line 73: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) hint += ", ";
- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan.active_schema_hints.push_back(std::move(hint));
  Confidence: band=high; score=0.74
- Line 76: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan.active_schema_hints.push_back(std::move(hint));
- Line 80: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) hint += ", ";
  Confidence: band=high; score=0.74
- Line 81: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) hint += ", ";
- Line 83: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan.active_schema_hints.push_back(std::move(hint));
  Confidence: band=high; score=0.74
- Line 84: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan.active_schema_hints.push_back(std::move(hint));
- Line 142: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: alternatives.push_back({TraversalAlgorithm::BFS, bfs_cost});
- Line 151: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: alternatives.push_back({TraversalAlgorithm::BIDIRECTIONAL, bidirectional_cost});
- Line 382: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: alternatives.push_back({TraversalAlgorithm::BFS, bfs_cost});
- Line 682: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(node);
  Confidence: band=high; score=0.74
- Line 682: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(node);
  Confidence: band=high; score=0.74
- Line 683: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(node);
- Line 717: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next_frontier.push_back(nb);
  Confidence: band=high; score=0.74
- Line 718: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: next_frontier.push_back(nb);
- Line 816: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: current_frontier.push_back(std::string(start_vertex));
- Line 836: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(node);
  Confidence: band=high; score=0.74
- Line 836: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(node);
  Confidence: band=high; score=0.74
- Line 837: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(node);
- Line 867: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next_frontier.push_back(nb);
  Confidence: band=high; score=0.74
- Line 867: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next_frontier.push_back(nb);
  Confidence: band=high; score=0.74
- Line 868: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: next_frontier.push_back(nb);
- Line 890: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(std::launch::async, [&, begin_idx, end_idx]() {
  Confidence: band=high; score=0.74
- Line 891: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(std::async(std::launch::async, [&, begin_idx, end_idx]() {
- Line 907: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cr.neighbors.push_back(nb);
  Confidence: band=high; score=0.74
- Line 907: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cr.neighbors.push_back(nb);
  Confidence: band=high; score=0.74
- Line 908: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cr.neighbors.push_back(nb);
- Line 929: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next_frontier.push_back(nb);
  Confidence: band=high; score=0.74
- Line 929: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next_frontier.push_back(nb);
  Confidence: band=high; score=0.74
- Line 930: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: next_frontier.push_back(nb);
- Line 1061: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stack.push_back({neighbor, depth + 1});
  Confidence: band=high; score=0.74
- Line 1062: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stack.push_back({neighbor, depth + 1});
- Line 1189: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: sum += graph_manager_.getEdgeWeight("", adj.edgeId, "_weight");
  Confidence: band=high; score=0.74
- Line 1258: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(std::launch::async,
  Confidence: band=high; score=0.74
- Line 1259: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(std::async(std::launch::async,
- Line 1277: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.relaxations.push_back({adj.targetPk, nd, v});
  Confidence: band=high; score=0.74
- Line 1277: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.relaxations.push_back({adj.targetPk, nd, v});
  Confidence: band=high; score=0.74
- Line 1278: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.relaxations.push_back({adj.targetPk, nd, v});
- Line 1365: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: path.push_back(cur);
- Line 1371: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: path.push_back(start);
- Line 1610: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: forward_path.push_back(current);
- Line 1617: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: forward_path.push_back(std::string(start_vertex));
- Line 1626: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: backward_path.push_back(current);
- Line 1636: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: backward_path.push_back(std::string(target_vertex));
- Line 1723: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.matches.push_back({});
- Line 1741: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::unordered_set<std::string>> pattern_adj;
  Confidence: band=medium; score=0.66
- Line 1756: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::unordered_set<std::string>> data_adj_cache;
  Confidence: band=medium; score=0.66
- Line 1767: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> mapping;
  Confidence: band=medium; score=0.66
- Line 1768: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> used_data_vertices;
  Confidence: band=medium; score=0.66
- Line 1916: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> unique_edge_ids;
  Confidence: band=medium; score=0.66
- Line 1963: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, size_t>& label_counts) {
  Confidence: band=medium; score=0.66
- Line 1994: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: explanation += "Selected Algorithm: " + algo_name + "\n";
  Confidence: band=high; score=0.74
- Line 2010: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) explanation += ", ";
  Confidence: band=high; score=0.74
- Line 2011: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) explanation += ", ";
- Line 2028: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: explanation += "  " + alt_name + ": " + std::to_string(alt_cost) + "\n";
  Confidence: band=high; score=0.74
- Line 2029: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: explanation += "  " + alt_name + ": " + std::to_string(alt_cost) + "\n";
- Line 2035: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: explanation += "  " + hint + "\n";
  Confidence: band=high; score=0.74
- Line 2036: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: explanation += "  " + hint + "\n";
- Line 2250: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back(TraversalAlgorithm::BIDIRECTIONAL);
  Confidence: band=high; score=0.74
- Line 2251: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: candidates.push_back(TraversalAlgorithm::BIDIRECTIONAL);
- Line 2256: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: candidates.push_back(TraversalAlgorithm::BIDIRECTIONAL);
- Line 2390: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += lbl + "|";
  Confidence: band=high; score=0.74
- Line 2399: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += et + "|";
  Confidence: band=high; score=0.74
- Line 2466: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += lbl + "|";
  Confidence: band=high; score=0.74
- Line 2475: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += et + "|";
  Confidence: band=high; score=0.74
- Line 2601: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2625: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<TraversalAlgorithm,
  Confidence: band=medium; score=0.66
- Line 2630: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: acc.actual_times.push_back(s.execution_time_ms);
  Confidence: band=high; score=0.74
- Line 2631: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: acc.actual_times.push_back(s.execution_time_ms);
- Line 2728: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void GraphQueryOptimizer::unregisterIncrementalQuery(IncrementalQueryHandle handle) {
  Confidence: band=high; score=0.74
- Line 2738: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> changed_vertices;
  Confidence: band=medium; score=0.66
- Line 2787: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string> new_result(result.value().begin(),
  Confidence: band=medium; score=0.66
- Line 2793: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: delta.added.push_back(v);
  Confidence: band=high; score=0.74
- Line 2794: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: delta.added.push_back(v);
- Line 2799: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: delta.removed.push_back(v);
  Confidence: band=high; score=0.74
- Line 2800: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: delta.removed.push_back(v);
- Line 2812: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pending.push_back({entry.callback, std::move(delta)});
- Line 1938: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::log(static_cast<double>(stats.vertex_count)) /
  Confidence: band=medium; score=0.6
- Line 1939: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::log(stats.avg_branching_factor)
  Confidence: band=medium; score=0.6
- Line 2125: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::log(statistics_.vertex_count + 1.0);
  Confidence: band=medium; score=0.6
- Line 2131: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::log(statistics_.vertex_count + 1.0) * 0.7;
  Confidence: band=medium; score=0.6

### src/graph/tensor_deduplication_manager.cpp
Total findings: 72

- Line 145: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator record_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: const auto record_it = records_.find(tensor_id);
- Line 185: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator key_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto key_it = tensor_id_to_key_.find(tensor_id);
- Line 1064: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto existing_namespaced = storage->getRawMetadata(namespaced_key);
- Line 1625: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: = [&graph_idx, default_snapshot_key](std::string_view snap, std::string_view tensor_id) -> bool {
- Line 1660: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: = [&graph_idx, default_snapshot_key](std::string_view snap) -> bool {
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    bytes_saved        = static_cast<std::size_t>(bytes_saved_u64);', '    if (pos != size) {', '        const auto trailing_bytes = size - pos;', '        THEMIS_WARN("[TensorDeduplicationManager] restore snapshot: trailing bytes detected ({})", trailing_bytes);', '        return false;']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 41: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("TensorDeduplicationManager: null dependency");
- Line 70: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (const auto &tensor_id_suffix : storage->listRawMetadataKeys(prefix)) {
- Line 73: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: const auto payload = storage->getRawMetadata(prefix + tensor_id_suffix);
- Line 73: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const auto payload = storage->getRawMetadata(prefix + tensor_id_suffix);
- Line 86: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (const auto &key : storage->listRawMetadataKeys(prefix)) {
- Line 86: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (const auto &key : storage->listRawMetadataKeys(prefix)) {
- Line 87: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: const auto deleted = storage->deleteRawMetadata(prefix + key);
- Line 87: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const auto deleted = storage->deleteRawMetadata(prefix + key);
- Line 136: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: return counter.fetch_sub(value, std::memory_order_relaxed) - value;
- Line 164: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: storage_->setWriteObserverFn(nullptr);
- Line 165: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: storage_->setDeleteObserverFn(nullptr);
- Line 248: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto [new_train, stats] = decomposer_->decompose(data, mode_sizes, cfg);
- Line 268: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto ref_dense_opt = storage_->get(makeKey(ref_tenant, ref_collection, ref_field));
- Line 274: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto [ref_train, _] = decomposer_->decompose(*ref_dense_opt, ref_ms, ref_cfg);
- Line 299: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: storage_->put(key, data, mode_sizes);
- Line 373: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ref_rec = ref_it->second;
- Line 377: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto ref_opt = storage_->get(makeKey(ref_rec.tenant, ref_rec.collection, ref_rec.field));
- Line 389: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (ref_opt->size() != delta_opt->size()) {
- Line 394: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::vector<float> result(ref_opt->size());
- Line 1003: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: //   2) legacy key "<snapshot>::wal"
  Confidence: band=high; score=0.8
- Line 1017: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const auto payload = storage->getRawMetadata(key);
- Line 1064: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const auto existing_namespaced = storage->getRawMetadata(namespaced_key);
- Line 1081: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Skip no-op empty rewrites: legacy key may already exist with an empty
  Confidence: band=high; score=0.8
- Line 1233: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto bytes_opt = storage_->getRawMetadata(snapshot_key);
- Line 1273: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: wlk.unlock(); // Release before replayMutationJournal, which re-acquires per entry.
- Line 1352: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &entry : entries) {
  Confidence: band=very_high; score=0.9
- Line 1393: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: storage_->putRawMetadata(kActiveSnapshotMetaKey, std::vector<uint8_t>{snapshot_key.begin(), snapshot
- Line 1404: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Also clear the legacy blob keys so they don't confuse future restores.
  Confidence: band=high; score=0.8
- Line 1405: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: storage_->putRawMetadata(mutationJournalKeyForSnapshot(snapshot_key), {});
- Line 1406: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: storage_->putRawMetadata(legacyMutationJournalKeyForSnapshot(snapshot_key), {});
- Line 1409: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: storage_->putRawMetadata(mutationJournalKeyForSnapshot(snapshot_key), {});
- Line 1410: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: storage_->putRawMetadata(legacyMutationJournalKeyForSnapshot(snapshot_key), {});
- Line 1541: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Must remain colon-free because GraphIndex legacy out-key parsing splits on
  Confidence: band=high; score=0.8
- Line 461: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: constexpr uint64_t kGraphSnapshotMagic          = 0x504E535F47465400ULL; // "TFG_SNP\0"
- Line 463: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: constexpr uint64_t kDedupSnapshotMagic          = 0x504E535F4D445400ULL; // "TDM_SNP\0"
- Line 465: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: constexpr uint64_t kMutationJournalMagic        = 0x4A4E4C5F4D445400ULL; // "TDM_JNL\0"
- Line 471: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: enum class JournalLoadStatus { Missing, Loaded, InvalidReset };
- Line 492: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.push_back(p[i]);
  Confidence: band=high; score=0.74
- Line 493: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(p[i]);
- Line 499: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.push_back(c);
  Confidence: band=high; score=0.74
- Line 500: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(c);
- Line 776: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: snapshot.edges.push_back(std::move(e));
  Confidence: band=high; score=0.74
- Line 777: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: snapshot.edges.push_back(std::move(e));
- Line 929: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entry.edges.push_back(std::move(edge));
  Confidence: band=high; score=0.74
- Line 930: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entry.edges.push_back(std::move(edge));
- Line 979: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: compacted.push_back(std::move(entry));
- Line 1175: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: records.push_back(std::move(record));
  Confidence: band=high; score=0.74
- Line 1176: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: records.push_back(std::move(record));
- Line 1215: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: records.push_back(record);
  Confidence: band=high; score=0.74
- Line 1216: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: records.push_back(record);
- Line 1283: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1303: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entries.push_back(std::move(one[0]));
- Line 1306: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 1402: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) {}
- Line 1449: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) {
- Line 1458: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entries.push_back(std::move(entry));
- Line 1482: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) {}
- Line 1489: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entries.push_back(std::move(entry));
- Line 1589: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>((hi << 4U) | lo));
  Confidence: band=high; score=0.74
- Line 1590: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint8_t>((hi << 4U) | lo));

### src/graph/tensor_fingerprint_graph.cpp
Total findings: 66

- Line 193: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it                   = lsh_buckets_.find(bucket_key);
- Line 222: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = lsh_buckets_.find(bucket_key);
- Line 257: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator ait may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto ait               = adj_.find(tensor_id);
- Line 376: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator nit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto nit = nodes_.find(tensor_id);
- Line 382: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator ait may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto ait = adj_.find(tensor_id);
- Line 468: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator nit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto nit     = nodes_.find(e.to);
- Line 629: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: const auto it = adj_.find(tensor_id);
- Line 687: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator node_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: const auto node_it = nodes_.find(tensor_id);
- Line 692: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator adj_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto adj_it = adj_.find(tensor_id);
- Line 34: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("TensorFingerprintGraph: num_hash_funcs and num_bands must be > 0");
- Line 37: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("TensorFingerprintGraph: num_hash_funcs must be divisible by num_bands")
- Line 138: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: const uint64_t a = fnv1a64(&h, sizeof(h)) | 1ULL;
- Line 140: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: b_params[h]      = fnv1a64(&a, sizeof(a));
- Line 166: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: uint64_t h = fnv1a64(&band_idx, sizeof(band_idx));
- Line 384: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &e : ait->second) {
  Confidence: band=very_high; score=0.9
- Line 426: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto nit = nodes_.find(cid);
- Line 464: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &e : ait->second) {
  Confidence: band=very_high; score=0.9
- Line 486: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &[tensor_id, node] : nodes_) {
  Confidence: band=very_high; score=0.9
- Line 506: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &persisted : nodes) {
  Confidence: band=very_high; score=0.9
- Line 522: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &[from, edges] : adj_) {
  Confidence: band=very_high; score=0.9
- Line 523: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &edge : edges) {
  Confidence: band=very_high; score=0.9
- Line 548: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &edge : edges) {
  Confidence: band=very_high; score=0.9
- Line 551: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (nodes_.find(edge.from) == nodes_.end() || nodes_.find(edge.to) == nodes_.end()) {
- Line 582: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &[from, edges] : adj_) {
  Confidence: band=very_high; score=0.9
- Line 583: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &edge : edges) {
  Confidence: band=very_high; score=0.9
- Line 634: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &edge : it->second) {
  Confidence: band=very_high; score=0.9
- Line 664: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &edge : snapshot.edges) {
  Confidence: band=very_high; score=0.9
- Line 112: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: elements.push_back(encoded);
  Confidence: band=high; score=0.74
- Line 112: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: elements.push_back(encoded);
  Confidence: band=high; score=0.74
- Line 113: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: elements.push_back(encoded);
- Line 122: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: elements.push_back((static_cast<uint64_t>(0xFF) << 24) | (static_cast<uint64_t>(k) << 16)
  Confidence: band=high; score=0.74
- Line 123: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: elements.push_back((static_cast<uint64_t>(0xFF) << 24) | (static_cast<uint64_t>(k) << 16)
- Line 205: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> TensorFingerprintGraph::lshCandidates(const TensorFingerprint &fp) const {
  Confidence: band=medium; score=0.66
- Line 206: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> candidates;
  Confidence: band=medium; score=0.66
- Line 256: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto previous_fp = nodes_[tensor_id].fingerprint;
  Confidence: band=high; score=0.74
- Line 304: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adj_[tensor_id].push_back({cid, similarity});
  Confidence: band=high; score=0.74
- Line 305: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: adj_[tensor_id].push_back({cid, similarity});
- Line 306: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: adj_[cid].push_back({tensor_id, similarity});
- Line 324: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { /* hook must not throw; swallow */ }
- Line 404: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { /* hook must not throw; swallow */ }
- Line 443: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(r);
  Confidence: band=high; score=0.74
- Line 444: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(r);
- Line 473: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(r);
  Confidence: band=high; score=0.74
- Line 474: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(r);
- Line 492: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(std::move(persisted));
  Confidence: band=high; score=0.74
- Line 493: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(std::move(persisted));
- Line 527: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(std::move(persisted));
  Confidence: band=high; score=0.74
- Line 527: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(std::move(persisted));
  Confidence: band=high; score=0.74
- Line 528: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(std::move(persisted));
- Line 543: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 560: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adj_[edge.from].push_back({edge.to, edge.similarity});
  Confidence: band=high; score=0.74
- Line 561: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: adj_[edge.from].push_back({edge.to, edge.similarity});
- Line 578: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: snapshot.nodes.push_back(std::move(persisted));
  Confidence: band=high; score=0.74
- Line 579: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: snapshot.nodes.push_back(std::move(persisted));
- Line 587: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: snapshot.edges.push_back(std::move(persisted));
  Confidence: band=high; score=0.74
- Line 587: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: snapshot.edges.push_back(std::move(persisted));
  Confidence: band=high; score=0.74
- Line 588: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: snapshot.edges.push_back(std::move(persisted));
- Line 634: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back({tensor_id, edge.to, edge.similarity});
  Confidence: band=high; score=0.74
- Line 635: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back({tensor_id, edge.to, edge.similarity});
- Line 659: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 676: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adj_[edge.from].push_back({edge.to, edge.similarity});
  Confidence: band=high; score=0.74
- Line 677: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: adj_[edge.from].push_back({edge.to, edge.similarity});
- Line 720: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen_targets;
  Confidence: band=medium; score=0.66
- Line 732: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adj_[node.tensor_id].push_back({edge.to, edge.similarity});
  Confidence: band=high; score=0.74
- Line 733: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: adj_[node.tensor_id].push_back({edge.to, edge.similarity});
- Line 734: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: adj_[edge.to].push_back({node.tensor_id, edge.similarity});

### src/graph/scheduled_edge_refresh.cpp
Total findings: 48

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 155: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(policy_mutex_);
- Line 160: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(stats_mutex_);
- Line 198: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return clampf((cos_sim + 1.0f) / 2.0f, 0.0f, 1.0f); // map [-1,1] → [0,1]
- Line 355: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("RefreshPolicy: relevance_threshold must be >= 0");
- Line 358: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("RefreshPolicy: relevance_threshold must be <= 1");
- Line 362: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("RefreshPolicy: add_threshold must be <= 1");
- Line 365: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("RefreshPolicy: add_threshold must be >= 0");
- Line 368: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("RefreshPolicy: max_removal_fraction must be >= 0");
- Line 371: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("RefreshPolicy: max_removal_fraction must be <= 1");
- Line 374: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("RefreshPolicy: decay_half_life_seconds must be >= 0");
- Line 377: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("RefreshPolicy: top_k_candidates must be > 0");
- Line 380: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("RefreshPolicy: anomaly_threshold_removal_rate must be >= 0");
- Line 383: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("RefreshPolicy: anomaly_threshold_removal_rate must be <= 1");
- Line 390: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (!stop_requested_.load(std::memory_order_acquire)) {
- Line 393: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(policy_mutex_);
- Line 400: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: cv_.wait_for(lk, interval, [this] { return stop_requested_.load(std::memory_order_acquire); });
- Line 400: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: cv_.wait_for(lk, interval, [this] { return stop_requested_.load(std::memory_order_acquire); });
  Confidence: band=very_high; score=0.9
- Line 403: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (stop_requested_.load(std::memory_order_acquire)) {
- Line 446: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &s : scores) {
  Confidence: band=very_high; score=0.9
- Line 634: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &edge : edges) {
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 338: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: flat_ids.push_back(idx);
  Confidence: band=high; score=0.74
- Line 339: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: flat_ids.push_back(idx);
- Line 342: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ann_idx_to_vertex_.push_back(v);
- Line 460: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_remove.push_back(s.edge_id);
  Confidence: band=high; score=0.74
- Line 460: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_remove.push_back(s.edge_id);
  Confidence: band=high; score=0.74
- Line 461: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: to_remove.push_back(s.edge_id);
- Line 511: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> removed_set(to_remove.begin(), to_remove.end());
  Confidence: band=medium; score=0.66
- Line 572: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 578: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back(std::move(e));
  Confidence: band=high; score=0.74
- Line 579: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edges.push_back(std::move(e));
- Line 590: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen_edges;
  Confidence: band=medium; score=0.66
- Line 615: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 634: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scores.push_back(scoreEdge(edge));
  Confidence: band=high; score=0.74
- Line 635: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: scores.push_back(scoreEdge(edge));
- Line 654: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> existing_pairs;
  Confidence: band=medium; score=0.66
- Line 667: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> dedup;
  Confidence: band=medium; score=0.66
- Line 731: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.emplace_back(sim, other);
  Confidence: band=high; score=0.74
- Line 747: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.emplace_back(vertex, other, sim);
  Confidence: band=high; score=0.74
- Line 778: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.emplace_back(sim, other);
  Confidence: band=high; score=0.74
- Line 794: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.emplace_back(vertex, other, sim);
  Confidence: band=high; score=0.74
- Line 860: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: added_records.push_back({new_id, from, to, sim});
  Confidence: band=high; score=0.74

### src/graph/parallel_traversal.cpp
Total findings: 38

- Line 36: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: size_t requested = (config.num_threads > 0) ? static_cast<size_t>(config.num_threads) : []() -> size
- Line 0: severity=HIGH; category=uncategorized
  Context: ['            // all tasks complete (no data races).', '            const size_t nthreads   = effectiveThreadCount(config, current_frontier.size());', '            const size_t chunk_size = (current_frontier.size() + nthreads - 1) / nthreads;', '', '            struct ChunkResult {']
  Confidence: band=high; score=0.78
- Line 147: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(forbidden.begin(), forbidden.end(), nb) != forbidden.end()) {
- Line 147: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(forbidden.begin(), forbidden.end(), nb) != forbidden.end()) {
- Line 166: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(forbidden.begin(), forbidden.end(), nb) != forbidden.end()) {
- Line 166: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(forbidden.begin(), forbidden.end(), nb) != forbidden.end()) {
- Line 243: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(forbidden.begin(), forbidden.end(), nb) != forbidden.end()) {
- Line 67: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> visited;
  Confidence: band=medium; score=0.66
- Line 85: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.visited.push_back(node);
  Confidence: band=high; score=0.74
- Line 85: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.visited.push_back(node);
  Confidence: band=high; score=0.74
- Line 86: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.visited.push_back(node);
- Line 125: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(std::launch::async, [this, &current_frontier, begin, end]() {
  Confidence: band=high; score=0.74
- Line 126: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(std::async(std::launch::async, [this, &current_frontier, begin, end]() {
- Line 133: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cr.candidates.push_back(nb);
  Confidence: band=high; score=0.74
- Line 133: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cr.candidates.push_back(nb);
  Confidence: band=high; score=0.74
- Line 134: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cr.candidates.push_back(nb);
- Line 150: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next_frontier.push_back(nb);
  Confidence: band=high; score=0.74
- Line 150: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next_frontier.push_back(nb);
  Confidence: band=high; score=0.74
- Line 151: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: next_frontier.push_back(nb);
- Line 169: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next_frontier.push_back(nb);
  Confidence: band=high; score=0.74
- Line 169: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next_frontier.push_back(nb);
  Confidence: band=high; score=0.74
- Line 170: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: next_frontier.push_back(nb);
- Line 208: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stack.push_back({source, 0});
- Line 224: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.visited.push_back(current);
- Line 245: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stack.push_back({nb, depth + 1});
  Confidence: band=high; score=0.74
- Line 246: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stack.push_back({nb, depth + 1});
- Line 263: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 272: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.visited_vertices.push_back(v);
  Confidence: band=high; score=0.74
- Line 272: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.visited_vertices.push_back(v);
  Confidence: band=high; score=0.74
- Line 273: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged.visited_vertices.push_back(v);
- Line 312: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(
  Confidence: band=high; score=0.74
- Line 313: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(std::async(
- Line 317: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: per_source.push_back(fut.get());
  Confidence: band=high; score=0.74
- Line 318: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: per_source.push_back(fut.get());
- Line 359: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(
  Confidence: band=high; score=0.74
- Line 360: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(std::async(
- Line 364: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: per_source.push_back(fut.get());
  Confidence: band=high; score=0.74
- Line 365: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: per_source.push_back(fut.get());

### src/graph/gpu_traversal.cpp
Total findings: 37

- Line 162: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = vertex_to_id_.find(nb);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 161: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = vertex_to_id_.find(nb);
- Line 168: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Extend adj list to accommodate new vertex.
- Line 168: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Extend adj list to accommodate new vertex.
- Line 129: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "GPUGraphTraversal::load: failed to enumerate vertices: " + status.message);
- Line 145: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: id_to_vertex_.push_back(v);
  Confidence: band=high; score=0.74
- Line 146: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: id_to_vertex_.push_back(v);
- Line 166: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: id_to_vertex_.push_back(nb);
  Confidence: band=high; score=0.74
- Line 166: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: id_to_vertex_.push_back(nb);
  Confidence: band=high; score=0.74
- Line 167: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: id_to_vertex_.push_back(nb);
- Line 170: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: adj[i].push_back(new_id);
- Line 172: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: adj[i].push_back(it->second);
- Line 187: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: row_offsets_.push_back(static_cast<uint32_t>(column_indices_.size()));
  Confidence: band=high; score=0.74
- Line 188: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: row_offsets_.push_back(static_cast<uint32_t>(column_indices_.size()));
- Line 191: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: column_indices_.push_back(nb);
  Confidence: band=high; score=0.74
- Line 192: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: column_indices_.push_back(nb);
- Line 196: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: row_offsets_.push_back(static_cast<uint32_t>(column_indices_.size()));
- Line 235: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<uint32_t> forbidden_ids;
  Confidence: band=medium; score=0.66
- Line 265: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ordered.emplace_back(gpu_dist[i], i);
  Confidence: band=high; score=0.74
- Line 278: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.visited_vertices.push_back(id_to_vertex_[vid]);
  Confidence: band=high; score=0.74
- Line 279: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.visited_vertices.push_back(id_to_vertex_[vid]);
- Line 305: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.visited_vertices.push_back(id_to_vertex_[v]);
  Confidence: band=high; score=0.74
- Line 305: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.visited_vertices.push_back(id_to_vertex_[v]);
  Confidence: band=high; score=0.74
- Line 306: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.visited_vertices.push_back(id_to_vertex_[v]);
- Line 336: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next_frontier.push_back(nb);
  Confidence: band=high; score=0.74
- Line 336: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next_frontier.push_back(nb);
  Confidence: band=high; score=0.74
- Line 337: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: next_frontier.push_back(nb);
- Line 356: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<uint32_t> forbidden_ids;
  Confidence: band=medium; score=0.66
- Line 385: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ordered.emplace_back(gpu_order[i], i);
  Confidence: band=high; score=0.74
- Line 393: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.visited_vertices.push_back(id_to_vertex_[vid]);
  Confidence: band=high; score=0.74
- Line 394: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.visited_vertices.push_back(id_to_vertex_[vid]);
- Line 422: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.visited_vertices.push_back(id_to_vertex_[cur]);
- Line 442: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stack.emplace_back(nb, depth + 1);
  Confidence: band=high; score=0.74

### src/graph/distributed_graph.cpp
Total findings: 35

- Line 294: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::vector<std::string> merged;
  Confidence: band=very_high; score=0.99
- Line 304: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged.push_back(v);
  Confidence: band=very_high; score=0.99
- Line 309: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return Ok(std::move(merged));
  Confidence: band=very_high; score=0.99
- Line 72: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &v : res->path) {
  Confidence: band=very_high; score=0.9
- Line 234: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: std::async(std::launch::async, [exec_ptr = exec.get(), &start_local, &target_local, &constraints]() {
  Confidence: band=very_high; score=0.9
- Line 235: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return exec_ptr->executeDijkstra(start_local, target_local, constraints);
- Line 235: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return exec_ptr->executeDijkstra(start_local, target_local, constraints);
- Line 245: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto res = f.get();
  Confidence: band=very_high; score=0.9
- Line 287: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: futures.push_back(std::async(std::launch::async, [exec_ptr = exec.get(), start_local, k, &constraints]() {
  Confidence: band=very_high; score=0.9
- Line 288: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return exec_ptr->executeBFS(start_local, k, constraints);
- Line 292: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Merge results: de-duplicate qualified vertex IDs.
  Confidence: band=very_high; score=0.9
- Line 294: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<std::string> merged;
  Confidence: band=very_high; score=0.9
- Line 297: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto res = f.get();
  Confidence: band=very_high; score=0.9
- Line 304: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.push_back(v);
  Confidence: band=very_high; score=0.9
- Line 309: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return Ok(std::move(merged));
  Confidence: band=very_high; score=0.9
- Line 55: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: qualified.push_back(qualify(v));
- Line 72: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: qualified_path.path.push_back(qualify(v));
  Confidence: band=high; score=0.74
- Line 73: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: qualified_path.path.push_back(qualify(v));
- Line 99: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ids.push_back(id);
- Line 115: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(id, exec);
  Confidence: band=high; score=0.74
- Line 151: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ordered.push_back(id);
  Confidence: band=high; score=0.74
- Line 152: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ordered.push_back(id);
- Line 196: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "No healthy shards available for distributed shortest path query");
- Line 232: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(
  Confidence: band=high; score=0.74
- Line 233: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(
- Line 274: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "No healthy shards available for distributed k-hop neighbors query");
- Line 286: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(std::launch::async, [exec_ptr = exec.get(), start_local, k, &constraints]() {
  Confidence: band=high; score=0.74
- Line 287: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(std::async(std::launch::async, [exec_ptr = exec.get(), start_local, k, &constraint
- Line 293: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 303: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(v);
  Confidence: band=high; score=0.74
- Line 303: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(v);
  Confidence: band=high; score=0.74
- Line 304: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged.push_back(v);
- Line 325: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "No healthy shards available to generate distributed query plan");
- Line 348: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan.shard_ids.push_back(sid);
  Confidence: band=high; score=0.74
- Line 349: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan.shard_ids.push_back(sid);

### src/graph/knowledge_graph_reasoner.cpp
Total findings: 35

- Line 59: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = insertion_order_.begin();
- Line 61: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator eit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto eit = entries_.find(*it);
- Line 72: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator existing may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto existing = entries_.find(key);
- Line 137: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = insertion_order_.begin();
- Line 139: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator eit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto eit = entries_.find(*it);
- Line 496: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (const auto it = rule_cfg_by_id.find(stored->rule_id); it != rule_cfg_by_id.end()) {
- Line 521: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto managerScore = [&](std::string_view adapter, const InferenceEdge &edge) -> std::optional<
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 123: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &[key, entry] : entries_) {
  Confidence: band=very_high; score=0.9
- Line 138: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto eit = entries_.find(*it);
- Line 206: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &f : base_facts_) {
  Confidence: band=very_high; score=0.9
- Line 509: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Fail-closed hardening: malformed scorer outputs (NaN / +/-Inf) should
  Confidence: band=very_high; score=0.9
- Line 531: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (effective_adapter.empty() && edge_cfg != nullptr && !edge_cfg->adapter_id.empty()) {
- Line 127: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry.edge);
  Confidence: band=high; score=0.74
- Line 128: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(entry.edge);
- Line 210: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: base_facts_.push_back(std::move(fact));
  Confidence: band=high; score=0.74
- Line 211: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: base_facts_.push_back(std::move(fact));
- Line 291: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> known;
  Confidence: band=medium; score=0.66
- Line 316: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conclusions.push_back(std::move(derived_triple));
  Confidence: band=high; score=0.74
- Line 316: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conclusions.push_back(std::move(derived_triple));
  Confidence: band=high; score=0.74
- Line 316: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conclusions.push_back(std::move(derived_triple));
  Confidence: band=high; score=0.74
- Line 316: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conclusions.push_back(std::move(derived_triple));
  Confidence: band=high; score=0.74
- Line 317: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: conclusions.push_back(std::move(derived_triple));
- Line 325: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: premises.push_back(ground(cond, binds));
  Confidence: band=high; score=0.74
- Line 326: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: premises.push_back(ground(cond, binds));
- Line 335: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: new_this_hop.push_back(conclusion);
  Confidence: band=high; score=0.74
- Line 336: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: new_this_hop.push_back(conclusion);
- Line 346: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: derived_out.push_back(std::move(edge));
- Line 355: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: working_set.push_back(std::move(t));
  Confidence: band=high; score=0.74
- Line 356: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: working_set.push_back(std::move(t));
- Line 387: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chain.edges.push_back(std::move(edge));
  Confidence: band=high; score=0.74
- Line 388: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chain.edges.push_back(std::move(edge));
- Line 480: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, RuleLoRAConfig> rule_cfg_by_id;
  Confidence: band=medium; score=0.66

### src/graph/graph_query_rewriter.cpp
Total findings: 29

- Line 93: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: base /= std::max(1.0, static_cast<double>(filters_it->size()) * 2.0);
- Line 98: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: base /= std::max(1.0, static_cast<double>(prune_it->size()) * 3.0);
- Line 104: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: base *= std::max(1.0, static_cast<double>(sv_it->size()));
- Line 262: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: orig_prune += it->size();
- Line 359: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: orig_prune += it->size();
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 418: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("GraphQueryRewriter::addCustomRule: null rule");
- Line 491: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!child_it->is_object()) {
- Line 580: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ref["alias"] = alias_it->second;
- Line 612: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: wrapper["bindings"] = nlohmann::json::array();
- Line 625: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto a = b.find("alias");
  Confidence: band=very_high; score=0.9
- Line 841: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: plan["prune_conditions"] = nlohmann::json::array();
- Line 874: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: plan["prune_conditions"] = nlohmann::json::array();
- Line 279: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " BFS/DFS to reduce explored nodes.\n";
- Line 470: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: node["prune_conditions"].push_back(f);
  Confidence: band=high; score=0.74
- Line 470: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: node["prune_conditions"].push_back(f);
  Confidence: band=high; score=0.74
- Line 471: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: node["prune_conditions"].push_back(f);
- Line 511: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: (*child_it)["vertex_filters"].push_back(*filter_it);
  Confidence: band=high; score=0.74
- Line 512: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: (*child_it)["vertex_filters"].push_back(*filter_it);
- Line 530: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> seen;
  Confidence: band=medium; score=0.66
- Line 551: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> expr_to_alias;
  Confidence: band=medium; score=0.66
- Line 605: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 612: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: wrapper["bindings"].push_back(std::move(let_node));
  Confidence: band=high; score=0.74
- Line 613: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: wrapper["bindings"].push_back(std::move(let_node));
- Line 637: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 639: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: bindings_it->push_back(std::move(let_node));
  Confidence: band=high; score=0.74
- Line 639: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: bindings_it->push_back(std::move(let_node));
  Confidence: band=high; score=0.74
- Line 809: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: subqueries.push_back(std::move(sub));
  Confidence: band=high; score=0.74
- Line 810: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: subqueries.push_back(std::move(sub));

### src/graph/ontology_manager.cpp
Total findings: 22

- Line 179: severity=CRITICAL; category=missing_dtor
  Description: Class YamlEntry allocates resources but has no destructor
  Remediation: Add explicit destructor: ~YamlEntry() { /* cleanup */ }
  Context: class/struct YamlEntry
- Line 227: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator colon may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto colon       = rest.find(':');
- Line 244: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator colon may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto colon = line.find(':');
- Line 335: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: if (auto it = concepts_.find(axiom.source_class); it != concepts_.end()) {
- Line 338: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: if (auto it = concepts_.find(axiom.target_class); it != concepts_.end()) {
- Line 362: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = concepts_.find(std::string(cur));
- Line 70: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += '"';
- Line 73: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += '\\';
- Line 76: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += '/';
- Line 79: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += '\n';
- Line 82: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += '\t';
- Line 85: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += '\r';
- Line 114: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(val));
- Line 353: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string_view> visited;
  Confidence: band=medium; score=0.66
- Line 418: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> OntologyManager::allowedEdgeTypes(std::string_view sourceClass,
  Confidence: band=medium; score=0.66
- Line 420: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> result;
  Confidence: band=medium; score=0.66
- Line 576: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: axioms_.push_back({std::move(src), std::move(et), std::move(tgt)});
- Line 618: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: lines.push_back(line);
- Line 670: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: node.parents.push_back(e.scalar.at("parents"));
  Confidence: band=high; score=0.74
- Line 671: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: node.parents.push_back(e.scalar.at("parents"));
- Line 680: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: axioms_.push_back({std::move(src), std::move(et), std::move(tgt)});
  Confidence: band=high; score=0.74
- Line 681: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: axioms_.push_back({std::move(src), std::move(et), std::move(tgt)});

### src/graph/path_constraints.cpp
Total findings: 9

- Line 32: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return errors::ErrorCode::ERR_QUERY_INVALID_INPUT;
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 32: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return errors::ErrorCode::ERR_QUERY_INVALID_INPUT;
  Confidence: band=very_high; score=0.9
- Line 409: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Validate constraint compatibility
  Confidence: band=high; score=0.8
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 77: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: constraints_.emplace_back(ConstraintType::MIN_LENGTH, min_length);
  Confidence: band=high; score=0.74
- Line 264: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 276: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 448: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: initial.nodes.push_back(std::string(start_node));

### src/graph/graph_watermark.cpp
Total findings: 8

- Line 77: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.data.node_metadata[id] = "wm:tenant=" + tenant_id + ";seed=" + std::to_string(effective_seed)
- Line 34: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(oss.str());
  Confidence: band=high; score=0.74
- Line 35: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ids.push_back(oss.str());
- Line 41: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string> existing(snapshot.node_ids.begin(), snapshot.node_ids.end());
  Confidence: band=medium; score=0.66
- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.data.node_ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 76: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.data.node_ids.push_back(id);
- Line 81: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.data.edges.emplace_back(wm_ids[i], wm_ids[i + 1]);
  Confidence: band=high; score=0.74
- Line 101: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string> set_a(a.begin(), a.end());
  Confidence: band=medium; score=0.66

### src/graph/explain_plan.cpp
Total findings: 5

- Line 41: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': out += "\\\\"; break;
- Line 42: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"': out += "\\\""; break;
- Line 43: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\n': out += "\\n"; break;
- Line 44: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\r': out += "\\r"; break;
- Line 45: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\t': out += "\\t"; break;

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
