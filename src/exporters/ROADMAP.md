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

- [~] hardening policy/filter parity and edge behavior across all exporter variants (Target: Q3 2026)
- [~] benchmark stabilization for export throughput and delta/stream hot paths (Target: Q3 2026)
- [~] diagnostics consistency improvements for policy denial and export failure classes (Target: Q3 2026)

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
- [ ] complete hardening for format pipelines and orchestration internals (Target: Q4 2026)
- [ ] align security and governance behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-closed behavior for unauthorized/unsafe export scenarios (Target: Q4 2026)
- [ ] unify diagnostics across stream/incremental/join and hub upload failures (Target: Q4 2026)

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
- [ ] remaining hardening tasks closed for policy/filter/checkpoint edge paths
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

- [x] validate and refine extracted roadmap priorities against full module docs in `src/exporters/ROADMAP.md`
- [x] validate and refine extracted future focus points against full module docs in `src/exporters/FUTURE_ENHANCEMENTS.md`
- [~] add/refresh focused build and test evidence for this module (configure blocker documented in Evidence Summary)
- [x] mark completed synced items and risks with explicit status transitions

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