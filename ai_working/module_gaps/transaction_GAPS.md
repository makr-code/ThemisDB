# transaction Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: transaction
- Generated: 2026-06-02 11:09:13
- Status: Critical Findings Present
- Total Findings: 176
- Actionable Findings (Critical + High): 45
- Affected Files: 18

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 13 |
| High | 32 |
| Medium | 131 |
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
| src/transaction/distributed_saga.cpp | 49 | 13 | 4 | 32 | 0 |
| src/transaction/transaction_manager.cpp | 29 | 0 | 3 | 26 | 0 |
| src/transaction/saga_orchestrator.cpp | 20 | 0 | 2 | 18 | 0 |
| src/transaction/merge_engine.cpp | 17 | 0 | 0 | 17 | 0 |
| src/transaction/distributed_transaction_manager.cpp | 13 | 0 | 9 | 4 | 0 |
| src/transaction/lock_manager.cpp | 12 | 0 | 3 | 9 | 0 |
| src/transaction/crash_recovery_manager.cpp | 6 | 0 | 0 | 6 | 0 |
| src/transaction/deadlock_predictor.cpp | 6 | 0 | 3 | 3 | 0 |
| src/transaction/transaction_batcher.cpp | 5 | 0 | 5 | 0 | 0 |
| src/transaction/branch_manager.cpp | 4 | 0 | 0 | 4 | 0 |
| src/transaction/snapshot_manager.cpp | 4 | 0 | 0 | 4 | 0 |
| src/transaction/global_transaction_manager.cpp | 3 | 0 | 3 | 0 | 0 |
| src/transaction/saga.cpp | 2 | 0 | 0 | 2 | 0 |
| src/transaction/saga_plugin/saga_orchestrator_plugin.cpp | 2 | 0 | 0 | 2 | 0 |
| src/transaction/transaction_auditor.cpp | 2 | 0 | 0 | 2 | 0 |
| src/transaction/transaction_semantic_advisor.cpp | 2 | 0 | 0 | 2 | 0 |
| include/transaction/examples/transaction_semantic_advisor_example.cpp | 0 | 0 | 0 | 0 | 0 |
| src/transaction/saga_plugin_bridge.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/transaction/distributed_saga.cpp
Total findings: 49

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
- Line 424: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto st = futures[i].get();
  Confidence: band=very_high; score=0.9
- Line 509: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: last_status = fut.get();
  Confidence: band=very_high; score=0.9
- Line 633: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: last_status = fut.get();
  Confidence: band=very_high; score=0.9
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
- Line 329: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(node);
  Confidence: band=high; score=0.74
- Line 346: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, DistributedSagaStep>&  step_map,
  Confidence: band=high; score=0.74
- Line 397: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: wave_records.push_back(rec);
  Confidence: band=high; score=0.74
- Line 424: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto st = futures[i].get();
  Confidence: band=high; score=0.74
- Line 552: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, DistributedSagaStep>& step_map,
  Confidence: band=high; score=0.74
- Line 704: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"')  escaped += "\\\"";
  Confidence: band=high; score=0.74
- Line 841: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: local_def.steps.push_back(remoteStepToLocal(remote_step));
  Confidence: band=high; score=0.74
- Line 905: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: recovered.push_back(sid);
  Confidence: band=high; score=0.74
- Line 924: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> phase_label;
  Confidence: band=medium; score=0.66

### src/transaction/transaction_manager.cpp
Total findings: 29

- Line 193: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto lock_it = held_locks_.find(key);
  Confidence: band=very_high; score=0.9
- Line 1545: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function commit without trace point
  Context: TransactionManager::Status TransactionManager::Transaction::commit() {
  Confidence: band=very_high; score=0.9
- Line 1573: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function commit without trace point
  Context: THEMIS_DEBUG("Transaction {} is read-only — skipping WAL write on commit (duration: {} ms)",
  Confidence: band=very_high; score=0.9
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

### src/transaction/saga_orchestrator.cpp
Total findings: 20

- Line 85: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (names.find(dep) == names.end()) {
  Confidence: band=very_high; score=0.9
- Line 340: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: SagaOrchestratorStatus SAGAOrchestrator::execute(const SAGADefinition& saga) {
  Confidence: band=very_high; score=0.9
- Line 29: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '\\': out += "\\\\"; break;
  Confidence: band=high; score=0.74
- Line 34: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: default: out.push_back(c); break;
  Confidence: band=high; score=0.74
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
- Line 300: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: status_rec.failure_reason += " | compensation failed for " + step.name + ": " + ex.what();
  Confidence: band=high; score=0.74
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
Total findings: 17

- Line 98: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resolutions.push_back(res.toJson());
  Confidence: band=high; score=0.74
- Line 118: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: opts.manual_resolutions.push_back(ConflictResolution::fromJson(res_json));
  Confidence: band=high; score=0.74
- Line 157: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conflicts_arr.push_back(conflict.toJson());
  Confidence: band=high; score=0.74
- Line 163: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: changes_arr.push_back(change.toJson());
  Confidence: band=high; score=0.74
- Line 185: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.conflicts.push_back(Conflict::fromJson(conflict_json));
  Confidence: band=high; score=0.74
- Line 191: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.changes_applied.push_back(analytics::DiffEngine::Change::fromJson(change_json));
  Confidence: band=high; score=0.74
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

### src/transaction/distributed_transaction_manager.cpp
Total findings: 13

- Line 197: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: batch_flush_thread_ = std::thread(&DistributedTransactionManager::batchFlushLoop, this);
  Confidence: band=very_high; score=0.9
- Line 297: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: all_voted_commit = fut.get();
  Confidence: band=very_high; score=0.9
- Line 527: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& item : pending) {
  Confidence: band=very_high; score=0.9
- Line 641: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& tid : timed_out) {
  Confidence: band=very_high; score=0.9
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
- Line 1196: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: if (!fut.get()) {
  Confidence: band=very_high; score=0.9
- Line 1219: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < n; ++i) {
  Confidence: band=very_high; score=0.9
- Line 522: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pending.push_back(PendingAbort{tid, txn.participants});
  Confidence: band=high; score=0.74
- Line 634: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: timed_out.push_back(tid);
  Confidence: band=high; score=0.74
- Line 892: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(submitTask([nid]() -> VoteResult {
  Confidence: band=high; score=0.74
- Line 1219: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: worker_threads_.emplace_back([this] {
  Confidence: band=high; score=0.74

### src/transaction/lock_manager.cpp
Total findings: 12

- Line 19: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Compatibility matrix for lock types
  Confidence: band=high; score=0.8
- Line 21: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Compatibility table (held × requested):
  Confidence: band=high; score=0.8
- Line 417: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Check compatibility with all current holders
  Confidence: band=high; score=0.8
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
- Line 401: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entry.holders.push_back({txn_id, type, std::chrono::system_clock::now()});
  Confidence: band=high; score=0.74
- Line 427: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entry.holders.push_back(
  Confidence: band=high; score=0.74
- Line 427: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entry.holders.push_back(
  Confidence: band=high; score=0.74
- Line 464: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: row_keys.push_back(k);
  Confidence: band=high; score=0.74
- Line 585: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(pl.start_key, pl.end_key);
  Confidence: band=high; score=0.74

### src/transaction/crash_recovery_manager.cpp
Total findings: 6

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

### src/transaction/deadlock_predictor.cpp
Total findings: 6

- Line 162: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (std::find(p.keys.begin(), p.keys.end(), k) == p.keys.end()) {
  Confidence: band=very_high; score=0.9
- Line 239: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (dx != dy) return dx < dy;
  Confidence: band=very_high; score=0.9
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

### src/transaction/transaction_batcher.cpp
Total findings: 5

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

### src/transaction/branch_manager.cpp
Total findings: 4

- Line 232: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: branches.push_back(branch.value());
  Confidence: band=high; score=0.74
- Line 244: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: } else if (sort_by == "timestamp") {
  Confidence: band=high; score=0.74
- Line 392: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.conflicts.push_back(conflict.key);
  Confidence: band=high; score=0.74
- Line 822: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(*entry);
  Confidence: band=high; score=0.74

### src/transaction/snapshot_manager.cpp
Total findings: 4

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

### src/transaction/global_transaction_manager.cpp
Total findings: 3

- Line 167: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function commit without trace point
  Context: GlobalTxnOutcome GlobalTransactionManager::commit(const std::string& txn_id) {
  Confidence: band=very_high; score=0.9
- Line 282: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function abort without trace point
  Context: bool GlobalTransactionManager::abort(const std::string& txn_id) {
  Confidence: band=very_high; score=0.9
- Line 396: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function abort without trace point
  Context: "decision – marking ABORT (manual resolution may be required)",
  Confidence: band=very_high; score=0.9

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

### src/transaction/saga_plugin/saga_orchestrator_plugin.cpp
Total findings: 2

- Line 61: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto timeout_ms = cfg["default_timeout_ms"].get<int64_t>();
  Confidence: band=high; score=0.74
- Line 67: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto retry_ms = cfg["default_retry_delay_ms"].get<int64_t>();
  Confidence: band=high; score=0.74

### src/transaction/transaction_auditor.cpp
Total findings: 2

- Line 65: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(rec);
  Confidence: band=high; score=0.74
- Line 126: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ops.push_back(std::move(j));
  Confidence: band=high; score=0.74

### src/transaction/transaction_semantic_advisor.cpp
Total findings: 2

- Line 82: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hints.push_back(std::move(hint));
  Confidence: band=high; score=0.74
- Line 82: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hints.push_back(std::move(hint));
  Confidence: band=high; score=0.74

### include/transaction/examples/transaction_semantic_advisor_example.cpp
Total findings: 0


### src/transaction/saga_plugin_bridge.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
