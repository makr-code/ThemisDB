# Exporter Plugins – Roadmap

## Current Status

**Status:** ✅ Production-ready

| Exporter | Implementation | Status |
|----------|---------------|--------|
| JSONL LLM Exporter | `src/exporters/jsonl_llm_exporter.cpp` | ✅ Production |

Entry-point: `plugins/exporters/` (plugin manifests) · implementation in `src/exporters/`

---

## In Progress

- [~] Integration test that round-trips data through the JSONL exporter and verifies output schema
- [~] Export progress metric (documents exported / total)

## Planned Features

- [ ] **Parquet exporter** – export ThemisDB collections to Apache Parquet (Target: Q3 2026)
- [ ] **CSV exporter** – flat-file export for spreadsheet tooling (Target: Q3 2026)
- [ ] **Arrow IPC exporter** – zero-copy integration with data-science frameworks (Target: Q3 2026)
- [ ] Streaming export mode for large collections (> 10 M records) (Target: Q3 2026)
- [ ] **Delta Lake / Iceberg exporter** (Target: Q4 2026)
- [ ] **GraphML exporter** – export graph collections (Target: Q4 2026)
- [ ] Plugin-based exporter registry for third-party exporters (Target: 2027)
- [ ] Export scheduling (cron-style) via ThemisDB job system (Target: 2027)

---

## Short-term Goals (next 1–2 sprints)

- [ ] Add integration test that round-trips data through the JSONL exporter and verifies output schema.
- [ ] Document all configuration options for `jsonl_llm_exporter` in `exporters/README.md`.
- [ ] Expose export progress as a metric (documents exported / total).

## Mid-term Goals (1–3 months)

- [ ] **Parquet exporter** – export ThemisDB collections to Apache Parquet for analytics pipelines.
- [ ] **CSV exporter** – simple flat-file export for spreadsheet tooling.
- [ ] **Arrow IPC exporter** – zero-copy integration with data-science frameworks.
- [ ] Streaming export mode: write output incrementally for large collections (> 10 M records).

## Long-term Goals (3–12 months)

- [ ] **Delta Lake / Iceberg exporter** – write to open table formats for data lakehouse integration.
- [ ] **GraphML exporter** – export graph collections to GraphML / Cypher.
- [ ] Plugin-based exporter registry: allow third-party exporters loaded at runtime.
- [ ] Export scheduling (cron-style) via ThemisDB job system.

## Milestones

| Milestone | Target | Status |
|-----------|--------|--------|
| Parquet exporter MVP | TODO | 🔲 Planned |
| Streaming export | TODO | 🔲 Planned |
| GraphML exporter | TODO | 🔲 Planned |

## Implementation Phases

### Phase 1 – Parquet & CSV Exporters
- [ ] Add Apache Arrow / Parquet C++ library to `vcpkg.json`
- [ ] Implement `ParquetExporter` writing Arrow record batches from ThemisDB query results
- [ ] Implement `CSVExporter` with configurable delimiter, quoting, and header options
- [ ] Unit tests: schema fidelity, null handling, large-column encoding

### Phase 2 – Arrow IPC & Streaming Export
- [ ] Implement `ArrowIPCExporter` using Arrow IPC stream format
- [ ] Streaming export: cursor-based pagination writing output incrementally
- [ ] Back-pressure handling: pause export when downstream consumer is slow
- [ ] Integration test: 10 M record export round-trip without OOM

### Phase 3 – Delta Lake / Iceberg
- [ ] Evaluate `delta-rs` or Apache Iceberg C++ / Python bridge
- [ ] Implement `DeltaLakeExporter` writing Parquet + transaction log
- [ ] Schema evolution handling (add/remove columns across export runs)

### Phase 4 – Plugin Registry & Scheduling
- [ ] Runtime-loadable exporter registry (`IExporterPlugin` interface)
- [ ] Cron-style export scheduling via ThemisDB job system
- [ ] GraphML exporter for graph collections
- [ ] Performance benchmarks: throughput (records/sec) per exporter type

---

## Dependencies

- `src/exporters/jsonl_llm_exporter.cpp`
- Apache Arrow / Parquet libraries (to be added to vcpkg)
- ThemisDB `QueryEngine` + AQL

## Open Questions

- [ ] Should exporters be synchronous or always async/job-based?
- [ ] What is the target throughput for streaming export (records/sec)?

---

## Production Readiness Checklist

| Item | Status |
|------|--------|
| JSONL LLM exporter | ✅ Ready |
| Integration test (JSONL round-trip) | ❌ Pending |
| Parquet exporter | ❌ Not implemented |
| Streaming export (> 10 M records) | ❌ Not implemented |
| Delta Lake / Iceberg exporter | ❌ Not implemented |
| Apache Arrow / Parquet in `vcpkg.json` | ❌ Not added |
| Export progress metric | ❌ Pending |

## Known Issues & Limitations

- No streaming export available for large collections; full result set is held in memory
- Parquet and Arrow libraries are not yet in the vcpkg manifest; must be added before implementation
- Only one exporter (JSONL) exists; no registry or third-party extension point yet

---

*See also: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)*
