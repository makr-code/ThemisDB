# observability Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: observability
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 468
- Actionable Findings (Critical + High): 196
- Affected Files: 21

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 41 |
| High | 155 |
| Medium | 272 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 144 |
| container | 110 |
| reliability | 46 |
| performance | 33 |
| concurrency | 27 |
| distributed_consistency | 27 |
| platform | 20 |
| determinism | 16 |
| observability | 11 |
| exception_safety | 9 |
| raii | 9 |
| memory | 8 |
| audit_logging | 3 |
| security | 2 |
| input_validation | 1 |
| legacy_duplication | 1 |
| uninitialized | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/observability/metric_aggregator.cpp | 56 | 4 | 22 | 30 | 0 |
| src/observability/distributed_flame_graph.cpp | 54 | 10 | 25 | 19 | 0 |
| src/observability/continuous_profiler.cpp | 46 | 2 | 15 | 29 | 0 |
| src/observability/log_aggregator.cpp | 38 | 5 | 13 | 13 | 7 |
| src/observability/performance_analyzer.cpp | 36 | 1 | 1 | 34 | 0 |
| src/observability/query_profiler.cpp | 34 | 1 | 15 | 18 | 0 |
| src/observability/alertmanager.cpp | 26 | 7 | 7 | 12 | 0 |
| src/observability/ml_anomaly_detector.cpp | 25 | 1 | 7 | 17 | 0 |
| src/observability/root_cause_analyzer.cpp | 23 | 3 | 1 | 19 | 0 |
| src/observability/metrics_stream_server.cpp | 17 | 0 | 7 | 10 | 0 |
| src/observability/storage_profiler.cpp | 16 | 0 | 6 | 10 | 0 |
| src/observability/metric_anomaly_detector.cpp | 14 | 1 | 5 | 8 | 0 |
| src/observability/opentelemetry_tracer.cpp | 14 | 3 | 5 | 6 | 0 |
| src/observability/slo_reporter.cpp | 14 | 1 | 2 | 11 | 0 |
| src/observability/advanced_metrics.cpp | 12 | 0 | 8 | 2 | 2 |
| src/observability/alerting_engine.cpp | 12 | 0 | 7 | 5 | 0 |
| src/observability/metrics_collector.cpp | 8 | 0 | 1 | 7 | 0 |
| src/observability/log_search_engine.cpp | 7 | 0 | 2 | 5 | 0 |
| src/observability/ebpf_tracer.cpp | 6 | 1 | 4 | 1 | 0 |
| src/observability/tenant_metrics_namespace.cpp | 6 | 0 | 1 | 5 | 0 |
| src/observability/tracer.cpp | 4 | 1 | 1 | 2 | 0 |

## Full Scanner Findings

### src/observability/metric_aggregator.cpp
Total findings: 56

- Line 110: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator limit_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto limit_it = cardinality_limits_.find(metric_name);
- Line 312: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = effective_labels.find(gl);
- Line 396: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = effective_labels.find(gl);
- Line 450: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = snapshots_.begin(); it != snapshots_.end();) {
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 54: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(drop.begin(), drop.end(), k) == drop.end()) {
- Line 194: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 209: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = snap.labels.find(fk);
- Line 209: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = snap.labels.find(fk);
- Line 210: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = snap.labels.find(fk);
  Confidence: band=very_high; score=0.9
- Line 224: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 242: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 255: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [name, rule] : rules_) {
  Confidence: band=very_high; score=0.9
- Line 272: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto brace = key.find('{');
- Line 272: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto brace = key.find('{');
- Line 273: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto brace = key.find('{');
  Confidence: band=very_high; score=0.9
- Line 311: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = effective_labels.find(gl);
- Line 311: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = effective_labels.find(gl);
- Line 312: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = effective_labels.find(gl);
  Confidence: band=very_high; score=0.9
- Line 328: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [gk, vals] : grouped) {
  Confidence: band=very_high; score=0.9
- Line 395: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = effective_labels.find(gl);
- Line 395: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = effective_labels.find(gl);
- Line 396: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = effective_labels.find(gl);
  Confidence: band=very_high; score=0.9
- Line 462: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [key, deque] : rate_samples_) {
  Confidence: band=very_high; score=0.9
- Line 127: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::string>& labels,
  Confidence: band=high; score=0.74
- Line 146: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::string>& labels) const {
  Confidence: band=high; score=0.74
- Line 192: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::string>& filter_labels) const {
  Confidence: band=high; score=0.74
- Line 255: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(rule);
  Confidence: band=high; score=0.74
- Line 256: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(rule);
- Line 298: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<double>> grouped;               // group_key → values
  Confidence: band=high; score=0.74
- Line 299: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::map<std::string, std::string>> glabels; // group_key → labels
  Confidence: band=high; score=0.74
- Line 310: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> group_labels;
  Confidence: band=high; score=0.74
- Line 320: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: grouped[gk].push_back(v);
  Confidence: band=high; score=0.74
- Line 320: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: grouped[gk].push_back(v);
  Confidence: band=high; score=0.74
- Line 320: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: grouped[gk].push_back(v);
  Confidence: band=high; score=0.74
- Line 320: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: grouped[gk].push_back(v);
  Confidence: band=high; score=0.74
- Line 321: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: grouped[gk].push_back(v);
- Line 334: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 335: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(r));
- Line 352: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<HistogramSnapshot>> transient_snapshots;
  Confidence: band=high; score=0.74
- Line 357: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> base_labels = shard.labels;
  Confidence: band=high; score=0.74
- Line 367: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: transient_snapshots[key].push_back(std::move(snap));
  Confidence: band=high; score=0.74
- Line 367: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: transient_snapshots[key].push_back(std::move(snap));
  Confidence: band=high; score=0.74
- Line 368: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: transient_snapshots[key].push_back(std::move(snap));
- Line 384: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<double>> grouped;               // group_key → values
  Confidence: band=high; score=0.74
- Line 385: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::map<std::string, std::string>> glabels; // group_key → labels
  Confidence: band=high; score=0.74
- Line 394: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> group_labels;
  Confidence: band=high; score=0.74
- Line 404: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: grouped[gk].push_back(v);
  Confidence: band=high; score=0.74
- Line 404: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: grouped[gk].push_back(v);
  Confidence: band=high; score=0.74
- Line 404: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: grouped[gk].push_back(v);
  Confidence: band=high; score=0.74
- Line 404: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: grouped[gk].push_back(v);
  Confidence: band=high; score=0.74
- Line 405: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: grouped[gk].push_back(v);
- Line 418: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 419: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(r));

### src/observability/distributed_flame_graph.cpp
Total findings: 54

- Line 55: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // MergedFlameGraph
  Confidence: band=very_high; score=0.99
- Line 58: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::string MergedFlameGraph::toFoldedText() const {
  Confidence: band=very_high; score=0.99
- Line 69: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: json MergedFlameGraph::toJSON() const {
  Confidence: band=very_high; score=0.99
- Line 123: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: MergedFlameGraph mergeFiltered(const std::vector<std::string>& node_ids) const {
  Confidence: band=very_high; score=0.99
- Line 125: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: return mergeProfiles(node_ids);
  Confidence: band=very_high; score=0.99
- Line 128: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: ProfileDiff diff(const MergedFlameGraph& baseline,
  Confidence: band=very_high; score=0.99
- Line 129: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: const MergedFlameGraph& current) const {
  Confidence: band=very_high; score=0.99
- Line 151: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator baseIt may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto baseIt = baseMap.find(stack);
- Line 216: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = profiles_.find(id);
- Line 236: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // low-throughput ones in the merged flame graph.
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 55: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // MergedFlameGraph
  Confidence: band=very_high; score=0.9
- Line 58: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::string MergedFlameGraph::toFoldedText() const {
  Confidence: band=very_high; score=0.9
- Line 69: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: json MergedFlameGraph::toJSON() const {
  Confidence: band=very_high; score=0.9
- Line 113: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: MergedFlameGraph merge() const {
  Confidence: band=very_high; score=0.9
- Line 117: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, _] : profiles_) {
  Confidence: band=very_high; score=0.9
- Line 120: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return mergeProfiles(ids);
  Confidence: band=very_high; score=0.9
- Line 123: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: MergedFlameGraph mergeFiltered(const std::vector<std::string>& node_ids) const {
  Confidence: band=very_high; score=0.9
- Line 125: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return mergeProfiles(node_ids);
  Confidence: band=very_high; score=0.9
- Line 128: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: ProfileDiff diff(const MergedFlameGraph& baseline,
  Confidence: band=very_high; score=0.9
- Line 129: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: const MergedFlameGraph& current) const {
  Confidence: band=very_high; score=0.9
- Line 151: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto baseIt = baseMap.find(stack);
  Confidence: band=very_high; score=0.9
- Line 169: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [stack, _] : baseMap) {
  Confidence: band=very_high; score=0.9
- Line 190: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, _] : profiles_) {
  Confidence: band=very_high; score=0.9
- Line 208: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: * Merge a specific list of node IDs.
  Confidence: band=very_high; score=0.9
- Line 211: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: MergedFlameGraph mergeProfiles(const std::vector<std::string>& ids) const {
  Confidence: band=very_high; score=0.9
- Line 212: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: MergedFlameGraph result;
  Confidence: band=very_high; score=0.9
- Line 215: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = profiles_.find(id);
- Line 236: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // low-throughput ones in the merged flame graph.
  Confidence: band=very_high; score=0.9
- Line 277: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: MergedFlameGraph DistributedFlameGraph::merge() const {
  Confidence: band=very_high; score=0.9
- Line 278: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return impl_->merge();
  Confidence: band=very_high; score=0.9
- Line 281: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: MergedFlameGraph DistributedFlameGraph::mergeFiltered(
  Confidence: band=very_high; score=0.9
- Line 283: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return impl_->mergeFiltered(node_ids);
  Confidence: band=very_high; score=0.9
- Line 286: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: ProfileDiff DistributedFlameGraph::diff(const MergedFlameGraph& baseline,
  Confidence: band=very_high; score=0.9
- Line 287: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: const MergedFlameGraph& current) const {
  Confidence: band=very_high; score=0.9
- Line 44: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 61: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += ' ';
  Confidence: band=high; score=0.74
- Line 62: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += ' ';
- Line 64: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += '\n';
- Line 76: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stacks_arr.push_back({{"stack", stack}, {"count", count}});
  Confidence: band=high; score=0.74
- Line 76: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stacks_arr.push_back({{"stack", stack}, {"count", count}});
  Confidence: band=high; score=0.74
- Line 77: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stacks_arr.push_back({{"stack", stack}, {"count", count}});
- Line 117: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 118: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ids.push_back(id);
- Line 152: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.new_hotspots.push_back(stack);
  Confidence: band=high; score=0.74
- Line 152: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.new_hotspots.push_back(stack);
  Confidence: band=high; score=0.74
- Line 152: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.new_hotspots.push_back(stack);
  Confidence: band=high; score=0.74
- Line 153: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.new_hotspots.push_back(stack);
- Line 164: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.changed_hotspots.push_back(stack);
- Line 171: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.removed_hotspots.push_back(stack);
- Line 190: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 191: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ids.push_back(id);
- Line 220: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.node_ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 221: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.node_ids.push_back(id);

### src/observability/continuous_profiler.cpp
Total findings: 46

- Line 299: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = history_.find(type);
- Line 331: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator baseIt may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto baseIt = baseMap.find(stack);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 101: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(addr_buf, sizeof(addr_buf), "0x%p", frame_ptrs[i]);
  Confidence: band=very_high; score=0.9
- Line 101: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::snprintf(addr_buf, sizeof(addr_buf), "0x%p", frame_ptrs[i]);
- Line 173: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ContinuousProfiler: cannot open file for writing: " + filename);
- Line 178: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ContinuousProfiler: write error to file: " + filename);
- Line 185: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ContinuousProfiler: cannot open file for reading: " + filename);
- Line 265: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: enabled_.load(std::memory_order_acquire) &&
- Line 278: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [stack, count] : cpu_stacks_) {
  Confidence: band=very_high; score=0.9
- Line 331: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto baseIt = baseMap.find(stack);
  Confidence: band=very_high; score=0.9
- Line 347: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [stack, _] : baseMap) {
  Confidence: band=very_high; score=0.9
- Line 379: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return enabled_.load(std::memory_order_acquire);
- Line 411: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lk(mutex_);
- Line 412: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: cv_.wait_for(lk, sample_period, [this] { return !running_; });
  Confidence: band=very_high; score=0.9
- Line 416: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!enabled_.load(std::memory_order_acquire)) {
- Line 425: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < frames.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 67: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: frames.emplace_back(symbols[i] ? symbols[i] : "??");
  Confidence: band=high; score=0.74
- Line 70: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::free(symbols);
- Line 97: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: frames.emplace_back(sym->Name);
  Confidence: band=high; score=0.74
- Line 101: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::snprintf(addr_buf, sizeof(addr_buf), "0x%p", frame_ptrs[i]);
- Line 146: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 269: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) key += ';';
  Confidence: band=high; score=0.74
- Line 270: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) key += ';';
- Line 279: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: text += ' ';
  Confidence: band=high; score=0.74
- Line 280: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: text += ' ';
- Line 282: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: text += '\n';
- Line 302: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(s);
  Confidence: band=high; score=0.74
- Line 303: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(s);
- Line 332: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: diff.new_hotspots.push_back(stack);
  Confidence: band=high; score=0.74
- Line 332: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: diff.new_hotspots.push_back(stack);
  Confidence: band=high; score=0.74
- Line 332: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: diff.new_hotspots.push_back(stack);
  Confidence: band=high; score=0.74
- Line 333: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: diff.new_hotspots.push_back(stack);
- Line 342: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: diff.changed_hotspots.push_back(stack);
- Line 348: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: diff.removed_hotspots.push_back(stack);
  Confidence: band=high; score=0.74
- Line 349: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: diff.removed_hotspots.push_back(stack);
- Line 425: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) key += ';';
  Confidence: band=high; score=0.74
- Line 425: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) key += ';';
  Confidence: band=high; score=0.74
- Line 426: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) key += ';';
- Line 454: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { cb(snap, msg); } catch (...) {}
- Line 482: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: text += ' ';
  Confidence: band=high; score=0.74
- Line 483: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: text += ' ';
- Line 485: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: text += '\n';
- Line 494: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vec.push_back(snap);
  Confidence: band=high; score=0.74
- Line 495: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vec.push_back(snap);
- Line 511: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/observability/log_aggregator.cpp
Total findings: 38

- Line 269: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: async_cv_.wait(lk, [this] {
- Line 415: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if ((impl_->config_.sink_type == LogSinkType::FILE ||
- Line 416: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->config_.sink_type == LogSinkType::BOTH) &&
- Line 456: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: s.async_queue_overflows = impl_->async_overflows_.load();
- Line 456: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: s.async_queue_overflows = impl_->async_overflows_.load();
- Line 68: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x",
  Confidence: band=very_high; score=0.9
- Line 68: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x",
- Line 159: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: void accept(Level level,
- Line 268: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lk(async_mu_);
- Line 281: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: accept(task.level, task.message, task.fields);
- Line 334: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: impl_->accept(level, message, {});
- Line 364: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: impl_->accept(level, message, fields);
- Line 375: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: impl_->accept(level, message, merged);
- Line 379: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(impl_->mu_);
- Line 429: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return {impl_->buffer_.begin(), impl_->buffer_.end()};
- Line 442: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(impl_->mu_);
- Line 443: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: impl_->buffer_.clear();
- Line 457: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (int i = 0; i < 6; ++i) {
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 56: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"':  out += "\\\""; break;
  Confidence: band=high; score=0.74
- Line 57: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"':  out += "\\\""; break;
- Line 58: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': out += "\\\\"; break;
- Line 59: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\b': out += "\\b";  break;
- Line 60: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\f': out += "\\f";  break;
- Line 61: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\n': out += "\\n";  break;
- Line 62: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\r': out += "\\r";  break;
- Line 63: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\t': out += "\\t";  break;
- Line 68: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x",
- Line 156: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: if (ofs_.is_open()) ofs_.close();
- Line 282: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 400: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 333: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: void LogAggregator::log(Level level, const std::string& message) {
  Confidence: band=medium; score=0.6
- Line 338: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log(Level::TRACE, message);
  Confidence: band=medium; score=0.6
- Line 342: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log(Level::DEBUG, message);
  Confidence: band=medium; score=0.6
- Line 346: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log(Level::INFO, message);
  Confidence: band=medium; score=0.6
- Line 350: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log(Level::WARN, message);
  Confidence: band=medium; score=0.6
- Line 354: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log(Level::ERROR, message);
  Confidence: band=medium; score=0.6
- Line 358: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log(Level::CRITICAL, message);
  Confidence: band=medium; score=0.6

### src/observability/performance_analyzer.cpp
Total findings: 36

- Line 434: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (hit_rate >= impl_->config.cache_hit_rate_threshold) {
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #3577 [MODULE] network + observability â€” docs reality-check, ROADMAP fi... (2026-03-12T07:35
- Line 46: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: recs.push_back(rec);
  Confidence: band=high; score=0.74
- Line 47: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: recs.push_back(rec);
- Line 64: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: issues_json.push_back(issue.toJSON());
  Confidence: band=high; score=0.74
- Line 65: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: issues_json.push_back(issue.toJSON());
- Line 192: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: issues.push_back(slow_query_issue);
- Line 198: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: issues.push_back(full_scan_issue);
- Line 211: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: issues.push_back(write_amp_issue);
- Line 217: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: issues.push_back(read_amp_issue);
- Line 223: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: issues.push_back(slow_op_issue);
- Line 236: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: issues.push_back(cache_issue);
- Line 248: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: issues.push_back(index_issue);
- Line 258: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<IssueCategory, size_t> category_counts;
  Confidence: band=medium; score=0.66
- Line 264: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: recommendations.push_back("Review and create missing indexes for frequently queried columns");
  Confidence: band=high; score=0.74
- Line 265: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: recommendations.push_back("Review and create missing indexes for frequently queried columns");
- Line 269: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: recommendations.push_back("Increase cache size or improve cache key design");
- Line 273: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: recommendations.push_back("Tune RocksDB compaction settings to reduce amplification");
- Line 277: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: recommendations.push_back("Optimize slow queries and consider query rewrites");
- Line 302: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: file << "<h1>ThemisDB Performance Analysis Report</h1>\n";
- Line 306: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: file << "<h2>Summary</h2>\n";
- Line 307: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: file << "<p>Total Issues: " << analysis.issues.size() << "</p>\n";
- Line 308: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: file << "</div>\n";
- Line 312: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: file << "<h2>Issues</h2>\n";
- Line 316: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: file << "</div>\n";
- Line 563: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: return "</body>\n</html>\n";
- Line 577: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "<h3>" << issue.title << "</h3>\n";
- Line 578: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "<p><strong>Severity:</strong> " << to_string(issue.severity) << "</p>\n";
- Line 578: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "<p><strong>Severity:</strong> " << to_string(issue.severity) << "</p>\n";
- Line 579: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "<p><strong>Category:</strong> " << to_string(issue.category) << "</p>\n";
- Line 579: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "<p><strong>Category:</strong> " << to_string(issue.category) << "</p>\n";
- Line 580: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "<p>" << issue.description << "</p>\n";
- Line 583: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "<p><strong>Recommendations:</strong></p>\n<ul>\n";
- Line 585: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "<li>" << rec << "</li>\n";
- Line 587: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "</ul>\n";
- Line 590: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "</div>\n";

### src/observability/query_profiler.cpp
Total findings: 34

- Line 281: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: it->second->used_cache = true;
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #3328 [WIP] Add SLO/SLA compliance reporting with burn-rate alerts (2026-03-12T06:59:43Z)
- Line 173: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < to_delete; ++i) {
  Confidence: band=very_high; score=0.9
- Line 186: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string QueryProfiler::start_query(const std::string& query_id,
- Line 206: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: void QueryProfiler::end_query(const std::string& query_id) {
- Line 223: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: log_slow_query(*profile);
- Line 327: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [_, profile] : impl_->profiles) {
  Confidence: band=very_high; score=0.9
- Line 340: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [_, profile] : impl_->profiles) {
  Confidence: band=very_high; score=0.9
- Line 359: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [_, profile] : impl_->profiles) {
  Confidence: band=very_high; score=0.9
- Line 384: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [_, profile] : impl_->profiles) {
  Confidence: band=very_high; score=0.9
- Line 393: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(impl_->mutex);
- Line 398: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(impl_->mutex);
- Line 403: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(impl_->mutex);
- Line 408: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(impl_->mutex);
- Line 465: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: profiler_.start_query(query_id_, query_text);
- Line 469: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: profiler_.end_query(query_id_);
- Line 73: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: operators_json.push_back(op.toJSON());
  Confidence: band=high; score=0.74
- Line 73: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: operators_json.push_back(op.toJSON());
  Confidence: band=high; score=0.74
- Line 74: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: operators_json.push_back(op.toJSON());
- Line 109: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << (total_duration.count() / 1000.0) << " ms\n";
- Line 142: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::shared_ptr<QueryProfile>> profiles;
  Confidence: band=medium; score=0.66
- Line 154: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_remove.push_back(id);
  Confidence: band=high; score=0.74
- Line 155: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: to_remove.push_back(id);
- Line 166: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: times.push_back({id, profile->start_time});
  Confidence: band=high; score=0.74
- Line 166: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: times.push_back({id, profile->start_time});
  Confidence: band=high; score=0.74
- Line 167: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: times.push_back({id, profile->start_time});
- Line 172: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: size_t to_delete = profiles.size() - config.max_profiles_retained;
- Line 327: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(profile);
  Confidence: band=high; score=0.74
- Line 328: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(profile);
- Line 342: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(profile);
- Line 359: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(profile);
  Confidence: band=high; score=0.74
- Line 360: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(profile);
- Line 384: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: profiles_json.push_back(profile->toJSON());
  Confidence: band=high; score=0.74
- Line 385: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: profiles_json.push_back(profile->toJSON());

### src/observability/alertmanager.cpp
Total findings: 26

- Line 162: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto future = http_pool_->post(url, parsed_body, headers);
- Line 398: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto future = http_pool_->get(url, headers);
- Line 444: severity=CRITICAL; category=hardcoded_secret
  Description: hardcoded_secret: Hardcoded secret — use environment variable
  Remediation: Hardcoded secret — use environment variable
  Context: std::string metric_token = "{metric}";
- Line 451: severity=CRITICAL; category=hardcoded_secret
  Description: hardcoded_secret: Hardcoded secret — use environment variable
  Remediation: Hardcoded secret — use environment variable
  Context: std::string value_token = "{value}";
- Line 454: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator dot_pos may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto dot_pos = value_str.find('.');
- Line 512: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = rules_.find(rule_id);
- Line 579: severity=CRITICAL; category=missing_dtor
  Description: Class ResolveAction allocates resources but has no destructor
  Remediation: Add explicit destructor: ~ResolveAction() { /* cleanup */ }
  Context: class/struct ResolveAction
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 213: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: ss << "ALERT [" << severityToString(alert.severity) << "] "
- Line 358: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: payload["matchers"]  = json::array({matcher});
- Line 629: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& action : to_fire) {
  Confidence: band=very_high; score=0.9
- Line 632: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 646: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& action : to_resolve) {
  Confidence: band=very_high; score=0.9
- Line 649: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 168: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: path, resp.status_code, attempt, attempts);
- Line 236: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_alerts_.push_back(alert);
  Confidence: band=high; score=0.74
- Line 236: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_alerts_.push_back(alert);
  Confidence: band=high; score=0.74
- Line 237: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: active_alerts_.push_back(alert);
- Line 267: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: payload.push_back(entry);
  Confidence: band=high; score=0.74
- Line 267: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: payload.push_back(entry);
  Confidence: band=high; score=0.74
- Line 268: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(entry);
- Line 317: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: payload.push_back(entry);
  Confidence: band=high; score=0.74
- Line 318: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(entry);
- Line 564: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(rule);
  Confidence: band=high; score=0.74
- Line 565: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(rule);
- Line 570: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: int AlertRuleManager::evaluateRules(const std::map<std::string, double>& metrics,
  Confidence: band=high; score=0.74

### src/observability/ml_anomaly_detector.cpp
Total findings: 25

- Line 167: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator pos may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto pos = factor.find(':');
- Line 0: severity=HIGH; category=uncategorized
  Context: ['            if (labels[nidx] == NOISE) labels[nidx] = label;', '            if (labels[nidx] != UNVISITED) continue;', '            labels[nidx] = label;', '            auto nn = regionQuery(nidx);', '            if (nn.size() >= cfg_.dbscan_min_samples) {']
  Confidence: band=high; score=0.78
- Line 254: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 271: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (fcfg.alpha == 0.0) fcfg.alpha = 0.3;
  Confidence: band=very_high; score=0.9
- Line 272: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (fcfg.beta == 0.0)  fcfg.beta  = 0.1;
  Confidence: band=very_high; score=0.9
- Line 273: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (fcfg.gamma == 0.0) fcfg.gamma = 0.1;
  Confidence: band=very_high; score=0.9
- Line 311: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("MLAnomalyDetector::forecast requires train() first");
- Line 334: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("MLAnomalyDetector::detectAnomalies requires train() first");
- Line 74: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: diffs.push_back(pts[i].timestamp_ms - pts[i - 1].timestamp_ms);
  Confidence: band=high; score=0.74
- Line 75: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: diffs.push_back(pts[i].timestamp_ms - pts[i - 1].timestamp_ms);
- Line 98: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: neighbours.push_back(j);
  Confidence: band=high; score=0.74
- Line 99: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: neighbours.push_back(j);
- Line 170: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: e.feature_importance.push_back({factor.substr(0, pos), v});
  Confidence: band=high; score=0.74
- Line 171: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: e.feature_importance.push_back({factor.substr(0, pos), v});
- Line 173: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 177: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: e.feature_importance.push_back({factor, 0.0});
- Line 206: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fi.push_back({{"name", name}, {"value", value}});
  Confidence: band=high; score=0.74
- Line 207: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fi.push_back({{"name", name}, {"value", value}});
- Line 288: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dp.push_back(std::move(p));
  Confidence: band=high; score=0.74
- Line 289: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: dp.push_back(std::move(p));
- Line 344: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& p : pts) values.push_back(p.value);
- Line 406: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: a.contributing_factors.push_back("dbscan_noise:1.0");
- Line 409: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: a.contributing_factors.push_back(
- Line 413: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: a.contributing_factors.push_back(
- Line 418: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(a));

### src/observability/root_cause_analyzer.cpp
Total findings: 23

- Line 333: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->series_registry[series.name] = series;
- Line 423: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto target_it = impl_->series_registry.find(metric_name);
- Line 424: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (target_it == impl_->series_registry.end()) {
- Line 432: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (kv.first == metric_name) {
  Confidence: band=very_high; score=0.9
- Line 37: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: TimeSeries::mean()
  Context: double TimeSeries::mean() const {
  Confidence: band=medium; score=0.56
- Line 80: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int> in_degree;
  Confidence: band=medium; score=0.66
- Line 90: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: roots.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 90: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: roots.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 90: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: roots.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 91: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: roots.push_back(kv.first);
- Line 101: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges_json.push_back({
  Confidence: band=high; score=0.74
- Line 102: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edges_json.push_back({
- Line 206: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: v.push_back(p.value);
  Confidence: band=high; score=0.74
- Line 207: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: v.push_back(p.value);
- Line 316: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: static double deltaForKey(const std::map<std::string, double>& deltas,
  Confidence: band=high; score=0.74
- Line 354: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> deltas;
  Confidence: band=high; score=0.74
- Line 396: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.contributing_factors.push_back(oss.str());
  Confidence: band=high; score=0.74
- Line 396: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.contributing_factors.push_back(oss.str());
  Confidence: band=high; score=0.74
- Line 397: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: report.contributing_factors.push_back(oss.str());
- Line 449: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(cm));
  Confidence: band=high; score=0.74
- Line 450: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(cm));
- Line 472: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: graph.nodes.push_back(ts.name);
  Confidence: band=high; score=0.74
- Line 473: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: graph.nodes.push_back(ts.name);

### src/observability/metrics_stream_server.cpp
Total findings: 17

- Line 45: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("MetricsStreamServer::start: bind_address must not be empty");
- Line 48: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("MetricsStreamServer::start: port must not be 0");
- Line 62: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return running_.load(std::memory_order_acquire);
- Line 81: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 94: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 197: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x",
  Confidence: band=very_high; score=0.9
- Line 197: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x",
- Line 43: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void MetricsStreamServer::start(const std::string& bind_address, uint16_t port) {
  Confidence: band=medium; score=0.66
- Line 185: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"':  out += "\\\""; break;
  Confidence: band=high; score=0.74
- Line 186: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"':  out += "\\\""; break;
- Line 187: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': out += "\\\\"; break;
- Line 188: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\b': out += "\\b";  break;
- Line 189: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\f': out += "\\f";  break;
- Line 190: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\n': out += "\\n";  break;
- Line 191: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\r': out += "\\r";  break;
- Line 192: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\t': out += "\\t";  break;
- Line 197: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x",

### src/observability/storage_profiler.cpp
Total findings: 16

- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #3577 [MODULE] network + observability â€” docs reality-check, ROADMAP fi... (2026-03-12T07:35
- Line 249: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [type, count] : op_counts) {
  Confidence: band=very_high; score=0.9
- Line 339: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& stats : impl_->rocksdb_stats_history) {
  Confidence: band=very_high; score=0.9
- Line 356: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(impl_->mutex);
- Line 361: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(impl_->mutex);
- Line 366: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(impl_->mutex);
- Line 57: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: level_sizes_json.push_back(size);
  Confidence: band=high; score=0.74
- Line 58: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: level_sizes_json.push_back(size);
- Line 236: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<StorageOpType, size_t> op_counts;
  Confidence: band=medium; score=0.66
- Line 237: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<StorageOpType, std::chrono::microseconds> op_durations;
  Confidence: band=medium; score=0.66
- Line 238: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<StorageOpType, size_t> op_bytes_read;
  Confidence: band=medium; score=0.66
- Line 239: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<StorageOpType, size_t> op_bytes_written;
  Confidence: band=medium; score=0.66
- Line 334: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ops_json.push_back(op.toJSON());
  Confidence: band=high; score=0.74
- Line 335: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ops_json.push_back(op.toJSON());
- Line 339: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats_json.push_back(stats.toJSON());
  Confidence: band=high; score=0.74
- Line 340: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats_json.push_back(stats.toJSON());

### src/observability/metric_anomaly_detector.cpp
Total findings: 14

- Line 166: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = streams_.find(metric_name);
- Line 143: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::out_of_range("MetricAnomalyDetector: unknown metric '" +
- Line 152: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [name, state] : streams_) {
  Confidence: band=very_high; score=0.9
- Line 168: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::out_of_range("MetricAnomalyDetector: unknown metric '" +
- Line 177: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [name, state] : streams_) {
  Confidence: band=very_high; score=0.9
- Line 250: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = start; i < state->anomalies.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 96: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: auto raw = state.sad.process(dp);  // may return nullopt during warm-up
  Confidence: band=high; score=0.74
- Line 269: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: anomalies_arr.push_back(a.toJson());
  Confidence: band=high; score=0.74
- Line 269: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: anomalies_arr.push_back(a.toJson());
  Confidence: band=high; score=0.74
- Line 269: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: anomalies_arr.push_back(a.toJson());
  Confidence: band=high; score=0.74
- Line 270: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: anomalies_arr.push_back(a.toJson());
- Line 272: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: metrics_arr.push_back({
- Line 307: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(name);
  Confidence: band=high; score=0.74
- Line 308: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: names.push_back(name);

### src/observability/opentelemetry_tracer.cpp
Total findings: 14

- Line 330: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (auto exporter = weak_exporter.lock()) {
- Line 475: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->config_.service_name = serviceName;
- Line 476: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->config_.endpoint     = endpoint;
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 329: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return [weak_exporter](const SpanRecord& rec) {
- Line 562: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (metrics.cpu_usage_percent != 0.0) {
  Confidence: band=very_high; score=0.9
- Line 566: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (metrics.memory_usage_bytes != 0.0) {
  Confidence: band=very_high; score=0.9
- Line 578: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (metrics.cache_hit_rate != 0.0) {
  Confidence: band=very_high; score=0.9
- Line 192: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 222: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::string                           status_description_;
- Line 257: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_exporter_types_.push_back(et);
  Confidence: band=high; score=0.74
- Line 258: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: active_exporter_types_.push_back(et);
- Line 274: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: delegate_tracers_.push_back(std::move(jaeger));
- Line 278: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: delegate_tracers_.push_back(std::move(zipkin));

### src/observability/slo_reporter.cpp
Total findings: 14

- Line 76: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = slos_.find(slo_name);
- Line 96: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::out_of_range("SloReporter: unknown SLO name '" + slo_name + "'");
- Line 183: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& s : statuses) {
  Confidence: band=very_high; score=0.9
- Line 31: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: alerts_arr.push_back({
  Confidence: band=high; score=0.74
- Line 32: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: alerts_arr.push_back({
- Line 107: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::vector<SloStatus> result;
- Line 111: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(computeStatus(state_copy));
  Confidence: band=high; score=0.74
- Line 112: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(computeStatus(state_copy));
- Line 183: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(s.toJson());
  Confidence: band=high; score=0.74
- Line 183: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(s.toJson());
  Confidence: band=high; score=0.74
- Line 184: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(s.toJson());
- Line 226: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: SloStatus s;
- Line 297: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: s.active_burn_rate_alerts.push_back(std::move(alert));
  Confidence: band=high; score=0.74
- Line 298: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: s.active_burn_rate_alerts.push_back(std::move(alert));

### src/observability/advanced_metrics.cpp
Total findings: 12

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 43: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto& data = summary_data_[name];
- Line 79: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (double q : quantiles) {
  Confidence: band=very_high; score=0.9
- Line 93: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto& data = exp_hist_data_[name];
- Line 126: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const auto& data = it->second;
- Line 147: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [idx, count] : bucket_counts) {
  Confidence: band=very_high; score=0.9
- Line 149: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: b.lower_bound = std::pow(data.scale, static_cast<double>(idx));
- Line 150: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: b.upper_bound = std::pow(data.scale, static_cast<double>(idx + 1));
- Line 151: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.buckets.push_back(b);
  Confidence: band=high; score=0.74
- Line 152: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.buckets.push_back(b);
- Line 138: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: const double log_scale = std::log(data.scale);
  Confidence: band=medium; score=0.6
- Line 141: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: int idx = static_cast<int>(std::floor(std::log(v) / log_scale));
  Confidence: band=medium; score=0.6

### src/observability/alerting_engine.cpp
Total findings: 12

- Line 98: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<void> LogNotificationChannel::send(const Alert& alert) {
- Line 128: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<void> WebhookNotificationChannel::send(const Alert& alert) {
- Line 191: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<void> SlackNotificationChannel::send(const Alert& alert) {
- Line 224: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: payload["attachments"] = json::array({attachment});
- Line 275: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(channels_mutex_);
- Line 280: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(channels_mutex_);
- Line 421: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto res = ch->send(alert);
- Line 145: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> headers{
  Confidence: band=medium; score=0.66
- Line 209: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fields.push_back(field);
  Confidence: band=high; score=0.74
- Line 210: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back(field);
- Line 437: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_alerts_.push_back(alert);
  Confidence: band=high; score=0.74
- Line 438: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: active_alerts_.push_back(alert);

### src/observability/metrics_collector.cpp
Total findings: 8

- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #3328 [WIP] Add SLO/SLA compliance reporting with burn-rate alerts (2026-03-12T06:59:43Z)
- Line 288: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::string>& labels) {
  Confidence: band=high; score=0.74
- Line 296: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::string>& labels) {
  Confidence: band=high; score=0.74
- Line 306: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: void MetricsCollector::incrementCounter(const std::string& name, const std::map<std::string, std::string>& labels) {
  Confidence: band=high; score=0.74
- Line 313: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: void MetricsCollector::setGauge(const std::string& name, double value, const std::map<std::string, std::string>& labels) {
  Confidence: band=high; score=0.74
- Line 320: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: void MetricsCollector::observeHistogram(const std::string& name, double value, const std::map<std::string, std::string>& labels) {
  Confidence: band=high; score=0.74
- Line 334: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::string>& labels) {
  Confidence: band=high; score=0.74
- Line 401: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: values.push_back(value);

### src/observability/log_search_engine.cpp
Total findings: 7

- Line 163: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = entry.fields.find(field_key);
- Line 163: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = entry.fields.find(field_key);
- Line 94: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched.push_back(&entry);
  Confidence: band=high; score=0.74
- Line 95: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: matched.push_back(&entry);
- Line 101: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort by timestamp.
  Confidence: band=high; score=0.74
- Line 131: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.entries.push_back(*matched[i]);
  Confidence: band=high; score=0.74
- Line 132: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.entries.push_back(*matched[i]);

### src/observability/ebpf_tracer.cpp
Total findings: 6

- Line 104: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: ssize_t n = ::read(fd, &value, sizeof(value));
- Line 192: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(mu_);
- Line 374: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> wlk(mu_);
- Line 375: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: cv_.wait_for(wlk, interval, [this]{ return !running_; });
  Confidence: band=very_high; score=0.9
- Line 388: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& ev : batch) {
  Confidence: band=very_high; score=0.9
- Line 113: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);

### src/observability/tenant_metrics_namespace.cpp
Total findings: 6

- Line 308: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& kv : stores_) {
  Confidence: band=very_high; score=0.9
- Line 61: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 62: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ids.push_back(kv.first);
- Line 78: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::string>& labels)
  Confidence: band=high; score=0.74
- Line 314: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 315: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(s));

### src/observability/tracer.cpp
Total findings: 4

- Line 150: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (auto prof = profiler_.lock()) {
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 154: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 184: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::string                           status_description_;

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
