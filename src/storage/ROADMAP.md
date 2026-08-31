# Storage Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-19 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production-capable storage runtime exists for durable persistence, MVCC/WAL lifecycle behavior, backup/PITR flows, blob/tiering behavior, and storage audit/integrity surfaces. Source revalidation on 2026-08-31 found two degraded restore paths in `backup_manager.cpp`: when compression or OpenSSL dependencies are absent, restore currently copies bytes verbatim instead of performing real decompression/decryption.

## In Progress

- [~] hardening failure-path behavior under sustained write/load and maintenance overlap (Target: Q3 2026)
- [~] improving diagnostics consistency across storage, replay, and recovery stages (Target: Q3 2026)
- [~] stabilizing benchmark-backed release guardrails for storage hot paths (Target: Q3 2026)
- [x] BLOCK 3: Storage Module Integration with AccessCoordinator (Target: Q4 2026) ✅ COMPLETE
  - [x] Added PromotionListener support to TieredStorageManager
  - [x] Added `setPromotionListener()` method in header and implementation
  - [x] Emit onStorageAccess() signals when detecting hot tiers
    - [x] emitPromotionEvent() helper method
    - [x] Hot pattern detection in get() for WARM/COLD tiers
    - [x] Hot pattern detection in runMigrationCycle()
  - [x] Implement predictive promotion callbacks with access window tracking

## Planned Features

### Short-term (3-6 months)
- [ ] integrate with AccessCoordinator for unified cache-storage tier management (Target: Q4 2026)
  - Add PromotionListener callbacks to emit hot-access signals to coordinator
  - Implement coordinator-guided promotion paths (cold→warm→L3)
  - Extend TieredStorageManager with coordinator hooks
  - See: `src/access_model/ROADMAP.md` Phase 4
- [ ] make `BackupManager::decompressPath()` fail closed or perform real decompression when zstd/lz4 are unavailable; the current degraded path copies bytes verbatim and is only safe when the matching write path also ran in raw-copy mode (Target: Q4 2026)
- [ ] make `BackupManager::decryptFile()` fail closed or perform real decryption when OpenSSL is unavailable; the current degraded path copies ciphertext verbatim (Target: Q4 2026)
- [ ] wire the remaining `ggml_tensor_bridge.cpp` production injection seams (`GgmlAllocFn`, `PrefetchFn`, `TypeRegistrationFn`) at server initialization (Target: Q4 2026)
- [ ] tighten deterministic behavior under heavy WAL replay and compaction pressure (Target: Q4 2026)
- [ ] expand stress coverage for blob/tiering and PITR edge scenarios (Target: Q4 2026)
- [ ] improve operator-facing diagnostics for recovery and maintenance incidents (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for write/replay/recovery-sensitive paths (Target: Q1 2027)
- [ ] broaden benchmark depth for mount-latency and storage allocator edge paths (Target: Q1 2027)
- [ ] harden long-run reliability under sustained mixed read/write pressure (Target: Q1 2027)

### Distributed Maturity Phase 3 — Track 2 Items (Q3–Q4 2026)

These items are part of the next-phase **Track 2: Distributed Systems Maturity — 3.4 Storage** plan
(see `ROADMAP.md §Track 2`). Hard gate per item: deterministic under-load benchmark + `release_critical` CI green.

- [ ] **Tiered storage (hot/warm/cold) with automatic data migration**: implement age- and
  access-frequency-based tier migration; data that has not been accessed within a configurable
  window automatically demoted from hot (NVMe) → warm (HDD) → cold (object storage) (Target: Q4 2026)
  - Inputs: tier policy config (`storage.tier.hot_max_age_s`, `storage.tier.warm_max_age_s`,
    `storage.tier.cold_backend`); access-frequency tracker
  - Acceptance: migration runs without blocking foreground I/O; cold-to-hot promotion completes
    within 100 ms for objects ≤ 1 MB; `storage_tier_migration_total` counter wired; `release_critical` green
- [ ] **Cloud-native S3/GCS/Azure backend hardening**: harden the existing cloud backend adapter
  for sustained production use: retry with exponential backoff, integrity checksums (MD5/CRC32C),
  multipart upload for objects > 100 MB, SDK version pinning (Target: Q4 2026)
  - Acceptance: 72-hour soak at 1 000 object writes/s with zero data-integrity errors;
    multipart upload tested for 1 GB object; SDK versions pinned in vcpkg manifest

## Implementation Phases

### Phase 1: Design / API Contract
- [x] Freeze storage module API contract — WAL durability, MVCC snapshot isolation, crash-recovery, backup/PITR, tiering transparency, error taxonomy (include/storage/storage_api_contract.h) (Target: Q3 2026)
- [x] Define explicit StorageErrorCode taxonomy (WAL_WRITE_FAILED, CHECKPOINT_FAILED, RECOVERY_INCOMPLETE, PITR_INVALID_TIMESTAMP, COMPACTION_ABORTED, STORAGE_EXHAUSTED, …) (Target: Q3 2026)

### Phase 2: Core Implementation
- [ ] complete hardening for WAL/MVCC and backup/PITR internals (Target: Q4 2026)
- [ ] align tiered/blob/redundancy behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [x] standardize fail-safe behavior for replay faults, storage pressure, and recovery errors (Target: Q4 2026) ✅ COMPLETE (2026-08-03)
  - [x] Created storage_error_diagnostics.h/cpp with unified error classification
  - [x] Implemented storage_recovery_fault_handler.h/cpp for recovery fault handling
  - [x] Implemented storage_pressure_manager.h/cpp for capacity management
  - [x] Created comprehensive Phase 3 focused tests (24 test cases)
- [x] unify diagnostics across persistence, maintenance, and recovery incident classes (Target: Q4 2026) ✅ COMPLETE (2026-08-03)
  - [x] StorageErrorContext struct for detailed error information
  - [x] emitDiagnosticEvent() for structured event emission
  - [x] Incident type classification (RECOVERY_FAULT, STORAGE_PRESSURE, etc.)
  - [x] Error severity levels (INFO, LOW, MEDIUM, HIGH, CRITICAL)

### Phase 4: Tests
- [x] Contract-hardening focused tests STR-01..STR-16 covering WAL durability, MVCC isolation, crash-recovery, and PITR invariants (tests/storage/test_storage_contract_hardening_focused.cpp) (Target: Q4 2026)
- [x] Expand focused regressions for replay, PITR, and tiered/blob edge scenarios (Target: Q4 2026)
- [x] Extend deterministic stress fixtures for mixed write + maintenance workloads (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [x] Lock benchmark-backed release gates for storage hot paths: SGRG-01..SGRG-06 in benchmarks/storage/bench_storage_release_gates.cpp (WAL throughput ≥100k ops/s, MVCC read p99≤100µs, MVCC write p99≤500µs, checkpoint p99≤10ms, tiering decision p99≤50µs, compaction check p99≤100µs) (Target: Q4 2026)
- [x] Validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core storage module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] Contract header frozen: include/storage/storage_api_contract.h
- [x] Contract-hardening tests delivered: tests/storage/test_storage_contract_hardening_focused.cpp (STR-01..STR-16)
- [x] Release-gate benchmarks delivered: benchmarks/storage/bench_storage_release_gates.cpp (SGRG-01..SGRG-06)

## Production Readiness Checklist

- [x] core storage surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] Contract header frozen: include/storage/storage_api_contract.h (Phase 1)
- [x] Contract-hardening tests: tests/storage/test_storage_contract_hardening_focused.cpp (Phase 4, STR-01..STR-16)
- [x] Release-gate benchmarks: benchmarks/storage/bench_storage_release_gates.cpp (Phase 5, SGRG-01..SGRG-06)
- [x] Benchmark CMakeLists registered: benchmarks/storage/CMakeLists.txt
- [x] Error diagnostics system: include/storage/storage_error_diagnostics.h (Phase 3)
- [x] Recovery fault handler: include/storage/storage_recovery_fault_handler.h (Phase 3)
- [x] Storage pressure manager: include/storage/storage_pressure_manager.h (Phase 3)
- [x] Phase 3 focused tests: tests/storage/test_storage_phase3_error_handling_focused.cpp (24 test cases)
- [ ] remaining hardening tasks closed for durability/recovery edge paths
- [ ] release benchmark stabilization complete

## Known Issues and Limitations

- runtime behavior depends on storage configuration, backend profile, and workload shape.
- selected replay/recovery/tiering edge scenarios need continued hardening.
- benchmark depth should continue expanding for advanced storage workloads.

## Breaking Changes

No breaking storage contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `storage`
- [ ] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027)
- [ ] Contribute to or validate long-duration soak test coverage for this module's primary paths (Target: Q1 2027)
- [ ] Ensure runbook coverage for operator-critical scenarios in this module (Target: Q1 2027)

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [ ] This module's distributed/acceleration paths fail closed (Target: Q1 2027)
- [ ] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027)
- [ ] Operator-critical paths have diagnostics, alerts, and runbooks (Target: Q1 2027)
