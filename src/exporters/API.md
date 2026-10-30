---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Exporters API Reference

Exact model backend not verifiable from final document text alone.

## Shared interface

The public API is in `include/exporters/` and uses the
`themis::exporters` namespace. Implementations conform to
[`IExporter`](../../include/exporters/exporter_interface.h):

- `exportEntities(const std::vector<BaseEntity>&, const ExportOptions&)`
  returns `ExportStats`.
- `getSupportedFormats()`, `getName()`, and `getVersion()` identify the
  implementation.
- `ExportOptions` carries output path, projection and predicate options,
  compression, resource limits, progress callback, error policy, optional
  encryption, and policy/audit context.
- `ExportStats` reports total/exported/failed/skipped entities, bytes,
  duration, errors, optional metrics, and estimated ETA.

For file-backed exporters, set `output_path`. If a `PolicyEngine` is attached,
the exporter must enforce its policy before opening output or a cursor.
Tenant context is optional in the shared type; integrations that require
tenant isolation must populate and enforce it rather than relying on the
default.

## Exporter entry points

| API | Formats / purpose | Key configuration |
|---|---|---|
| `JSONLLLMExporter` | `jsonl`, `llm_jsonl`, `lora_jsonl`, `qlora_jsonl` | `JSONLLLMConfig`: style, field mapping, quality/schema checks, template, metadata and PII options |
| `ParquetExporter` | `parquet` | row-group size, compression, schema inference, column hints and projection |
| `ArrowIPCExporter` | `arrow`, `arrows`, `arrow_ipc` | file or stream, schema inference, column projection and null tracking |
| `HuggingFaceExporter` | `huggingface`, `hf_datasets`, `hf_jsonl` | dataset metadata, split, features and embedded JSONL config |
| `StreamingExporter` | `jsonl`, `streaming_jsonl` | cursor, page size, memory ceiling and checkpoint path |
| `IncrementalExporter` | `jsonl`, `incremental_jsonl` | sequence field, watermark path and missing-sequence policy |
| `JoinExporter` | `jsonl`, `join_jsonl` | join keys, predicate, output aliases, PII options and right-side memory budget |
| `HuggingFaceHubClient` | remote dataset upload | repo, token/key reference, policy/audit hooks, retries and timeout |

Use the public headers for exact constructor and method signatures:
[`include/exporters/README.md`](../../include/exporters/README.md) indexes
the header set.

## Contracts and edge cases

- Determinism is byte-for-byte for repeated calls on the same platform with
  equal input and format version; this is not a cross-platform guarantee.
- Streaming chunks retain production order. A mid-stream interruption yields
  `STREAM_INTERRUPTED`; discard partial output and resume only from an
  acknowledged boundary supported by the caller.
- CSV, Parquet, and Arrow represent null values differently; see
  `include/exporters/exporters_api_contract.h`.
- `IncrementalExporter` selects sequence values strictly above its persisted
  watermark. It leaves the watermark unchanged on output I/O failure.
- `JoinExporter` requires `setRightCollection()` before export and stores the
  right side in memory. Configure the memory limit for the expected input.
- `HuggingFaceHubClient` returns `HubUploadResult`; HTTP, auth, policy, and
  missing-libcurl failures are not equivalent to a successful local dataset
  export.
- `StreamWriter` accepts `CompressionType::GZIP` for compatibility but writes
  ZSTD-framed output. Consumers must not infer gzip from that enum value.

## Error contract

`ExporterException` and its typed subclasses carry error codes and context.
The canonical `ExporterErrorCode` taxonomy and retry/resume predicates are
defined in `include/exporters/exporters_api_contract.h`. Only
`STREAM_INTERRUPTED` is resumable by `isResumableError`; policy denials are
fail-closed and require a policy/configuration correction before retry.

Related: [IMPLEMENTATION.md](./IMPLEMENTATION.md),
[TRUST.md](./TRUST.md), [RESEARCH.md](./RESEARCH.md).
