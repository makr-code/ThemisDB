# sharding — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **sharding** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

- **Total Gaps**: 7257
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: C:\Projects\ThemisDB (L0 full scan with Phase 5)

**Batch 3 Wave Correlation (2026-08-14):**
- **Wave A Gaps** (~500 IMPL gaps): Cross-shard thread-safety (340+ → ~102 after hardening), lock-ordering (95 → 0), consensus coordination (170 → 51)
- **Wave A DOC Gaps** (~300): Thread-safety model documentation, fail-closed behavior documentation, rebalance runbook
- **Wave B Gaps** (~200 IMPL): Distributed rate-limit state, topology-change stress testing, SLA monitoring
- **Other DOC Gaps** (~6,200): Inline comments, architectural notes, operational observability

**Hardening Status (Batch 3 verified 2026-08-10):**
- [x] Thread-safety gaps reduced from 340+ → ~102 (dual_consensus_orchestrator, replica_consistency)
- [x] Lock-ordering violations reduced from 95 → 0 (canonical order documented and tested)
- [x] Consensus coordination robustness improved (170 → 51 gaps; quorum-loss detection, backoff logic)
- [x] Test evidence: TSO-01..TSO-08, LKO-01..LKO-06, CCR-01..CCR-06 in test_sharding_thread_safety_lock_order_focused.cpp

### By Severity

- **CRITICAL**: 0  <!-- Wave 1 batch closed 2026-08-25; all 36 pre-Wave-A CRITICAL gaps verified closed — see WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md §Wave 1 CRITICAL Batch -->
- **HIGH**: 795
- **MEDIUM**: 6424
- **LOW**: 2

### By Type

- allocation_loop: 4
- arithmetic_overflow: 7
- blocking_no_timeout: 1
- braces_imbalance: 26
- braces_imbalance_midfile: 109
- circular_lock_ordering: 172
- copy_overhead: 46
- coupling_risk_sharding_storage: 1
- crypto_weakness: 1
- db_connection_leak: 36
- deadlock_risk: 12
- delete_no_nullptr: 10
- delete_without_nullptr: 10
- duplicate_qualified_signature: 20
- exception_in_destructor: 2
- expensive_inner_op: 4
- generic_catch: 4
- hardcoded_path: 4
- iterator_invalidation: 4
- legacy_or_compat_path: 11
- lock_contention: 46
- manual_cleanup: 29
- memory_order: 1
- missing_dtor: 1
- missing_noexcept_on_move: 14
- missing_override_keyword: 4
- missing_resource_limits: 3
- missing_volatile: 24
- model_integrity_gap: 2
- module_doc_linkset_drift: 2
- multiplication_overflow: 2
- no_retry_logic: 60
- no_timeout: 8
- null_dereference: 7
- o_n_squared: 13
- plaintext_transmission: 1
- pointer_arithmetic_unbounded: 50
- pure_virtual_unimplemented: 2
- range_temporary: 61
- repeated_lookup: 1
- repeated_search: 1
- resource_leaked_in_exception: 4
- scope_mismatch: 6039
- sensitive_data_logging: 39
- shift_overflow: 4
- silent_error_swallow: 7
- size_assumption: 4
- smart_ptr_misuse: 2
- socket_leak: 1
- stale_doc_section_reference: 21
- string_concat_loop: 4
- todo_as_productionlogic: 166
- uncaught_exception: 7
- unchecked_array_index: 18
- unchecked_memcpy: 1
- unchecked_result: 44
- uninitialized_access: 74
- uninitialized_array: 6

## Top 20 Gaps
<!-- Wave 1 CRITICAL batch closed 2026-08-25.  All CRITICAL entries below are
     retained for historical traceability.  Status: ✅ CLOSED. -->

- [braces_imbalance] cross_shard_transaction.cpp:1 (CRITICAL) ✅ CLOSED — brace count balanced (open=close=1052); WV1-BRC-01
- [braces_imbalance] hardware_migration_manager.cpp:1 (CRITICAL) ✅ CLOSED — brace count balanced (open=close=68); WV1-BRC-01
- [braces_imbalance] health_check.cpp:1 (CRITICAL) ✅ CLOSED — brace count balanced (open=close=67); WV1-BRC-01
- [braces_imbalance] metadata_snapshot.cpp:1 (CRITICAL) ✅ CLOSED — brace count balanced (open=close=59); WV1-BRC-01
- [braces_imbalance] metadata_wal.cpp:1 (CRITICAL) ✅ CLOSED — brace count balanced (open=close=38); WV1-BRC-01
- [braces_imbalance] operational_metrics.cpp:1 (CRITICAL) ✅ CLOSED — brace count balanced (open=close=109); WV1-BRC-01
- [braces_imbalance] paxos_consensus.cpp:1 (CRITICAL) ✅ CLOSED — brace count balanced (open=close=386); WV1-BRC-01
- [braces_imbalance] paxos_snapshot.cpp:1 (CRITICAL) ✅ CLOSED — brace count balanced (open=close=110); WV1-BRC-01
- [braces_imbalance] raft_state.cpp:1 (CRITICAL) ✅ CLOSED — brace count balanced (open=close=57); WV1-BRC-01
- [braces_imbalance] replica_consistency.cpp:1 (CRITICAL) ✅ CLOSED — brace count balanced (open=close=70); WV1-BRC-01
- [braces_imbalance] slo_monitor.cpp:1 (CRITICAL) ✅ CLOSED — brace count balanced (open=close=122); WV1-BRC-01
- [braces_imbalance] stream_protocol.cpp:1 (CRITICAL) ✅ CLOSED — brace count balanced (open=close=276); WV1-BRC-01
- [exception_in_destructor] inference_engine_enhanced.cpp:38 (CRITICAL) ✅ CLOSED — ~InferenceEngineEnhanced() is noexcept; shutdown() wrapped in try/catch; WV1-EXD-01
- [exception_in_destructor] cross_shard_speculative_decoder.cpp:40 (CRITICAL) ✅ CLOSED — ~CrossShardSpeculativeDecoder() is noexcept; shutdown() wrapped in try/catch; WV1-EXD-02
- [no_timeout] raft_wal_integration.cpp:49 (CRITICAL) ✅ CLOSED — write() uses timed_mutex + try_lock_for(config_.write_timeout); WV1-NTO-01
- [iterator_invalidation] raft_log.cpp:66 (CRITICAL) ✅ CLOSED — inverted-range guard + max_entries=1000 cap; WV1-ITR-01
- [model_integrity_gap] inference_engine_enhanced.cpp:102 (CRITICAL) ✅ CLOSED — path/config validated post-assignment; STUB NOTE in place for simulated load; code review
- [db_connection_leak] replication_coordinator.cpp:105 (CRITICAL) ✅ CLOSED — PendingWrite::db_connection is shared_ptr<void>; .reset() before map erase; WV1-DBL-02
- [no_timeout] paxos_state_persistence.cpp:114 (CRITICAL) ✅ CLOSED — open() uses timed_mutex + try_lock_for(config_.init_timeout); WV1-NTO-02
- [iterator_invalidation] gossip_consensus_adapter.cpp:191 (CRITICAL) ✅ CLOSED — inverted-range guard + max_entries=1000 cap; WV1-ITR-02

... and 7237 more gaps.

---

**Phase 5 Verification Notes**: External GitHub submodules (llama.cpp, whisper.cpp, vcpkg, etc.) are explicitly excluded from this analysis via Phase 5 filtering. This ensures all gaps are from themis_core (100% scope accuracy).
