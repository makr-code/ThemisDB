# timeseries Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: timeseries
- Generated: 2026-06-02 12:40:51
- Status: Critical Findings Present
- Total Findings: 150
- Actionable Findings (Critical + High): 84
- Affected Files: 20

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 13 |
| High | 71 |
| Medium | 66 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| reliability | 31 |
| performance_patterns | 30 |
| container | 20 |
| exception_safety | 20 |
| memory | 16 |
| determinism | 8 |
| performance | 7 |
| platform | 7 |
| concurrency | 3 |
| legacy_duplication | 3 |
| raii | 3 |
| input_validation | 2 |
| uninitialized | 2 |
| observability | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/timeseries/tsstore.cpp | 30 | 0 | 16 | 14 | 0 |
| src/timeseries/hypertable.cpp | 14 | 0 | 6 | 8 | 0 |
| src/timeseries/aggregates.cpp | 11 | 0 | 2 | 9 | 0 |
| src/timeseries/adaptive_flush_controller.cpp | 10 | 2 | 6 | 2 | 0 |
| src/timeseries/anomaly_detection.cpp | 10 | 0 | 4 | 6 | 0 |
| src/timeseries/ts_auto_buffer.cpp | 10 | 3 | 7 | 0 | 0 |
| src/timeseries/continuous_agg.cpp | 8 | 1 | 2 | 5 | 0 |
| src/timeseries/gap_fill.cpp | 8 | 0 | 0 | 8 | 0 |
| src/timeseries/timeseries.cpp | 8 | 0 | 5 | 3 | 0 |
| src/timeseries/ts_encrypted_key_rotation.cpp | 7 | 1 | 4 | 2 | 0 |
| src/timeseries/gorilla_simd.cpp | 6 | 1 | 4 | 1 | 0 |
| src/timeseries/ts_stream_cursor.cpp | 6 | 4 | 1 | 1 | 0 |
| src/timeseries/compression_selector.cpp | 4 | 1 | 2 | 1 | 0 |
| src/timeseries/gorilla.cpp | 4 | 0 | 1 | 3 | 0 |
| src/timeseries/aggregate_scheduler.cpp | 3 | 0 | 2 | 1 | 0 |
| src/timeseries/encrypted_chunk_store.cpp | 3 | 0 | 1 | 2 | 0 |
| src/timeseries/timeseries_metrics.cpp | 3 | 0 | 3 | 0 | 0 |
| src/timeseries/prometheus_remote_write.cpp | 2 | 0 | 2 | 0 | 0 |
| src/timeseries/ts_auto_buffer_adaptive.cpp | 2 | 0 | 2 | 0 | 0 |
| src/timeseries/retention.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### src/timeseries/tsstore.cpp
Total findings: 30

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 42: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["metadata"] = metadata;
- Line 369: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: static_cast<double>(group_points.size() * (sizeof(int64_t) + sizeof(double))) / compressed.size());
- Line 373: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: size_t uncompressed_size = group_points.size() * (sizeof(int64_t) + sizeof(double));
- Line 407: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& point : points) {
  Confidence: band=very_high; score=0.9
- Line 485: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(watermark_mutex_);
- Line 572: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: size_t uncompressed = indices.size() * (sizeof(int64_t) + sizeof(double));
- Line 661: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: TSStore::query(const QueryOptions& options) const {
- Line 926: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result_or_error = query(agg_options);
- Line 965: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result_or_error = query(options);
- Line 1112: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: old = nullptr;
  Context: THEMIS_ERROR("Failed to delete old data: {}", s.ToString());
- Line 1160: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: old = nullptr;
  Context: THEMIS_ERROR("Failed to delete old data for metric {}: {}", metric, s.ToString());
- Line 1216: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: metric = nullptr;
  Context: THEMIS_ERROR("Failed to delete metric {}: {}", metric, s.ToString());
- Line 1218: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: metric = nullptr;
  Context: fmt::format("Failed to delete metric {}: {}", metric, s.ToString()));
- Line 104: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 222: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status s;
- Line 258: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // 2. Sorted by timestamp
  Confidence: band=high; score=0.74
- Line 272: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: grouped[group_key].push_back(point);
  Confidence: band=high; score=0.74
- Line 279: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort by timestamp for Gorilla efficiency
  Confidence: band=high; score=0.74
- Line 312: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: timestamps.push_back(p.timestamp_ms);
  Confidence: band=high; score=0.74
- Line 507: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort by timestamp for Gorilla efficiency
  Confidence: band=high; score=0.74
- Line 578: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.row_errors.emplace_back(idx,
  Confidence: band=high; score=0.74
- Line 661: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: TSStore::query(const QueryOptions& options) const {
  Confidence: band=high; score=0.74
- Line 844: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(dp);
  Confidence: band=high; score=0.74
- Line 859: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort results by timestamp (mixed raw + compressed may be out of order)
  Confidence: band=high; score=0.74
- Line 1272: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status s;
- Line 1289: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status s;
- Line 1308: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status s;

### src/timeseries/hypertable.cpp
Total findings: 14

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3599 feat(timeseries): FlushCont... (2026-03-12) | #712 [Error Handling] Pha
- Line 99: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: chunk_batches[chunk_name].emplace_back(timestamp, data);
- Line 103: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (const auto& [chunk_name, chunk_data] : chunk_batches) {
- Line 104: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (const auto& [timestamp, data] : chunk_data) {
- Line 142: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: size_t underscore_pos = key_str.find('_');
- Line 95: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::pair<int64_t, std::string>>> chunk_batches;
  Confidence: band=high; score=0.74
- Line 98: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunk_batches[chunk_name].emplace_back(timestamp, data);
  Confidence: band=high; score=0.74
- Line 126: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks_to_scan.push_back(getChunkName(t));
  Confidence: band=high; score=0.74
- Line 127: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks_to_scan.push_back(getChunkName(t));
- Line 150: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(timestamp, data);
  Confidence: band=high; score=0.74
- Line 153: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 161: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort by timestamp
  Confidence: band=high; score=0.74
- Line 226: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/timeseries/aggregates.cpp
Total findings: 11

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 54: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: window_data[window_start].push_back(values[i]);
  Confidence: band=high; score=0.74
- Line 55: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: window_data[window_start].push_back(values[i]);
- Line 60: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.timestamps.push_back(window_ts);
  Confidence: band=high; score=0.74
- Line 62: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.values.push_back(
- Line 123: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: window_values.push_back(values[j]);
  Confidence: band=high; score=0.74
- Line 123: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: window_values.push_back(values[j]);
  Confidence: band=high; score=0.74
- Line 124: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: window_values.push_back(values[j]);
- Line 129: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.timestamps.push_back(timestamps[i]);
- Line 130: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.values.push_back(

### src/timeseries/adaptive_flush_controller.cpp
Total findings: 10

- Line 128: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: bp_cv_.wait(bp_lock, [this]() noexcept {
- Line 190: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: bp_cv_.wait(bp_lock, [this]() noexcept {
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
  Context: * PR History (last 5): #4491 [PERF-D1-A] AdaptiveFlushCo... (2026-04-09) | #4500 feat(timeseries): i
- Line 304: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(flush_mutex_);
- Line 207: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buffer_.push_back(p);
  Confidence: band=high; score=0.74
- Line 246: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch.push_back(std::move(buffer_.front()));
  Confidence: band=high; score=0.74

### src/timeseries/anomaly_detection.cpp
Total findings: 10

- Line 65: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = score_map.find(p.timestamp_ms);
- Line 65: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = score_map.find(p.timestamp_ms);
- Line 66: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = score_map.find(p.timestamp_ms);
  Confidence: band=very_high; score=0.9
- Line 166: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = merged.find(ap.timestamp_ms);
  Confidence: band=very_high; score=0.9
- Line 60: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<int64_t, double> score_map;
  Confidence: band=medium; score=0.66
- Line 74: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(ap);
  Confidence: band=high; score=0.74
- Line 74: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(ap);
  Confidence: band=high; score=0.74
- Line 123: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(ap);
  Confidence: band=high; score=0.74
- Line 163: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<int64_t, AnomalyPoint> merged;
  Confidence: band=medium; score=0.66
- Line 174: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Collect and sort by timestamp
  Confidence: band=high; score=0.74

### src/timeseries/ts_auto_buffer.cpp
Total findings: 10

- Line 129: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: backpressure_cv_.wait(bp_lock, [this] {
- Line 157: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 236: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 356: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::unique_lock<std::mutex> lock(flush_mutex_);
- Line 356: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(flush_mutex_);
- Line 377: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> buf_lock(buffers_mutex_);
- Line 482: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: entry["metadata"]      = pt.metadata;
- Line 522: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: buffers_[key].add(pt);

### src/timeseries/continuous_agg.cpp
Total findings: 8

- Line 363: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: total += mgr_.refreshIncremental(it->second.config, it->second.agg_id,
- Line 228: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto first_point = store_->query(first);
- Line 387: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = store_->query(qopt);
- Line 112: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: partial_results.push_back(shard_query_(s, cfg, from_ms, to_ms));
  Confidence: band=high; score=0.74
- Line 131: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 398: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ContinuousAggMaterializationStatus s;
- Line 410: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::vector<ContinuousAggMaterializationStatus> result;
- Line 413: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (s.has_value()) result.push_back(std::move(*s));
  Confidence: band=high; score=0.74

### src/timeseries/gap_fill.cpp
Total findings: 8

- Line 91: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(*exact);
  Confidence: band=high; score=0.74
- Line 106: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(
- Line 143: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(*exact);
  Confidence: band=high; score=0.74
- Line 164: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(makeSynthetic(points.front(), target_ts, val));
- Line 174: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(makeSynthetic(points.back(), target_ts, val));
- Line 217: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(*exact);
  Confidence: band=high; score=0.74
- Line 233: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(makeSynthetic(points.back(), target_ts, val));
- Line 299: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ts.push_back(t);
  Confidence: band=high; score=0.74

### src/timeseries/timeseries.cpp
Total findings: 8

- Line 26: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["metadata"] = metadata;
- Line 111: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::vector<TimeSeriesStore::DataPoint> TimeSeriesStore::query(
- Line 173: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::vector<TimeSeriesStore::DataPoint> TimeSeriesStore::query(
- Line 176: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return query(metric, entity, RangeQuery{});
- Line 229: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::unique_ptr<rocksdb::Iterator> it(db_->NewIterator(read_opts, column_family));
- Line 145: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(DataPoint::fromJson(j));
- Line 161: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(DataPoint::fromJson(j));
- Line 259: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::string end_key = prefix + "\xFF"; // Seek to end of prefix range

### src/timeseries/ts_encrypted_key_rotation.cpp
Total findings: 7

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 87: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lk(cv_mu_);
- Line 201: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: chunk_meta["data"]   = nlohmann::json::binary(enc_result.blob);
- Line 100: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 207: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status s;

### src/timeseries/gorilla_simd.cpp
Total findings: 6

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    // These are stack-allocated and stay in registers/L1 cache throughout.', '    static constexpr int kBatchSize = 4;', '    alignas(32) int64_t  dods_buf[kBatchSize] = {};', '    alignas(32) uint64_t xors_buf[kBatchSize] = {};', '']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    static constexpr int kBatchSize = 4;', '    alignas(32) int64_t  dods_buf[kBatchSize] = {};', '    alignas(32) uint64_t xors_buf[kBatchSize] = {};', '', '    size_t total = 1;']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 359: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.emplace_back(dods_buf[b], bits_to_dbl_simd(xors_buf[b]));
  Confidence: band=high; score=0.74

### src/timeseries/ts_stream_cursor.cpp
Total findings: 6

- Line 28: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: Result<std::unique_ptr<TsStreamCursor>> TsStreamCursor::open(
- Line 32: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: return open(store, std::move(options), Config{});
- Line 35: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: Result<std::unique_ptr<TsStreamCursor>> TsStreamCursor::open(
- Line 46: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: new TsStreamCursor(store, std::move(options), cfg));
- Line 144: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = store_->query(page_opts);
- Line 98: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void TsStreamCursor::close() noexcept {

### src/timeseries/compression_selector.cpp
Total findings: 4

- Line 162: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != cached_.end()) return it->second;
- Line 98: severity=HIGH; category=posix_only_api
  Description: POSIX-only API select( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: CompressionStrategy HeuristicCompressionSelector::select(
- Line 120: severity=HIGH; category=posix_only_api
  Description: POSIX-only API select( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: return select(profileSeries(points));
- Line 48: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: deltas.push_back(points[i].timestamp_ms - points[i - 1].timestamp_ms);
  Confidence: band=high; score=0.74

### src/timeseries/gorilla.cpp
Total findings: 4

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 72: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf_.push_back(static_cast<uint8_t>(v & 0x7FUL) | 0x80U);
  Confidence: band=high; score=0.74
- Line 73: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf_.push_back(static_cast<uint8_t>(v & 0x7FUL) | 0x80U);
- Line 76: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf_.push_back(static_cast<uint8_t>(v & 0x7FUL));

### src/timeseries/aggregate_scheduler.cpp
Total findings: 3

- Line 208: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 343: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_DEBUG("Caught up window [{}, {}] for aggregate {}",
- Line 184: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(agg);
  Confidence: band=high; score=0.74

### src/timeseries/encrypted_chunk_store.cpp
Total findings: 3

- Line 0: severity=HIGH; category=uncategorized
  Context: Pointer dereference without null check
  Confidence: band=high; score=0.81
- Line 136: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 248: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/timeseries/timeseries_metrics.cpp
Total findings: 3

- Line 328: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["ingestion"]["data_points_written_total"] = total_data_points_written_.load();
- Line 339: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["query"]["data_points_returned_total"] = total_data_points_returned_.load();
- Line 350: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["storage"]["current_data_points"] = current_data_points_.load();

### src/timeseries/prometheus_remote_write.cpp
Total findings: 2

- Line 121: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: static_assert(sizeof(double) == 8, "double must be 64-bit");
- Line 290: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: "Snappy decompression: failed to allocate output buffer");

### src/timeseries/ts_auto_buffer_adaptive.cpp
Total findings: 2

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73

### src/timeseries/retention.cpp
Total findings: 1

- Line 97: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(async_mutex_);

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
