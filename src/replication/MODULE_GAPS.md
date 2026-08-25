# replication — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **replication** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

- **Total Gaps**: 1519
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: C:\Projects\ThemisDB (L0 full scan with Phase 5)
- **Wave 1 CRITICAL Batch Fixed**: 2026-08-25 (all 16 CRITICAL gaps remediated)

### By Severity

- **CRITICAL**: 0  ← Wave 1 batch closed all 16
- **HIGH**: 194
- **MEDIUM**: 1307
- **LOW**: 2

### By Type

- allocation_loop: 1
- arithmetic_overflow: 1
- braces_imbalance: 4
- braces_imbalance_midfile: 9
- circular_lock_ordering: 96
- copy_overhead: 5
- db_connection_leak: 5
- duplicate_qualified_signature: 9
- hardcoded_path: 1
- iterator_invalidation: 2
- legacy_or_compat_path: 1
- lock_contention: 11
- manual_cleanup: 11
- missing_noexcept_on_move: 2
- missing_volatile: 14
- module_doc_linkset_drift: 2
- multiplication_overflow: 1
- no_timeout: 10
- null_dereference: 1
- o_n_squared: 8
- pointer_arithmetic_unbounded: 8
- range_temporary: 21
- repeated_lookup: 1
- resource_leaked_in_exception: 2
- scope_mismatch: 1262
- silent_error_swallow: 2
- string_concat_loop: 1
- todo_as_productionlogic: 20
- unchecked_array_index: 1
- unchecked_result: 6
- uninitialized_array: 1

## Top 20 Gaps

- [braces_imbalance] observability.cpp:1 (**CLOSED** Wave 1 — verified balanced 28/28 braces)
- [braces_imbalance] policy.cpp:1 (**CLOSED** Wave 1 — verified balanced 23/23 braces)
- [scope_mismatch] observability.cpp:34 (**CLOSED** Wave 1 — constructor parameter renamed config→observer_config)
- [multiplication_overflow] replication_manager.cpp:549 (**CLOSED** Wave 1 — __builtin_mul_overflow guard added)
- [no_timeout] replication_manager.cpp:558 (**CLOSED** Wave 1 — FILE_IO_TIMEOUT_MS driven by config_.file_io_timeout_ms)
- [no_timeout] logical_replication.cpp:647 (**CLOSED** Wave 1 — persistSlot I/O wrapped in lrm_executeWithTimeout)
- [no_timeout] replication_manager.cpp:654 (**CLOSED** Wave 1 — ::fsync wrapped in executeWithTimeout)
- [no_timeout] logical_replication.cpp:702 (**CLOSED** Wave 1 — persistSlot I/O wrapped in lrm_executeWithTimeout)
- [iterator_invalidation] replication_manager.cpp:2769 (**CLOSED** Wave 1 — doc.substr materialised as named variable)
- [no_timeout] replication_manager.cpp:3331 (**CLOSED** Wave 1 — verified O(1) non-blocking; comment added)
- [iterator_invalidation] replication_manager.cpp:4052 (**CLOSED** Wave 1 — const iterator scoped inside unique_lock)
- [no_timeout] replication_manager.cpp:4170 (**CLOSED** Wave 1 — 5ms driven by config_.idle_poll_interval_ms)
- [no_timeout] replication_manager.cpp:6024 (**CLOSED** Wave 1 — archival scan deadline via archival_scan_timeout_ms)
- [no_timeout] replication_manager.cpp:6059 (**CLOSED** Wave 1 — verified O(N) in-memory only; comment added)
- [no_timeout] replication_manager.cpp:6857 (**CLOSED** Wave 1 — verified O(N) in-memory only; comment added)
- [no_timeout] replication_manager.cpp:6895 (**CLOSED** Wave 1 — verified O(1) catch block; improved log message)
- [braces_imbalance] logical_replication.cpp:1 (HIGH)
- [braces_imbalance] replication_manager.cpp:1 (HIGH)
- [circular_lock_ordering] replication_slot.cpp:74 (HIGH)
- [circular_lock_ordering] replication_slot.cpp:84 (HIGH)

... and 1499 more gaps.

---

## Wave 1 CRITICAL Batch Fixed — 2026-08-25

| # | Gap Type | File | Original Line | Disposition | PR Deliverable |
|---|----------|------|--------------|-------------|----------------|
| 1 | braces_imbalance | observability.cpp | 1 | VERIFIED — 28 `{` / 28 `}` balanced; no code change needed | Brace count confirmed by automated check |
| 2 | braces_imbalance | policy.cpp | 1 | VERIFIED — 23 `{` / 23 `}` balanced; no code change needed | Brace count confirmed |
| 3 | scope_mismatch | observability.cpp | 34 | FIXED — constructor parameter renamed `config` → `observer_config` to eliminate `config`/`config_` same-scope ambiguity | `src/replication/observability.cpp` |
| 4 | multiplication_overflow | replication_manager.cpp | 549 | FIXED — `__builtin_mul_overflow` guard added before `std::vector<uint8_t>` allocation; cast widened to `uint64_t` for gcount comparison | `src/replication/replication_manager.cpp` |
| 5 | no_timeout | replication_manager.cpp | 558 | FIXED — hardcoded `5000` replaced by `config_.file_io_timeout_ms` (new field, default 5000 ms) | `include/replication/replication_manager.h` + `.cpp` |
| 6 | no_timeout | logical_replication.cpp | 647 | FIXED — entire `persistSlot` blocking I/O body wrapped in `lrm_executeWithTimeout(config_.file_io_timeout_ms, ...)` | `include/replication/logical_replication.h` + `.cpp` |
| 7 | no_timeout | replication_manager.cpp | 654 | FIXED — `::fsync` wrapped in `executeWithTimeout(config_.file_io_timeout_ms, ...)` | `src/replication/replication_manager.cpp` |
| 8 | no_timeout | logical_replication.cpp | 702 | FIXED — same `lrm_executeWithTimeout` wrapper covering close/rename/dir-fsync | `src/replication/logical_replication.cpp` |
| 9 | iterator_invalidation | replication_manager.cpp | 2769 | FIXED — `doc.substr(vp)` materialised as named `const std::string sub` before passing to `std::stoll` | `src/replication/replication_manager.cpp` |
| 10 | no_timeout | replication_manager.cpp | 3331 | VERIFIED — `generateWriteId` is O(1) non-blocking (atomic + vDSO clock); WAVE1-FIX comment added | `src/replication/replication_manager.cpp` |
| 11 | iterator_invalidation | replication_manager.cpp | 4052 | FIXED — iterator `it` declared `const`; scope comment added confirming it is destroyed before lock releases | `src/replication/replication_manager.cpp` |
| 12 | no_timeout | replication_manager.cpp | 4170 | FIXED — hardcoded `5ms` replaced by `std::chrono::milliseconds(config_.idle_poll_interval_ms)` (new field, default 5 ms) | `include/replication/replication_manager.h` + `.cpp` |
| 13 | no_timeout | replication_manager.cpp | 6024 | FIXED — `runArchivalCycle` directory iteration checks `steady_clock::now() >= scan_deadline`; returns 0 + logs error on breach | `include/replication/replication_manager.h` (`archival_scan_timeout_ms`) + `.cpp` |
| 14 | no_timeout | replication_manager.cpp | 6059 | VERIFIED — `MultiRegionActiveActiveManager` ctor loop is pure in-memory O(N); WAVE1-FIX comment added | `src/replication/replication_manager.cpp` |
| 15 | no_timeout | replication_manager.cpp | 6857 | VERIFIED — `GeoReplicationManager` ctor loop is pure in-memory O(N); WAVE1-FIX comment added | `src/replication/replication_manager.cpp` |
| 16 | no_timeout | replication_manager.cpp | 6895 | FIXED — improved log message; WAVE1-FIX comment confirms O(1) catch block | `src/replication/replication_manager.cpp` |

### New configurable fields introduced

| Struct | Field | Default | Purpose |
|--------|-------|---------|---------|
| `ReplicationConfig` | `file_io_timeout_ms` | 5000 | Max ms for WAL read/fsync blocking ops |
| `ParallelReplicationWorker::ParallelConfig` | `idle_poll_interval_ms` | 5 | Worker `wait_for` poll interval (replaces hardcoded 5 ms) |
| `WALArchivalManager::ArchivalConfig` | `archival_scan_timeout_ms` | 30000 | Max ms for full archival-cycle directory scan |
| `LogicalReplicationManager::Config` | `file_io_timeout_ms` | 5000 | Max ms for `persistSlot` blocking I/O |

### Test coverage added

`tests/replication/test_replication_wave1_critical_fixes.cpp` — 9 test cases covering all five gap classes.

---

**Phase 5 Verification Notes**: External GitHub submodules (llama.cpp, whisper.cpp, vcpkg, etc.) are explicitly excluded from this analysis via Phase 5 filtering. This ensures all gaps are from themis_core (100% scope accuracy).
