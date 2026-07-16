> **Build:** `cmake --preset release && cmake --build build/release`

# Exporters Module — Public Headers

**Module Path:** `include/exporters/`
**Implementation Overview:** `../../src/exporters/README.md`

## Purpose

Public interfaces for ThemisDB export pipelines: JSONL training-data export,
columnar export (Parquet / Arrow IPC), Hugging Face dataset packaging and Hub
upload, streaming and incremental export, join export, encryption, PII
redaction, and metrics.

## Header Entry-Points

| Header | Primary API | Runtime Role |
|--------|-------------|--------------|
| `exporter_interface.h` | `IExporter`, `ExportOptions`, `ExportStats` | Shared export contract, output path/filter/compression/progress options, authorization context |
| `exporter_errors.h` | `ExporterException`, `SchemaValidationException`, `ExportIOException`, `SizeLimitException`, `ConfigException` | Stable exception surface for config, schema, I/O, and limit failures |
| `jsonl_llm_exporter.h` | `JSONLLLMExporter`, `JSONLLLMConfig` | JSONL export for instruction/chat/text completion datasets with template validation, weighting, metadata, and PII controls |
| `format_template.h` | `FormatTemplateType`, `FormatTemplateFieldMapping`, `validateTemplate()` | Named Alpaca / ShareGPT / ChatML / OpenAI fine-tuning templates and preflight checks |
| `streaming_exporter.h` | `ExportCursor`, `VectorExportCursor`, `StreamingExporter`, `StreamingExportConfig` | Cursor-driven export with checkpoint-based resume and ETA reporting |
| `incremental_exporter.h` | `IncrementalExporter`, `IncrementalExportConfig` | Delta export based on a persisted sequence watermark |
| `join_exporter.h` | `JoinExporter`, `JoinExportConfig` | Cross-collection hash join with output aliasing, optional AQL predicate, and memory budgeting |
| `parquet_exporter.h` | `ParquetExporter`, `ParquetExportConfig` | Columnar export for analytics and dataset interchange |
| `arrow_ipc_exporter.h` | `ArrowIPCExporter`, `ArrowIPCExportConfig`, `ArrowIPCFormat` | Arrow file / stream export for zero-copy pipelines |
| `huggingface_exporter.h` | `HuggingFaceExporter`, `HuggingFaceExportConfig` | Local Hugging Face dataset directory generation |
| `huggingface_hub_client.h` | `HuggingFaceHubClient`, `HubUploadConfig`, `MemoryShardSpec`, `HubUploadResult` | Hub upload from disk or in-memory shards with retry, audit, and policy checks |
| `export_encryption.h` | `ExportEncryption`, `ExportEncryptionConfig` | AES-256-GCM file encryption with KEK/DEK indirection |
| `pii_detector.h` | `PIIDetector`, `PIIDetector::Config`, `PIIMetrics` | Pattern-based PII detection and redaction helpers |
| `stream_writer.h` | `StreamWriter`, `CompressionType` | Buffered output writer with ZSTD-backed compression |
| `export_format_registry.h` | `ExportFormatRegistry` | Central format registration for built-ins and user-defined templates |
| `exporter_metrics.h` | `ExporterMetrics` | Metrics for throughput, delta skips, checkpoints, and rate limits |
| `aql_predicate_filter.h` | `AqlPredicateFilter` | Predicate-based export filtering |
| `data_augmentation.h` | `DataAugmentation`, `AugmentationConfig` | Optional synthetic augmentation for training exports |

## Public API Behavior

### Core contract (`IExporter`, `ExportOptions`, `ExportStats`)

- `IExporter::exportEntities()` returns `ExportStats` with entity counters,
  bytes written, duration, optional error strings, and ETA for streaming runs.
- `ExportOptions::output_path` is the required destination for file-backed
  exporters.
- `include_fields`, `exclude_fields`, and `filter_expression` shape which data
  reaches the serializer.
- `progress_callback` receives periodic snapshots every
  `ExportOptions::progress_interval` records.
- `enforceExportPolicy(options)` is the required authorization gate when
  `policy_engine` is configured; exporters are expected to call it before
  opening any cursor or output.

### Exporter-specific behavior

- `JSONLLLMExporter` supports style-based formatting or named templates,
  optional weighting, schema checks, LoRA metadata, quality filters, and PII
  handling.
- `StreamingExporter` keeps peak memory bounded by page size and stream-writer
  buffers, and persists cursor offsets to `checkpoint_path` for resume.
- `IncrementalExporter` only exports entities whose sequence value is strictly
  greater than the persisted watermark; watermark writes are atomic.
- `JoinExporter` performs an in-memory hash join, requires `setRightCollection()`
  before export, and rejects ambiguous output fields without aliases.
- `HuggingFaceHubClient` uploads either an on-disk dataset directory or
  `MemoryShardSpec` buffers; it is not thread-safe and surfaces network / HTTP
  failures via `HubUploadResult`.

## Configuration and Limits

| Config Type | Important Options | Operational Notes |
|-------------|-------------------|-------------------|
| `ExportOptions` | `output_path`, `include_fields`, `exclude_fields`, `filter_expression`, `compression_type`, `max_file_size_bytes`, `buffer_size_bytes`, `continue_on_error`, `max_errors`, `collection_name`, `requesting_user` | Shared across exporters; also carries optional encryption, PolicyEngine, and AuditLogger hooks |
| `JSONLLLMConfig` | `style`, `field_mapping`, `quality`, `structured_gen`, `adapter_metadata`, `pii_config`, `format_template_type` | Default quality filter skips empty outputs and duplicates; template dry-runs are available via `validateTemplate()` |
| `StreamingExportConfig` | `page_size`, `max_buffer_bytes`, `checkpoint_path` | Default buffer ceiling is 256 MiB; empty checkpoint path disables resume |
| `IncrementalExportConfig` | `sequence_field`, `watermark_path`, `export_missing_sequence` | Empty watermark path falls back to full export behavior |
| `JoinExportConfig` | `left_collection`, `right_collection`, `left_key_field`, `right_key_field`, `join_predicate`, `output_fields`, `pii_config`, `right_side_memory_limit_bytes` | Default right-side memory budget is 1 GiB; `0` disables the limit |
| `HubUploadConfig` | `hf_token`, `hf_token_kek_id`, `repo_id`, `create_repo`, `private_repo`, `max_retries`, `retry_delay_ms`, `timeout_seconds`, `policy_engine`, `audit_log` | `uploadDataset()` / `uploadShards()` return `success=false` when auth, policy, or libcurl prerequisites fail |
| `StreamWriter::Config` | `output_path`, `compression`, `compression_level`, `buffer_size`, `max_file_size` | `CompressionType::GZIP` is accepted for backward compatibility but produces ZSTD-framed output |

## Installation

Headers are included with ThemisDB. Ensure your target exposes the project
include directory:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

### JSONL fine-tuning export

```cpp
#include "exporters/jsonl_llm_exporter.h"

using namespace themis::exporters;

JSONLLLMConfig config;
config.format_template_type = FormatTemplateType::ALPACA;
config.pii_config.enable_detection = true;
config.pii_config.enable_redaction = true;

ExportOptions options;
options.output_path = "/tmp/training.jsonl";
options.collection_name = "qa_pairs";

JSONLLLMExporter exporter(config);
auto stats = exporter.exportEntities(entities, options);
```

### Streaming export with checkpoint resume

```cpp
#include "exporters/streaming_exporter.h"

using namespace themis::exporters;

StreamingExportConfig config;
config.page_size = 5000;
config.checkpoint_path = "/tmp/exporters.checkpoint.json";

ExportOptions options;
options.output_path = "/tmp/export.jsonl";

StreamingExporter exporter(config);
VectorExportCursor cursor(entities, config.page_size);
auto stats = exporter.exportFromCursor(cursor, options);
```

## Troubleshooting

- `ERR_EXPORT_POLICY_DENIED`: verify `collection_name`, `requesting_user`, and
  the attached `PolicyEngine` policy set.
- `ERR_EXPORT_CONFIG_INVALID`: required paths or join collection names are
  missing, or Hub upload config is incomplete.
- Template exports skip or fail records when required fields are absent; run
  `validateTemplate()` first for deterministic preflight feedback.
- `ERR_EXPORT_JOIN_AMBIGUOUS_FIELD`: qualify/alias colliding fields such as
  `"left.title:title"` and `"right.title:annotation_title"`.
- No incremental resume: confirm `watermark_path` / `checkpoint_path` is set
  and writable.
- Hub uploads return `success=false` when `CURL_ENABLED` is unavailable or the
  Hub token is missing/invalid.

## Related Docs

- Implementation overview: [`../../src/exporters/README.md`](../../src/exporters/README.md)
- Architecture guide: [`../../src/exporters/ARCHITECTURE.md`](../../src/exporters/ARCHITECTURE.md)
- Module roadmap: [`../../src/exporters/ROADMAP.md`](../../src/exporters/ROADMAP.md)
- Future enhancements: [`../../src/exporters/FUTURE_ENHANCEMENTS.md`](../../src/exporters/FUTURE_ENHANCEMENTS.md)
- German module index: [`../../docs/de/exporters/README.md`](../../docs/de/exporters/README.md)
- Primary sources (EN): [`../../docs/en/exporters/PRIMARY_SOURCES.md`](../../docs/en/exporters/PRIMARY_SOURCES.md)
