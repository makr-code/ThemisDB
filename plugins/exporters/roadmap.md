# Exporter Plugins – Roadmap

## Current Status

**Status:** ✅ Production-ready

| Exporter | Implementation | Status |
|----------|---------------|--------|
| JSONL LLM Exporter | `src/exporters/jsonl_llm_exporter.cpp` | ✅ Production |

Entry-point: `plugins/exporters/` (plugin manifests) · implementation in `src/exporters/`

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

## Dependencies

- `src/exporters/jsonl_llm_exporter.cpp`
- Apache Arrow / Parquet libraries (to be added to vcpkg)
- ThemisDB `QueryEngine` + AQL

## Open Questions

- [ ] Should exporters be synchronous or always async/job-based?
- [ ] What is the target throughput for streaming export (records/sec)?

---

*See also: [`future_enhancements.md`](future_enhancements.md)*
