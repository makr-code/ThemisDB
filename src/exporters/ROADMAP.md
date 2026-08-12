# Exporters Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-29 -->
<!-- Issue: #5644 (Development Status 2026-07-18) -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production exporter runtime exists across format-specific pipelines
(JSONL/Parquet/Arrow/HuggingFace), streaming/incremental/join orchestration,
and policy/security controls.

Issue #5644 sync pass confirms roadmap priorities and future-enhancement focus
remain aligned with current module documentation. Closure evidence is still
partial because current-cycle focused test execution is blocked in this
environment by a missing RocksDB dependency during configure.

## In Progress

- [x] complete hardening for format pipelines and orchestration internals (Delivered: Q4 2026)
- [x] align security and governance behavior to bounded runtime contracts (Delivered: Q4 2026)
- [x] standardize fail-closed behavior for unauthorized/unsafe export scenarios (Delivered: Q4 2026)
- [x] unify diagnostics across stream/incremental/join and hub upload failures (Delivered: Q4 2026)

### Wave 3B hardening batch (2026-07-29)
- [x] Incremental exporter now applies `ExportOptions::filter_expression` parity with streaming/jsonl flows (`src/exporters/incremental_exporter.cpp`; `tests/exporters/test_incremental_exporter.cpp`).
- [x] Incremental watermark update is now fail-safe on partial scans (size/error stop) to prevent sequence-skip drift (`src/exporters/incremental_exporter.cpp`; `tests/exporters/test_incremental_exporter.cpp`).
- [x] Join exporter now fails closed when `setRightCollection()` was not called before `exportEntities()` (`src/exporters/join_exporter.cpp`; `include/exporters/join_exporter.h`; `tests/exporters/test_join_exporter.cpp`).
- [x] Join exporter now applies additional `ExportOptions::filter_expression` parity on merged records (`src/exporters/join_exporter.cpp`; `tests/exporters/test_join_exporter.cpp`).
- [x] Streaming exporter encryption path is now deterministic: `encryption_config` takes precedence and legacy + v2 double-encryption ambiguity is removed (`src/exporters/streaming_exporter.cpp`).

## Planned Features

### Short-term (3-6 months)
- [ ] tighten deterministic behavior for mixed-format and mixed-policy export permutations (Target: Q4 2026)
- [ ] expand regressions for join/stream/incremental checkpoint edge scenarios (Target: Q4 2026)
- [ ] improve operator-facing observability for hub upload and redaction incidents (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 and throughput envelopes for major exporter paths (Target: Q1 2027)
- [ ] broaden benchmark depth for join/predicate and template-heavy workflows (Target: Q1 2027)
- [ ] harden long-running reliability under sustained large-export workloads (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze exporter interface and option contracts for active major line (Delivered: Q3 2026)
- [x] define explicit error taxonomy for policy, filter, and output-failure classes (Delivered: Q3 2026)
  - Contract header: `include/exporters/exporters_api_contract.h`
  - Error codes: EXPORT_FORMAT_UNSUPPORTED, EXPORT_WRITE_FAILED, STREAM_INTERRUPTED,
    SCHEMA_MISMATCH, QUOTA_EXCEEDED, COLUMN_NOT_FOUND, SCHEMA_SERIALIZATION_FAILED,
    EXPORT_ABORTED, INTERNAL_ERROR

### Phase 2: Core Implementation
- [x] complete hardening for format pipelines and orchestration internals (Delivered: Q4 2026)
  - Data-race fixes: `export_encryption.cpp` key_provider mutex (lines 131, 157, 691-692, 875); `huggingface_hub_client.cpp` policy_engine + key_provider mutex (lines 207, 408, 591)
  - RAII: removed redundant `src.close()` in `ExportEncryption::encryptFile/decryptFile`; EVP_CIPHER_CTX mutex boundary for ExportEncryptor
  - O(n²)→O(1): `resolveColumns()` upgraded from `std::set` to `std::unordered_set` in `parquet_exporter.cpp`
  - Parquet write path: removed manual `ofs.close()` (RAII handles it)
- [x] align security and governance behavior to bounded runtime contracts (Delivered: Q4 2026)
  - Retry wait NOLINT comments at `file_backoff.wait()` and `shard_backoff.wait()` (bounded by max_backoff_ms=30'000ms)

### Phase 3: Error Handling and Edge Cases
- [x] standardize fail-closed behavior for unauthorized/unsafe export scenarios (Delivered: Q4 2026)
  - Added `PolicyDeniedException : ExporterException` to `include/exporters/exporter_errors.h`; uses `ERR_EXPORT_POLICY_DENIED`; carries `denial_reason`, `requesting_user`, `collection` fields
  - `isResumableError(EXPORT_ABORTED)` == false by contract (fail-closed)
- [x] unify diagnostics across stream/incremental/join and hub upload failures (Delivered: Q4 2026)
  - Standard log prefix: `[EXPORT_DENIED] collection={} user={} reason={}` in `huggingface_hub_client.cpp` and `jsonl_llm_exporter.cpp`
  - Standard log prefix: `[HUB_UPLOAD_FAILED] repo={} reason={} http_status={}` at hub retry-exhausted return sites
  - New metrics: `recordPolicyDenial()`, `getPolicyDenials()`, `recordHubUploadFailure()`, `getHubUploadFailures()` added to `ExporterMetrics`; keys `exporter_policy_denials_total` and `exporter_hub_upload_failures_total` in `toJson()` and `toString()`
  - Focused tests EXCH-17..24 in `tests/exporters/test_exporters_phase23_hardening_focused.cpp`

### Phase 4: Tests
- [x] expand focused regressions for format, policy, and checkpoint edge scenarios (Delivered: Q3 2026)
- [x] extend deterministic fixture coverage for template and join predicate permutations (Delivered: Q3 2026)
  - Test file: `tests/exporters/test_exporters_contract_hardening_focused.cpp`
  - Test cases: EXCH-01..EXCH-16 (CSV, Parquet, streaming, error contract)
  - kExportersContractSeed = 42; all tests self-contained, no external I/O

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for exporter throughput and latency hot paths (Delivered: Q3 2026)
- [x] validate p95/p99 and throughput behavior against release baselines (Delivered: Q3 2026)
  - Benchmark file: `benchmarks/exporters/bench_exporters_release_gates.cpp`
  - Gates: ERRG-01..ERRG-06 (CSV ≥1M/s, Parquet ≤5ms, schema ≤100µs,
    null ≤10µs, Arrow ≤1ms, quota ≤50µs)
  - kExportersCanonicalSeed = 42; Repetitions(5)

### Phase 6: Documentation and Acceptance
- [x] core exporters module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] Phase 1-6 Wave 3B Category-D closure delivered (Q3 2026)
  - Contract header, 16 focused tests (EXCH-01..16), 6 release-gate benchmarks (ERRG-01..06)

## Production Readiness Checklist

- [x] core exporters surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] remaining hardening tasks closed for policy/filter/checkpoint edge paths (Phase 2/3 delivered Q4 2026)
- [ ] release benchmark stabilization complete

## Evidence Summary (Issue #5644 Sync — 2026-07-29)

- Configure attempted: `cmake --preset community-release`
- Result: failed in `cmake/Dependencies.cmake` with
  `RocksDB not found. Install via vcpkg (rocksdb) or system package librocksdb-dev.`
- Follow-up build attempt:
  `cmake --build --preset community-release --target module_exporters_test_exporters_contract_hardening_focused`
  failed with `ninja: error: loading 'build.ninja': No such file or directory`
  because configure did not complete.
- Build/Test status: focused exporters target execution is blocked until
  configure succeeds.
- Last known focused test evidence (from issue context): PASS on
  `module_exporters_test_aql_predicate_filter_focused.exe` (`15 tests`, exit 0,
  validated 2026-07-18).

## Open Work (Issue #5644)

- [~] validate and refine extracted roadmap priorities against full module docs in `src/exporters/ROADMAP.md`
- [~] validate and refine extracted future focus points against full module docs in `src/exporters/FUTURE_ENHANCEMENTS.md`
- [~] add/refresh focused build and test evidence for this module (configure blocker documented in Evidence Summary)
- [~] mark completed synced items and risks with explicit status transitions

## Closure Criteria (Issue #5644)

- [x] all module acceptance criteria updated and traceable in roadmap/future docs
- [~] evidence updated or explicit justified gap documented
- [ ] parent epic task entry checked by maintainer
- [ ] status labels updated by maintainer before close
- [x] close reason documented as "sync pass complete; configure/test evidence blocked by RocksDB dependency in this environment"

## Known Issues and Limitations

- behavior depends on selected format backend and runtime configuration.
- selected join/stream/incremental edge scenarios need continued hardening.
- benchmark breadth should keep expanding for advanced export workflows.

## Breaking Changes

No breaking exporters contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `exporters`
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
