<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Exporters Module (Public Headers)

All notable changes to the Exporters module public headers are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
For implementation-level changes see `../../src/exporters/CHANGELOG.md`.

## [Unreleased]

## [1.8.0] — 2026-03-22
### Added
- `join_exporter.h`: `JoinExporter` + `JoinExportConfig` — cross-collection inner hash-join export with AQL predicate filtering, `output_fields` selection, field aliasing, and `right_side_memory_limit_bytes` budget (Issue #1722)
- Error codes `ERR_EXPORT_JOIN_AMBIGUOUS_FIELD` (9313), `ERR_EXPORT_JOIN_PREDICATE_INVALID` (9312), `ERR_EXPORT_JOIN_MEMORY_LIMIT` (9314) in `exporter_errors.h`

## [1.7.0] — 2026-03-12
### Added
- `huggingface_hub_client.h`: `HuggingFaceHubClient` for direct dataset upload to HuggingFace Hub
- `data_augmentation.h`: `DataAugmentationPipeline` with `AugmentationConfig` and `AugmentationStats`
- `export_encryption.h`: `ExportEncryption` / `ExportEncryptor` for output stream encryption

## [1.0.0] — 2024-01-01
### Added
- `IExporter` base interface and `ExportFormatRegistry` with pluggable format support
- Arrow IPC, Parquet, JSONL, HuggingFace, streaming, and incremental exporters
- Typed error hierarchy (`exporter_errors.h`)
- AQL predicate filtering (`aql_predicate_filter.h`)
- PII detection and redaction (`pii_detector.h`)
