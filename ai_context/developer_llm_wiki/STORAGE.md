---
Author: "ThemisDB Contributors"
Created: "2026-11-01"
Last Updated: "2026-11-01"
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
  - "include/storage/mvcc_store.h"
  - "src/storage/storage_engine.cpp"
  - "src/storage/wal_storage.cpp"
  - "src/storage/mvcc_store.cpp"
  - "src/storage/README.md"
  - "src/storage/IMPLEMENTATION.md"
  - "ai_context/api_contracts/storage.md"
  - "tests/storage/test_storage_contract_hardening_focused.cpp"
  - "tests/storage/test_storage_engine_prod.cpp"
  - "tests/test_mvcc_wal_integration.cpp"
  - "tests/storage/CMakeLists.txt"
  - "benchmarks/storage/bench_storage_release_gates.cpp"
  - "benchmarks/storage/CMakeLists.txt"
  - "src/storage/DOXYGEN.md"
source: "include/storage/storage_api_contract.h; include/storage/storage_engine.h; include/storage/wal_storage.h; include/storage/mvcc_store.h; src/storage/storage_engine.cpp; src/storage/wal_storage.cpp; src/storage/mvcc_store.cpp; tests/storage/test_storage_contract_hardening_focused.cpp; tests/storage/test_storage_engine_prod.cpp; tests/test_mvcc_wal_integration.cpp; tests/storage/CMakeLists.txt; benchmarks/storage/bench_storage_release_gates.cpp; benchmarks/storage/CMakeLists.txt; src/storage/DOXYGEN.md"
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
  passes through this WAL.
- [`MVCCStore`](../../include/storage/mvcc_store.h) stores HLC-versioned values
  in a supplied, already-open `RocksDBWrapper`, with latest and
  timestamp-relative reads and version history. It is not, by itself, the
  transaction manager described by the broader contract header.
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

Focused suites under `tests/storage/` are registered by
[`tests/storage/CMakeLists.txt`](../../tests/storage/CMakeLists.txt); that
registration does not establish that a target was built or passed.

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
and `WIKI_STATUS.json` were not regenerated for it. Their recorded generation
snapshot is 2026-10-31, and this page does not claim a fresh full-source scan.
