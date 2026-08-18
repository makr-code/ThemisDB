# Batch 1 Implementation Checklist - Sharding Module Critical Gaps

**Date**: 2026-08-17
**Target**: Close all 20 CRITICAL gaps across 14 files
**Status**: Implementation in progress

## BATCH A: Namespace Consistency + Connection Leak (13 files)

### Namespace Fixes (12 files)
- [ ] stream_protocol.cpp - Convert `namespace streaming` to `namespace sharding`
- [ ] hardware_migration_manager.cpp - Convert inline to nested namespace
- [ ] slo_monitor.cpp - Convert inline to nested namespace  
- [ ] cross_shard_transaction.cpp - Standardize namespace pattern
- [ ] health_check.cpp - Standardize namespace pattern
- [ ] metadata_snapshot.cpp - Standardize namespace pattern
- [ ] metadata_wal.cpp - Standardize namespace pattern
- [ ] operational_metrics.cpp - Standardize namespace pattern
- [ ] paxos_consensus.cpp - Standardize namespace pattern
- [ ] paxos_snapshot.cpp - Standardize namespace pattern
- [ ] raft_state.cpp - Standardize namespace pattern
- [ ] replica_consistency.cpp - Standardize namespace pattern

### Resource Management
- [ ] replication_coordinator.cpp:105 - Add DB connection close before erase()
  - Add try-catch for exception safety
  - Verify connection pool cleanup

## BATCH B: Exception Safety (2 files)

- [ ] inference_engine_enhanced.cpp:38 - Add noexcept to destructor, wrap shutdown() in try-catch
- [ ] cross_shard_speculative_decoder.cpp:40 - Add noexcept to destructor, wrap shutdown() in try-catch

## BATCH C: Timeout Enforcement (2 files)

- [ ] raft_wal_integration.cpp:49 - Convert to std::timed_mutex with try_lock_for()
- [ ] paxos_state_persistence.cpp:114 - Convert lock_guard to unique_lock with try_lock_for()

## BATCH D: Iterator Validation (2 files)

- [ ] raft_log.cpp:66 - Add bounds checking and lock protection for iteration
- [ ] gossip_consensus_adapter.cpp:191 - Add validation and protection for iterator access

## VERIFICATION STEPS

### Build Verification
- [ ] cmake --preset community-release-allow-missing-rocksdb succeeds
- [ ] No compilation errors after each batch
- [ ] No new warnings introduced

### Test Verification
- [ ] ctest -L release_critical passes
- [ ] test_sharding_thread_safety_lock_order_focused passes all 20 cases (TSO-01..08, LKO-01..06, CCR-01..06)
- [ ] test_sharding_core passes
- [ ] test_sharding_phase6_hardening passes

### Code Quality Verification
- [ ] No undefined behavior introduced
- [ ] Exception safety maintained (all resource cleanups on exception paths)
- [ ] Lock hierarchies consistent (no circular lock ordering)
- [ ] Iterator validity protected with bounds checks

## KNOWN ISSUES TO AVOID

1. Don't use `std::lock_guard` with `std::timed_mutex` (it ignores timeout)
   - Must use `std::unique_lock` with `try_lock_for()`
2. Don't let destructors throw (add noexcept and wrap in try-catch)
3. Don't erase map entries without cleaning up contained resources first
4. Don't iterate without bounds checking in concurrent context

## IMPLEMENTATION NOTES

- All fixes preserve existing functionality
- Timeout values derived from config_ members (write_timeout, init_timeout)
- Namespace consolidation should follow nested pattern: `namespace themisdb { namespace sharding { ... } }`
- All FIXED comments should follow pattern: `// FIXED: <issue type> <date>`

## FOLLOW-UP WORK (Batch E)

After Batch 1 verification:
- [ ] Model integrity gaps (2 files)
  - inference_engine_enhanced.cpp:102
  - cross_shard_speculative_decoder.cpp:115
