# analytics Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: analytics
- Generated: 2026-06-02 11:09:12
- Status: Critical Findings Present
- Total Findings: 611
- Actionable Findings (Critical + High): 149
- Affected Files: 24

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 33 |
| High | 116 |
| Medium | 457 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 438 |
| container | 199 |
| reliability | 87 |
| concurrency | 86 |
| distributed_consistency | 60 |
| exception_safety | 54 |
| platform | 48 |
| determinism | 46 |
| performance | 30 |
| memory | 29 |
| observability | 27 |
| audit_logging | 20 |
| llm_ai_safety | 19 |
| raii | 14 |
| uninitialized | 12 |
| legacy_duplication | 6 |
| oop_design | 3 |
| type_conversion | 3 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/analytics/process_mining.cpp | 113 | 0 | 8 | 105 | 0 |
| src/analytics/distributed_analytics.cpp | 84 | 27 | 42 | 15 | 0 |
| src/analytics/olap.cpp | 76 | 0 | 9 | 67 | 0 |
| src/analytics/cep_engine.cpp | 45 | 0 | 1 | 44 | 0 |
| src/analytics/automl.cpp | 43 | 0 | 2 | 40 | 1 |
| src/analytics/streaming_join.cpp | 30 | 0 | 1 | 29 | 0 |
| src/analytics/columnar_execution.cpp | 28 | 0 | 10 | 18 | 0 |
| src/analytics/ml_serving.cpp | 28 | 0 | 18 | 10 | 0 |
| src/analytics/forecasting.cpp | 23 | 0 | 4 | 17 | 2 |
| src/analytics/model_serving.cpp | 18 | 4 | 11 | 3 | 0 |
| src/analytics/nlp_text_analyzer.cpp | 18 | 0 | 0 | 17 | 1 |
| src/analytics/anomaly_detection.cpp | 15 | 0 | 0 | 14 | 1 |
| src/analytics/llm_process_analyzer.cpp | 14 | 2 | 2 | 10 | 0 |
| src/analytics/process_pattern_matcher.cpp | 14 | 0 | 1 | 13 | 0 |
| src/analytics/streaming_window.cpp | 14 | 0 | 1 | 13 | 0 |
| src/analytics/arrow_flight.cpp | 10 | 0 | 0 | 10 | 0 |
| src/analytics/diff_engine.cpp | 8 | 0 | 0 | 8 | 0 |
| src/analytics/incremental_view.cpp | 8 | 0 | 2 | 6 | 0 |
| src/analytics/expert_system_engine.cpp | 6 | 0 | 0 | 6 | 0 |
| src/analytics/analytics_export.cpp | 5 | 0 | 4 | 1 | 0 |
| src/analytics/arrow_export.cpp | 3 | 0 | 0 | 3 | 0 |
| src/analytics/knowledge_base.cpp | 3 | 0 | 0 | 3 | 0 |
| src/analytics/lora_pattern_classifier.cpp | 3 | 0 | 0 | 3 | 0 |
| src/analytics/jit_aggregation.cpp | 2 | 0 | 0 | 2 | 0 |

## Full Scanner Findings

### src/analytics/process_mining.cpp
Total findings: 113

- Line 17: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: //   are defined.  The stub flag is an explicit opt-in compatibility switch for
  Confidence: band=high; score=0.8
- Line 793: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it1 = parallel.find({targets[i], targets[j]});
  Confidence: band=very_high; score=0.9
- Line 794: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it2 = parallel.find({targets[j], targets[i]});
  Confidence: band=very_high; score=0.9
- Line 851: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it1 = parallel.find({sources[i], sources[j]});
  Confidence: band=very_high; score=0.9
- Line 852: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it2 = parallel.find({sources[j], sources[i]});
  Confidence: band=very_high; score=0.9
- Line 1050: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (ids.find(e.activity) == ids.end()) {
  Confidence: band=very_high; score=0.9
- Line 1809: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: tokens.erase(tokens.find(token));
  Confidence: band=very_high; score=0.9
- Line 2337: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: m.type     = (syncCost == 0.0) ? "sync" : "log+model";
  Confidence: band=very_high; score=0.9
- Line 222: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<ProcessEvent>> cases;
  Confidence: band=high; score=0.74
- Line 306: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, int> variant_counts;
  Confidence: band=high; score=0.74
- Line 310: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort by timestamp
  Confidence: band=high; score=0.74
- Line 326: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: activitySeq.push_back(e.activity);
  Confidence: band=high; score=0.74
- Line 326: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: activitySeq.push_back(e.activity);
  Confidence: band=high; score=0.74
- Line 346: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: log.traces.push_back(std::move(trace));
  Confidence: band=high; score=0.74
- Line 353: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: log.id_to_activity.push_back(act);
  Confidence: band=high; score=0.74
- Line 377: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<ProcessEvent>> cases;
  Confidence: band=high; score=0.74
- Line 427: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort events within each case by timestamp
  Confidence: band=high; score=0.74
- Line 434: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: log.traces.push_back(std::move(trace));
  Confidence: band=high; score=0.74
- Line 464: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<ProcessEvent>> cases;
  Confidence: band=high; score=0.74
- Line 484: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> visited;
  Confidence: band=medium; score=0.66
- Line 543: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort by timestamp
  Confidence: band=high; score=0.74
- Line 549: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: log.traces.push_back(std::move(trace));
  Confidence: band=high; score=0.74
- Line 613: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dfg.edges.push_back(edge);
  Confidence: band=high; score=0.74
- Line 708: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: process.nodes.push_back(node);
  Confidence: band=high; score=0.74
- Line 725: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: process.edges.push_back(edge);
  Confidence: band=high; score=0.74
- Line 743: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: process.edges.push_back(edge);
  Confidence: band=high; score=0.74
- Line 743: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: process.edges.push_back(edge);
  Confidence: band=high; score=0.74
- Line 757: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: process.edges.push_back(edge);
  Confidence: band=high; score=0.74
- Line 763: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::string>> outgoing; // activity -> list of following activities
  Confidence: band=high; score=0.74
- Line 764: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::string>> incoming; // activity -> list of preceding activities
  Confidence: band=high; score=0.74
- Line 777: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: outgoing[fromName].push_back(toName);
  Confidence: band=high; score=0.74
- Line 777: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: outgoing[fromName].push_back(toName);
  Confidence: band=high; score=0.74
- Line 806: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: process.nodes.push_back(gateway);
  Confidence: band=high; score=0.74
- Line 806: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: process.nodes.push_back(gateway);
  Confidence: band=high; score=0.74
- Line 810: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto actNode = actToNode[activity];
  Confidence: band=high; score=0.74
- Line 813: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> targetNodes;
  Confidence: band=medium; score=0.66
- Line 829: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: process.edges.push_back(toGateway);
  Confidence: band=high; score=0.74
- Line 837: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: process.edges.push_back(fromGateway);
  Confidence: band=high; score=0.74
- Line 864: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: process.nodes.push_back(gateway);
  Confidence: band=high; score=0.74
- Line 864: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: process.nodes.push_back(gateway);
  Confidence: band=high; score=0.74
- Line 864: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: process.nodes.push_back(gateway);
  Confidence: band=high; score=0.74
- Line 868: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto actNode = actToNode[activity];
  Confidence: band=high; score=0.74
- Line 871: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> sourceNodes;
  Confidence: band=medium; score=0.66
- Line 888: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: process.edges.push_back(toGateway);
  Confidence: band=high; score=0.74
- Line 888: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: process.edges.push_back(toGateway);
  Confidence: band=high; score=0.74
- Line 955: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: process.nodes.push_back(startNode);
  Confidence: band=high; score=0.74
- Line 974: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: process.nodes.push_back(node);
  Confidence: band=high; score=0.74
- Line 974: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: process.nodes.push_back(node);
  Confidence: band=high; score=0.74
- Line 974: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: process.nodes.push_back(node);
  Confidence: band=high; score=0.74
- Line 993: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: process.edges.push_back(edge);
  Confidence: band=high; score=0.74
- Line 1011: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: process.edges.push_back(edge);
  Confidence: band=high; score=0.74
- Line 1011: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: process.edges.push_back(edge);
  Confidence: band=high; score=0.74
- Line 1021: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: process.edges.push_back(edge);
  Confidence: band=high; score=0.74
- Line 1045: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, int> buildActivityIds(const std::vector<ProcessTrace> &traces) {
  Confidence: band=high; score=0.74
- Line 1046: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, int> ids;
  Confidence: band=high; score=0.74
- Line 1101: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> visited;
  Confidence: band=medium; score=0.66
- Line 1122: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> parent;
  Confidence: band=medium; score=0.66
- Line 1145: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(g));
  Confidence: band=high; score=0.74
- Line 1145: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(g));
  Confidence: band=high; score=0.74
- Line 1145: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(g));
  Confidence: band=high; score=0.74
- Line 1200: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: order.push_back(node);
  Confidence: band=high; score=0.74
- Line 1200: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: order.push_back(node);
  Confidence: band=high; score=0.74
- Line 1345: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sub.events.push_back(e);
  Confidence: band=high; score=0.74
- Line 1345: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sub.events.push_back(e);
  Confidence: band=high; score=0.74
- Line 1345: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sub.events.push_back(e);
  Confidence: band=high; score=0.74
- Line 1359: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sub.events.push_back(e);
  Confidence: band=high; score=0.74
- Line 1359: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sub.events.push_back(e);
  Confidence: band=high; score=0.74
- Line 1373: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sub.events.push_back(e);
  Confidence: band=high; score=0.74
- Line 1373: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sub.events.push_back(e);
  Confidence: band=high; score=0.74
- Line 1413: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: process.nodes.push_back(task);
  Confidence: band=high; score=0.74
- Line 1466: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: process.nodes.push_back(task);
  Confidence: band=high; score=0.74
- Line 1466: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: process.nodes.push_back(task);
  Confidence: band=high; score=0.74
- Line 1555: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: process.nodes.push_back(mid);
  Confidence: band=high; score=0.74
- Line 1600: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: process.nodes.push_back(loopStart);
  Confidence: band=high; score=0.74
- Line 1662: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: process.nodes.push_back(startNode);
  Confidence: band=high; score=0.74
- Line 1662: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: process.nodes.push_back(startNode);
  Confidence: band=high; score=0.74
- Line 1694: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: actSeq.push_back(e.activity);
  Confidence: band=high; score=0.74
- Line 1694: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: actSeq.push_back(e.activity);
  Confidence: band=high; score=0.74
- Line 1713: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(v));
  Confidence: band=high; score=0.74
- Line 1746: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: transitions[fromName].push_back(toName);
  Confidence: band=high; score=0.74
- Line 1746: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: transitions[fromName].push_back(toName);
  Confidence: band=high; score=0.74
- Line 1823: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.deviations.push_back("Case " + trace.case_id + ": missing token for activity '" + activity
  Confidence: band=high; score=0.74
- Line 1857: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.deviations.push_back("Case " + trace.case_id + ": ended without reaching end node");
  Confidence: band=high; score=0.74
- Line 1857: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.deviations.push_back("Case " + trace.case_id + ": ended without reaching end node");
  Confidence: band=high; score=0.74
- Line 2009: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: embedding.push_back(normalized);
  Confidence: band=high; score=0.74
- Line 2009: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: embedding.push_back(normalized);
  Confidence: band=high; score=0.74
- Line 2056: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: variant_keys.push_back(sig);
  Confidence: band=high; score=0.74
- Line 2067: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result[0].push_back(idx);
  Confidence: band=high; score=0.74
- Line 2067: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result[0].push_back(idx);
  Confidence: band=high; score=0.74
- Line 2156: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result[cluster_id].push_back(trace_idx);
  Confidence: band=high; score=0.74
- Line 2156: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result[cluster_id].push_back(trace_idx);
  Confidence: band=high; score=0.74
- Line 2156: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result[cluster_id].push_back(trace_idx);
  Confidence: band=high; score=0.74
- Line 2156: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result[cluster_id].push_back(trace_idx);
  Confidence: band=high; score=0.74
- Line 2251: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: succ_list[src].push_back(d);
  Confidence: band=high; score=0.74
- Line 2251: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: succ_list[src].push_back(d);
  Confidence: band=high; score=0.74
- Line 2251: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: succ_list[src].push_back(d);
  Confidence: band=high; score=0.74
- Line 2265: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: modelOrder.push_back(cur);
  Confidence: band=high; score=0.74
- Line 2275: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: modelOrder.push_back(act);
  Confidence: band=high; score=0.74
- Line 2275: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: modelOrder.push_back(act);
  Confidence: band=high; score=0.74
- Line 2396: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: activity_durations[trace.events[i].activity].push_back(duration);
  Confidence: band=high; score=0.74
- Line 2396: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: activity_durations[trace.events[i].activity].push_back(duration);
  Confidence: band=high; score=0.74
- Line 2423: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: durations.push_back(duration);
  Confidence: band=high; score=0.74
- Line 2423: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: durations.push_back(duration);
  Confidence: band=high; score=0.74
- Line 2439: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: bottlenecks.push_back(activity);
  Confidence: band=high; score=0.74
- Line 2489: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: activities.push_back(event.activity);
  Confidence: band=high; score=0.74
- Line 2489: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: activities.push_back(event.activity);
  Confidence: band=high; score=0.74
- Line 2489: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: activities.push_back(event.activity);
  Confidence: band=high; score=0.74
- Line 2496: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pattern_info[window].second.push_back(trace.case_id);
  Confidence: band=high; score=0.74
- Line 2503: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: freq_sorted.push_back({info.first, {seq, info.second}});
  Confidence: band=high; score=0.74
- Line 2517: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(frag);
  Confidence: band=high; score=0.74
- Line 2532: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: variant_traces[log.traces[i].variant_signature].push_back(i);
  Confidence: band=high; score=0.74
- Line 2549: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: clusters.push_back(cluster);
  Confidence: band=high; score=0.74
- Line 2583: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: evolution.snapshots.push_back(snapshot);
  Confidence: band=high; score=0.74

### src/analytics/distributed_analytics.cpp
Total findings: 84

- Line 21: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: *     → merge: SUM/COUNT aggregated, AVG recomputed, MIN/MAX reduced
  Confidence: band=very_high; score=0.99
- Line 22: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: *     → returns merged OLAPResult; partial results returned when < 20% shards fail
  Confidence: band=very_high; score=0.99
- Line 32: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: * Thread safety: `DistributedAnalyticsSharding` is thread-safe; concurrent
  Confidence: band=very_high; score=0.99
- Line 110: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Per-group merge accumulator
  Confidence: band=very_high; score=0.99
- Line 114: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: * Tracks the partial state needed to correctly merge one measure column
  Confidence: band=very_high; score=0.99
- Line 159: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // during the shard-merge step we use the weighted approach.
  Confidence: band=very_high; score=0.99
- Line 188: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // duplicates).  A full HyperLogLog merge would be exact.
  Confidence: band=very_high; score=0.99
- Line 232: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: * Merge another Chan state (for STDDEV/VARIANCE parallel combination).
  Confidence: band=very_high; score=0.99
- Line 234: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: void mergeVarianceState(double other_count, double other_mean, double other_m2) {
  Confidence: band=very_high; score=0.99
- Line 245: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: /** Finalise and return the merged aggregate value. */
  Confidence: band=very_high; score=0.99
- Line 507: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: OLAPResult DistributedAnalyticsSharding::mergeResults(const std::vector<OLAPResult> &partials, const OLAPQuery &query) {
  Confidence: band=very_high; score=0.99
- Line 515: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: OLAPResult merged;
  Confidence: band=very_high; score=0.99
- Line 518: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged.columns = p.columns;
  Confidence: band=very_high; score=0.99
- Line 586: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // the caller added one.  We use it for weighted merge.
  Confidence: band=very_high; score=0.99
- Line 608: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Step 3: Merge grand_totals (SUM / COUNT / MIN / MAX)
  Confidence: band=very_high; score=0.99
- Line 628: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Step 4: Build the merged rows from the accumulators
  Confidence: band=very_high; score=0.99
- Line 630: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged.rows.reserve(group_order.size());
  Confidence: band=very_high; score=0.99
- Line 642: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged.rows.push_back(std::move(out));
  Confidence: band=very_high; score=0.99
- Line 651: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged.grand_totals[m.name] = toDouble(it->second.finalise());
  Confidence: band=very_high; score=0.99
- Line 655: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged.total_rows        = static_cast<int64_t>(merged.rows.size());
  Confidence: band=very_high; score=0.99
- Line 656: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged.has_more          = false;
  Confidence: band=very_high; score=0.99
- Line 792: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: "allow_partial_results=false; aborting merge",
  Confidence: band=very_high; score=0.99
- Line 807: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: "max_failure_rate {:.1f}% ({}/{} shards failed); aborting merge",
  Confidence: band=very_high; score=0.99
- Line 809: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Return partial shard_info without a merged result so the caller
  Confidence: band=very_high; score=0.99
- Line 821: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Merge
  Confidence: band=very_high; score=0.99
- Line 823: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: result.merged = mergeResults(partials, query);
  Confidence: band=very_high; score=0.99
- Line 832: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return executeDistributed(query).merged;
  Confidence: band=very_high; score=0.99
- Line 21: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: *     → merge: SUM/COUNT aggregated, AVG recomputed, MIN/MAX reduced
  Confidence: band=very_high; score=0.9
- Line 22: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: *     → returns merged OLAPResult; partial results returned when < 20% shards fail
  Confidence: band=very_high; score=0.9
- Line 110: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Per-group merge accumulator
  Confidence: band=very_high; score=0.9
- Line 114: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: * Tracks the partial state needed to correctly merge one measure column
  Confidence: band=very_high; score=0.9
- Line 159: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // during the shard-merge step we use the weighted approach.
  Confidence: band=very_high; score=0.9
- Line 188: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // duplicates).  A full HyperLogLog merge would be exact.
  Confidence: band=very_high; score=0.9
- Line 232: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: * Merge another Chan state (for STDDEV/VARIANCE parallel combination).
  Confidence: band=very_high; score=0.9
- Line 234: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: void mergeVarianceState(double other_count, double other_mean, double other_m2) {
  Confidence: band=very_high; score=0.9
- Line 245: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: /** Finalise and return the merged aggregate value. */
  Confidence: band=very_high; score=0.9
- Line 255: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (count == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 261: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (min_val == std::numeric_limits<double>::max()) {
  Confidence: band=very_high; score=0.9
- Line 267: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (max_val == std::numeric_limits<double>::lowest()) {
  Confidence: band=very_high; score=0.9
- Line 294: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (count == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 350: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: health_monitor_thread_ = std::thread(&DistributedAnalyticsSharding::runHealthMonitor, this);
  Confidence: band=very_high; score=0.9
- Line 503: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // mergeResults
  Confidence: band=very_high; score=0.9
- Line 507: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: OLAPResult DistributedAnalyticsSharding::mergeResults(const std::vector<OLAPResult> &partials, const OLAPQuery &query) {
  Confidence: band=very_high; score=0.9
- Line 515: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: OLAPResult merged;
  Confidence: band=very_high; score=0.9
- Line 518: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.columns = p.columns;
  Confidence: band=very_high; score=0.9
- Line 546: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = groups.find(key);
  Confidence: band=very_high; score=0.9
- Line 555: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto vit = row.values.find(dim.name);
  Confidence: band=very_high; score=0.9
- Line 586: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // the caller added one.  We use it for weighted merge.
  Confidence: band=very_high; score=0.9
- Line 608: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Step 3: Merge grand_totals (SUM / COUNT / MIN / MAX)
  Confidence: band=very_high; score=0.9
- Line 618: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto git = partial.grand_totals.find(m.name);
  Confidence: band=very_high; score=0.9
- Line 628: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Step 4: Build the merged rows from the accumulators
  Confidence: band=very_high; score=0.9
- Line 630: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.rows.reserve(group_order.size());
  Confidence: band=very_high; score=0.9
- Line 636: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto ait = acc.measures.find(m.name);
  Confidence: band=very_high; score=0.9
- Line 642: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.rows.push_back(std::move(out));
  Confidence: band=very_high; score=0.9
- Line 651: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.grand_totals[m.name] = toDouble(it->second.finalise());
  Confidence: band=very_high; score=0.9
- Line 655: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.total_rows        = static_cast<int64_t>(merged.rows.size());
  Confidence: band=very_high; score=0.9
- Line 656: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.has_more          = false;
  Confidence: band=very_high; score=0.9
- Line 657: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.execution_time_ms = 0.0;
  Confidence: band=very_high; score=0.9
- Line 659: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.execution_time_ms += p.execution_time_ms;
  Confidence: band=very_high; score=0.9
- Line 662: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return merged;
  Confidence: band=very_high; score=0.9
- Line 729: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: std::thread([entry, query, promise = std::move(promise)]() mutable {
  Confidence: band=very_high; score=0.9
- Line 784: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto [partial, info] = f.get();
  Confidence: band=very_high; score=0.9
- Line 792: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: "allow_partial_results=false; aborting merge",
  Confidence: band=very_high; score=0.9
- Line 807: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: "max_failure_rate {:.1f}% ({}/{} shards failed); aborting merge",
  Confidence: band=very_high; score=0.9
- Line 809: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Return partial shard_info without a merged result so the caller
  Confidence: band=very_high; score=0.9
- Line 821: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Merge
  Confidence: band=very_high; score=0.9
- Line 823: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: result.merged = mergeResults(partials, query);
  Confidence: band=very_high; score=0.9
- Line 831: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: OLAPResult DistributedAnalyticsSharding::execute(const OLAPQuery &query) {
  Confidence: band=very_high; score=0.9
- Line 832: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return executeDistributed(query).merged;
  Confidence: band=very_high; score=0.9
- Line 424: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shards_.push_back(std::move(entry));
  Confidence: band=high; score=0.74
- Line 474: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(e.shard_id);
  Confidence: band=high; score=0.74
- Line 524: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, Measure::Function> measure_funcs;
  Confidence: band=medium; score=0.66
- Line 530: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, bool> dim_set;
  Confidence: band=medium; score=0.66
- Line 539: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, GroupAccumulator> groups;
  Confidence: band=medium; score=0.66
- Line 569: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: group_order.push_back(key);
  Confidence: band=high; score=0.74
- Line 569: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: group_order.push_back(key);
  Confidence: band=high; score=0.74
- Line 610: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, MeasureAccumulator> grand_accs;
  Confidence: band=medium; score=0.66
- Line 641: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.rows.push_back(std::move(out));
  Confidence: band=high; score=0.74
- Line 641: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.rows.push_back(std::move(out));
  Confidence: band=high; score=0.74
- Line 686: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active.push_back(e);
  Confidence: band=high; score=0.74
- Line 726: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(promise.get_future());
  Confidence: band=high; score=0.74
- Line 778: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.shard_info.push_back(std::move(info));
  Confidence: band=high; score=0.74
- Line 778: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.shard_info.push_back(std::move(info));
  Confidence: band=high; score=0.74
- Line 831: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: OLAPResult DistributedAnalyticsSharding::execute(const OLAPQuery &query) {
  Confidence: band=high; score=0.74

### src/analytics/olap.cpp
Total findings: 76

- Line 293: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: OLAPResult OLAPEngine::execute(const OLAPQuery &query) {
  Confidence: band=very_high; score=0.9
- Line 408: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto fieldIt = row.find(dim.name);
  Confidence: band=very_high; score=0.9
- Line 491: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (aVal != bVal) {
  Confidence: band=very_high; score=0.9
- Line 668: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = row.values.find(dim.name);
  Confidence: band=very_high; score=0.9
- Line 1192: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = row.find(name);
  Confidence: band=very_high; score=0.9
- Line 1530: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = agg_map.find(m.name);
  Confidence: band=very_high; score=0.9
- Line 1780: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (aVal != bVal) {
  Confidence: band=very_high; score=0.9
- Line 1836: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = row.values.find(col_name);
  Confidence: band=very_high; score=0.9
- Line 1891: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = row.values.find(col_name);
  Confidence: band=very_high; score=0.9
- Line 254: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: fstr += "null";
  Confidence: band=high; score=0.74
- Line 254: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: fstr += "null";
  Confidence: band=high; score=0.74
- Line 254: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: fstr += "null";
  Confidence: band=high; score=0.74
- Line 254: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: fstr += "null";
  Confidence: band=high; score=0.74
- Line 254: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: fstr += "null";
  Confidence: band=high; score=0.74
- Line 266: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: fstr += ',';
  Confidence: band=high; score=0.74
- Line 271: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filter_strs.push_back(std::move(fstr));
  Confidence: band=high; score=0.74
- Line 389: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.columns.push_back(dim.name);
  Confidence: band=high; score=0.74
- Line 392: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.columns.push_back(measure.name);
  Confidence: band=high; score=0.74
- Line 397: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::vector<std::string>, std::vector<double>> groups;
  Confidence: band=high; score=0.74
- Line 410: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: groupKey.push_back(*s);
  Confidence: band=high; score=0.74
- Line 410: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: groupKey.push_back(*s);
  Confidence: band=high; score=0.74
- Line 434: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: groups[groupKey].push_back(val);
  Confidence: band=high; score=0.74
- Line 452: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: measureValues.push_back(values[i]);
  Confidence: band=high; score=0.74
- Line 452: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: measureValues.push_back(values[i]);
  Confidence: band=high; score=0.74
- Line 452: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: measureValues.push_back(values[i]);
  Confidence: band=high; score=0.74
- Line 452: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: measureValues.push_back(values[i]);
  Confidence: band=high; score=0.74
- Line 518: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: OLAPResult OLAPEngine::executeCubeQuery(const OLAPQuery &query) {
  Confidence: band=high; score=0.74
- Line 527: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.columns.push_back(dim.name);
  Confidence: band=high; score=0.74
- Line 530: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.columns.push_back(measure.name);
  Confidence: band=high; score=0.74
- Line 542: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: subQuery.dimensions.push_back(query.dimensions[i]);
  Confidence: band=high; score=0.74
- Line 542: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: subQuery.dimensions.push_back(query.dimensions[i]);
  Confidence: band=high; score=0.74
- Line 557: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 557: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 566: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: OLAPResult OLAPEngine::executeRollupQuery(const OLAPQuery &query) {
  Confidence: band=high; score=0.74
- Line 574: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.columns.push_back(dim.name);
  Confidence: band=high; score=0.74
- Line 577: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.columns.push_back(measure.name);
  Confidence: band=high; score=0.74
- Line 588: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: subQuery.dimensions.push_back(query.dimensions[i]);
  Confidence: band=high; score=0.74
- Line 588: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: subQuery.dimensions.push_back(query.dimensions[i]);
  Confidence: band=high; score=0.74
- Line 599: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 599: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 608: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: OLAPResult OLAPEngine::executeGroupingSetsQuery(const OLAPQuery &query) {
  Confidence: band=high; score=0.74
- Line 612: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.columns.push_back(dim.name);
  Confidence: band=high; score=0.74
- Line 615: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.columns.push_back(measure.name);
  Confidence: band=high; score=0.74
- Line 625: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> setDimensions(groupingSet.dimensions.begin(), groupingSet.dimensions.end());
  Confidence: band=medium; score=0.66
- Line 628: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: subQuery.dimensions.push_back(dim);
  Confidence: band=high; score=0.74
- Line 628: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: subQuery.dimensions.push_back(dim);
  Confidence: band=high; score=0.74
- Line 641: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 641: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 688: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cells.push_back(std::move(cell));
  Confidence: band=high; score=0.74
- Line 721: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rollupRow.dimension_values.push_back(std::nullopt);
  Confidence: band=high; score=0.74
- Line 721: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rollupRow.dimension_values.push_back(std::nullopt);
  Confidence: band=high; score=0.74
- Line 743: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rows.push_back(std::move(rollupRow));
  Confidence: band=high; score=0.74
- Line 751: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: OLAPEngine::evaluateWindowFunctions(const std::vector<std::unordered_map<std::string, double>> &data,
  Confidence: band=medium; score=0.66
- Line 777: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto it = data[j].find(measure.field);
  Confidence: band=high; score=0.74
- Line 778: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: windowValues.push_back(it->second);
  Confidence: band=high; score=0.74
- Line 778: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: windowValues.push_back(it->second);
  Confidence: band=high; score=0.74
- Line 913: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: gpu_rows.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 1146: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, Column> columns;
  Confidence: band=medium; score=0.66
- Line 1159: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.push_back(*d);
  Confidence: band=high; score=0.74
- Line 1188: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::vector<std::unordered_map<std::string, std::variant<std::nullptr_t, bool, int64_t, double, std::string>>>
  Confidence: band=medium; score=0.66
- Line 1193: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: col.data.push_back(it->second);
  Confidence: band=high; score=0.74
- Line 1193: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: col.data.push_back(it->second);
  Confidence: band=high; score=0.74
- Line 1284: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> unique;
  Confidence: band=medium; score=0.66
- Line 1330: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> unique;
  Confidence: band=medium; score=0.66
- Line 1446: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::map<std::string, std::unordered_map<std::string, AggState>> groups;
  Confidence: band=medium; score=0.66
- Line 1446: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::unordered_map<std::string, AggState>> groups;
  Confidence: band=high; score=0.74
- Line 1460: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += '\0';
  Confidence: band=high; score=0.74
- Line 1513: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.columns.push_back(d.name);
  Confidence: band=high; score=0.74
- Line 1513: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.columns.push_back(d.name);
  Confidence: band=high; score=0.74
- Line 1516: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.columns.push_back(m.name);
  Confidence: band=high; score=0.74
- Line 1534: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.rows.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 1534: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.rows.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 1534: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.rows.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 1561: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::vector<std::unordered_map<std::string, std::variant<std::nullptr_t, bool, int64_t, double, std::string>>>
  Confidence: band=medium; score=0.66
- Line 1805: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: MaterializedView::isStale()
  Context: bool MaterializedView::isStale() const {
  Confidence: band=medium; score=0.56
- Line 1957: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arrays.push_back(array);
  Confidence: band=high; score=0.74

### src/analytics/cep_engine.cpp
Total findings: 45

- Line 2720: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (result == EventStream::PushResult::DROPPED) {
  Confidence: band=very_high; score=0.9
- Line 440: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: partitions_.emplace_back(std::make_unique<Partition>());
  Confidence: band=high; score=0.74
- Line 596: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: s.transitions.push_back(i + 1);
  Confidence: band=high; score=0.74
- Line 629: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> ctx;
  Confidence: band=high; score=0.74
- Line 667: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: group_key += (it != event.fields.end() ? fieldValueToString(it->second) : "") + ":";
  Confidence: band=high; score=0.74
- Line 667: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: group_key += (it != event.fields.end() ? fieldValueToString(it->second) : "") + ":";
  Confidence: band=high; score=0.74
- Line 682: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pm.matched_events.push_back(event);
  Confidence: band=high; score=0.74
- Line 682: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pm.matched_events.push_back(event);
  Confidence: band=high; score=0.74
- Line 715: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: completed.push_back(std::move(result));
  Confidence: band=high; score=0.74
- Line 744: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pm.matched_events.push_back(event);
  Confidence: band=high; score=0.74
- Line 744: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pm.matched_events.push_back(event);
  Confidence: band=high; score=0.74
- Line 780: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: completed.push_back(std::move(result));
  Confidence: band=high; score=0.74
- Line 780: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: completed.push_back(std::move(result));
  Confidence: band=high; score=0.74
- Line 780: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: completed.push_back(std::move(result));
  Confidence: band=high; score=0.74
- Line 799: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: extended.matched_events.push_back(event);
  Confidence: band=high; score=0.74
- Line 815: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next_active.push_back(extended);
  Confidence: band=high; score=0.74
- Line 823: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next_active.push_back(std::move(advanced));
  Confidence: band=high; score=0.74
- Line 851: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: newpm.matched_events.push_back(event);
  Confidence: band=high; score=0.74
- Line 1061: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: w.events.push_back(event);
  Confidence: band=high; score=0.74
- Line 1070: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batches.push_back(std::move(*b));
  Confidence: band=high; score=0.74
- Line 1099: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: w.events.push_back(event);
  Confidence: band=high; score=0.74
- Line 1189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(ev);
  Confidence: band=high; score=0.74
- Line 1189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(ev);
  Confidence: band=high; score=0.74
- Line 1189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(ev);
  Confidence: band=high; score=0.74
- Line 1197: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(ev);
  Confidence: band=high; score=0.74
- Line 1197: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(ev);
  Confidence: band=high; score=0.74
- Line 1242: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batches.push_back({w.events, w.start, now});
  Confidence: band=high; score=0.74
- Line 1242: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batches.push_back({w.events, w.start, now});
  Confidence: band=high; score=0.74
- Line 1264: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batches.push_back(std::move(*b));
  Confidence: band=high; score=0.74
- Line 1264: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batches.push_back(std::move(*b));
  Confidence: band=high; score=0.74
- Line 1322: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += (it != event.fields.end() ? fieldValueToString(it->second) : "") + "|";
  Confidence: band=high; score=0.74
- Line 1322: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += (it != event.fields.end() ? fieldValueToString(it->second) : "") + "|";
  Confidence: band=high; score=0.74
- Line 1410: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: strs.push_back(std::to_string(v));
  Confidence: band=high; score=0.74
- Line 1422: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: strs.push_back(std::to_string(v));
  Confidence: band=high; score=0.74
- Line 1457: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, AggregationResult> Aggregator::getResults() const {
  Confidence: band=high; score=0.74
- Line 1459: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, AggregationResult> results;
  Confidence: band=high; score=0.74
- Line 1565: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(s.config);
  Confidence: band=high; score=0.74
- Line 1729: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: alerts.push_back(std::move(alert));
  Confidence: band=high; score=0.74
- Line 1769: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: norm += ' ';
  Confidence: band=high; score=0.74
- Line 2099: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: params.push_back(val);
  Confidence: band=high; score=0.74
- Line 2263: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: worker_threads_.emplace_back([this] { workerLoop(); });
  Confidence: band=high; score=0.74
- Line 2323: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(s);
  Confidence: band=high; score=0.74
- Line 2468: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(*it);
  Confidence: band=high; score=0.74
- Line 2492: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: alerts_.push_back(alert);
  Confidence: band=high; score=0.74
- Line 2686: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry.path().stem().string());
  Confidence: band=high; score=0.74

### src/analytics/automl.cpp
Total findings: 43

- Line 146: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = p.fields.find(fm.names[j]);
  Confidence: band=very_high; score=0.9
- Line 860: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (t == p) {
  Confidence: band=very_high; score=0.9
- Line 156: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fm.X.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 168: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back("");
  Confidence: band=high; score=0.74
- Line 198: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(0.0);
  Confidence: band=high; score=0.74
- Line 277: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.push_back(v * v);
  Confidence: band=high; score=0.74
- Line 277: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.push_back(v * v);
  Confidence: band=high; score=0.74
- Line 287: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(n + "^2");
  Confidence: band=high; score=0.74
- Line 299: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, int> index;
  Confidence: band=high; score=0.74
- Line 313: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(it != index.end() ? it->second : 0);
  Confidence: band=high; score=0.74
- Line 490: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vals.push_back(X[i][static_cast<size_t>(f)]);
  Confidence: band=high; score=0.74
- Line 520: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: left_idx.push_back(i);
  Confidence: band=high; score=0.74
- Line 596: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: left.push_back(i);
  Confidence: band=high; score=0.74
- Line 596: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: left.push_back(i);
  Confidence: band=high; score=0.74
- Line 932: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: train.push_back(idx[i]);
  Confidence: band=high; score=0.74
- Line 932: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: train.push_back(idx[i]);
  Confidence: band=high; score=0.74
- Line 1160: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dists.emplace_back(l2sq(x, X_train[i]), static_cast<int>(i));
  Confidence: band=high; score=0.74
- Line 1214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: copy->members.push_back(m->clone());
  Confidence: band=high; score=0.74
- Line 1214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: copy->members.push_back(m->clone());
  Confidence: band=high; score=0.74
- Line 1214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: copy->members.push_back(m->clone());
  Confidence: band=high; score=0.74
- Line 1214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: copy->members.push_back(m->clone());
  Confidence: band=high; score=0.74
- Line 1261: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: Xb.push_back(X[i]);
  Confidence: band=high; score=0.74
- Line 1314: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: gb->stages.push_back({std::move(t), learning_rate});
  Confidence: band=high; score=0.74
- Line 1314: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: gb->stages.push_back({std::move(t), learning_rate});
  Confidence: band=high; score=0.74
- Line 1314: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: gb->stages.push_back({std::move(t), learning_rate});
  Confidence: band=high; score=0.74
- Line 1346: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: gb->stages.push_back({std::move(t), learning_rate});
  Confidence: band=high; score=0.74
- Line 1346: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: gb->stages.push_back({std::move(t), learning_rate});
  Confidence: band=high; score=0.74
- Line 1346: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: gb->stages.push_back({std::move(t), learning_rate});
  Confidence: band=high; score=0.74
- Line 1476: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> sampleHP(const HPGrid &grid, std::mt19937 &rng) {
  Confidence: band=high; score=0.74
- Line 1477: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> hp;
  Confidence: band=high; score=0.74
- Line 1514: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> feat_importance;
  Confidence: band=high; score=0.74
- Line 1588: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(explainOne(p));
  Confidence: band=high; score=0.74
- Line 1632: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: exp.feature_contributions.emplace_back(name, contrib);
  Confidence: band=high; score=0.74
- Line 1644: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: tf += ", ";
  Confidence: band=high; score=0.74
- Line 1644: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: tf += ", ";
  Confidence: band=high; score=0.74
- Line 1714: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "\\\"";
  Confidence: band=high; score=0.74
- Line 1939: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: ModelAlgorithm algo, const std::map<std::string, double> &hp, std::mt19937 &rng, AutoMLMetric metric) {
  Confidence: band=high; score=0.74
- Line 1961: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: y_bin.push_back(v >= mean_y ? 1 : 0);
  Confidence: band=high; score=0.74
- Line 2013: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: Xt.push_back(X[i]);
  Confidence: band=high; score=0.74
- Line 2013: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: Xt.push_back(X[i]);
  Confidence: band=high; score=0.74
- Line 2021: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: Xv.push_back(X[i]);
  Confidence: band=high; score=0.74
- Line 2233: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ens->members.push_back(std::move(m));
  Confidence: band=high; score=0.74
- Line 1325: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: gb->base_value = std::log(mean_p / (1.0 - mean_p));
  Confidence: band=medium; score=0.6

### src/analytics/streaming_join.cpp
Total findings: 30

- Line 368: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto bit   = std::find(build_column_names_.begin(), build_column_names_.end(), n);
  Confidence: band=very_high; score=0.9
- Line 119: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += '\xFF'; // separator
  Confidence: band=high; score=0.74
- Line 119: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += '\xFF'; // separator
  Confidence: band=high; score=0.74
- Line 119: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += '\xFF'; // separator
  Confidence: band=high; score=0.74
- Line 141: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(col);
  Confidence: band=high; score=0.74
- Line 184: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::make_shared<Column>(c->name(), c->type()));
  Confidence: band=high; score=0.74
- Line 211: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: build_columns_.push_back(std::make_shared<Column>(col->name(), col->type()));
  Confidence: band=high; score=0.74
- Line 247: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: key_indices.push_back(i);
  Confidence: band=high; score=0.74
- Line 247: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: key_indices.push_back(i);
  Confidence: band=high; score=0.74
- Line 301: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: probe_key_indices.push_back(i);
  Confidence: band=high; score=0.74
- Line 301: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: probe_key_indices.push_back(i);
  Confidence: band=high; score=0.74
- Line 331: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out_cols.push_back(std::make_shared<Column>(c->name(), c->type()));
  Confidence: band=high; score=0.74
- Line 336: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out_cols.push_back(std::make_shared<Column>(n, build_columns_[ci]->type()));
  Confidence: band=high; score=0.74
- Line 441: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vals.push_back(c->get(r));
  Confidence: band=high; score=0.74
- Line 486: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: probe_cols.push_back(probe_batch.getColumnAt(i));
  Confidence: band=high; score=0.74
- Line 495: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: probe_cols.push_back(c);
  Confidence: band=high; score=0.74
- Line 507: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: probe_key_indices.push_back(i);
  Confidence: band=high; score=0.74
- Line 507: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: probe_key_indices.push_back(i);
  Confidence: band=high; score=0.74
- Line 520: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: build_key_indices.push_back(i);
  Confidence: band=high; score=0.74
- Line 588: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: probe_vals.push_back(probe_cols[ci]->get(r));
  Confidence: band=high; score=0.74
- Line 595: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: probe_key += '\xFF';
  Confidence: band=high; score=0.74
- Line 595: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: probe_key += '\xFF';
  Confidence: band=high; score=0.74
- Line 595: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: probe_key += '\xFF';
  Confidence: band=high; score=0.74
- Line 605: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: build_key += '\xFF';
  Confidence: band=high; score=0.74
- Line 605: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: build_key += '\xFF';
  Confidence: band=high; score=0.74
- Line 615: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: build_out_vals.push_back(it->values[bi]);
  Confidence: band=high; score=0.74
- Line 684: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out_cols_mat.push_back(std::make_shared<Column>(probe_col_names[i], t));
  Confidence: band=high; score=0.74
- Line 684: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto t = probe_types[i] == ColumnType::Null ? ColumnType::String : probe_types[i];
  Confidence: band=high; score=0.74
- Line 688: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out_cols_mat.push_back(std::make_shared<Column>(build_output_names[i], t));
  Confidence: band=high; score=0.74
- Line 688: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto t = build_types[i] == ColumnType::Null ? ColumnType::String : build_types[i];
  Confidence: band=high; score=0.74

### src/analytics/columnar_execution.cpp
Total findings: 28

- Line 592: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: ColumnBatch FilterOperator::execute(const ColumnBatch &input) const {
  Confidence: band=very_high; score=0.9
- Line 627: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: ColumnBatch ProjectOperator::execute(const ColumnBatch &input) const {
  Confidence: band=very_high; score=0.9
- Line 901: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: ColumnBatch AggregateOperator::execute(const ColumnBatch &input) const {
  Confidence: band=very_high; score=0.9
- Line 1085: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: ColumnBatch SortOperator::execute(const ColumnBatch &input) const {
  Confidence: band=very_high; score=0.9
- Line 1119: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (va != vb) {
  Confidence: band=very_high; score=0.9
- Line 1127: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (va != vb) {
  Confidence: band=very_high; score=0.9
- Line 1135: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (va != vb) {
  Confidence: band=very_high; score=0.9
- Line 1143: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (va != vb) {
  Confidence: band=very_high; score=0.9
- Line 1205: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: ColumnBatch VectorizedPipeline::execute(const ColumnBatch &input) const {
  Confidence: band=very_high; score=0.9
- Line 1238: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: ColumnBatch ColumnarExecutionEngine::execute(const ColumnBatch &input, const VectorizedPipeline &pipeline) {
  Confidence: band=very_high; score=0.9
- Line 390: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(sub));
  Confidence: band=high; score=0.74
- Line 390: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(sub));
  Confidence: band=high; score=0.74
- Line 496: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sel.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 504: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sel.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 519: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sel.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 527: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sel.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 546: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sel.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 561: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sel.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 578: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sel.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 592: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: ColumnBatch FilterOperator::execute(const ColumnBatch &input) const {
  Confidence: band=high; score=0.74
- Line 627: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: ColumnBatch ProjectOperator::execute(const ColumnBatch &input) const {
  Confidence: band=high; score=0.74
- Line 901: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: ColumnBatch AggregateOperator::execute(const ColumnBatch &input) const {
  Confidence: band=high; score=0.74
- Line 993: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: key_order.push_back(it->first); // reference key already in the map
  Confidence: band=high; score=0.74
- Line 1085: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: ColumnBatch SortOperator::execute(const ColumnBatch &input) const {
  Confidence: band=high; score=0.74
- Line 1157: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sel.push_back(static_cast<uint32_t>(idx));
  Confidence: band=high; score=0.74
- Line 1176: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stages_.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 1205: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: ColumnBatch VectorizedPipeline::execute(const ColumnBatch &input) const {
  Confidence: band=high; score=0.74
- Line 1258: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(execute(batch, pipeline));
  Confidence: band=high; score=0.74

### src/analytics/ml_serving.cpp
Total findings: 28

- Line 247: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: if (req.inputs.empty()) {
  Confidence: band=very_high; score=0.9
- Line 275: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (const auto &t : req.inputs) {
  Confidence: band=very_high; score=0.9
- Line 302: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Convert outputs
  Confidence: band=very_high; score=0.9
- Line 316: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: resp.outputs.push_back(std::move(out_tensor));
  Confidence: band=very_high; score=0.9
- Line 402: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: if (req.inputs.empty()) {
  Confidence: band=very_high; score=0.9
- Line 408: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Build JSON payload: { "inputs": { "<name>": [[...]] } }
  Confidence: band=very_high; score=0.9
- Line 410: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: json inputs_json = json::object();
  Confidence: band=very_high; score=0.9
- Line 411: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (const auto &t : req.inputs) {
  Confidence: band=very_high; score=0.9
- Line 414: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: inputs_json[t.name] = json(t.data.begin(), t.data.end());
  Confidence: band=very_high; score=0.9
- Line 416: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: payload["inputs"] = inputs_json;
  Confidence: band=very_high; score=0.9
- Line 477: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Parse JSON response: { "outputs": { "<name>": [...] } }
  Confidence: band=very_high; score=0.9
- Line 481: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: if (!jresp.contains("outputs")) {
  Confidence: band=very_high; score=0.9
- Line 483: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: resp.error_message = "TF Serving response missing 'outputs' field";
  Confidence: band=very_high; score=0.9
- Line 487: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const auto &joutputs = jresp["outputs"];
  Confidence: band=very_high; score=0.9
- Line 488: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: if (joutputs.is_object()) {
  Confidence: band=very_high; score=0.9
- Line 489: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (auto &[name, val] : joutputs.items()) {
  Confidence: band=very_high; score=0.9
- Line 505: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: resp.outputs.push_back(std::move(t));
  Confidence: band=very_high; score=0.9
- Line 652: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: req.inputs.push_back(MLTensor{input_name, {1, static_cast<int64_t>(values.size())}, std::move(values)});
  Confidence: band=very_high; score=0.9
- Line 122: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::shared_ptr<Ort::Session>> sessions;
  Confidence: band=high; score=0.74
- Line 127: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::shared_ptr<std::mutex>> model_load_mutexes;
  Confidence: band=high; score=0.74
- Line 275: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: input_names.push_back(t.name.c_str());
  Confidence: band=high; score=0.74
- Line 291: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out_name_strs.emplace_back(name_alloc.get());
  Confidence: band=high; score=0.74
- Line 294: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: output_names.push_back(s.c_str());
  Confidence: band=high; score=0.74
- Line 315: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resp.outputs.push_back(std::move(out_tensor));
  Confidence: band=high; score=0.74
- Line 422: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: url += "/versions/" + req.model_version;
  Confidence: band=high; score=0.74
- Line 498: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: t.data.push_back(node.get<float>());
  Confidence: band=high; score=0.74
- Line 498: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: t.data.push_back(node.get<float>());
  Confidence: band=high; score=0.74
- Line 636: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.push_back(static_cast<float>(*v_double));
  Confidence: band=high; score=0.74

### src/analytics/forecasting.cpp
Total findings: 23

- Line 534: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (base == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 562: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: Snew         = gamma * (y[i] / (Lnew != 0.0 ? Lnew : 1e-10)) + (1.0 - gamma) * S[static_cast<size_t>(si)];
  Confidence: band=very_high; score=0.9
- Line 1239: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (a != b) {
  Confidence: band=very_high; score=0.9
- Line 2032: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: season_acc[static_cast<size_t>(si)] += multiplicative ? (base != 0.0 ? y[i] / base : 1.0) : (y[i] - base);
  Confidence: band=very_high; score=0.9
- Line 121: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: t.push_back(p.timestamp_ms);
  Confidence: band=high; score=0.74
- Line 130: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.points_.push_back(p);
  Confidence: band=high; score=0.74
- Line 149: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: TimeSeries::mean()
  Context: double TimeSeries::mean() const {
  Confidence: band=medium; score=0.56
- Line 387: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: diffs.push_back(static_cast<double>(timestamps[i] - timestamps[i - 1]));
  Confidence: band=high; score=0.74
- Line 516: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: season_avgs.push_back(avg / static_cast<double>(im));
  Confidence: band=high; score=0.74
- Line 516: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: season_avgs.push_back(avg / static_cast<double>(im));
  Confidence: band=high; score=0.74
- Line 829: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ar_lags.push_back(i);
  Confidence: band=high; score=0.74
- Line 829: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ar_lags.push_back(i);
  Confidence: band=high; score=0.74
- Line 832: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ar_lags.push_back(i * params.m);
  Confidence: band=high; score=0.74
- Line 934: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ma_lags.push_back(i);
  Confidence: band=high; score=0.74
- Line 934: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ma_lags.push_back(i);
  Confidence: band=high; score=0.74
- Line 934: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ma_lags.push_back(i);
  Confidence: band=high; score=0.74
- Line 937: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ma_lags.push_back(i * params.m);
  Confidence: band=high; score=0.74
- Line 1344: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(trend + s_weekly + s_yearly);
  Confidence: band=high; score=0.74
- Line 1577: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(pred_val);
  Confidence: band=high; score=0.74
- Line 1577: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(pred_val);
  Confidence: band=high; score=0.74
- Line 1843: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(m.predict(steps));
  Confidence: band=high; score=0.74
- Line 252: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double t = std::sqrt(-2.0 * std::log(p));
  Confidence: band=medium; score=0.6
- Line 261: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double t = std::sqrt(-2.0 * std::log(1.0 - p));
  Confidence: band=medium; score=0.6

### src/analytics/model_serving.cpp
Total findings: 18

- Line 30: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: *     when called via loadModel() with existing key.
  Confidence: band=very_high; score=0.99
- Line 399: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // loadModel
  Confidence: band=very_high; score=0.99
- Line 402: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: void ModelServingEngine::loadModel(const std::string &name, const std::string &version,
  Confidence: band=very_high; score=0.99
- Line 404: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto model = AutoMLModel::deserialize(serialized_data);
  Confidence: band=very_high; score=0.99
- Line 11: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: * Model Serving and Online Inference Pipeline – Implementation
  Confidence: band=very_high; score=0.9
- Line 20: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: *     → InferenceResult{class_label, probabilities} + latency update
  Confidence: band=very_high; score=0.9
- Line 27: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: *   - `std::runtime_error`: inference failure inside AutoMLModel::predict()
  Confidence: band=very_high; score=0.9
- Line 35: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: *   tests/analytics/test_model_serving.cpp — registry, inference, health metrics
  Confidence: band=very_high; score=0.9
- Line 45: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: *     stores the duration of each inference call in milliseconds.
  Confidence: band=very_high; score=0.9
- Line 67: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Latency window - protected by its own mutex so concurrent inference
  Confidence: band=very_high; score=0.9
- Line 148: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // inference and metrics updates outside the registry lock, so that concurrent
  Confidence: band=very_high; score=0.9
- Line 149: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // register/unregister calls are never starved by long-running inference.
  Confidence: band=very_high; score=0.9
- Line 235: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Run inference outside the registry lock so that concurrent
  Confidence: band=very_high; score=0.9
- Line 269: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Run batch inference outside the registry lock.
  Confidence: band=very_high; score=0.9
- Line 302: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Run inference outside the registry lock.
  Confidence: band=very_high; score=0.9
- Line 306: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::vector<std::map<std::string, double>> out;
  Confidence: band=high; score=0.74
- Line 316: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back({{"value", val}});
  Confidence: band=high; score=0.74
- Line 343: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(e->info);
  Confidence: band=high; score=0.74

### src/analytics/nlp_text_analyzer.cpp
Total findings: 18

- Line 162: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<Language, size_t> scores;
  Confidence: band=high; score=0.74
- Line 228: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(std::move(token));
  Confidence: band=high; score=0.74
- Line 258: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> unique_terms;
  Confidence: band=medium; score=0.66
- Line 278: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keywords.emplace_back(term, score, freq);
  Confidence: band=high; score=0.74
- Line 278: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keywords.emplace_back(term, score, freq);
  Confidence: band=high; score=0.74
- Line 306: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(entity);
  Confidence: band=high; score=0.74
- Line 306: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(entity);
  Confidence: band=high; score=0.74
- Line 319: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(entity);
  Confidence: band=high; score=0.74
- Line 378: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> unique;
  Confidence: band=medium; score=0.66
- Line 433: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> NlpTextAnalyzer::extractQueryHints(std::string_view query_text) const {
  Confidence: band=high; score=0.74
- Line 434: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> hints;
  Confidence: band=high; score=0.74
- Line 503: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string NlpTextAnalyzer::normalizeQuery(std::string_view query_text) const {
  Confidence: band=high; score=0.74
- Line 1676: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> set1, set2;
  Confidence: band=medium; score=0.66
- Line 1695: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, size_t> NlpTextAnalyzer::getStatistics() const {
  Confidence: band=high; score=0.74
- Line 1743: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: double NlpTextAnalyzer::calculateTfIdf(const std::string &term, const std::map<std::string, size_t> &term_freqs,
  Confidence: band=high; score=0.74
- Line 1808: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool NlpTextAnalyzer::containsSubquery(std::string_view query) const {
  Confidence: band=high; score=0.74
- Line 2137: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: modalities.push_back(modality);
  Confidence: band=high; score=0.74
- Line 1755: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double idf = std::log(static_cast<double>(term_freqs.size()) / it->second);
  Confidence: band=medium; score=0.6

### src/analytics/anomaly_detection.cpp
Total findings: 15

- Line 62: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(*d);
  Confidence: band=high; score=0.74
- Line 77: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(name);
  Confidence: band=high; score=0.74
- Line 198: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(row[col]);
  Confidence: band=high; score=0.74
- Line 315: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: left_idx.push_back(i);
  Confidence: band=high; score=0.74
- Line 381: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dists.emplace_back(euclidean(train[i], query), i);
  Confidence: band=high; score=0.74
- Line 440: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(0.0);
  Confidence: band=high; score=0.74
- Line 657: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: lof_train.push_back(impl_extractForLof(p));
  Confidence: band=high; score=0.74
- Line 790: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sub_detectors.push_back(std::move(sub));
  Confidence: band=high; score=0.74
- Line 881: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(predict(p));
  Confidence: band=high; score=0.74
- Line 966: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: exp.feature_contributions.emplace_back(impl_->feature_names[i], contrib[i]);
  Confidence: band=high; score=0.74
- Line 966: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: exp.feature_contributions.emplace_back(impl_->feature_names[i], contrib[i]);
  Confidence: band=high; score=0.74
- Line 966: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: exp.feature_contributions.emplace_back(impl_->feature_names[i], contrib[i]);
  Confidence: band=high; score=0.74
- Line 1062: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: try { v.push_back(std::stod(t)); } catch (...) {}
  Confidence: band=high; score=0.74
- Line 1169: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::optional<AnomalyResult> StreamingAnomalyDetector::process(const DataPoint &point) {
  Confidence: band=high; score=0.74
- Line 218: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double H = std::log(n - 1.0) + 0.5772156649; // harmonic number approximation
  Confidence: band=medium; score=0.6

### src/analytics/llm_process_analyzer.cpp
Total findings: 14

- Line 500: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto sha256hex = [](const std::string &input) -> std::string {
  Confidence: band=very_high; score=0.99
- Line 502: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: SHA256(reinterpret_cast<const unsigned char *>(input.data()), input.size(), hash);
  Confidence: band=very_high; score=0.99
- Line 500: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto sha256hex = [](const std::string &input) -> std::string {
  Confidence: band=very_high; score=0.9
- Line 502: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: SHA256(reinterpret_cast<const unsigned char *>(input.data()), input.size(), hash);
  Confidence: band=very_high; score=0.9
- Line 215: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response.deviations.push_back(dev);
  Confidence: band=high; score=0.74
- Line 225: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response.compliance_issues.push_back(issue);
  Confidence: band=high; score=0.74
- Line 235: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response.recommendations.push_back(rec);
  Confidence: band=high; score=0.74
- Line 254: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: response.predictions.push_back(pred);
  Confidence: band=high; score=0.74
- Line 263: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto fr                  = parsed["five_rights_check"];
  Confidence: band=high; score=0.74
- Line 272: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: check.corrective_actions.push_back(action.get<std::string>());
  Confidence: band=high; score=0.74
- Line 283: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto fa          = parsed["fraud_analysis"];
  Confidence: band=high; score=0.74
- Line 286: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fraud.detected_anomalies.push_back(anomaly.get<std::string>());
  Confidence: band=high; score=0.74
- Line 291: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto flags                        = fa["flags"];
  Confidence: band=high; score=0.74
- Line 395: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: [[maybe_unused]] const std::map<std::string, std::string> &params) {
  Confidence: band=high; score=0.74

### src/analytics/process_pattern_matcher.cpp
Total findings: 14

- Line 806: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (norm_a == 0.0 || norm_b == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 69: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: acts.push_back(e.activity);
  Confidence: band=high; score=0.74
- Line 377: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched.push_back(a);
  Confidence: band=high; score=0.74
- Line 384: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing.push_back(a);
  Confidence: band=high; score=0.74
- Line 392: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_edges.push_back(e);
  Confidence: band=high; score=0.74
- Line 490: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.deviations.push_back("MISSING: " + a);
  Confidence: band=high; score=0.74
- Line 490: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.deviations.push_back("MISSING: " + a);
  Confidence: band=high; score=0.74
- Line 495: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.deviations.push_back("EXTRA: " + a);
  Confidence: band=high; score=0.74
- Line 502: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.deviations.push_back("MISSING_EDGE: " + e.first + "->" + e.second);
  Confidence: band=high; score=0.74
- Line 556: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::pair<ProcessPatternMatcher::Status, std::map<std::string, SimilarityResult>>
  Confidence: band=high; score=0.74
- Line 559: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, SimilarityResult> results;
  Confidence: band=high; score=0.74
- Line 572: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, const ProcessTrace *> trace_map;
  Confidence: band=medium; score=0.66
- Line 625: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.matched_activities.push_back(a);
  Confidence: band=high; score=0.74
- Line 632: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.missing_activities.push_back(a);
  Confidence: band=high; score=0.74

### src/analytics/streaming_window.cpp
Total findings: 14

- Line 203: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = rec.fields.find(spec.field);
  Confidence: band=very_high; score=0.9
- Line 213: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nums.push_back(d);
  Confidence: band=high; score=0.74
- Line 290: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(av));
  Confidence: band=high; score=0.74
- Line 403: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pending.push_back(computeResult(closed, false));
  Confidence: band=high; score=0.74
- Line 634: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: windows_.push_back(std::move(win));
  Confidence: band=high; score=0.74
- Line 648: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pending.push_back(computeResult(w, false));
  Confidence: band=high; score=0.74
- Line 701: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: w.records.push_back(record);
  Confidence: band=high; score=0.74
- Line 934: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pending.push_back(computeResult(s, s.has_late_records));
  Confidence: band=high; score=0.74
- Line 974: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_close.push_back(key);
  Confidence: band=high; score=0.74
- Line 974: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_close.push_back(key);
  Confidence: band=high; score=0.74
- Line 980: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pending.push_back(computeResult(s, s.has_late_records));
  Confidence: band=high; score=0.74
- Line 1068: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: windows_.push_back(std::move(win));
  Confidence: band=high; score=0.74
- Line 1082: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pending.push_back(computeResult(w, false));
  Confidence: band=high; score=0.74
- Line 1125: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: w.records.push_back(record);
  Confidence: band=high; score=0.74

### src/analytics/arrow_flight.cpp
Total findings: 10

- Line 183: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: parts.push_back(part);
  Confidence: band=high; score=0.74
- Line 280: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fields.push_back(arrow::field(col.schema.name, arrow::int64(), col.schema.nullable));
  Confidence: band=high; score=0.74
- Line 298: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arrays.push_back(arr);
  Confidence: band=high; score=0.74
- Line 321: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arrays.push_back(arr);
  Confidence: band=high; score=0.74
- Line 342: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arrays.push_back(arr);
  Confidence: band=high; score=0.74
- Line 362: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arrays.push_back(arr);
  Confidence: band=high; score=0.74
- Line 406: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: row.emplace_back(nullptr);
  Confidence: band=high; score=0.74
- Line 406: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: row.emplace_back(nullptr);
  Confidence: band=high; score=0.74
- Line 547: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: flight_infos.push_back(std::move(ainfo));
  Confidence: band=high; score=0.74
- Line 887: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fields.push_back(arrow::field(col.schema.name, dt, col.schema.nullable));
  Confidence: band=high; score=0.74

### src/analytics/diff_engine.cpp
Total findings: 8

- Line 81: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: added_arr.push_back(change.toJson());
  Confidence: band=high; score=0.74
- Line 87: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: modified_arr.push_back(change.toJson());
  Confidence: band=high; score=0.74
- Line 93: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: deleted_arr.push_back(change.toJson());
  Confidence: band=high; score=0.74
- Line 115: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.added.push_back(Change::fromJson(item));
  Confidence: band=high; score=0.74
- Line 118: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.modified.push_back(Change::fromJson(item));
  Confidence: band=high; score=0.74
- Line 121: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.deleted.push_back(Change::fromJson(item));
  Confidence: band=high; score=0.74
- Line 383: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: key_events[event.key].push_back(event);
  Confidence: band=high; score=0.74
- Line 492: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.added.push_back(change);
  Confidence: band=high; score=0.74

### src/analytics/incremental_view.cpp
Total findings: 8

- Line 418: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t batch_start = 0; batch_start < filtered.size(); batch_start += kMicroBatchSize) {
  Confidence: band=very_high; score=0.9
- Line 473: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function query without trace point
  Context: ViewQueryResult IncrementalView::query(const std::vector<ViewFilter> &filters, int64_t limit, int64_t offset) const {
  Confidence: band=very_high; score=0.9
- Line 223: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> IncrementalView::parseGroupKey(const GroupKey &gk) const {
  Confidence: band=medium; score=0.66
- Line 224: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> result;
  Confidence: band=medium; score=0.66
- Line 249: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: bool IncrementalView::passesRuntimeFilters(const std::unordered_map<std::string, std::string> &gk,
  Confidence: band=medium; score=0.66
- Line 473: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: ViewQueryResult IncrementalView::query(const std::vector<ViewFilter> &filters, int64_t limit, int64_t offset) const {
  Confidence: band=high; score=0.74
- Line 509: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 593: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(n);
  Confidence: band=high; score=0.74

### src/analytics/expert_system_engine.cpp
Total findings: 6

- Line 318: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched.push_back(f);
  Confidence: band=high; score=0.74
- Line 318: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched.push_back(f);
  Confidence: band=high; score=0.74
- Line 318: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched.push_back(f);
  Confidence: band=high; score=0.74
- Line 318: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched.push_back(f);
  Confidence: band=high; score=0.74
- Line 318: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched.push_back(f);
  Confidence: band=high; score=0.74
- Line 463: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sub_trace.push_back(step);
  Confidence: band=high; score=0.74

### src/analytics/analytics_export.cpp
Total findings: 5

- Line 394: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: bool needs_quotes = str.find(',') != std::string::npos || str.find('"') != std::string::npos
  Confidence: band=very_high; score=0.9
- Line 395: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: || str.find('\n') != std::string::npos
  Confidence: band=very_high; score=0.9
- Line 396: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: || str.find('\r') != std::string::npos
  Confidence: band=very_high; score=0.9
- Line 397: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: || (!str.empty() && kFormulaChars.find(str[0]) != std::string::npos);
  Confidence: band=very_high; score=0.9
- Line 199: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arrays.push_back(array);
  Confidence: band=high; score=0.74

### src/analytics/arrow_export.cpp
Total findings: 3

- Line 27: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: columns_[i].data.push_back(row_data[i]);
  Confidence: band=high; score=0.74
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: meta.schema.push_back(col.schema);
  Confidence: band=high; score=0.74
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: meta.schema.push_back(col.schema);
  Confidence: band=high; score=0.74

### src/analytics/knowledge_base.cpp
Total findings: 3

- Line 148: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(f);
  Confidence: band=high; score=0.74
- Line 153: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(it->second);
  Confidence: band=high; score=0.74
- Line 245: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: tp.object += "," + parts[i];
  Confidence: band=high; score=0.74

### src/analytics/lora_pattern_classifier.cpp
Total findings: 3

- Line 190: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: numeric_values.push_back(*d);
  Confidence: band=high; score=0.74
- Line 190: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: numeric_values.push_back(*d);
  Confidence: band=high; score=0.74
- Line 345: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(
  Confidence: band=high; score=0.74

### src/analytics/jit_aggregation.cpp
Total findings: 2

- Line 299: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: key_order.push_back(key);
  Confidence: band=high; score=0.74
- Line 331: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> first_row;
  Confidence: band=medium; score=0.66

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
