---
Author: "ThemisDB Contributors"
Created: "2026-07-19"
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
  - "../../ROADMAP.md"
  - "../../include/storage/storage_engine.h"
  - "../../include/storage/tiered_storage.h"
  - "../../include/storage/wal_storage.h"
  - "../../include/storage/rocksdb_wrapper.h"
  - "../../include/storage/mvcc_store.h"
  - "../../tests/storage/test_mvcc_range_export.cpp"
  - "../../include/storage/streaming_ingest_manager.h"
  - "../../tests/storage/test_streaming_ingest_rocksdb_failure.cpp"
  - "../../tests/storage/CMakeLists.txt"
  - "../../benchmarks/storage/CMakeLists.txt"
  - "../../benchmarks/storage/bench_storage_release_gates.cpp"
  - "../../benchmarks/storage/bench_storage_dedicated_gates.cpp"
  - "../../tests/storage/test_wal_process_crash.cpp"
  - "../../tests/integration/CMakeLists.txt"
  - "../../tests/integration/test_storage_engine_soak.cpp"
  - "../../research/implementation_influence/by_module.md"
  - "RESEARCH.md"
  - "wal_storage.cpp"
  - "rocksdb_wrapper.cpp"
  - "mvcc_store.cpp"
  - "streaming_ingest_manager.cpp"
  - "tiered_storage.cpp"
  - "backup_manager.cpp"
  - "storage_engine.cpp"
Related_Issue: "#7456"
Related_Milestone: "DOC-WEEKLY-2026-W45"
---

Exact model backend not verifiable from final document text alone.

# Storage Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | source inventory checked: 2026-10-31; execution status not revalidated -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

The source tree contains the storage, MVCC, WAL, backup/PITR, blob/tiering, and integrity components listed below. Their presence does not establish a combined transaction path, deployment configuration, or blanket production-readiness claim. `StorageEngine` production-mode dependency checks are implemented in `storage_engine.cpp`; other status claims require matching current execution evidence.

### STO-01 / STO-07 / STO-10 follow-up (2026-11-02)

- [~] **STO-01 — partially implemented:** `WALStorage::open()` exposes structured recovery status, strict-by-default recovery, configurable timeout (zero unlimited; negative rejected), final-tail handling, explicit corrupt-segment quarantine, and failure propagation for callback/I/O errors. Replication deserialization failure now aborts recovery. Process-crash, focused test-execution, and ≤5% replay-regression evidence remain open.
- [~] **STO-07 — partial source coverage, not executed:** `tests/storage/test_wal_process_crash.cpp` starts a POSIX WAL worker with `fsync_on_write=true`, blocks it between operations, sends `SIGKILL` after acknowledged appends at rotation and checkpoint boundaries, then checks that recovery replays exactly the acknowledged PUTs. It does not kill inside an active write/fsync/rotation/checkpoint call and does not cover backup/restore or RocksDB index consistency. Test execution and all required run artifacts remain open.
- [~] **STO-10 — partially implemented:** batch admission is all-or-nothing; WriteBatch chunks are atomic and failed chunks plus suffix are retained in order. `tests/storage/test_streaming_ingest_rocksdb_failure.cpp` now injects an I/O failure into a real later RocksDB WAL append while a newer event is queued, then checks the reported offset, retained suffix order, retry result, and close/reopen persistence. The source has not yet been executed; test-execution evidence remains open.
- **Evidence gates:** ≥50 PR and ≥1,000 nightly process-kill artifact runs, in-syscall and backup/index coverage, the ≤5% replay benchmark against a recorded baseline, and actual test execution evidence are pending. The subprocess test source is registered but was not run during this follow-up.

## In Progress

- [~] Provide a process-local MVCC export reader using one RocksDB snapshot sequence and HLC watermark, bounded output, cancellation, and explicit tombstones; durable restart/resume and range authorization remain outside this primitive (Target: M2 · 2027-03-31)
- [~] hardening failure-path behavior under sustained write/load and maintenance overlap (Target: Q3 2026)
- [~] improving diagnostics consistency across storage, replay, and recovery stages (Target: Q3 2026)
- [~] stabilizing benchmark-backed release guardrails for storage hot paths (Target: Q3 2026)
- [x] remove implicit `SecuritySignatureManager(nullptr)` memory-store fallback from production paths; null-backend construction now fails closed unless the caller explicitly enables the test-only fallback option (Target: Q3 2026)
- [x] enforce `StorageEngine` production fail-closed dependency prerequisites: default/no-op encryption, key-provider, index-manager, and null index-manager construction are rejected in production mode (Target: Q4 2026)
- [x] BLOCK 3: Storage Module Integration with AccessCoordinator (Target: Q4 2026) ✅ COMPLETE
  - [x] Added PromotionListener support to TieredStorageManager
  - [x] Added `setPromotionListener()` method in header and implementation
  - [x] Emit onStorageAccess() signals when detecting hot tiers
    - [x] emitPromotionEvent() helper method
    - [x] Hot pattern detection in get() for WARM/COLD tiers
    - [x] Hot pattern detection in runMigrationCycle()
  - [x] Implement predictive promotion callbacks with access window tracking

## Issue #7456 — Versioned Storage Roadmap

This is forward planning, not a status promotion: all work below remains open. The root roadmap currently identifies v2.4.0-alpha as the current release; v2.5.0 and v3.0.0 are planning targets, not release dates or commitments.

### Current maturity and evidence boundary

- **Qualitative maturity:** implementation maturity is **partial** (source surfaces and some registered tests exist); evidence maturity is **early/insufficient** (execution records and production-backed measurements are not established here). Workload/application fit is not yet demonstrated across representative use cases or mixed workloads. Security, compliance, and operability evidence also remains incomplete: provider/restore failure tests, an explicit control/support matrix, and validated operator recovery procedures are still planned.
- **Implementation:** WAL recovery, RocksDB integration, streamed ingest, tiering, backup/PITR, and production-mode dependency checks have source surfaces in `src/storage/wal_storage.cpp`, `src/storage/rocksdb_wrapper.cpp`, `src/storage/streaming_ingest_manager.cpp`, `src/storage/tiered_storage.cpp`, `src/storage/backup_manager.cpp`, and `src/storage/storage_engine.cpp`. In particular, the current roadmap records STO-01 and STO-10 as partially implemented and STO-07 as source-covered but not executed; production-mode checks are not proof that every module path or configuration is secure.
- **Test source and registration:** `tests/storage/CMakeLists.txt` globs `test_*.cpp` into focused targets and includes explicit WAL crash-test setup; the sources `test_wal_process_crash.cpp` and `test_streaming_ingest_rocksdb_failure.cpp` exercise acknowledged-write recovery boundaries and a later RocksDB WAL append failure with retry/reopen checks. `tests/integration/CMakeLists.txt` lists `test_storage_engine_soak`; its CMake notes identify the soak as using in-process stubs. These registrations and sources do not show that tests were built or run. The crash source does not cover kills within an active write/fsync call, backup/restore, or index consistency.
- **Benchmark boundary:** `benchmarks/storage/CMakeLists.txt` registers local benchmark targets only when benchmarks and Google Benchmark are available, and excludes `bench_crud.cpp`, `bench_mvcc.cpp`, and `bench_wal_stress.cpp` from its local loop to preserve top-level names. `bench_storage_release_gates.cpp` and `bench_storage_dedicated_gates.cpp` explicitly use in-memory mocks/stubs, with no real RocksDB or disk I/O. Their thresholds and registration are harness definitions, not measured repository results. Representative-hardware baselines, execution artifacts, and the STO-01 replay-regression comparison remain open.

### Research basis and interpretation

The `src/storage/RESEARCH.md` file records component and evidence boundaries; `research/implementation_influence/by_module.md` lists the LSM/RocksDB work as storage foundations and ARIES under transaction processing. These sources inform what to measure and validate; the literature does **not** establish ThemisDB results:

- Patrick O'Neil, Edward Cheng, Dieter Gawlick, and Elizabeth O'Neil, “The Log-Structured Merge-Tree (LSM-Tree)” (1996), DOI: [10.1007/BF00221852](https://doi.org/10.1007/BF00221852). Motivates measuring write/read/space-amplification trade-offs rather than treating compaction as a free optimization.
- Siying Dong et al., “Optimizing Space Amplification in RocksDB” (2017), USENIX FAST: [stable paper page](https://www.usenix.org/conference/fast17/technical-sessions/presentation/dong). Motivates recording workload, configuration, and space/compaction costs alongside throughput.
- Siying Dong et al., “Evolution of Development Priorities in Key-value Stores Serving Large-scale Applications: The RocksDB Experience” (FAST'21): [USENIX paper page](https://www.usenix.org/conference/fast21/presentation/dong). Its application-driven evolution motivates reassessing workload fit and priorities as use cases change; it is research guidance, not evidence that these workloads fit ThemisDB.
- Brian F. Cooper et al., “Benchmarking Cloud Serving Systems with YCSB” (SoCC'10), [DOI: 10.1145/1807128.1807152](https://doi.org/10.1145/1807128.1807152). Motivates workload-defined, comparable benchmarks; it does not supply ThemisDB measurements or validate its application fit.
- C. Mohan et al., “ARIES: A Transaction Recovery Method Supporting Fine-Granularity Locking and Partial Rollbacks Using Write-Ahead Logging” (1992), DOI: [10.1145/128765.128770](https://doi.org/10.1145/128765.128770). Informs recovery/failure-boundary validation.
- NIST SP 800-209, “Security Guidelines for Storage Infrastructure” ([DOI: 10.6028/NIST.SP.800-209](https://doi.org/10.6028/NIST.SP.800-209)). Provides security and operational-control guidance for the planned support matrix and recovery validation; it is not a local compliance assessment or evidence of implemented controls.
- RocksDB, “Basic Operations — Atomic Updates” (living documentation): [WriteBatch section](https://github.com/facebook/rocksdb/wiki/Basic-Operations#atomic-updates). Informs the atomic-batch contract to verify against the deployed integration.

These publications are research/standards guidance only. Local evidence remains limited to the source, test, and benchmark boundaries described above; targets and KPIs below are proposed repository acceptance gates, not claims from those publications or current benchmark readings.

### v2.5.0 priorities — P0: durability proof; P1: secure restore and cloud hardening

#### Wave 1 — v2.5.0 durability, replay, and ingest evidence (Priority P0)

- **Goal:** close the gap between implemented failure-path behavior and repeatable, auditable evidence for WAL recovery and streaming-ingest retries.
- **Tasks:**
  - [ ] Execute the registered WAL subprocess test; extend fault points to active write/fsync/rotation/checkpoint boundaries and assert acknowledged versus unacknowledged records after restart (Target: v2.5.0)
  - [ ] Add and execute recovery cases for backup/restore and index consistency, including malformed/truncated WAL and callback/I/O failures (Target: v2.5.0)
  - [ ] Run focused ingest retry/reopen coverage and publish per-run CI artifacts for the existing PR/nightly iteration gates (Target: v2.5.0)
  - [ ] Record a production-backed replay baseline and evaluate the existing ≤5% replay-regression gate (Target: v2.5.0)
- **Dependencies:** `WALStorage` recovery behavior; RocksDB and backup/PITR integration; POSIX subprocess CI plus platform-specific alternatives; stable baseline hardware and artifact retention.
- **Risks/trade-offs:** process-kill tests can be filesystem/platform-sensitive; expanded fault injection increases CI time. A fast mock replay number cannot substitute for durable-media results.
- **Acceptance/KPI:** exact acknowledged-record recovery and no acknowledged-record loss in every defined crash case; test artifacts from ≥50 PR runs and ≥1,000 nightly runs; production-backed replay regression ≤5% against a recorded, configuration-matched baseline. These are gates to achieve, not current results.
- **Validation strategy:** execute the focused registered tests on supported POSIX and non-POSIX paths as applicable; retain logs, configuration, and recovery assertions; run benchmark comparisons only on the same recorded production-backed profile. No validation is implied by source registration alone.
- **Effort estimate:** 4–6 engineer-weeks.
- **Assumptions/open knowledge gaps:** estimate assumes WAL and integration APIs remain stable. The subprocess source has not been executed in this update; exact filesystem matrix, artifact-retention implementation, backup/index fixture design, and the representative baseline host remain to be confirmed.

#### Wave 2 — v2.5.0 secure restore and cloud-backend hardening (Priority P1)

- **Goal:** make restore and remote-archive behavior explicit, integrity-checked, and fail-closed across supported providers and optional crypto/compression builds.
- **Tasks:**
  - [ ] Add restore tests proving unsupported compression/encryption, corrupt manifests, and checksum mismatch fail closed without modifying restored state (Target: v2.5.0)
  - [ ] Verify retry/backoff, checksums, and multipart boundaries for S3/GCS/Azure using provider-backed integration fixtures; retain provider/configuration evidence (Target: v2.5.0)
  - [ ] Test restore/key-rotation and authorization failure paths; document supported provider, crypto, and recovery combinations (Target: v2.5.0)
- **Dependencies:** `backup_manager.cpp`, blob backend adapters, manifest/checksum contract, test credentials or provider-compatible test environments, and security review.
- **Risks/trade-offs:** strict validation may reject legacy or incomplete archives; real cloud tests incur cost and can be flaky. Provider emulators do not prove provider-side durability.
- **Acceptance/KPI:** 100% of declared unsupported/corrupt restore cases fail closed; all declared supported restore combinations pass with integrity checks; complete the existing proposed 72-hour, 1,000-object-writes/s cloud soak with zero integrity errors and multipart coverage for a 1 GB object. No such result is claimed here.
- **Validation strategy:** focused failure-injection tests plus provider-backed restore/soak runs; compare restored manifest and payload checksums; review security logs and resulting filesystem state.
- **Effort estimate:** 5–8 engineer-weeks.
- **Assumptions/open knowledge gaps:** provider quotas, credential handling, archive compatibility policy, checksum algorithms, and whether CI can run a 72-hour provider-backed soak need owner confirmation.

### v3.0.0 priorities — P0: measured production envelopes; P1: explicit compatibility and operations contract

#### Wave 3 — v3.0.0 production-backed performance and mixed-load validation (Priority P0)

- **Goal:** replace mock-only gate readings with reproducible measurements of the real storage path under representative durability, compaction, and concurrent workload settings.
- **Tasks:**
  - [ ] Add production-backed WAL/RocksDB read, write, replay, checkpoint, and compaction benchmarks with documented fsync, cache, dataset, and concurrency settings (Target: v3.0.0)
  - [ ] Capture p50/p95/p99 latency, throughput, and space/write-amplification data on agreed representative storage profiles; publish raw results and environment metadata (Target: v3.0.0)
  - [ ] Add mixed foreground I/O plus maintenance/tiering soak coverage and compare results to the approved release envelope (Target: v3.0.0)
- **Dependencies:** Wave 1 correctness gates; production-backed benchmark fixtures; representative hardware/storage; approved workload definitions and performance budgets.
- **Risks/trade-offs:** real devices and cache state increase variance; durability settings affect throughput. Tighter p99 gates may trade throughput or resource cost for predictable tails.
- **Acceptance/KPI:** at least two agreed hardware/storage profiles have reproducible baselines and raw artifacts; no profile exceeds the approved latency/throughput regression budget, and WAL replay remains within the ≤5% comparison gate. Budgets must be set before results are evaluated; no performance value is asserted in this roadmap.
- **Validation strategy:** repeated benchmark runs with fixed configuration and warm/cold-cache labeling; compare same-profile distributions rather than mock thresholds; verify correctness and integrity checks during the mixed-load soak.
- **Effort estimate:** 6–10 engineer-weeks.
- **Assumptions/open knowledge gaps:** hardware profiles, dataset scale, release envelopes, confidence/repetition policy, and whether space/write amplification can be measured consistently across configurations are unresolved.

#### Wave 4 — v3.0.0 compatibility, security, and operability contract (Priority P1)

- **Goal:** define the supported storage deployment and recovery contract for a major release, with operator-visible failure diagnosis and tested compatibility boundaries.
- **Tasks:**
  - [ ] Publish a support matrix for RocksDB/backend versions, operating systems, filesystem/durability settings, optional crypto/compression features, and cloud providers (Target: v3.0.0)
  - [ ] Define and test backup/WAL compatibility, upgrade/rollback, and restore procedures for supported version transitions (Target: v3.0.0)
  - [ ] Link each operator-critical failure mode to structured diagnostics, alert/action guidance, and a validated recovery procedure (Target: v3.0.0)
- **Dependencies:** Wave 1–3 evidence, release/version policy, storage error diagnostics, provider/backend owners, and security/operations review.
- **Risks/trade-offs:** a narrower support matrix may constrain deployments; compatibility promises increase long-term test and maintenance burden.
- **Acceptance/KPI:** every declared supported matrix entry has a linked validation artifact; unsupported combinations are explicitly rejected or documented; 100% of operator-critical recovery scenarios have a reviewed runbook and corresponding test/evidence reference.
- **Validation strategy:** automated matrix/compatibility checks where feasible, upgrade-and-restore rehearsal for each declared transition, and operator review of runbook scenarios against emitted diagnostics.
- **Effort estimate:** 4–6 engineer-weeks.
- **Assumptions/open knowledge gaps:** final support owners, backward-compatibility duration, rollback guarantees, and required release artifact retention are not yet specified.

## Existing source surfaces and remaining validation

### Short-term (3-6 months)
- [x] `TieredStorageManager` declares `setPromotionListener()` and migration-cycle APIs; this records source presence, not verified application bootstrap wiring or cache-placement behavior.
- [x] make `BackupManager::decompressPath()` fail closed or perform real decompression when zstd/lz4 are unavailable; unsupported restore builds now reject the operation instead of copying raw bytes verbatim (Target: Q4 2026)
- [x] make `BackupManager::decryptFile()` fail closed or perform real decryption when OpenSSL is unavailable; unsupported restore builds now reject the operation instead of copying ciphertext verbatim (Target: Q4 2026)
- [~] wire the remaining `ggml_tensor_bridge.cpp` production injection seams (`GgmlAllocFn`, `PrefetchFn`, `TypeRegistrationFn`) at server initialization (Target: Q4 2026)
  - [x] `map(ctx, ...)` now forwards the live `ggml_context*` into `GgmlAllocFn` and still uses that context for real ggml allocation/copy when `THEMIS_HAS_GGML` is enabled, reducing reliance on the allocator stub path
  - [x] EmbeddedLLM startup now calls `registerGgmlTypeTT()` once under `THEMIS_ENABLE_GGML_BRIDGE`, closing the uninitialized TT-type registration path for ggml-backed inference startup
  - [x] fail-closed production guards added: `map()` invalidates when no allocator path exists, `prefetch()` throws when no prefetch backend is available, and `registerGgmlTypeTT()` throws when no registration backend exists
  - [ ] startup wiring of concrete allocator/prefetch/type-registration providers in server bootstrap remains open
- [x] tighten deterministic behavior under heavy WAL replay and compaction pressure (Target: Q4 2026)
  - Addressed by ST-SOAK-02 (WALReplayStability) and ST-STRESS-02 (ConcurrentCompactionStress)
- [x] expand stress coverage for blob/tiering and PITR edge scenarios (Target: Q4 2026)
  - Addressed by ST-STRESS-03 (BlobTieringEdgeCases) in tests/storage/test_storage_highcardinality_stress.cpp
- [x] improve operator-facing diagnostics for recovery and maintenance incidents (Target: Q4 2026)
  - Addressed by docs/operability/RUNBOOK_STORAGE_ENGINE.md (5 scenarios with structured log patterns)
- [x] finish remote cloud backup transport wiring in `backup_manager.cpp` for S3/GCS/Azure so cloud restore now reconstructs backups from a manifest plus payload blobs instead of depending on local-mirror-only behavior (Target: Q4 2026)

### Mid-term (6-12 months)
- [~] re-baseline p95/p99 envelopes for write/replay/recovery-sensitive paths (Target: Q1 2027)
- [~] broaden benchmark depth for mount-latency and storage allocator edge paths (Target: Q1 2027)
- [~] harden long-run reliability under sustained mixed read/write pressure (Target: Q1 2027)

### Distributed Maturity Phase 3 — Track 2 Items (Q3–Q4 2026)

These items are part of the next-phase **Track 2: Distributed Systems Maturity — 3.4 Storage** plan
(see `ROADMAP.md §Track 2`). Hard gate per item: deterministic under-load benchmark + `release_critical` CI green.

- [~] **Tiered storage validation:** `include/storage/tiered_storage.h` declares hot/warm/cold paths, age/access policies, a background worker, and migration statistics. The Q4 latency, non-blocking, and metric acceptance claims below require implementation-level tests and recorded results; do not treat them as established guarantees.
  - TODO: verify policy behavior and concurrent foreground I/O against current tests.
  - TODO: verify whether `storage_tier_migration_total` is exported; the header exposes an in-process `Stats` structure, not that named metric.
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
- [~] complete hardening for WAL/MVCC and backup/PITR internals (Target: Q4 2026)
  - [x] Make the WAL replay timeout configurable across all segments through `WALStorage::Config::recovery_timeout_seconds`; zero retains unbounded recovery. The file-backed timeout regression passed in the full 29-case WAL runtime suite.
  - [x] Fail closed when a persisted replication WAL payload cannot be deserialized; `WALManager` construction now aborts recovery rather than skipping the entry. `WALChecksumTest.InvalidSerializedEntryAbortsRecovery` passed in the separately linked replication runner; the standard CMake target remains blocked by the generated-gRPC compiler error (STO-01).
  - [x] Expose structured `WALStorage::RecoveryResult`, synchronous per-record progress snapshots, non-complete status audit events through `storage.audit`, and cumulative replay/terminal metrics through the process-wide `MetricsCollector`; the full 29-case WAL runtime suite passed (STO-01).
  - [x] Add a deterministic file-backed corruption regression with 10,000 seeded single-bit mutations; all cases failed closed in the standalone WAL runner.
  - [x] Add 1,024 seeded two-bit mutation cases within one WAL frame and verify corruption in segment 3 fails closed after reporting the two-entry valid prefix.
  - [x] Reject incomplete records in non-final WAL segments without truncating their data; add a deterministic regression for this boundary, 256 seeded single-bit cases, and 384 seeded two-bit cases across 12 segments containing payload lengths from 0 to 2048 bytes. The full 29-case Windows WAL runtime suite passes.
  - [ ] Add an explicit strict/skip-corrupted mode with quarantine (STO-01; requires human approval).
  - [x] `SecuritySignatureManager` now rejects null-backend production use instead of silently downgrading integrity persistence to an in-memory map
  - [x] remote cloud backup archive transport now uses provider blob backends plus a manifest-driven restore contract for S3/GCS/Azure
- [~] align tiered/blob/redundancy behavior to bounded runtime contracts (Target: Q4 2026)

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
- [x] Contract-hardening focused tests STR-01..STR-16 covering declared WAL durability, MVCC isolation, recovery, and PITR scenarios with mock fixtures (tests/storage/test_storage_contract_hardening_focused.cpp) (Target: Q4 2026). These cases do not prove real process-crash recovery.
- [x] Expand focused regressions for replay, PITR, and tiered/blob edge scenarios (Target: Q4 2026)
- [x] Extend deterministic stress fixtures for mixed write + maintenance workloads (Target: Q4 2026)
- [~] Add real process-termination evidence for `WALStorage`: `WALProcessCrashRecoveryTests` passed 50/50 CTest iterations, verifying exact replay of parent-observed fsynced ACKs; 44 iterations crossed segment rotation. This does not yet test interruption during an append/fsync, RocksDB/index consistency, checkpoint, or backup/restore (STO-07).
  - [x] Run 1,000 local ACK-boundary crashes with a structured JSON report: 1,000/1,000 passed, including 783 rotations in 272.695 seconds. CTest now produces `test-results/wal-process-crash.json`; report schema/replacement, failed-worker reporting, and report I/O failure have focused regressions.
  - [~] Existing PR Quality-Build and sanitizer-nightly callers now build the worker, run 50/1,000 ACK-boundary iterations, and declare secondary result artifacts that are uploaded even after failure unless cancelled. Successful GitHub execution/upload is still required; local scale results are not sanitizer/CI evidence (STO-07).
  - [~] Add `WALProcessUnobservedAppendTests`: release one append beyond the observed ACK prefix, then kill after a seeded delay without awaiting its ACK. Recovery must preserve the prefix and may contain exactly one additional fully validated PUT. CI requests 50/1,000 iterations per scenario and separate JSON reports. This does not prove a kill inside fsync/rotation or transaction rollback semantics.
    - [x] Local Windows run passed 1,000/1,000 iterations (764 rotations); 762 recovered the additional PUT and 238 only the acknowledged prefix. Both valid outcomes were observed, with no acknowledged entry lost.
  - [x] Verify repeat-recovery stability in both crash scenarios: two fresh verifier processes must validate the same replay prefix, and the second must preserve segment names/sizes/exact byte contents. Both 50-iteration CTests passed (100 crashes, 200 validated reopens); the initial implementation used SHA-256 checks. This proves repeated WAL replay stability, not idempotent application to a primary store.
  - [x] Enforce the first-recovery mutation boundary (2026-11-04): only a strictly shorter identical prefix of the numerically newest segment is permitted in the unobserved-append scenario; all older segments and the segment set remain unchanged. ACK-boundary recovery permits no byte changes. Both 50-iteration crash suites passed with the stronger check.
  - [x] Retain failed-iteration evidence when `--report` is supplied: unique report-adjacent directories preserve the pre-recovery WAL, current WAL, reproduction parameters and error; JSON links the artifact. Successful iterations still clean up. Both CI callers declare failure directories for secondary upload; actual GitHub publication remains unverified.
    - [x] Preserve combined writer stdout/stderr as UTF-8 `writer.log` after shutdown and reader join, including consumed ACKs and unconsumed diagnostic lines. Transcript I/O failure remains fatal and preserves the original writer error in the report. Both 50-iteration crash suites and 40 focused tests plus 14 subtests passed; a real failed iteration retained all 12 ordered ACKs.
    - [x] Prevent evidence loss on retention I/O failure: workspaces are created directly in the evidence root, so failed WALs need no post-failure copy. A failed `failure.txt` write preserves the workspace and reports both the original failure and `retention_error`. Both 50-iteration CTests and 37 focused tests plus 14 subtests passed; a real worker with a diagnostic filesystem failure retained both WAL states.

### Phase 5: Performance and Hardening
- [x] Initial header-bit campaign covered the low bit at each of 156 magic/sequence/type bytes; its report `wal-all-frame-header-corruption.json` passed 502 cases (80 successful reopens, 924 closed failures, 1,005 verifier processes, 120.286 s). The subsequent all-bit campaign below supersedes that mutation breadth for non-length fields. Length fields (13..20) remain a separate confirmed gap: all 64 bit flips across the eight bytes in the ACKed final frame yielded 61 `TruncatedTail` recoveries (11/12 entries and 117-to-78-byte truncation) plus three fail-closed CRC errors; see the blocked design item.
- [x] Expand all non-length protected bytes to all eight bit positions (2026-11-04): 2,832 single-bit faults across magic/sequence/type, payload, and CRC bytes of all 12 fixture frames failed closed twice without WAL mutation. `wal-all-protected-bits.json`: 2,980 total cases (40 tail prefixes, 108 older-segment tails, 2,832 bit faults), 80 successful reopens, 5,880 failed opens, 5,961 verifier processes, 826.929 s. Exact byte/bit sets per segment, three fingerprints, report status, and successful cleanup verified independently. 67 tests plus 40 subtests passed. The separate 64-bit length-field diagnostic still finds 61 destructive tail truncations; this campaign does not claim length-field safety.
- [?] Resolve WAL length-field integrity before claiming STO-01 recovery is fail-closed (Target: M1 · 2026-12-15): the current frame CRC is checked only after declared lengths fit in the remaining file, so an inflated key/value length is treated as an incomplete newest tail. Define a versioned frame/commit-boundary design and migration/rollback policy with maintainer approval; tests must prove that a fully persisted ACKed frame with any length-byte corruption is not dropped while a genuine interrupted append remains recoverable. Do not add a silent compatibility path.
- [x] Cover payload/CRC corruption in all twelve fixture frames, not only the final frame: flip the low bit once at each of 198 byte positions, and reject each fault twice without WAL mutation. Reports and reproduction metadata include absolute `segment_byte_offset`; older-segment fixture lengths are explicitly guarded. Local `wal-all-frame-corruption.json` passed 346 cases (80 successful reopens, 612 closed failures, 693 verifier processes, 83.410 s); exact per-frame offset sets, fingerprints and cleanup verified independently. 67 focused tests plus 39 subtests and Python syntax checks passed (2026-11-04). Historical reports unchanged; this is not all-bit/header corruption or targeted syscall evidence.
- [x] Preserve the injected fault state in `before-recovery/` before any boundary-campaign verifier; retain it alongside current `wal/` on failure. Snapshot I/O failure stops before recovery and retains the original WAL. Regression tests reproduced missing evidence on both positive and negative recovery paths. All 166 real-worker cases passed again (`wal-tail-forensic-retention.json`, 80 successful reopens, 252 closed failures, 333 processes, 40.822 s); 66 focused tests plus 37 subtests passed (2026-11-04). A diagnostic real-worker count mismatch retained the original 79-byte tail and repaired 78-byte tail with explicit failed status; it is retention evidence, not a production recovery defect.
- [x] Extend the checked-in tail campaign to all three older segments: 36 incomplete-frame cuts per segment, with `affected_segment` in results and reproduction metadata. Local report `build-msvc-windows-debug/test-results/wal-tail-all-segments.json` passed 166 cases (80 successful reopens, 252 closed failures, 333 verifier processes, 43.566 s). Every older-segment failure preserved WAL bytes on both opens; 38 partial latest tails were repaired. Exact fault sets and existing contracts passed 64 focused tests plus 35 subtests (2026-11-04). Existing CI callers run the expanded script; GitHub execution and syscall/primary-store evidence remain open.
- [x] Check in `tests/storage/test_wal_tail_boundaries.py` and wire its 94 deterministic disk-fault cases into existing PR/nightly callers with JSON and failure-directory uploads. Local execution passed (80 successful reopens, 108 closed failures, 189 verifier processes, 21.146 s); 64 focused tests plus 35 subtests, actionlint and Bash syntax checks passed (2026-11-04). No production C++ or CMake changes; actual GitHub execution remains open.
- [x] Exhaustive local disk-fault campaign after a real writer kill (2026-11-04): all 40 byte-prefix lengths of the final 39-byte frame passed exact prefix/repeat-replay checks; all 18 payload/CRC byte bit-flips and 36 incomplete older-segment tails failed closed without mutation on two attempts. Evidence: `build-msvc-windows-debug/test-results/wal-tail-boundaries-20261104.json` (94 cases, 80 successful reopens, 108 failed opens, 189 verifier processes including one count probe, 31.203 s). This ad-hoc campaign is not CI-registered or a syscall-kill test; the fixture ACKed 12 writes before deliberate disk mutation, which tests preservation of entries 1..11 rather than ACK durability under process kill alone.
- [x] Checkpoint the JSON report after every validated iteration, retaining `running` until final checks pass. A forced parent termination preserves the last checkpoint; progress-write errors stop subsequent iterations. Both 50-iteration crash suites and 60 focused tests plus 33 subtests passed (2026-11-04). The parent-termination regression uses test-only iteration results to verify reporting, not additional WAL durability.
- [x] Preserve verifier stdout/stderr on 30-second subprocess timeouts in both replay attempts, with explicit expected count and chained timeout cause. Timeout never triggers N+1 tolerance. Both 50-iteration crash suites and 57 focused tests plus 33 subtests passed (2026-11-04); a real subprocess timeout preserved both flushed output channels.
- [x] Reject recognized sanitizer diagnostics independently of exit status in the writer and both verifier attempts, including writer output after the final ACK. Both 50-iteration crash suites and 53 focused tests plus 30 subtests passed (2026-11-04). Tests verify diagnostic text detection, not execution of instrumented sanitizer binaries; scheduled sanitizer evidence remains open.
- [x] Bind new crash reports to worker and harness file SHA-256 fingerprints; fail on fingerprint I/O errors or differences at run end. Both 50-iteration crash suites and 46 focused tests plus 16 subtests passed (2026-11-04). This does not identify loaded dependency DLLs or detect changes reverted between checks; older scale reports remain historical, without retroactive fingerprints.
- [x] Revalidate the fully hardened harness at nightly-sized local scale (2026-11-04): both scenarios passed 1,000 crashes each with two verified reopens and first/repeat byte checks (4,000 reopens total). ACK-boundary: 783 rotations, no byte changes, 378.079 s. Unobserved append: 758 rotations, 635 additional PUTs, 365 prefix-only recoveries, 174 permitted first tail repairs, 560.185 s. Separate `*-hardened-1000.json` reports preserve this evidence without replacing older reports; both evidence roots are empty after successful cleanup. This is Windows local evidence, not sanitizer/GitHub execution.
- [x] Propagate writer pipe/read decoding failures through the ACK channel and check unconsumed errors after reader join. Errors after the last ACK must fail before recovery, with the partial writer transcript retained. Both 50-iteration crash suites and 42 focused tests plus 16 subtests passed (2026-11-04).
- [~] Sanitizer-nightly now disables recoverable sanitizer errors with `-fno-sanitize-recover=all` for both C/C++ and both modes. Contract tests and executed Bash flag selection pass; sanitizer runtime execution remains unverified. The latest inspected develop run (41760710588) failed compiling sharding before validation.
- [x] Add SGRG-01..SGRG-06 benchmark harnesses in `benchmarks/storage/bench_storage_release_gates.cpp`. The harness uses in-memory mocks and contains target labels; it is not evidence that production storage met these thresholds.
- [!] Representative-hardware p95/p99 and throughput comparison against a release baseline is not evidenced by the benchmark source or CMake registration; TODO: link executed results before marking validated.

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
- [~] remaining hardening tasks closed for durability/recovery edge paths
- [x] benchmark gate harness sources and CMake registration exist; measured release-baseline stabilization is not established by this checklist
  - Wave D dedicated gates ST-BM-01..04 delivered in `benchmarks/storage/bench_storage_dedicated_gates.cpp`

## Known Issues and Limitations

- runtime behavior depends on storage configuration, backend profile, and workload shape.
- selected replay/recovery/tiering edge scenarios need continued hardening.
- benchmark depth should continue expanding for advanced storage workloads.
- provider-native remote transport now depends on the linked blob backend plus credentials/runtime environment; multipart/retry soak hardening remains follow-up work.

## Breaking Changes

No breaking storage contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `storage`
- [x] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027)
  - Delivered: `tests/storage/test_storage_highcardinality_stress.cpp` (ST-STRESS-01..03: 100k-key write, concurrent compaction, blob tiering edge cases)
  - Delivered: `benchmarks/storage/bench_storage_dedicated_gates.cpp` (ST-BM-01..04: write p95, read p95, WAL-replay throughput, compaction trigger p95)
- [x] Contribute to or validate long-duration soak test coverage for this module's primary paths (Target: Q1 2027)
  - Delivered: `tests/integration/test_storage_engine_soak.cpp` (ST-SOAK-01..03: write throughput ≥10k w/s, WAL replay stability, compaction reliability)
- [x] Ensure runbook coverage for operator-critical scenarios in this module (Target: Q1 2027)
  - Delivered: `docs/operability/RUNBOOK_STORAGE_ENGINE.md` (5 scenarios: WAL corruption, compaction stall, blob tier mount failure, PITR restore failure, write stall)

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [~] This module's distributed/acceleration paths fail closed (Target: Q1 2027)
- [~] Benchmark harness sources exist; benchmark-backed p95/p99 baselines on representative hardware are not evidenced (Target: Q1 2027)
  - `benchmarks/storage/bench_storage_dedicated_gates.cpp` ST-BM-01..04 is a harness, not a result artifact. The STO-01 ≤5% replay-regression comparison against a recorded baseline remains open.
- [x] Operator-critical paths have diagnostics, alerts, and runbooks (Target: Q1 2027)
  - Delivered: `docs/operability/RUNBOOK_STORAGE_ENGINE.md`
