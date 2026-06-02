# graph Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: graph
- Generated: 2026-06-02 11:09:13
- Status: Critical Findings Present
- Total Findings: 190
- Actionable Findings (Critical + High): 21
- Affected Files: 14

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 4 |
| High | 17 |
| Medium | 165 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 138 |
| container | 40 |
| determinism | 34 |
| performance | 29 |
| exception_safety | 27 |
| reliability | 15 |
| distributed_consistency | 11 |
| concurrency | 10 |
| platform | 7 |
| raii | 6 |
| legacy_duplication | 5 |
| memory | 5 |
| observability | 5 |
| security | 4 |
| uninitialized | 4 |
| type_conversion | 3 |
| llm_ai_safety | 2 |
| audit_logging | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/graph/graph_query_optimizer.cpp | 54 | 0 | 6 | 44 | 4 |
| src/graph/tensor_fingerprint_graph.cpp | 22 | 0 | 0 | 22 | 0 |
| src/graph/distributed_graph.cpp | 18 | 3 | 6 | 9 | 0 |
| src/graph/parallel_traversal.cpp | 18 | 0 | 0 | 18 | 0 |
| src/graph/gpu_traversal.cpp | 16 | 0 | 0 | 16 | 0 |
| src/graph/scheduled_edge_refresh.cpp | 14 | 0 | 0 | 14 | 0 |
| src/graph/knowledge_graph_reasoner.cpp | 13 | 0 | 1 | 12 | 0 |
| src/graph/graph_query_rewriter.cpp | 10 | 0 | 1 | 9 | 0 |
| src/graph/tensor_deduplication_manager.cpp | 8 | 0 | 1 | 7 | 0 |
| src/graph/path_constraints.cpp | 6 | 1 | 2 | 3 | 0 |
| src/graph/graph_watermark.cpp | 5 | 0 | 0 | 5 | 0 |
| src/graph/ontology_manager.cpp | 5 | 0 | 0 | 5 | 0 |
| src/graph/rotate_completion.cpp | 1 | 0 | 0 | 1 | 0 |
| src/graph/explain_plan.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/graph/graph_query_optimizer.cpp
Total findings: 54

- Line 1266: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto dit = dist.find(v);
  Confidence: band=very_high; score=0.9
- Line 1277: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto nit = dist.find(adj.targetPk);
  Confidence: band=very_high; score=0.9
- Line 1294: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = dist.find(r.vertex);
  Confidence: band=very_high; score=0.9
- Line 1303: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto bit = buckets.find(old_idx);
  Confidence: band=very_high; score=0.9
- Line 1333: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = dist.find(adj.targetPk);
  Confidence: band=very_high; score=0.9
- Line 1341: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto bit = buckets.find(old_idx);
  Confidence: band=very_high; score=0.9
- Line 73: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) hint += ", ";
  Confidence: band=high; score=0.74
- Line 73: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) hint += ", ";
  Confidence: band=high; score=0.74
- Line 76: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan.active_schema_hints.push_back(std::move(hint));
  Confidence: band=high; score=0.74
- Line 81: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) hint += ", ";
  Confidence: band=high; score=0.74
- Line 84: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan.active_schema_hints.push_back(std::move(hint));
  Confidence: band=high; score=0.74
- Line 683: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(node);
  Confidence: band=high; score=0.74
- Line 683: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(node);
  Confidence: band=high; score=0.74
- Line 718: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next_frontier.push_back(nb);
  Confidence: band=high; score=0.74
- Line 837: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(node);
  Confidence: band=high; score=0.74
- Line 837: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(node);
  Confidence: band=high; score=0.74
- Line 868: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next_frontier.push_back(nb);
  Confidence: band=high; score=0.74
- Line 868: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next_frontier.push_back(nb);
  Confidence: band=high; score=0.74
- Line 891: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(std::launch::async, [&, begin_idx, end_idx]() {
  Confidence: band=high; score=0.74
- Line 908: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cr.neighbors.push_back(nb);
  Confidence: band=high; score=0.74
- Line 908: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cr.neighbors.push_back(nb);
  Confidence: band=high; score=0.74
- Line 930: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next_frontier.push_back(nb);
  Confidence: band=high; score=0.74
- Line 930: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next_frontier.push_back(nb);
  Confidence: band=high; score=0.74
- Line 1062: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stack.push_back({neighbor, depth + 1});
  Confidence: band=high; score=0.74
- Line 1190: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: sum += graph_manager_.getEdgeWeight("", adj.edgeId, "_weight");
  Confidence: band=high; score=0.74
- Line 1259: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(std::launch::async,
  Confidence: band=high; score=0.74
- Line 1278: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.relaxations.push_back({adj.targetPk, nd, v});
  Confidence: band=high; score=0.74
- Line 1278: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.relaxations.push_back({adj.targetPk, nd, v});
  Confidence: band=high; score=0.74
- Line 1742: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::unordered_set<std::string>> pattern_adj;
  Confidence: band=medium; score=0.66
- Line 1757: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::unordered_set<std::string>> data_adj_cache;
  Confidence: band=medium; score=0.66
- Line 1768: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> mapping;
  Confidence: band=medium; score=0.66
- Line 1769: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> used_data_vertices;
  Confidence: band=medium; score=0.66
- Line 1917: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> unique_edge_ids;
  Confidence: band=medium; score=0.66
- Line 1964: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, size_t>& label_counts) {
  Confidence: band=medium; score=0.66
- Line 1995: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: explanation += "Selected Algorithm: " + algo_name + "\n";
  Confidence: band=high; score=0.74
- Line 2011: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) explanation += ", ";
  Confidence: band=high; score=0.74
- Line 2029: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: explanation += "  " + alt_name + ": " + std::to_string(alt_cost) + "\n";
  Confidence: band=high; score=0.74
- Line 2036: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: explanation += "  " + hint + "\n";
  Confidence: band=high; score=0.74
- Line 2251: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back(TraversalAlgorithm::BIDIRECTIONAL);
  Confidence: band=high; score=0.74
- Line 2391: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += lbl + "|";
  Confidence: band=high; score=0.74
- Line 2400: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += et + "|";
  Confidence: band=high; score=0.74
- Line 2467: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += lbl + "|";
  Confidence: band=high; score=0.74
- Line 2476: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += et + "|";
  Confidence: band=high; score=0.74
- Line 2626: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<TraversalAlgorithm,
  Confidence: band=medium; score=0.66
- Line 2631: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: acc.actual_times.push_back(s.execution_time_ms);
  Confidence: band=high; score=0.74
- Line 2729: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void GraphQueryOptimizer::unregisterIncrementalQuery(IncrementalQueryHandle handle) {
  Confidence: band=high; score=0.74
- Line 2739: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> changed_vertices;
  Confidence: band=medium; score=0.66
- Line 2788: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string> new_result(result.value().begin(),
  Confidence: band=medium; score=0.66
- Line 2794: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: delta.added.push_back(v);
  Confidence: band=high; score=0.74
- Line 2800: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: delta.removed.push_back(v);
  Confidence: band=high; score=0.74
- Line 1939: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::log(static_cast<double>(stats.vertex_count)) /
  Confidence: band=medium; score=0.6
- Line 1940: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::log(stats.avg_branching_factor)
  Confidence: band=medium; score=0.6
- Line 2126: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::log(statistics_.vertex_count + 1.0);
  Confidence: band=medium; score=0.6
- Line 2132: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::log(statistics_.vertex_count + 1.0) * 0.7;
  Confidence: band=medium; score=0.6

### src/graph/tensor_fingerprint_graph.cpp
Total findings: 22

- Line 113: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: elements.push_back(encoded);
  Confidence: band=high; score=0.74
- Line 113: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: elements.push_back(encoded);
  Confidence: band=high; score=0.74
- Line 123: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: elements.push_back((static_cast<uint64_t>(0xFF) << 24) | (static_cast<uint64_t>(k) << 16)
  Confidence: band=high; score=0.74
- Line 206: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> TensorFingerprintGraph::lshCandidates(const TensorFingerprint &fp) const {
  Confidence: band=medium; score=0.66
- Line 207: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> candidates;
  Confidence: band=medium; score=0.66
- Line 257: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto previous_fp = nodes_[tensor_id].fingerprint;
  Confidence: band=high; score=0.74
- Line 305: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adj_[tensor_id].push_back({cid, similarity});
  Confidence: band=high; score=0.74
- Line 444: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(r);
  Confidence: band=high; score=0.74
- Line 474: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(r);
  Confidence: band=high; score=0.74
- Line 493: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(std::move(persisted));
  Confidence: band=high; score=0.74
- Line 528: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(std::move(persisted));
  Confidence: band=high; score=0.74
- Line 528: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(std::move(persisted));
  Confidence: band=high; score=0.74
- Line 544: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 561: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adj_[edge.from].push_back({edge.to, edge.similarity});
  Confidence: band=high; score=0.74
- Line 579: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: snapshot.nodes.push_back(std::move(persisted));
  Confidence: band=high; score=0.74
- Line 588: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: snapshot.edges.push_back(std::move(persisted));
  Confidence: band=high; score=0.74
- Line 588: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: snapshot.edges.push_back(std::move(persisted));
  Confidence: band=high; score=0.74
- Line 635: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back({tensor_id, edge.to, edge.similarity});
  Confidence: band=high; score=0.74
- Line 660: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 677: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adj_[edge.from].push_back({edge.to, edge.similarity});
  Confidence: band=high; score=0.74
- Line 721: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen_targets;
  Confidence: band=medium; score=0.66
- Line 733: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adj_[node.tensor_id].push_back({edge.to, edge.similarity});
  Confidence: band=high; score=0.74

### src/graph/distributed_graph.cpp
Total findings: 18

- Line 295: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::vector<std::string> merged;
  Confidence: band=very_high; score=0.99
- Line 305: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged.push_back(v);
  Confidence: band=very_high; score=0.99
- Line 310: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return Ok(std::move(merged));
  Confidence: band=very_high; score=0.99
- Line 246: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto res = f.get();
  Confidence: band=very_high; score=0.9
- Line 293: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Merge results: de-duplicate qualified vertex IDs.
  Confidence: band=very_high; score=0.9
- Line 295: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<std::string> merged;
  Confidence: band=very_high; score=0.9
- Line 298: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto res = f.get();
  Confidence: band=very_high; score=0.9
- Line 305: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.push_back(v);
  Confidence: band=very_high; score=0.9
- Line 310: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return Ok(std::move(merged));
  Confidence: band=very_high; score=0.9
- Line 73: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: qualified_path.path.push_back(qualify(v));
  Confidence: band=high; score=0.74
- Line 116: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(id, exec);
  Confidence: band=high; score=0.74
- Line 152: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ordered.push_back(id);
  Confidence: band=high; score=0.74
- Line 233: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(
  Confidence: band=high; score=0.74
- Line 287: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(std::launch::async, [exec_ptr = exec.get(), start_local, k, &constraints]() {
  Confidence: band=high; score=0.74
- Line 294: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 304: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(v);
  Confidence: band=high; score=0.74
- Line 304: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(v);
  Confidence: band=high; score=0.74
- Line 349: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan.shard_ids.push_back(sid);
  Confidence: band=high; score=0.74

### src/graph/parallel_traversal.cpp
Total findings: 18

- Line 68: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> visited;
  Confidence: band=medium; score=0.66
- Line 86: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.visited.push_back(node);
  Confidence: band=high; score=0.74
- Line 86: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.visited.push_back(node);
  Confidence: band=high; score=0.74
- Line 126: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(std::launch::async, [this, &current_frontier, begin, end]() {
  Confidence: band=high; score=0.74
- Line 134: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cr.candidates.push_back(nb);
  Confidence: band=high; score=0.74
- Line 134: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cr.candidates.push_back(nb);
  Confidence: band=high; score=0.74
- Line 151: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next_frontier.push_back(nb);
  Confidence: band=high; score=0.74
- Line 151: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next_frontier.push_back(nb);
  Confidence: band=high; score=0.74
- Line 170: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next_frontier.push_back(nb);
  Confidence: band=high; score=0.74
- Line 170: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next_frontier.push_back(nb);
  Confidence: band=high; score=0.74
- Line 246: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stack.push_back({nb, depth + 1});
  Confidence: band=high; score=0.74
- Line 264: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 273: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.visited_vertices.push_back(v);
  Confidence: band=high; score=0.74
- Line 273: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.visited_vertices.push_back(v);
  Confidence: band=high; score=0.74
- Line 313: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(
  Confidence: band=high; score=0.74
- Line 318: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: per_source.push_back(fut.get());
  Confidence: band=high; score=0.74
- Line 360: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(
  Confidence: band=high; score=0.74
- Line 365: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: per_source.push_back(fut.get());
  Confidence: band=high; score=0.74

### src/graph/gpu_traversal.cpp
Total findings: 16

- Line 146: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: id_to_vertex_.push_back(v);
  Confidence: band=high; score=0.74
- Line 167: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: id_to_vertex_.push_back(nb);
  Confidence: band=high; score=0.74
- Line 167: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: id_to_vertex_.push_back(nb);
  Confidence: band=high; score=0.74
- Line 188: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: row_offsets_.push_back(static_cast<uint32_t>(column_indices_.size()));
  Confidence: band=high; score=0.74
- Line 192: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: column_indices_.push_back(nb);
  Confidence: band=high; score=0.74
- Line 236: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<uint32_t> forbidden_ids;
  Confidence: band=medium; score=0.66
- Line 266: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ordered.emplace_back(gpu_dist[i], i);
  Confidence: band=high; score=0.74
- Line 279: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.visited_vertices.push_back(id_to_vertex_[vid]);
  Confidence: band=high; score=0.74
- Line 306: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.visited_vertices.push_back(id_to_vertex_[v]);
  Confidence: band=high; score=0.74
- Line 306: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.visited_vertices.push_back(id_to_vertex_[v]);
  Confidence: band=high; score=0.74
- Line 337: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next_frontier.push_back(nb);
  Confidence: band=high; score=0.74
- Line 337: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next_frontier.push_back(nb);
  Confidence: band=high; score=0.74
- Line 357: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<uint32_t> forbidden_ids;
  Confidence: band=medium; score=0.66
- Line 386: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ordered.emplace_back(gpu_order[i], i);
  Confidence: band=high; score=0.74
- Line 394: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.visited_vertices.push_back(id_to_vertex_[vid]);
  Confidence: band=high; score=0.74
- Line 443: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stack.emplace_back(nb, depth + 1);
  Confidence: band=high; score=0.74

### src/graph/scheduled_edge_refresh.cpp
Total findings: 14

- Line 339: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: flat_ids.push_back(idx);
  Confidence: band=high; score=0.74
- Line 461: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_remove.push_back(s.edge_id);
  Confidence: band=high; score=0.74
- Line 461: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_remove.push_back(s.edge_id);
  Confidence: band=high; score=0.74
- Line 512: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> removed_set(to_remove.begin(), to_remove.end());
  Confidence: band=medium; score=0.66
- Line 579: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back(std::move(e));
  Confidence: band=high; score=0.74
- Line 591: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen_edges;
  Confidence: band=medium; score=0.66
- Line 635: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scores.push_back(scoreEdge(edge));
  Confidence: band=high; score=0.74
- Line 655: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> existing_pairs;
  Confidence: band=medium; score=0.66
- Line 668: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> dedup;
  Confidence: band=medium; score=0.66
- Line 732: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.emplace_back(sim, other);
  Confidence: band=high; score=0.74
- Line 748: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.emplace_back(vertex, other, sim);
  Confidence: band=high; score=0.74
- Line 779: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.emplace_back(sim, other);
  Confidence: band=high; score=0.74
- Line 795: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.emplace_back(vertex, other, sim);
  Confidence: band=high; score=0.74
- Line 861: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: added_records.push_back({new_id, from, to, sim});
  Confidence: band=high; score=0.74

### src/graph/knowledge_graph_reasoner.cpp
Total findings: 13

- Line 510: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Fail-closed hardening: malformed scorer outputs (NaN / +/-Inf) should
  Confidence: band=very_high; score=0.9
- Line 128: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry.edge);
  Confidence: band=high; score=0.74
- Line 211: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: base_facts_.push_back(std::move(fact));
  Confidence: band=high; score=0.74
- Line 292: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> known;
  Confidence: band=medium; score=0.66
- Line 317: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conclusions.push_back(std::move(derived_triple));
  Confidence: band=high; score=0.74
- Line 317: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conclusions.push_back(std::move(derived_triple));
  Confidence: band=high; score=0.74
- Line 317: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conclusions.push_back(std::move(derived_triple));
  Confidence: band=high; score=0.74
- Line 317: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conclusions.push_back(std::move(derived_triple));
  Confidence: band=high; score=0.74
- Line 326: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: premises.push_back(ground(cond, binds));
  Confidence: band=high; score=0.74
- Line 336: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: new_this_hop.push_back(conclusion);
  Confidence: band=high; score=0.74
- Line 356: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: working_set.push_back(std::move(t));
  Confidence: band=high; score=0.74
- Line 388: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chain.edges.push_back(std::move(edge));
  Confidence: band=high; score=0.74
- Line 481: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, RuleLoRAConfig> rule_cfg_by_id;
  Confidence: band=medium; score=0.66

### src/graph/graph_query_rewriter.cpp
Total findings: 10

- Line 626: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto a = b.find("alias");
  Confidence: band=very_high; score=0.9
- Line 471: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: node["prune_conditions"].push_back(f);
  Confidence: band=high; score=0.74
- Line 471: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: node["prune_conditions"].push_back(f);
  Confidence: band=high; score=0.74
- Line 512: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: (*child_it)["vertex_filters"].push_back(*filter_it);
  Confidence: band=high; score=0.74
- Line 531: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> seen;
  Confidence: band=medium; score=0.66
- Line 552: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> expr_to_alias;
  Confidence: band=medium; score=0.66
- Line 613: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: wrapper["bindings"].push_back(std::move(let_node));
  Confidence: band=high; score=0.74
- Line 640: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: bindings_it->push_back(std::move(let_node));
  Confidence: band=high; score=0.74
- Line 640: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: bindings_it->push_back(std::move(let_node));
  Confidence: band=high; score=0.74
- Line 810: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: subqueries.push_back(std::move(sub));
  Confidence: band=high; score=0.74

### src/graph/tensor_deduplication_manager.cpp
Total findings: 8

- Line 1353: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &entry : entries) {
  Confidence: band=very_high; score=0.9
- Line 493: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.push_back(p[i]);
  Confidence: band=high; score=0.74
- Line 500: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.push_back(c);
  Confidence: band=high; score=0.74
- Line 777: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: snapshot.edges.push_back(std::move(e));
  Confidence: band=high; score=0.74
- Line 930: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entry.edges.push_back(std::move(edge));
  Confidence: band=high; score=0.74
- Line 1176: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: records.push_back(std::move(record));
  Confidence: band=high; score=0.74
- Line 1216: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: records.push_back(record);
  Confidence: band=high; score=0.74
- Line 1590: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>((hi << 4U) | lo));
  Confidence: band=high; score=0.74

### src/graph/path_constraints.cpp
Total findings: 6

- Line 33: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return errors::ErrorCode::ERR_QUERY_INVALID_INPUT;
  Confidence: band=very_high; score=0.99
- Line 33: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return errors::ErrorCode::ERR_QUERY_INVALID_INPUT;
  Confidence: band=very_high; score=0.9
- Line 410: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Validate constraint compatibility
  Confidence: band=high; score=0.8
- Line 78: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: constraints_.emplace_back(ConstraintType::MIN_LENGTH, min_length);
  Confidence: band=high; score=0.74
- Line 265: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 277: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66

### src/graph/graph_watermark.cpp
Total findings: 5

- Line 35: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(oss.str());
  Confidence: band=high; score=0.74
- Line 42: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string> existing(snapshot.node_ids.begin(), snapshot.node_ids.end());
  Confidence: band=medium; score=0.66
- Line 76: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.data.node_ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 82: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.data.edges.emplace_back(wm_ids[i], wm_ids[i + 1]);
  Confidence: band=high; score=0.74
- Line 102: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string> set_a(a.begin(), a.end());
  Confidence: band=medium; score=0.66

### src/graph/ontology_manager.cpp
Total findings: 5

- Line 354: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string_view> visited;
  Confidence: band=medium; score=0.66
- Line 419: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> OntologyManager::allowedEdgeTypes(std::string_view sourceClass,
  Confidence: band=medium; score=0.66
- Line 421: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> result;
  Confidence: band=medium; score=0.66
- Line 671: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: node.parents.push_back(e.scalar.at("parents"));
  Confidence: band=high; score=0.74
- Line 681: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: axioms_.push_back({std::move(src), std::move(et), std::move(tgt)});
  Confidence: band=high; score=0.74

### src/graph/rotate_completion.cpp
Total findings: 1

- Line 377: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back({entity_names_[scored[i].second],
  Confidence: band=high; score=0.74

### src/graph/explain_plan.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
