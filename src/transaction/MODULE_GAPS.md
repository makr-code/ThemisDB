# transaction — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **transaction** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

- **Total Gaps**: 1660  <!-- Reduced by 22 CRITICAL (Wave A 2026-08-25) -->
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: C:\Projects\ThemisDB (L0 full scan with Phase 5)

### By Severity

- **CRITICAL**: 0  <!-- Wave A remediation 2026-08-25: all 22 CRITICAL gaps closed; see WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md -->
- **HIGH**: 181
- **MEDIUM**: 1476
- **LOW**: 3

### By Type

- allocation_loop: 2
- blocking_no_timeout: 5
- braces_imbalance: 3
- circular_lock_ordering: 45
- copy_overhead: 7
- db_connection_leak: 39
- deadlock_risk: 1
- delete_no_nullptr: 2
- delete_without_nullptr: 2
- generic_catch: 3
- iterator_invalidation: 5
- legacy_or_compat_path: 7
- lock_contention: 6
- manual_cleanup: 2
- memory_order: 6
- missing_noexcept_on_move: 2
- missing_volatile: 3
- module_doc_linkset_drift: 4
- new_without_raii: 1
- no_timeout: 5
- o_n_squared: 14
- pointer_arithmetic_unbounded: 2
- repeated_search: 2
- resource_leaked_in_exception: 5
- scope_mismatch: 1413
- smart_ptr_misuse: 1
- string_concat_loop: 8
- todo_as_productionlogic: 34
- uncaught_exception: 3
- unchecked_result: 7
- uninitialized_access: 41
- uninitialized_array: 1
- unsafe_singleton: 1

## Top 20 Gaps

> **Wave A Remediation (2026-08-25):** All 22 CRITICAL gaps below have been closed.
> See `WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md` for full disposition.

- [braces_imbalance] distributed_transaction_manager.cpp:1 (**CLOSED** — braces balanced, scanner false-positive)
- [braces_imbalance] global_transaction_manager.cpp:1 (**CLOSED** — braces balanced, scanner false-positive)
- [new_without_raii] saga_orchestrator_plugin.cpp:112 (**CLOSED** — already uses std::make_unique + std::move)
- [smart_ptr_misuse] saga_orchestrator_plugin.cpp:112 (**CLOSED** — same location; safe ownership transfer)
- [iterator_invalidation] lock_manager.cpp:153 (**CLOSED** — erase-remove idiom; no invalidation)
- [iterator_invalidation] lock_manager.cpp:178 (**CLOSED** — keys collected before map modification)
- [iterator_invalidation] lock_manager.cpp:194 (**CLOSED** — same function; safe pattern)
- [blocking_no_timeout] transaction_batcher.cpp:233 (**CLOSED** — wait_for(30s) already at line 227)
- [no_timeout] transaction_batcher.cpp:233 (**CLOSED** — same location)
- [iterator_invalidation] deadlock_predictor.cpp:268 (**CLOSED** — const read-only function, no mutation)
- [blocking_no_timeout] distributed_transaction_manager.cpp:314 (**CLOSED** — line 314 is a closing brace; scanner false-positive)
- [no_timeout] distributed_transaction_manager.cpp:314 (**CLOSED** — same location)
- [db_connection_leak] lock_manager.cpp:350 (**CLOSED** — no raw DB connection; scanner false-positive on atomic store)
- [blocking_no_timeout] distributed_transaction_manager.cpp:377 (**CLOSED** — actual blocking call (fut.get) was at line 372 and **fixed this session**: replaced with `fut.wait_until(batch_deadline)` using `config_.prepare_timeout`)
- [no_timeout] distributed_transaction_manager.cpp:377 (**CLOSED** — same fix)
- [iterator_invalidation] lock_manager.cpp:387 (**CLOSED** — `getWaiters` is const read-only; no mutation)
- [blocking_no_timeout] distributed_transaction_manager.cpp:433 (**CLOSED** — line 433 empty; `runPhase2Unlocked` already uses per-participant deadline)
- [no_timeout] distributed_transaction_manager.cpp:433 (**CLOSED** — same location)
- [db_connection_leak] transaction_manager.cpp:615 (**CLOSED** — inside seqlock read loop; no DB connection acquired)
- [db_connection_leak] transaction_manager.cpp:651 (**CLOSED** — `stats_sequence_.load()`; no DB connection)

... and 1638 more gaps (HIGH/MEDIUM/LOW; see roadmap for prioritisation).

---

**Phase 5 Verification Notes**: External GitHub submodules (llama.cpp, whisper.cpp, vcpkg, etc.) are explicitly excluded from this analysis via Phase 5 filtering. This ensures all gaps are from themis_core (100% scope accuracy).
