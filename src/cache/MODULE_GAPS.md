# cache Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: cache
- Generated: 2026-06-02 12:40:50
- Status: Critical Findings Present
- Total Findings: 186
- Actionable Findings (Critical + High): 141
- Affected Files: 12

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 15 |
| High | 126 |
| Medium | 44 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| security | 57 |
| concurrency | 23 |
| reliability | 20 |
| performance_patterns | 18 |
| legacy_duplication | 16 |
| container | 13 |
| exception_safety | 8 |
| performance | 8 |
| raii | 7 |
| memory | 6 |
| distributed_consistency | 4 |
| observability | 4 |
| determinism | 2 |
| uninitialized | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/cache/adaptive_query_cache.cpp | 65 | 3 | 56 | 5 | 1 |
| src/cache/bounded_lru_cache.cpp | 25 | 0 | 25 | 0 | 0 |
| src/cache/distributed_cache_coordinator.cpp | 24 | 3 | 11 | 10 | 0 |
| src/cache/redis_cache_coordinator.cpp | 18 | 0 | 11 | 7 | 0 |
| src/cache/cache_replication_coordinator.cpp | 11 | 1 | 7 | 3 | 0 |
| src/cache/predictive_prefetcher.cpp | 11 | 0 | 7 | 4 | 0 |
| src/cache/embedding_cache.cpp | 9 | 6 | 2 | 1 | 0 |
| src/cache/cache_replication.cpp | 7 | 1 | 0 | 6 | 0 |
| src/cache/semantic_cache.cpp | 7 | 0 | 5 | 2 | 0 |
| src/cache/warmup.cpp | 6 | 0 | 1 | 5 | 0 |
| src/cache/grpc_remote_cache_peer.cpp | 2 | 0 | 1 | 1 | 0 |
| src/cache/cache_hit_rate_slo_monitor.cpp | 1 | 1 | 0 | 0 | 0 |

## Full Scanner Findings

### src/cache/adaptive_query_cache.cpp
Total findings: 65

- Line 117: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (!l3_db_->open()) {
- Line 973: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 982: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 127: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
- Line 213: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (ptr->expired_flag.load(std::memory_order_relaxed)) {
- Line 215: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: } else if (isExpired(ptr->created_at_ms.load(std::memory_order_relaxed),
- Line 216: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ptr->ttl_seconds.load(std::memory_order_relaxed))) {
- Line 219: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (ptr->expired_flag.compare_exchange_strong(expected, true, std::memory_order_relaxed)) {
- Line 226: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ptr->last_accessed_ms.store(now_ms, std::memory_order_relaxed);
- Line 227: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: int64_t new_count = ptr->access_count.fetch_add(1, std::memory_order_relaxed) + 1;
- Line 231: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: int64_t ws = ptr->window_start_ms.load(std::memory_order_relaxed);
- Line 234: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (ptr->window_start_ms.compare_exchange_strong(ws, now_ms, std::memory_order_relaxed)) {
- Line 235: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: uint32_t wc = ptr->window_count.exchange(1, std::memory_order_relaxed);
- Line 237: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: int old_ttl = ptr->ttl_seconds.load(std::memory_order_relaxed);
- Line 240: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (ptr->ttl_seconds.compare_exchange_strong(old_ttl, new_ttl,
- Line 245: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ptr->created_at_ms.store(now_ms, std::memory_order_relaxed);
- Line 248: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: uint32_t wc = ptr->window_count.fetch_add(1, std::memory_order_relaxed) + 1;
- Line 250: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: int old_ttl = ptr->ttl_seconds.load(std::memory_order_relaxed);
- Line 252: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (ptr->ttl_seconds.compare_exchange_strong(old_ttl, new_ttl, std::memory_order_relaxed)) {
- Line 257: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ptr->ttl_seconds.store(new_ttl, std::memory_order_relaxed);
- Line 259: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ptr->created_at_ms.store(now_ms, std::memory_order_relaxed);
- Line 274: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: prefetcher_->recordQueryAccess(fingerprint, tenant_id);
- Line 280: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: cache_result.result            = ptr->result;
- Line 282: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: cache_result.created_at_ms     = ptr->created_at_ms.load(std::memory_order_relaxed);
- Line 283: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: cache_result.last_accessed_ms  = ptr->last_accessed_ms.load(std::memory_order_relaxed);
- Line 284: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: cache_result.access_count      = ptr->access_count.load(std::memory_order_relaxed);
- Line 285: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: cache_result.ttl_seconds       = ptr->ttl_seconds.load(std::memory_order_relaxed);
- Line 285: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cache_result.ttl_seconds       = ptr->ttl_seconds.load(std::memory_order_relaxed);
- Line 295: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(l2_mutex_);
- Line 360: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: prefetcher_->recordQueryAccess(fingerprint, tenant_id);
- Line 374: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::unique_lock<std::shared_mutex> l1_lock(l1_mutex_);
- Line 423: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(l3_mutex_);
- Line 521: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: prefetcher_->recordQueryAccess(fingerprint, tenant_id);
- Line 554: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: prefetcher_->recordQueryAccess(fingerprint, tenant_id);
- Line 628: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::unique_lock<std::shared_mutex> l1_lock(l1_mutex_);
- Line 744: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::unique_lock<std::shared_mutex> lock(l1_mutex_);
- Line 1035: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::unique_lock<std::shared_mutex> lock(l1_mutex_);
- Line 1230: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = l1_cache_.begin(); it != l1_cache_.end();) {
  Confidence: band=very_high; score=0.9
- Line 1233: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> evict_lock(l1_eviction_mutex_);
- Line 1262: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: double min_score = calculateLRUScore(lru_it->second->last_accessed_ms.load(std::memory_order_relaxed
- Line 1596: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: health["warnings"] = nlohmann::json::array();
- Line 1802: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::unique_lock<std::shared_mutex> lock(l1_mutex_);
- Line 1803: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = l1_cache_.begin(); it != l1_cache_.end();) {
  Confidence: band=very_high; score=0.9
- Line 1806: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> evict_lock(l1_eviction_mutex_);
- Line 1949: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::unique_lock<std::shared_mutex> lock(l1_mutex_);
- Line 1950: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &k : keys_to_purge) {
  Confidence: band=very_high; score=0.9
- Line 1954: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> evict_lock(l1_eviction_mutex_);
- Line 2126: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: prefetcher_->recordQueryAccess(fingerprint, tenant_id);
- Line 2172: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: prefetcher_->loadModel(l3_db_.get());
- Line 2181: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(coordinator_mutex_);
- Line 2247: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::unique_lock<std::shared_mutex> lock(l1_mutex_);
- Line 2305: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::unique_lock<std::shared_mutex> lock(l1_mutex_);
- Line 2306: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = l1_cache_.begin(); it != l1_cache_.end();) {
  Confidence: band=very_high; score=0.9
- Line 2309: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> evict_lock(l1_eviction_mutex_);
- Line 2377: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::unique_lock<std::shared_mutex> lock(l1_mutex_);
- Line 2428: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: const bool expired = e->expired_flag.load(std::memory_order_acquire);
- Line 1696: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back("L1:" + key.substr(0, 16) + "...");
  Confidence: band=high; score=0.74
- Line 1707: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back("L2:" + key.substr(0, 16) + "...");
  Confidence: band=high; score=0.74
- Line 1708: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: keys.push_back("L2:" + key.substr(0, 16) + "...");
- Line 1935: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> keys_to_purge;
  Confidence: band=medium; score=0.66
- Line 2085: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: nlohmann::json status;
- Line 1207: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double log_factor = std::log(static_cast<double>(access_count + 1)) / config_.adaptive_ttl_scaling_factor;
  Confidence: band=medium; score=0.6

### src/cache/bounded_lru_cache.cpp
Total findings: 25

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 58: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (isExpired(node->entry)) {
- Line 69: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->entry.last_access = std::chrono::steady_clock::now();
- Line 78: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return node->entry.value;
- Line 90: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->entry.value       = std::move(value);
- Line 91: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->entry.expiry      = std::chrono::steady_clock::now() + ttl;
- Line 92: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->entry.last_access = std::chrono::steady_clock::now();
- Line 110: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->key   = key;
- Line 111: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->entry = std::move(entry);
- Line 172: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (node->prev) {
- Line 173: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->prev->next = node->next;
- Line 175: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (node->next) {
- Line 176: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->next->prev = node->prev;
- Line 179: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: tail_ = node->prev;
- Line 183: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->prev = nullptr;
- Line 184: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->next = head_;
- Line 196: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (node->prev) {
- Line 197: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->prev->next = node->next;
- Line 199: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: head_ = node->next;
- Line 202: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (node->next) {
- Line 203: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->next->prev = node->prev;
- Line 205: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: tail_ = node->prev;
- Line 210: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->prev = nullptr;
- Line 211: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->next = head_;

### src/cache/distributed_cache_coordinator.cpp
Total findings: 24

- Line 424: severity=CRITICAL; category=missing_dtor
  Description: Class addrinfo allocates resources but has no destructor
  Remediation: Add explicit destructor: ~addrinfo() { /* cleanup */ }
  Context: class/struct addrinfo
- Line 428: severity=CRITICAL; category=missing_dtor
  Description: Class addrinfo allocates resources but has no destructor
  Remediation: Add explicit destructor: ~addrinfo() { /* cleanup */ }
  Context: class/struct addrinfo
- Line 435: severity=CRITICAL; category=missing_dtor
  Description: Class addrinfo allocates resources but has no destructor
  Remediation: Add explicit destructor: ~addrinfo() { /* cleanup */ }
  Context: class/struct addrinfo
- Line 84: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: //   Windows SDK builds without POSIX compatibility shims).  The coordinator
  Confidence: band=high; score=0.8
- Line 283: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: sub_thread_ = std::thread(&RedisCacheCoordinator::subscriberLoop, this);
  Confidence: band=very_high; score=0.9
- Line 436: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: fd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
- Line 447: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (::connect(fd, p->ai_addr, p->ai_addrlen) == 0) {
- Line 469: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ssize_t n = ::send(fd, buf.data() + sent, buf.size() - sent, MSG_NOSIGNAL);
- Line 482: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ssize_t n = ::recv(fd, &ch, 1, 0);
- Line 627: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(50));
- Line 639: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(50));
- Line 653: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(50));
- Line 683: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(50));
- Line 752: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (::recv(fd, crlf, 2, MSG_WAITALL) != 2)
- Line 93: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: //   one node is never propagated to other nodes via Redis pub/sub; stale reads
  Confidence: band=high; score=0.74
- Line 145: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::publishInvalidation(const std::string &pattern, const std::string &tenant_id)
  Context: void RedisCacheCoordinator::publishInvalidation(const std::string &pattern, const std::string &tenant_id) {
  Confidence: band=medium; score=0.56
- Line 178: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::subscribeEntries(EntryCallback callback)
  Context: void RedisCacheCoordinator::subscribeEntries(EntryCallback callback) {
  Confidence: band=medium; score=0.56
- Line 183: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::subscribeInvalidations(InvalidationCallback callback)
  Context: void RedisCacheCoordinator::subscribeInvalidations(InvalidationCallback callback) {
  Confidence: band=medium; score=0.56
- Line 188: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::isConnected()
  Context: bool RedisCacheCoordinator::isConnected() const {
  Confidence: band=medium; score=0.56
- Line 192: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::getStats()
  Context: nlohmann::json RedisCacheCoordinator::getStats() const {
  Confidence: band=medium; score=0.56
- Line 450: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
- Line 460: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
- Line 844: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::computeHmac(const std::string &payload)
  Context: std::string RedisCacheCoordinator::computeHmac(const std::string &payload) const {
  Confidence: band=medium; score=0.56
- Line 871: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::verifyHmac(const nlohmann::json &j)
  Context: bool RedisCacheCoordinator::verifyHmac(const nlohmann::json &j) const {
  Confidence: band=medium; score=0.56

### src/cache/redis_cache_coordinator.cpp
Total findings: 18

- Line 126: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(pub_mutex_);
- Line 136: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (reply == nullptr || pub_ctx_->err) {
- Line 138: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: pub_ctx_ ? pub_ctx_->errstr : "null context");
- Line 202: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(pub_mutex_);
- Line 212: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (reply == nullptr || pub_ctx_->err) {
- Line 214: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: pub_ctx_ ? pub_ctx_->errstr : "null context");
- Line 387: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (r == nullptr || sub_ctx_->err || (r->type == REDIS_REPLY_ERROR)) {
- Line 400: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (r == nullptr || sub_ctx_->err) {
- Line 433: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> slk(stats_mutex_);
- Line 454: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(50));
- Line 532: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(50));
- Line 193: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::publishInvalidation(const std::string &pattern, const std::string &tenant_id)
  Context: void RedisCacheCoordinator::publishInvalidation(const std::string &pattern, const std::string &tenant_id) {
  Confidence: band=medium; score=0.56
- Line 266: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::subscribeEntries(EntryCallback callback)
  Context: void RedisCacheCoordinator::subscribeEntries(EntryCallback callback) {
  Confidence: band=medium; score=0.56
- Line 271: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::subscribeInvalidations(InvalidationCallback callback)
  Context: void RedisCacheCoordinator::subscribeInvalidations(InvalidationCallback callback) {
  Confidence: band=medium; score=0.56
- Line 280: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::isConnected()
  Context: bool RedisCacheCoordinator::isConnected() const {
  Confidence: band=medium; score=0.56
- Line 288: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::getStats()
  Context: nlohmann::json RedisCacheCoordinator::getStats() const {
  Confidence: band=medium; score=0.56
- Line 638: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::computeHmac(const std::string &payload)
  Context: std::string RedisCacheCoordinator::computeHmac(const std::string &payload) const {
  Confidence: band=medium; score=0.56
- Line 665: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::verifyHmac(const nlohmann::json &j)
  Context: bool RedisCacheCoordinator::verifyHmac(const nlohmann::json &j) const {
  Confidence: band=medium; score=0.56

### src/cache/cache_replication_coordinator.cpp
Total findings: 11

- Line 303: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: queue_cv_.wait(lk, [this] {
- Line 163: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: fanout_thread_ = std::thread(&CacheReplicationCoordinator::fanoutWorker, this);
  Confidence: band=very_high; score=0.9
- Line 198: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = existing.find(addr);
- Line 198: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = existing.find(addr);
- Line 281: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(queue_mutex_);
- Line 302: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lk(queue_mutex_);
- Line 327: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& peer : peers_to_contact) {
  Confidence: band=very_high; score=0.9
- Line 335: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> ml(metrics_mutex_);
- Line 190: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::shared_ptr<IRemoteCachePeer>> existing;
  Confidence: band=medium; score=0.66
- Line 201: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: new_peers.emplace_back(it->second);
  Confidence: band=high; score=0.74
- Line 340: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failed_peers.push_back(peer);
  Confidence: band=high; score=0.74

### src/cache/predictive_prefetcher.cpp
Total findings: 11

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4293 Implement predictive prefet... (2026-03-19) | #3473 docs(cache): sync s
- Line 328: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: batch = nullptr;
  Context: THEMIS_WARN("PredictivePrefetcher::saveModel: failed to create delete batch");
- Line 335: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: batch = nullptr;
  Context: THEMIS_WARN("PredictivePrefetcher::saveModel: failed to commit delete batch");
- Line 354: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto tod_from_it = tod_buckets_.find(from);
  Confidence: band=very_high; score=0.9
- Line 356: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto tod_to_it = tod_from_it->second.find(to);
  Confidence: band=very_high; score=0.9
- Line 199: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.emplace_back(score, to);
  Confidence: band=high; score=0.74
- Line 213: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(candidates[i].second));
  Confidence: band=high; score=0.74
- Line 458: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: range_end.push_back(static_cast<char>(0xFF));
  Confidence: band=high; score=0.74
- Line 459: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: range_end.push_back(static_cast<char>(0xFF));

### src/cache/embedding_cache.cpp
Total findings: 9

- Line 60: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->cache_dir = config_.cache_dir;
- Line 60: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->cache_dir = config_.cache_dir;
- Line 62: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: db_config.db_path           = impl_->cache_dir;
- Line 73: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: = impl_->vector_index->init("embedding_cache", static_cast<int>(config_.embedding_dim), impl_->metri
- Line 73: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: = impl_->vector_index->init("embedding_cache", static_cast<int>(config_.embedding_dim), impl_->metri
- Line 73: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: = impl_->vector_index->init("embedding_cache", static_cast<int>(config_.embedding_dim), impl_->metri
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 96: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function query without trace point
  Context: std::optional<EmbeddingCache::CacheEntry> EmbeddingCache::query(const std::vector<float> &query_embedding) const {
  Confidence: band=very_high; score=0.9
- Line 96: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::optional<EmbeddingCache::CacheEntry> EmbeddingCache::query(const std::vector<float> &query_embedding) const {
  Confidence: band=high; score=0.74

### src/cache/cache_replication.cpp
Total findings: 7

- Line 176: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: void CacheReplicationManager::notifyWrite(const std::string &key, const std::string &payload,
  Confidence: band=very_high; score=0.99
- Line 146: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ++unhealthy_count;
- Line 149: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ++unhealthy_count;
- Line 231: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 249: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ++unhealthy_count;
- Line 250: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: continue; // Skip until probed healthy again
- Line 289: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ++unhealthy_count;

### src/cache/semantic_cache.cpp
Total findings: 7

- Line 64: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lk(bg_cv_mutex_);
- Line 140: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::optional<SemanticCache::CacheEntry> SemanticCache::query(const std::string &prompt, const nlohm
- Line 140: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function query without trace point
  Context: std::optional<SemanticCache::CacheEntry> SemanticCache::query(const std::string &prompt, const nlohmann::json &params) {
  Confidence: band=very_high; score=0.9
- Line 214: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::unique_ptr<rocksdb::Iterator> it(cf_handle_ ? db_->NewIterator(read_opts, cf_handle_)
- Line 296: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::unique_ptr<rocksdb::Iterator> it(cf_handle_ ? db_->NewIterator(read_opts, cf_handle_)
- Line 124: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status s;
- Line 147: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status s;

### src/cache/warmup.cpp
Total findings: 6

- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 72: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<char>((buf >> bits) & 0xFF));
  Confidence: band=high; score=0.74
- Line 73: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<char>((buf >> bits) & 0xFF));
- Line 93: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(kB64Chars[(buf >> bits) & 0x3F]);
  Confidence: band=high; score=0.74
- Line 181: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: file.close();
- Line 435: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(std::launch::async, processChunk, start, end));
  Confidence: band=high; score=0.74

### src/cache/grpc_remote_cache_peer.cpp
Total findings: 2

- Line 113: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto rpc = stub_->PrepareUnaryCall(&ctx, kInvalidateMethod, request_buf, &cq);
- Line 111: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: grpc::Status          status;

### src/cache/cache_hit_rate_slo_monitor.cpp
Total findings: 1

- Line 413: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto res = alertmanager_->resolveAlert(active_latency_warning_alert_id_);

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
