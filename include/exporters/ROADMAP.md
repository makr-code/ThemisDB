<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ../../src/exporters/ROADMAP.md -->

# Roadmap — Exporters Module (Public Headers)

> Implementation roadmap: `../../src/exporters/ROADMAP.md`

## Current Status

v1.8.0 — Production-ready. 18 public headers, 15 registered export formats. `JoinExporter` added in v1.8.0.

## Completed ✅

- [x] `IExporter` base interface and `ExportFormatRegistry`
- [x] Arrow IPC, Parquet, JSONL, HuggingFace, streaming, incremental exporters
- [x] PII detection and redaction in export pipeline
- [x] AQL predicate filtering
- [x] `JoinExporter` cross-collection hash-join (v1.8.0, Issue #1722)
- [x] Typed error hierarchy in `exporter_errors.h`

## Planned

- [ ] Delta/CDC export format for change-data-capture scenarios (Target: v1.9.0)
- [ ] Avro export format support (Target: v1.9.0)
- [ ] Federated export across remote sources (Target: v2.0.0)

## Implementation Phases

### Phase 1: Core Interface (Complete ✅)
- [x] `IExporter` interface and `ExportFormatRegistry`

### Phase 2: Format Backends (Complete ✅)
- [x] Arrow IPC, Parquet, JSONL, HuggingFace, streaming, incremental

### Phase 3: Advanced Features (Complete ✅)
- [x] Join export, PII detection, data augmentation, output encryption

### Phase 4: Extended Formats (Planned)
- [ ] Delta/CDC, Avro, federated export

## Production Readiness Checklist

- [x] All public headers compile cleanly with `-Wall -Wextra`
- [x] Typed error hierarchy covers all failure modes
- [x] PII detection integrated in join and stream pipelines
- [ ] Avro format support
- [ ] CDC export format
