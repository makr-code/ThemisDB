# observability Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: observability
- Generated: 2026-06-02 12:40:50
- Status: Critical Findings Present
- Total Findings: 332
- Actionable Findings (Critical + High): 119
- Affected Files: 21

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 24 |
| High | 95 |
| Medium | 205 |
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
| src/observability/distributed_flame_graph.cpp | 44 | 9 | 22 | 13 | 0 |
| src/observability/metric_aggregator.cpp | 40 | 0 | 16 | 24 | 0 |
| src/observability/continuous_profiler.cpp | 31 | 1 | 7 | 23 | 0 |
| src/observability/log_aggregator.cpp | 28 | 1 | 9 | 12 | 6 |
| src/observability/performance_analyzer.cpp | 28 | 1 | 1 | 26 | 0 |
| src/observability/root_cause_analyzer.cpp | 21 | 3 | 1 | 17 | 0 |
| src/observability/alertmanager.cpp | 20 | 5 | 7 | 8 | 0 |
| src/observability/query_profiler.cpp | 20 | 0 | 6 | 14 | 0 |
| src/observability/ml_anomaly_detector.cpp | 17 | 1 | 4 | 12 | 0 |
| src/observability/metrics_stream_server.cpp | 11 | 0 | 2 | 9 | 0 |
| src/observability/storage_profiler.cpp | 10 | 0 | 1 | 9 | 0 |
| src/observability/slo_reporter.cpp | 9 | 0 | 0 | 9 | 0 |
| src/observability/alerting_engine.cpp | 8 | 0 | 5 | 3 | 0 |
| src/observability/metric_anomaly_detector.cpp | 8 | 1 | 0 | 7 | 0 |
| src/observability/opentelemetry_tracer.cpp | 8 | 0 | 5 | 3 | 0 |
| src/observability/metrics_collector.cpp | 7 | 0 | 1 | 6 | 0 |
| src/observability/advanced_metrics.cpp | 6 | 0 | 3 | 1 | 2 |
| src/observability/log_search_engine.cpp | 5 | 0 | 2 | 3 | 0 |
| src/observability/tracer.cpp | 5 | 1 | 2 | 2 | 0 |
| src/observability/ebpf_tracer.cpp | 3 | 1 | 1 | 1 | 0 |
| src/observability/tenant_metrics_namespace.cpp | 3 | 0 | 0 | 3 | 0 |

## Full Scanner Findings

### src/observability/distributed_flame_graph.cpp
Total findings: 44

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
- Line 149: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator baseIt may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto baseIt = baseMap.find(stack);
- Line 234: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // low-throughput ones in the merged flame graph.
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
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
- Line 213: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = profiles_.find(id);
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
- Line 42: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 59: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += ' ';
  Confidence: band=high; score=0.74
- Line 60: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += ' ';
- Line 62: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += '\n';
- Line 74: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stacks_arr.push_back({{"stack", stack}, {"count", count}});
  Confidence: band=high; score=0.74
- Line 74: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stacks_arr.push_back({{"stack", stack}, {"count", count}});
  Confidence: band=high; score=0.74
- Line 75: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stacks_arr.push_back({{"stack", stack}, {"count", count}});
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
Total findings: 40

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 52: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(drop.begin(), drop.end(), k) == drop.end()) {
- Line 207: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = snap.labels.find(fk);
- Line 207: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = snap.labels.find(fk);
- Line 208: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = snap.labels.find(fk);
  Confidence: band=very_high; score=0.9
- Line 270: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto brace = key.find('{');
- Line 270: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto brace = key.find('{');
- Line 271: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto brace = key.find('{');
  Confidence: band=very_high; score=0.9
- Line 309: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = effective_labels.find(gl);
- Line 309: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = effective_labels.find(gl);
- Line 310: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = effective_labels.find(gl);
  Confidence: band=very_high; score=0.9
- Line 393: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = effective_labels.find(gl);
- Line 393: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = effective_labels.find(gl);
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
Total findings: 31

- Line 329: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator baseIt may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto baseIt = baseMap.find(stack);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4833 Continue Phase-6 tensorgrap... (2026-05-07) | #3783 feat(observability)
- Line 263: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: enabled_.load(std::memory_order_acquire) &&
- Line 329: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto baseIt = baseMap.find(stack);
  Confidence: band=very_high; score=0.9
- Line 377: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return enabled_.load(std::memory_order_acquire);
- Line 409: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lk(mutex_);
- Line 414: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!enabled_.load(std::memory_order_acquire)) {
- Line 65: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: frames.emplace_back(symbols[i] ? symbols[i] : "??");
  Confidence: band=high; score=0.74
- Line 95: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: frames.emplace_back(sym->Name);
  Confidence: band=high; score=0.74
- Line 99: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::snprintf(addr_buf, sizeof(addr_buf), "0x%p", frame_ptrs[i]);
- Line 144: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 267: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) key += ';';
  Confidence: band=high; score=0.74
- Line 268: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) key += ';';
- Line 277: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: text += ' ';
  Confidence: band=high; score=0.74
- Line 278: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: text += ' ';
- Line 280: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: text += '\n';
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
- Line 424: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) key += ';';
- Line 452: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { cb(snap, msg); } catch (...) {}
- Line 480: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: text += ' ';
  Confidence: band=high; score=0.74
- Line 481: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: text += ' ';
- Line 483: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: text += '\n';
- Line 492: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vec.push_back(snap);
  Confidence: band=high; score=0.74
- Line 509: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/observability/log_aggregator.cpp
Total findings: 28

- Line 267: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: async_cv_.wait(lk, [this] {
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3795 feat(observability): LogAgg... (2026-03-12) | #3783 feat(observability)
- Line 66: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x",
- Line 157: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: void accept(Level level,
- Line 266: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lk(async_mu_);
- Line 279: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: accept(task.level, task.message, task.fields);
- Line 332: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: impl_->accept(level, message, {});
- Line 362: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: impl_->accept(level, message, fields);
- Line 373: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: impl_->accept(level, message, merged);
- Line 427: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return {impl_->buffer_.begin(), impl_->buffer_.end()};
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 54: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"':  out += "\\\""; break;
  Confidence: band=high; score=0.74
- Line 55: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"':  out += "\\\""; break;
- Line 56: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': out += "\\\\"; break;
- Line 57: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\b': out += "\\b";  break;
- Line 58: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\f': out += "\\f";  break;
- Line 59: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\n': out += "\\n";  break;
- Line 60: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\r': out += "\\r";  break;
- Line 61: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\t': out += "\\t";  break;
- Line 66: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x",
- Line 280: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 398: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
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

### src/observability/performance_analyzer.cpp
Total findings: 28

- Line 432: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (hit_rate >= impl_->config.cache_hit_rate_threshold) {
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3577 [MODULE] network + observab... (2026-03-12)
- Line 44: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: recs.push_back(rec);
  Confidence: band=high; score=0.74
- Line 62: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: issues_json.push_back(issue.toJSON());
  Confidence: band=high; score=0.74
- Line 63: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: issues_json.push_back(issue.toJSON());
- Line 256: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<IssueCategory, size_t> category_counts;
  Confidence: band=medium; score=0.66
- Line 262: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: recommendations.push_back("Review and create missing indexes for frequently queried columns");
  Confidence: band=high; score=0.74
- Line 263: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: recommendations.push_back("Review and create missing indexes for frequently queried columns");
- Line 267: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: recommendations.push_back("Increase cache size or improve cache key design");
- Line 271: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: recommendations.push_back("Tune RocksDB compaction settings to reduce amplification");
- Line 275: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: recommendations.push_back("Optimize slow queries and consider query rewrites");
- Line 300: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: file << "<h1>ThemisDB Performance Analysis Report</h1>\n";
- Line 304: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: file << "<h2>Summary</h2>\n";
- Line 305: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: file << "<p>Total Issues: " << analysis.issues.size() << "</p>\n";
- Line 306: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: file << "</div>\n";
- Line 310: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: file << "<h2>Issues</h2>\n";
- Line 314: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: file << "</div>\n";
- Line 561: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: return "</body>\n</html>\n";
- Line 575: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "<h3>" << issue.title << "</h3>\n";
- Line 576: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "<p><strong>Severity:</strong> " << to_string(issue.severity) << "</p>\n";
- Line 576: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "<p><strong>Severity:</strong> " << to_string(issue.severity) << "</p>\n";
- Line 577: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "<p><strong>Category:</strong> " << to_string(issue.category) << "</p>\n";
- Line 577: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "<p><strong>Category:</strong> " << to_string(issue.category) << "</p>\n";
- Line 578: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "<p>" << issue.description << "</p>\n";
- Line 581: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "<p><strong>Recommendations:</strong></p>\n<ul>\n";
- Line 583: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "<li>" << rec << "</li>\n";
- Line 585: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "</ul>\n";
- Line 588: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "</div>\n";

### src/observability/root_cause_analyzer.cpp
Total findings: 21

- Line 331: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->series_registry[series.name] = series;
- Line 421: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto target_it = impl_->series_registry.find(metric_name);
- Line 422: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (target_it == impl_->series_registry.end()) {
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
- Line 89: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: roots.push_back(kv.first);
- Line 99: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges_json.push_back({
  Confidence: band=high; score=0.74
- Line 100: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edges_json.push_back({
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
- Line 395: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: report.contributing_factors.push_back(oss.str());
- Line 447: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(cm));
  Confidence: band=high; score=0.74
- Line 470: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: graph.nodes.push_back(ts.name);
  Confidence: band=high; score=0.74
- Line 471: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: graph.nodes.push_back(ts.name);

### src/observability/alertmanager.cpp
Total findings: 20

- Line 160: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto future = http_pool_->post(url, parsed_body, headers);
- Line 396: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto future = http_pool_->get(url, headers);
- Line 442: severity=CRITICAL; category=hardcoded_secret
  Description: hardcoded_secret: Hardcoded secret — use environment variable
  Remediation: Hardcoded secret — use environment variable
  Context: std::string metric_token = "{metric}";
- Line 449: severity=CRITICAL; category=hardcoded_secret
  Description: hardcoded_secret: Hardcoded secret — use environment variable
  Remediation: Hardcoded secret — use environment variable
  Context: std::string value_token = "{value}";
- Line 577: severity=CRITICAL; category=missing_dtor
  Description: Class ResolveAction allocates resources but has no destructor
  Remediation: Add explicit destructor: ~ResolveAction() { /* cleanup */ }
  Context: class/struct ResolveAction
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 211: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: ss << "ALERT [" << severityToString(alert.severity) << "] "
- Line 356: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: payload["matchers"]  = json::array({matcher});
- Line 627: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& action : to_fire) {
  Confidence: band=very_high; score=0.9
- Line 630: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 644: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& action : to_resolve) {
  Confidence: band=very_high; score=0.9
- Line 647: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 166: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: path, resp.status_code, attempt, attempts);
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
Total findings: 20

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3577 [MODULE] network + observab... (2026-03-12) | #3328 [WIP] Add SLO/SLA c
- Line 184: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::string QueryProfiler::start_query(const std::string& query_id,
- Line 204: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: void QueryProfiler::end_query(const std::string& query_id) {
- Line 221: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: log_slow_query(*profile);
- Line 463: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: profiler_.start_query(query_id_, query_text);
- Line 467: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: profiler_.end_query(query_id_);
- Line 71: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: operators_json.push_back(op.toJSON());
  Confidence: band=high; score=0.74
- Line 71: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: operators_json.push_back(op.toJSON());
  Confidence: band=high; score=0.74
- Line 72: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: operators_json.push_back(op.toJSON());
- Line 107: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << (total_duration.count() / 1000.0) << " ms\n";
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
- Line 165: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: times.push_back({id, profile->start_time});
- Line 170: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: size_t to_delete = profiles.size() - config.max_profiles_retained;
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
- Line 383: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: profiles_json.push_back(profile->toJSON());

### src/observability/ml_anomaly_detector.cpp
Total findings: 17

- Line 165: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator pos may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto pos = factor.find(':');
- Line 0: severity=HIGH; category=uncategorized
  Context: ['            if (labels[nidx] == NOISE) labels[nidx] = label;', '            if (labels[nidx] != UNVISITED) continue;', '            labels[nidx] = label;', '            auto nn = regionQuery(nidx);', '            if (nn.size() >= cfg_.dbscan_min_samples) {']
  Confidence: band=high; score=0.78
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
- Line 169: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: e.feature_importance.push_back({factor.substr(0, pos), v});
- Line 171: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 175: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: e.feature_importance.push_back({factor, 0.0});
- Line 204: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fi.push_back({{"name", name}, {"value", value}});
  Confidence: band=high; score=0.74
- Line 205: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fi.push_back({{"name", name}, {"value", value}});
- Line 286: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dp.push_back(std::move(p));
  Confidence: band=high; score=0.74
- Line 404: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: a.contributing_factors.push_back("dbscan_noise:1.0");
- Line 407: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: a.contributing_factors.push_back(
- Line 411: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: a.contributing_factors.push_back(

### src/observability/metrics_stream_server.cpp
Total findings: 11

- Line 60: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return running_.load(std::memory_order_acquire);
- Line 195: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x",
- Line 183: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"':  out += "\\\""; break;
  Confidence: band=high; score=0.74
- Line 184: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"':  out += "\\\""; break;
- Line 185: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': out += "\\\\"; break;
- Line 186: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\b': out += "\\b";  break;
- Line 187: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\f': out += "\\f";  break;
- Line 188: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\n': out += "\\n";  break;
- Line 189: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\r': out += "\\r";  break;
- Line 190: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\t': out += "\\t";  break;
- Line 195: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x",

### src/observability/storage_profiler.cpp
Total findings: 10

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3577 [MODULE] network + observab... (2026-03-12)
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
- Line 333: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ops_json.push_back(op.toJSON());
- Line 337: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats_json.push_back(stats.toJSON());
  Confidence: band=high; score=0.74
- Line 338: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats_json.push_back(stats.toJSON());

### src/observability/slo_reporter.cpp
Total findings: 9

- Line 29: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: alerts_arr.push_back({
  Confidence: band=high; score=0.74
- Line 30: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: alerts_arr.push_back({
- Line 105: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::vector<SloStatus> result;
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
- Line 182: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(s.toJson());
- Line 224: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: SloStatus s;
- Line 295: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: s.active_burn_rate_alerts.push_back(std::move(alert));
  Confidence: band=high; score=0.74

### src/observability/alerting_engine.cpp
Total findings: 8

- Line 96: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<void> LogNotificationChannel::send(const Alert& alert) {
- Line 126: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<void> WebhookNotificationChannel::send(const Alert& alert) {
- Line 189: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<void> SlackNotificationChannel::send(const Alert& alert) {
- Line 222: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: payload["attachments"] = json::array({attachment});
- Line 419: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto res = ch->send(alert);
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

### src/observability/metric_anomaly_detector.cpp
Total findings: 8

- Line 164: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = streams_.find(metric_name);
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
- Line 268: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: anomalies_arr.push_back(a.toJson());
- Line 270: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: metrics_arr.push_back({
- Line 305: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(name);
  Confidence: band=high; score=0.74

### src/observability/opentelemetry_tracer.cpp
Total findings: 8

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 327: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return [weak_exporter](const SpanRecord& rec) {
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
- Line 190: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 220: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::string                           status_description_;
- Line 255: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_exporter_types_.push_back(et);
  Confidence: band=high; score=0.74

### src/observability/metrics_collector.cpp
Total findings: 7

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4272 feat(observability): upgrad... (2026-03-15) | #3328 [WIP] Add SLO/SLA c
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

### src/observability/advanced_metrics.cpp
Total findings: 6

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 147: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: b.lower_bound = std::pow(data.scale, static_cast<double>(idx));
- Line 148: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: b.upper_bound = std::pow(data.scale, static_cast<double>(idx + 1));
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

### src/observability/log_search_engine.cpp
Total findings: 5

- Line 161: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = entry.fields.find(field_key);
- Line 161: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = entry.fields.find(field_key);
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

### src/observability/tracer.cpp
Total findings: 5

- Line 148: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (auto prof = profiler_.lock()) {
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3795 feat(observability): LogAgg... (2026-03-12) | #3783 feat(observability)
- Line 152: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 182: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::string                           status_description_;

### src/observability/ebpf_tracer.cpp
Total findings: 3

- Line 102: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: ssize_t n = ::read(fd, &value, sizeof(value));
- Line 372: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> wlk(mu_);
- Line 111: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);

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

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
