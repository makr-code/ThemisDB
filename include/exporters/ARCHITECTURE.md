---
Author: "ThemisDB Team"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "architecture-governance"
Canonical_References: "include/exporters/*.h; src/exporters/ARCHITECTURE.md"
Related: "Issue #7346; DOC-WEEKLY-2026-44"
---

Exact model backend not verifiable from final document text alone.

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: reviewed against public headers 2026-10-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/exporters/ARCHITECTURE.md -->

# Exporters Module — Public Header Architecture

**Module Path:** `include/exporters/`  
**Implementation:** `../../src/exporters/`  
**Canonical architecture doc:** [`../../src/exporters/ARCHITECTURE.md`](../../src/exporters/ARCHITECTURE.md)

---

## 1. Overview

`include/exporters/` defines the **public data export in Parquet, Arrow IPC, JSONL/LLM, HuggingFace, streaming, incremental, and encrypted formats API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/exporters/ARCHITECTURE.md`](../../src/exporters/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Exporter Interfaces

| Header | Public Type | Purpose |
|--------|------------|---------|
| `exporter_interface.h` | `IExporter` | Pluggable exporter contract |
| `export_format_registry.h` | `ExportFormatRegistry` | Format-to-exporter registry |
| `exporter_errors.h` | `ExporterException` and typed subclasses | Exporter exception surface; error-code taxonomy is in `exporters_api_contract.h` |
| `exporter_metrics.h` | `ExporterMetrics` | Export throughput and latency telemetry |
| `format_template.h` | `IFormatTemplate`, `FormatTemplateType` | Template-driven JSONL record rendering and validation |
### 2.2 Batch and Streaming Exporters

| Header | Public Type | Purpose |
|--------|------------|---------|
| `parquet_exporter.h` | `ParquetExporter` | Apache Parquet columnar export |
| `arrow_ipc_exporter.h` | `ArrowIPCExporter` | Arrow IPC file/stream serialization; availability depends on the Arrow-enabled build |
| `jsonl_llm_exporter.h` | `JSONLLLMExporter` | JSONL format for LLM fine-tuning datasets |
| `streaming_exporter.h` | `StreamingExporter` | Streaming chunk export |
| `stream_writer.h` | `StreamWriter` | Low-level streaming write utilities |
| `incremental_exporter.h` | `IncrementalExporter` | Sequence-watermark filtering over caller-supplied entities |
| `join_exporter.h` | `JoinExporter` | Multi-collection join export |
### 2.3 ML and HuggingFace

| Header | Public Type | Purpose |
|--------|------------|---------|
| `huggingface_exporter.h` | `HuggingFaceExporter` | HuggingFace dataset format export |
| `huggingface_hub_client.h` | `HuggingFaceHubClient` | HuggingFace Hub upload client |
| `data_augmentation.h` | `DataAugmentation` | Export-time data augmentation pipeline |
### 2.4 Security and Filtering

| Header | Public Type | Purpose |
|--------|------------|---------|
| `export_encryption.h` | `ExportEncryption` | At-rest encryption for exported data |
| `pii_detector.h` | `PIIDetector` | PII detection before export |
| `aql_predicate_filter.h` | `AqlPredicateFilter` | AQL predicate evaluation for supported export paths |

---

## 3. Namespace Layout

All public types reside in the `themis::exporters` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/exporters/` expose the public declarations; internal implementation lives in `src/exporters/`.
- Compatibility guarantees are defined by the applicable contract and release policy, not by the header location alone.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
