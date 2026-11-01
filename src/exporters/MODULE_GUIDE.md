---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "include/exporters/streaming_exporter.h; src/exporters/streaming_exporter.cpp; include/exporters/*.h"
Related: "Issue #7346; DOC-WEEKLY-2026-44"
---

# Exporters Module Guide

Exact model backend not verifiable from final document text alone.

## Choose the right path

| Need | Start here |
|---|---|
| JSONL training records or named prompt/chat shape | `JSONLLLMExporter` and `FormatTemplateType` |
| Columnar analytical artifact | `ParquetExporter` or `ArrowIPCExporter` |
| Cursor page-based output and resume state | `StreamingExporter` with an `ExportCursor` |
| Sequence-based delta output | `IncrementalExporter` and a writable watermark path |
| Combine two entity sets before output | `JoinExporter`; load the right collection first |
| Hugging Face directory metadata and dataset card | `HuggingFaceExporter` |
| Upload a dataset directory or in-memory shards | `HuggingFaceHubClient` |
| Shared policy, projection, compression and output settings | `ExportOptions` |

All of these interfaces are declared under `include/exporters/`. Implementation
ownership, formats, and source boundaries are summarized in
[`README.md`](./README.md) and
[`ARCHITECTURE.md`](./ARCHITECTURE.md).

## Standard integration sequence

1. Select the exporter and build its typed configuration. Decide which fields
   may leave the source and set `include_fields`/`exclude_fields` or the
   supported predicate.
2. Provide `output_path` for file-backed export. Configure explicit resource
   limits and writable checkpoint/watermark paths for stateful modes.
3. Attach the deployment's `PolicyEngine` and audit context, and supply the
   requesting user and collection identifiers expected by the policy.
4. For templates, validate representative entities with `validateTemplate()`.
   For Parquet/Arrow/Hugging Face, choose explicit schema/features when
   inference from one batch is not sufficient.
5. Run `exportEntities()` or `exportFromCursor()`. Check `ExportStats`, error
   details, and the resulting artifact; a return value alone is not proof of
   downstream compatibility.
6. Publish only a complete artifact. For Hub transfer, inspect
   `HubUploadResult.success`, status, and error; the Hub client is a separate
   network operation.

The concise code examples in [`include/exporters/README.md`](../../include/exporters/README.md)
show JSONL and checkpointed streaming setup.

## Operational limits to account for

- A zero `max_file_size_bytes` or throughput limit is unlimited, not a
  conservative bound.
- Streaming defaults to page size 1,000 and `max_buffer_bytes` of 256 MiB. The
  implementation uses that value as a flush trigger, not a cap on total
  process memory. Its checkpoint path is optional; without one there is no
  persisted resume offset.
- Incremental export with an empty watermark path falls back to full-export
  behavior.
- Join export has a default 1 GiB right-side memory budget; setting the limit
  to zero disables that bound.
- Hub upload defaults include three retries and a 120-second request timeout.
  Confirm that these fit the deployment's external-service policy.
- Build features affect format paths; check `isArrowAvailable()` before
  depending on Arrow-backed behavior.

## Where to go next

- Contract changes and typed API details: [API.md](./API.md).
- Code organization and regression expectations: [IMPLEMENTATION.md](./IMPLEMENTATION.md).
- Performance evidence: [RESEARCH.md](./RESEARCH.md) and
  [PERFORMANCE_EXPECTATIONS.md](./PERFORMANCE_EXPECTATIONS.md).
- Incident triage and rollback: [RUNBOOK.md](./RUNBOOK.md) and
  [OPERATIONS.md](./OPERATIONS.md).
