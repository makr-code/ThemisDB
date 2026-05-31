# cache Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: cache
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 261
- Actionable Findings (Critical + High): 204
- Affected Files: 12

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 46 |
| High | 158 |
| Medium | 57 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| security | 65 |
| container | 41 |
| performance_patterns | 35 |
| concurrency | 25 |
| reliability | 23 |
| memory | 20 |
| legacy_duplication | 16 |
| raii | 9 |
| exception_safety | 8 |
| performance | 8 |
| distributed_consistency | 4 |
| observability | 4 |
| determinism | 2 |
| uninitialized | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/cache/adaptive_query_cache.cpp | 118 | 25 | 86 | 6 | 1 |
| src/cache/distributed_cache_coordinator.cpp | 26 | 3 | 13 | 10 | 0 |
| src/cache/predictive_prefetcher.cpp | 19 | 3 | 9 | 7 | 0 |
| src/cache/redis_cache_coordinator.cpp | 17 | 0 | 10 | 7 | 0 |
| src/cache/semantic_cache.cpp | 17 | 0 | 15 | 2 | 0 |
| src/cache/cache_replication_coordinator.cpp | 15 | 1 | 9 | 5 | 0 |
| src/cache/embedding_cache.cpp | 13 | 10 | 2 | 1 | 0 |
| src/cache/warmup.cpp | 11 | 0 | 1 | 10 | 0 |
| src/cache/cache_replication.cpp | 9 | 1 | 1 | 7 | 0 |
| src/cache/bounded_lru_cache.cpp | 8 | 1 | 7 | 0 | 0 |
| src/cache/cache_hit_rate_slo_monitor.cpp | 4 | 2 | 2 | 0 | 0 |
| src/cache/grpc_remote_cache_peer.cpp | 4 | 0 | 3 | 1 | 0 |

## Full Scanner Findings

### src/cache/adaptive_query_cache.cpp
Total findings: 118

- Line 116: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (!l3_db_->open()) {
- Line 279: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: cache_result.result            = ptr->result;
- Line 281: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: cache_result.created_at_ms     = ptr->created_at_ms.load(std::memory_order_relaxed);
- Line 282: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: cache_result.last_accessed_ms  = ptr->last_accessed_ms.load(std::memory_order_relaxed);
- Line 283: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: cache_result.access_count      = ptr->access_count.load(std::memory_order_relaxed);
- Line 284: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: cache_result.ttl_seconds       = ptr->ttl_seconds.load(std::memory_order_relaxed);
- Line 295: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = l2_cache_.find(key);
- Line 912: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = l1_cache_.begin(); it != l1_cache_.end();) {
- Line 926: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = l2_cache_.begin(); it != l2_cache_.end();) {
- Line 972: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 981: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 1091: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = l1_cache_.begin(); it != l1_cache_.end();) {
- Line 1106: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = l2_cache_.begin(); it != l2_cache_.end();) {
- Line 1229: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = l1_cache_.begin(); it != l1_cache_.end();) {
- Line 1292: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator lru_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto lru_it      = l2_cache_.begin();
- Line 1294: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = l2_cache_.begin(); it != l2_cache_.end(); ++it) {
- Line 1802: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = l1_cache_.begin(); it != l1_cache_.end();) {
- Line 1819: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = l2_cache_.begin(); it != l2_cache_.end();) {
- Line 1937: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = pii_key_index_.find(pii_uuid);
- Line 1950: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = l1_cache_.find(k);
- Line 1975: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = l2_cache_.find(l2_key);
- Line 2305: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = l1_cache_.begin(); it != l1_cache_.end();) {
- Line 2320: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = l2_cache_.begin(); it != l2_cache_.end();) {
- Line 2377: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = l1_cache_.find(fingerprint);
- Line 2391: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = l2_cache_.find(fingerprint);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 46: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Invalid cache configuration: " + validation_error);
- Line 126: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
- Line 209: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: L1Entry *ptr = it->second.get();
- Line 212: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (ptr->expired_flag.load(std::memory_order_relaxed)) {
- Line 214: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: } else if (isExpired(ptr->created_at_ms.load(std::memory_order_relaxed),
- Line 215: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ptr->ttl_seconds.load(std::memory_order_relaxed))) {
- Line 218: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (ptr->expired_flag.compare_exchange_strong(expected, true, std::memory_order_relaxed)) {
- Line 225: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ptr->last_accessed_ms.store(now_ms, std::memory_order_relaxed);
- Line 225: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ptr->last_accessed_ms.store(now_ms, std::memory_order_relaxed);
- Line 226: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: int64_t new_count = ptr->access_count.fetch_add(1, std::memory_order_relaxed) + 1;
- Line 226: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: int64_t new_count = ptr->access_count.fetch_add(1, std::memory_order_relaxed) + 1;
- Line 230: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: int64_t ws = ptr->window_start_ms.load(std::memory_order_relaxed);
- Line 233: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (ptr->window_start_ms.compare_exchange_strong(ws, now_ms, std::memory_order_relaxed)) {
- Line 234: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: uint32_t wc = ptr->window_count.exchange(1, std::memory_order_relaxed);
- Line 236: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: int old_ttl = ptr->ttl_seconds.load(std::memory_order_relaxed);
- Line 239: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (ptr->ttl_seconds.compare_exchange_strong(old_ttl, new_ttl,
- Line 244: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ptr->created_at_ms.store(now_ms, std::memory_order_relaxed);
- Line 244: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ptr->created_at_ms.store(now_ms, std::memory_order_relaxed);
- Line 247: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: uint32_t wc = ptr->window_count.fetch_add(1, std::memory_order_relaxed) + 1;
- Line 247: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: uint32_t wc = ptr->window_count.fetch_add(1, std::memory_order_relaxed) + 1;
- Line 249: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: int old_ttl = ptr->ttl_seconds.load(std::memory_order_relaxed);
- Line 251: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (ptr->ttl_seconds.compare_exchange_strong(old_ttl, new_ttl, std::memory_order_relaxed)) {
- Line 256: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ptr->ttl_seconds.store(new_ttl, std::memory_order_relaxed);
- Line 256: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ptr->ttl_seconds.store(new_ttl, std::memory_order_relaxed);
- Line 258: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ptr->created_at_ms.store(now_ms, std::memory_order_relaxed);
- Line 258: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ptr->created_at_ms.store(now_ms, std::memory_order_relaxed);
- Line 273: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: prefetcher_->recordQueryAccess(fingerprint, tenant_id);
- Line 279: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: cache_result.result            = ptr->result;
- Line 279: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cache_result.result            = ptr->result;
- Line 281: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: cache_result.created_at_ms     = ptr->created_at_ms.load(std::memory_order_relaxed);
- Line 281: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cache_result.created_at_ms     = ptr->created_at_ms.load(std::memory_order_relaxed);
- Line 282: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: cache_result.last_accessed_ms  = ptr->last_accessed_ms.load(std::memory_order_relaxed);
- Line 282: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cache_result.last_accessed_ms  = ptr->last_accessed_ms.load(std::memory_order_relaxed);
- Line 283: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: cache_result.access_count      = ptr->access_count.load(std::memory_order_relaxed);
- Line 283: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cache_result.access_count      = ptr->access_count.load(std::memory_order_relaxed);
- Line 284: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: cache_result.ttl_seconds       = ptr->ttl_seconds.load(std::memory_order_relaxed);
- Line 284: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cache_result.ttl_seconds       = ptr->ttl_seconds.load(std::memory_order_relaxed);
- Line 359: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: prefetcher_->recordQueryAccess(fingerprint, tenant_id);
- Line 520: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: prefetcher_->recordQueryAccess(fingerprint, tenant_id);
- Line 553: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: prefetcher_->recordQueryAccess(fingerprint, tenant_id);
- Line 717: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &pii_uuid : pii_uuids) {
  Confidence: band=very_high; score=0.9
- Line 757: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> evict_lock(l1_eviction_mutex_);
- Line 862: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: static_cast<void>(l3_db_->put("pii_ref:" + pii_uuid + ":" + fingerprint, ""));
- Line 912: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = l1_cache_.begin(); it != l1_cache_.end();) {
  Confidence: band=very_high; score=0.9
- Line 926: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = l2_cache_.begin(); it != l2_cache_.end();) {
  Confidence: band=very_high; score=0.9
- Line 949: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: l3_db_->scanPrefix(QUERY_CACHE_PREFIX, [&](std::string_view key, std::string_view) {
- Line 1034: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::unique_lock<std::shared_mutex> lock(l1_mutex_);
- Line 1061: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: static_cast<void>(l3_db_->scanPrefix(QUERY_CACHE_PREFIX, [&keys](std::string_view key, std::string_v
- Line 1066: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: static_cast<void>(l3_db_->scanPrefix("pii_ref:", [&pii_ref_keys](std::string_view key, std::string_v
- Line 1072: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &del_key : keys) {
  Confidence: band=very_high; score=0.9
- Line 1075: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &del_key : pii_ref_keys) {
  Confidence: band=very_high; score=0.9
- Line 1091: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = l1_cache_.begin(); it != l1_cache_.end();) {
  Confidence: band=very_high; score=0.9
- Line 1229: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = l1_cache_.begin(); it != l1_cache_.end();) {
  Confidence: band=very_high; score=0.9
- Line 1232: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> evict_lock(l1_eviction_mutex_);
- Line 1261: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: double min_score = calculateLRUScore(lru_it->second->last_accessed_ms.load(std::memory_order_relaxed
- Line 1263: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = l1_cache_.begin(); it != l1_cache_.end(); ++it) {
  Confidence: band=very_high; score=0.9
- Line 1376: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Validate adaptive TTL (legacy + current fields)
  Confidence: band=high; score=0.8
- Line 1574: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: stats["l3"]["enabled"] = (l3_db_ != nullptr);
- Line 1595: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: health["warnings"] = nlohmann::json::array();
- Line 1692: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &[key, entry] : l1_cache_) {
  Confidence: band=very_high; score=0.9
- Line 1802: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = l1_cache_.begin(); it != l1_cache_.end();) {
  Confidence: band=very_high; score=0.9
- Line 1803: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (it->first.find(tenant_prefix) == 0) {
- Line 1805: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> evict_lock(l1_eviction_mutex_);
- Line 1819: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = l2_cache_.begin(); it != l2_cache_.end();) {
  Confidence: band=very_high; score=0.9
- Line 1820: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (it->first.find(tenant_prefix) == 0) {
- Line 1842: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: l3_db_->scanPrefix(QUERY_CACHE_PREFIX, [&](std::string_view key, std::string_view) {
- Line 1860: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &key : keys_to_delete) {
  Confidence: band=very_high; score=0.9
- Line 1949: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &k : keys_to_purge) {
  Confidence: band=very_high; score=0.9
- Line 1953: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> evict_lock(l1_eviction_mutex_);
- Line 2001: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: l3_db_->scanPrefix(pii_ref_prefix, [&](std::string_view key, std::string_view) {
- Line 2026: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &ck : cache_keys) {
  Confidence: band=very_high; score=0.9
- Line 2031: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &rk : pii_ref_keys) {
  Confidence: band=very_high; score=0.9
- Line 2125: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: prefetcher_->recordQueryAccess(fingerprint, tenant_id);
- Line 2135: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto candidates = prefetcher_->getPrefetchCandidates(fingerprint, tenant_id);
- Line 2141: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: prefetcher_->recordCandidatesGenerated(candidates.size(), tenant_id);
- Line 2150: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: nlohmann::json j = prefetcher_->getStats();
- Line 2159: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: prefetcher_->recordOverheadBytes(bytes);
- Line 2165: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: prefetcher_->saveModel(l3_db_.get());
- Line 2171: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: prefetcher_->loadModel(l3_db_.get());
- Line 2305: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = l1_cache_.begin(); it != l1_cache_.end();) {
  Confidence: band=very_high; score=0.9
- Line 2308: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> evict_lock(l1_eviction_mutex_);
- Line 2320: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = l2_cache_.begin(); it != l2_cache_.end();) {
  Confidence: band=very_high; score=0.9
- Line 2427: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: const bool expired = e->expired_flag.load(std::memory_order_acquire);
- Line 1695: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back("L1:" + key.substr(0, 16) + "...");
  Confidence: band=high; score=0.74
- Line 1696: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: keys.push_back("L1:" + key.substr(0, 16) + "...");
- Line 1706: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back("L2:" + key.substr(0, 16) + "...");
  Confidence: band=high; score=0.74
- Line 1707: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: keys.push_back("L2:" + key.substr(0, 16) + "...");
- Line 1934: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> keys_to_purge;
  Confidence: band=medium; score=0.66
- Line 2084: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: nlohmann::json status;
- Line 1206: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double log_factor = std::log(static_cast<double>(access_count + 1)) / config_.adaptive_ttl_scaling_factor;
  Confidence: band=medium; score=0.6

### src/cache/distributed_cache_coordinator.cpp
Total findings: 26

- Line 423: severity=CRITICAL; category=missing_dtor
  Description: Class addrinfo allocates resources but has no destructor
  Remediation: Add explicit destructor: ~addrinfo() { /* cleanup */ }
  Context: class/struct addrinfo
- Line 427: severity=CRITICAL; category=missing_dtor
  Description: Class addrinfo allocates resources but has no destructor
  Remediation: Add explicit destructor: ~addrinfo() { /* cleanup */ }
  Context: class/struct addrinfo
- Line 434: severity=CRITICAL; category=missing_dtor
  Description: Class addrinfo allocates resources but has no destructor
  Remediation: Add explicit destructor: ~addrinfo() { /* cleanup */ }
  Context: class/struct addrinfo
- Line 83: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: //   Windows SDK builds without POSIX compatibility shims).  The coordinator
  Confidence: band=high; score=0.8
- Line 282: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: sub_thread_ = std::thread(&RedisCacheCoordinator::subscriberLoop, this);
  Confidence: band=very_high; score=0.9
- Line 434: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (struct addrinfo *p = res; p != nullptr; p = p->ai_next) {
- Line 434: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (struct addrinfo *p = res; p != nullptr; p = p->ai_next) {
- Line 435: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: fd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
- Line 446: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (::connect(fd, p->ai_addr, p->ai_addrlen) == 0) {
- Line 468: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ssize_t n = ::send(fd, buf.data() + sent, buf.size() - sent, MSG_NOSIGNAL);
- Line 481: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ssize_t n = ::recv(fd, &ch, 1, 0);
- Line 626: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(50));
- Line 638: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(50));
- Line 652: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(50));
- Line 682: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(50));
- Line 751: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (::recv(fd, crlf, 2, MSG_WAITALL) != 2)
- Line 92: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: //   one node is never propagated to other nodes via Redis pub/sub; stale reads
  Confidence: band=high; score=0.74
- Line 144: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::publishInvalidation(const std::string &pattern, const std::string &tenant_id)
  Context: void RedisCacheCoordinator::publishInvalidation(const std::string &pattern, const std::string &tenant_id) {
  Confidence: band=medium; score=0.56
- Line 177: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::subscribeEntries(EntryCallback callback)
  Context: void RedisCacheCoordinator::subscribeEntries(EntryCallback callback) {
  Confidence: band=medium; score=0.56
- Line 182: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::subscribeInvalidations(InvalidationCallback callback)
  Context: void RedisCacheCoordinator::subscribeInvalidations(InvalidationCallback callback) {
  Confidence: band=medium; score=0.56
- Line 187: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::isConnected()
  Context: bool RedisCacheCoordinator::isConnected() const {
  Confidence: band=medium; score=0.56
- Line 191: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::getStats()
  Context: nlohmann::json RedisCacheCoordinator::getStats() const {
  Confidence: band=medium; score=0.56
- Line 449: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
- Line 459: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
- Line 843: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::computeHmac(const std::string &payload)
  Context: std::string RedisCacheCoordinator::computeHmac(const std::string &payload) const {
  Confidence: band=medium; score=0.56
- Line 870: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::verifyHmac(const nlohmann::json &j)
  Context: bool RedisCacheCoordinator::verifyHmac(const nlohmann::json &j) const {
  Confidence: band=medium; score=0.56

### src/cache/predictive_prefetcher.cpp
Total findings: 19

- Line 105: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = last_fingerprint_.find(session_key);
- Line 187: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: uint32_t hour_count = tod_to_it->second[static_cast<size_t>(current_hour)];
- Line 431: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: uint32_t old_val = (existing_it != successors.end()) ? existing_it->second : 0;
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 212: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < limit; ++i) {
  Confidence: band=very_high; score=0.9
- Line 318: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db->scanPrefix(PREFETCH_MODEL_PREFIX, [&](std::string_view key, std::string_view /*value*/) {
- Line 327: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: batch = nullptr;
  Context: THEMIS_WARN("PredictivePrefetcher::saveModel: failed to create delete batch");
- Line 334: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: batch = nullptr;
  Context: THEMIS_WARN("PredictivePrefetcher::saveModel: failed to commit delete batch");
- Line 353: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto tod_from_it = tod_buckets_.find(from);
  Confidence: band=very_high; score=0.9
- Line 355: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto tod_to_it = tod_from_it->second.find(to);
  Confidence: band=very_high; score=0.9
- Line 448: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db->scanPrefix(PREFETCH_MODEL_PREFIX, [&](std::string_view raw_key, std::string_view raw_value) {
- Line 198: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.emplace_back(score, to);
  Confidence: band=high; score=0.74
- Line 212: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(candidates[i].second));
  Confidence: band=high; score=0.74
- Line 213: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(candidates[i].second));
- Line 327: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: THEMIS_WARN("PredictivePrefetcher::saveModel: failed to create delete batch");
- Line 334: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: THEMIS_WARN("PredictivePrefetcher::saveModel: failed to commit delete batch");
- Line 457: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: range_end.push_back(static_cast<char>(0xFF));
  Confidence: band=high; score=0.74
- Line 458: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: range_end.push_back(static_cast<char>(0xFF));

### src/cache/redis_cache_coordinator.cpp
Total findings: 17

- Line 135: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (reply == nullptr || pub_ctx_->err) {
- Line 211: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (reply == nullptr || pub_ctx_->err) {
- Line 314: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (pub_ctx_ != nullptr && !pub_ctx_->err) {
- Line 346: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (r == nullptr || pub_ctx_->err || (r->type == REDIS_REPLY_ERROR)) {
- Line 374: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (sub_ctx_ == nullptr || sub_ctx_->err) {
- Line 386: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (r == nullptr || sub_ctx_->err || (r->type == REDIS_REPLY_ERROR)) {
- Line 399: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (r == nullptr || sub_ctx_->err) {
- Line 432: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> slk(stats_mutex_);
- Line 453: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(50));
- Line 531: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(50));
- Line 192: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::publishInvalidation(const std::string &pattern, const std::string &tenant_id)
  Context: void RedisCacheCoordinator::publishInvalidation(const std::string &pattern, const std::string &tenant_id) {
  Confidence: band=medium; score=0.56
- Line 265: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::subscribeEntries(EntryCallback callback)
  Context: void RedisCacheCoordinator::subscribeEntries(EntryCallback callback) {
  Confidence: band=medium; score=0.56
- Line 270: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::subscribeInvalidations(InvalidationCallback callback)
  Context: void RedisCacheCoordinator::subscribeInvalidations(InvalidationCallback callback) {
  Confidence: band=medium; score=0.56
- Line 279: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::isConnected()
  Context: bool RedisCacheCoordinator::isConnected() const {
  Confidence: band=medium; score=0.56
- Line 287: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::getStats()
  Context: nlohmann::json RedisCacheCoordinator::getStats() const {
  Confidence: band=medium; score=0.56
- Line 637: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::computeHmac(const std::string &payload)
  Context: std::string RedisCacheCoordinator::computeHmac(const std::string &payload) const {
  Confidence: band=medium; score=0.56
- Line 664: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RedisCacheCoordinator::verifyHmac(const nlohmann::json &j)
  Context: bool RedisCacheCoordinator::verifyHmac(const nlohmann::json &j) const {
  Confidence: band=medium; score=0.56

### src/cache/semantic_cache.cpp
Total findings: 17

- Line 63: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lk(bg_cv_mutex_);
- Line 125: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: s = db_->Put(write_opts, cf_handle_, key, value);
- Line 139: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::optional<SemanticCache::CacheEntry> SemanticCache::query(const std::string &prompt, const nlohm
- Line 139: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function query without trace point
  Context: std::optional<SemanticCache::CacheEntry> SemanticCache::query(const std::string &prompt, const nlohmann::json &params) {
  Confidence: band=very_high; score=0.9
- Line 148: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: s = db_->Get(read_opts, cf_handle_, key, &value);
- Line 213: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::unique_ptr<rocksdb::Iterator> it(cf_handle_ ? db_->NewIterator(read_opts, cf_handle_)
- Line 213: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::unique_ptr<rocksdb::Iterator> it(cf_handle_ ? db_->NewIterator(read_opts, cf_handle_)
- Line 234: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: batch.Delete(cf_handle_, it->key());
- Line 243: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: batch.Delete(cf_handle_, it->key());
- Line 251: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: batch.Delete(cf_handle_, it->key());
- Line 259: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: batch.Delete(cf_handle_, it->key());
- Line 267: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: batch.Delete(cf_handle_, it->key());
- Line 295: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::unique_ptr<rocksdb::Iterator> it(cf_handle_ ? db_->NewIterator(read_opts, cf_handle_)
- Line 295: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::unique_ptr<rocksdb::Iterator> it(cf_handle_ ? db_->NewIterator(read_opts, cf_handle_)
- Line 308: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: batch.Delete(cf_handle_, it->key());
- Line 123: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status s;
- Line 146: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: rocksdb::Status s;

### src/cache/cache_replication_coordinator.cpp
Total findings: 15

- Line 302: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: queue_cv_.wait(lk, [this] {
- Line 60: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto* peer : bus_->peers) {
  Confidence: band=very_high; score=0.9
- Line 84: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto* peer : bus_->peers) {
  Confidence: band=very_high; score=0.9
- Line 162: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: fanout_thread_ = std::thread(&CacheReplicationCoordinator::fanoutWorker, this);
  Confidence: band=very_high; score=0.9
- Line 197: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = existing.find(addr);
- Line 197: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = existing.find(addr);
- Line 280: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(queue_mutex_);
- Line 301: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lk(queue_mutex_);
- Line 326: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& peer : peers_to_contact) {
  Confidence: band=very_high; score=0.9
- Line 334: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> ml(metrics_mutex_);
- Line 189: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::shared_ptr<IRemoteCachePeer>> existing;
  Confidence: band=medium; score=0.66
- Line 200: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: new_peers.emplace_back(it->second);
  Confidence: band=high; score=0.74
- Line 339: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failed_peers.push_back(peer);
  Confidence: band=high; score=0.74
- Line 340: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: failed_peers.push_back(peer);
- Line 344: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: failed_peers.push_back(peer);

### src/cache/embedding_cache.cpp
Total findings: 13

- Line 59: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->cache_dir = config_.cache_dir;
- Line 59: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->cache_dir = config_.cache_dir;
- Line 61: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: db_config.db_path           = impl_->cache_dir;
- Line 72: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: = impl_->vector_index->init("embedding_cache", static_cast<int>(config_.embedding_dim), impl_->metri
- Line 72: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: = impl_->vector_index->init("embedding_cache", static_cast<int>(config_.embedding_dim), impl_->metri
- Line 72: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: = impl_->vector_index->init("embedding_cache", static_cast<int>(config_.embedding_dim), impl_->metri
- Line 236: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: while (impl_->entries.size() >= config_.max_entries) {
- Line 357: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto status = impl_->vector_index->init("embedding_cache", static_cast<int>(config_.embedding_dim),
- Line 357: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto status = impl_->vector_index->init("embedding_cache", static_cast<int>(config_.embedding_dim),
- Line 357: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto status = impl_->vector_index->init("embedding_cache", static_cast<int>(config_.embedding_dim),
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 95: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function query without trace point
  Context: std::optional<EmbeddingCache::CacheEntry> EmbeddingCache::query(const std::vector<float> &query_embedding) const {
  Confidence: band=very_high; score=0.9
- Line 95: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::optional<EmbeddingCache::CacheEntry> EmbeddingCache::query(const std::vector<float> &query_embedding) const {
  Confidence: band=high; score=0.74

### src/cache/warmup.cpp
Total findings: 11

- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 71: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<char>((buf >> bits) & 0xFF));
  Confidence: band=high; score=0.74
- Line 72: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<char>((buf >> bits) & 0xFF));
- Line 92: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(kB64Chars[(buf >> bits) & 0x3F]);
  Confidence: band=high; score=0.74
- Line 93: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(kB64Chars[(buf >> bits) & 0x3F]);
- Line 98: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(kB64Chars[buf & 0x3F]);
- Line 102: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back('=');
- Line 177: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: lines.push_back(std::move(line));
- Line 180: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: file.close();
- Line 434: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(std::launch::async, processChunk, start, end));
  Confidence: band=high; score=0.74
- Line 435: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(std::async(std::launch::async, processChunk, start, end));

### src/cache/cache_replication.cpp
Total findings: 9

- Line 175: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: void CacheReplicationManager::notifyWrite(const std::string &key, const std::string &payload,
  Confidence: band=very_high; score=0.99
- Line 224: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &state : replicas_) {
  Confidence: band=very_high; score=0.9
- Line 145: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ++unhealthy_count;
- Line 148: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ++unhealthy_count;
- Line 230: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 231: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(std::move(r));
- Line 248: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ++unhealthy_count;
- Line 249: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: continue; // Skip until probed healthy again
- Line 288: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ++unhealthy_count;

### src/cache/bounded_lru_cache.cpp
Total findings: 8

- Line 120: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = cache_.find(key);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 182: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->prev = nullptr;
- Line 182: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: node->prev = nullptr;
- Line 209: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: node->prev = nullptr;
- Line 209: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: node->prev = nullptr;
- Line 232: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: tail_->next = nullptr;

### src/cache/cache_hit_rate_slo_monitor.cpp
Total findings: 4

- Line 304: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto res = alertmanager_->resolveAlert(active_warning_alert_id_);
- Line 412: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto res = alertmanager_->resolveAlert(active_latency_warning_alert_id_);
- Line 165: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 170: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);

### src/cache/grpc_remote_cache_peer.cpp
Total findings: 4

- Line 112: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto rpc = stub_->PrepareUnaryCall(&ctx, kInvalidateMethod, request_buf, &cq);
- Line 128: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(msg);
- Line 141: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(msg);
- Line 110: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: grpc::Status          status;

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
