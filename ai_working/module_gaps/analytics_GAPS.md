# analytics Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: analytics
- Generated: 2026-06-02 12:40:50
- Status: Critical Findings Present
- Total Findings: 1176
- Actionable Findings (Critical + High): 483
- Affected Files: 24

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 133 |
| High | 350 |
| Medium | 688 |
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
| src/analytics/process_mining.cpp | 159 | 0 | 14 | 145 | 0 |
| src/analytics/olap.cpp | 150 | 9 | 42 | 99 | 0 |
| src/analytics/forecasting.cpp | 110 | 43 | 42 | 23 | 2 |
| src/analytics/distributed_analytics.cpp | 103 | 28 | 57 | 18 | 0 |
| src/analytics/cep_engine.cpp | 99 | 2 | 22 | 75 | 0 |
| src/analytics/automl.cpp | 77 | 9 | 13 | 54 | 1 |
| src/analytics/columnar_execution.cpp | 60 | 1 | 32 | 27 | 0 |
| src/analytics/streaming_window.cpp | 53 | 9 | 18 | 26 | 0 |
| src/analytics/streaming_join.cpp | 49 | 0 | 10 | 39 | 0 |
| src/analytics/anomaly_detection.cpp | 42 | 7 | 9 | 25 | 1 |
| src/analytics/nlp_text_analyzer.cpp | 38 | 8 | 4 | 25 | 1 |
| src/analytics/ml_serving.cpp | 34 | 0 | 20 | 14 | 0 |
| src/analytics/diff_engine.cpp | 29 | 1 | 13 | 15 | 0 |
| src/analytics/arrow_flight.cpp | 23 | 3 | 3 | 17 | 0 |
| src/analytics/llm_process_analyzer.cpp | 23 | 3 | 8 | 12 | 0 |
| src/analytics/model_serving.cpp | 22 | 4 | 14 | 4 | 0 |
| src/analytics/incremental_view.cpp | 18 | 0 | 10 | 8 | 0 |
| src/analytics/process_pattern_matcher.cpp | 17 | 0 | 1 | 16 | 0 |
| src/analytics/expert_system_engine.cpp | 16 | 1 | 2 | 13 | 0 |
| src/analytics/jit_aggregation.cpp | 14 | 4 | 6 | 4 | 0 |
| src/analytics/arrow_export.cpp | 13 | 0 | 4 | 9 | 0 |
| src/analytics/analytics_export.cpp | 12 | 1 | 6 | 5 | 0 |
| src/analytics/lora_pattern_classifier.cpp | 8 | 0 | 0 | 8 | 0 |
| src/analytics/knowledge_base.cpp | 7 | 0 | 0 | 7 | 0 |

## Full Scanner Findings

### src/analytics/process_mining.cpp
Total findings: 159

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4626 fix(analytics): replace who... (2026-04-13) | #4402 [WIP] Add automated
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
- Line 1049: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (ids.find(e.activity) == ids.end()) {
- Line 1049: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (ids.find(e.activity) == ids.end()) {
- Line 1050: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (ids.find(e.activity) == ids.end()) {
  Confidence: band=very_high; score=0.9
- Line 1809: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: tokens.erase(tokens.find(token));
  Confidence: band=very_high; score=0.9
- Line 2275: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(modelOrder.begin(), modelOrder.end(), act) == modelOrder.end()) {
- Line 2337: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: m.type     = (syncCost == 0.0) ? "sync" : "log+model";
  Confidence: band=very_high; score=0.9
- Line 2510: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (const auto &[freq, data] : freq_sorted) {
- Line 222: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<ProcessEvent>> cases;
  Confidence: band=high; score=0.74
- Line 299: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
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
- Line 420: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
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
- Line 534: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
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
- Line 1695: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: actSeq.push_back(e.activity);
- Line 1707: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: v.case_ids.push_back(trace.case_id);
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
- Line 1824: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.deviations.push_back("Case " + trace.case_id + ": missing token for activity '" + activity
- Line 1857: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.deviations.push_back("Case " + trace.case_id + ": ended without reaching end node");
  Confidence: band=high; score=0.74
- Line 1857: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.deviations.push_back("Case " + trace.case_id + ": ended without reaching end node");
  Confidence: band=high; score=0.74
- Line 1858: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.deviations.push_back("Case " + trace.case_id + ": ended without reaching end node");
- Line 1891: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << R"(<definitions xmlns="http://www.omg.org/spec/BPMN/20100524/MODEL")" << "\n";
- Line 1891: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << R"(<definitions xmlns="http://www.omg.org/spec/BPMN/20100524/MODEL")" << "\n";
- Line 1892: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << R"(             xmlns:bpmndi="http://www.omg.org/spec/BPMN/20100524/DI")" << "\n";
- Line 1892: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << R"(             xmlns:bpmndi="http://www.omg.org/spec/BPMN/20100524/DI")" << "\n";
- Line 1893: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << R"(             targetNamespace="http://themis.db/process">)" << "\n";
- Line 1893: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << R"(             targetNamespace="http://themis.db/process">)" << "\n";
- Line 1916: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "    <" << element << R"( id=")" << node.id << R"(" name=")" << node.name << R"("/>)" << "\n"
- Line 1916: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "    <" << element << R"( id=")" << node.id << R"(" name=")" << node.name << R"("/>)" << "\n"
- Line 1922: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << edge.to << R"("/>)" << "\n";
- Line 1922: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << edge.to << R"("/>)" << "\n";
- Line 1925: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "  </process>\n";
- Line 1926: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "</definitions>\n";
- Line 2009: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: embedding.push_back(normalized);
  Confidence: band=high; score=0.74
- Line 2009: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: embedding.push_back(normalized);
  Confidence: band=high; score=0.74
- Line 2043: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: info.trace_indices.push_back(static_cast<int>(i));
- Line 2046: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: info.activities.push_back(ev.activity);
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
- Line 2451: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "<pnml xmlns=\"http://www.pnml.org/version-1-0/pnml\">\n";
- Line 2451: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "<pnml xmlns=\"http://www.pnml.org/version-1-0/pnml\">\n";
- Line 2452: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "  <net id=\"net1\" type=\"http://www.pnml.org/version-1-0/ptnet\">\n";
- Line 2452: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "  <net id=\"net1\" type=\"http://www.pnml.org/version-1-0/ptnet\">\n";
- Line 2452: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "  <net id=\"net1\" type=\"http://www.pnml.org/version-1-0/ptnet\">\n";
- Line 2453: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "    <name><text>" << model.name << "</text></name>\n";
- Line 2458: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "      <name><text>" << node.name << "</text></name>\n";
- Line 2459: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "    </place>\n";
- Line 2465: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "      <name><text>" << edge.from << " -> " << edge.to << "</text></name>\n";
- Line 2466: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "    </transition>\n";
- Line 2471: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "    <arc id=\"arc_" << edge.id << "\" source=\"" << edge.from << "\" target=\"" << edge.to <
- Line 2471: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "    <arc id=\"arc_" << edge.id << "\" source=\"" << edge.from << "\" target=\"" << edge.to <
- Line 2471: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "    <arc id=\"arc_" << edge.id << "\" source=\"" << edge.from << "\" target=\"" << edge.to <
- Line 2471: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "    <arc id=\"arc_" << edge.id << "\" source=\"" << edge.from << "\" target=\"" << edge.to <
- Line 2474: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "  </net>\n";
- Line 2475: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: xml << "</pnml>\n";
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
- Line 2490: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: activities.push_back(event.activity);
- Line 2496: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pattern_info[window].second.push_back(trace.case_id);
  Confidence: band=high; score=0.74
- Line 2497: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pattern_info[window].second.push_back(trace.case_id);
- Line 2503: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: freq_sorted.push_back({info.first, {seq, info.second}});
  Confidence: band=high; score=0.74
- Line 2504: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: freq_sorted.push_back({info.first, {seq, info.second}});
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

### src/analytics/olap.cpp
Total findings: 150

- Line 188: severity=CRITICAL; category=no_timeout
  Description: thread_join without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: cleanup_thread.join();
- Line 296: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const size_t max_entries = impl_->config.result_cache_max_entries;
- Line 297: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const int64_t ttl_ms     = impl_->config.result_cache_ttl_ms;
- Line 408: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator fieldIt may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto fieldIt = row.find(dim.name);
- Line 926: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto agg_result = impl_->gpu_accelerator->aggregate(gpu_rows, *gpu_func, value_fn);
- Line 1192: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = row.find(name);
- Line 1555: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->cached_result  = engine.execute(query);
- Line 1569: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->cached_result  = impl_->buildResult(definition_.dimensions, definition_.measures);
- Line 1581: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: OLAPResult result = impl_->cached_result;
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4929 [Docs][analytics] Refresh m... (2026-05-10) | #4626 fix(analytics): rep
- Line 79: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: * Thread safety: OLAPEngine is fully thread-safe; each execute* call acquires a
- Line 172: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(result_cache_mutex);
- Line 293: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: OLAPResult OLAPEngine::execute(const OLAPQuery &query) {
- Line 293: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: OLAPResult OLAPEngine::execute(const OLAPQuery &query) {
  Confidence: band=very_high; score=0.9
- Line 407: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto fieldIt = row.find(dim.name);
- Line 407: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto fieldIt = row.find(dim.name);
- Line 407: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto fieldIt = row.find(dim.name);
- Line 407: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto fieldIt = row.find(dim.name);
- Line 408: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto fieldIt = row.find(dim.name);
  Confidence: band=very_high; score=0.9
- Line 425: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto fieldIt = row.find(measure.field);
- Line 469: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto aIt = a.values.find(sort.field);
- Line 469: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto aIt = a.values.find(sort.field);
- Line 470: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto bIt = b.values.find(sort.field);
- Line 491: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (aVal != bVal) {
  Confidence: band=very_high; score=0.9
- Line 597: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: row.values[query.dimensions[i].name] = nullptr;
- Line 667: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = row.values.find(dim.name);
- Line 668: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = row.values.find(dim.name);
  Confidence: band=very_high; score=0.9
- Line 678: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = row.values.find(measure.name);
- Line 710: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto levelIt = row.values.find("_level");
- Line 718: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = row.values.find(dim.name);
- Line 776: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = data[j].find(measure.field);
- Line 776: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = data[j].find(measure.field);
- Line 1191: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = row.find(name);
- Line 1191: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = row.find(name);
- Line 1192: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = row.find(name);
  Confidence: band=very_high; score=0.9
- Line 1450: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = row.find(d.name);
- Line 1485: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it     = row.find(m.field);
- Line 1493: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto git = groups.find(gk);
- Line 1529: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = agg_map.find(m.name);
- Line 1530: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = agg_map.find(m.name);
  Confidence: band=very_high; score=0.9
- Line 1555: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: impl_->cached_result  = engine.execute(query);
- Line 1574: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: OLAPResult MaterializedView::query(const std::vector<Filter> &filters, const std::vector<Sort> &sort
- Line 1780: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (aVal != bVal) {
  Confidence: band=very_high; score=0.9
- Line 1835: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = row.values.find(col_name);
- Line 1836: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = row.values.find(col_name);
  Confidence: band=very_high; score=0.9
- Line 1890: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = row.values.find(col_name);
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
- Line 255: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: fstr += "null";
- Line 266: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: fstr += ',';
  Confidence: band=high; score=0.74
- Line 267: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: fstr += ',';
- Line 267: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: fstr += ',';
- Line 271: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filter_strs.push_back(std::move(fstr));
  Confidence: band=high; score=0.74
- Line 389: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.columns.push_back(dim.name);
  Confidence: band=high; score=0.74
- Line 390: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.columns.push_back(dim.name);
- Line 392: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.columns.push_back(measure.name);
  Confidence: band=high; score=0.74
- Line 393: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.columns.push_back(measure.name);
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
- Line 411: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: groupKey.push_back(*s);
- Line 413: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: groupKey.push_back(std::to_string(*i));
- Line 415: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: groupKey.push_back(std::to_string(*d));
- Line 417: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: groupKey.push_back("");
- Line 420: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: groupKey.push_back("");
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
- Line 453: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: measureValues.push_back(values[i]);
- Line 518: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: OLAPResult OLAPEngine::executeCubeQuery(const OLAPQuery &query) {
  Confidence: band=high; score=0.74
- Line 527: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.columns.push_back(dim.name);
  Confidence: band=high; score=0.74
- Line 528: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.columns.push_back(dim.name);
- Line 530: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.columns.push_back(measure.name);
  Confidence: band=high; score=0.74
- Line 531: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.columns.push_back(measure.name);
- Line 533: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.columns.push_back("_grouping_id");
- Line 542: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: subQuery.dimensions.push_back(query.dimensions[i]);
  Confidence: band=high; score=0.74
- Line 542: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: subQuery.dimensions.push_back(query.dimensions[i]);
  Confidence: band=high; score=0.74
- Line 543: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: subQuery.dimensions.push_back(query.dimensions[i]);
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
- Line 578: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.columns.push_back(measure.name);
- Line 580: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.columns.push_back("_level");
- Line 588: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: subQuery.dimensions.push_back(query.dimensions[i]);
  Confidence: band=high; score=0.74
- Line 588: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: subQuery.dimensions.push_back(query.dimensions[i]);
  Confidence: band=high; score=0.74
- Line 589: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: subQuery.dimensions.push_back(query.dimensions[i]);
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
- Line 722: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rollupRow.dimension_values.push_back(std::nullopt);
- Line 724: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rollupRow.dimension_values.push_back(*s);
- Line 726: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rollupRow.dimension_values.push_back(std::nullopt);
- Line 729: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rollupRow.dimension_values.push_back(std::nullopt);
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
- Line 779: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: windowValues.push_back(it->second);
- Line 820: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan.optimization_notes.push_back("Full table scan required (no filters)");
- Line 822: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan.optimization_notes.push_back("Filter pushdown applied");
- Line 828: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan.optimization_notes.push_back("CUBE will generate " + std::to_string(combinations)
- Line 832: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: plan.optimization_notes.push_back("ROLLUP will generate " + std::to_string(query.dimensions.size() +
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
- Line 1461: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: key += '\0';
- Line 1513: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.columns.push_back(d.name);
  Confidence: band=high; score=0.74
- Line 1513: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.columns.push_back(d.name);
  Confidence: band=high; score=0.74
- Line 1514: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: r.columns.push_back(d.name);
- Line 1516: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.columns.push_back(m.name);
  Confidence: band=high; score=0.74
- Line 1517: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: r.columns.push_back(m.name);
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
- Line 2024: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2042: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/analytics/forecasting.cpp
Total findings: 110

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['                        // partial derivative w.r.t. delta[ci]', '                        double dt = t_norm[i] - p.changepoints_t[static_cast<size_t>(ci)];', '                        grad += 2.0 * err * dt / static_cast<double>(n);', '                    }', '                }']
  Confidence: band=very_high; score=0.9
- Line 466: severity=CRITICAL; category=missing_dtor
  Description: Class HoltWintersParams allocates resources but has no destructor
  Remediation: Add explicit destructor: ~HoltWintersParams() { /* cleanup */ }
  Context: class/struct HoltWintersParams
- Line 1700: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (impl_->cache_valid && impl_->cache_key == ck) {
- Line 1702: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->linear_p       = impl_->cache_entry.linear_p;
- Line 1703: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->ses_p          = impl_->cache_entry.ses_p;
- Line 1704: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->hw_p           = impl_->cache_entry.hw_p;
- Line 1705: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->arima_p        = impl_->cache_entry.arima_p;
- Line 1706: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->in_sample_rmse = impl_->cache_entry.in_sample_rmse;
- Line 1707: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->config         = impl_->cache_entry.config;
- Line 1708: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->lin_sx         = impl_->cache_entry.lin_sx;
- Line 1709: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->lin_sy         = impl_->cache_entry.lin_sy;
- Line 1710: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->lin_sxx        = impl_->cache_entry.lin_sxx;
- Line 1711: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->lin_sxy        = impl_->cache_entry.lin_sxy;
- Line 1712: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->lin_n          = impl_->cache_entry.lin_n;
- Line 1768: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto preds = impl_->predict(static_cast<int>(impl_->train_y.size()) - 1);
- Line 1774: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->in_sample_rmse = preds.empty() ? 0.0 : std::sqrt(ss / static_cast<double>(preds.size()));
- Line 1777: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->cache_entry.linear_p       = impl_->linear_p;
- Line 1778: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->cache_entry.ses_p          = impl_->ses_p;
- Line 1779: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->cache_entry.hw_p           = impl_->hw_p;
- Line 1780: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->cache_entry.arima_p        = impl_->arima_p;
- Line 1781: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->cache_entry.in_sample_rmse = impl_->in_sample_rmse;
- Line 1782: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->cache_entry.config         = impl_->config;
- Line 1783: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->cache_entry.lin_sx         = impl_->lin_sx;
- Line 1784: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->cache_entry.lin_sy         = impl_->lin_sy;
- Line 1785: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->cache_entry.lin_sxx        = impl_->lin_sxx;
- Line 1786: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->cache_entry.lin_sxy        = impl_->lin_sxy;
- Line 1787: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->cache_entry.lin_n          = impl_->lin_n;
- Line 1788: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->cache_key                  = ck;
- Line 1789: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->cache_valid                = true;
- Line 1861: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->cache_valid = false;
- Line 1873: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: double x_new = static_cast<double>(impl_->lin_n); // next 0-based index
- Line 1879: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: double dn    = static_cast<double>(impl_->lin_n);
- Line 1922: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: int n_prev      = static_cast<int>(impl_->train_y.size()) - 1; // index before this obs
- Line 2010: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: int m = (impl_->config.seasonality >= 2) ? impl_->config.seasonality : static_cast<int>(std::max(siz
- Line 2060: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: oss << "method=" << static_cast<int>(impl_->method) << "\n";
- Line 2204: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: model.impl_->method                = static_cast<ForecastMethod>(readI("method"));
- Line 2235: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: model.impl_->hw_p.S[static_cast<size_t>(i)] = readD("hw_S_" + std::to_string(i));
- Line 2245: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: model.impl_->arima_p.ar_coeffs[static_cast<size_t>(i)] = readD("ar_c_" + std::to_string(i));
- Line 2250: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: model.impl_->arima_p.ma_coeffs[static_cast<size_t>(i)] = readD("ma_c_" + std::to_string(i));
- Line 2255: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: model.impl_->arima_p.last_window[static_cast<size_t>(i)] = readD("ar_w_" + std::to_string(i));
- Line 2260: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: model.impl_->arima_p.last_resid[static_cast<size_t>(i)] = readD("ar_r_" + std::to_string(i));
- Line 2266: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: model.impl_->train_ts[static_cast<size_t>(i)] = readL("ts_" + std::to_string(i));
- Line 2342: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: mi.training_points = impl_->train_y.size();
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
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4929 [Docs][analytics] Refresh m... (2026-05-10) | #4317 feat(analytics): SI
- Line 534: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (base == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 562: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: Snew         = gamma * (y[i] / (Lnew != 0.0 ? Lnew : 1e-10)) + (1.0 - gamma) * S[static_cast<size_t>(si)];
  Confidence: band=very_high; score=0.9
- Line 1084: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: std::vector<double> fourier_weekly; ///< 2*fourier_order_weekly coeffs [a1,b1,a2,b2,...]
- Line 1239: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (a != b) {
  Confidence: band=very_high; score=0.9
- Line 1436: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: k.data_hash = fnv1a64(y.data(), y.size() * sizeof(double));
- Line 1437: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: k.data_hash = fnv1a64(ts.data(), ts.size() * sizeof(int64_t)) ^ k.data_hash;
- Line 1439: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: uint64_t cfg_h = fnv1a64(&cfg.alpha, sizeof(cfg.alpha));
- Line 1440: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: cfg_h ^= fnv1a64(&cfg.beta, sizeof(cfg.beta));
- Line 1441: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: cfg_h ^= fnv1a64(&cfg.gamma, sizeof(cfg.gamma));
- Line 1442: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: cfg_h ^= fnv1a64(&cfg.seasonality, sizeof(cfg.seasonality));
- Line 1443: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: cfg_h ^= fnv1a64(&cfg.ar_order, sizeof(cfg.ar_order));
- Line 1444: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: cfg_h ^= fnv1a64(&cfg.diff_order, sizeof(cfg.diff_order));
- Line 1445: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: cfg_h ^= fnv1a64(&cfg.ma_order, sizeof(cfg.ma_order));
- Line 2032: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: season_acc[static_cast<size_t>(si)] += multiplicative ? (base != 0.0 ? y[i] / base : 1.0) : (y[i] - base);
  Confidence: band=very_high; score=0.9
- Line 2301: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: sp.seasonal_buffer[static_cast<size_t>(i)] = readD("sarima_sb_" + std::to_string(i));
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
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
- Line 833: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ar_lags.push_back(i * params.m);
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
- Line 938: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ma_lags.push_back(i * params.m);
- Line 1053: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: window.push_back(pred_diff - p.mean_diff);
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
- Line 1581: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: window.push_back(pred_diff - arima_p.mean_diff);
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

### src/analytics/distributed_analytics.cpp
Total findings: 103

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['        double delta = other_mean - mean;', '        mean         = (count * mean + other_count * other_mean) / total;', '        m2 += other_m2 + delta * delta * count * other_count / total;', '        count = total;', '    }']
  Confidence: band=very_high; score=0.9
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
- Line 0: severity=HIGH; category=uncategorized
  Context: ['            return;', '        }', '        double total = count + other_count;', '        double delta = other_mean - mean;', '        mean         = (count * mean + other_count * other_mean) / total;']
  Confidence: band=high; score=0.81
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4929 [Docs][analytics] Refresh m... (2026-05-10) | #4324 Implement cached he
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
- Line 355: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (!stopping_.load(std::memory_order_acquire)) {
- Line 358: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(health_monitor_mutex_);
- Line 360: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: [this] { return stopping_.load(std::memory_order_acquire); });
- Line 363: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (stopping_.load(std::memory_order_acquire)) {
- Line 491: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = row.values.find(dim.name);
- Line 491: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = row.values.find(dim.name);
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
- Line 545: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = groups.find(key);
- Line 545: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = groups.find(key);
- Line 545: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = groups.find(key);
- Line 546: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = groups.find(key);
  Confidence: band=very_high; score=0.9
- Line 554: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto vit = row.values.find(dim.name);
- Line 554: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto vit = row.values.find(dim.name);
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
- Line 735: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto partial = entry.executor->execute(entry.shard_id, query);
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
- Line 831: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: OLAPResult DistributedAnalyticsSharding::execute(const OLAPQuery &query) {
- Line 831: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: OLAPResult DistributedAnalyticsSharding::execute(const OLAPQuery &query) {
  Confidence: band=very_high; score=0.9
- Line 832: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return executeDistributed(query).merged;
  Confidence: band=very_high; score=0.9
- Line 383: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 402: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
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
- Line 751: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
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

### src/analytics/cep_engine.cpp
Total findings: 99

- Line 1095: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = session_windows_.find(key);
- Line 1710: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto agg_results = state.aggregator->getResults();
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
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4339 Analytics module: stats.h u... (2026-03-19) | #4311 Implement memory po
- Line 63: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::snprintf(buf, sizeof(buf), "%08x-%04x-%04x-%04x-%012llx", static_cast<unsigned>(a >> 32),
- Line 769: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (seen.find(et) == seen.end()) {
- Line 769: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (seen.find(et) == seen.end()) {
- Line 1225: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock lk(timer_mutex_);
- Line 1842: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (; it != std::sregex_iterator(); ++it) {
- Line 2096: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (; pit != std::sregex_iterator(); ++pit) {
- Line 2316: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return (it != streams_.end()) ? it->second : nullptr;
- Line 2685: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto &entry : std::filesystem::directory_iterator(cp_dir)) {
- Line 2701: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock lk(mutex_);
- Line 2702: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: cv_.wait_for(lk, std::chrono::milliseconds(100),
- Line 2720: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (result == EventStream::PushResult::DROPPED) {
  Confidence: band=very_high; score=0.9
- Line 2744: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock lk(metrics_mutex_);
- Line 184: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({t, word});
- Line 196: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { num_val = std::stod(ns); } catch (...) {}
- Line 197: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({TokType::NUMBER, ns, num_val});
- Line 205: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({TokType::STRING, expr.substr(start, i - start)});
- Line 210: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({TokType::LPAREN, "("});
- Line 213: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({TokType::RPAREN, ")"});
- Line 216: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({TokType::EQ, "=="});
- Line 220: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({TokType::NEQ, "!="});
- Line 339: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { lhs_num = std::stod(lhs); lhs_is_num = true; } catch (...) {}
- Line 370: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 423: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 440: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: partitions_.emplace_back(std::make_unique<Partition>());
  Confidence: band=high; score=0.74
- Line 458: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { cb(event); } catch (...) {}
- Line 596: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: s.transitions.push_back(i + 1);
  Confidence: band=high; score=0.74
- Line 597: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: s.transitions.push_back(i + 1);
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
- Line 932: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { continue; }
- Line 1039: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { spdlog::warn("CEPEngine: window callback threw unknown exception"); }
- Line 1061: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: w.events.push_back(event);
  Confidence: band=high; score=0.74
- Line 1070: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batches.push_back(std::move(*b));
  Confidence: band=high; score=0.74
- Line 1084: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { spdlog::warn("CEPEngine: window callback threw unknown exception"); }
- Line 1099: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: w.events.push_back(event);
  Confidence: band=high; score=0.74
- Line 1123: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { spdlog::warn("CEPEngine: window callback threw unknown exception"); }
- Line 1153: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { spdlog::warn("CEPEngine: window callback threw unknown exception"); }
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
- Line 1243: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batches.push_back({w.events, w.start, now});
- Line 1250: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { spdlog::warn("CEPEngine: window callback threw unknown exception"); }
- Line 1264: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batches.push_back(std::move(*b));
  Confidence: band=high; score=0.74
- Line 1264: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batches.push_back(std::move(*b));
  Confidence: band=high; score=0.74
- Line 1273: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { spdlog::warn("CEPEngine: window callback threw unknown exception"); }
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
- Line 1770: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: norm += ' ';
- Line 1789: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { val = std::stoull(val_str); } catch (...) { return 0; }
- Line 1933: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { pc.within = std::chrono::milliseconds(timeToMs(m[3], unit)); } catch (...) {}
- Line 1964: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { wc.count = std::stoull(m[2]); } catch (...) {}
- Line 2002: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { wc.count = std::stoull(m[2]); } catch (...) {}
- Line 2009: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 2018: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 2027: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
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
- Line 2501: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { alert_callback_(alert); } catch (...) {}
- Line 2686: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry.path().stem().string());
  Confidence: band=high; score=0.74
- Line 2687: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(entry.path().stem().string());

### src/analytics/automl.cpp
Total findings: 77

- Line 313: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = index.find(l);
- Line 1293: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: gb->base_value = y_reg.empty() ? 0.0 : mean_y / static_cast<double>(n);
- Line 1578: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: m[impl_->label_enc.decode(static_cast<int>(c))] = probs[c];
- Line 1878: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ss << "task=" << static_cast<int>(impl_->task) << "\n";
- Line 1879: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ss << "algorithm=" << static_cast<int>(impl_->algo) << "\n";
- Line 1904: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: m.impl_->task = static_cast<AutoMLTask>(std::stoi(val));
- Line 1906: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: m.impl_->algo = static_cast<ModelAlgorithm>(std::stoi(val));
- Line 1922: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: for (int i = 0; i < static_cast<int>(m.impl_->label_enc.classes.size()); ++i) {
- Line 1923: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: m.impl_->label_enc.index[m.impl_->label_enc.classes[static_cast<size_t>(i)]] = i;
- Line 145: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = p.fields.find(fm.names[j]);
- Line 145: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = p.fields.find(fm.names[j]);
- Line 145: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = p.fields.find(fm.names[j]);
- Line 145: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = p.fields.find(fm.names[j]);
- Line 146: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = p.fields.find(fm.names[j]);
  Confidence: band=very_high; score=0.9
- Line 166: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = p.fields.find(target);
- Line 196: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = p.fields.find(target);
- Line 312: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = index.find(l);
- Line 312: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = index.find(l);
- Line 860: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (t == p) {
  Confidence: band=very_high; score=0.9
- Line 1119: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: for (auto [d2, i] : nbrs) {
- Line 1134: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: for (auto [d2, i] : nbrs) {
- Line 1525: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = p.fields.find(base);
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
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
- Line 933: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: train.push_back(idx[i]);
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
- Line 1215: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: copy->members.push_back(m->clone());
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
- Line 1645: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: tf += ", ";
- Line 1714: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "\\\"";
  Confidence: band=high; score=0.74
- Line 1715: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\\"";
- Line 1717: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\\\";
- Line 1719: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\n";
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
- Line 2018: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: yrt.push_back(y_reg[i]);
- Line 2021: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: Xv.push_back(X[i]);
  Confidence: band=high; score=0.74
- Line 2022: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: Xv.push_back(X[i]);
- Line 2024: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ycv.push_back(y_cls[i]);
- Line 2026: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: yrv.push_back(y_reg[i]);
- Line 2233: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ens->members.push_back(std::move(m));
  Confidence: band=high; score=0.74
- Line 1325: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: gb->base_value = std::log(mean_p / (1.0 - mean_p));
  Confidence: band=medium; score=0.6

### src/analytics/columnar_execution.cpp
Total findings: 60

- Line 1030: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto out_col = std::make_shared<Column>(gc, src->type());
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4929 [Docs][analytics] Refresh m... (2026-05-10) | #4317 feat(analytics): SI
- Line 265: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: out->appendBool(bool_data_[row], is_null);
- Line 289: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: out->appendDouble(double_data_[row], is_null);
- Line 292: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: out->appendString(string_data_[row], is_null);
- Line 295: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: out->appendBool(bool_data_[row], is_null);
- Line 592: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ColumnBatch FilterOperator::execute(const ColumnBatch &input) const {
- Line 592: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: ColumnBatch FilterOperator::execute(const ColumnBatch &input) const {
  Confidence: band=very_high; score=0.9
- Line 627: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: ColumnBatch ProjectOperator::execute(const ColumnBatch &input) const {
  Confidence: band=very_high; score=0.9
- Line 745: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: r.sum += data[i];
- Line 901: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ColumnBatch AggregateOperator::execute(const ColumnBatch &input) const {
- Line 901: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: ColumnBatch AggregateOperator::execute(const ColumnBatch &input) const {
  Confidence: band=very_high; score=0.9
- Line 1085: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ColumnBatch SortOperator::execute(const ColumnBatch &input) const {
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
- Line 1211: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: current = stage.filter->execute(current);
- Line 1215: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: current = stage.project->execute(current);
- Line 1219: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: current = stage.aggregate->execute(current);
- Line 1223: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: current = stage.sort->execute(current);
- Line 1238: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ColumnBatch ColumnarExecutionEngine::execute(const ColumnBatch &input, const VectorizedPipeline &pip
- Line 1238: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: ColumnBatch ColumnarExecutionEngine::execute(const ColumnBatch &input, const VectorizedPipeline &pipeline) {
  Confidence: band=very_high; score=0.9
- Line 1244: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ColumnBatch result = pipeline.execute(input);
- Line 1259: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: results.push_back(execute(batch, pipeline));
- Line 1267: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return execute(input, p);
- Line 1273: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return execute(input, p);
- Line 1279: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return execute(input, p);
- Line 1285: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return execute(input, p);
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
- Line 497: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sel.push_back(static_cast<uint32_t>(i));
- Line 504: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sel.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 505: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sel.push_back(static_cast<uint32_t>(i));
- Line 519: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sel.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 520: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sel.push_back(static_cast<uint32_t>(i));
- Line 527: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sel.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 528: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sel.push_back(static_cast<uint32_t>(i));
- Line 546: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sel.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 547: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sel.push_back(static_cast<uint32_t>(i));
- Line 561: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sel.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 562: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sel.push_back(static_cast<uint32_t>(i));
- Line 578: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sel.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 579: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sel.push_back(static_cast<uint32_t>(i));
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
- Line 994: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: key_order.push_back(it->first); // reference key already in the map
- Line 1085: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: ColumnBatch SortOperator::execute(const ColumnBatch &input) const {
  Confidence: band=high; score=0.74
- Line 1157: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sel.push_back(static_cast<uint32_t>(idx));
  Confidence: band=high; score=0.74
- Line 1158: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sel.push_back(static_cast<uint32_t>(idx));
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

### src/analytics/streaming_window.cpp
Total findings: 53

- Line 1224: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pipeline->config_    = config_;
- Line 1225: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pipeline->agg_specs_ = agg_specs_;
- Line 1233: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pipeline->tumbling_ = std::make_shared<TumblingWindow>(cfg);
- Line 1233: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pipeline->tumbling_ = std::make_shared<TumblingWindow>(cfg);
- Line 1247: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pipeline->sliding_ = std::make_shared<SlidingWindow>(cfg);
- Line 1247: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pipeline->sliding_ = std::make_shared<SlidingWindow>(cfg);
- Line 1261: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pipeline->session_                   = std::make_shared<SessionWindow>(cfg);
- Line 1275: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pipeline->hopping_ = std::make_shared<HoppingWindow>(cfg);
- Line 1275: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pipeline->hopping_ = std::make_shared<HoppingWindow>(cfg);
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
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4929 [Docs][analytics] Refresh m... (2026-05-10) | #4339 Analytics module: s
- Line 113: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: uint64_t c = counter.fetch_add(1, std::memory_order_relaxed);
- Line 122: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::snprintf(buf, sizeof(buf), "%08x-%04x-%04x-%04x-%012llx", static_cast<unsigned>(a >> 32),
- Line 202: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = rec.fields.find(spec.field);
- Line 203: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = rec.fields.find(spec.field);
  Confidence: band=very_high; score=0.9
- Line 504: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock lk(idle_mutex_);
- Line 525: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: int64_t wm = watermark_us_.load(std::memory_order_acquire);
- Line 746: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock lk(idle_mutex_);
- Line 767: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: int64_t wm = watermark_us_.load(std::memory_order_acquire);
- Line 851: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: int64_t wm = watermark_us_.load(std::memory_order_acquire);
- Line 959: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock lk(expiry_mutex_);
- Line 1109: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: int64_t wm    = watermark_us_.load(std::memory_order_acquire);
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
- Line 475: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { cb(r); } catch (...) {}
- Line 491: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { cb(r); } catch (...) {}
- Line 536: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { cb(r); } catch (...) {}
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
- Line 717: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { cb(r); } catch (...) {}
- Line 733: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { cb(r); } catch (...) {}
- Line 778: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { cb(r); } catch (...) {}
- Line 923: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { cb(pending_result); } catch (...) {}
- Line 934: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pending.push_back(computeResult(s, s.has_late_records));
  Confidence: band=high; score=0.74
- Line 935: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pending.push_back(computeResult(s, s.has_late_records));
- Line 946: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { cb(r); } catch (...) {}
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
- Line 981: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pending.push_back(computeResult(s, s.has_late_records));
- Line 991: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { cb(r); } catch (...) {}
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
- Line 1140: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { cb(r); } catch (...) {}
- Line 1156: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { cb(r); } catch (...) {}

### src/analytics/streaming_join.cpp
Total findings: 49

- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4929 [Docs][analytics] Refresh m... (2026-05-10)
- Line 335: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: auto it   = std::find(build_column_names_.begin(), build_column_names_.end(), n);
- Line 344: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it  = hash_table_.find(key);
- Line 368: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto bit   = std::find(build_column_names_.begin(), build_column_names_.end(), n);
  Confidence: band=very_high; score=0.9
- Line 368: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: auto bit   = std::find(build_column_names_.begin(), build_column_names_.end(), n);
- Line 368: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: auto bit   = std::find(build_column_names_.begin(), build_column_names_.end(), n);
- Line 575: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: int64_t probe_ts = probe_time_data[r];
- Line 600: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (auto it = it_lo; it != build_buffer_.end() && it->timestamp_ms <= hi; ++it) {
- Line 60: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: return std::string("\x00\x00", 2); // null sentinel
- Line 80: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: s += '\x01'; // terminator
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
- Line 120: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: key += '\xFF'; // separator
- Line 120: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: key += '\xFF'; // separator
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
- Line 221: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: build_columns_.push_back(std::make_shared<Column>(col->name(), col->type()));
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
- Line 487: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: probe_cols.push_back(probe_batch.getColumnAt(i));
- Line 488: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: probe_col_names.push_back(probe_cols.back()->name());
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
- Line 596: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: probe_key += '\xFF';
- Line 605: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: build_key += '\xFF';
  Confidence: band=high; score=0.74
- Line 605: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: build_key += '\xFF';
  Confidence: band=high; score=0.74
- Line 606: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: build_key += '\xFF';
- Line 606: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: build_key += '\xFF';
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

### src/analytics/anomaly_detection.cpp
Total findings: 42

- Line 222: severity=CRITICAL; category=missing_dtor
  Description: Class IFNode allocates resources but has no destructor
  Remediation: Add explicit destructor: ~IFNode() { /* cleanup */ }
  Context: class/struct IFNode
- Line 230: severity=CRITICAL; category=missing_dtor
  Description: Class ITree allocates resources but has no destructor
  Remediation: Add explicit destructor: ~ITree() { /* cleanup */ }
  Context: class/struct ITree
- Line 1009: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ss << "method=" << static_cast<int>(impl_->cfg.method) << "\n";
- Line 1077: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: det.impl_->cfg.method = static_cast<AnomalyMethod>(std::stoi(val));
- Line 1085: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: det.impl_->n_features = static_cast<size_t>(std::stoul(val));
- Line 1130: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: s.training_samples = impl_->training_samples_count;
- Line 1158: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: retrain_future_.wait();
- Line 16: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: *                      mapped to [0,1] via a logistic squashing function.
- Line 438: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = p.fields.find(name);
- Line 625: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: means[f]      = computeMean(col_data);
- Line 626: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: stddevs[f]    = computeStddev(col_data, means[f]);
- Line 627: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: medians[f]    = computeMedian(col_data);
- Line 628: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: mads[f]       = computeMAD(col_data, medians[f]);
- Line 631: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: computeQuartiles(col_data, q1[f], q3[f]);
- Line 1062: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& t : splitComma(s)) {
- Line 1234: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: && retrain_future_.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 62: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(*d);
  Confidence: band=high; score=0.74
- Line 63: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(*d);
- Line 65: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<double>(*i));
- Line 67: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(*b ? 1.0 : 0.0);
- Line 77: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(name);
  Confidence: band=high; score=0.74
- Line 198: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(row[col]);
  Confidence: band=high; score=0.74
- Line 255: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stack.push_back({indices, height, -1, 0});
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
- Line 1063: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: try { v.push_back(std::stod(t)); } catch (...) {}
- Line 1063: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { v.push_back(std::stod(t)); } catch (...) {}
- Line 1110: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { /* skip malformed line */ }
- Line 1169: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::optional<AnomalyResult> StreamingAnomalyDetector::process(const DataPoint &point) {
  Confidence: band=high; score=0.74
- Line 1217: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 1253: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 1269: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 218: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double H = std::log(n - 1.0) + 0.5772156649; // harmonic number approximation
  Confidence: band=medium; score=0.6

### src/analytics/nlp_text_analyzer.cpp
Total findings: 38

- Line 1074: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto ends_with = [&](std::string_view suffix, size_t min_stem) -> bool {
- Line 1077: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto strip = [&](size_t n, std::string_view add = "") -> std::string {
- Line 1162: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto bends = [&](std::string_view suffix, size_t min_stem) -> bool {
- Line 1165: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto bstrip = [&](size_t n, std::string_view add = "") -> std::string {
- Line 1657: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto form_it = lang_it->second.find(lower);
- Line 1658: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (form_it != lang_it->second.end()) {
- Line 1751: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: double tf = static_cast<double>(it->second) / total_terms;
- Line 1755: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: double idf = std::log(static_cast<double>(term_freqs.size()) / it->second);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3507 docs(analytics): reconcile ... (2026-03-12) | #2990 [analytics] Full mo
- Line 408: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: bool has_subquery    = containsSubquery(query_text);
- Line 641: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: en["data"]      = "datum";
- Line 60: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 142: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entity_patterns_.push_back({R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})", "EMAIL"});
- Line 145: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entity_patterns_.push_back({R"(https?://[^\s]+)", "URL"});
- Line 145: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: entity_patterns_.push_back({R"(https?://[^\s]+)", "URL"});
- Line 148: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entity_patterns_.push_back({R"(\d{1,2}[./-]\d{1,2}[./-]\d{2,4})", "DATE"});
- Line 148: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: entity_patterns_.push_back({R"(\d{1,2}[./-]\d{1,2}[./-]\d{2,4})", "DATE"});
- Line 151: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entity_patterns_.push_back({R"(\d+\s*(GB|MB|KB|TB|kg|km|m|cm))", "MEASUREMENT"});
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
- Line 2049: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { current.strength = std::stof(val); } catch (...) {
- Line 2137: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: modalities.push_back(modality);
  Confidence: band=high; score=0.74
- Line 1755: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double idf = std::log(static_cast<double>(term_freqs.size()) / it->second);
  Confidence: band=medium; score=0.6

### src/analytics/ml_serving.cpp
Total findings: 34

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4929 [Docs][analytics] Refresh m... (2026-05-10) | #4315 fix(analytics): eli
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
- Line 414: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: inputs_json[t.name] = json(t.data.begin(), t.data.end());
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
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
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
- Line 276: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: input_names.push_back(t.name.c_str());
- Line 282: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: input_tensors.push_back(Ort::Value::CreateTensor<float>(memory_info, data_copy.data(), data_copy.siz
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
- Line 499: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: t.data.push_back(node.get<float>());
- Line 636: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.push_back(static_cast<float>(*v_double));
  Confidence: band=high; score=0.74

### src/analytics/diff_engine.cpp
Total findings: 29

- Line 181: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: inflight_cv_.wait(lock);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4325 [Issue] Implement DiffEngin... (2026-03-19) | #1444 feat(analytics): im
- Line 187: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: spdlog::debug("Cache hit for diff range [{}, {}]", from_sequence, to_sequence);
- Line 232: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: spdlog::debug("Found {} events in range [{}, {}]", events.size(), from_sequence, to_sequence);
- Line 252: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto it = std::next(diff_cache_.begin()); it != diff_cache_.end(); ++it) {
- Line 269: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: spdlog::debug("Evicting oldest cache entry: range [{}, {}]", evict_key.first, evict_key.second);
- Line 306: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: spdlog::debug("Timestamp range maps to sequence range [{}, {}]", from_seq, to_seq);
- Line 549: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: spdlog::warn("No events available for timestamp range [{}, {}]", from_timestamp, to_timestamp);
- Line 579: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: spdlog::warn("No events found in timestamp range [{}, {}]", from_timestamp, to_timestamp);
- Line 584: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: spdlog::warn("No events found in timestamp range [{}, {}]", from_timestamp, to_timestamp);
- Line 588: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: spdlog::debug("Timestamp range [{}, {}] maps to sequence range [{}, {}]", from_timestamp, to_timesta
- Line 619: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: spdlog::debug("Evicting oldest cache entry: range [{}, {}]", oldest->first.first, oldest->first.seco
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 81: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: added_arr.push_back(change.toJson());
  Confidence: band=high; score=0.74
- Line 82: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: added_arr.push_back(change.toJson());
- Line 87: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: modified_arr.push_back(change.toJson());
  Confidence: band=high; score=0.74
- Line 88: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: modified_arr.push_back(change.toJson());
- Line 93: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: deleted_arr.push_back(change.toJson());
  Confidence: band=high; score=0.74
- Line 94: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: deleted_arr.push_back(change.toJson());
- Line 115: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.added.push_back(Change::fromJson(item));
  Confidence: band=high; score=0.74
- Line 116: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.added.push_back(Change::fromJson(item));
- Line 118: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.modified.push_back(Change::fromJson(item));
  Confidence: band=high; score=0.74
- Line 119: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.modified.push_back(Change::fromJson(item));
- Line 121: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.deleted.push_back(Change::fromJson(item));
  Confidence: band=high; score=0.74
- Line 122: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.deleted.push_back(Change::fromJson(item));
- Line 383: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: key_events[event.key].push_back(event);
  Confidence: band=high; score=0.74
- Line 492: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.added.push_back(change);
  Confidence: band=high; score=0.74

### src/analytics/arrow_flight.cpp
Total findings: 23

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 787: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto listing_result = candidate->ListFlights();
- Line 808: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto listing_result = native_client_->ListFlights();
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3478 docs(analytics): sync READM... (2026-03-12) | #2987 [analytics] Impleme
- Line 151: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: spdlog::debug("[ArrowFlight] registered dataset '{}' on '{}'", key, endpoint);
- Line 928: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::unique_ptr<ArrowFlightClient> ArrowFlightClient::connect(const FlightClientOptions &opts) {
- Line 183: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: parts.push_back(part);
  Confidence: band=high; score=0.74
- Line 280: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fields.push_back(arrow::field(col.schema.name, arrow::int64(), col.schema.nullable));
  Confidence: band=high; score=0.74
- Line 281: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back(arrow::field(col.schema.name, arrow::int64(), col.schema.nullable));
- Line 287: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arrays.push_back(std::make_shared<arrow::Int64Array>(static_cast<int64_t>(tb.rowCount()), buf));
- Line 298: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arrays.push_back(arr);
  Confidence: band=high; score=0.74
- Line 304: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back(arrow::field(col.schema.name, arrow::float64(), col.schema.nullable));
- Line 321: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arrays.push_back(arr);
  Confidence: band=high; score=0.74
- Line 327: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back(arrow::field(col.schema.name, arrow::utf8(), col.schema.nullable));
- Line 342: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arrays.push_back(arr);
  Confidence: band=high; score=0.74
- Line 347: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back(arrow::field(col.schema.name, arrow::boolean(), col.schema.nullable));
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
- Line 741: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void close() override {
- Line 887: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fields.push_back(arrow::field(col.schema.name, dt, col.schema.nullable));
  Confidence: band=high; score=0.74
- Line 888: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back(arrow::field(col.schema.name, dt, col.schema.nullable));

### src/analytics/llm_process_analyzer.cpp
Total findings: 23

- Line 400: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: spdlog::debug("LLM call: provider={}, model={}, key={}", static_cast<int>(pImpl->config.provider),
- Line 500: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto sha256hex = [](const std::string &input) -> std::string {
  Confidence: band=very_high; score=0.99
- Line 502: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: SHA256(reinterpret_cast<const unsigned char *>(input.data()), input.size(), hash);
  Confidence: band=very_high; score=0.99
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4322 [LLMProcessAnalyzer] Implem... (2026-03-18) | #3041 analytics: improve
- Line 345: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ss << "Bisheriger Verlauf:\n" << data["trace"].dump(2) << "\n\n";
- Line 346: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ss << "Prozessmodell:\n" << data["model"].dump(2) << "\n\n";
- Line 475: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return response.contains("predictions") && response["predictions"].is_array();
- Line 500: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto sha256hex = [](const std::string &input) -> std::string {
  Confidence: band=very_high; score=0.9
- Line 502: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: SHA256(reinterpret_cast<const unsigned char *>(input.data()), input.size(), hash);
  Confidence: band=very_high; score=0.9
- Line 510: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: const auto &trace            = request.process_trace.is_null() ? request.process_data : request.proc
- Line 512: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: const std::string model_hash = sha256hex(request.ideal_model.dump());
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
- Line 273: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: check.corrective_actions.push_back(action.get<std::string>());
- Line 283: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto fa          = parsed["fraud_analysis"];
  Confidence: band=high; score=0.74
- Line 286: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fraud.detected_anomalies.push_back(anomaly.get<std::string>());
  Confidence: band=high; score=0.74
- Line 287: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fraud.detected_anomalies.push_back(anomaly.get<std::string>());
- Line 291: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto flags                        = fa["flags"];
  Confidence: band=high; score=0.74
- Line 395: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: [[maybe_unused]] const std::map<std::string, std::string> &params) {
  Confidence: band=high; score=0.74

### src/analytics/model_serving.cpp
Total findings: 22

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
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4929 [Docs][analytics] Refresh m... (2026-05-10) | #4314 fix(analytics): Rel
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
- Line 40: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: *     list*, health*) acquire a shared lock; write operations
- Line 41: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: *     (register, unregister, load) acquire an exclusive lock.
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
- Line 316: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { val = std::stod(p); } catch (...) {}
- Line 343: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(e->info);
  Confidence: band=high; score=0.74

### src/analytics/incremental_view.cpp
Total findings: 18

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4316 feat(analytics): Incrementa... (2026-03-18) | #3610 fix(analytics): reg
- Line 251: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it         = gk.find(f.field);
- Line 251: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it         = gk.find(f.field);
- Line 270: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = row.find(spec.source_field);
- Line 418: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t batch_start = 0; batch_start < filtered.size(); batch_start += kMicroBatchSize) {
  Confidence: band=very_high; score=0.9
- Line 473: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function query without trace point
  Context: ViewQueryResult IncrementalView::query(const std::vector<ViewFilter> &filters, int64_t limit, int64_t offset) const {
  Confidence: band=very_high; score=0.9
- Line 586: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return (it != views_.end()) ? it->second : nullptr;
- Line 619: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ViewQueryResult IncrementalViewManager::query(const std::string &view_name, const std::vector<ViewFi
- Line 627: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return it->second->query(filters, limit, offset);
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
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
- Line 410: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: filtered.push_back({i, bp, ap});
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

### src/analytics/process_pattern_matcher.cpp
Total findings: 17

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
- Line 491: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.deviations.push_back("MISSING: " + a);
- Line 495: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.deviations.push_back("EXTRA: " + a);
  Confidence: band=high; score=0.74
- Line 496: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.deviations.push_back("EXTRA: " + a);
- Line 502: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.deviations.push_back("MISSING_EDGE: " + e.first + "->" + e.second);
  Confidence: band=high; score=0.74
- Line 503: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.deviations.push_back("MISSING_EDGE: " + e.first + "->" + e.second);
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

### src/analytics/expert_system_engine.cpp
Total findings: 16

- Line 331: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 34: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\\"";
- Line 36: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\\\";
- Line 38: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\n";
- Line 40: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\r";
- Line 260: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 275: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 278: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
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

### src/analytics/jit_aggregation.cpp
Total findings: 14

- Line 297: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = groups.find(key);
- Line 344: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto out_col = std::make_shared<Column>(gc, src->type());
- Line 501: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: cache_[key] = [captured_specs](const ColumnBatch &batch) -> ColumnBatch {
- Line 505: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: cache_[key] = [captured_specs, captured_groups](const ColumnBatch &batch) -> ColumnBatch {
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3610 fix(analytics): register mi... (2026-03-12) | #3478 docs(analytics): sy
- Line 233: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const auto &data = col->doubleData();
- Line 251: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const auto &data = col->doubleData();
- Line 296: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = groups.find(key);
- Line 296: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = groups.find(key);
- Line 300: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: it = groups.find(key);
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 299: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: key_order.push_back(key);
  Confidence: band=high; score=0.74
- Line 331: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> first_row;
  Confidence: band=medium; score=0.66

### src/analytics/arrow_export.cpp
Total findings: 13

- Line 39: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: is_null ? int64_t(0) : std::get<int64_t>(row_data[i]));
- Line 42: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: columns_[i].double_buffer.push_back(
- Line 43: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: is_null ? 0.0 : std::get<double>(row_data[i]));
- Line 173: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: total_bytes += col.int64_buffer.size() * sizeof(int64_t);  // Zero-copy int64 buffer
- Line 27: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: columns_[i].data.push_back(row_data[i]);
  Confidence: band=high; score=0.74
- Line 28: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: columns_[i].data.push_back(row_data[i]);
- Line 38: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: columns_[i].int64_buffer.push_back(
- Line 42: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: columns_[i].double_buffer.push_back(
- Line 153: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "    // ... " << (row_count_ - 10) << " more rows\n";
- Line 153: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "    // ... " << (row_count_ - 10) << " more rows\n";
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: meta.schema.push_back(col.schema);
  Confidence: band=high; score=0.74
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: meta.schema.push_back(col.schema);
  Confidence: band=high; score=0.74
- Line 180: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: meta.schema.push_back(col.schema);

### src/analytics/analytics_export.cpp
Total findings: 12

- Line 615: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const size_t total_sz = static_cast<size_t>(ipc_buffer->size());
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4929 [Docs][analytics] Refresh m... (2026-05-10) | #4339 Analytics module: s
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
- Line 614: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const uint8_t *base   = ipc_buffer->data();
- Line 37: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: *   - `ExportStatus::NOT_SUPPORTED`: requested format unavailable without Arrow;
- Line 39: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: *   - `ExportStatus::IO_ERROR`: file open or write failure; spdlog::error logged.
- Line 40: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: *   - `ExportStatus::CONVERSION_ERROR`: Arrow RecordBatch serialisation failed;
- Line 199: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arrays.push_back(array);
  Confidence: band=high; score=0.74
- Line 403: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "\"\""; // Escape double-quotes per RFC 4180

### src/analytics/lora_pattern_classifier.cpp
Total findings: 8

- Line 126: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 190: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: numeric_values.push_back(*d);
  Confidence: band=high; score=0.74
- Line 190: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: numeric_values.push_back(*d);
  Confidence: band=high; score=0.74
- Line 194: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: numeric_values.push_back(static_cast<double>(*iv));
- Line 197: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: numeric_values.push_back(*bv ? 1.0 : 0.0);
- Line 282: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { return domains_.front().adapter_id; }
- Line 317: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 345: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(
  Confidence: band=high; score=0.74

### src/analytics/knowledge_base.cpp
Total findings: 7

- Line 148: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(f);
  Confidence: band=high; score=0.74
- Line 153: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(it->second);
  Confidence: band=high; score=0.74
- Line 233: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: parts.push_back(themis::utils::trim(stripQuotes(token)));
- Line 245: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: tp.object += "," + parts[i];
  Confidence: band=high; score=0.74
- Line 246: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: tp.object += "," + parts[i];
- Line 314: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { current.priority = 0; }
- Line 323: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { current.ml_confidence_threshold = 0.0; }

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
