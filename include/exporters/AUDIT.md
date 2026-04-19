<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Audit Report — Exporters Module (Public Headers)

**Last Audit:** 2026-03-22
**Auditor:** Copilot
**Status:** ✅ Pass

## Summary

| Metric | Result |
|--------|--------|
| Public Header Files | 18 |
| Registered Export Formats | 15 (`ExportFormatRegistry`) |
| Stubs | 0 |
| Security Issues | None |
| Open TODOs | Low (streaming edge-cases) |

## Header Files Audited

| Header | Status | Notes |
|--------|--------|-------|
| `exporter_interface.h` | ✅ Current | `IExporter` base interface stable |
| `export_format_registry.h` | ✅ Current | 15 registered formats |
| `exporter_errors.h` | ✅ Current | 7-class typed error hierarchy |
| `exporter_metrics.h` | ✅ Current | `ExportStats` atomic counters |
| `arrow_ipc_exporter.h` | ✅ Current | Apache Arrow IPC export |
| `parquet_exporter.h` | ✅ Current | Columnar Parquet export |
| `jsonl_llm_exporter.h` | ✅ Current | JSONL/LLM fine-tune format |
| `huggingface_exporter.h` | ✅ Current | HuggingFace Datasets export |
| `huggingface_hub_client.h` | ✅ Current | Hub upload client |
| `streaming_exporter.h` | ✅ Current | Low-memory streaming |
| `incremental_exporter.h` | ✅ Current | Checkpoint-resumable export |
| `join_exporter.h` | ✅ Current | Hash-join export (v1.8.0) |
| `stream_writer.h` | ✅ Current | Abstract stream writer |
| `format_template.h` | ✅ Current | Alpaca and custom templates |
| `aql_predicate_filter.h` | ✅ Current | AQL predicate filter |
| `data_augmentation.h` | ✅ Current | On-the-fly augmentation |
| `export_encryption.h` | ✅ Current | Output stream encryption |
| `pii_detector.h` | ✅ Current | PII detection/redaction |

## Findings

### Resolved
- `JoinExporter` right-side memory budget enforced (`right_side_memory_limit_bytes`, default 1 GiB) — `ERR_EXPORT_JOIN_MEMORY_LIMIT` (9314).
- PII detection applied on merged join records before serialisation.

### Open
- Implementation-level audit: `../../src/exporters/AUDIT.md`.
