<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · ../../src/exporters/ -->

# Exporters Module — Public Header Architecture
**Version:** 1.8.0
**Module Path:** `include/exporters/`
**Implementation:** `../../src/exporters/`

---

## Overview

The Exporters module provides a pluggable export pipeline supporting 15+ output formats including Arrow IPC, Parquet, JSONL/LLM, HuggingFace Hub, and streaming exports. A central `ExportFormatRegistry` manages format discovery; `IExporter` defines the uniform export interface.

## Design Principles

- **Format Registry** — `ExportFormatRegistry` enumerates all registered export formats; new formats register via plugin pattern.
- **Streaming First** — `StreamingExporter` and `IncrementalExporter` handle large result sets without buffering to memory.
- **PII-Aware** — `PiiDetector` scans merged records before output; redaction is applied in the export pipeline.
- **Join Semantics** — `JoinExporter` performs in-memory hash-join across collections before export with AQL predicate filtering.

## Interface Inventory

| Header | Classes / Structs | Purpose |
|--------|-------------------|---------|
| `exporter_interface.h` | `IExporter`, `ExportOptions`, `ExportTenantContext` | Base exporter interface |
| `export_format_registry.h` | `ExportFormatRegistry` | Registry of all 15 supported export formats |
| `exporter_errors.h` | `ExporterException`, `SchemaValidationException`, `ExportIOException`, `SizeLimitException`, `QualityFilterException`, `FormatException`, `ConfigException` | Typed error hierarchy |
| `exporter_metrics.h` | `ExporterMetrics`, `ExportStats` | Per-export telemetry |
| `arrow_ipc_exporter.h` | `ArrowIPCExporter`, `ArrowIPCExportConfig` | Apache Arrow IPC export |
| `parquet_exporter.h` | `ParquetExporter` | Apache Parquet columnar export |
| `jsonl_llm_exporter.h` | `JsonlLlmExporter` | JSONL export optimised for LLM fine-tuning |
| `huggingface_exporter.h` | `HuggingFaceExporter` | HuggingFace Datasets-compatible export |
| `huggingface_hub_client.h` | `HuggingFaceHubClient` | HTTP client for HuggingFace Hub upload |
| `streaming_exporter.h` | `StreamingExporter` | Low-memory streaming export |
| `incremental_exporter.h` | `IncrementalExporter` | Checkpoint-resumable incremental export |
| `join_exporter.h` | `JoinExporter`, `JoinExportConfig` | Cross-collection hash-join export |
| `stream_writer.h` | `IStreamWriter` | Abstract stream writer for format backends |
| `format_template.h` | `IFormatTemplate`, `AlpacaTemplate` | Prompt format templates for LLM exports |
| `aql_predicate_filter.h` | `AqlPredicateFilter` | AQL predicate filtering in export pipeline |
| `data_augmentation.h` | `DataAugmentationPipeline`, `AugmentationConfig` | On-the-fly data augmentation |
| `export_encryption.h` | `ExportEncryption`, `ExportEncryptor` | Encryption of exported output streams |
| `pii_detector.h` | `PolicyEngine`, `AuditLogger` | PII detection and redaction |

## References

- Implementation details: `../../src/exporters/`
- Format plugin guide: `../../src/exporters/README.md`
