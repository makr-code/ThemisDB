# observability Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: observability
- Generated: 2026-06-02 11:09:13
- Status: Critical Findings Present
- Total Findings: 170
- Actionable Findings (Critical + High): 42
- Affected Files: 21

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 8 |
| High | 34 |
| Medium | 120 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 117 |
| container | 49 |
| performance | 33 |
| reliability | 31 |
| distributed_consistency | 27 |
| platform | 20 |
| determinism | 16 |
| observability | 11 |
| exception_safety | 9 |
| raii | 7 |
| concurrency | 6 |
| audit_logging | 3 |
| memory | 3 |
| security | 2 |
| input_validation | 1 |
| legacy_duplication | 1 |
| uninitialized | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/observability/distributed_flame_graph.cpp | 37 | 8 | 20 | 9 | 0 |
| src/observability/metric_aggregator.cpp | 28 | 0 | 4 | 24 | 0 |
| src/observability/continuous_profiler.cpp | 14 | 0 | 1 | 13 | 0 |
| src/observability/root_cause_analyzer.cpp | 14 | 0 | 1 | 13 | 0 |
| src/observability/alertmanager.cpp | 9 | 0 | 2 | 7 | 0 |
| src/observability/query_profiler.cpp | 9 | 0 | 0 | 9 | 0 |
| src/observability/ml_anomaly_detector.cpp | 8 | 0 | 3 | 5 | 0 |
| src/observability/log_aggregator.cpp | 7 | 0 | 0 | 1 | 6 |
| src/observability/storage_profiler.cpp | 7 | 0 | 0 | 7 | 0 |
| src/observability/metrics_collector.cpp | 6 | 0 | 0 | 6 | 0 |
| src/observability/metric_anomaly_detector.cpp | 5 | 0 | 0 | 5 | 0 |
| src/observability/slo_reporter.cpp | 5 | 0 | 0 | 5 | 0 |
| src/observability/opentelemetry_tracer.cpp | 4 | 0 | 3 | 1 | 0 |
| src/observability/performance_analyzer.cpp | 4 | 0 | 0 | 4 | 0 |
| src/observability/advanced_metrics.cpp | 3 | 0 | 0 | 1 | 2 |
| src/observability/alerting_engine.cpp | 3 | 0 | 0 | 3 | 0 |
| src/observability/log_search_engine.cpp | 3 | 0 | 0 | 3 | 0 |
| src/observability/tenant_metrics_namespace.cpp | 3 | 0 | 0 | 3 | 0 |
| src/observability/metrics_stream_server.cpp | 1 | 0 | 0 | 1 | 0 |
| src/observability/ebpf_tracer.cpp | 0 | 0 | 0 | 0 | 0 |
| src/observability/tracer.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/observability/distributed_flame_graph.cpp
Total findings: 37

- Line 53: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // MergedFlameGraph
  Confidence: band=very_high; score=0.99
- Line 56: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::string MergedFlameGraph::toFoldedText() const {
  Confidence: band=very_high; score=0.99
- Line 67: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: json MergedFlameGraph::toJSON() const {
  Confidence: band=very_high; score=0.99
- Line 121: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: MergedFlameGraph mergeFiltered(const std::vector<std::string>& node_ids) const {
  Confidence: band=very_high; score=0.99
- Line 123: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return mergeProfiles(node_ids);
  Confidence: band=very_high; score=0.99
- Line 126: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: ProfileDiff diff(const MergedFlameGraph& baseline,
  Confidence: band=very_high; score=0.99
- Line 127: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: const MergedFlameGraph& current) const {
  Confidence: band=very_high; score=0.99
- Line 234: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // low-throughput ones in the merged flame graph.
  Confidence: band=very_high; score=0.99
- Line 53: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // MergedFlameGraph
  Confidence: band=very_high; score=0.9
- Line 56: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string MergedFlameGraph::toFoldedText() const {
  Confidence: band=very_high; score=0.9
- Line 67: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: json MergedFlameGraph::toJSON() const {
  Confidence: band=very_high; score=0.9
- Line 111: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: MergedFlameGraph merge() const {
  Confidence: band=very_high; score=0.9
- Line 118: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return mergeProfiles(ids);
  Confidence: band=very_high; score=0.9
- Line 121: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: MergedFlameGraph mergeFiltered(const std::vector<std::string>& node_ids) const {
  Confidence: band=very_high; score=0.9
- Line 123: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return mergeProfiles(node_ids);
  Confidence: band=very_high; score=0.9
- Line 126: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: ProfileDiff diff(const MergedFlameGraph& baseline,
  Confidence: band=very_high; score=0.9
- Line 127: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: const MergedFlameGraph& current) const {
  Confidence: band=very_high; score=0.9
- Line 149: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto baseIt = baseMap.find(stack);
  Confidence: band=very_high; score=0.9
- Line 206: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: * Merge a specific list of node IDs.
  Confidence: band=very_high; score=0.9
- Line 209: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: MergedFlameGraph mergeProfiles(const std::vector<std::string>& ids) const {
  Confidence: band=very_high; score=0.9
- Line 210: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: MergedFlameGraph result;
  Confidence: band=very_high; score=0.9
- Line 234: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // low-throughput ones in the merged flame graph.
  Confidence: band=very_high; score=0.9
- Line 275: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: MergedFlameGraph DistributedFlameGraph::merge() const {
  Confidence: band=very_high; score=0.9
- Line 276: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return impl_->merge();
  Confidence: band=very_high; score=0.9
- Line 279: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: MergedFlameGraph DistributedFlameGraph::mergeFiltered(
  Confidence: band=very_high; score=0.9
- Line 281: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return impl_->mergeFiltered(node_ids);
  Confidence: band=very_high; score=0.9
- Line 284: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: ProfileDiff DistributedFlameGraph::diff(const MergedFlameGraph& baseline,
  Confidence: band=very_high; score=0.9
- Line 285: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: const MergedFlameGraph& current) const {
  Confidence: band=very_high; score=0.9
- Line 59: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += ' ';
  Confidence: band=high; score=0.74
- Line 74: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stacks_arr.push_back({{"stack", stack}, {"count", count}});
  Confidence: band=high; score=0.74
- Line 74: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stacks_arr.push_back({{"stack", stack}, {"count", count}});
  Confidence: band=high; score=0.74
- Line 115: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 150: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.new_hotspots.push_back(stack);
  Confidence: band=high; score=0.74
- Line 150: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.new_hotspots.push_back(stack);
  Confidence: band=high; score=0.74
- Line 150: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.new_hotspots.push_back(stack);
  Confidence: band=high; score=0.74
- Line 188: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 218: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.node_ids.push_back(id);
  Confidence: band=high; score=0.74

### src/observability/metric_aggregator.cpp
Total findings: 28

- Line 208: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = snap.labels.find(fk);
  Confidence: band=very_high; score=0.9
- Line 271: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto brace = key.find('{');
  Confidence: band=very_high; score=0.9
- Line 310: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = effective_labels.find(gl);
  Confidence: band=very_high; score=0.9
- Line 394: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = effective_labels.find(gl);
  Confidence: band=very_high; score=0.9
- Line 125: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::string>& labels,
  Confidence: band=high; score=0.74
- Line 144: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::string>& labels) const {
  Confidence: band=high; score=0.74
- Line 190: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::string>& filter_labels) const {
  Confidence: band=high; score=0.74
- Line 253: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(rule);
  Confidence: band=high; score=0.74
- Line 296: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<double>> grouped;               // group_key → values
  Confidence: band=high; score=0.74
- Line 297: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::map<std::string, std::string>> glabels; // group_key → labels
  Confidence: band=high; score=0.74
- Line 308: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> group_labels;
  Confidence: band=high; score=0.74
- Line 318: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: grouped[gk].push_back(v);
  Confidence: band=high; score=0.74
- Line 318: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: grouped[gk].push_back(v);
  Confidence: band=high; score=0.74
- Line 318: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: grouped[gk].push_back(v);
  Confidence: band=high; score=0.74
- Line 318: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: grouped[gk].push_back(v);
  Confidence: band=high; score=0.74
- Line 332: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 350: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<HistogramSnapshot>> transient_snapshots;
  Confidence: band=high; score=0.74
- Line 355: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> base_labels = shard.labels;
  Confidence: band=high; score=0.74
- Line 365: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: transient_snapshots[key].push_back(std::move(snap));
  Confidence: band=high; score=0.74
- Line 365: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: transient_snapshots[key].push_back(std::move(snap));
  Confidence: band=high; score=0.74
- Line 382: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<double>> grouped;               // group_key → values
  Confidence: band=high; score=0.74
- Line 383: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::map<std::string, std::string>> glabels; // group_key → labels
  Confidence: band=high; score=0.74
- Line 392: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> group_labels;
  Confidence: band=high; score=0.74
- Line 402: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: grouped[gk].push_back(v);
  Confidence: band=high; score=0.74
- Line 402: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: grouped[gk].push_back(v);
  Confidence: band=high; score=0.74
- Line 402: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: grouped[gk].push_back(v);
  Confidence: band=high; score=0.74
- Line 402: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: grouped[gk].push_back(v);
  Confidence: band=high; score=0.74
- Line 416: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(r));
  Confidence: band=high; score=0.74

### src/observability/continuous_profiler.cpp
Total findings: 14

- Line 329: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto baseIt = baseMap.find(stack);
  Confidence: band=very_high; score=0.9
- Line 65: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: frames.emplace_back(symbols[i] ? symbols[i] : "??");
  Confidence: band=high; score=0.74
- Line 95: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: frames.emplace_back(sym->Name);
  Confidence: band=high; score=0.74
- Line 267: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) key += ';';
  Confidence: band=high; score=0.74
- Line 277: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: text += ' ';
  Confidence: band=high; score=0.74
- Line 300: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(s);
  Confidence: band=high; score=0.74
- Line 330: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: diff.new_hotspots.push_back(stack);
  Confidence: band=high; score=0.74
- Line 330: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: diff.new_hotspots.push_back(stack);
  Confidence: band=high; score=0.74
- Line 330: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: diff.new_hotspots.push_back(stack);
  Confidence: band=high; score=0.74
- Line 346: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: diff.removed_hotspots.push_back(stack);
  Confidence: band=high; score=0.74
- Line 423: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) key += ';';
  Confidence: band=high; score=0.74
- Line 423: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) key += ';';
  Confidence: band=high; score=0.74
- Line 480: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: text += ' ';
  Confidence: band=high; score=0.74
- Line 492: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vec.push_back(snap);
  Confidence: band=high; score=0.74

### src/observability/root_cause_analyzer.cpp
Total findings: 14

- Line 430: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (kv.first == metric_name) {
  Confidence: band=very_high; score=0.9
- Line 35: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: TimeSeries::mean()
  Context: double TimeSeries::mean() const {
  Confidence: band=medium; score=0.56
- Line 78: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int> in_degree;
  Confidence: band=medium; score=0.66
- Line 88: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: roots.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 88: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: roots.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 88: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: roots.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 99: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges_json.push_back({
  Confidence: band=high; score=0.74
- Line 204: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: v.push_back(p.value);
  Confidence: band=high; score=0.74
- Line 314: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: static double deltaForKey(const std::map<std::string, double>& deltas,
  Confidence: band=high; score=0.74
- Line 352: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> deltas;
  Confidence: band=high; score=0.74
- Line 394: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.contributing_factors.push_back(oss.str());
  Confidence: band=high; score=0.74
- Line 394: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.contributing_factors.push_back(oss.str());
  Confidence: band=high; score=0.74
- Line 447: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(cm));
  Confidence: band=high; score=0.74
- Line 470: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: graph.nodes.push_back(ts.name);
  Confidence: band=high; score=0.74

### src/observability/alertmanager.cpp
Total findings: 9

- Line 627: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& action : to_fire) {
  Confidence: band=very_high; score=0.9
- Line 644: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& action : to_resolve) {
  Confidence: band=very_high; score=0.9
- Line 234: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_alerts_.push_back(alert);
  Confidence: band=high; score=0.74
- Line 234: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_alerts_.push_back(alert);
  Confidence: band=high; score=0.74
- Line 265: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: payload.push_back(entry);
  Confidence: band=high; score=0.74
- Line 265: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: payload.push_back(entry);
  Confidence: band=high; score=0.74
- Line 315: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: payload.push_back(entry);
  Confidence: band=high; score=0.74
- Line 562: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(rule);
  Confidence: band=high; score=0.74
- Line 568: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: int AlertRuleManager::evaluateRules(const std::map<std::string, double>& metrics,
  Confidence: band=high; score=0.74

### src/observability/query_profiler.cpp
Total findings: 9

- Line 71: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: operators_json.push_back(op.toJSON());
  Confidence: band=high; score=0.74
- Line 71: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: operators_json.push_back(op.toJSON());
  Confidence: band=high; score=0.74
- Line 140: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::shared_ptr<QueryProfile>> profiles;
  Confidence: band=medium; score=0.66
- Line 152: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_remove.push_back(id);
  Confidence: band=high; score=0.74
- Line 164: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: times.push_back({id, profile->start_time});
  Confidence: band=high; score=0.74
- Line 164: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: times.push_back({id, profile->start_time});
  Confidence: band=high; score=0.74
- Line 325: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(profile);
  Confidence: band=high; score=0.74
- Line 357: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(profile);
  Confidence: band=high; score=0.74
- Line 382: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: profiles_json.push_back(profile->toJSON());
  Confidence: band=high; score=0.74

### src/observability/ml_anomaly_detector.cpp
Total findings: 8

- Line 269: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (fcfg.alpha == 0.0) fcfg.alpha = 0.3;
  Confidence: band=very_high; score=0.9
- Line 270: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (fcfg.beta == 0.0)  fcfg.beta  = 0.1;
  Confidence: band=very_high; score=0.9
- Line 271: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (fcfg.gamma == 0.0) fcfg.gamma = 0.1;
  Confidence: band=very_high; score=0.9
- Line 72: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: diffs.push_back(pts[i].timestamp_ms - pts[i - 1].timestamp_ms);
  Confidence: band=high; score=0.74
- Line 96: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: neighbours.push_back(j);
  Confidence: band=high; score=0.74
- Line 168: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: e.feature_importance.push_back({factor.substr(0, pos), v});
  Confidence: band=high; score=0.74
- Line 204: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fi.push_back({{"name", name}, {"value", value}});
  Confidence: band=high; score=0.74
- Line 286: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dp.push_back(std::move(p));
  Confidence: band=high; score=0.74

### src/observability/log_aggregator.cpp
Total findings: 7

- Line 54: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"':  out += "\\\""; break;
  Confidence: band=high; score=0.74
- Line 331: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: void LogAggregator::log(Level level, const std::string& message) {
  Confidence: band=medium; score=0.6
- Line 340: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log(Level::DEBUG, message);
  Confidence: band=medium; score=0.6
- Line 344: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log(Level::INFO, message);
  Confidence: band=medium; score=0.6
- Line 348: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log(Level::WARN, message);
  Confidence: band=medium; score=0.6
- Line 352: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log(Level::ERROR, message);
  Confidence: band=medium; score=0.6
- Line 356: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log(Level::CRITICAL, message);
  Confidence: band=medium; score=0.6

### src/observability/storage_profiler.cpp
Total findings: 7

- Line 55: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: level_sizes_json.push_back(size);
  Confidence: band=high; score=0.74
- Line 234: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<StorageOpType, size_t> op_counts;
  Confidence: band=medium; score=0.66
- Line 235: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<StorageOpType, std::chrono::microseconds> op_durations;
  Confidence: band=medium; score=0.66
- Line 236: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<StorageOpType, size_t> op_bytes_read;
  Confidence: band=medium; score=0.66
- Line 237: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<StorageOpType, size_t> op_bytes_written;
  Confidence: band=medium; score=0.66
- Line 332: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ops_json.push_back(op.toJSON());
  Confidence: band=high; score=0.74
- Line 337: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats_json.push_back(stats.toJSON());
  Confidence: band=high; score=0.74

### src/observability/metrics_collector.cpp
Total findings: 6

- Line 286: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::string>& labels) {
  Confidence: band=high; score=0.74
- Line 294: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::string>& labels) {
  Confidence: band=high; score=0.74
- Line 304: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: void MetricsCollector::incrementCounter(const std::string& name, const std::map<std::string, std::string>& labels) {
  Confidence: band=high; score=0.74
- Line 311: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: void MetricsCollector::setGauge(const std::string& name, double value, const std::map<std::string, std::string>& labels) {
  Confidence: band=high; score=0.74
- Line 318: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: void MetricsCollector::observeHistogram(const std::string& name, double value, const std::map<std::string, std::string>& labels) {
  Confidence: band=high; score=0.74
- Line 332: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::string>& labels) {
  Confidence: band=high; score=0.74

### src/observability/metric_anomaly_detector.cpp
Total findings: 5

- Line 94: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: auto raw = state.sad.process(dp);  // may return nullopt during warm-up
  Confidence: band=high; score=0.74
- Line 267: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: anomalies_arr.push_back(a.toJson());
  Confidence: band=high; score=0.74
- Line 267: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: anomalies_arr.push_back(a.toJson());
  Confidence: band=high; score=0.74
- Line 267: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: anomalies_arr.push_back(a.toJson());
  Confidence: band=high; score=0.74
- Line 305: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(name);
  Confidence: band=high; score=0.74

### src/observability/slo_reporter.cpp
Total findings: 5

- Line 29: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: alerts_arr.push_back({
  Confidence: band=high; score=0.74
- Line 109: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(computeStatus(state_copy));
  Confidence: band=high; score=0.74
- Line 181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(s.toJson());
  Confidence: band=high; score=0.74
- Line 181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(s.toJson());
  Confidence: band=high; score=0.74
- Line 295: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: s.active_burn_rate_alerts.push_back(std::move(alert));
  Confidence: band=high; score=0.74

### src/observability/opentelemetry_tracer.cpp
Total findings: 4

- Line 560: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (metrics.cpu_usage_percent != 0.0) {
  Confidence: band=very_high; score=0.9
- Line 564: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (metrics.memory_usage_bytes != 0.0) {
  Confidence: band=very_high; score=0.9
- Line 576: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (metrics.cache_hit_rate != 0.0) {
  Confidence: band=very_high; score=0.9
- Line 255: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_exporter_types_.push_back(et);
  Confidence: band=high; score=0.74

### src/observability/performance_analyzer.cpp
Total findings: 4

- Line 44: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: recs.push_back(rec);
  Confidence: band=high; score=0.74
- Line 62: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: issues_json.push_back(issue.toJSON());
  Confidence: band=high; score=0.74
- Line 256: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<IssueCategory, size_t> category_counts;
  Confidence: band=medium; score=0.66
- Line 262: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: recommendations.push_back("Review and create missing indexes for frequently queried columns");
  Confidence: band=high; score=0.74

### src/observability/advanced_metrics.cpp
Total findings: 3

- Line 149: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.buckets.push_back(b);
  Confidence: band=high; score=0.74
- Line 136: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: const double log_scale = std::log(data.scale);
  Confidence: band=medium; score=0.6
- Line 139: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: int idx = static_cast<int>(std::floor(std::log(v) / log_scale));
  Confidence: band=medium; score=0.6

### src/observability/alerting_engine.cpp
Total findings: 3

- Line 143: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> headers{
  Confidence: band=medium; score=0.66
- Line 207: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fields.push_back(field);
  Confidence: band=high; score=0.74
- Line 435: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_alerts_.push_back(alert);
  Confidence: band=high; score=0.74

### src/observability/log_search_engine.cpp
Total findings: 3

- Line 92: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched.push_back(&entry);
  Confidence: band=high; score=0.74
- Line 99: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort by timestamp.
  Confidence: band=high; score=0.74
- Line 129: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.entries.push_back(*matched[i]);
  Confidence: band=high; score=0.74

### src/observability/tenant_metrics_namespace.cpp
Total findings: 3

- Line 59: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 76: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::string>& labels)
  Confidence: band=high; score=0.74
- Line 312: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(s));
  Confidence: band=high; score=0.74

### src/observability/metrics_stream_server.cpp
Total findings: 1

- Line 183: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"':  out += "\\\""; break;
  Confidence: band=high; score=0.74

### src/observability/ebpf_tracer.cpp
Total findings: 0


### src/observability/tracer.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
