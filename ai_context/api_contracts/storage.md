---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-11-01"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "api-contract"
Canonical_References:
  - "include/storage/storage_api_contract.h"
  - "include/storage/storage_engine.h"
  - "include/themis/base/interfaces/storage_interface.h"
  - "include/storage/wal_storage.h"
  - "include/storage/mvcc_store.h"
  - "src/storage/storage_engine.cpp"
  - "src/storage/wal_storage.cpp"
  - "src/storage/mvcc_store.cpp"
  - "tests/storage/test_storage_contract_hardening_focused.cpp"
  - "tests/storage/test_storage_engine_prod.cpp"
  - "tests/test_mvcc_wal_integration.cpp"
  - "tests/storage/CMakeLists.txt"
  - "benchmarks/storage/bench_storage_release_gates.cpp"
  - "benchmarks/storage/CMakeLists.txt"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

> Exact model backend not verifiable from final document text alone.

# Storage API contract and conformance evidence

## Scope and authority

[`include/storage/storage_api_contract.h`](../../include/storage/storage_api_contract.h)
declares a normative, binding v1.x contract for WAL durability, MVCC,
recovery, backup/PITR, tiering, and storage error codes. Its wording expresses
the intended contract; declaration, constants, and contract-focused tests do
not by themselves show that all storage components enforce it end to end.

Use the public API and matching implementation for concrete runtime behavior.
Where this document identifies a gap, retain the header as the contract
declaration and treat conformance as unverified until implementation-linked
tests and execution evidence close the gap. This document does not modify or
weaken the C++ contract.

## Current public surfaces

- [`StorageEngine`](../../include/storage/storage_engine.h) implements
  `IStorageEngine` in namespace `themis`. Its declared operations include
  `open`, `close`, `put`, `get`, `del`, and callback-based `scanRange`,
  `scanPrefix`, and `scanPredicate`. The CRUD methods return `Result` values;
  `get` returns `Result<std::string>`, not `std::optional<std::string>`.
- [`WALStorage`](../../include/storage/wal_storage.h) is a separate append,
  replay, checkpoint, and flush API. `Config::fsync_on_write` defaults to
  `true` but is configurable. Its existence does not establish that every
  `StorageEngine` write passes through this WAL.
- [`MVCCStore`](../../include/storage/mvcc_store.h) is a separate
  HLC-versioned store backed by a supplied, already-open `RocksDBWrapper`. It
  exposes version writes and timestamp-relative reads; this surface is not
  itself a general transaction manager.
- `StorageErrorCode` and the inline classification helpers are declared in
  `themis::storage` by `storage_api_contract.h`. They are not the `Result`
  error codes returned by `StorageEngine`'s `IStorageEngine` methods.

The former API table included operations, options, and limits not declared by
the current `IStorageEngine` public interface; those unsupported entries have
been removed from this API summary.

## Declaration versus demonstrated behavior

| Contract area | Normative declaration | Current test/implementation evidence | Conformance status |
|---|---|---|---|
| WAL durability and sequencing | Write-before-ack, idempotent replay, monotonic sequence numbers, checkpoint ordering, and fixed WAL limits/timeouts. | `WALStorage` has a real file-backed append/replay/checkpoint API. STR-01–04 and STR-09–11 in the contract-hardening test use `MockWal` and `MockKvStore`; they do not call `WALStorage` or exercise disk I/O. `tests/test_mvcc_wal_integration.cpp` configures RocksDB's internal WAL through `RocksDBWrapper`; it does not call `WALStorage` or test ACK ordering. | Not demonstrated end to end. Test the actual WAL and engine write path together, including restart and failure cases. |
| MVCC isolation | Read-your-writes, snapshot isolation, dirty-read prevention, and transaction/version limits. | The contract-hardening test's STR-05–08 use a minimal `MockMvccTx`. The source for `tests/test_mvcc_wal_integration.cpp` uses RocksDB-backed `MVCCStore` and includes timestamp reads, version scans, GC, and close/reopen persistence cases. Those cases do not establish transaction-level read-your-writes, dirty-read prevention, or conflict semantics. | Some concrete MVCC read/version/GC behaviors have implementation-backed test source; full declared transaction semantics remain unverified. No test execution or pass is claimed here. |
| Recovery | Checkpoint-based replay, torn-tail handling, and recovery limits/diagnostics. | The contract-hardening replay helper operates on mock entries. The MVCC integration test source includes normal database close/reopen persistence using RocksDB, not a crash/torn-tail test of `WALStorage`. `WALStorage::open()` provides a recovery callback, but that is not evidence the full declaration is met after a real process crash. | Normal MVCC persistence has test source; standalone WAL crash/recovery conformance still requires file-backed fault-injection tests. |
| Backup and PITR | Consistent snapshots, timestamp restore rules, retention, and concurrency limits. | STR-13–15 use a `MockPitrManager`; the focused contract suite does not exercise actual backup artifacts or restore through the storage components. | Not demonstrated end to end. Exercise real backup, concurrent writes, restore, and invalid timestamps. |
| Tiering | Hot/warm/cold migration is transparent to readers and obeys the declared failure/timeout behavior. | The contract-hardening suite has no tiering conformance case. Separate tiering APIs and tests do not by themselves prove the complete declaration across concurrent reads and migration. | Requires implementation-linked concurrent migration/read tests and recorded results. |
| Error taxonomy | Storage operation failures map to the declared codes; helper functions classify selected errors. | STR-12 and STR-16 check classification helpers. The phase-3 focused suite checks diagnostic/fault-handler behavior, but neither establishes that all real operation errors map to the declared taxonomy. | Helpers are source-declared and selectively tested; end-to-end mapping remains open. |

The test registration in
[`tests/storage/CMakeLists.txt`](../../tests/storage/CMakeLists.txt) is source
registration, not evidence that a target was built or passed. Separately,
[`test_storage_engine_prod.cpp`](../../tests/storage/test_storage_engine_prod.cpp)
exercises basic `StorageEngine` CRUD and callback scans against RocksDB; it does
not establish cross-component WAL or broader contract conformance. The source
for [`tests/test_mvcc_wal_integration.cpp`](../../tests/test_mvcc_wal_integration.cpp)
covers RocksDB-backed MVCC timestamp reads, version enumeration, GC, and
close/reopen persistence. Its RocksDB internal WAL is not the separate
`WALStorage` API, and the source does not prove WAL ACK ordering or PITR.
Source or registration presence is not execution evidence; no result for this
test is claimed in this task.

## Performance and limits

The size, timeout, concurrency, and retention values in
`storage_api_contract.h` are normative declarations, not proof that each value
is enforced by every implementation. The prior latency table had no matching
current benchmark sources or recorded performance evidence, so its numerical
claims have been removed.

The existing
[`bench_storage_release_gates.cpp`](../../benchmarks/storage/bench_storage_release_gates.cpp)
is an in-memory/mock harness with no real RocksDB or disk I/O. Its source
thresholds and CMake registration do not establish measured production
performance or a passing release gate. The benchmark target is conditional on
`THEMIS_BUILD_BENCHMARKS` and Google Benchmark availability, as shown in
[`benchmarks/storage/CMakeLists.txt`](../../benchmarks/storage/CMakeLists.txt).

## Conformance follow-up

Before describing the whole header contract as implemented, add and run
implementation-linked tests for:

1. The real WAL and primary-store write path, including process restart,
   torn/failed writes, replay idempotency, and checkpoint ordering.
2. Record execution evidence for the existing RocksDB-backed MVCC test source;
   separately test the transaction boundary intended to provide
   read-your-writes, isolation, and conflict behavior.
3. Actual backup/PITR artifact consistency and restore, including concurrent
   writes and invalid timestamps.
4. Tier migration with concurrent foreground reads, plus end-to-end error-code
   mapping and enforcement of any declared limits.
5. Any claimed performance threshold using a stated build/hardware setup and
   recorded benchmark output; do not substitute the mock harness for storage
   I/O measurements.

Until that evidence exists, keep the header's normative declarations distinct
from demonstrated runtime behavior. For detailed component boundaries see the
[storage implementation guide](../../src/storage/IMPLEMENTATION.md), and for
test/benchmark registration see the linked CMake files above.
