# cache Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: cache
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 187
- Actionable Findings (Critical + High): 151
- Affected Files: 14

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 19 |
| High | 132 |
| Medium | 33 |
| Low | 3 |

## Category Summary

| Category | Count |
|---|---:|
| null_dereference | 57 |
| deadlock_risk | 15 |
| duplicate_qualified_signature | 14 |
| lock_contention | 8 |
| resource_leaked_in_exception | 8 |
| explicit_delete | 7 |
| primitive_no_volatile | 7 |
| range_temporary | 7 |
| delete_without_nullptr | 6 |
| lock_in_loop | 5 |
| data_race | 4 |
| no_timeout | 4 |
| thread_join_no_timeout | 4 |
| blocking_no_timeout | 3 |
| manual_cleanup | 3 |
| missing_dtor | 3 |
| stale_doc_section_reference | 3 |
| delete_no_nullptr | 2 |
| legacy_or_compat_path | 2 |
| missing_trace_point | 2 |
| module_doc_linkset_drift | 2 |
| nested_loop_find | 2 |
| no_retry_logic | 2 |
| unordered_container_iter | 2 |
| unspecified_consistency | 2 |
| command_injection | 1 |
| db_connection_leak | 1 |
| explicit_lock_unlock | 1 |
| generic_catch | 1 |
| memory_order | 1 |
| missing_consensus | 1 |
| missing_latency_metric | 1 |
| o_n_squared | 1 |
| stale_read_undocumented | 1 |
| uncaught_exception | 1 |
| uninitialized_access | 1 |
| uninitialized_array | 1 |
| unstructured_log | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| cache/adaptive_query_cache.cpp | 69 | 5 | 62 | 1 | 1 |
| cache/distributed_cache_coordinator.cpp | 27 | 4 | 7 | 16 | 0 |
| cache/bounded_lru_cache.cpp | 25 | 0 | 25 | 0 | 0 |
| cache/redis_cache_coordinator.cpp | 22 | 1 | 11 | 10 | 0 |
| cache/predictive_prefetcher.cpp | 14 | 0 | 14 | 0 | 0 |
| cache/cache_replication_coordinator.cpp | 10 | 3 | 6 | 1 | 0 |
| cache/embedding_cache.cpp | 7 | 3 | 3 | 1 | 0 |
| cache/semantic_cache.cpp | 5 | 1 | 2 | 2 | 0 |
| cache/grpc_remote_cache_peer.cpp | 2 | 0 | 1 | 1 | 0 |
| cache/warmup.cpp | 2 | 0 | 1 | 1 | 0 |
| cache/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| cache/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| cache/cache_hit_rate_slo_monitor.cpp | 1 | 1 | 0 | 0 | 0 |
| cache/cache_replication.cpp | 1 | 1 | 0 | 0 | 0 |

## Full Scanner Findings

### cache/adaptive_query_cache.cpp
Total findings: 69

- Line 117: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: if (!l3_db_->open()) {
- Line 973: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: count++;

                }



                lock.lock();

                if (l3_circuit_breaker_) {

                    l3_circuit_breaker_->recordSuccess();

                    enhanced_metrics_.l3_circuit_breaker_open = false;
- Line 973: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: lock.lock();
- Line 982: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: THEMIS_DEBUG("Invalidated {} L3 cache entries", keys_to_delete.size());

            } catch (const std::exception &e) {

                if (!lock.owns_lock()) {

                    lock.lock();

                }

                THEMIS_WARN("Failed to invalidate L3 cache entries: {}", e.what());

                enhanced_metrics_.l3_read_errors++;
- Line 982: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: lock.lock();
- Line 127: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
- Line 213: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (ptr->expired_flag.load(std::memory_order_relaxed)) {
- Line 215: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: } else if (isExpired(ptr->created_at_ms.load(std::memory_order_relaxed),
- Line 216: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ptr->ttl_seconds.load(std::memory_order_relaxed))) {
- Line 219: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (ptr->expired_flag.compare_exchange_strong(expected, true, std::memory_order_relaxed)) {
- Line 226: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ptr->last_accessed_ms.store(now_ms, std::memory_order_relaxed);
- Line 227: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: int64_t new_count = ptr->access_count.fetch_add(1, std::memory_order_relaxed) + 1;
- Line 231: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: int64_t ws = ptr->window_start_ms.load(std::memory_order_relaxed);
- Line 234: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (ptr->window_start_ms.compare_exchange_strong(ws, now_ms, std::memory_order_relaxed)) {
- Line 235: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: uint32_t wc = ptr->window_count.exchange(1, std::memory_order_relaxed);
- Line 237: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: int old_ttl = ptr->ttl_seconds.load(std::memory_order_relaxed);
- Line 238: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 240: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (ptr->ttl_seconds.compare_exchange_strong(old_ttl, new_ttl,
- Line 245: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ptr->created_at_ms.store(now_ms, std::memory_order_relaxed);
- Line 248: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: uint32_t wc = ptr->window_count.fetch_add(1, std::memory_order_relaxed) + 1;
- Line 250: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: int old_ttl = ptr->ttl_seconds.load(std::memory_order_relaxed);
- Line 252: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (ptr->ttl_seconds.compare_exchange_strong(old_ttl, new_ttl, std::memory_order_relaxed)) {
- Line 252: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 256: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 257: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ptr->ttl_seconds.store(new_ttl, std::memory_order_relaxed);
- Line 259: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ptr->created_at_ms.store(now_ms, std::memory_order_relaxed);
- Line 274: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: prefetcher_->recordQueryAccess(fingerprint, tenant_id);
- Line 280: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: cache_result.result            = ptr->result;
- Line 282: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: cache_result.created_at_ms     = ptr->created_at_ms.load(std::memory_order_relaxed);
- Line 283: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: cache_result.last_accessed_ms  = ptr->last_accessed_ms.load(std::memory_order_relaxed);
- Line 284: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: cache_result.access_count      = ptr->access_count.load(std::memory_order_relaxed);
- Line 285: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: cache_result.ttl_seconds       = ptr->ttl_seconds.load(std::memory_order_relaxed);
- Line 295: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lock(l2_mutex_);
- Line 360: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: prefetcher_->recordQueryAccess(fingerprint, tenant_id);
- Line 374: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::unique_lock<std::shared_mutex> l1_lock(l1_mutex_);
- Line 423: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lock(l3_mutex_);
- Line 521: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: prefetcher_->recordQueryAccess(fingerprint, tenant_id);
- Line 554: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: prefetcher_->recordQueryAccess(fingerprint, tenant_id);
- Line 628: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::unique_lock<std::shared_mutex> l1_lock(l1_mutex_);
- Line 744: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::unique_lock<std::shared_mutex> lock(l1_mutex_);
- Line 961: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // [C-2] Copy l3_db_ into a local shared_ptr while holding the lock so

                // that the pointer remains valid even if a concurrent circuit-breaker

                // trip resets l3_db_ between the unlock and the bulk delete loop.

                auto local_l3_db = l3_db_;



                // Release lock before issuing bulk deletes so readers are not
- Line 961: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // trip resets l3_db_ between the unlock and the bulk delete loop.
- Line 965: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: auto local_l3_db = l3_db_;



                // Release lock before issuing bulk deletes so readers are not

                // blocked during the I/O-intensive delete phase.

                lock.unlock();



                for (const auto &key : keys_to_delete) {
- Line 965: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // blocked during the I/O-intensive delete phase.
- Line 973: severity=HIGH; category=explicit_lock_unlock
  Description: Explicit lock/unlock without RAII guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: lock.lock();
- Line 1035: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::unique_lock<std::shared_mutex> lock(l1_mutex_);
- Line 1056: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }



    if (l3_db_) {

        // Scan under lock to collect keys, then delete outside lock.

        std::vector<std::string> keys;

        std::vector<std::string> pii_ref_keys;

        try {
- Line 1056: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Scan under lock to collect keys, then delete outside lock.
- Line 1230: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto it = l1_cache_.begin(); it != l1_cache_.end();) {
- Line 1233: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> evict_lock(l1_eviction_mutex_);
- Line 1262: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: double min_score = calculateLRUScore(lru_it->second->last_accessed_ms.load(std::memory_order_relaxed
- Line 1377: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Validate adaptive TTL (legacy + current fields)
- Line 1802: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::unique_lock<std::shared_mutex> lock(l1_mutex_);
- Line 1803: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto it = l1_cache_.begin(); it != l1_cache_.end();) {
- Line 1806: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> evict_lock(l1_eviction_mutex_);
- Line 1949: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::unique_lock<std::shared_mutex> lock(l1_mutex_);
- Line 1950: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (const auto &k : keys_to_purge) {
- Line 1954: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> evict_lock(l1_eviction_mutex_);
- Line 2126: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: prefetcher_->recordQueryAccess(fingerprint, tenant_id);
- Line 2172: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: prefetcher_->loadModel(l3_db_.get());
- Line 2181: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lk(coordinator_mutex_);
- Line 2247: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::unique_lock<std::shared_mutex> lock(l1_mutex_);
- Line 2305: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::unique_lock<std::shared_mutex> lock(l1_mutex_);
- Line 2306: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto it = l1_cache_.begin(); it != l1_cache_.end();) {
- Line 2309: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> evict_lock(l1_eviction_mutex_);
- Line 2377: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::unique_lock<std::shared_mutex> lock(l1_mutex_);
- Line 2428: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: const bool expired = e->expired_flag.load(std::memory_order_acquire);
- Line 1935: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> keys_to_purge;
- Line 1207: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: double log_factor = std::log(static_cast<double>(access_count + 1)) / config_.adaptive_ttl_scaling_factor;

### cache/distributed_cache_coordinator.cpp
Total findings: 27

- Line 294: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: sub_thread_.join();
- Line 424: severity=CRITICAL; category=missing_dtor
  Description: Class addrinfo allocates resources but has no destructor
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct addrinfo
- Line 428: severity=CRITICAL; category=missing_dtor
  Description: Class addrinfo allocates resources but has no destructor
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct addrinfo
- Line 435: severity=CRITICAL; category=missing_dtor
  Description: Class addrinfo allocates resources but has no destructor
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct addrinfo
- Line 84: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: //   Windows SDK builds without POSIX compatibility shims).  The coordinator
- Line 283: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: sub_thread_ = std::thread(&RedisCacheCoordinator::subscriberLoop, this);
- Line 447: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));



        if (::connect(fd, p->ai_addr, p->ai_addrlen) == 0) {

            break; // connected

        }

        ::close(fd);
- Line 627: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(50));
- Line 639: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(50));
- Line 653: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(50));
- Line 683: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(50));
- Line 93: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: //   one node is never propagated to other nodes via Redis pub/sub; stale reads
- Line 100: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Redis Pub/Sub Activation' that was not found in 'src/cache/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/cache/FUTURE_ENHANCEMENTS.md §"Redis Pub/Sub Activation"
- Line 145: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::publishInvalidation(const std::string &pattern, const std::string &tenant_id)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: void RedisCacheCoordinator::publishInvalidation(const std::string &pattern, const std::string &tenant_id) {
- Line 178: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::subscribeEntries(EntryCallback callback)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: void RedisCacheCoordinator::subscribeEntries(EntryCallback callback) {
- Line 183: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::subscribeInvalidations(InvalidationCallback callback)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: void RedisCacheCoordinator::subscribeInvalidations(InvalidationCallback callback) {
- Line 188: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::isConnected()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: bool RedisCacheCoordinator::isConnected() const {
- Line 192: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::getStats()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: nlohmann::json RedisCacheCoordinator::getStats() const {
- Line 450: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::close(fd);
- Line 460: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::close(fd);
- Line 626: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int elapsed = 0; elapsed < backoff_ms && !stop_.load(); elapsed += 50) {
- Line 638: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int elapsed = 0; elapsed < backoff_ms && !stop_.load(); elapsed += 50) {
- Line 652: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int elapsed = 0; elapsed < backoff_ms && !stop_.load(); elapsed += 50) {
- Line 660: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int i = 0; i < 2 && !stop_.load(); ++i) {
- Line 682: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int elapsed = 0; elapsed < backoff_ms && !stop_.load(); elapsed += 50) {
- Line 844: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::computeHmac(const std::string &payload)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: std::string RedisCacheCoordinator::computeHmac(const std::string &payload) const {
- Line 871: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::verifyHmac(const nlohmann::json &j)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: bool RedisCacheCoordinator::verifyHmac(const nlohmann::json &j) const {

### cache/bounded_lru_cache.cpp
Total findings: 25

- Line 58: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (isExpired(node->entry)) {
- Line 69: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->entry.last_access = std::chrono::steady_clock::now();
- Line 78: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: return node->entry.value;
- Line 90: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->entry.value       = std::move(value);
- Line 91: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->entry.expiry      = std::chrono::steady_clock::now() + ttl;
- Line 92: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->entry.last_access = std::chrono::steady_clock::now();
- Line 102: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 108: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 110: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->key   = key;
- Line 111: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->entry = std::move(entry);
- Line 172: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (node->prev) {
- Line 173: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->prev->next = node->next;
- Line 175: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (node->next) {
- Line 176: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->next->prev = node->prev;
- Line 179: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: tail_ = node->prev;
- Line 183: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->prev = nullptr;
- Line 184: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->next = head_;
- Line 196: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (node->prev) {
- Line 197: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->prev->next = node->next;
- Line 199: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: head_ = node->next;
- Line 202: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (node->next) {
- Line 203: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->next->prev = node->prev;
- Line 205: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: tail_ = node->prev;
- Line 210: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->prev = nullptr;
- Line 211: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->next = head_;

### cache/redis_cache_coordinator.cpp
Total findings: 22

- Line 88: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: sub_thread_.join();
- Line 126: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lk(pub_mutex_);
- Line 136: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (reply == nullptr || pub_ctx_->err) {
- Line 138: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: pub_ctx_ ? pub_ctx_->errstr : "null context");
- Line 202: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lk(pub_mutex_);
- Line 212: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (reply == nullptr || pub_ctx_->err) {
- Line 214: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: pub_ctx_ ? pub_ctx_->errstr : "null context");
- Line 387: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (r == nullptr || sub_ctx_->err || (r->type == REDIS_REPLY_ERROR)) {
- Line 400: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (r == nullptr || sub_ctx_->err) {
- Line 433: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> slk(stats_mutex_);
- Line 454: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(50));
- Line 532: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(50));
- Line 193: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::publishInvalidation(const std::string &pattern, const std::string &tenant_id)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: void RedisCacheCoordinator::publishInvalidation(const std::string &pattern, const std::string &tenant_id) {
- Line 266: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::subscribeEntries(EntryCallback callback)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: void RedisCacheCoordinator::subscribeEntries(EntryCallback callback) {
- Line 271: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::subscribeInvalidations(InvalidationCallback callback)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: void RedisCacheCoordinator::subscribeInvalidations(InvalidationCallback callback) {
- Line 280: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::isConnected()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: bool RedisCacheCoordinator::isConnected() const {
- Line 288: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::getStats()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: nlohmann::json RedisCacheCoordinator::getStats() const {
- Line 453: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int elapsed = 0; elapsed < backoff_ms && running_.load(); elapsed += 50) {
- Line 531: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int elapsed = 0; elapsed < backoff_ms && running_.load(); elapsed += 50) {
- Line 555: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Redis Pub/Sub Invalidation (v1' that was not found in 'src/cache/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/cache/FUTURE_ENHANCEMENTS.md § "Redis Pub/Sub Invalidation (v1.6.0)"
- Line 638: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::computeHmac(const std::string &payload)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: std::string RedisCacheCoordinator::computeHmac(const std::string &payload) const {
- Line 665: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::verifyHmac(const nlohmann::json &j)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: bool RedisCacheCoordinator::verifyHmac(const nlohmann::json &j) const {

### cache/predictive_prefetcher.cpp
Total findings: 14

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4293 Implement predictive prefet... (2026-03-19) | #3473 docs(cache): sync s
- Line 316: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Step 1: collect all existing keys under the prefix so we can delete
- Line 324: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: return true;

    });



    // Step 2: batch-delete all existing prefix keys

    if (!stale_keys.empty()) {

        auto batch = db->createWriteBatch();

        if (!batch) {
- Line 324: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Step 2: batch-delete all existing prefix keys
- Line 328: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: THEMIS_WARN("PredictivePrefetcher::saveModel: failed to create delete batch");
- Line 328: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (!stale_keys.empty()) {

        auto batch = db->createWriteBatch();

        if (!batch) {

            THEMIS_WARN("PredictivePrefetcher::saveModel: failed to create delete batch");

            return;

        }

        for (const auto &k : stale_keys) {
- Line 328: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_WARN("PredictivePrefetcher::saveModel: failed to create delete batch");
- Line 335: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: THEMIS_WARN("PredictivePrefetcher::saveModel: failed to commit delete batch");
- Line 335: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: batch->del(k);

        }

        if (!batch->commit()) {

            THEMIS_WARN("PredictivePrefetcher::saveModel: failed to commit delete batch");

            return;

        }

    }
- Line 335: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_WARN("PredictivePrefetcher::saveModel: failed to commit delete batch");
- Line 354: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto tod_from_it = tod_buckets_.find(from);
- Line 356: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto tod_to_it = tod_from_it->second.find(to);
- Line 433: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 434: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### cache/cache_replication_coordinator.cpp
Total findings: 10

- Line 175: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: fanout_thread_.join();
- Line 303: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: FanoutItem item;

        {

            std::unique_lock<std::mutex> lk(queue_mutex_);

            queue_cv_.wait(lk, [this] {

                return !fanout_queue_.empty() || stopping_;

            });

            if (stopping_ && fanout_queue_.empty()) {
- Line 303: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: queue_cv_.wait(lk, [this] {
- Line 163: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: fanout_thread_ = std::thread(&CacheReplicationCoordinator::fanoutWorker, this);
- Line 198: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = existing.find(addr);
- Line 281: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lk(queue_mutex_);
- Line 302: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lk(queue_mutex_);
- Line 327: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (const auto& peer : peers_to_contact) {
- Line 335: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> ml(metrics_mutex_);
- Line 190: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::shared_ptr<IRemoteCachePeer>> existing;

### cache/embedding_cache.cpp
Total findings: 7

- Line 60: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->cache_dir = config_.cache_dir;
- Line 62: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: db_config.db_path           = impl_->cache_dir;
- Line 73: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: = impl_->vector_index->init("embedding_cache", static_cast<int>(config_.embedding_dim), impl_->metri
- Line 96: severity=HIGH; category=missing_trace_point
  Description: Critical function query without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::optional<EmbeddingCache::CacheEntry> EmbeddingCache::query(const std::vector<float> &query_embedding) const {
- Line 268: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 318: severity=HIGH; category=command_injection
  Description: sql_injection_concat: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: THEMIS_DEBUG("Stored embedding in cache: pk={}, query='{}', metadata='{}'", pk, query_text.substr(0,
- Line 96: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::optional<EmbeddingCache::CacheEntry> EmbeddingCache::query(const std::vector<float> &query_embedding) const {

### cache/semantic_cache.cpp
Total findings: 5

- Line 80: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bg_expiry_thread_.join();
- Line 64: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lk(bg_cv_mutex_);
- Line 140: severity=HIGH; category=missing_trace_point
  Description: Critical function query without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::optional<SemanticCache::CacheEntry> SemanticCache::query(const std::string &prompt, const nlohmann::json &params) {
- Line 265: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: batch.Delete(it->key());

            }

            removed++;

        } catch (...) {

            // Invalid entry, remove it.

            if (cf_handle_) {

                batch.Delete(cf_handle_, it->key());
- Line 265: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### cache/grpc_remote_cache_peer.cpp
Total findings: 2

- Line 113: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: grpc::ByteBuffer      response_buf;

    grpc::Status          status;



    auto rpc = stub_->PrepareUnaryCall(&ctx, kInvalidateMethod, request_buf, &cq);

    rpc->StartCall();

    rpc->Finish(&response_buf, &status, reinterpret_cast<void*>(1));
- Line 26: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'gRPC Remote Cache Peer Activation' that was not found in 'src/cache/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/cache/FUTURE_ENHANCEMENTS.md §"gRPC Remote Cache Peer Activation"

### cache/warmup.cpp
Total findings: 2

- Line 46: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization
- Line 181: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: file.close();

### cache/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### cache/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### cache/cache_hit_rate_slo_monitor.cpp
Total findings: 1

- Line 413: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto res = alertmanager_->resolveAlert(active_latency_warning_alert_id_);

### cache/cache_replication.cpp
Total findings: 1

- Line 176: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: void CacheReplicationManager::notifyWrite(const std::string &key, const std::string &payload,

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
