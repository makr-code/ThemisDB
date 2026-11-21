---
Author: "ThemisDB Contributors"
Created: "2026-11-01"
Last Updated: "2026-11-20"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References:
  - "include/storage/storage_api_contract.h"
  - "include/storage/storage_engine.h"
  - "include/themis/base/interfaces/storage_interface.h"
  - "include/storage/wal_storage.h"
  - "include/storage/rocksdb_wrapper.h"
  - "include/storage/mvcc_store.h"
  - "src/storage/storage_engine.cpp"
  - "src/storage/wal_storage.cpp"
  - "src/storage/streaming_ingest_manager.cpp"
  - "src/storage/detail/streaming_ingest_retry.h"
  - "src/storage/mvcc_store.cpp"
  - "tests/storage/test_mvcc_range_export.cpp"
  - "src/storage/README.md"
  - "src/storage/IMPLEMENTATION.md"
  - "ai_context/api_contracts/storage.md"
  - "tests/storage/test_storage_contract_hardening_focused.cpp"
  - "tests/storage/test_storage_engine_prod.cpp"
  - "tests/storage/test_wal_process_crash.cpp"
  - "tests/storage/wal_crash_worker.cpp"
  - "tests/storage/test_streaming_ingest_retry_queue.cpp"
  - "tests/storage/test_streaming_ingest_rocksdb_failure.cpp"
  - "tests/storage/CMakeLists.txt"
  - "tests/test_mvcc_wal_integration.cpp"
  - "benchmarks/storage/bench_storage_release_gates.cpp"
  - "benchmarks/storage/CMakeLists.txt"
  - "src/storage/DOXYGEN.md"
source: "include/storage/storage_api_contract.h; include/storage/storage_engine.h; include/storage/wal_storage.h; include/storage/rocksdb_wrapper.h; include/storage/mvcc_store.h; src/storage/storage_engine.cpp; src/storage/streaming_ingest_manager.cpp; src/storage/detail/streaming_ingest_retry.h; src/storage/wal_storage.cpp; src/storage/mvcc_store.cpp; tests/storage/test_storage_contract_hardening_focused.cpp; tests/storage/test_storage_engine_prod.cpp; tests/storage/test_streaming_ingest_retry_queue.cpp; tests/storage/test_streaming_ingest_rocksdb_failure.cpp; tests/storage/test_wal_process_crash.cpp; tests/storage/wal_crash_worker.cpp; tests/test_mvcc_wal_integration.cpp; tests/storage/CMakeLists.txt; benchmarks/storage/bench_storage_release_gates.cpp; benchmarks/storage/CMakeLists.txt; src/storage/DOXYGEN.md"
source_class: "secondary"
source_weight: 60
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

> Exact model backend not verifiable from final document text alone.

# Storage — Developer LLM Wiki

Curated orientation for storage implementation and review. This is a secondary
summary, not a public API contract or release-status source. Follow the exact
public header and implementation for a component; use focused tests for the
behavior they actually exercise. A declaration, registration, or mock test is
not proof of system-wide behavior.

## Component boundaries

The storage tree contains several distinct APIs; do not assume they form one
transaction or durability path:

- [`StorageEngine`](../../include/storage/storage_engine.h) implements
  `IStorageEngine` and exposes `open`/`close`, `put`, `get`, `del`, and callback
  scans. The methods use `Result`; a missing key is an error result, not an
  `optional` value.
- [`WALStorage`](../../include/storage/wal_storage.h) is a separate
  append/replay/checkpoint/flush facility. Its `fsync_on_write` option defaults
  to true but is configurable. Do not infer that every `StorageEngine` write
  passes through this WAL. `Config::recovery_timeout_seconds` bounds replay
  across all segments when nonzero; `0` means recovery is unbounded, and
  blocking file I/O and callbacks cannot be interrupted.
  Optional `RecoveryResult` output reports status, the valid replay prefix,
  byte/segment progress, elapsed time, and failure detail. The process-wide
  `MetricsCollector` records `storage_wal_recovery_entries_total`,
  `storage_wal_recovery_bytes_total`, terminal
  `storage_wal_recovery_attempts_total{status}`, and
  `storage_wal_recovery_duration_ms{status}`; an optional per-record observer can also
  adapt snapshots to caller-specific metrics. Non-complete outcomes are
  emitted on the existing `storage.audit` channel. The observer is synchronous,
  must be fast, and must not throw.
- [`MVCCStore`](../../include/storage/mvcc_store.h) stores HLC-versioned values
  in a supplied, already-open `RocksDBWrapper`, with latest and
  timestamp-relative reads, version history, and a move-only snapshot reader.
  The snapshot reader selects visible versions from one RocksDB sequence under
  an HLC watermark and emits explicit `Put`/`Delete` records with bounded
  output and cancellation. It is process-local, not a durable transfer export,
  and does not provide range authorization or write quiescence. It is not, by
  itself, the transaction manager described by the broader contract header.
- Backup, PITR, tiering, blob, and maintenance components have their own
  interfaces and configuration. Their presence does not establish a combined
  backup/recovery or transparent-tiering guarantee.

Use the [storage module README](../../src/storage/README.md),
[implementation guide](../../src/storage/IMPLEMENTATION.md), and
[header index](../../include/storage/README.md) to navigate component
ownership. For the declared API surface and conformance gaps, see the
[storage API contract summary](../api_contracts/storage.md).

## Contract declaration versus evidence

[`storage_api_contract.h`](../../include/storage/storage_api_contract.h)
declares a binding v1.x contract for WAL durability, MVCC, recovery, backup/PITR,
tiering, and error codes. Treat these statements as normative design
declarations, not as evidence that each component currently enforces the full
contract.

The DIS-03 storage primitive is documented in the
[`storage API contract summary`](../api_contracts/storage.md) and
[`src/storage/API.md`](../../src/storage/API.md). Its focused source
[`test_mvcc_range_export.cpp`](../../tests/storage/test_mvcc_range_export.cpp)
covers snapshot visibility, tombstone mapping, cancellation, bounds, and
callback failure; source presence and registration are not execution evidence.

The contract-hardening suite
([`test_storage_contract_hardening_focused.cpp`](../../tests/storage/test_storage_contract_hardening_focused.cpp))
defines in-process `MockWal`, `MockKvStore`, `MockMvccTx`, and
`MockPitrManager` fixtures. Its STR-01–STR-16 cases exercise those fixtures and
the header's classification helpers; they do not call the real WAL, MVCC,
backup, or PITR implementations. In particular, its recovery behavior is a
mock replay function, not a real crash/restart test.

Separately,
[`test_storage_engine_prod.cpp`](../../tests/storage/test_storage_engine_prod.cpp)
uses RocksDB to check `StorageEngine` lifecycle, CRUD, and callback scans. It
does not join `StorageEngine` writes to `WALStorage` or establish the full
cross-component contract.

[`tests/test_mvcc_wal_integration.cpp`](../../tests/test_mvcc_wal_integration.cpp)
uses a real `RocksDBWrapper` with its internal WAL enabled and tests MVCC
timestamp reads, version scans, garbage collection, and database close/reopen
persistence in its source. It does not call the separate `WALStorage` API, prove
WAL ACK ordering or PITR, or establish transaction-manager guarantees. Test
source or CTest registration is not evidence that it ran or passed; no execution
result for it is claimed here.

[`test_wal_process_crash.cpp`](../../tests/storage/test_wal_process_crash.cpp)
and its [`wal_crash_worker.cpp`](../../tests/storage/wal_crash_worker.cpp)
helper add source-level POSIX process coverage for separate `WALStorage`: the
worker fsyncs each PUT, waits for a parent permit between operations, and is
SIGKILLed after acknowledged writes at rotation or checkpoint boundaries. The
test checks exact PUT replay after reopen. It does not kill inside an active
write/fsync/rotation/checkpoint syscall and does not cover backup/restore or
RocksDB index consistency; it has not been executed, so it is not run evidence.

[`test_streaming_ingest_retry_queue.cpp`](../../tests/storage/test_streaming_ingest_retry_queue.cpp)
checks that the internal requeue helper excludes a committed prefix and places
the failed suffix before newer queued PUTs for a supplied later-chunk offset.
[`test_streaming_ingest_rocksdb_failure.cpp`](../../tests/storage/test_streaming_ingest_rocksdb_failure.cpp)
now injects an error into a real later RocksDB WAL-file append and checks the
failed offset, committed prefix, retry ordering with a concurrently queued PUT,
and close/reopen values. This test source has not been built or run; it is not
execution evidence. `RocksDBWrapper::Config::env` is a borrowed pointer and
must outlive its wrapper and RocksDB handles.

Focused suites under `tests/storage/` are registered by
[`tests/storage/CMakeLists.txt`](../../tests/storage/CMakeLists.txt); that
registration does not establish that a target was built or passed.

The file-backed WAL suite in [`tests/test_wal_storage.cpp`](../../tests/test_wal_storage.cpp)
was executed through a separately linked Windows runner: all 29 tests passed,
including 10,000 seeded single-bit cases, 1,024 seeded two-bit cases, 256
seeded single-bit and 384 seeded two-bit cases across 12 segments with varied
payload lengths, rejection
of incomplete records in earlier segments without truncation, and fail-closed
handling of corruption in a later segment. Tail truncation is limited to the
latest segment. The regular CMake
focused target remains blocked by an unrelated generated-gRPC compiler error;
the standalone suite alone does not establish process-crash recovery or
quarantine behavior. Separately, [`WALProcessCrashRecoveryTests`](../../tests/storage/test_wal_process_crash.py)
passed 50 CTest iterations with real child-process termination after
parent-observed fsynced ACKs and exact replay verification, including
segment-rotation coverage. It does not kill during an in-flight append/fsync
and does not cover RocksDB primary/index consistency, checkpoint, or backup
restore. The separate replication regression
`WALChecksumTest.InvalidSerializedEntryAbortsRecovery` also passes in a
standalone Windows runner; the full replication suite has not been run. The
Developer LLM Wiki currently has no dedicated replication module page, so the
replication finding was checked against its API, roadmap, source, and test.

The ACK-boundary harness also passed a local Windows 1,000-iteration run:
783 iterations crossed segment rotation, with 1,000 result records in
`build-msvc-windows-debug/test-results/wal-process-crash-1000.json`.
Its optional `--report` output identifies the scenario and seed, per-iteration
ACK/segment counts, elapsed time, and running/passed/failed status. The standard
50-iteration CTest writes `test-results/wal-process-crash.json` below the build
directory. These are local results, not evidence of scheduled nightly execution
or uploaded CI artifacts. The first iteration now deliberately reaches rotation
so even a one-iteration smoke run exercises that boundary.

CI configuration now requests 50 iterations in the PR Quality-Build caller and
1,000 in the existing sanitizer-nightly caller, with secondary JSON uploads
also attempted after failure unless cancelled. Workflow contracts and local
Windows runs do not establish that the Linux/sanitizer jobs or GitHub uploads
have succeeded; executed CI artifacts remain an acceptance gap.

`WALProcessUnobservedAppendTests` adds a second scenario: after N observed ACKs,
release one more append, then kill without awaiting its ACK. Recovery must
contain N or N+1 exact entries. Only the worker's explicit count-mismatch
result with exactly one extra entry permits a second content-verification run;
corruption and missing acknowledged entries fail immediately. The JSON reports
record recovered count and requested kill delay. PR/nightly callers request
50/1,000 iterations per scenario. This is a timing race, not a proof of
interruption inside a particular write/fsync syscall; a missing ACK does not
establish that a write was not committed.
The local Windows run passed 1,000/1,000 iterations, with 764 rotations:
762 recovered the extra PUT and 238 recovered only the acknowledged prefix.
The evidence is in `build-msvc-windows-debug/test-results/wal-unobserved-append-1000.json`;
Linux/sanitizer execution is still unverified.

Both scenarios now validate recovery in two fresh processes per crash. The
second uses the exact count accepted by the first, with no uncertain append
allowance, and must preserve every segment name, size and exact byte contents.
Both local 50-iteration CTests passed with this check (100 crashes and 200
validated reopens). Report fields `verified_reopens`,
`wal_bytes_before_recovery` and `wal_bytes_after_recovery` distinguish replay
stability from possible first-recovery truncation. No actual torn-tail
occurrence is claimed merely because this measurement exists. This does not
prove idempotent application to RocksDB or secondary indexes.

First recovery now has a separate exact-byte check (2026-11-04): no segment
may be added or removed, all older segments must stay identical, and only the
numerically newest segment may shrink to an identical prefix when an append
was released without observing its ACK. ACK-boundary recovery allows no
mutation. Both local 50-iteration crash suites passed with these checks.
These byte snapshots are small test-fixture data, not a production backup
or a replacement for the worker's full entry validation.
An ad-hoc local campaign on 2026-11-04 also passed all 40 byte prefixes of a
final 39-byte frame after a real writer kill, with exact expected replay and
repeat byte stability. All 18 payload/CRC byte bit-flips and 36 partial
older-segment tails failed closed twice without mutation. The report
`build-msvc-windows-debug/test-results/wal-tail-boundaries-20261104.json`
contains 94 cases, 80 successful reopens and 108 failed opens (189 verifier
processes including one count probe), in 31.203 s. The fixture ACKed 12 writes
before deliberate disk faults; partial tails test preservation of entries
1..11, not loss of an ACK caused by termination alone. This is not CI-registered
or targeted syscall/primary-store evidence.
The campaign is now checked in as
[`test_wal_tail_boundaries.py`](../../tests/storage/test_wal_tail_boundaries.py)
and called by the existing PR/nightly lanes, with `wal-tail-boundaries.json`
and failure-directory uploads. Local execution passed all 94 cases again
(80 successful reopens, 108 closed failures, 189 verifier processes, 21.146 s).
64 focused tests plus 35 subtests, actionlint and caller Bash syntax passed.
The new report fingerprints the worker, campaign and shared crash harness;
fixture layout changes fail explicitly. Historical ad-hoc evidence is unchanged,
and configured GitHub invocation is not a successful execution/publication claim.
The checked-in campaign subsequently expanded to all three older segments,
36 incomplete-frame cuts per segment. `affected_segment` distinguishes the
repeated offsets in results and retained reproduction metadata. Local report
`build-msvc-windows-debug/test-results/wal-tail-all-segments.json` passed
166 cases: 80 successful reopens, 252 closed failures, 333 verifier processes
(43.566 s). Older segments stayed byte-identical on both failed opens; all
38 nonempty partial latest tails were repaired exactly. The exact per-segment
fault sets and existing contracts passed 64 tests plus 35 subtests.
Historical 94-case evidence is unchanged; GitHub/sanitizer execution and
targeted syscall/primary-store evidence remain open.
Boundary cases now also copy their injected WAL into `before-recovery/`
before any verifier. Snapshot I/O failure stops recovery; failing cases retain
the original injected bytes alongside current `wal/`. A real-worker diagnostic
count mismatch after tail repair preserved exact 79-byte input and 78-byte
output, with a failed report (`wal-tail-forensic-diagnostic.json`).
The full campaign passed again (`wal-tail-forensic-retention.json`, 166 cases,
80 successful reopens, 252 closed failures, 333 processes, 40.822 s).
66 focused tests plus 37 subtests passed, including both recovery paths and
snapshot failure. This demonstrates test evidence retention, not a production
defect or backup/power-loss guarantee; historical reports remain unchanged.
The campaign now flips the low bit at each of the 198 payload/CRC byte
positions in all twelve fixture frames, including earlier frames in the newest
segment. Each corruption fails closed twice without WAL changes.
Results and retained reproduction metadata carry absolute `segment_byte_offset`;
unexpected older-segment lengths fail the fixture contract explicitly.
Local `build-msvc-windows-debug/test-results/wal-all-frame-corruption.json`
passed 346 cases: 80 successful reopens, 612 closed failures, 693 verifier
processes (83.410 s). Exact per-frame byte sets, fingerprints and cleanup
were independently verified; 67 tests plus 39 subtests and syntax checks passed.
Historical reports remain unchanged. This is one bit per payload/CRC byte,
not every bit/header field or targeted syscall/primary-store/sanitizer evidence.
The subsequent header campaign flips the low bit at each magic, sequence and
entry-type byte (frame offsets 0..12), covering 156 positions in all twelve
frames. Every fault fails closed twice without mutation; absolute offsets are
included in results and retained reproduction metadata.
`build-msvc-windows-debug/test-results/wal-all-frame-header-corruption.json`
passed 502 cases: 80 successful reopens, 924 closed failures, 1,005 verifier
processes (120.286 s). Exact header/payload/CRC sets, fingerprints and cleanup
were independently checked; 67 tests plus 40 subtests passed.
Length fields (13..20) remain separate because corrupted lengths may resemble
an incomplete newest tail. No blanket length-corruption rejection, all-bit,
targeted syscall, primary-store or successful sanitizer coverage is claimed;
historical reports remain unchanged.
An explicit probe of all 64 individual bits across the eight tail-frame
key/value length bytes (absolute offsets 91..98) exposed a remaining data-loss
gap. On a fixture with 12 fsynced ACKed PUTs, 61 mutations made recovery report
`TruncatedTail`, replay only 11 entries, and truncate the latest segment from
117 to 78 bytes; three smaller declared lengths reached CRC validation and
failed closed. The worker exited 6 on its 61 post-recovery count assertions.
The frame CRC is checked only after declared lengths fit in the remaining segment, so the current path
cannot distinguish these corrupted lengths from an incomplete final append.
This is not fail-closed corruption handling and blocks STO-01 closure. Closing
it requires a maintainer-approved versioned framing/commit-boundary design,
migration/rollback policy, tests for all eight length bytes, and retained
incomplete-tail recovery; no production fix is claimed. Per-bit outcomes are
in `build-msvc-windows-debug/test-results/wal-tail-length-all-bits.json`.
All eight bit positions at every non-length header, payload and CRC byte were
also tested across the fixture: 2,832 faults, each rejected twice without
mutation. The independent report
`build-msvc-windows-debug/test-results/wal-all-protected-bits.json` passed
2,980 cases (80 successful reopens, 5,880 failed opens, 5,961 verifier
processes, 826.929 s); per-segment byte/bit sets, fingerprints and cleanup
were checked, and 67 tests plus 40 subtests passed. The eight length fields
remain excluded because of the data-loss result above.

With `--report`, failed iterations retain report-adjacent `*-failures/iteration-*`
directories with pre-recovery and failure-time WAL, reproduction parameters
and error. The report's `failure_artifact` field identifies the unique directory.
Early writer failures may have no WAL or pre-recovery snapshot. Successful
iterations still clean up, and retention I/O failures propagate. Both CI callers
declare these directories for secondary upload; GitHub publication is still
unverified. This test evidence retention is not production backup/quarantine.
Workspaces are now created directly in the evidence root to avoid losing
original WALs on post-failure copy errors. A diagnostic `failure.txt` write error
preserves the workspace and appears as `retention_error` alongside the original
failure. An unwritable evidence root fails before the writer starts; successful
cleanup may leave an empty root. Both local 50-iteration CTests and 37 focused
tests plus 14 subtests passed. A real worker retained both WAL states after a
deliberate recovery failure and an actual diagnostic filesystem write error.
Failed iterations now also retain combined writer stdout/stderr as UTF-8
`writer.log`, saved after process shutdown and reader join. Consumed ACKs
and remaining diagnostic lines are included, not unflushed child buffers.
Transcript I/O failure fails the run and preserves any original writer error
in the report. Both 50-iteration crash suites and 40 focused tests plus 14
subtests passed; a real failed iteration retained all 12 ordered ACKs.
Writer pipe/read decoding errors now propagate through the ACK queue.
Unconsumed errors are checked after reader join, so an error after the last
ACK still fails before recovery and retains the partial transcript. Both
50-iteration crash suites and 42 focused tests plus 16 subtests passed.

The fully hardened harness was revalidated locally at 1,000 crashes per
scenario on 2026-11-04 (4,000 verified reopens). ACK-boundary: 783 rotations,
no byte changes, 378.079 s. Unobserved append: 758 rotations, 635 additional
PUTs, 365 prefix-only recoveries, 174 permitted first tail repairs, 560.185 s.
Reports `wal-process-crash-hardened-1000.json` and
`wal-unobserved-append-hardened-1000.json` under the Windows build's
`test-results/` are distinct from older scale evidence. Both successful
evidence roots were empty; 42 focused tests plus 16 subtests passed.
This is not sanitizer/GitHub execution or targeted syscall/primary-store proof.
New reports now include `worker_sha256` and `harness_sha256` from files read
before iterations, with start/end equality required for `passed`. Both local
50-iteration crash suites and 46 focused tests plus 16 subtests passed.
Fingerprint I/O errors fail explicitly. This is file identification, not
dependency-DLL/compiler/build attestation; changes reverted between checks are
not detected. Earlier scale reports are not retroactively fingerprinted.
Reports are now atomically checkpointed after each validated iteration, with
elapsed time and status `running` until final checks pass. Hard parent
termination leaves the last completed prefix, not proof of the next iteration
or successful completion. Progress-write errors stop further iterations.
Both 50-iteration crash suites and 60 focused tests plus 33 subtests passed.
The forced-parent regression validates reporting with test-only iteration
results, not WAL recovery; atomic replacement is not power-loss durability.

The sanitizer-nightly caller now uses `-fno-sanitize-recover=all` for C/C++
in both modes, preventing recoverable UBSan errors from returning success.
The harness also rejects recognized ASan/LSan/UBSan/TSan/MSan diagnostic
headers and source-location-prefixed UBSan errors independently of exit status.
It checks the writer before ACK acceptance and after reader join, and both
verifier stdout/stderr including N+1 retries. Both 50-iteration crash suites
and 53 focused tests plus 30 subtests passed. These diagnostic-text tests do
not execute instrumented sanitizer binaries; signature detection is not
exhaustive and cannot inspect unflushed output.
Verifier timeout errors now preserve captured stdout/stderr with byte-safe
`repr`, expected replay count and the chained original timeout. The existing
30-second deadline applies to each process, including N+1 retries; a timeout
never grants another retry. Failed reports and retained `failure.txt` include
the diagnostics. Both 50-iteration crash suites and 57 focused tests plus 33
subtests passed, including a real subprocess timeout with flushed output.
This is harness diagnostics, not measured production recovery latency.
Local contract tests and executed Bash flag selection pass, not sanitizer
runtime execution. The inspected develop run
[41760710588](https://github.com/makr-code/ThemisDB/actions/runs/41760710588)
failed compiling `src/sharding/raft_state.cpp` (`spdlog` undeclared) before
validation. The generated build/CI wiki index remains a 2026-11-02 snapshot;
this curated note does not claim a refreshed inventory or successful CI run.
Revision comparison on 2026-11-04 shows the failing run built
`d66b7a7f4f50e08c3bebeb8b5ef57d28745b2da5`. Its new Raft error paths use
`spdlog::error` without a direct include; those paths are not present in this
follow-up workspace. The local configured Raft object build passes, but it
does not fix or validate the different develop revision. See the
[sharding roadmap](../../src/sharding/ROADMAP.md) for the exact compilation
scope. Neither a linked sharding build nor sanitizer runtime success is claimed.

The current gap-by-gap assessment and follow-up test requirements are in the
[API contract summary](../api_contracts/storage.md). Do not silently promote
the mock test cases to implementation conformance evidence.

## Performance and examples

[`bench_storage_release_gates.cpp`](../../benchmarks/storage/bench_storage_release_gates.cpp)
uses in-memory mocks and explicitly performs no real RocksDB or disk I/O. Its
thresholds and conditional CMake registration are not measured production
performance or proof of a release gate. The target is enabled only when
`THEMIS_BUILD_BENCHMARKS` is on and Google Benchmark is available.

The
[schema/layout example guide](../../include/storage/examples/README.md)
labels `schema_layout_advisor_example.cpp` as specification/planned API. Its
advisor calls are commented out and its output is simulated; do not treat it as
a current implementation or verified recommendation.

## Generated reference artifact

The separately generated
[storage Doxygen page](../../src/storage/DOXYGEN.md) reports a successful
Doxygen 1.9.8 run covering 164 C/C++ files and 547 compounds; its warning log
contains 656 untriaged warnings. This documents Doxygen generation only, not
runtime conformance or test execution. The generated artifact was not edited as
part of this documentation update.

## Source priority and freshness

For behavior, use the public header and matching implementation first, then
focused tests and module documentation. The storage contract header defines
normative intent; implementation and test evidence determine what is currently
demonstrated. Root governance remains authoritative for release status.

This page was manually curated and linked; the generated wiki source inventory
was not regenerated for it. `WIKI_STATUS.json` records an incremental snapshot
from 2026-11-02 with 1,450 removed and 26 changed sources, so it is not evidence
of a fresh full-source scan. This page does not claim that broader validation.
