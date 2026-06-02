# transaction Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: transaction
- Generated: 2026-06-02 11:55:48
- Status: Critical Findings Present
- Total Findings: 414
- Actionable Findings (Critical + High): 221
- Affected Files: 18

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 31 |
| High | 190 |
| Medium | 193 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 111 |
| container | 98 |
| raii | 39 |
| exception_safety | 30 |
| determinism | 29 |
| reliability | 25 |
| distributed_consistency | 20 |
| performance | 20 |
| concurrency | 15 |
| memory | 8 |
| legacy_duplication | 7 |
| observability | 7 |
| audit_logging | 5 |
| uninitialized | 2 |
| platform | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/transaction/distributed_saga.cpp | 71 | 13 | 10 | 48 | 0 |
| src/transaction/transaction_manager.cpp | 67 | 4 | 36 | 27 | 0 |
| src/transaction/distributed_transaction_manager.cpp | 54 | 4 | 37 | 13 | 0 |
| src/transaction/saga_orchestrator.cpp | 43 | 0 | 10 | 33 | 0 |
| src/transaction/merge_engine.cpp | 36 | 0 | 12 | 24 | 0 |
| src/transaction/lock_manager.cpp | 28 | 4 | 10 | 14 | 0 |
| src/transaction/deadlock_predictor.cpp | 23 | 1 | 19 | 3 | 0 |
| src/transaction/global_transaction_manager.cpp | 23 | 0 | 23 | 0 | 0 |
| src/transaction/transaction_batcher.cpp | 15 | 1 | 14 | 0 | 0 |
| src/transaction/crash_recovery_manager.cpp | 13 | 0 | 4 | 9 | 0 |
| src/transaction/branch_manager.cpp | 11 | 0 | 4 | 7 | 0 |
| src/transaction/snapshot_manager.cpp | 10 | 0 | 6 | 4 | 0 |
| src/transaction/saga_plugin/saga_orchestrator_plugin.cpp | 5 | 1 | 1 | 3 | 0 |
| src/transaction/transaction_auditor.cpp | 5 | 0 | 2 | 3 | 0 |
| src/transaction/transaction_semantic_advisor.cpp | 5 | 2 | 1 | 2 | 0 |
| src/transaction/saga.cpp | 2 | 0 | 0 | 2 | 0 |
| src/transaction/saga_plugin_bridge.cpp | 2 | 1 | 1 | 0 | 0 |
| include/transaction/examples/transaction_semantic_advisor_example.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### src/transaction/distributed_saga.cpp
Total findings: 71

- Line 51: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: if (!names.insert(step.name).second) {
  Confidence: band=very_high; score=0.99
- Line 125: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: journalWrite(saga.saga_id, "REJECTED_DUPLICATE", report.failure_reason);
  Confidence: band=very_high; score=0.99
- Line 242: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: listed.insert(rec.name);
  Confidence: band=very_high; score=0.99
- Line 248: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: journalWrite(saga.saga_id, "COMPENSATING", failure_reason);
  Confidence: band=very_high; score=0.99
- Line 268: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: journalWrite(saga.saga_id,
  Confidence: band=very_high; score=0.99
- Line 279: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: journalWrite(saga.saga_id, "COMPLETED");
  Confidence: band=very_high; score=0.99
- Line 333: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: ready.insert(succ);
  Confidence: band=very_high; score=0.99
- Line 679: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: // journalWrite()
  Confidence: band=very_high; score=0.99
- Line 682: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: void DistributedSagaCoordinator::journalWrite(
  Confidence: band=very_high; score=0.99
- Line 781: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: journalWrite(remote_saga.saga_id,
  Confidence: band=very_high; score=0.99
- Line 899: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // Only insert if not already present (a concurrent execute() may have
  Confidence: band=very_high; score=0.99
- Line 1030: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: journalWrite(saga_id, "FORCE_COMPENSATED");
  Confidence: band=very_high; score=0.99
- Line 1041: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: journalWrite(saga_id, "FORCE_COMPLETED");
  Confidence: band=very_high; score=0.99
- Line 63: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (names.find(dep) == names.end()) {
  Confidence: band=very_high; score=0.9
- Line 108: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: DistributedSagaReport DistributedSagaCoordinator::execute(
- Line 239: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: listed.find(rec.name) == listed.end()) {
- Line 395: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = index.find(name);
- Line 397: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: StepRecord* rec = (it != index.end()) ? it->second : nullptr;
- Line 424: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto st = futures[i].get();
  Confidence: band=very_high; score=0.9
- Line 509: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: last_status = fut.get();
  Confidence: band=very_high; score=0.9
- Line 559: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto rit = index.find(name);
- Line 633: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: last_status = fut.get();
  Confidence: band=very_high; score=0.9
- Line 845: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return execute(local_def);
- Line 46: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> names;
  Confidence: band=medium; score=0.66
- Line 72: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::string>> adj;
  Confidence: band=medium; score=0.66
- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adj[dep].push_back(step.name);
  Confidence: band=high; score=0.74
- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adj[dep].push_back(step.name);
  Confidence: band=high; score=0.74
- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adj[dep].push_back(step.name);
  Confidence: band=high; score=0.74
- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adj[dep].push_back(step.name);
  Confidence: band=high; score=0.74
- Line 76: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: adj[dep].push_back(step.name);
- Line 81: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, Color> color;
  Confidence: band=medium; score=0.66
- Line 166: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, DistributedSagaStep> step_map;
  Confidence: band=high; score=0.74
- Line 168: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, StepRecord*> record_index;
  Confidence: band=medium; score=0.66
- Line 173: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.step_records.push_back(rec);
  Confidence: band=high; score=0.74
- Line 194: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int> level;
  Confidence: band=medium; score=0.66
- Line 206: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& [name, lvl] : level) waves[lvl].push_back(name);
  Confidence: band=high; score=0.74
- Line 206: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& [name, lvl] : level) waves[lvl].push_back(name);
  Confidence: band=high; score=0.74
- Line 206: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& [name, lvl] : level) waves[lvl].push_back(name);
  Confidence: band=high; score=0.74
- Line 224: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: executed_order.push_back(name);
  Confidence: band=high; score=0.74
- Line 224: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: executed_order.push_back(name);
  Confidence: band=high; score=0.74
- Line 224: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: executed_order.push_back(name);
  Confidence: band=high; score=0.74
- Line 234: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> listed(
  Confidence: band=medium; score=0.66
- Line 240: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_compensate.push_back(rec.name);
  Confidence: band=high; score=0.74
- Line 241: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: to_compensate.push_back(rec.name);
- Line 307: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int>                  in_degree;
  Confidence: band=medium; score=0.66
- Line 308: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::string>> successors;
  Confidence: band=medium; score=0.66
- Line 312: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: successors[dep].push_back(step.name);
  Confidence: band=high; score=0.74
- Line 312: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: successors[dep].push_back(step.name);
  Confidence: band=high; score=0.74
- Line 313: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: successors[dep].push_back(step.name);
- Line 329: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(node);
  Confidence: band=high; score=0.74
- Line 346: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, DistributedSagaStep>&  step_map,
  Confidence: band=high; score=0.74
- Line 392: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::vector<std::future<DistributedSagaStatus>> futures;
- Line 397: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: wave_records.push_back(rec);
  Confidence: band=high; score=0.74
- Line 410: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(
- Line 421: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: DistributedSagaStatus wave_status;
- Line 424: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto st = futures[i].get();
  Confidence: band=high; score=0.74
- Line 434: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 465: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: DistributedSagaStatus last_status;
- Line 519: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 537: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: step.name, attempt + 1, last_status.message);
- Line 552: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, DistributedSagaStep>& step_map,
  Confidence: band=high; score=0.74
- Line 611: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: DistributedSagaStatus last_status;
- Line 643: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 650: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: step.name, attempt + 1, last_status.message);
- Line 704: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"')  escaped += "\\\"";
  Confidence: band=high; score=0.74
- Line 705: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '"')  escaped += "\\\"";
- Line 706: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\\') escaped += "\\\\";
- Line 841: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: local_def.steps.push_back(remoteStepToLocal(remote_step));
  Confidence: band=high; score=0.74
- Line 842: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: local_def.steps.push_back(remoteStepToLocal(remote_step));
- Line 905: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: recovered.push_back(sid);
  Confidence: band=high; score=0.74
- Line 924: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> phase_label;
  Confidence: band=medium; score=0.66

### src/transaction/transaction_manager.cpp
Total findings: 67

- Line 1535: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto conflicting_txn = lock_manager_->checkPredicateConflict(id_, key);
- Line 1628: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: conflict_set_id = conflict_mgr_->storeConflictSet(cset);
- Line 1628: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: conflict_set_id = conflict_mgr_->storeConflictSet(cset);
- Line 1997: severity=CRITICAL; category=data_race
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
- Line 159: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(deadlock_detector_mutex_);
- Line 193: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto lock_it = held_locks_.find(key);
  Confidence: band=very_high; score=0.9
- Line 218: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: auto cycle_start = std::find(path.begin(), path.end(), neighbor);
- Line 293: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: uint64_t prev = deadlock_max_cycle_len_.load(std::memory_order_relaxed);
- Line 313: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (deadlock_predictor_.load(std::memory_order_acquire)) {
- Line 357: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (DeadlockPredictor* dp = deadlock_predictor_.load(std::memory_order_acquire)) {
- Line 492: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (DeadlockPredictor* dp = deadlock_predictor_.load(std::memory_order_acquire)) {
- Line 536: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (DeadlockPredictor* dp = deadlock_predictor_.load(std::memory_order_acquire)) {
- Line 613: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: stats.total_begun = total_begun_.load(std::memory_order_relaxed);
- Line 614: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: stats.total_committed = total_committed_.load(std::memory_order_relaxed);
- Line 724: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto ac = active_counts.find(tid);
- Line 724: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto ac = active_counts.find(tid);
- Line 909: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (finished_.load(std::memory_order_acquire)) {
- Line 1516: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (finished_.load(std::memory_order_acquire)) {
- Line 1525: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: lock_manager_->acquirePredicateLock(id_, start_key, end_key);
- Line 1526: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: THEMIS_DEBUG("Transaction {} acquired predicate lock on [{}, {}]", id_, start_key, end_key);
- Line 1526: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_DEBUG("Transaction {} acquired predicate lock on [{}, {}]", id_, start_key, end_key);
- Line 1545: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function commit without trace point
  Context: TransactionManager::Status TransactionManager::Transaction::commit() {
  Confidence: band=very_high; score=0.9
- Line 1573: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function commit without trace point
  Context: THEMIS_DEBUG("Transaction {} is read-only — skipping WAL write on commit (duration: {} ms)",
  Confidence: band=very_high; score=0.9
- Line 1612: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto base_it = base_values_.find(key);
- Line 1676: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (finished_.load(std::memory_order_acquire)) {
- Line 1687: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (finished_.load(std::memory_order_acquire)) {
- Line 1700: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (finished_.load(std::memory_order_acquire)) {
- Line 1713: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (finished_.load(std::memory_order_acquire)) {
- Line 1739: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (finished_.load(std::memory_order_acquire)) {
- Line 1915: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: result.is_finished    = finished_.load(std::memory_order_acquire);
- Line 2028: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return deadlock_predictor_.load(std::memory_order_acquire);
- Line 2034: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: DeadlockPredictor* p = deadlock_predictor_.load(std::memory_order_acquire);
- Line 2053: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: DeadlockPredictor* p = deadlock_predictor_.load(std::memory_order_acquire);
- Line 2066: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: DeadlockPredictor* p = deadlock_predictor_.load(std::memory_order_acquire);
- Line 107: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(deadlock);
  Confidence: band=high; score=0.74
- Line 189: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<TransactionId, std::unordered_set<TransactionId>> wait_graph;
  Confidence: band=medium; score=0.66
- Line 210: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: path.push_back(node);
  Confidence: band=high; score=0.74
- Line 210: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: path.push_back(node);
  Confidence: band=high; score=0.74
- Line 314: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 317: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cycle_keys.push_back(key);
  Confidence: band=high; score=0.74
- Line 317: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cycle_keys.push_back(key);
  Confidence: band=high; score=0.74
- Line 324: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cycle_keys.push_back(wkey);
  Confidence: band=high; score=0.74
- Line 496: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (info.holder == id) keys.push_back(key);
  Confidence: band=high; score=0.74
- Line 540: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (info.holder == id) keys.push_back(key);
  Confidence: band=high; score=0.74
- Line 710: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, uint64_t> active_counts;
  Confidence: band=medium; score=0.66
- Line 726: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 742: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(txn_id);
  Confidence: band=high; score=0.74
- Line 755: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_abort.push_back(txn_id);
  Confidence: band=high; score=0.74
- Line 814: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: expired.push_back(id);
  Confidence: band=high; score=0.74
- Line 1292: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: k += "occ:ver:";
  Confidence: band=high; score=0.74
- Line 1292: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: k += "occ:ver:";
  Confidence: band=high; score=0.74
- Line 1606: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conflict_keys.push_back(key);
  Confidence: band=high; score=0.74
- Line 1798: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(e.name);
  Confidence: band=high; score=0.74
- Line 1839: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: expired.push_back(id);
  Confidence: band=high; score=0.74
- Line 1919: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.locks_held.push_back({key, lockTypeName(lock_type)});
  Confidence: band=high; score=0.74
- Line 1920: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.locks_held.push_back({key, lockTypeName(lock_type)});
- Line 2013: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(toTimeTravelRecord(rec));
  Confidence: band=high; score=0.74
- Line 2143: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: others.emplace_back(id, lock_manager_.getPredicateLockRanges(id));
  Confidence: band=high; score=0.74
- Line 2168: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(sc));
  Confidence: band=high; score=0.74
- Line 2168: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(sc));
  Confidence: band=high; score=0.74
- Line 2168: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(sc));
  Confidence: band=high; score=0.74

### src/transaction/distributed_transaction_manager.cpp
Total findings: 54

- Line 303: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 366: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 422: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 1225: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: pool_cv_.wait(lock, [this] {
- Line 187: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("DistributedTransactionManager [{}] WAL initialised at {}",
- Line 197: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: batch_flush_thread_ = std::thread(&DistributedTransactionManager::batchFlushLoop, this);
  Confidence: band=very_high; score=0.9
- Line 271: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::unique_lock<std::mutex> lock(mutex_);
- Line 297: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: all_voted_commit = fut.get();
  Confidence: band=very_high; score=0.9
- Line 304: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: txn = findTransaction(txn_id);  // re-acquire pointer after re-lock
- Line 505: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_DEBUG("DistributedTransactionManager [{}] recoverInDoubtTransactions: WAL disabled; "
- Line 527: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& item : pending) {
  Confidence: band=very_high; score=0.9
- Line 529: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 540: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("DistributedTransactionManager [{}] starting in-doubt recovery", coordinator_id_);
- Line 600: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("DistributedTransactionManager [{}] recovery: broadcasting ABORT for "
- Line 615: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("DistributedTransactionManager [{}] recovery complete: {} in-doubt txns resolved",
- Line 641: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& tid : timed_out) {
  Confidence: band=very_high; score=0.9
- Line 707: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("DistributedTransactionManager [{}] liveness_check_fn threw for "
- Line 712: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("DistributedTransactionManager [{}] liveness_check_fn threw for "
- Line 717: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("DistributedTransactionManager [{}] liveness_check_fn threw for "
- Line 729: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("DistributedTransactionManager [{}] static liveness check threw for "
- Line 734: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("DistributedTransactionManager [{}] static liveness check threw for "
- Line 739: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("DistributedTransactionManager [{}] static liveness check threw for "
- Line 845: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return (it != transactions_.end()) ? &it->second : nullptr;
- Line 851: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return (it != transactions_.end()) ? &it->second : nullptr;
- Line 890: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("DistributedTransactionManager [{}] cannot send Phase-1 PREPARE for "
- Line 973: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backwards-compatibility path: a Phase-2 bridge is configured but
  Confidence: band=high; score=0.8
- Line 992: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function abort without trace point
  Context: "(remote) — voting ABORT (no RPC bridge configured)",
  Confidence: band=very_high; score=0.9
- Line 1036: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: const VoteResult result = fut.get();
  Confidence: band=very_high; score=0.9
- Line 1091: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // bridges (new Phase2RpcFn, legacy static RpcPhase2Fn, or
- Line 1100: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("DistributedTransactionManager [{}] cannot deliver Phase-2 {} for "
- Line 1144: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("DistributedTransactionManager [{}] cannot deliver Phase-2 {} for remote "
- Line 1196: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (!fut.get()) {
  Confidence: band=very_high; score=0.9
- Line 1219: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < n; ++i) {
  Confidence: band=very_high; score=0.9
- Line 1224: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(pool_mutex_);
- Line 1224: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(pool_mutex_);
- Line 1237: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_DEBUG("DistributedTransactionManager [{}] thread pool started: {} workers",
- Line 1261: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(batch_mutex_);
- Line 1272: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_DEBUG("DistributedTransactionManager [{}] batch-flush: {} transactions",
- Line 1285: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("DistributedTransactionManager [{}] batch Phase-1 failed for txn {}: {}",
- Line 1289: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("DistributedTransactionManager [{}] batch Phase-1 failed for txn {}: {}",
- Line 1293: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("DistributedTransactionManager [{}] batch Phase-1 failed for txn {}: {}",
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 522: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pending.push_back(PendingAbort{tid, txn.participants});
  Confidence: band=high; score=0.74
- Line 523: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pending.push_back(PendingAbort{tid, txn.participants});
- Line 634: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: timed_out.push_back(tid);
  Confidence: band=high; score=0.74
- Line 892: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(submitTask([nid]() -> VoteResult {
  Confidence: band=high; score=0.74
- Line 903: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(submitTask([nid]() -> VoteResult {
- Line 913: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(submitTask([rpc_fn, ep, nid, tid, cid, keys]() -> VoteResult {
- Line 932: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(submitTask([dispatch, ep, nid, tid, cid, keys]() -> VoteResult {
- Line 950: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(submitTask([legacy_p1_fn, ep, nid, tid, cid, keys]() -> VoteResult {
- Line 1109: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(submitTask([rpc_fn, ep, nid, tid, cid, dc]() {
- Line 1156: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(submitTask([cb, nid, tid, cid]() {
- Line 1167: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(submitTask([cb, nid, tid, cid]() {
- Line 1219: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: worker_threads_.emplace_back([this] {
  Confidence: band=high; score=0.74

### src/transaction/saga_orchestrator.cpp
Total findings: 43

- Line 85: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (names.find(dep) == names.end()) {
  Confidence: band=very_high; score=0.9
- Line 139: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: const auto it = dependents.find(step.name);
- Line 139: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: const auto it = dependents.find(step.name);
- Line 231: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(metrics_mutex_);
- Line 275: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto step_it = step_map.find(*it);
- Line 340: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: SagaOrchestratorStatus SAGAOrchestrator::execute(const SAGADefinition& saga) {
- Line 340: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: SagaOrchestratorStatus SAGAOrchestrator::execute(const SAGADefinition& saga) {
  Confidence: band=very_high; score=0.9
- Line 345: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: const auto seq = next_id.fetch_add(1, std::memory_order_relaxed);
- Line 439: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = step_map.find(name);
- Line 439: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = step_map.find(name);
- Line 29: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '\\': out += "\\\\"; break;
  Confidence: band=high; score=0.74
- Line 30: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': out += "\\\\"; break;
- Line 31: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"': out += "\\\""; break;
- Line 32: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\n': out += "\\n"; break;
- Line 33: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\r': out += "\\r"; break;
- Line 34: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: default: out.push_back(c); break;
  Confidence: band=high; score=0.74
- Line 34: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\t': out += "\\t"; break;
- Line 108: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> context_overrides) const {
  Confidence: band=high; score=0.74
- Line 129: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::string>> dependents;
  Confidence: band=medium; score=0.66
- Line 134: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dependents[dep].push_back(step.name);
  Confidence: band=high; score=0.74
- Line 134: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dependents[dep].push_back(step.name);
  Confidence: band=high; score=0.74
- Line 134: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dependents[dep].push_back(step.name);
  Confidence: band=high; score=0.74
- Line 134: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dependents[dep].push_back(step.name);
  Confidence: band=high; score=0.74
- Line 161: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> indegree;
  Confidence: band=medium; score=0.66
- Line 162: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::string>> outgoing;
  Confidence: band=medium; score=0.66
- Line 258: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 300: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: status_rec.failure_reason += " | compensation failed for " + step.name + ": " + ex.what();
  Confidence: band=high; score=0.74
- Line 310: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 359: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: SAGAExecutionStatus status_rec;
- Line 376: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ++status_rec.completed_steps;
- Line 379: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ++status_rec.failed_steps;
- Line 382: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ++status_rec.skipped_steps;
- Line 387: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ++status_rec.pending_steps;
- Line 399: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> remaining_dependencies;
  Confidence: band=medium; score=0.66
- Line 400: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::string>> dependents;
  Confidence: band=medium; score=0.66
- Line 446: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(
  Confidence: band=high; score=0.74
- Line 455: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(future.get());
  Confidence: band=high; score=0.74
- Line 458: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({std::string{}, StepState::FAILED});
- Line 462: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 463: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({std::string{}, StepState::FAILED});
- Line 487: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: executed.push_back(name);
  Confidence: band=high; score=0.74
- Line 512: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ready.push_back(dependent);
  Confidence: band=high; score=0.74
- Line 512: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ready.push_back(dependent);
  Confidence: band=high; score=0.74

### src/transaction/merge_engine.cpp
Total findings: 36

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
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3416 [transaction] Implement Mer... (2026-03-12) | #1084 Implement Three-Way
- Line 325: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (conflict_keys.find(change.key) == conflict_keys.end()) {
- Line 325: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (conflict_keys.find(change.key) == conflict_keys.end()) {
- Line 525: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto res_it = resolution_map.find(conflict.key);
- Line 600: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: event.metadata["merge"] = true;
- Line 601: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: event.metadata["original_sequence"] = change.sequence;
- Line 98: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resolutions.push_back(res.toJson());
  Confidence: band=high; score=0.74
- Line 99: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: resolutions.push_back(res.toJson());
- Line 118: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: opts.manual_resolutions.push_back(ConflictResolution::fromJson(res_json));
  Confidence: band=high; score=0.74
- Line 119: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: opts.manual_resolutions.push_back(ConflictResolution::fromJson(res_json));
- Line 157: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conflicts_arr.push_back(conflict.toJson());
  Confidence: band=high; score=0.74
- Line 158: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: conflicts_arr.push_back(conflict.toJson());
- Line 163: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: changes_arr.push_back(change.toJson());
  Confidence: band=high; score=0.74
- Line 164: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: changes_arr.push_back(change.toJson());
- Line 185: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.conflicts.push_back(Conflict::fromJson(conflict_json));
  Confidence: band=high; score=0.74
- Line 186: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.conflicts.push_back(Conflict::fromJson(conflict_json));
- Line 191: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.changes_applied.push_back(analytics::DiffEngine::Change::fromJson(change_json));
  Confidence: band=high; score=0.74
- Line 192: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.changes_applied.push_back(analytics::DiffEngine::Change::fromJson(change_json));
- Line 268: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.changes_applied.push_back(change);
  Confidence: band=high; score=0.74
- Line 271: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.changes_applied.push_back(change);
  Confidence: band=high; score=0.74
- Line 274: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.changes_applied.push_back(change);
  Confidence: band=high; score=0.74
- Line 319: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> conflict_keys;
  Confidence: band=medium; score=0.66
- Line 326: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.changes_applied.push_back(change);
  Confidence: band=high; score=0.74
- Line 326: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.changes_applied.push_back(change);
  Confidence: band=high; score=0.74
- Line 459: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, const analytics::DiffEngine::Change*> source_map;
  Confidence: band=medium; score=0.66
- Line 460: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, const analytics::DiffEngine::Change*> target_map;
  Confidence: band=medium; score=0.66
- Line 519: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, const ConflictResolution*> resolution_map;
  Confidence: band=medium; score=0.66
- Line 535: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resolved_changes.push_back(change);
  Confidence: band=high; score=0.74
- Line 535: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resolved_changes.push_back(change);
  Confidence: band=high; score=0.74
- Line 544: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: resolved_changes.push_back(*auto_resolved);

### src/transaction/lock_manager.cpp
Total findings: 28

- Line 142: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator lt_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto lt_it = lock_table_.find(key);
- Line 167: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator txn_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto txn_it = held_by_txn_.find(txn_id);
- Line 183: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator lt_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto lt_it = lock_table_.find(key);
- Line 376: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = waiting_for_.find(txn_id);
- Line 19: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Compatibility matrix for lock types
  Confidence: band=high; score=0.8
- Line 21: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Compatibility table (held × requested):
  Confidence: band=high; score=0.8
- Line 52: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: LockManager::LockResult LockManager::acquireLock(
- Line 63: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: "2PL violation: transaction is in shrinking phase and cannot acquire new locks");
- Line 85: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: stats_acquired_.fetch_add(1, std::memory_order_relaxed);
- Line 182: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto lt_it = lock_table_.find(key);
- Line 417: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Check compatibility with all current holders
  Confidence: band=high; score=0.8
- Line 450: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: const size_t threshold = escalation_threshold_.load(std::memory_order_relaxed);
- Line 463: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (k.find(table_prefix) == 0 && k.size() > table_prefix.size()) {
- Line 508: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: bool LockManager::acquirePredicateLock(TransactionId txn_id,
- Line 178: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back(k);
  Confidence: band=high; score=0.74
- Line 301: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(k, lt);
  Confidence: band=high; score=0.74
- Line 365: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(req->txn_id);
  Confidence: band=high; score=0.74
- Line 365: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(req->txn_id);
  Confidence: band=high; score=0.74
- Line 366: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(req->txn_id);
- Line 378: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(it->second);
- Line 401: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entry.holders.push_back({txn_id, type, std::chrono::system_clock::now()});
  Confidence: band=high; score=0.74
- Line 402: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entry.holders.push_back({txn_id, type, std::chrono::system_clock::now()});
- Line 427: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entry.holders.push_back(
  Confidence: band=high; score=0.74
- Line 427: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entry.holders.push_back(
  Confidence: band=high; score=0.74
- Line 428: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entry.holders.push_back(
- Line 464: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: row_keys.push_back(k);
  Confidence: band=high; score=0.74
- Line 522: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: predicate_locks_.push_back({txn_id, start_key, end_key});
- Line 585: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(pl.start_key, pl.end_key);
  Confidence: band=high; score=0.74

### src/transaction/deadlock_predictor.cpp
Total findings: 23

- Line 257: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = hold_times_.find(key);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 37: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: const std::vector<std::string>& locks_acquired,
- Line 40: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (locks_acquired.empty()) {
- Line 50: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: static_cast<long>(locks_acquired.size())};
- Line 51: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: for (const auto& key : locks_acquired) {
- Line 63: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (p.keys == locks_acquired) {
- Line 75: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: pat.keys       = locks_acquired;
- Line 91: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: for (size_t i = 0; i < locks_acquired.size(); ++i) {
- Line 92: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: for (size_t j = i + 1; j < locks_acquired.size(); ++j) {
- Line 93: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: const std::string pk = makePairKey(locks_acquired[i], locks_acquired[j]);
- Line 162: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (std::find(p.keys.begin(), p.keys.end(), k) == p.keys.end()) {
  Confidence: band=very_high; score=0.9
- Line 162: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(p.keys.begin(), p.keys.end(), k) == p.keys.end()) {
- Line 162: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(p.keys.begin(), p.keys.end(), k) == p.keys.end()) {
- Line 239: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (dx != dy) return dx < dy;
  Confidence: band=very_high; score=0.9
- Line 256: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = hold_times_.find(key);
- Line 325: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = pair_conflicts_.find(pk);
- Line 325: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = pair_conflicts_.find(pk);
- Line 326: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = pair_conflicts_.find(pk);
  Confidence: band=very_high; score=0.9
- Line 52: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vec.push_back(per_key);
  Confidence: band=high; score=0.74
- Line 77: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: patterns_.push_back(std::move(pat));
  Confidence: band=high; score=0.74
- Line 217: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> danger;
  Confidence: band=medium; score=0.66

### src/transaction/global_transaction_manager.cpp
Total findings: 23

- Line 54: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("GlobalTransactionManager [{}] WAL initialised at {}",
- Line 72: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_DEBUG("GlobalTransactionManager [{}] registered region {}",
- Line 116: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: rec.region_ops[rid]     = nlohmann::json::array();
- Line 130: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_DEBUG("GlobalTransactionManager [{}] began txn {} across {} region(s)",
- Line 144: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("GlobalTransactionManager [{}] addOperation: unknown txn {}",
- Line 151: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("GlobalTransactionManager [{}] addOperation: txn {} not ACTIVE",
- Line 158: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("GlobalTransactionManager [{}] addOperation: region {} not in txn {}",
- Line 167: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function commit without trace point
  Context: GlobalTxnOutcome GlobalTransactionManager::commit(const std::string& txn_id) {
  Confidence: band=very_high; score=0.9
- Line 282: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function abort without trace point
  Context: bool GlobalTransactionManager::abort(const std::string& txn_id) {
  Confidence: band=very_high; score=0.9
- Line 287: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("GlobalTransactionManager [{}] abort: unknown txn {}",
- Line 315: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("GlobalTransactionManager [{}] txn {} explicitly ABORTED",
- Line 327: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("GlobalTransactionManager [{}] recovering from WAL…", coordinator_id_);
- Line 395: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("GlobalTransactionManager [{}] in-doubt txn {} has no "
- Line 396: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function abort without trace point
  Context: "decision – marking ABORT (manual resolution may be required)",
  Confidence: band=very_high; score=0.9
- Line 408: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("GlobalTransactionManager [{}] re-driving in-doubt txn {} "
- Line 431: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("GlobalTransactionManager [{}] WAL recovery failed: {}",
- Line 435: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("GlobalTransactionManager [{}] recovery complete – {} in-doubt txn(s) resolved",
- Line 487: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("GlobalTransactionManager [{}] Phase 1 – region {} not found for txn {}",
- Line 502: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("GlobalTransactionManager [{}] Phase 1 – region {} threw on PREPARE "
- Line 510: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("GlobalTransactionManager [{}] region {} voted ABORT for txn {}",
- Line 523: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("GlobalTransactionManager [{}] Phase 2 – region {} not found for txn {} "
- Line 537: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("GlobalTransactionManager [{}] Phase 2 – region {} threw on {} "
- Line 569: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("GlobalTransactionManager [{}] WAL write failed for txn {}: {}",

### src/transaction/transaction_batcher.cpp
Total findings: 15

- Line 222: severity=CRITICAL; category=no_timeout
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
- Line 162: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (stopping_.load(std::memory_order_acquire)) {
- Line 247: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> cfg_lk(config_mutex_);
- Line 269: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: !stopping_.load(std::memory_order_acquire)) {
- Line 272: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Notified: recompute in case new items have earlier deadlines.
- Line 304: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (stopping_.load(std::memory_order_acquire)) {
- Line 411: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: recent_throughputs_.push_back(throughput);
  Confidence: band=very_high; score=0.9
- Line 412: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: if (recent_throughputs_.size() > kMaxThroughputSamples)
  Confidence: band=very_high; score=0.9
- Line 413: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: recent_throughputs_.pop_front();
  Confidence: band=very_high; score=0.9
- Line 416: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: recent_throughputs_.begin(), recent_throughputs_.end(), 0.0)
  Confidence: band=very_high; score=0.9
- Line 417: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: / static_cast<double>(recent_throughputs_.size());
  Confidence: band=very_high; score=0.9

### src/transaction/crash_recovery_manager.cpp
Total findings: 13

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 58: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += '=';
  Confidence: band=high; score=0.74
- Line 301: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<uint64_t> in_flight = scanInFlight();
  Confidence: band=medium; score=0.66
- Line 316: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<uint64_t, std::vector<OperationEntry>> ops_by_txn;
  Confidence: band=medium; score=0.66
- Line 328: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: while (std::getline(f, line)) all_lines.push_back(line);
  Confidence: band=high; score=0.74
- Line 332: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<uint64_t, std::vector<OperationEntry>> tmp_ops;
  Confidence: band=medium; score=0.66
- Line 341: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tmp_ops[entry->txn_id].push_back(entry->operation);
  Confidence: band=high; score=0.74
- Line 342: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tmp_ops[entry->txn_id].push_back(entry->operation);
- Line 442: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: f.close();
- Line 480: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (e) result.push_back(*e);

### src/transaction/branch_manager.cpp
Total findings: 11

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 791: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: uint64_t counter = hist_counter.fetch_add(1, std::memory_order_relaxed);
- Line 232: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: branches.push_back(branch.value());
  Confidence: band=high; score=0.74
- Line 233: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: branches.push_back(branch.value());
- Line 244: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: } else if (sort_by == "timestamp") {
  Confidence: band=high; score=0.74
- Line 392: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.conflicts.push_back(conflict.key);
  Confidence: band=high; score=0.74
- Line 393: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.conflicts.push_back(conflict.key);
- Line 822: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(*entry);
  Confidence: band=high; score=0.74
- Line 823: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(*entry);

### src/transaction/snapshot_manager.cpp
Total findings: 10

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
  Context: * PR History (last 5): #461 Refactor PITR implementatio... (2026-03-11) | #386 feat: Implement Phase
- Line 196: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: snapshot = nullptr;
  Context: spdlog::error("Failed to delete snapshot: {}", tag_name);
- Line 151: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: snapshots.push_back(*snapshot);
  Confidence: band=high; score=0.74
- Line 157: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: if (sort_by == "timestamp") {
  Confidence: band=high; score=0.74
- Line 342: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (s.has_value()) snapshots.push_back(*s);
  Confidence: band=high; score=0.74
- Line 348: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort oldest-first by timestamp
  Confidence: band=high; score=0.74

### src/transaction/saga_plugin/saga_orchestrator_plugin.cpp
Total findings: 5

- Line 101: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: return new themis::transaction::SagaOrchestratorPlugin();
- Line 105: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: plugin = nullptr;
  Context: delete plugin;
- Line 61: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto timeout_ms = cfg["default_timeout_ms"].get<int64_t>();
  Confidence: band=high; score=0.74
- Line 67: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto retry_ms = cfg["default_retry_delay_ms"].get<int64_t>();
  Confidence: band=high; score=0.74
- Line 105: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete plugin;

### src/transaction/transaction_auditor.cpp
Total findings: 5

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 36: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!enabled_.load(std::memory_order_acquire)) return;
- Line 65: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(rec);
  Confidence: band=high; score=0.74
- Line 126: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ops.push_back(std::move(j));
  Confidence: band=high; score=0.74
- Line 148: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += '\n';

### src/transaction/transaction_semantic_advisor.cpp
Total findings: 5

- Line 138: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != b.entity_map.end() && it->second == id) {
- Line 166: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != b.entity_map.end() && it->second == id) {
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 82: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hints.push_back(std::move(hint));
  Confidence: band=high; score=0.74
- Line 82: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hints.push_back(std::move(hint));
  Confidence: band=high; score=0.74

### src/transaction/saga.cpp
Total findings: 2

- Line 98: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: history.push_back(status + " " + step.operation_name);
  Confidence: band=high; score=0.74
- Line 107: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto first_step_time = steps_[0].executed_at;
  Confidence: band=high; score=0.74

### src/transaction/saga_plugin_bridge.cpp
Total findings: 2

- Line 36: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto* orchestrator = static_cast<SAGAOrchestrator*>(plugin->getInstance());
- Line 36: severity=HIGH; category=unsafe_singleton
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
