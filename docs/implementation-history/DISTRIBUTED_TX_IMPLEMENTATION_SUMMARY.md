# Distributed Transaction Coordinator Implementation Summary

## Overview

This document summarises the complete Two-Phase Commit (2PC) implementation for cross-shard
distributed transactions in ThemisDB. The implementation provides ACID guarantees across multiple
Raft-group shards with TrueTime integration for external consistency.

---

## Completed Work

### 1. WAL Improvements (✅ Complete)

**File:** `include/storage/wal_manager.h`

- Added `PREPARE_TX = 8` to `WALEntryType` — `CHECKPOINT` was previously overloaded as a PREPARE
  marker, leading to semantic confusion during recovery

**File:** `src/sharding/distributed_transaction.cpp`

- `recoverTransactions()` rewritten with a two-pass WAL scan: builds committed/aborted sets first,
  then safely aborts any `PREPARE_TX` entries with no matching decision (true in-doubt recovery)
- Recovery now logs an `ABORT_TX` entry for each resolved in-doubt transaction

---

### 2. Shard-Side Participant (`TwoPhaseCommitParticipant`) (✅ Complete)

**File:** `include/sharding/two_phase_commit_participant.h`  
**File:** `src/sharding/two_phase_commit_participant.cpp`

Implements `ShardRPCServer::RequestHandler`. Attach to any Raft-leader's RPC server:

```cpp
TwoPhaseCommitParticipant participant(
    "shard-1", cfg,
    [](auto& id, auto& ops)             { return acquireLocks(ops); },
    [](auto& id, auto& ops, int64_t ts) { return applyOps(ops, ts); },
    [](auto& id)                        { releaseLocks(id); }
);
rpc_server.setRequestHandler(&participant);
```

**Key properties:**
- **Idempotent** — duplicate PREPARE/COMMIT/ABORT returns stored result without re-locking
- **Durable** — WAL-flush (`PREPARE_TX` → `COMMIT_TX` / `ABORT_TX`) before each response
- **Crash recovery** — `recoverFromWAL()` rebuilds in-memory state; in-doubt txns stay
  `PREPARED` for coordinator re-resolution
- **Timeout** — `abortTimedOutTransactions()` auto-aborts stale `PREPARED` entries
- **Health** — `onHealthCheck()` returns real uptime computed from construction timestamp
- **Observability** — `getStatistics()` exposes counters, active-state counts, and uptime

---

### 3. Prometheus Metrics Integration (✅ Complete)

**File:** `src/sharding/distributed_transaction.cpp`

- `commit()`: records `record2PCPreparePhase` (with timing), `record2PCCommitPhase` (with timing),
  `record2PCTransaction`, and `record2PCAbort` via `ShardingMetricsRegistry` singleton (null-safe)
- `abort()`: records `record2PCAbort("explicit_abort")` and `record2PCTransaction(false)`

**File:** `src/sharding/two_phase_commit_participant.cpp`

- `onPrepare`, `onCommit`, `onAbort`: measure elapsed time with `steady_clock` and call
  `record2PCParticipantResponse(shard_id_, phase, latency_ms)`

**File:** `include/sharding/distributed_transaction.h`

- Added `coordinator_id` to `DistributedTransactionCoordinator::Config` for Prometheus label
  cardinality control

---

### 4. HTTP REST API (✅ Complete)

**File:** `include/server/distributed_txn_api_handler.h`  
**File:** `src/server/distributed_txn_api_handler.cpp`  
**File:** `include/server/http_server.h` (updated)  
**File:** `src/server/http_server.cpp` (updated)

Seven REST endpoints backed by `DistributedTransactionCoordinator`:

| Method | Endpoint | Description |
|--------|----------|-------------|
| POST | `/dtxn/begin` | Begin with shard list |
| POST | `/dtxn/operation` | Append operation to active txn |
| POST | `/dtxn/commit` | Run 2PC (PREPARE → COMMIT/ABORT) |
| POST | `/dtxn/abort` | Explicit abort |
| POST | `/dtxn/readonly` | Snapshot read, no 2PC overhead |
| GET  | `/dtxn/status/{id}` | Transaction state |
| GET  | `/dtxn/stats` | Coordinator statistics |

---

### 5. OpenAPI Specification (✅ Complete)

**File:** `docs/openapi.yaml`

- Added `DistributedTransaction` tag to global tags list
- Added 7 `/dtxn/*` path entries with full request/response schemas
- Added `components/responses/BadRequest` reusable response

---

### 6. Tests (✅ Complete)

**File:** `tests/test_two_phase_commit.cpp` (14 tests)
- Normal commit path, prepare failure, idempotency, prepare timeout, full coordinator-crash
  recovery, partial crash (prepare only), concurrent transactions

**File:** `tests/test_distributed_txn_api_handler.cpp` (19 tests)
- All 7 REST endpoints, input validation (missing fields, bad JSON, empty shard list),
  state transitions, full begin→operate→abort flow

**File:** `tests/test_multi_shard_transactions.cpp` (re-enabled)
- Was excluded from CMake; now included — covers multi-shard atomicity with `MockShard`

---

### 7. Documentation (✅ Complete)

**File:** `docs/DISTRIBUTED_TRANSACTIONS.md`
- Removed stale "limitations" items now resolved (in-doubt recovery, `PREPARE_TX`)
- Added ✅ entries for all completed features
- Added HTTP API reference table with correct `curl` examples (including host/port)

**File:** `docs/en/sharding/RAID_SHARD_REFERENCING_ARCHITECTURE.md`
- Replaced stub `RAID1Coordinator` pseudo-code with real `TwoPhaseCommitParticipant` +
  `DistributedTransactionCoordinator` API; added cross-reference to `DISTRIBUTED_TRANSACTIONS.md`

**File:** `examples/distributed_transaction_example.cpp`
- Added Example 7: shard-side participant setup with conflict detection, timestamp-ordered
  apply, and direct PREPARE→COMMIT protocol walkthrough
- Added `#include "sharding/two_phase_commit_participant.h"`

---

## Configuration

```cpp
// Coordinator
DistributedTransactionCoordinator::Config coord_cfg;
coord_cfg.prepare_timeout_ms    = 10000;
coord_cfg.commit_timeout_ms     = 10000;
coord_cfg.max_concurrent_txns   = 1000;
coord_cfg.enable_read_only_opt  = true;
coord_cfg.rpc_timeout_ms        = 5000;
coord_cfg.max_retries           = 3;
coord_cfg.max_commit_retries    = 5;
coord_cfg.retry_backoff_base_ms = 100;
coord_cfg.max_backoff_ms        = 5000;
coord_cfg.enable_recovery_log   = true;
coord_cfg.coordinator_id        = "primary";  // Prometheus label

// Participant (shard side)
TwoPhaseCommitParticipant::Config part_cfg;
part_cfg.prepare_timeout_ms = 10000;
part_cfg.sync_wal_writes    = true;
part_cfg.max_active_txns    = 1000;
```

---

## Testing Status

| Test File | Tests | Status |
|-----------|-------|--------|
| `test_two_phase_commit.cpp` | 14 | ✅ |
| `test_distributed_txn_api_handler.cpp` | 19 | ✅ |
| `test_multi_shard_transactions.cpp` | re-enabled | ✅ |
| `test_distributed_transactions.cpp` | existing | ✅ |
| `benchmarks/bench_distributed_coordinator.cpp` | perf | ready |

---

## Security

- ✅ No security vulnerabilities detected (CodeQL scan)
- ✅ Proper synchronization — all shared state protected by `std::mutex`
- ✅ Input validation on all public REST APIs
- ✅ Null-safe Prometheus calls (no crash if metrics registry not configured)
- ✅ WAL flush before returning votes (prevents vote loss on participant crash)

---

## Future Enhancements

- Three-Phase Commit (3PC) for non-blocking guarantee
- Coordinator replication and failover
- Optimistic concurrency control
- ~~Distributed deadlock detection~~ — **implemented** (cluster-wide WFG via `CrossShardTransactionCoordinator`; issue #5396)
- Saga pattern for long-running transactions

---

## Files Modified/Created

### New Files
1. `include/sharding/two_phase_commit_participant.h`
2. `src/sharding/two_phase_commit_participant.cpp`
3. `include/server/distributed_txn_api_handler.h`
4. `src/server/distributed_txn_api_handler.cpp`
5. `tests/test_two_phase_commit.cpp`
6. `tests/test_distributed_txn_api_handler.cpp`

### Modified Files
1. `include/storage/wal_manager.h` — added `PREPARE_TX = 8`
2. `include/sharding/distributed_transaction.h` — added `coordinator_id` to Config
3. `src/sharding/distributed_transaction.cpp` — metrics wiring, in-doubt recovery fix
4. `include/server/http_server.h` — added handler member
5. `src/server/http_server.cpp` — wired 7 `/dtxn/*` routes
6. `cmake/CMakeLists.txt` — added new source files
7. `tests/CMakeLists.txt` — re-enabled `test_multi_shard_transactions.cpp`
8. `docs/openapi.yaml` — 7 new endpoints + `DistributedTransaction` tag
9. `docs/DISTRIBUTED_TRANSACTIONS.md` — updated status sections
10. `docs/en/sharding/RAID_SHARD_REFERENCING_ARCHITECTURE.md` — replaced stub 2PC code
11. `examples/distributed_transaction_example.cpp` — added Example 7

---

**Status:** ✅ Complete and ready for review

