---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References:
  - "storage_engine.h"
  - "mvcc_store.h"
  - "wal_storage.h"
  - "storage_api_contract.h"
  - "../../src/storage/README.md"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Storage public headers

This directory declares storage APIs and supporting types. A declaration does
not by itself prove that a feature is wired into the application, enabled in a
deployment, or covered by an executed test.

## Core interfaces

- [`storage_engine.h`](storage_engine.h) declares `StorageEngine`, an
  `IStorageEngine` implementation with `open`/`close`, key-value operations,
  callback scans, dependency injection, and cumulative I/O/scan counters.
- [`mvcc_store.h`](mvcc_store.h) declares `MVCCStore`, backed by a supplied
  `RocksDBWrapper`, with HLC-versioned writes, latest/timestamp reads, history
  scans, and version garbage collection.
- [`wal_storage.h`](wal_storage.h) declares a separate append/replay/
  checkpoint/flush API. Its `fsync_on_write` option defaults to true and is
  configurable. Do not equate it with RocksDB's internal WAL.
- [`backup_manager.h`](backup_manager.h), [`pitr_manager.h`](pitr_manager.h),
  [`blob_storage_backend.h`](blob_storage_backend.h), and
  [`tiered_storage.h`](tiered_storage.h) declare other separate facilities.

## Contract and implementation references

- [`storage_api_contract.h`](storage_api_contract.h) contains normative
  contract statements; check each statement against the relevant implementation
  and tests before presenting it as observed runtime behavior.
- [`../../src/storage/ARCHITECTURE.md`](../../src/storage/ARCHITECTURE.md) is the
  module architecture overview.
- [`../../src/storage/API.md`](../../src/storage/API.md) summarizes selected
  caller obligations.
- [`../../tests/storage/CMakeLists.txt`](../../tests/storage/CMakeLists.txt)
  and [`../../benchmarks/storage/CMakeLists.txt`](../../benchmarks/storage/CMakeLists.txt)
  show current test/benchmark source registration.

No latency, throughput, encryption-at-rest, ACID, or end-to-end recovery
guarantee is implied by this index.
