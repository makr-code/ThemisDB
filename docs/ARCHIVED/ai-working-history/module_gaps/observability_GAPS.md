# observability Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: observability
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 206
- Actionable Findings (Critical + High): 102
- Affected Files: 22

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 27 |
| High | 75 |
| Medium | 91 |
| Low | 13 |

## Category Summary

| Category | Count |
|---|---:|
| string_concat_loop | 29 |
| map_vs_unordered_map | 21 |
| undefined_conflict_resolution | 19 |
| hardcoded_path | 16 |
| resource_leaked_in_exception | 9 |
| uninitialized_access | 9 |
| unstructured_log | 9 |
| missing_version_tracking | 8 |
| unordered_container_iter | 8 |
| fp_exact_comparison | 7 |
| data_race | 6 |
| nested_loop_find | 6 |
| o_n_squared | 6 |
| lock_contention | 5 |
| db_connection_leak | 4 |
| iterator_invalidation | 4 |
| expensive_inner_op | 3 |
| hardcoded_output | 3 |
| no_timeout | 3 |
| repeated_search | 3 |
| thread_join_no_timeout | 3 |
| blocking_no_timeout | 2 |
| generic_catch | 2 |
| lock_in_loop | 2 |
| manual_cleanup | 2 |
| module_doc_linkset_drift | 2 |
| primitive_no_volatile | 2 |
| size_assumption | 2 |
| uncaught_exception | 2 |
| copy_overhead | 1 |
| duplicate_qualified_signature | 1 |
| manual_cleanup_in_destructor | 1 |
| missing_dtor | 1 |
| missing_health_check | 1 |
| missing_latency_metric | 1 |
| timestamp_sorting_unstable | 1 |
| unchecked_array_index | 1 |
| uninitialized_member_field | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| observability/distributed_flame_graph.cpp | 34 | 9 | 22 | 3 | 0 |
| observability/log_aggregator.cpp | 28 | 3 | 4 | 13 | 8 |
| observability/continuous_profiler.cpp | 24 | 2 | 7 | 14 | 1 |
| observability/metric_aggregator.cpp | 23 | 0 | 12 | 11 | 0 |
| observability/performance_analyzer.cpp | 18 | 1 | 1 | 16 | 0 |
| observability/metrics_stream_server.cpp | 13 | 0 | 3 | 10 | 0 |
| observability/alertmanager.cpp | 10 | 3 | 6 | 1 | 0 |
| observability/root_cause_analyzer.cpp | 9 | 3 | 1 | 5 | 0 |
| observability/metrics_collector.cpp | 7 | 0 | 1 | 6 | 0 |
| observability/advanced_metrics.cpp | 5 | 0 | 3 | 0 | 2 |
| observability/ml_anomaly_detector.cpp | 5 | 1 | 4 | 0 | 0 |
| observability/opentelemetry_tracer.cpp | 5 | 0 | 5 | 0 | 0 |
| observability/storage_profiler.cpp | 5 | 0 | 1 | 4 | 0 |
| observability/ebpf_tracer.cpp | 4 | 2 | 1 | 1 | 0 |
| observability/query_profiler.cpp | 4 | 0 | 1 | 3 | 0 |
| observability/tracer.cpp | 4 | 2 | 2 | 0 | 0 |
| observability/log_search_engine.cpp | 2 | 0 | 1 | 1 | 0 |
| observability/metric_anomaly_detector.cpp | 2 | 1 | 0 | 1 | 0 |
| observability/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| observability/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| observability/alerting_engine.cpp | 1 | 0 | 0 | 1 | 0 |
| observability/tenant_metrics_namespace.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### observability/distributed_flame_graph.cpp
Total findings: 34

- Line 53: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // MergedFlameGraph
- Line 56: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string MergedFlameGraph::toFoldedText() const {
- Line 67: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: json MergedFlameGraph::toJSON() const {
- Line 121: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: MergedFlameGraph mergeFiltered(const std::vector<std::string>& node_ids) const {
- Line 123: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return mergeProfiles(node_ids);
- Line 126: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: ProfileDiff diff(const MergedFlameGraph& baseline,
- Line 127: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: const MergedFlameGraph& current) const {
- Line 149: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator baseIt may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto baseIt = baseMap.find(stack);
- Line 234: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // low-throughput ones in the merged flame graph.
- Line 53: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // MergedFlameGraph
- Line 56: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::string MergedFlameGraph::toFoldedText() const {
- Line 67: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: json MergedFlameGraph::toJSON() const {
- Line 111: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: MergedFlameGraph merge() const {
- Line 118: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return mergeProfiles(ids);
- Line 121: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: MergedFlameGraph mergeFiltered(const std::vector<std::string>& node_ids) const {
- Line 123: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return mergeProfiles(node_ids);
- Line 126: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: ProfileDiff diff(const MergedFlameGraph& baseline,
- Line 127: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: const MergedFlameGraph& current) const {
- Line 149: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto baseIt = baseMap.find(stack);
- Line 177: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 206: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: * Merge a specific list of node IDs.
- Line 209: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: MergedFlameGraph mergeProfiles(const std::vector<std::string>& ids) const {
- Line 210: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: MergedFlameGraph result;
- Line 213: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = profiles_.find(id);
- Line 234: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // low-throughput ones in the merged flame graph.
- Line 275: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: MergedFlameGraph DistributedFlameGraph::merge() const {
- Line 276: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return impl_->merge();
- Line 279: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: MergedFlameGraph DistributedFlameGraph::mergeFiltered(
- Line 281: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return impl_->mergeFiltered(node_ids);
- Line 284: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: ProfileDiff DistributedFlameGraph::diff(const MergedFlameGraph& baseline,
- Line 285: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: const MergedFlameGraph& current) const {
- Line 59: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: out += ' ';
- Line 60: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += ' ';
- Line 62: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += '\n';

### observability/log_aggregator.cpp
Total findings: 28

- Line 267: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: AsyncTask task;

            {

                std::unique_lock<std::mutex> lk(async_mu_);

                async_cv_.wait(lk, [this] {

                    return !async_queue_.empty() || worker_stop_.load();

                });

                if (async_queue_.empty()) {
- Line 267: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: async_cv_.wait(lk, [this] {
- Line 294: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: worker_.join();
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3795 feat(observability): LogAgg... (2026-03-12) | #3783 feat(observability)
- Line 66: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x",
- Line 152: severity=HIGH; category=manual_cleanup_in_destructor
  Description: Manual resource cleanup in destructor (should use RAII wrapper)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: ~Impl() {
- Line 266: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lk(async_mu_);
- Line 54: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: case '"':  out += "\\\""; break;
- Line 55: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '"':  out += "\\\""; break;
- Line 56: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\\': out += "\\\\"; break;
- Line 57: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\b': out += "\\b";  break;
- Line 58: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\f': out += "\\f";  break;
- Line 59: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\n': out += "\\n";  break;
- Line 60: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\r': out += "\\r";  break;
- Line 61: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\t': out += "\\t";  break;
- Line 66: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x",
- Line 280: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Process outside the queue lock to maximise throughput

            try {

                accept(task.level, task.message, task.fields);

            } catch (...) {

                // accept() must never throw, but guard defensively

            }

            task.promise->set_value();
- Line 280: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 314: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields
- Line 455: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int i = 0; i < 6; ++i) {
- Line 66: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x",
- Line 331: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void LogAggregator::log(Level level, const std::string& message) {
- Line 336: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: log(Level::TRACE, message);
- Line 340: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: log(Level::DEBUG, message);
- Line 344: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: log(Level::INFO, message);
- Line 348: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: log(Level::WARN, message);
- Line 352: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: log(Level::ERROR, message);
- Line 356: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: log(Level::CRITICAL, message);

### observability/continuous_profiler.cpp
Total findings: 24

- Line 329: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator baseIt may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto baseIt = baseMap.find(stack);
- Line 522: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: worker_.join();
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4833 Continue Phase-6 tensorgrap... (2026-05-07) | #3783 feat(observability)
- Line 263: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: enabled_.load(std::memory_order_acquire) &&
- Line 329: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto baseIt = baseMap.find(stack);
- Line 355: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 377: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return enabled_.load(std::memory_order_acquire);
- Line 409: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lk(mutex_);
- Line 414: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!enabled_.load(std::memory_order_acquire)) {
- Line 99: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::snprintf(addr_buf, sizeof(addr_buf), "0x%p", frame_ptrs[i]);
- Line 267: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i > 0) key += ';';
- Line 268: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i > 0) key += ';';
- Line 277: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: text += ' ';
- Line 278: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: text += ' ';
- Line 280: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: text += '\n';
- Line 423: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i > 0) key += ';';
- Line 424: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i > 0) key += ';';
- Line 452: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (diff.cpu_regression_percent > 20.0) {

                        std::string msg = "CPU regression detected: +" +

                            std::to_string(static_cast<int>(diff.cpu_regression_percent)) + "%";

                        try { cb(snap, msg); } catch (...) {}

                    }

                }

                last_flush_snap = snap;
- Line 452: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { cb(snap, msg); } catch (...) {}
- Line 480: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: text += ' ';
- Line 481: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: text += ' ';
- Line 483: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: text += '\n';
- Line 533: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool running_ = false;
- Line 99: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(addr_buf, sizeof(addr_buf), "0x%p", frame_ptrs[i]);

### observability/metric_aggregator.cpp
Total findings: 23

- Line 52: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(drop.begin(), drop.end(), k) == drop.end()) {
- Line 158: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 207: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = snap.labels.find(fk);
- Line 208: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto it = snap.labels.find(fk);
- Line 270: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto brace = key.find('{');
- Line 271: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto brace = key.find('{');
- Line 279: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 282: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 309: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = effective_labels.find(gl);
- Line 310: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto it = effective_labels.find(gl);
- Line 393: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = effective_labels.find(gl);
- Line 394: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto it = effective_labels.find(gl);
- Line 125: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<std::string, std::string>& labels,
- Line 144: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<std::string, std::string>& labels) const {
- Line 190: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<std::string, std::string>& filter_labels) const {
- Line 296: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::vector<double>> grouped;               // group_key → values
- Line 297: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::map<std::string, std::string>> glabels; // group_key → labels
- Line 308: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::string> group_labels;
- Line 350: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::vector<HistogramSnapshot>> transient_snapshots;
- Line 355: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::string> base_labels = shard.labels;
- Line 382: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::vector<double>> grouped;               // group_key → values
- Line 383: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::map<std::string, std::string>> glabels; // group_key → labels
- Line 392: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::string> group_labels;

### observability/performance_analyzer.cpp
Total findings: 18

- Line 432: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (hit_rate >= impl_->config.cache_hit_rate_threshold) {
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3577 [MODULE] network + observab... (2026-03-12)
- Line 256: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<IssueCategory, size_t> category_counts;
- Line 300: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: file << "<h1>ThemisDB Performance Analysis Report</h1>\n";
- Line 304: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: file << "<h2>Summary</h2>\n";
- Line 305: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: file << "<p>Total Issues: " << analysis.issues.size() << "</p>\n";
- Line 306: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: file << "</div>\n";
- Line 310: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: file << "<h2>Issues</h2>\n";
- Line 314: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: file << "</div>\n";
- Line 561: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: return "</body>\n</html>\n";
- Line 575: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: oss << "<h3>" << issue.title << "</h3>\n";
- Line 576: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: oss << "<p><strong>Severity:</strong> " << to_string(issue.severity) << "</p>\n";
- Line 577: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: oss << "<p><strong>Category:</strong> " << to_string(issue.category) << "</p>\n";
- Line 578: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: oss << "<p>" << issue.description << "</p>\n";
- Line 581: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: oss << "<p><strong>Recommendations:</strong></p>\n<ul>\n";
- Line 583: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: oss << "<li>" << rec << "</li>\n";
- Line 585: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: oss << "</ul>\n";
- Line 588: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: oss << "</div>\n";

### observability/metrics_stream_server.cpp
Total findings: 13

- Line 60: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return running_.load(std::memory_order_acquire);
- Line 195: severity=HIGH; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x",
- Line 195: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x",
- Line 41: severity=MEDIUM; category=missing_health_check
  Description: Service initialization without nearby health/status handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void MetricsStreamServer::start(const std::string& bind_address, uint16_t port) {
- Line 183: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: case '"':  out += "\\\""; break;
- Line 184: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '"':  out += "\\\""; break;
- Line 185: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\\': out += "\\\\"; break;
- Line 186: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\b': out += "\\b";  break;
- Line 187: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\f': out += "\\f";  break;
- Line 188: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\n': out += "\\n";  break;
- Line 189: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\r': out += "\\r";  break;
- Line 190: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\t': out += "\\t";  break;
- Line 195: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x",

### observability/alertmanager.cpp
Total findings: 10

- Line 160: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto future = http_pool_->post(url, parsed_body, headers);
- Line 396: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto future = http_pool_->get(url, headers);
- Line 577: severity=CRITICAL; category=missing_dtor
  Description: Class ResolveAction allocates resources but has no destructor
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct ResolveAction
- Line 211: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: ss << "ALERT [" << severityToString(alert.severity) << "] "
- Line 625: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 627: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto& action : to_fire) {
- Line 630: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 644: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto& action : to_resolve) {
- Line 647: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 568: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: int AlertRuleManager::evaluateRules(const std::map<std::string, double>& metrics,

### observability/root_cause_analyzer.cpp
Total findings: 9

- Line 331: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->series_registry[series.name] = series;
- Line 421: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto target_it = impl_->series_registry.find(metric_name);
- Line 422: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (target_it == impl_->series_registry.end()) {
- Line 430: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (kv.first == metric_name) {
- Line 35: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: TimeSeries::mean()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: double TimeSeries::mean() const {
- Line 78: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, int> in_degree;
- Line 89: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: roots.push_back(kv.first);
- Line 314: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: static double deltaForKey(const std::map<std::string, double>& deltas,
- Line 352: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, double> deltas;

### observability/metrics_collector.cpp
Total findings: 7

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4272 feat(observability): upgrad... (2026-03-15) | #3328 [WIP] Add SLO/SLA c
- Line 286: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<std::string, std::string>& labels) {
- Line 294: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<std::string, std::string>& labels) {
- Line 304: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: void MetricsCollector::incrementCounter(const std::string& name, const std::map<std::string, std::string>& labels) {
- Line 311: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: void MetricsCollector::setGauge(const std::string& name, double value, const std::map<std::string, std::string>& labels) {
- Line 318: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: void MetricsCollector::observeHistogram(const std::string& name, double value, const std::map<std::string, std::string>& labels) {
- Line 332: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<std::string, std::string>& labels) {

### observability/advanced_metrics.cpp
Total findings: 5

- Line 147: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: b.lower_bound = std::pow(data.scale, static_cast<double>(idx));
- Line 148: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: b.upper_bound = std::pow(data.scale, static_cast<double>(idx + 1));
- Line 264: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 136: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: const double log_scale = std::log(data.scale);
- Line 139: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: int idx = static_cast<int>(std::floor(std::log(v) / log_scale));

### observability/ml_anomaly_detector.cpp
Total findings: 5

- Line 165: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator pos may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto pos = factor.find(':');
- Line 117: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['            if (labels[nidx] == NOISE) labels[nidx] = label;', '            if (labels[nidx] != UNVISITED) continue;', '            labels[nidx] = label;', '            auto nn = regionQuery(nidx);', '            if (nn.size() >= cfg_.dbscan_min_samples) {']
- Line 269: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (fcfg.alpha == 0.0) fcfg.alpha = 0.3;
- Line 270: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (fcfg.beta == 0.0)  fcfg.beta  = 0.1;
- Line 271: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (fcfg.gamma == 0.0) fcfg.gamma = 0.1;

### observability/opentelemetry_tracer.cpp
Total findings: 5

- Line 327: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: return [weak_exporter](const SpanRecord& rec) {
- Line 431: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 560: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (metrics.cpu_usage_percent != 0.0) {
- Line 564: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (metrics.memory_usage_bytes != 0.0) {
- Line 576: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (metrics.cache_hit_rate != 0.0) {

### observability/storage_profiler.cpp
Total findings: 5

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3577 [MODULE] network + observab... (2026-03-12)
- Line 234: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<StorageOpType, size_t> op_counts;
- Line 235: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<StorageOpType, std::chrono::microseconds> op_durations;
- Line 236: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<StorageOpType, size_t> op_bytes_read;
- Line 237: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<StorageOpType, size_t> op_bytes_written;

### observability/ebpf_tracer.cpp
Total findings: 4

- Line 102: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: ssize_t n = ::read(fd, &value, sizeof(value));
- Line 169: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: if (thread_.joinable()) thread_.join();
- Line 372: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> wlk(mu_);
- Line 111: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::close(fd);

### observability/query_profiler.cpp
Total findings: 4

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3577 [MODULE] network + observab... (2026-03-12) | #3328 [WIP] Add SLO/SLA c
- Line 107: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << (total_duration.count() / 1000.0) << " ms\n";
- Line 140: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::shared_ptr<QueryProfile>> profiles;
- Line 170: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: size_t to_delete = profiles.size() - config.max_profiles_retained;

### observability/tracer.cpp
Total findings: 4

- Line 148: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // ContinuousProfiler integration: attach CPU snapshot when configured

            if (attach_profile_) {

                if (auto prof = profiler_.lock()) {

                    try {

                        auto snap = prof->snapshot(ProfileType::CPU);

                        rec.cpu_profile_folded = snap.dataAsString();
- Line 148: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: if (auto prof = profiler_.lock()) {
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3795 feat(observability): LogAgg... (2026-03-12) | #3783 feat(observability)
- Line 299: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### observability/log_search_engine.cpp
Total findings: 2

- Line 161: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = entry.fields.find(field_key);
- Line 99: severity=MEDIUM; category=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: // Sort by timestamp.

### observability/metric_anomaly_detector.cpp
Total findings: 2

- Line 164: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = streams_.find(metric_name);
- Line 94: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: auto raw = state.sad.process(dp);  // may return nullopt during warm-up

### observability/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### observability/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### observability/alerting_engine.cpp
Total findings: 1

- Line 143: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::string> headers{

### observability/tenant_metrics_namespace.cpp
Total findings: 1

- Line 76: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<std::string, std::string>& labels)

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
