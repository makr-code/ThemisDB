**Author:** ThemisDB Team
**Created:** 2026-10-30
**Last Updated:** 2026-10-30
**Status:** active
**Source level:** Level 1 — `src/content/`
**SOT domain:** `api-contract`
**Canonical source references:** [ContentManager API](../../include/content/content_manager.h), [processor API](../../include/content/content_processor.h), [scoped AbuseDetector/ArchiveProcessor contract](../../include/content/content_api_contract.h), [contract tests](../../tests/content/test_content_contract_hardening_focused.cpp)
**Related issue:** #7109
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot
**Human_Review:** false
**Human_Author:** ThemisDB Team
**Provenance note:** Exact model backend not verifiable from final document text alone.

# Content module API contracts

## Purpose and scope

This page summarizes caller-facing C++ interfaces. The declarations in
[`include/content/`](../../include/content/README.md) are authoritative; this document
does not add guarantees beyond those declarations and their implementations.

## Core interfaces

### `ContentManager`

Construct with shared storage, vector-index, graph-index, and secondary-index managers;
field encryption is optional in the constructor signature. Register processors explicitly.
Primary operations include:

- `ingestRawBlob(blob, filename, mime_type, user_context, config)` and
  `ingestStream(stream, filename, mime_type, user_context, config)`;
- `importContent(spec, optional_blob, user_context)` for structured import;
- metadata/blob/chunk retrieval and `assembleContent()`;
- vector, hybrid, and expansion search;
- content deletion and virtual-path operations;
- optional embedding and deduplication pipeline attachment.

Read each declaration for defaults and exact types. `Status` contains `ok` and `message`;
ingestion returns `IngestResult`, not `Status`.

### Ingestion result

`IngestResult` exposes `success`, `error_message`, `primary_content_id`,
`extracted_content_ids`, `metadata`, and ordered `stage_outcomes`. A stage outcome carries
`stage_name`, `succeeded`, `skipped`, `attempts`, and `error_message`. `success` indicates
whether required stages succeeded; it does not guarantee every optional enrichment ran.
Callers should inspect the per-stage details before reporting completion or scheduling
follow-up work.

`ContentManager::getMetrics()` returns manager-owned compression counters and compression
ratio buckets, plus deduplication check/hit counters. These are separate from
`ContentMetrics`, which has its own counters and JSON/Prometheus serialization methods.
The APIs do not by themselves establish that either metrics object is registered or
automatically exposed by a server endpoint.

### Processor extension

`IContentProcessor` is an abstract interface with extraction, chunking, embedding, name,
and supported-category methods. `ExtractionResult` carries text, structured metadata,
optional embedding, error state, and type-specific geo/media/CAD values. Implementations
must preserve the meaning of `ok` and provide an actionable error on failure.

### Async ingestion

`AsyncIngestionWorker` submissions return job IDs; callers can query status and job
history. `submitStream()` stores a non-owning `std::istream*` in the job, so the stream
must outlive its processing. Submission while stopped and queue-full conditions can
throw `std::runtime_error`; stream submissions may block at the configured queue-depth
back-pressure threshold. `stop(false)` cancels queued jobs, while `stop(true)` waits for
queued work to finish.

## Embedding API caveat

The `ContentManager::generateEmbedding(text, model_name)` signature accepts a model-name
argument, but the current implementation does not use it. The attached
`EmbeddingPipeline` uses a non-empty configured name as its enabled flag and delegates to
the shared LLM embed call. Empty output is a valid failure/disabled signal at this API
boundary; inspect the implementation and [embedding guide](EMBEDDING.md) before assuming
model selection, vector normalization, or a particular dimension contract.

## Error surfaces

The [runtime content-errors header](../../include/content/content_errors.h) defines
`ContentErrorCode` categories for input validation, processing, security, storage,
queue/back-pressure, resources, and internal/configuration failures. The separate
[scoped API contract](../../include/content/content_api_contract.h) freezes the
documented `AbuseDetector` and `ArchiveProcessor` behavior and defines its own error
codes. It does not freeze or specify every `ContentManager` API. Apply the declaration
relevant to the API being called; do not assume these error enums are interchangeable.

## Compatibility and validation

The content header overview describes compatibility expectations and compile-time
dependencies. Before changing a public declaration, check its users and tests. Existing
focused contract coverage includes
[`test_content_contract_hardening_focused.cpp`](../../tests/content/test_content_contract_hardening_focused.cpp),
[`test_content_embedding_pipeline.cpp`](../../tests/content/test_content_embedding_pipeline.cpp),
and [`test_content_security.cpp`](../../tests/content/test_content_security.cpp).

## Sources

- [ContentManager declaration](../../include/content/content_manager.h),
  [processor declaration](../../include/content/content_processor.h),
  [async worker declaration](../../include/content/async_ingestion_worker.h), and
  [error declarations](../../include/content/content_errors.h).
- [AbuseDetector/ArchiveProcessor scoped contract](../../include/content/content_api_contract.h) and
  [public-header overview](../../include/content/README.md).
- [Manager implementation](content_manager.cpp),
  [embedding adapter](content_manager_embedding.cpp), and
  [async worker implementation](async_ingestion_worker.cpp).
- [Contract tests](../../tests/content/test_content_contract_hardening_focused.cpp),
  [embedding tests](../../tests/content/test_content_embedding_pipeline.cpp), and
  [security tests](../../tests/content/test_content_security.cpp).
- [Module architecture](ARCHITECTURE.md) and [roadmap](ROADMAP.md).
