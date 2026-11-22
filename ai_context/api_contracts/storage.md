---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-11-22"
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
  - "include/storage/rocksdb_wrapper.h"
  - "include/storage/mvcc_store.h"
  - "src/storage/storage_engine.cpp"
  - "src/storage/wal_storage.cpp"
  - "src/storage/mvcc_store.cpp"
  - "tests/storage/test_storage_contract_hardening_focused.cpp"
  - "tests/storage/test_storage_engine_prod.cpp"
  - "tests/storage/test_streaming_ingest_rocksdb_failure.cpp"
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
  In the implementation, `flush()` succeeds when there is no active file
  descriptor or `fsync` succeeds. If `fsync` fails, it returns
  `ERR_STORAGE_DISK_FULL`; when that failed call has elapsed more than the
  existing 30-second threshold, the error message specifically says durability
  was not confirmed. The elapsed threshold is checked after `fsync` returns; it
  does not establish that a hung call is interrupted at 30 seconds. This is
  implementation behavior from
  [`wal_storage.cpp`](../../src/storage/wal_storage.cpp), not new test evidence.
- [`MVCCStore`](../../include/storage/mvcc_store.h) is a separate
  HLC-versioned store backed by a supplied, already-open `RocksDBWrapper`. It
  exposes version writes and timestamp-relative reads; this surface is not
  itself a general transaction manager.
- [`RocksDBWrapper::Config`](../../include/storage/rocksdb_wrapper.h) accepts an
  optional borrowed `rocksdb::Env*` for file I/O. Null selects RocksDB's default
  environment; a supplied environment must outlive the wrapper and its RocksDB
  handles. The field is appended to preserve existing aggregate-initializer
  ordering, but consumers that depend on binary layout must rebuild.
- `StorageErrorCode` and the inline classification helpers are declared in
  `themis::storage` by `storage_api_contract.h`. They are not the `Result`
  error codes returned by `StorageEngine`'s `IStorageEngine` methods.

The former API table included operations, options, and limits not declared by
the current `IStorageEngine` public interface; those unsupported entries have
been removed from this API summary.

## Declaration versus demonstrated behavior

| Contract area | Normative declaration | Current test/implementation evidence | Conformance status |
|---|---|---|---|
| WAL durability and sequencing | Write-before-ack, idempotent replay, monotonic sequence numbers, checkpoint ordering, and bounded replay when a timeout is configured. | `WALStorage` has a real file-backed append/replay/checkpoint API. `Config::recovery_timeout` applies one millisecond-duration deadline across segments (`0` means unbounded). The standalone Windows WAL runner passed all 29 cases, including timeout handling, 10,000 seeded single-bit mutations, 1,024 seeded two-bit mutations, 256 seeded single-bit and 384 seeded two-bit mutations across 12 segments with varied payload lengths, and corruption detected in a later segment. `WALProcessCrashRecoveryTests` additionally passed 50 real child-process terminations after the parent observed successful fsynced PUT ACKs; replay contained exactly those ACKs, with segment-rotation coverage. STR-01–04 and STR-09–11 in the contract-hardening test use `MockWal` and `MockKvStore`; they do not call `WALStorage` or exercise disk I/O. `tests/test_mvcc_wal_integration.cpp` configures RocksDB's internal WAL through `RocksDBWrapper`; it does not call `WALStorage`. | File-backed timeout/corruption and process termination at ACK boundaries are demonstrated for the separate `WALStorage` API. Interruption during an in-flight append/fsync, the RocksDB primary-store path, and checkpoint ordering under process kill remain unverified. |
| MVCC isolation | Read-your-writes, snapshot isolation, dirty-read prevention, and transaction/version limits. | The contract-hardening test's STR-05–08 use a minimal `MockMvccTx`. The source for `tests/test_mvcc_wal_integration.cpp` uses RocksDB-backed `MVCCStore` and includes timestamp reads, version scans, GC, and close/reopen persistence cases. Those cases do not establish transaction-level read-your-writes, dirty-read prevention, or conflict semantics. | Some concrete MVCC read/version/GC behaviors have implementation-backed test source; full declared transaction semantics remain unverified. No test execution or pass is claimed here. |
| Recovery | Checkpoint-based replay, torn-tail handling, structured recovery status/progress, and recovery limits/diagnostics. | `WALStorage::open()` exposes `RecoveryResult` output and per-record progress snapshots; `MetricsCollector` records cumulative replay progress and terminal attempt/duration; non-complete outcomes emit through `storage.audit`. The standalone Windows WAL runner passed 29 file-backed cases, including timeout, torn-tail, checksum, 10,000 seeded single-bit cases, 1,024 seeded two-bit cases, 256 seeded single-bit and 384 seeded two-bit mutations across 12 segments and varied payload lengths, and rejection of incomplete records in earlier segments without truncation. `WALChecksumTest.InvalidSerializedEntryAbortsRecovery` also passed in a separately linked replication runner. `WALProcessCrashRecoveryTests` passed 50 process-kill/reopen iterations for ACK-gated WAL writes. The contract-hardening replay helper operates on mock entries. The MVCC integration test source includes normal database close/reopen persistence using RocksDB, not a crash/torn-tail test of `WALStorage`. | Structured report, metrics, timeout, torn-tail, sampled corruption, later-segment fail-closed handling, invalid replication payload failure, and ACK-boundary process restart are demonstrated; in-flight crash, RocksDB/index recovery, checkpoint/backup coverage, and quarantine override remain open. |
| Backup and PITR | Consistent snapshots, timestamp restore rules, retention, and concurrency limits. | STR-13–15 use a `MockPitrManager`; the focused contract suite does not exercise actual backup artifacts or restore through the storage components. | Not demonstrated end to end. Exercise real backup, concurrent writes, restore, and invalid timestamps. |
| Tiering | Hot/warm/cold migration is transparent to readers and obeys the declared failure/timeout behavior. | The contract-hardening suite has no tiering conformance case. Separate tiering APIs and tests do not by themselves prove the complete declaration across concurrent reads and migration. | Requires implementation-linked concurrent migration/read tests and recorded results. |
| Error taxonomy | Storage operation failures map to the declared codes; helper functions classify selected errors. | STR-12 and STR-16 check classification helpers. The phase-3 focused suite checks diagnostic/fault-handler behavior, but neither establishes that all real operation errors map to the declared taxonomy. | Helpers are source-declared and selectively tested; end-to-end mapping remains open. |

### Recovery conformance gap: corrupted length fields

The CRC covers the frame header, but `WALStorage::replaySegment()` checks
whether the declared frame size exceeds the remaining bytes before it reads
the payload or validates the CRC. On a real fixture with 12 fsynced, ACKed
PUTs, all 64 individual bit flips across the final frame's eight key/value
length bytes were tested: 61 caused `TruncatedTail`, replayed 11 entries, and
truncated `wal_000004.log` from 117 to 78 bytes; three smaller lengths reached
CRC validation and failed closed. The worker returned exit 6 on the 61
post-recovery count mismatches. This violates the intended no-loss guarantee
under many on-disk length corruptions, despite the exhaustive individual-bit
coverage of all non-length header, payload, and CRC bytes: 2,832 mutations
across the 12-frame fixture each failed twice without WAL mutation (report
`wal-all-protected-bits.json`). A genuine incomplete append has the same
current length/remaining-byte signal; a fix therefore
requires an approved, versioned framing or independent commit-boundary design
and explicit migration/rollback behavior. The diagnostic report
`wal-tail-length-all-bits.json` records each bit outcome. No production
remediation or backward-compatibility policy is claimed.

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
Source or registration presence is not execution evidence. The focused
`test_streaming_ingest_rocksdb_failure.cpp` source injects an error into a real
later RocksDB WAL append, but it has not yet been executed; no pass is claimed.

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

The `unobserved-append` crash scenario releases one more append after N observed
ACKs, then kills without observing its ACK. Its valid replay range is exactly
N..N+1 fully checked PUT entries; corrupt/missing-prefix/extra-entry outcomes
are failures. This explicitly separates an unobserved ACK from an uncommitted
transaction. Timing-based kills do not prove interruption inside a syscall.
Its local Windows 1,000-iteration run passed, observing both valid outcomes:
762 recovered the extra entry, 238 only the ACK prefix, with 764 rotations.
This is not evidence for primary-store/index or transaction recovery.

Both crash scenarios now verify a second fresh recovery with the exact first
replay count and unchanged segment names, sizes and exact byte contents. The two
50-iteration CTests passed locally (100 crashes, 200 validated reopens).
This demonstrates repeated WAL replay stability only, not idempotent
application to a primary store. JSON reports record the verified reopen count
and pre-/post-first-recovery WAL sizes without claiming torn-tail coverage
when no truncation was observed.

First recovery is also checked byte-for-byte: no segment-set change, no older
segment mutation, and only prefix-preserving truncation of the numerically
newest segment in the unobserved-append scenario. ACK-boundary recovery must
leave every byte unchanged. Both 50-iteration crash suites passed with this
additional check on 2026-11-04; primary-store conformance remains open.

When `--report` is supplied, failed iterations preserve test evidence in
unique report-adjacent `<report-stem>-failures/iteration-*` directories.
`failure_artifact` identifies the retained original pre-recovery WAL, current
WAL, requested reproduction parameters and error. Early writer failures may
have no WAL or pre-recovery snapshot. This is test diagnostics, not a production
backup or quarantine API; actual CI artifact publication remains unverified.
The writer workspace is created directly in the evidence root; failure retention
does not copy the original. If `failure.txt` cannot be written, the workspace
remains and the failed report records `retention_error` alongside the original
failure. An unwritable evidence root stops the run before the writer starts.

The separate ACK-boundary process harness passed an additional local Windows
run of 1,000/1,000 iterations (783 with rotation). Its `--report` option emits
a JSON scenario/seed/status summary plus ACK and segment counts for each
completed iteration. CTest produces a report under
`test-results/wal-process-crash.json` in the build directory. This extends
local evidence volume only: it does not demonstrate in-flight fsync/crash
semantics or scheduled CI execution/artifact publication.

Before describing the whole header contract as implemented, add and run
implementation-linked tests for:

1. Extend process-crash coverage from ACK-gated `WALStorage` writes to the
   RocksDB primary-store/index path, in-flight torn/failed writes, replay
   idempotency, and checkpoint ordering.
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
