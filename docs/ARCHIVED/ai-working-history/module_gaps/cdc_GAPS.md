# cdc Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: cdc
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 125
- Actionable Findings (Critical + High): 101
- Affected Files: 14

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 18 |
| High | 83 |
| Medium | 18 |
| Low | 6 |

## Category Summary

| Category | Count |
|---|---:|
| resource_leaked_in_exception | 17 |
| size_assumption | 9 |
| uncaught_exception | 8 |
| db_connection_leak | 7 |
| delete_without_nullptr | 7 |
| explicit_delete | 7 |
| uninitialized_access | 7 |
| legacy_or_compat_path | 6 |
| thread_join_no_timeout | 5 |
| copy_overhead | 4 |
| hardcoded_output | 4 |
| manual_cleanup | 4 |
| data_race | 3 |
| o_n_squared | 3 |
| pointer_arithmetic_unbounded | 3 |
| smart_ptr_misuse | 3 |
| blocking_no_timeout | 2 |
| delete_no_nullptr | 2 |
| exception_in_destructor | 2 |
| lock_contention | 2 |
| map_vs_unordered_map | 2 |
| memory_order | 2 |
| module_doc_linkset_drift | 2 |
| no_timeout | 2 |
| primitive_no_volatile | 2 |
| range_temporary | 2 |
| unordered_container_iter | 2 |
| explicit_lock_unlock | 1 |
| generic_catch | 1 |
| missing_dtor | 1 |
| repeated_search | 1 |
| stale_doc_section_reference | 1 |
| timestamp_sorting_unstable | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| cdc/changefeed.cpp | 48 | 5 | 36 | 6 | 1 |
| cdc/consumer_group.cpp | 13 | 0 | 11 | 2 | 0 |
| cdc/outbox.cpp | 10 | 2 | 5 | 1 | 2 |
| cdc/tenant_buffer_manager.cpp | 10 | 3 | 5 | 2 | 0 |
| cdc/changefeed_buffer.cpp | 9 | 5 | 4 | 0 | 0 |
| cdc/cdc_admin.cpp | 8 | 0 | 8 | 0 | 0 |
| cdc/dead_letter_queue.cpp | 8 | 1 | 5 | 1 | 1 |
| cdc/kafka_cdc_producer.cpp | 5 | 1 | 3 | 1 | 0 |
| cdc/cross_collection_stream.cpp | 4 | 0 | 1 | 3 | 0 |
| cdc/ws_transport.cpp | 4 | 0 | 3 | 1 | 0 |
| cdc/delivery_tracker.cpp | 3 | 1 | 1 | 1 | 0 |
| cdc/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| cdc/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| cdc/cdc_ws_handler.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### cdc/changefeed.cpp
Total findings: 48

- Line 43: severity=CRITICAL; category=missing_dtor
  Description: Class SequenceIncrementOperator allocates resources but has no destructor
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct SequenceIncrementOperator
- Line 48: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (existing_value != nullptr && !existing_value->empty()) {
- Line 49: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (existing_value->size() == sizeof(uint64_t)) {
- Line 55: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: base = std::stoull(std::string(existing_value->data(),
- Line 1076: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: retention_thread_.join();
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4325 [Issue] Implement DiffEngin... (2026-03-19) | #4294 docs(cdc): audit v1
- Line 38: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // practice).  Handles legacy decimal-string base values for backward
- Line 39: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // compatibility with existing deployments.
- Line 46: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 49: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: if (existing_value->size() == sizeof(uint64_t)) {
- Line 50: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 51: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: memcpy(&base, existing_value->data(), sizeof(uint64_t));
- Line 53: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Legacy decimal-string format (backward compatibility)
- Line 68: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: if (value.size() == sizeof(uint64_t)) {
- Line 69: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: memcpy(&delta, value.data(), sizeof(uint64_t));
- Line 73: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 73: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: new_value->resize(sizeof(uint64_t));
- Line 74: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 74: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: memcpy(&(*new_value)[0], &result, sizeof(uint64_t));
- Line 195: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: if (seq_value.size() == sizeof(uint64_t)) {
- Line 200: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Legacy decimal-string format (backward compatibility)
- Line 317: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const uint64_t seq = sequence_counter_.fetch_add(1, std::memory_order_relaxed) + 1;
- Line 324: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (sequence_merge_supported_.load(std::memory_order_acquire)) {
- Line 349: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: uint64_t persisted = persisted_sequence_.load(std::memory_order_acquire);
- Line 360: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::string seq_value(sizeof(uint64_t), '\0');
- Line 361: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::memcpy(seq_value.data(), &seq, sizeof(uint64_t));
- Line 422: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 477: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Multi-type filter takes precedence; fall back to legacy single-type filter
- Line 506: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: return sequence_counter_.load(std::memory_order_relaxed);
- Line 526: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(50));
- Line 667: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

    }



    // Phase 2: Re-scan and delete any event that is NOT the latest for its key,

    // unless it is a DELETE event (tombstone must be preserved for consumers).

    {

        std::unique_ptr<rocksdb::Iterator> it;
- Line 667: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Phase 2: Re-scan and delete any event that is NOT the latest for its key,
- Line 717: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: THEMIS_WARN("compactByKey: failed to delete event {}: {}", k, s.ToString());
- Line 717: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.events_deleted++;

                        compacted_keys.insert(ev.key);

                    } else {

                        THEMIS_WARN("compactByKey: failed to delete event {}: {}", k, s.ToString());

                        result.events_retained++;

                    }

                } else {
- Line 717: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_WARN("compactByKey: failed to delete event {}: {}", k, s.ToString());
- Line 788: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 893: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1006: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Apply size-based retention

    if (stats.total_size_bytes > policy.max_size_bytes) {

        // Estimate how many events to delete based on average event size

        size_t avg_event_size = stats.total_events > 0 ? (stats.total_size_bytes / stats.total_events) : 1024;

        size_t excess_bytes   = stats.total_size_bytes - policy.max_size_bytes;
- Line 1006: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Estimate how many events to delete based on average event size
- Line 1115: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::seconds(ERROR_RETRY_DELAY_SECONDS));
- Line 1157: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (subscription_count_.load(std::memory_order_acquire) == 0) {
- Line 61: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: base = 0;

                } catch (const char*) {

                    base = 0;

                } catch (...) {

                    base = 0;

                }

            }
- Line 61: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 717: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: THEMIS_WARN("compactByKey: failed to delete event {}: {}", k, s.ToString());
- Line 996: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: size_t to_delete         = stats.total_events - policy.max_event_count;
- Line 1012: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: size_t events_to_delete                       = (excess_bytes / avg_event_size) + SIZE_RETENTION_BUF
- Line 1087: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: constexpr int ERROR_RETRY_DELAY_SECONDS = 60;
- Line 310: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: snprintf(buf, sizeof(buf), "%s%020llu", KEY_PREFIX, static_cast<unsigned long long>(sequence));

### cdc/consumer_group.cpp
Total findings: 13

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4239 feat(cdc): Consumer Group S... (2026-03-15) | #3106 [cdc] Add event thr
- Line 56: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: ConsumerGroupManager::ConsumerGroupManager(rocksdb::TransactionDB *db, rocksdb::ColumnFamilyHandle *cf)

    : db_(db), cf_(cf) {

    if (!db_) {

        throw error::invalidArgument("ConsumerGroupManager: db cannot be null");

    }

}
- Line 341: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: bool ConsumerGroupManager::consumerHandlesKey(const std::string &group_id, const std::string &consumer_id,

                                              const std::string &event_key) const {

    if (group_id.empty()) {

        throw error::invalidArgument("group_id", "must not be empty");

    }



    std::lock_guard<std::mutex> lock(mutex_);
- Line 354: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: uint32_t ConsumerGroupManager::getPartitionForKey(const std::string &group_id, const std::string &key) const {

    if (group_id.empty()) {

        throw error::invalidArgument("group_id", "must not be empty");

    }



    std::lock_guard<std::mutex> lock(mutex_);
- Line 422: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: ConsumerGroupManager::fetchEventsAtLeastOnce(const std::string &group_id, const std::string &consumer_id,

                                             const Changefeed &changefeed, size_t limit, uint32_t ack_timeout_ms) {

    if (group_id.empty()) {

        throw error::invalidArgument("group_id", "must not be empty");

    }

    if (consumer_id.empty()) {

        throw error::invalidArgument("consumer_id", "must not be empty");
- Line 425: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: throw error::invalidArgument("group_id", "must not be empty");

    }

    if (consumer_id.empty()) {

        throw error::invalidArgument("consumer_id", "must not be empty");

    }



    const size_t effective_limit = (limit == 0) ? 100 : limit;
- Line 474: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 496: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 502: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 509: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(overdue_seqs.begin(), overdue_seqs.end(), rec.sequence) != overdue_seqs.end()) {
- Line 520: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 450: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: overdue_seqs.push_back(rec.sequence);
- Line 496: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: new_records.push_back({ev.sequence, now, 1});

### cdc/outbox.cpp
Total findings: 10

- Line 225: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 365: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: relay_thread_.join();
- Line 122: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: OutboxWriter::OutboxWriter(rocksdb::TransactionDB *db, rocksdb::ColumnFamilyHandle *cf) : db_(db), cf_(cf) {

    if (!db_) {

        throw error::invalidArgument("OutboxWriter: db cannot be null");

    }

}
- Line 221: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: OutboxRelayConfig config)

    : db_(db), cf_(cf), changefeed_(changefeed), config_(std::move(config)) {

    if (!db_) {

        throw error::invalidArgument("OutboxRelay: db cannot be null");

    }

}
- Line 299: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: event.value    = rec.value;

            event.metadata = rec.metadata;

            if (!rec.collection.empty()) {

                event.metadata["collection"] = rec.collection;

            }

            event.timestamp_ms = rec.created_at_ms;
- Line 342: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: while (running_.load(std::memory_order_acquire)) {
- Line 346: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: cv_.wait_for(lock, config_.poll_interval, [this] { return !running_.load(std::memory_order_acquire);
- Line 351: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool expected = false;
- Line 128: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(buf, sizeof(buf), "%s%020llu", KEY_PREFIX, static_cast<unsigned long long>(seq));
- Line 231: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(buf, sizeof(buf), "%s%020llu", KEY_PREFIX, static_cast<unsigned long long>(seq));

### cdc/tenant_buffer_manager.cpp
Total findings: 10

- Line 26: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 179: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: THEMIS_INFO("Created new tenant: {}", config.tenant_id);
- Line 356: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: THEMIS_INFO("Auto-created buffer for new tenant: {}", tenant_id);
- Line 167: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 168: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 169: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 344: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 356: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 254: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, TenantStats> TenantBufferManager::getAllTenantStats() const {
- Line 257: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, TenantStats> all_stats;

### cdc/changefeed_buffer.cpp
Total findings: 9

- Line 65: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: flush_thread_.join();
- Line 148: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Unlock before flush to avoid deadlock, flush will re-acquire lock

            lock.unlock();

            flushInternal(false);

            lock.lock();

        }

        

        // Add to buffer
- Line 148: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: lock.lock();
- Line 203: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::unique_lock<std::mutex> lock(buffers_mutex_, std::defer_lock);

    if (!lock_held) {

        lock.lock();

    }

    

    if (buffers_.empty()) {
- Line 203: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: lock.lock();
- Line 112: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: auto compressed = utils::zstd_compress(*event.value, 3);

                if (!compressed.empty() && compressed.size() < payload_size) {

                    event.value = std::string(compressed.begin(), compressed.end());

                    event.metadata["_compressed"] = true;

                    stats_.compressed_payloads++;

                    metrics_.compression_count++;
- Line 148: severity=HIGH; category=explicit_lock_unlock
  Description: Explicit lock/unlock without RAII guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: lock.lock();
- Line 255: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: try {

                // Decompress if needed

                Changefeed::ChangeEvent event = buffered_event.event;

                if (event.metadata.contains("_compressed") && event.metadata["_compressed"] == true) {

                    if (event.value.has_value()) {

                        {

#pragma warning(suppress: 4456)
- Line 396: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(flush_mutex_);

### cdc/cdc_admin.cpp
Total findings: 8

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4833 Continue Phase-6 tensorgrap... (2026-05-07) | #3687 feat(cdc): runtime-
- Line 74: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_INFO("CDC Admin: Purging sequence range [{}, {}]", start_sequence, end_sequence);
- Line 88: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Delete events up to end_sequence (exclusive, so add 1)

    uint64_t total_deleted = changefeed_->deleteOldEvents(end_sequence + 1);

    

    // If start_sequence > 0, we deleted too many, but RocksDB delete is by prefix

    // For now, we delete everything up to end_sequence

    // A proper implementation would iterate and delete specific keys

    result.events_deleted = total_deleted;
- Line 88: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // If start_sequence > 0, we deleted too many, but RocksDB delete is by prefix
- Line 89: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: uint64_t total_deleted = changefeed_->deleteOldEvents(end_sequence + 1);

    

    // If start_sequence > 0, we deleted too many, but RocksDB delete is by prefix

    // For now, we delete everything up to end_sequence

    // A proper implementation would iterate and delete specific keys

    result.events_deleted = total_deleted;
- Line 89: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // For now, we delete everything up to end_sequence
- Line 90: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // If start_sequence > 0, we deleted too many, but RocksDB delete is by prefix

    // For now, we delete everything up to end_sequence

    // A proper implementation would iterate and delete specific keys

    result.events_deleted = total_deleted;

    

    auto end = steady_clock::now();
- Line 90: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // A proper implementation would iterate and delete specific keys

### cdc/dead_letter_queue.cpp
Total findings: 8

- Line 167: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: THEMIS_INFO("DeadLetterQueue: replayed dlq_seq={} → new seq={}",
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3552 docs(cdc): full module docu... (2026-03-12) | #2796 [cdc] Dead-letter q
- Line 167: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 243: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: THEMIS_WARN("DeadLetterQueue: drain failed to delete key={}: {}",
- Line 243: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (s.ok()) {

            ++deleted;

        } else {

            THEMIS_WARN("DeadLetterQueue: drain failed to delete key={}: {}",

                        key, s.ToString());

        }
- Line 243: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_WARN("DeadLetterQueue: drain failed to delete key={}: {}",
- Line 243: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: THEMIS_WARN("DeadLetterQueue: drain failed to delete key={}: {}",
- Line 57: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(buf, sizeof(buf), "%s%020llu",

### cdc/kafka_cdc_producer.cpp
Total findings: 5

- Line 171: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: thread_.join();
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4607 feat(cdc): register CDCKafk... (2026-04-13) | #3106 [cdc] Add event thr
- Line 94: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (running_.load(std::memory_order_acquire)) {
- Line 309: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: while (running_.load(std::memory_order_acquire)) {
- Line 35: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Kafka CDC Producer Activation' that was not found in 'src/cdc/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/cdc/FUTURE_ENHANCEMENTS.md §"Kafka CDC Producer Activation"

### cdc/cross_collection_stream.cpp
Total findings: 4

- Line 122: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto cursor_it = options.from_sequence.find(name);
- Line 80: severity=MEDIUM; category=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: // Comparator: sort by (timestamp_ms ASC, collection ASC, sequence ASC).
- Line 103: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, Changefeed*> feeds_snapshot;
- Line 183: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, Changefeed*> feeds_snapshot;

### cdc/ws_transport.cpp
Total findings: 4

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4184 feat(cdc): WebSocket Change... (2026-03-13) | #3616 fix(cdc): build sys
- Line 178: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto sit = sessions_.find(result.session_id);
- Line 184: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto sub_it = session.subscriptions.find(result.sub_id);
- Line 198: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: overflow_sessions.push_back(result.session_id);

### cdc/delivery_tracker.cpp
Total findings: 3

- Line 47: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: redelivery_thread_.join();
- Line 246: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(cv_mutex_);
- Line 168: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: to_redeliver.push_back(pending.event);

### cdc/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### cdc/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### cdc/cdc_ws_handler.cpp
Total findings: 1

- Line 191: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Legacy id-based ack.

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
