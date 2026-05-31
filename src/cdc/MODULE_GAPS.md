# cdc Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: cdc
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 257
- Actionable Findings (Critical + High): 169
- Affected Files: 13

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 20 |
| High | 149 |
| Medium | 88 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| reliability | 89 |
| container | 39 |
| performance_patterns | 35 |
| exception_safety | 19 |
| memory | 18 |
| raii | 16 |
| concurrency | 10 |
| platform | 9 |
| legacy_duplication | 6 |
| security | 6 |
| audit_logging | 4 |
| determinism | 3 |
| performance | 3 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/cdc/changefeed.cpp | 58 | 4 | 37 | 17 | 0 |
| src/cdc/consumer_group.cpp | 50 | 2 | 31 | 17 | 0 |
| src/cdc/tenant_buffer_manager.cpp | 34 | 4 | 26 | 4 | 0 |
| src/cdc/outbox.cpp | 21 | 1 | 13 | 7 | 0 |
| src/cdc/cdc_admin.cpp | 19 | 0 | 16 | 3 | 0 |
| src/cdc/ws_transport.cpp | 17 | 6 | 3 | 8 | 0 |
| src/cdc/dead_letter_queue.cpp | 15 | 1 | 5 | 9 | 0 |
| src/cdc/delivery_tracker.cpp | 11 | 0 | 4 | 7 | 0 |
| src/cdc/cross_collection_stream.cpp | 10 | 0 | 3 | 7 | 0 |
| src/cdc/cdc_ws_handler.cpp | 9 | 0 | 2 | 7 | 0 |
| src/cdc/changefeed_buffer.cpp | 7 | 2 | 5 | 0 | 0 |
| src/cdc/cdc_materialized_view.cpp | 4 | 0 | 2 | 2 | 0 |
| src/cdc/kafka_cdc_producer.cpp | 2 | 0 | 2 | 0 | 0 |

## Full Scanner Findings

### src/cdc/changefeed.cpp
Total findings: 58

- Line 42: severity=CRITICAL; category=missing_dtor
  Description: Class SequenceIncrementOperator allocates resources but has no destructor
  Remediation: Add explicit destructor: ~SequenceIncrementOperator() { /* cleanup */ }
  Context: class/struct SequenceIncrementOperator
- Line 47: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (existing_value != nullptr && !existing_value->empty()) {
- Line 48: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (existing_value->size() == sizeof(uint64_t)) {
- Line 54: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: base = std::stoull(std::string(existing_value->data(),
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
- Line 37: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // practice).  Handles legacy decimal-string base values for backward
  Confidence: band=high; score=0.8
- Line 38: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // compatibility with existing deployments.
  Confidence: band=high; score=0.8
- Line 47: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (existing_value != nullptr && !existing_value->empty()) {
- Line 48: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: if (existing_value->size() == sizeof(uint64_t)) {
- Line 50: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: memcpy(&base, existing_value->data(), sizeof(uint64_t));
- Line 52: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Legacy decimal-string format (backward compatibility)
  Confidence: band=high; score=0.8
- Line 54: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: base = std::stoull(std::string(existing_value->data(),
- Line 67: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: if (value.size() == sizeof(uint64_t)) {
- Line 68: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: memcpy(&delta, value.data(), sizeof(uint64_t));
- Line 72: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: new_value->resize(sizeof(uint64_t));
- Line 73: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: memcpy(&(*new_value)[0], &result, sizeof(uint64_t));
- Line 113: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["metadata"]     = metadata;
- Line 194: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: if (seq_value.size() == sizeof(uint64_t)) {
- Line 199: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Legacy decimal-string format (backward compatibility)
  Confidence: band=high; score=0.8
- Line 259: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Changefeed: db cannot be null");
- Line 310: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: snprintf(buf, sizeof(buf), "%s%020llu", KEY_PREFIX, (unsigned long long)sequence);
  Confidence: band=very_high; score=0.9
- Line 317: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: const uint64_t seq = sequence_counter_.fetch_add(1, std::memory_order_relaxed) + 1;
- Line 324: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (sequence_merge_supported_.load(std::memory_order_acquire)) {
- Line 349: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: uint64_t persisted = persisted_sequence_.load(std::memory_order_acquire);
- Line 360: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::string seq_value(sizeof(uint64_t), '\0');
- Line 361: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(seq_value.data(), &seq, sizeof(uint64_t));
- Line 405: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::eventRecordFailed(s.ToString());
- Line 477: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Multi-type filter takes precedence; fall back to legacy single-type filter
  Confidence: band=high; score=0.8
- Line 505: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: return sequence_counter_.load(std::memory_order_relaxed);
- Line 525: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(50));
- Line 620: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::dbOperationFailed("getEvent", s.ToString());
- Line 716: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: event = nullptr;
  Context: THEMIS_WARN("compactByKey: failed to delete event {}: {}", k, s.ToString());
- Line 738: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::invalidArgument("redactByKeyPrefix: key_prefix cannot be empty");
- Line 1114: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::seconds(ERROR_RETRY_DELAY_SECONDS));
- Line 1156: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (subscription_count_.load(std::memory_order_acquire) == 0) {
- Line 184: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status s;
- Line 328: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status s;
- Line 363: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status persist_status;
- Line 395: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status s;
- Line 580: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status s;
- Line 611: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status s;
- Line 701: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: bool is_delete = (ev.type == ChangeEventType::EVENT_DELETE);
- Line 705: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status s;
- Line 716: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: THEMIS_WARN("compactByKey: failed to delete event {}: {}", k, s.ToString());
- Line 788: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status s;
- Line 801: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.affected_keys.push_back(affected_key);
- Line 839: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status s;
- Line 941: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status s;
- Line 995: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: size_t to_delete         = stats.total_events - policy.max_event_count;
- Line 1011: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: size_t events_to_delete                       = (excess_bytes / avg_event_size) + SIZE_RETENTION_BUF
- Line 1166: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: snapshot.push_back(entry);
  Confidence: band=high; score=0.74
- Line 1167: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: snapshot.push_back(entry);

### src/cdc/consumer_group.cpp
Total findings: 50

- Line 442: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator git may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto git = inflight_.find(group_id);
- Line 542: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator git may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto git = inflight_.find(group_id);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 55: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::invalidArgument("ConsumerGroupManager: db cannot be null");
- Line 87: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::invalidArgument("Consumer group not found: " + group_id);
- Line 90: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::dbOperationFailed("Get group config", s.ToString());
- Line 115: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::dbOperationFailed("Get group offset", s.ToString());
- Line 141: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::dbOperationFailed("Put group config", s.ToString());
- Line 157: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw CDCException(ErrorCode::DB_WRITE_FAILED, ErrorSeverity::ERROR, "Failed to commit group offset"
- Line 168: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::invalidArgument("group_id", "must not be empty");
- Line 171: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::invalidArgument("consumer_count", "must be >= 1");
- Line 181: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::invalidArgument("group_id", "must not be empty");
- Line 199: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::dbOperationFailed("Delete group config", s1.ToString());
- Line 202: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::dbOperationFailed("Delete group offset", s2.ToString());
- Line 233: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::invalidArgument("group_id", "must not be empty");
- Line 241: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::invalidArgument("group_id", "must not be empty");
- Line 267: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (it->Seek(prefix); it->Valid(); it->Next()) {
- Line 293: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::invalidArgument("group_id", "must not be empty");
- Line 304: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::invalidArgument("group_id", "must not be empty");
- Line 329: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::invalidArgument("group_id", "must not be empty");
- Line 340: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::invalidArgument("group_id", "must not be empty");
- Line 353: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::invalidArgument("group_id", "must not be empty");
- Line 370: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::invalidArgument("group_id", "must not be empty");
- Line 421: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::invalidArgument("group_id", "must not be empty");
- Line 424: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::invalidArgument("consumer_id", "must not be empty");
- Line 488: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto &ev : all_events) {
  Confidence: band=very_high; score=0.9
- Line 508: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(overdue_seqs.begin(), overdue_seqs.end(), rec.sequence) != overdue_seqs.end()) {
- Line 519: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto &rec : new_records) {
  Confidence: band=very_high; score=0.9
- Line 530: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::invalidArgument("group_id", "must not be empty");
- Line 533: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::invalidArgument("consumer_id", "must not be empty");
- Line 78: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status s;
- Line 132: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status s;
- Line 187: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status s1, s2;
- Line 220: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status s;
- Line 278: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: groups.push_back(std::move(gid));
  Confidence: band=high; score=0.74
- Line 279: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: groups.push_back(std::move(gid));
- Line 402: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(ev));
  Confidence: band=high; score=0.74
- Line 403: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(ev));
- Line 449: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: overdue_seqs.push_back(rec.sequence);
- Line 466: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(changefeed.getEvent(seq));
  Confidence: band=high; score=0.74
- Line 467: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(changefeed.getEvent(seq));
- Line 494: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: new_records.push_back({ev.sequence, now, 1});
  Confidence: band=high; score=0.74
- Line 495: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: new_records.push_back({ev.sequence, now, 1});
- Line 496: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(ev));
- Line 519: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: consumer_inflight.push_back(std::move(rec));
  Confidence: band=high; score=0.74
- Line 519: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: consumer_inflight.push_back(std::move(rec));
  Confidence: band=high; score=0.74
- Line 520: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: consumer_inflight.push_back(std::move(rec));

### src/cdc/tenant_buffer_manager.cpp
Total findings: 34

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 178: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: THEMIS_INFO("Created new tenant: {}", config.tenant_id);
- Line 319: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = tenant_buffers_.find(tenant_id);
- Line 355: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: THEMIS_INFO("Auto-created buffer for new tenant: {}", tenant_id);
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
- Line 21: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::invalidArgument("changefeed", "Cannot be null");
- Line 36: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto &[tenant_id, state] : tenant_buffers_) {
  Confidence: band=very_high; score=0.9
- Line 36: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (auto &[tenant_id, state] : tenant_buffers_) {
- Line 53: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto &[tenant_id, state] : tenant_buffers_) {
  Confidence: band=very_high; score=0.9
- Line 53: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (auto &[tenant_id, state] : tenant_buffers_) {
- Line 64: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw CDCException(ErrorCode::BUFFER_NOT_RUNNING, ErrorSeverity::ERROR, "TenantBufferManager not run
- Line 69: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::invalidArgument("tenant_id", "Cannot be empty");
- Line 79: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw CDCException(ErrorCode::TENANT_UNAUTHORIZED, ErrorSeverity::ERROR, "Tenant is disabled",
- Line 87: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw CDCException(ErrorCode::TENANT_QUOTA_EXCEEDED, ErrorSeverity::WARNING, "Tenant quota exceeded"
- Line 94: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto recorded_event = state.buffer->recordEvent(std::move(event));
- Line 138: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto &[tenant_id, state] : tenant_buffers_) {
  Confidence: band=very_high; score=0.9
- Line 138: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (auto &[tenant_id, state] : tenant_buffers_) {
- Line 152: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::invalidArgument("tenant_id", "Cannot be empty");
- Line 167: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto [new_it, inserted] = tenant_buffers_.try_emplace(config.tenant_id);
- Line 208: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return std::cref(it->second.buffer->getMetrics());
- Line 224: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (const auto &[tenant_id, state] : tenant_buffers_) {
- Line 257: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &[tenant_id, state] : tenant_buffers_) {
  Confidence: band=very_high; score=0.9
- Line 257: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (const auto &[tenant_id, state] : tenant_buffers_) {
- Line 270: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &[tenant_id, state] : tenant_buffers_) {
  Confidence: band=very_high; score=0.9
- Line 344: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto [inserted_it, inserted] = tenant_buffers_.try_emplace(tenant_id);
- Line 400: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const auto &metrics      = state.buffer->getMetrics();
- Line 253: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, TenantStats> TenantBufferManager::getAllTenantStats() const {
  Confidence: band=high; score=0.74
- Line 256: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, TenantStats> all_stats;
  Confidence: band=high; score=0.74
- Line 271: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tenants.push_back(tenant_id);
  Confidence: band=high; score=0.74
- Line 272: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tenants.push_back(tenant_id);

### src/cdc/outbox.cpp
Total findings: 21

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 91: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["metadata"]        = metadata;
- Line 121: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::invalidArgument("OutboxWriter: db cannot be null");
- Line 127: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(buf, sizeof(buf), "%s%020llu", KEY_PREFIX, static_cast<unsigned long long>(seq));
  Confidence: band=very_high; score=0.9
- Line 166: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw CDCException(ErrorCode::DB_WRITE_FAILED, ErrorSeverity::CRITICAL,
- Line 178: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::invalidArgument("OutboxWriter::writeToOutbox: txn cannot be null");
- Line 181: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::invalidArgument("OutboxWriter::writeToOutbox: record key cannot be empty");
- Line 204: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw CDCException(ErrorCode::DB_WRITE_FAILED, ErrorSeverity::ERROR,
- Line 220: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::invalidArgument("OutboxRelay: db cannot be null");
- Line 230: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(buf, sizeof(buf), "%s%020llu", KEY_PREFIX, static_cast<unsigned long long>(seq));
  Confidence: band=very_high; score=0.9
- Line 254: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::unique_ptr<rocksdb::Iterator> it(cf_ ? db_->NewIterator(read_opts, cf_) : db_->NewIterator(read
- Line 254: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::unique_ptr<rocksdb::Iterator> it(cf_ ? db_->NewIterator(read_opts, cf_) : db_->NewIterator(read
- Line 341: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (running_.load(std::memory_order_acquire)) {
- Line 345: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: cv_.wait_for(lock, config_.poll_interval, [this] { return !running_.load(std::memory_order_acquire);
- Line 136: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status s;
- Line 159: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status ws;
- Line 196: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status s;
- Line 239: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status s;
- Line 266: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(rec));
  Confidence: band=high; score=0.74
- Line 267: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(rec));
- Line 384: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status s;

### src/cdc/cdc_admin.cpp
Total findings: 19

- Line 30: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::invalidArgument("Changefeed cannot be null");
- Line 40: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::invalidArgument("TenantBufferManager cannot be null");
- Line 51: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::internalError("No changefeed available for purge");
- Line 73: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("CDC Admin: Purging sequence range [{}, {}]", start_sequence, end_sequence);
- Line 81: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::internalError("No changefeed available for purge");
- Line 106: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::internalError("No changefeed available for purge");
- Line 122: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::internalError("No tenant manager available for tenant purge");
- Line 126: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::invalidArgument("Tenant ID cannot be empty");
- Line 170: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::internalError("No changefeed available for replay");
- Line 275: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::invalidArgument(
- Line 283: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::invalidArgument(
- Line 294: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::internalError("No changefeed available for compaction");
- Line 316: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::internalError("No changefeed available for GDPR redaction");
- Line 320: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::invalidArgument("redactByKeyPrefix: key_prefix cannot be empty");
- Line 327: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: Changefeed::RedactionResult inner = changefeed_->redactByKeyPrefix(key_prefix);
- Line 415: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::internalError("No changefeed available for retention status");
- Line 188: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: HealthStatus status;
- Line 254: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: total_events, health.is_healthy ? "OK" : "ISSUES");
- Line 418: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: RetentionStatus status;

### src/cdc/ws_transport.cpp
Total findings: 17

- Line 56: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = sessions_.find(session_id);
- Line 92: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = sessions_.find(session_id);
- Line 234: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it->second.pending_events >= delivered) {
- Line 235: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: it->second.pending_events -= delivered;
- Line 237: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: it->second.pending_events = 0;
- Line 295: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: poll_timer_->async_wait([this](const boost::system::error_code& ec) {
- Line 177: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto sit = sessions_.find(result.session_id);
- Line 183: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto sub_it = session.subscriptions.find(result.sub_id);
- Line 210: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& event : delivery.events) {
  Confidence: band=very_high; score=0.9
- Line 132: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: query_items.push_back({sid, sub_id, std::move(opts)});
  Confidence: band=high; score=0.74
- Line 132: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: query_items.push_back({sid, sub_id, std::move(opts)});
  Confidence: band=high; score=0.74
- Line 133: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: query_items.push_back({sid, sub_id, std::move(opts)});
- Line 153: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({item.session_id, item.sub_id, std::move(events)});
  Confidence: band=high; score=0.74
- Line 154: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({item.session_id, item.sub_id, std::move(events)});
- Line 196: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: overflow_sessions.push_back(result.session_id);
  Confidence: band=high; score=0.74
- Line 197: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: overflow_sessions.push_back(result.session_id);
- Line 204: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: deliveries.push_back({result.session_id, result.sub_id, std::move(result.events)});

### src/cdc/dead_letter_queue.cpp
Total findings: 15

- Line 166: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: THEMIS_INFO("DeadLetterQueue: replayed dlq_seq={} → new seq={}",
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 56: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(buf, sizeof(buf), "%s%020llu",
  Confidence: band=very_high; score=0.9
- Line 90: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw error::sequenceGenerationFailed(write_status.ToString());
- Line 223: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: it->Seek(prefix_start);
- Line 242: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: key = nullptr;
  Context: THEMIS_WARN("DeadLetterQueue: drain failed to delete key={}: {}",
- Line 66: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status s;
- Line 81: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status write_status;
- Line 112: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status s;
- Line 135: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status s;
- Line 175: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status read_status;
- Line 193: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status s;
- Line 232: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status s;
- Line 242: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: THEMIS_WARN("DeadLetterQueue: drain failed to delete key={}: {}",
- Line 277: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(entry));

### src/cdc/delivery_tracker.cpp
Total findings: 11

- Line 73: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& ev : events) {
  Confidence: band=very_high; score=0.9
- Line 175: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (uint64_t seq : to_expire) {
  Confidence: band=very_high; score=0.9
- Line 220: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [consumer_id, state] : consumers_) {
  Confidence: band=very_high; score=0.9
- Line 245: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(cv_mutex_);
- Line 163: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_expire.push_back(seq);
  Confidence: band=high; score=0.74
- Line 164: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: to_expire.push_back(seq);
- Line 167: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: to_redeliver.push_back(pending.event);
- Line 227: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(stats));
  Confidence: band=high; score=0.74
- Line 228: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(stats));
- Line 276: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: consumer_ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 277: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: consumer_ids.push_back(id);

### src/cdc/cross_collection_stream.cpp
Total findings: 10

- Line 49: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 54: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 121: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto cursor_it = options.from_sequence.find(name);
- Line 67: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 68: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: names.push_back(kv.first);
- Line 79: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Comparator: sort by (timestamp_ms ASC, collection ASC, sequence ASC).
  Confidence: band=high; score=0.74
- Line 102: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, Changefeed*> feeds_snapshot;
  Confidence: band=medium; score=0.66
- Line 141: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: all_events.push_back({name, std::move(ev)});
  Confidence: band=high; score=0.74
- Line 142: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: all_events.push_back({name, std::move(ev)});
- Line 182: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, Changefeed*> feeds_snapshot;
  Confidence: band=medium; score=0.66

### src/cdc/cdc_ws_handler.cpp
Total findings: 9

- Line 190: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Legacy id-based ack.
  Confidence: band=high; score=0.8
- Line 342: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto &pending : sub.pending_ack) {
  Confidence: band=very_high; score=0.9
- Line 192: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: responses.push_back({{"action", "error"}, {"message", "ack requires 'id' or 'group_id'"}});
- Line 230: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: responses.push_back({{"action", "error"}, {"message", "unknown action: " + action}});
- Line 304: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sub.pending_ack.push_back({event_frame, now});
- Line 306: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: frames.push_back(std::move(event_frame));
- Line 343: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: frames.push_back(pending.frame);
  Confidence: band=high; score=0.74
- Line 343: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: frames.push_back(pending.frame);
  Confidence: band=high; score=0.74
- Line 344: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: frames.push_back(pending.frame);

### src/cdc/changefeed_buffer.cpp
Total findings: 7

- Line 147: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 202: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 25: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("ChangefeedBuffer: changefeed cannot be null");
- Line 151: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto& buffer = buffers_[event.type];
- Line 212: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (auto& [event_type, buffer] : buffers_) {
- Line 260: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::vector<uint8_t> compressed_data(event.value->begin(), event.value->end());
- Line 395: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(flush_mutex_);

### src/cdc/cdc_materialized_view.cpp
Total findings: 4

- Line 88: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: CDCMaterializedViewMaintainer::query(const std::string &view_name,
- Line 91: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return view_manager_.query(view_name, filters, limit, offset);
- Line 73: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: records.push_back(std::move(rec));
  Confidence: band=high; score=0.74
- Line 74: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: records.push_back(std::move(rec));

### src/cdc/kafka_cdc_producer.cpp
Total findings: 2

- Line 93: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (running_.load(std::memory_order_acquire)) {
- Line 308: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (running_.load(std::memory_order_acquire)) {

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
