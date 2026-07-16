### Context

This issue implements the roadmap item 'ExporterFactory Stub Replacement' for the analytics domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: 1 · ExporterFactory Stub Replacement

### Goal

Deliver the scoped changes for ExporterFactory Stub Replacement in src/analytics/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### 1 · ExporterFactory Stub Replacement
**Priority:** High
**Target Version:** v1.8.0
**Files:** `src/analytics/analytics_export.cpp` lines 728–734

`ExporterFactory::createExporter(ExportFormat)` and `createDefaultExporter()` both return
`std::make_unique<StubAnalyticsExporter>()` unconditionally.  The comment on line 728 reads
*"For now, return stub exporter for all formats – In the future, this would return
format-specific exporters"*.  The `StubAnalyticsExporter` class itself (line 203) delegates
to `exportToFileArrow()` only when `THEMIS_HAS_ARROW` is set, and for all three Arrow
formats falls through to a `NOT_SUPPORTED` status when Arrow is absent, but the factory
never instantiates any specialised class regardless.

**Implementation Notes:**
- `[ ]` Introduce `ArrowIPCExporter`, `ParquetExporter`, and `FeatherExporter` classes that wrap the existing `exportToFileArrow()` logic – remove dead `StubAnalyticsExporter` wrapper
- `[ ]` Rename `StubAnalyticsExporter` to `JSONCSVExporter` to reflect its actual capability scope
- `[ ]` `createExporter(ExportFormat)` must switch on `format` and return the correct concrete type; formats unavailable without Arrow must return `std::unexpected` / throw `std::runtime_error` with a clear message instead of silently returning the fallback
- `[ ]` Add unit test that asserts `createExporter(ExportFormat::FMT_ARROW_PARQUET)` returns a non-stub type when `THEMIS_HAS_ARROW` is defined
- `[ ]` Suppress the `6 Stubs` annotation in the file header once all stubs are promoted to real implementations

**Performance Targets:**
- Parquet export of 1 M rows: ≤ 2 s wall time with snappy compression on a single core
- CSV export of 1 M rows: ≤ 500 ms (streaming write, no full in-memory serialization)

---

### Acceptance Criteria

- [ ] Introduce `ArrowIPCExporter`, `ParquetExporter`, and `FeatherExporter` classes that wrap the existing `exportToFileArrow()` logic – remove dead `StubAnalyticsExporter` wrapper
- [ ] Rename `StubAnalyticsExporter` to `JSONCSVExporter` to reflect its actual capability scope
- [ ] `createExporter(ExportFormat)` must switch on `format` and return the correct concrete type; formats unavailable without Arrow must return `std::unexpected` / throw `std::runtime_error` with a clear message instead of silently returning the fallback
- [ ] Add unit test that asserts `createExporter(ExportFormat::FMT_ARROW_PARQUET)` returns a non-stub type when `THEMIS_HAS_ARROW` is defined
- [ ] Suppress the `6 Stubs` annotation in the file header once all stubs are promoted to real implementations
- [ ] Parquet export of 1 M rows: ≤ 2 s wall time with snappy compression on a single core
- [ ] CSV export of 1 M rows: ≤ 500 ms (streaming write, no full in-memory serialization)

### Relationships

- Roadmap row: #40 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/analytics/FUTURE_ENHANCEMENTS.md#1--exporterfactory-stub-replacement
- Source key: roadmap:40:analytics:v1.8.0:1-exporterfactory-stub-replacement

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:40:analytics:v1.8.0:1-exporterfactory-stub-replacement -->
<!-- roadmap-ref: row=40;module=analytics;target=v1.8.0 -->
<!-- roadmap-detail: src/analytics/FUTURE_ENHANCEMENTS.md#1--exporterfactory-stub-replacement -->
