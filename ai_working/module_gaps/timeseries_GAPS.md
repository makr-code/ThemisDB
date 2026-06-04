# timeseries Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: timeseries
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 104
- Actionable Findings (Critical + High): 87
- Affected Files: 20

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 22 |
| High | 65 |
| Medium | 15 |
| Low | 2 |

## Category Summary

| Category | Count |
|---|---:|
| resource_leaked_in_exception | 18 |
| lock_contention | 7 |
| no_timeout | 7 |
| timestamp_sorting_unstable | 6 |
| blocking_no_timeout | 5 |
| delete_without_nullptr | 5 |
| explicit_delete | 5 |
| thread_join_no_timeout | 5 |
| delete_no_nullptr | 4 |
| size_assumption | 4 |
| copy_overhead | 3 |
| legacy_or_compat_path | 3 |
| uninitialized_access | 3 |
| data_race | 2 |
| module_doc_linkset_drift | 2 |
| nested_loop_find | 2 |
| o_n_squared | 2 |
| posix_only_api | 2 |
| uncaught_exception | 2 |
| unchecked_array_index | 2 |
| unordered_container_iter | 2 |
| broken_raii_in_assignment | 1 |
| db_connection_leak | 1 |
| deadlock_risk | 1 |
| exception_in_destructor | 1 |
| explicit_lock_unlock | 1 |
| hardcoded_path | 1 |
| lock_in_loop | 1 |
| manual_cleanup | 1 |
| map_vs_unordered_map | 1 |
| missing_latency_metric | 1 |
| pointer_without_null_check | 1 |
| smart_ptr_misuse | 1 |
| uninitialized_array | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| timeseries/tsstore.cpp | 27 | 0 | 22 | 5 | 0 |
| timeseries/ts_auto_buffer.cpp | 13 | 7 | 6 | 0 | 0 |
| timeseries/adaptive_flush_controller.cpp | 11 | 5 | 6 | 0 | 0 |
| timeseries/anomaly_detection.cpp | 6 | 0 | 3 | 3 | 0 |
| timeseries/gorilla_simd.cpp | 6 | 1 | 5 | 0 | 0 |
| timeseries/hypertable.cpp | 5 | 0 | 3 | 2 | 0 |
| timeseries/ts_encrypted_key_rotation.cpp | 5 | 2 | 3 | 0 | 0 |
| timeseries/ts_stream_cursor.cpp | 4 | 3 | 0 | 1 | 0 |
| timeseries/aggregate_scheduler.cpp | 3 | 1 | 2 | 0 | 0 |
| timeseries/aggregates.cpp | 3 | 0 | 2 | 1 | 0 |
| timeseries/compression_selector.cpp | 3 | 1 | 2 | 0 | 0 |
| timeseries/encrypted_chunk_store.cpp | 3 | 0 | 3 | 0 | 0 |
| timeseries/gorilla.cpp | 3 | 0 | 3 | 0 | 0 |
| timeseries/timeseries.cpp | 3 | 0 | 0 | 3 | 0 |
| timeseries/prometheus_remote_write.cpp | 2 | 0 | 2 | 0 | 0 |
| timeseries/retention.cpp | 2 | 1 | 1 | 0 | 0 |
| timeseries/ts_auto_buffer_adaptive.cpp | 2 | 0 | 2 | 0 | 0 |
| timeseries/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| timeseries/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| timeseries/continuous_agg.cpp | 1 | 1 | 0 | 0 | 0 |

## Full Scanner Findings

### timeseries/tsstore.cpp
Total findings: 27

- Line 369: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: static_cast<double>(group_points.size() * (sizeof(int64_t) + sizeof(double))) / compressed.size());
- Line 373: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: size_t uncompressed_size = group_points.size() * (sizeof(int64_t) + sizeof(double));
- Line 407: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (const auto& point : points) {
- Line 485: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(watermark_mutex_);
- Line 572: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: size_t uncompressed = indices.size() * (sizeof(int64_t) + sizeof(double));
- Line 1013: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1030: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1040: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1112: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: THEMIS_ERROR("Failed to delete old data: {}", s.ToString());
- Line 1112: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: rocksdb::Status s = db_->Write(write_opts, &batch);

        

        if (!s.ok()) {

            THEMIS_ERROR("Failed to delete old data: {}", s.ToString());

            return 0;

        }
- Line 1112: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_ERROR("Failed to delete old data: {}", s.ToString());
- Line 1141: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::string end_key;

    {

        // end_key as first key with timestamp >= cutoff for any entity; we'll check entities in loop

        // We will iterate all keys with metric prefix and delete those with timestamp < cutoff

    }



    rocksdb::WriteBatch batch;
- Line 1141: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // We will iterate all keys with metric prefix and delete those with timestamp < cutoff
- Line 1160: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: THEMIS_ERROR("Failed to delete old data for metric {}: {}", metric, s.ToString());
- Line 1160: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: rocksdb::WriteOptions write_opts;

        rocksdb::Status s = db_->Write(write_opts, &batch);

        if (!s.ok()) {

            THEMIS_ERROR("Failed to delete old data for metric {}: {}", metric, s.ToString());

            return 0;

        }

        THEMIS_INFO("Deleted {} old data points for metric {} (before {})", deleted_count, metric, before_timestamp_ms);
- Line 1160: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_ERROR("Failed to delete old data for metric {}: {}", metric, s.ToString());
- Line 1216: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: THEMIS_ERROR("Failed to delete metric {}: {}", metric, s.ToString());
- Line 1216: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: rocksdb::Status s = db_->Write(write_opts, &batch);

        

        if (!s.ok()) {

            THEMIS_ERROR("Failed to delete metric {}: {}", metric, s.ToString());

            return ErrVoid(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,

                           fmt::format("Failed to delete metric {}: {}", metric, s.ToString()));

        }
- Line 1216: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_ERROR("Failed to delete metric {}: {}", metric, s.ToString());
- Line 1218: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: fmt::format("Failed to delete metric {}: {}", metric, s.ToString()));
- Line 1218: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (!s.ok()) {

            THEMIS_ERROR("Failed to delete metric {}: {}", metric, s.ToString());

            return ErrVoid(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,

                           fmt::format("Failed to delete metric {}: {}", metric, s.ToString()));

        }

        

        THEMIS_INFO("Deleted metric {} ({} data points)", metric, count);
- Line 1218: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: fmt::format("Failed to delete metric {}: {}", metric, s.ToString()));
- Line 258: severity=MEDIUM; category=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: // 2. Sorted by timestamp
- Line 279: severity=MEDIUM; category=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: // Sort by timestamp for Gorilla efficiency
- Line 507: severity=MEDIUM; category=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: // Sort by timestamp for Gorilla efficiency
- Line 661: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: TSStore::query(const QueryOptions& options) const {
- Line 859: severity=MEDIUM; category=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: // Sort results by timestamp (mixed raw + compressed may be out of order)

### timeseries/ts_auto_buffer.cpp
Total findings: 13

- Line 77: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: flush_thread_.join();
- Line 129: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: flush_cv_.notify_one();



            std::unique_lock<std::mutex> bp_lock(backpressure_mutex_);

            backpressure_cv_.wait(bp_lock, [this] {

                return !running_.load() ||

                       bp_buffer_size_.load(std::memory_order_relaxed) <

                           config_.backpressure_low_watermark;
- Line 129: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: backpressure_cv_.wait(bp_lock, [this] {
- Line 157: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Flush without holding the lock

            lock.unlock();

            flushInternal(false);

            lock.lock();

        }

        

        auto& buffer = buffers_[buffer_key];
- Line 157: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: lock.lock();
- Line 236: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::unique_lock<std::mutex> lock(buffers_mutex_, std::defer_lock);

    if (!lock_held) {

        lock.lock();

    }

    

    if (buffers_.empty()) {
- Line 236: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: lock.lock();
- Line 157: severity=HIGH; category=explicit_lock_unlock
  Description: Explicit lock/unlock without RAII guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: lock.lock();
- Line 356: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::unique_lock<std::mutex> lock(flush_mutex_);
- Line 356: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(flush_mutex_);
- Line 377: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> buf_lock(buffers_mutex_);
- Line 466: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 524: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### timeseries/adaptive_flush_controller.cpp
Total findings: 11

- Line 88: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: flush_thread_.join();
- Line 128: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Block until buffer drains below watermark or controller stops

        std::unique_lock<std::mutex> bp_lock(bp_mutex_);

        bp_cv_.wait(bp_lock, [this]() noexcept {

            return !running_.load(std::memory_order_relaxed) || !watermarkReached();

        });
- Line 128: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: bp_cv_.wait(bp_lock, [this]() noexcept {
- Line 190: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: flush_cv_.notify_one();



        std::unique_lock<std::mutex> bp_lock(bp_mutex_);

        bp_cv_.wait(bp_lock, [this]() noexcept {

            return !running_.load(std::memory_order_relaxed) || !watermarkReached();

        });
- Line 190: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: bp_cv_.wait(bp_lock, [this]() noexcept {
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4491 [PERF-D1-A] AdaptiveFlushCo... (2026-04-09) | #4500 feat(timeseries): i
- Line 147: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 154: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 213: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 218: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 304: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(flush_mutex_);

### timeseries/anomaly_detection.cpp
Total findings: 6

- Line 65: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = score_map.find(p.timestamp_ms);
- Line 66: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto it = score_map.find(p.timestamp_ms);
- Line 166: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto it = merged.find(ap.timestamp_ms);
- Line 60: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<int64_t, double> score_map;
- Line 163: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<int64_t, AnomalyPoint> merged;
- Line 174: severity=MEDIUM; category=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: // Collect and sort by timestamp

### timeseries/gorilla_simd.cpp
Total findings: 6

- Line 354: severity=CRITICAL; category=broken_raii_in_assignment
  Description: Broken RAII: undefined behavior and memory corruption
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 255: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Legacy chunks (encoded before v1) have no header; fall through to decode as-is.
- Line 298: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['    // These are stack-allocated and stay in registers/L1 cache throughout.', '    static constexpr int kBatchSize = 4;', '    alignas(32) int64_t  dods_buf[kBatchSize] = {};', '    alignas(32) uint64_t xors_buf[kBatchSize] = {};', '']
- Line 299: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['    static constexpr int kBatchSize = 4;', '    alignas(32) int64_t  dods_buf[kBatchSize] = {};', '    alignas(32) uint64_t xors_buf[kBatchSize] = {};', '', '    size_t total = 1;']
- Line 364: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 365: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### timeseries/hypertable.cpp
Total findings: 5

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3599 feat(timeseries): FlushCont... (2026-03-12) | #712 [Error Handling] Pha
- Line 142: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: size_t underscore_pos = key_str.find('_');
- Line 355: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 95: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::vector<std::pair<int64_t, std::string>>> chunk_batches;
- Line 161: severity=MEDIUM; category=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: // Sort by timestamp

### timeseries/ts_encrypted_key_rotation.cpp
Total findings: 5

- Line 42: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 74: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: thread_.join();
- Line 87: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lk(cv_mu_);
- Line 210: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 214: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### timeseries/ts_stream_cursor.cpp
Total findings: 4

- Line 28: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: Result<std::unique_ptr<TsStreamCursor>> TsStreamCursor::open(
- Line 35: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: Result<std::unique_ptr<TsStreamCursor>> TsStreamCursor::open(
- Line 46: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: new TsStreamCursor(store, std::move(options), cfg));
- Line 98: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void TsStreamCursor::close() noexcept {

### timeseries/aggregate_scheduler.cpp
Total findings: 3

- Line 68: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: scheduler_thread_.join();
- Line 208: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 343: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_DEBUG("Caught up window [{}, {}] for aggregate {}",

### timeseries/aggregates.cpp
Total findings: 3

- Line 92: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 192: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization
- Line 124: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: window_values.push_back(values[j]);

### timeseries/compression_selector.cpp
Total findings: 3

- Line 162: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (it != cached_.end()) return it->second;
- Line 98: severity=HIGH; category=posix_only_api
  Description: POSIX-only API select( without platform guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: CompressionStrategy HeuristicCompressionSelector::select(
- Line 120: severity=HIGH; category=posix_only_api
  Description: POSIX-only API select( without platform guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: return select(profileSeries(points));

### timeseries/encrypted_chunk_store.cpp
Total findings: 3

- Line 170: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Minimum blob size: 4 (key_id len) + 0 (key_id) + 12 (IV) + 0 (CT) + 16 (TAG)

    constexpr size_t MIN_BLOB = KEY_ID_PREFIX_LEN_BYTES + IV_LEN + TAG_LEN;

    if (blob.size() < MIN_BLOB) {

        throw std::runtime_error("EncryptedChunkStore: blob too short");

    }



    const uint8_t* p = blob.data();
- Line 245: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &final_len) != 1) {

            // Do NOT free ctx here — the catch block below handles cleanup.

            // Freeing here and then rethrowing into catch would double-free.

            throw std::runtime_error(

                "EncryptedChunkStore: authentication tag mismatch — chunk is corrupted or tampered");

        }

    } catch (...) {
- Line 281: severity=HIGH; category=pointer_without_null_check
  Description: Potential null-pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Pointer dereference without null check

### timeseries/gorilla.cpp
Total findings: 3

- Line 116: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 173: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Legacy chunks (no header) are returned unchanged.
- Line 185: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Legacy format: no header — return as-is

### timeseries/timeseries.cpp
Total findings: 3

- Line 145: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: results.push_back(DataPoint::fromJson(j));
- Line 161: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: results.push_back(DataPoint::fromJson(j));
- Line 259: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::string end_key = prefix + "\xFF"; // Seek to end of prefix range

### timeseries/prometheus_remote_write.cpp
Total findings: 2

- Line 121: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: static_assert(sizeof(double) == 8, "double must be 64-bit");
- Line 290: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: "Snappy decompression: failed to allocate output buffer");

### timeseries/retention.cpp
Total findings: 2

- Line 91: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: async_thread_.join();
- Line 97: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(async_mutex_);

### timeseries/ts_auto_buffer_adaptive.cpp
Total findings: 2

- Line 56: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 62: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### timeseries/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### timeseries/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### timeseries/continuous_agg.cpp
Total findings: 1

- Line 363: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: total += mgr_.refreshIncremental(it->second.config, it->second.agg_id,

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
