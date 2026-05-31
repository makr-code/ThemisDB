# transaction Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: transaction
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 554
- Actionable Findings (Critical + High): 298
- Affected Files: 18

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 51 |
| High | 247 |
| Medium | 256 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| container | 178 |
| performance_patterns | 148 |
| raii | 40 |
| reliability | 33 |
| exception_safety | 30 |
| determinism | 29 |
| concurrency | 21 |
| performance | 20 |
| distributed_consistency | 18 |
| memory | 9 |
| legacy_duplication | 7 |
| observability | 7 |
| security | 6 |
| audit_logging | 5 |
| uninitialized | 2 |
| platform | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/transaction/transaction_manager.cpp | 106 | 15 | 46 | 45 | 0 |
| src/transaction/distributed_saga.cpp | 91 | 15 | 21 | 55 | 0 |
| src/transaction/distributed_transaction_manager.cpp | 70 | 4 | 50 | 16 | 0 |
| src/transaction/saga_orchestrator.cpp | 62 | 1 | 16 | 45 | 0 |
| src/transaction/merge_engine.cpp | 41 | 1 | 11 | 29 | 0 |
| src/transaction/lock_manager.cpp | 38 | 8 | 14 | 16 | 0 |
| src/transaction/deadlock_predictor.cpp | 28 | 1 | 22 | 5 | 0 |
| src/transaction/global_transaction_manager.cpp | 27 | 0 | 27 | 0 | 0 |
| src/transaction/crash_recovery_manager.cpp | 20 | 1 | 5 | 14 | 0 |
| src/transaction/snapshot_manager.cpp | 17 | 0 | 10 | 7 | 0 |
| src/transaction/transaction_batcher.cpp | 17 | 1 | 14 | 2 | 0 |
| src/transaction/branch_manager.cpp | 12 | 0 | 5 | 7 | 0 |
| src/transaction/transaction_auditor.cpp | 8 | 0 | 3 | 5 | 0 |
| src/transaction/transaction_semantic_advisor.cpp | 6 | 2 | 1 | 3 | 0 |
| src/transaction/saga_plugin/saga_orchestrator_plugin.cpp | 5 | 1 | 1 | 3 | 0 |
| src/transaction/saga.cpp | 3 | 0 | 0 | 3 | 0 |
| src/transaction/saga_plugin_bridge.cpp | 2 | 1 | 1 | 0 | 0 |
| include/transaction/examples/transaction_semantic_advisor_example.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### src/transaction/transaction_manager.cpp
Total findings: 106

- Line 124: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = waiting_for_.find(txn_id);
- Line 137: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = held_locks_.find(key);
- Line 195: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator lock_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto lock_it = held_locks_.find(key);
- Line 197: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: TransactionId holding_txn = lock_it->second.holder;
- Line 323: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator wit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto wit = waiting_for_.find(txn_id);
- Line 343: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = held_locks_.begin(); it != held_locks_.end();) {
- Line 556: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = active_transactions_.find(id);
- Line 559: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator completed_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto completed_it = completed_transactions_.find(id);
- Line 667: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = completed_transactions_.begin(); it != completed_transactions_.end(); ) {
- Line 727: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator ac may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto ac = active_counts.find(tid);
- Line 1537: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto conflicting_txn = lock_manager_->checkPredicateConflict(id_, key);
- Line 1615: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator base_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto base_it = base_values_.find(key);
- Line 1630: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: conflict_set_id = conflict_mgr_->storeConflictSet(cset);
- Line 1630: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: conflict_set_id = conflict_mgr_->storeConflictSet(cset);
- Line 1999: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto ts_opt = snapshot_mgr_->getTimestampForTag(tag_name);
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
- Line 108: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& deadlock : recent_deadlocks_) {
  Confidence: band=very_high; score=0.9
- Line 161: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(deadlock_detector_mutex_);
- Line 195: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto lock_it = held_locks_.find(key);
  Confidence: band=very_high; score=0.9
- Line 220: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: auto cycle_start = std::find(path.begin(), path.end(), neighbor);
- Line 295: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: uint64_t prev = deadlock_max_cycle_len_.load(std::memory_order_relaxed);
- Line 315: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (deadlock_predictor_.load(std::memory_order_acquire)) {
- Line 359: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (DeadlockPredictor* dp = deadlock_predictor_.load(std::memory_order_acquire)) {
- Line 494: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (DeadlockPredictor* dp = deadlock_predictor_.load(std::memory_order_acquire)) {
- Line 498: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [key, info] : held_locks_) {
  Confidence: band=very_high; score=0.9
- Line 538: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (DeadlockPredictor* dp = deadlock_predictor_.load(std::memory_order_acquire)) {
- Line 542: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [key, info] : held_locks_) {
  Confidence: band=very_high; score=0.9
- Line 615: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: stats.total_begun = total_begun_.load(std::memory_order_relaxed);
- Line 616: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: stats.total_committed = total_committed_.load(std::memory_order_relaxed);
- Line 680: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [txn_id, txn] : active_transactions_) {
  Confidence: band=very_high; score=0.9
- Line 721: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [tid, entry] : tenant_stats_) {
  Confidence: band=very_high; score=0.9
- Line 726: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto ac = active_counts.find(tid);
- Line 726: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto ac = active_counts.find(tid);
- Line 743: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [txn_id, txn] : active_transactions_) {
  Confidence: band=very_high; score=0.9
- Line 832: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Direct transaction (legacy API)
  Confidence: band=high; score=0.8
- Line 874: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to create MVCC transaction");
- Line 911: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (finished_.load(std::memory_order_acquire)) {
- Line 1518: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (finished_.load(std::memory_order_acquire)) {
- Line 1527: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: lock_manager_->acquirePredicateLock(id_, start_key, end_key);
- Line 1528: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: THEMIS_DEBUG("Transaction {} acquired predicate lock on [{}, {}]", id_, start_key, end_key);
- Line 1528: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_DEBUG("Transaction {} acquired predicate lock on [{}, {}]", id_, start_key, end_key);
- Line 1547: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function commit without trace point
  Context: TransactionManager::Status TransactionManager::Transaction::commit() {
  Confidence: band=very_high; score=0.9
- Line 1575: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function commit without trace point
  Context: THEMIS_DEBUG("Transaction {} is read-only — skipping WAL write on commit (duration: {} ms)",
  Confidence: band=very_high; score=0.9
- Line 1614: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto base_it = base_values_.find(key);
- Line 1678: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (finished_.load(std::memory_order_acquire)) {
- Line 1689: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (finished_.load(std::memory_order_acquire)) {
- Line 1702: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (finished_.load(std::memory_order_acquire)) {
- Line 1715: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (finished_.load(std::memory_order_acquire)) {
- Line 1741: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (finished_.load(std::memory_order_acquire)) {
- Line 1846: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (TransactionId id : expired) {
  Confidence: band=very_high; score=0.9
- Line 1917: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: result.is_finished    = finished_.load(std::memory_order_acquire);
- Line 1921: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [key, lock_type] : lock_manager_->getLocksHeld(id_)) {
  Confidence: band=very_high; score=0.9
- Line 2030: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return deadlock_predictor_.load(std::memory_order_acquire);
- Line 2036: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: DeadlockPredictor* p = deadlock_predictor_.load(std::memory_order_acquire);
- Line 2055: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: DeadlockPredictor* p = deadlock_predictor_.load(std::memory_order_acquire);
- Line 2068: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: DeadlockPredictor* p = deadlock_predictor_.load(std::memory_order_acquire);
- Line 109: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(deadlock);
  Confidence: band=high; score=0.74
- Line 110: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(deadlock);
- Line 191: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<TransactionId, std::unordered_set<TransactionId>> wait_graph;
  Confidence: band=medium; score=0.66
- Line 212: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: path.push_back(node);
  Confidence: band=high; score=0.74
- Line 212: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: path.push_back(node);
  Confidence: band=high; score=0.74
- Line 213: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: path.push_back(node);
- Line 316: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 319: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cycle_keys.push_back(key);
  Confidence: band=high; score=0.74
- Line 319: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cycle_keys.push_back(key);
  Confidence: band=high; score=0.74
- Line 320: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cycle_keys.push_back(key);
- Line 326: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cycle_keys.push_back(wkey);
  Confidence: band=high; score=0.74
- Line 327: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cycle_keys.push_back(wkey);
- Line 334: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: recent_deadlocks_.push_back(info);
- Line 498: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (info.holder == id) keys.push_back(key);
  Confidence: band=high; score=0.74
- Line 499: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (info.holder == id) keys.push_back(key);
- Line 542: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (info.holder == id) keys.push_back(key);
  Confidence: band=high; score=0.74
- Line 543: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (info.holder == id) keys.push_back(key);
- Line 712: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, uint64_t> active_counts;
  Confidence: band=medium; score=0.66
- Line 728: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 729: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(s));
- Line 744: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(txn_id);
  Confidence: band=high; score=0.74
- Line 745: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ids.push_back(txn_id);
- Line 757: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_abort.push_back(txn_id);
  Confidence: band=high; score=0.74
- Line 758: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: to_abort.push_back(txn_id);
- Line 816: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: expired.push_back(id);
  Confidence: band=high; score=0.74
- Line 817: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: expired.push_back(id);
- Line 1294: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: k += "occ:ver:";
  Confidence: band=high; score=0.74
- Line 1294: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: k += "occ:ver:";
  Confidence: band=high; score=0.74
- Line 1608: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conflict_keys.push_back(key);
  Confidence: band=high; score=0.74
- Line 1609: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: conflict_keys.push_back(key);
- Line 1622: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: conflict_record_ids.push_back(cid);
- Line 1736: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: savepoints_.push_back({std::move(sname), saga_->stepCount()});
- Line 1800: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(e.name);
  Confidence: band=high; score=0.74
- Line 1801: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: names.push_back(e.name);
- Line 1841: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: expired.push_back(id);
  Confidence: band=high; score=0.74
- Line 1842: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: expired.push_back(id);
- Line 1921: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.locks_held.push_back({key, lockTypeName(lock_type)});
  Confidence: band=high; score=0.74
- Line 1922: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.locks_held.push_back({key, lockTypeName(lock_type)});
- Line 2015: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(toTimeTravelRecord(rec));
  Confidence: band=high; score=0.74
- Line 2016: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(toTimeTravelRecord(rec));
- Line 2145: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: others.emplace_back(id, lock_manager_.getPredicateLockRanges(id));
  Confidence: band=high; score=0.74
- Line 2170: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(sc));
  Confidence: band=high; score=0.74
- Line 2170: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(sc));
  Confidence: band=high; score=0.74
- Line 2170: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(sc));
  Confidence: band=high; score=0.74
- Line 2171: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(sc));

### src/transaction/distributed_saga.cpp
Total findings: 91

- Line 53: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: if (!names.insert(step.name).second) {
  Confidence: band=very_high; score=0.99
- Line 222: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: listed.insert(rec.name);
  Confidence: band=very_high; score=0.99
- Line 228: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: journalWrite(saga.saga_id, "COMPENSATING", failure_reason);
  Confidence: band=very_high; score=0.99
- Line 248: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: journalWrite(saga.saga_id,
  Confidence: band=very_high; score=0.99
- Line 259: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: journalWrite(saga.saga_id, "COMPLETED");
  Confidence: band=very_high; score=0.99
- Line 307: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it   = ready.begin();
- Line 313: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: ready.insert(succ);
  Confidence: band=very_high; score=0.99
- Line 336: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto st = executeStep(step_map.at(name), *it->second);
- Line 350: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = index.find(name);
- Line 607: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: // journalWrite()
  Confidence: band=very_high; score=0.99
- Line 610: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: void DistributedSagaCoordinator::journalWrite(
  Confidence: band=very_high; score=0.99
- Line 760: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Only insert if not already present (a concurrent execute() may have
  Confidence: band=very_high; score=0.99
- Line 891: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: journalWrite(saga_id, "FORCE_COMPENSATED");
  Confidence: band=very_high; score=0.99
- Line 898: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = reports_.find(saga_id);
- Line 902: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: journalWrite(saga_id, "FORCE_COMPLETED");
  Confidence: band=very_high; score=0.99
- Line 65: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (names.find(dep) == names.end()) {
  Confidence: band=very_high; score=0.9
- Line 110: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: DistributedSagaReport DistributedSagaCoordinator::execute(
- Line 165: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::logic_error("Internal error: step record not found: " + name);
- Line 219: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: listed.find(rec.name) == listed.end()) {
- Line 237: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& name : to_compensate) {
  Confidence: band=very_high; score=0.9
- Line 349: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = index.find(name);
- Line 351: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: StepRecord* rec = (it != index.end()) ? it->second : nullptr;
- Line 351: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: StepRecord* rec = (it != index.end()) ? it->second : nullptr;
- Line 369: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto st = futures[i].get();
  Confidence: band=very_high; score=0.9
- Line 411: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t attempt = 0; attempt <= step.max_retries; ++attempt) {
  Confidence: band=very_high; score=0.9
- Line 414: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(backoff);
  Confidence: band=very_high; score=0.9
- Line 421: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: auto wait_result = fut.wait_for(timeout);
  Confidence: band=very_high; score=0.9
- Line 437: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: last_status = fut.get();
  Confidence: band=very_high; score=0.9
- Line 487: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto rit = index.find(name);
- Line 541: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t attempt = 0; attempt <= step.max_retries; ++attempt) {
  Confidence: band=very_high; score=0.9
- Line 544: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(backoff);
  Confidence: band=very_high; score=0.9
- Line 549: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: auto wait_result = fut.wait_for(timeout);
  Confidence: band=very_high; score=0.9
- Line 561: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: last_status = fut.get();
  Confidence: band=very_high; score=0.9
- Line 706: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return execute(local_def);
- Line 750: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [sid, event] : latest_event) {
  Confidence: band=very_high; score=0.9
- Line 867: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& d : step.depends_on) {
  Confidence: band=very_high; score=0.9
- Line 48: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> names;
  Confidence: band=medium; score=0.66
- Line 74: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::string>> adj;
  Confidence: band=medium; score=0.66
- Line 77: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adj[dep].push_back(step.name);
  Confidence: band=high; score=0.74
- Line 77: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adj[dep].push_back(step.name);
  Confidence: band=high; score=0.74
- Line 77: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adj[dep].push_back(step.name);
  Confidence: band=high; score=0.74
- Line 77: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adj[dep].push_back(step.name);
  Confidence: band=high; score=0.74
- Line 78: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: adj[dep].push_back(step.name);
- Line 83: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, Color> color;
  Confidence: band=medium; score=0.66
- Line 146: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, DistributedSagaStep> step_map;
  Confidence: band=high; score=0.74
- Line 148: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, StepRecord*> record_index;
  Confidence: band=medium; score=0.66
- Line 153: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.step_records.push_back(rec);
  Confidence: band=high; score=0.74
- Line 154: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: report.step_records.push_back(rec);
- Line 174: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int> level;
  Confidence: band=medium; score=0.66
- Line 186: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& [name, lvl] : level) waves[lvl].push_back(name);
  Confidence: band=high; score=0.74
- Line 186: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& [name, lvl] : level) waves[lvl].push_back(name);
  Confidence: band=high; score=0.74
- Line 186: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& [name, lvl] : level) waves[lvl].push_back(name);
  Confidence: band=high; score=0.74
- Line 187: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& [name, lvl] : level) waves[lvl].push_back(name);
- Line 204: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: executed_order.push_back(name);
  Confidence: band=high; score=0.74
- Line 204: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: executed_order.push_back(name);
  Confidence: band=high; score=0.74
- Line 204: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: executed_order.push_back(name);
  Confidence: band=high; score=0.74
- Line 205: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: executed_order.push_back(name);
- Line 214: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> listed(
  Confidence: band=medium; score=0.66
- Line 220: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_compensate.push_back(rec.name);
  Confidence: band=high; score=0.74
- Line 221: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: to_compensate.push_back(rec.name);
- Line 287: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int>                  in_degree;
  Confidence: band=medium; score=0.66
- Line 288: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::string>> successors;
  Confidence: band=medium; score=0.66
- Line 292: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: successors[dep].push_back(step.name);
  Confidence: band=high; score=0.74
- Line 292: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: successors[dep].push_back(step.name);
  Confidence: band=high; score=0.74
- Line 293: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: successors[dep].push_back(step.name);
- Line 309: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(node);
  Confidence: band=high; score=0.74
- Line 310: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(node);
- Line 326: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, DistributedSagaStep>&  step_map,
  Confidence: band=high; score=0.74
- Line 346: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::vector<std::future<DistributedSagaStatus>> futures;
- Line 351: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: wave_records.push_back(rec);
  Confidence: band=high; score=0.74
- Line 351: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: wave_records.push_back(rec);
  Confidence: band=high; score=0.74
- Line 352: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: wave_records.push_back(rec);
- Line 355: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(
- Line 366: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: DistributedSagaStatus wave_status;
- Line 369: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto st = futures[i].get();
  Confidence: band=high; score=0.74
- Line 379: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 409: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: DistributedSagaStatus last_status;
- Line 447: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 465: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: step.name, attempt + 1, last_status.message);
- Line 480: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, DistributedSagaStep>& step_map,
  Confidence: band=high; score=0.74
- Line 539: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: DistributedSagaStatus last_status;
- Line 571: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 578: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: step.name, attempt + 1, last_status.message);
- Line 632: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"')  escaped += "\\\"";
  Confidence: band=high; score=0.74
- Line 633: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '"')  escaped += "\\\"";
- Line 634: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\\') escaped += "\\\\";
- Line 702: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: local_def.steps.push_back(remoteStepToLocal(remote_step));
  Confidence: band=high; score=0.74
- Line 703: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: local_def.steps.push_back(remoteStepToLocal(remote_step));
- Line 766: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: recovered.push_back(sid);
  Confidence: band=high; score=0.74
- Line 767: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: recovered.push_back(sid);
- Line 785: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> phase_label;
  Confidence: band=medium; score=0.66

### src/transaction/distributed_transaction_manager.cpp
Total findings: 70

- Line 305: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 368: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 424: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 1227: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: pool_cv_.wait(lock, [this] {
- Line 49: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(s_rpc_phase2_fn_mutex);
- Line 73: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(s_rpc_phase1_fn_mutex);
- Line 97: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(s_liveness_check_fn_mutex);
- Line 189: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("DistributedTransactionManager [{}] WAL initialised at {}",
- Line 199: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: batch_flush_thread_ = std::thread(&DistributedTransactionManager::batchFlushLoop, this);
  Confidence: band=very_high; score=0.9
- Line 244: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 299: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: all_voted_commit = fut.get();
  Confidence: band=very_high; score=0.9
- Line 306: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: txn = findTransaction(txn_id);  // re-acquire pointer after re-lock
- Line 507: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_DEBUG("DistributedTransactionManager [{}] recoverInDoubtTransactions: WAL disabled; "
- Line 519: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [tid, txn] : transactions_) {
  Confidence: band=very_high; score=0.9
- Line 529: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& item : pending) {
  Confidence: band=very_high; score=0.9
- Line 531: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 542: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("DistributedTransactionManager [{}] starting in-doubt recovery", coordinator_id_);
- Line 577: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [tid, type] : last_decision) {
  Confidence: band=very_high; score=0.9
- Line 602: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("DistributedTransactionManager [{}] recovery: broadcasting ABORT for "
- Line 617: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("DistributedTransactionManager [{}] recovery complete: {} in-doubt txns resolved",
- Line 632: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [tid, txn] : transactions_) {
  Confidence: band=very_high; score=0.9
- Line 643: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& tid : timed_out) {
  Confidence: band=very_high; score=0.9
- Line 709: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("DistributedTransactionManager [{}] liveness_check_fn threw for "
- Line 714: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("DistributedTransactionManager [{}] liveness_check_fn threw for "
- Line 719: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("DistributedTransactionManager [{}] liveness_check_fn threw for "
- Line 731: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("DistributedTransactionManager [{}] static liveness check threw for "
- Line 736: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("DistributedTransactionManager [{}] static liveness check threw for "
- Line 741: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("DistributedTransactionManager [{}] static liveness check threw for "
- Line 847: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return (it != transactions_.end()) ? &it->second : nullptr;
- Line 847: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return (it != transactions_.end()) ? &it->second : nullptr;
- Line 853: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return (it != transactions_.end()) ? &it->second : nullptr;
- Line 853: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return (it != transactions_.end()) ? &it->second : nullptr;
- Line 892: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("DistributedTransactionManager [{}] cannot send Phase-1 PREPARE for "
- Line 975: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backwards-compatibility path: a Phase-2 bridge is configured but
  Confidence: band=high; score=0.8
- Line 994: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function abort without trace point
  Context: "(remote) — voting ABORT (no RPC bridge configured)",
  Confidence: band=very_high; score=0.9
- Line 1023: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& fut : futures) {
  Confidence: band=very_high; score=0.9
- Line 1031: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: const auto status = fut.wait_for(remaining);
  Confidence: band=very_high; score=0.9
- Line 1038: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: const VoteResult result = fut.get();
  Confidence: band=very_high; score=0.9
- Line 1093: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // bridges (new Phase2RpcFn, legacy static RpcPhase2Fn, or
- Line 1093: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // bridges (new Phase2RpcFn, legacy static RpcPhase2Fn, or
  Confidence: band=high; score=0.8
- Line 1102: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("DistributedTransactionManager [{}] cannot deliver Phase-2 {} for "
- Line 1146: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("DistributedTransactionManager [{}] cannot deliver Phase-2 {} for remote "
- Line 1198: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (!fut.get()) {
  Confidence: band=very_high; score=0.9
- Line 1221: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < n; ++i) {
  Confidence: band=very_high; score=0.9
- Line 1226: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(pool_mutex_);
- Line 1226: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(pool_mutex_);
- Line 1239: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_DEBUG("DistributedTransactionManager [{}] thread pool started: {} workers",
- Line 1249: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& t : worker_threads_) {
  Confidence: band=very_high; score=0.9
- Line 1263: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(batch_mutex_);
- Line 1274: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_DEBUG("DistributedTransactionManager [{}] batch-flush: {} transactions",
- Line 1279: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // worker pool (or std::async in legacy mode). Submitting the whole
  Confidence: band=high; score=0.8
- Line 1287: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("DistributedTransactionManager [{}] batch Phase-1 failed for txn {}: {}",
- Line 1291: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("DistributedTransactionManager [{}] batch Phase-1 failed for txn {}: {}",
- Line 1295: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("DistributedTransactionManager [{}] batch Phase-1 failed for txn {}: {}",
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 296: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batch_queue_.push_back({txn_id, std::move(promise)});
- Line 524: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pending.push_back(PendingAbort{tid, txn.participants});
  Confidence: band=high; score=0.74
- Line 525: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pending.push_back(PendingAbort{tid, txn.participants});
- Line 636: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: timed_out.push_back(tid);
  Confidence: band=high; score=0.74
- Line 637: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: timed_out.push_back(tid);
- Line 894: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(submitTask([nid]() -> VoteResult {
  Confidence: band=high; score=0.74
- Line 895: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(submitTask([nid]() -> VoteResult {
- Line 905: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(submitTask([nid]() -> VoteResult {
- Line 915: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(submitTask([rpc_fn, ep, nid, tid, cid, keys]() -> VoteResult {
- Line 934: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(submitTask([dispatch, ep, nid, tid, cid, keys]() -> VoteResult {
- Line 952: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(submitTask([legacy_p1_fn, ep, nid, tid, cid, keys]() -> VoteResult {
- Line 1111: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(submitTask([rpc_fn, ep, nid, tid, cid, dc]() {
- Line 1158: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(submitTask([cb, nid, tid, cid]() {
- Line 1169: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(submitTask([cb, nid, tid, cid]() {
- Line 1221: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: worker_threads_.emplace_back([this] {
  Confidence: band=high; score=0.74

### src/transaction/saga_orchestrator.cpp
Total findings: 62

- Line 442: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = step_map.find(name);
- Line 85: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& step : saga.steps) {
  Confidence: band=very_high; score=0.9
- Line 86: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& dep : step.depends_on) {
  Confidence: band=very_high; score=0.9
- Line 87: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (names.find(dep) == names.end()) {
  Confidence: band=very_high; score=0.9
- Line 115: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::out_of_range("saga template not found: " + template_name);
- Line 141: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: const auto it = dependents.find(step.name);
- Line 141: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: const auto it = dependents.find(step.name);
- Line 213: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& step : saga.steps) {
  Confidence: band=very_high; score=0.9
- Line 231: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t attempt = 0; attempt <= step.max_retries; ++attempt) {
  Confidence: band=very_high; score=0.9
- Line 233: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(metrics_mutex_);
- Line 277: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto step_it = step_map.find(*it);
- Line 277: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = executed_order.rbegin(); it != executed_order.rend(); ++it) {
  Confidence: band=very_high; score=0.9
- Line 342: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: SagaOrchestratorStatus SAGAOrchestrator::execute(const SAGADefinition& saga) {
- Line 342: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: SagaOrchestratorStatus SAGAOrchestrator::execute(const SAGADefinition& saga) {
  Confidence: band=very_high; score=0.9
- Line 347: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: const auto seq = next_id.fetch_add(1, std::memory_order_relaxed);
- Line 441: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = step_map.find(name);
- Line 441: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = step_map.find(name);
- Line 31: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '\\': out += "\\\\"; break;
  Confidence: band=high; score=0.74
- Line 32: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': out += "\\\\"; break;
- Line 33: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"': out += "\\\""; break;
- Line 34: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\n': out += "\\n"; break;
- Line 35: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\r': out += "\\r"; break;
- Line 36: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: default: out.push_back(c); break;
  Confidence: band=high; score=0.74
- Line 36: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\t': out += "\\t"; break;
- Line 37: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: default: out.push_back(c); break;
- Line 110: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> context_overrides) const {
  Confidence: band=high; score=0.74
- Line 131: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::string>> dependents;
  Confidence: band=medium; score=0.66
- Line 136: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dependents[dep].push_back(step.name);
  Confidence: band=high; score=0.74
- Line 136: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dependents[dep].push_back(step.name);
  Confidence: band=high; score=0.74
- Line 136: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dependents[dep].push_back(step.name);
  Confidence: band=high; score=0.74
- Line 136: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dependents[dep].push_back(step.name);
  Confidence: band=high; score=0.74
- Line 137: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: dependents[dep].push_back(step.name);
- Line 163: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> indegree;
  Confidence: band=medium; score=0.66
- Line 164: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::string>> outgoing;
  Confidence: band=medium; score=0.66
- Line 177: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: outgoing[dep].push_back(step.name);
- Line 194: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: order.push_back(current);
- Line 260: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 302: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: status_rec.failure_reason += " | compensation failed for " + step.name + ": " + ex.what();
  Confidence: band=high; score=0.74
- Line 312: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 361: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: SAGAExecutionStatus status_rec;
- Line 378: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ++status_rec.completed_steps;
- Line 381: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ++status_rec.failed_steps;
- Line 384: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ++status_rec.skipped_steps;
- Line 389: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ++status_rec.pending_steps;
- Line 401: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> remaining_dependencies;
  Confidence: band=medium; score=0.66
- Line 402: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::string>> dependents;
  Confidence: band=medium; score=0.66
- Line 411: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: dependents[dep].push_back(step.name);
- Line 420: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ready.push_back(name);
- Line 448: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(
  Confidence: band=high; score=0.74
- Line 449: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(std::async(
- Line 457: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(future.get());
  Confidence: band=high; score=0.74
- Line 458: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(future.get());
- Line 460: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({std::string{}, StepState::FAILED});
- Line 464: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 465: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({std::string{}, StepState::FAILED});
- Line 489: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: executed.push_back(name);
  Confidence: band=high; score=0.74
- Line 490: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: executed.push_back(name);
- Line 491: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: succeeded_in_wave.push_back(name);
- Line 496: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: succeeded_in_wave.push_back(name);
- Line 514: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ready.push_back(dependent);
  Confidence: band=high; score=0.74
- Line 514: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ready.push_back(dependent);
  Confidence: band=high; score=0.74
- Line 515: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ready.push_back(dependent);

### src/transaction/merge_engine.cpp
Total findings: 41

- Line 528: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator res_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto res_it = resolution_map.find(conflict.key);
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
- Line 327: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (conflict_keys.find(change.key) == conflict_keys.end()) {
- Line 327: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (conflict_keys.find(change.key) == conflict_keys.end()) {
- Line 527: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto res_it = resolution_map.find(conflict.key);
- Line 602: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: event.metadata["merge"] = true;
- Line 603: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: event.metadata["original_sequence"] = change.sequence;
- Line 100: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resolutions.push_back(res.toJson());
  Confidence: band=high; score=0.74
- Line 101: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: resolutions.push_back(res.toJson());
- Line 120: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: opts.manual_resolutions.push_back(ConflictResolution::fromJson(res_json));
  Confidence: band=high; score=0.74
- Line 121: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: opts.manual_resolutions.push_back(ConflictResolution::fromJson(res_json));
- Line 159: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conflicts_arr.push_back(conflict.toJson());
  Confidence: band=high; score=0.74
- Line 160: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: conflicts_arr.push_back(conflict.toJson());
- Line 165: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: changes_arr.push_back(change.toJson());
  Confidence: band=high; score=0.74
- Line 166: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: changes_arr.push_back(change.toJson());
- Line 187: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.conflicts.push_back(Conflict::fromJson(conflict_json));
  Confidence: band=high; score=0.74
- Line 188: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.conflicts.push_back(Conflict::fromJson(conflict_json));
- Line 193: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.changes_applied.push_back(analytics::DiffEngine::Change::fromJson(change_json));
  Confidence: band=high; score=0.74
- Line 194: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.changes_applied.push_back(analytics::DiffEngine::Change::fromJson(change_json));
- Line 270: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.changes_applied.push_back(change);
  Confidence: band=high; score=0.74
- Line 271: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.changes_applied.push_back(change);
- Line 273: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.changes_applied.push_back(change);
  Confidence: band=high; score=0.74
- Line 274: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.changes_applied.push_back(change);
- Line 276: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.changes_applied.push_back(change);
  Confidence: band=high; score=0.74
- Line 277: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.changes_applied.push_back(change);
- Line 321: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> conflict_keys;
  Confidence: band=medium; score=0.66
- Line 328: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.changes_applied.push_back(change);
  Confidence: band=high; score=0.74
- Line 328: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.changes_applied.push_back(change);
  Confidence: band=high; score=0.74
- Line 329: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.changes_applied.push_back(change);
- Line 461: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, const analytics::DiffEngine::Change*> source_map;
  Confidence: band=medium; score=0.66
- Line 462: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, const analytics::DiffEngine::Change*> target_map;
  Confidence: band=medium; score=0.66
- Line 521: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, const ConflictResolution*> resolution_map;
  Confidence: band=medium; score=0.66
- Line 537: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resolved_changes.push_back(change);
  Confidence: band=high; score=0.74
- Line 537: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resolved_changes.push_back(change);
  Confidence: band=high; score=0.74
- Line 538: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: resolved_changes.push_back(change);
- Line 546: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: resolved_changes.push_back(*auto_resolved);

### src/transaction/lock_manager.cpp
Total findings: 38

- Line 131: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator txn_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto txn_it = held_by_txn_.find(txn_id);
- Line 144: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator lt_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto lt_it = lock_table_.find(key);
- Line 169: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator txn_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto txn_it = held_by_txn_.find(txn_id);
- Line 185: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator lt_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto lt_it = lock_table_.find(key);
- Line 364: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = lock_table_.find(key);
- Line 378: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = waiting_for_.find(txn_id);
- Line 458: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator colon_pos may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto colon_pos = key.find(':');
- Line 486: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator lt_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto lt_it = lock_table_.find(rk);
- Line 21: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Compatibility matrix for lock types
  Confidence: band=high; score=0.8
- Line 23: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Compatibility table (held × requested):
  Confidence: band=high; score=0.8
- Line 54: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: LockManager::LockResult LockManager::acquireLock(
- Line 65: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: "2PL violation: transaction is in shrinking phase and cannot acquire new locks");
- Line 87: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: stats_acquired_.fetch_add(1, std::memory_order_relaxed);
- Line 184: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto lt_it = lock_table_.find(key);
- Line 303: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [k, lt] : it->second) {
  Confidence: band=very_high; score=0.9
- Line 349: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [txn, keys] : held_by_txn_) {
  Confidence: band=very_high; score=0.9
- Line 367: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& req : it->second.waiters) {
  Confidence: band=very_high; score=0.9
- Line 419: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Check compatibility with all current holders
  Confidence: band=high; score=0.8
- Line 452: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: const size_t threshold = escalation_threshold_.load(std::memory_order_relaxed);
- Line 465: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (k.find(table_prefix) == 0 && k.size() > table_prefix.size()) {
- Line 510: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: bool LockManager::acquirePredicateLock(TransactionId txn_id,
- Line 562: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& pl : predicate_locks_) {
  Confidence: band=very_high; score=0.9
- Line 180: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back(k);
  Confidence: band=high; score=0.74
- Line 181: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: keys.push_back(k);
- Line 303: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(k, lt);
  Confidence: band=high; score=0.74
- Line 367: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(req->txn_id);
  Confidence: band=high; score=0.74
- Line 367: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(req->txn_id);
  Confidence: band=high; score=0.74
- Line 368: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(req->txn_id);
- Line 380: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(it->second);
- Line 403: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entry.holders.push_back({txn_id, type, std::chrono::system_clock::now()});
  Confidence: band=high; score=0.74
- Line 404: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entry.holders.push_back({txn_id, type, std::chrono::system_clock::now()});
- Line 429: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entry.holders.push_back(
  Confidence: band=high; score=0.74
- Line 429: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entry.holders.push_back(
  Confidence: band=high; score=0.74
- Line 430: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entry.holders.push_back(
- Line 466: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: row_keys.push_back(k);
  Confidence: band=high; score=0.74
- Line 467: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: row_keys.push_back(k);
- Line 524: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: predicate_locks_.push_back({txn_id, start_key, end_key});
- Line 587: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(pl.start_key, pl.end_key);
  Confidence: band=high; score=0.74

### src/transaction/deadlock_predictor.cpp
Total findings: 28

- Line 259: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = hold_times_.find(key);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 39: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: const std::vector<std::string>& locks_acquired,
- Line 42: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (locks_acquired.empty()) {
- Line 52: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: static_cast<long>(locks_acquired.size())};
- Line 53: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: for (const auto& key : locks_acquired) {
- Line 65: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (p.keys == locks_acquired) {
- Line 77: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: pat.keys       = locks_acquired;
- Line 93: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: for (size_t i = 0; i < locks_acquired.size(); ++i) {
- Line 94: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: for (size_t j = i + 1; j < locks_acquired.size(); ++j) {
- Line 95: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: const std::string pk = makePairKey(locks_acquired[i], locks_acquired[j]);
- Line 163: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& k : keys) {
  Confidence: band=very_high; score=0.9
- Line 164: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (std::find(p.keys.begin(), p.keys.end(), k) == p.keys.end()) {
  Confidence: band=very_high; score=0.9
- Line 164: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(p.keys.begin(), p.keys.end(), k) == p.keys.end()) {
- Line 164: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(p.keys.begin(), p.keys.end(), k) == p.keys.end()) {
- Line 241: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (dx != dy) return dx < dy;
  Confidence: band=very_high; score=0.9
- Line 258: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = hold_times_.find(key);
- Line 286: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(mutex_);
- Line 291: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(mutex_);
- Line 327: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = pair_conflicts_.find(pk);
- Line 327: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = pair_conflicts_.find(pk);
- Line 328: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = pair_conflicts_.find(pk);
  Confidence: band=very_high; score=0.9
- Line 54: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vec.push_back(per_key);
  Confidence: band=high; score=0.74
- Line 55: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vec.push_back(per_key);
- Line 79: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: patterns_.push_back(std::move(pat));
  Confidence: band=high; score=0.74
- Line 80: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: patterns_.push_back(std::move(pat));
- Line 219: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> danger;
  Confidence: band=medium; score=0.66

### src/transaction/global_transaction_manager.cpp
Total findings: 27

- Line 48: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("GlobalTransactionManager: truetime must not be null");
- Line 56: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("GlobalTransactionManager [{}] WAL initialised at {}",
- Line 70: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("participant must not be null");
- Line 74: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_DEBUG("GlobalTransactionManager [{}] registered region {}",
- Line 96: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("region_ids must not be empty");
- Line 104: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 118: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: rec.region_ops[rid]     = nlohmann::json::array();
- Line 132: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_DEBUG("GlobalTransactionManager [{}] began txn {} across {} region(s)",
- Line 146: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("GlobalTransactionManager [{}] addOperation: unknown txn {}",
- Line 153: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("GlobalTransactionManager [{}] addOperation: txn {} not ACTIVE",
- Line 160: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("GlobalTransactionManager [{}] addOperation: region {} not in txn {}",
- Line 169: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function commit without trace point
  Context: GlobalTxnOutcome GlobalTransactionManager::commit(const std::string& txn_id) {
  Confidence: band=very_high; score=0.9
- Line 284: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function abort without trace point
  Context: bool GlobalTransactionManager::abort(const std::string& txn_id) {
  Confidence: band=very_high; score=0.9
- Line 289: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("GlobalTransactionManager [{}] abort: unknown txn {}",
- Line 317: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("GlobalTransactionManager [{}] txn {} explicitly ABORTED",
- Line 329: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("GlobalTransactionManager [{}] recovering from WAL…", coordinator_id_);
- Line 397: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("GlobalTransactionManager [{}] in-doubt txn {} has no "
- Line 398: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function abort without trace point
  Context: "decision – marking ABORT (manual resolution may be required)",
  Confidence: band=very_high; score=0.9
- Line 410: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("GlobalTransactionManager [{}] re-driving in-doubt txn {} "
- Line 433: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("GlobalTransactionManager [{}] WAL recovery failed: {}",
- Line 437: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("GlobalTransactionManager [{}] recovery complete – {} in-doubt txn(s) resolved",
- Line 489: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("GlobalTransactionManager [{}] Phase 1 – region {} not found for txn {}",
- Line 504: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("GlobalTransactionManager [{}] Phase 1 – region {} threw on PREPARE "
- Line 512: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("GlobalTransactionManager [{}] region {} voted ABORT for txn {}",
- Line 525: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("GlobalTransactionManager [{}] Phase 2 – region {} not found for txn {} "
- Line 539: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("GlobalTransactionManager [{}] Phase 2 – region {} threw on {} "
- Line 571: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("GlobalTransactionManager [{}] WAL write failed for txn {}: {}",

### src/transaction/crash_recovery_manager.cpp
Total findings: 20

- Line 206: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = pending_ops_.find(txn_id);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 285: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto id : finished) begun.erase(id);
  Confidence: band=very_high; score=0.9
- Line 60: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += '=';
  Confidence: band=high; score=0.74
- Line 208: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: it->second.push_back(oe);
- Line 303: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<uint64_t> in_flight = scanInFlight();
  Confidence: band=medium; score=0.66
- Line 318: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<uint64_t, std::vector<OperationEntry>> ops_by_txn;
  Confidence: band=medium; score=0.66
- Line 330: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: while (std::getline(f, line)) all_lines.push_back(line);
  Confidence: band=high; score=0.74
- Line 331: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: while (std::getline(f, line)) all_lines.push_back(line);
- Line 334: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<uint64_t, std::vector<OperationEntry>> tmp_ops;
  Confidence: band=medium; score=0.66
- Line 343: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tmp_ops[entry->txn_id].push_back(entry->operation);
  Confidence: band=high; score=0.74
- Line 344: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tmp_ops[entry->txn_id].push_back(entry->operation);
- Line 377: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.rolled_back_ids.push_back(txn_id);
- Line 433: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: keep_lines.push_back(line);
- Line 439: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: keep_lines.push_back(line);
- Line 444: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: f.close();
- Line 482: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (e) result.push_back(*e);

### src/transaction/snapshot_manager.cpp
Total findings: 17

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #384 [WIP] Add Named Snapshots feature for ThemisDB MVCC system (2026-03-11T21:29:03Z)
- Line 138: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (iterator->Seek(prefix); iterator->Valid(); iterator->Next()) {
- Line 148: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: iterator->value().data(),
- Line 198: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: snapshot = nullptr;
  Context: spdlog::error("Failed to delete snapshot: {}", tag_name);
- Line 337: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (it->Seek(prefix); it->Valid(); it->Next()) {
- Line 399: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (it->Seek(prefix); it->Valid(); it->Next()) {
- Line 153: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: snapshots.push_back(*snapshot);
  Confidence: band=high; score=0.74
- Line 154: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: snapshots.push_back(*snapshot);
- Line 159: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: if (sort_by == "timestamp") {
  Confidence: band=high; score=0.74
- Line 198: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: spdlog::error("Failed to delete snapshot: {}", tag_name);
- Line 344: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (s.has_value()) snapshots.push_back(*s);
  Confidence: band=high; score=0.74
- Line 345: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (s.has_value()) snapshots.push_back(*s);
- Line 350: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort oldest-first by timestamp
  Confidence: band=high; score=0.74

### src/transaction/transaction_batcher.cpp
Total findings: 17

- Line 224: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: flush_cv_.wait(lk, [this] {
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 164: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (stopping_.load(std::memory_order_acquire)) {
- Line 249: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> cfg_lk(config_mutex_);
- Line 271: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: !stopping_.load(std::memory_order_acquire)) {
- Line 274: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Notified: recompute in case new items have earlier deadlines.
- Line 306: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (stopping_.load(std::memory_order_acquire)) {
- Line 413: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: recent_throughputs_.push_back(throughput);
  Confidence: band=very_high; score=0.9
- Line 414: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: if (recent_throughputs_.size() > kMaxThroughputSamples)
  Confidence: band=very_high; score=0.9
- Line 415: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: recent_throughputs_.pop_front();
  Confidence: band=very_high; score=0.9
- Line 418: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: recent_throughputs_.begin(), recent_throughputs_.end(), 0.0)
  Confidence: band=very_high; score=0.9
- Line 419: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: / static_cast<double>(recent_throughputs_.size());
  Confidence: band=very_high; score=0.9
- Line 286: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batch.push_back(std::move(queue_.front()));
- Line 312: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: remainder.push_back(std::move(queue_.front()));

### src/transaction/branch_manager.cpp
Total findings: 12

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 793: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: uint64_t counter = hist_counter.fetch_add(1, std::memory_order_relaxed);
- Line 817: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (it.Seek(prefix); it.Valid(); it.Next()) {
  Confidence: band=very_high; score=0.9
- Line 234: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: branches.push_back(branch.value());
  Confidence: band=high; score=0.74
- Line 235: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: branches.push_back(branch.value());
- Line 246: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: } else if (sort_by == "timestamp") {
  Confidence: band=high; score=0.74
- Line 394: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.conflicts.push_back(conflict.key);
  Confidence: band=high; score=0.74
- Line 395: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.conflicts.push_back(conflict.key);
- Line 824: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(*entry);
  Confidence: band=high; score=0.74
- Line 825: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(*entry);

### src/transaction/transaction_auditor.cpp
Total findings: 8

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 38: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!enabled_.load(std::memory_order_acquire)) return;
- Line 61: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = log_.rbegin(); it != log_.rend(); ++it) {
  Confidence: band=very_high; score=0.9
- Line 67: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(rec);
  Confidence: band=high; score=0.74
- Line 68: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(rec);
- Line 128: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ops.push_back(std::move(j));
  Confidence: band=high; score=0.74
- Line 129: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ops.push_back(std::move(j));
- Line 150: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += '\n';

### src/transaction/transaction_semantic_advisor.cpp
Total findings: 6

- Line 140: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != b.entity_map.end() && it->second == id) {
- Line 168: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != b.entity_map.end() && it->second == id) {
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 84: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hints.push_back(std::move(hint));
  Confidence: band=high; score=0.74
- Line 84: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hints.push_back(std::move(hint));
  Confidence: band=high; score=0.74
- Line 85: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: hints.push_back(std::move(hint));

### src/transaction/saga_plugin/saga_orchestrator_plugin.cpp
Total findings: 5

- Line 92: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: return new themis::transaction::SagaOrchestratorPlugin();
- Line 96: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: plugin = nullptr;
  Context: delete plugin;
- Line 52: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto timeout_ms = cfg["default_timeout_ms"].get<int64_t>();
  Confidence: band=high; score=0.74
- Line 58: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto retry_ms = cfg["default_retry_delay_ms"].get<int64_t>();
  Confidence: band=high; score=0.74
- Line 96: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete plugin;

### src/transaction/saga.cpp
Total findings: 3

- Line 100: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: history.push_back(status + " " + step.operation_name);
  Confidence: band=high; score=0.74
- Line 101: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: history.push_back(status + " " + step.operation_name);
- Line 109: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto first_step_time = steps_[0].executed_at;
  Confidence: band=high; score=0.74

### src/transaction/saga_plugin_bridge.cpp
Total findings: 2

- Line 27: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto* orchestrator = static_cast<SAGAOrchestrator*>(plugin->getInstance());
- Line 27: severity=HIGH; category=unsafe_singleton
  Description: Singleton access without thread-safety mechanism
  Remediation: Protect with std::lock_guard or use Meyer singleton pattern
  Context: auto* orchestrator = static_cast<SAGAOrchestrator*>(plugin->getInstance());

### include/transaction/examples/transaction_semantic_advisor_example.cpp
Total findings: 1

- Line 168: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "\nSee docs/issues/optimization_layers/IMPL-B5-transaction-semantics.md\n";

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
