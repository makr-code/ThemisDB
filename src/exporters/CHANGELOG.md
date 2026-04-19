<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Exporters Module

All notable changes to the Exporters module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.8.0] — 2026-03-22
### Added
- Cross-collection hash-join export: `JoinExporter` + `JoinExportConfig` in `src/exporters/join_exporter.cpp` / `include/exporters/join_exporter.h` (Issue #1722)
  - Inner join semantics with in-memory right-side hash table keyed on `right_key_field`
  - `output_fields` selection and `"src_name:alias"` renaming; qualified `left.<field>` / `right.<field>` references resolve ambiguous names
  - AQL `join_predicate` filtering applied on the merged record
  - Right-side memory budget (`right_side_memory_limit_bytes`, default 1 GiB) enforced via `ERR_EXPORT_JOIN_MEMORY_LIMIT`
  - Error codes: `ERR_EXPORT_JOIN_AMBIGUOUS_FIELD` (9313), `ERR_EXPORT_JOIN_PREDICATE_INVALID` (9312), `ERR_EXPORT_JOIN_MEMORY_LIMIT` (9314)
  - PII detection and redaction on merged serialised JSON
  - Throughput ≥ 50 000 merged docs/sec
  - `JoinExporter` registered in `ExportFormatRegistry` as `"join"` and `"join_jsonl"` (13 → 15 built-in formats)
  - 676-line test suite in `tests/exporters/test_join_exporter.cpp`; CI: `.github/workflows/join-exporter-ci.yml`

## [1.7.0] — 2026-03-09
### Added
- `validate_template` dry-run mode: `validateTemplate()` free function in `format_template.h/cpp`; `JSONLLLMExporter::validateTemplate()`; 18 tests
- `ExportFormatRegistry` singleton: 13 built-in formats (9 plain + 4 instruction-tuning shortcuts: `jsonl_alpaca`, `jsonl_sharegpt`, `jsonl_chatml`, `jsonl_openai_ft`); user-defined templates loadable via `loadTemplatesFromConfig()` / `loadTemplatesFromJson()` (EXP-005)
- `ExporterMetrics` for per-exporter Prometheus metrics (`src/exporters/exporter_metrics.cpp`)

## [1.6.0] — 2026-02-01
### Added
- PolicyEngine authorization check (`enforceExportPolicy()`) in all 6 exporters; `ERR_EXPORT_POLICY_DENIED` (9310) added to error registry (EXP-001)
- Hugging Face Hub direct upload via libcurl: `HuggingFaceHubClient` with `HubUploadConfig` (policy engine, audit log, requesting user); authorization + audit logging on all return paths (Issue #1719, EXP-002)
- `--incremental` CLI flag for `tools/export_cli.cpp` (themis-export binary): `--format incremental` and `--incremental` shorthand (EXP-004)
- PII detection and redaction in export pipeline (`src/exporters/pii_detector.cpp`)
- Stream writer for low-memory large exports (`src/exporters/stream_writer.cpp`)

## [1.5.0] — 2026-01-10
### Added
- AES-256-GCM export encryption for sensitive training data (`src/exporters/export_encryption.cpp`) (Issue #1728)
- Synthetic data augmentation pipeline (`src/exporters/data_augmentation.cpp`)
- Instruction-tuning format templates: Alpaca, ShareGPT, ChatML, OpenAI (`src/exporters/format_template.cpp`) (Issue #1727)
- Incremental/delta export with watermark and checkpoint (`src/exporters/incremental_exporter.cpp`) (Issue #1726)

## [1.4.0] — 2025-12-01
### Added
- Apache Arrow IPC file (`.arrow`) and stream (`.arrows`) export: `ArrowIpcExporter` (`src/exporters/arrow_ipc_exporter.cpp`) (Issue #1714)
- Hugging Face Datasets-compatible export: JSONL data shards + `dataset_card.md` + `dataset_info.json` (`src/exporters/huggingface_exporter.cpp`) (Issue #1711)
- AQL predicate filtering to restrict exported records without code changes (`src/exporters/aql_predicate_filter.cpp`) (Issue #1715)

## [1.3.0] — 2025-10-01
### Added
- Parquet export with configurable Arrow schema (`src/exporters/parquet_exporter.cpp`) (Issue #1713)
- Streaming export for large collections with progress callbacks (records exported, bytes written, estimated ETA) (`src/exporters/streaming_exporter.cpp`)
- Resumable export with checkpoint support (Issue #1717)

## [1.0.0] — 2024-01-01
### Added
- JSONL exporter for LLM training data (`src/exporters/jsonl_llm_exporter.cpp`)
- Configurable field selection for export
- Batch export operations with configurable batch size and output file rotation
- LoRA adapter metadata generation compatible with PEFT format
- Export pipeline infrastructure
- vLLM multi-LoRA integration support
