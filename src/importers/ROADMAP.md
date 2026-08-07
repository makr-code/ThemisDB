# Importers Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-08-02 | Evidence: build/test verified via #5650 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · BUILD_STATUS.md -->

## Current Status

Production importer runtime exists across relational/document/stream/file/object ingestion paths, schema/conflict/quality handling, and auditable post-processing support.

## In Progress

- [x] benchmark stabilization for importer throughput and conflict-resolution hot paths (Delivered: Q3 2026)
- [x] final integration testing for Phase 2-3 hardening delivery (Delivered: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] tighten deterministic behavior under high-volume multi-connector ingestion loads (Target: Q4 2026)
- [ ] extend stress coverage for mixed schema drift and conflict strategy scenarios (Target: Q4 2026)
- [ ] improve operator-facing diagnostics for connector and validation failure incidents (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for parser/import/conflict pathways (Target: Q1 2027)
- [ ] broaden benchmark depth for CDC, stream, and quality/audit-intensive workflows (Target: Q1 2027)
- [ ] harden long-running reliability under sustained ingest pressure (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze connector/schema/conflict/audit contracts for active major line (Delivered: Q3 2026)
- [x] define explicit error taxonomy for validation, conflict, and connector capability classes (Delivered: Q3 2026)
  - Contract header: `include/importers/importers_api_contract.h`
  - Error codes: IMPORT_SCHEMA_MISMATCH, IMPORT_ROW_INVALID, IMPORT_DUPLICATE_KEY,
    IMPORT_FILE_NOT_FOUND, IMPORT_QUOTA_EXCEEDED, IMPORT_DUPLICATE_ID, IMPORT_TIMEOUT,
    IMPORT_CONNECTOR_UNAVAILABLE, IMPORT_ROLLBACK, INTERNAL_ERROR

### Phase 2: Core Implementation
- [x] complete hardening for connector import and schema/validation internals (Delivered: Q3 2026)
  - T2.1 Part 1: PostgreSQL connector hardening (commit 01e331e7)
  - T2.1 Part 2: MySQL/Oracle/SQLite connector hardening (commit 5646694c)
  - T2.1 Part 3/4: MongoDB/Kafka/S3 connector hardening (Q3 2026)
  - T2.2: Schema inference and validation hardening (commit 6d5e118c)
- [x] align conflict/quality/audit behavior to bounded runtime contracts (Delivered: Q3 2026)
  - T2.3: Conflict resolution determinism, quality scoring bounds, audit trail integration (commit fe2f4cfd)

### Phase 3: Error Handling and Edge Cases
- [x] standardize fail-safe behavior for unsupported connector and malformed schema scenarios (Delivered: Q3 2026)
  - T3.1.1: Connector capability fallback chain
  - T3.1.2: Malformed schema detection & degradation (STRICT/LENIENT/AUTO_REPAIR levels)
  - T3.1.3: Rollback & recovery audit trail with deterministic replay
- [x] unify diagnostics across schema, conflict, and capability failure incidents (Delivered: Q3 2026)
  - T3.2.1: Structured failure diagnostics (5 categories, root cause + remediation)
  - T3.2.2: Diagnostic aggregation & reporting with top 5 root causes
  - Commit: 8e36c2e6fe

### Phase 4: Tests
- [x] expand focused regressions for mixed connector/schema/conflict edge scenarios (Delivered: Q3 2026)
- [x] extend deterministic stress fixtures for high-throughput import workloads (Delivered: Q3 2026)
  - Test file: `tests/importers/test_importers_contract_hardening_focused.cpp`
  - Test cases: IMCH-01..IMCH-16 (idempotency, schema evolution, error handling, large import)
  - kImportersContractSeed = 42; all tests self-contained, no external I/O

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for importer hot paths (Delivered: Q3 2026)
- [x] validate p95/p99 and throughput behavior against release baselines (Delivered: Q3 2026)
  - Benchmark file: `benchmarks/importers/bench_importers_release_gates.cpp`
  - Gates: IMRG-01..IMRG-06 (CSV parse ≥5M/s, schema ≤50µs, dedup ≤100µs,
    commit ≤5ms, quota ≤50µs, schema-evolution ≤200µs)
  - kImportersCanonicalSeed = 42; Repetitions(5)

### Phase 6: Documentation and Acceptance
- [x] core importers module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] Phase 1-6 Wave 3B Category-D closure delivered (Q3 2026)
  - Contract header, 16 focused tests (IMCH-01..16), 6 release-gate benchmarks (IMRG-01..06)

## Production Readiness Checklist

- [x] core importer surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] remaining hardening tasks closed for connector/validation/conflict edge paths (Delivered: 2026-08-07)
- [x] release benchmark stabilization complete (Delivered: 2026-08-07)

## Build and Test Evidence

### Focused Test Inventory (Verified 2026-08-02)
- Test files: 6 source files in `tests/importers/test_*.cpp`
- Registered targets: `module_importers_<stem>_focused` (auto-discovered via glob)
- Test registration: `themis_register_module_focused_test()` in tests/importers/CMakeLists.txt
- IMCH test cases: 16 focused unit tests (IMCH-01..IMCH-16) in test_importers_contract_hardening_focused.cpp
  - Idempotency contract validation (IMCH-01..04)
  - Schema evolution handling (IMCH-05..08)
  - Error handling and rollback (IMCH-09..12)
  - Large import and edge cases (IMCH-13..16)
- Tier: unit, Timeout: 120s per test, Label: `importers phase4`

### Benchmark Inventory (Verified 2026-08-02)
- Benchmark file: `benchmarks/importers/bench_importers_release_gates.cpp`
- Registered target: `bench_importers_release_gates` via themis_add_standard_benchmark()
- Release gates (IMRG-01..IMRG-06):
  - IMRG-01: CSV parse ≥5M rows/s (GATE-IMRG-01)
  - IMRG-02: Schema validation p99 ≤50µs (GATE-IMRG-02)
  - IMRG-03: Dedup key check p99 ≤100µs (GATE-IMRG-03)
  - IMRG-04: Row commit p99 ≤5ms RT (GATE-IMRG-04)
  - IMRG-05: Import quota p99 ≤50µs (GATE-IMRG-05)
  - IMRG-06: Schema evolution p99 ≤200µs (GATE-IMRG-06)
- Seed: kImportersCanonicalSeed = 42; Repetitions(5), WarmupIterations(200)

### Build Configuration Status
- Preset: `community-release-allow-missing-rocksdb` (vcpkg fallback for Linux system packages)
- Test framework: GoogleTest/GTest (auto-discovery via find_package)
- Benchmark framework: google-benchmark
- Configuration verified: 2026-08-02 (cmake --preset community-release-allow-missing-rocksdb successful)
- Build targets verified: test and benchmark registration confirmed in CMake configuration

## Known Issues and Limitations

- runtime behavior depends on connector availability and build/runtime feature flags.
- Q3 2026 Phase 2-3 hardening completed (commit log: 01e331e7, 5646694c, 6d5e118c, fe2f4cfd, 8e36c2e6).
  - Phase 2 T2.1: Connector pooling/timeout/fallback/error mapping (4 parts)
  - Phase 2 T2.2: Schema bounds/cycles/validation levels
  - Phase 2 T2.3: Conflict determinism/quality scoring/audit schema
  - Phase 3 T3.1: Capability fallback/schema degradation/rollback recovery
  - Phase 3 T3.2: Structured diagnostics/aggregation
- 2026-08-07 Phase 2-3 Closure deliverables:
  - Phase 2-3 integration test suite: `tests/importers/test_importers_phase2_phase3_integration_focused.cpp` (IMPI-01..16)
  - Benchmark stabilization report: `benchmarks/importers/PHASE2_3_STABILIZATION.md`
  - All six release gates (IMRG-01..06) validated and stabilized with <3% variance
  - CI gate thresholds established and reproducible across environments
- benchmark breadth should continue expanding for advanced ingest workflows.

## Breaking Changes

No breaking importer contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.