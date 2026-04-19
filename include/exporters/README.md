> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Exporters Module Headers

This directory contains the public header files for the exporters module.

## Purpose

Public interfaces and declarations for exporters functionality.

## Public Headers

| Header | Class / Interface | Description |
|--------|-------------------|-------------|
| `exporter_interface.h` | `IExporter`, `ExportOptions`, `ExportStats`, `BaseEntity` | Core exporter interface and shared data types |
| `exporter_errors.h` | `ExporterException`, `SchemaValidationException`, `ExportIOException`, `SizeLimitException` | Exception hierarchy |
| `exporter_metrics.h` | `ExporterMetrics` | Per-exporter Prometheus-compatible throughput and quality metrics |
| `jsonl_llm_exporter.h` | `JSONLLLMExporter`, `JSONLLLMConfig` | JSONL export for LLM training data (Alpaca/ShareGPT/ChatML/OpenAI) |
| `parquet_exporter.h` | `ParquetExporter`, `ParquetExportConfig` | Apache Parquet columnar export |
| `arrow_ipc_exporter.h` | `ArrowIPCExporter`, `ArrowIPCExportConfig`, `ArrowIPCFormat` | Apache Arrow IPC file and stream export |
| `huggingface_exporter.h` | `HuggingFaceExporter`, `HuggingFaceExportConfig` | Hugging Face Datasets-compatible export |
| `huggingface_hub_client.h` | `HuggingFaceHubClient`, `HubUploadConfig` | Direct upload to Hugging Face Hub via libcurl |
| `streaming_exporter.h` | `StreamingExporter`, `StreamingExportConfig` | Cursor-driven streaming export for large collections |
| `incremental_exporter.h` | `IncrementalExporter`, `IncrementalExportConfig` | Incremental/delta export with watermark-based change tracking |
| `join_exporter.h` | `JoinExporter`, `JoinExportConfig` | Cross-collection hash-join export (inner join, output-field aliasing, AQL predicate) |
| `aql_predicate_filter.h` | `AqlPredicateFilter` | AQL predicate filtering on exported records |
| `format_template.h` | `FormatTemplate`, `FormatTemplateType`, `validateTemplate()` | Instruction-tuning format templates (Alpaca, ShareGPT, ChatML, OpenAI) |
| `export_encryption.h` | `ExportEncryption`, `ExportEncryptionConfig` | AES-256-GCM encryption for sensitive export data |
| `export_format_registry.h` | `ExportFormatRegistry` | Singleton format registry; 15 built-in formats + user-defined templates |
| `pii_detector.h` | `PIIDetector`, `PIIDetector::Config` | PII detection and redaction (mask / hash / remove / partial) |
| `stream_writer.h` | `StreamWriter`, `CompressionType` | Low-level streaming output writer with ZSTD compression and backpressure |
| `data_augmentation.h` | `DataAugmentation`, `AugmentationConfig` | Synthetic data augmentation pipeline |

## Implementation

See `../../src/exporters/` for the implementation code.

## Documentation

See `../../src/exporters/README.md` for the full module documentation.

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

Include the relevant headers from this module:

```cpp
#include "exporters/module_header.h"
```

See [`ARCHITECTURE.md`](ARCHITECTURE.md) and [`ROADMAP.md`](ROADMAP.md) for details.
