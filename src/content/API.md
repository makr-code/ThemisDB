**Author:** ThemisDB Team
**Created:** 2026-10-30
**Last Updated:** 2026-11-19
**Status:** active
**Source level:** Level 1 — `src/content/`
**SOT domain:** `api-contract`
**Canonical source references:** [ContentManager API](../../include/content/content_manager.h), [manager implementation](content_manager.cpp), [server manager setup](../server/http_server.cpp), [processor API](../../include/content/content_processor.h), [scoped AbuseDetector/ArchiveProcessor contract](../../include/content/content_api_contract.h), [contract tests](../../tests/content/test_content_contract_hardening_focused.cpp)
**Related issue:** #7109, #7508
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

`searchWithExpansion(..., RequestContext, StopReason*)` is the bounded overload for
request-aware callers. It propagates the context to KNN and checks it during graph
expansion and result handling. The built-in `TextProcessor` fallback polls during
tokenization and per-token embedding generation, returning no partial vector after a
stop. An injected synchronous embedding callback, other processor implementations,
graph neighbor/Dijkstra reads, whitelist scans, and sorting remain checkpoint-only.
A stopped request discards partial results and reports the stop reason through
`StopReason*`. The legacy overload remains unbounded.

Read each declaration for defaults and exact types. `Status` contains `ok` and `message`;
ingestion returns `IngestResult`, not `Status`.

### Ingestion result

`IngestResult` exposes `success`, `error_message`, `primary_content_id`,
`extracted_content_ids`, `metadata`, and ordered `stage_outcomes`. A stage outcome carries
`stage_name`, `succeeded`, `skipped`, `attempts`, and `error_message`. `success` indicates
whether required stages succeeded; it does not guarantee every optional enrichment ran.
Callers should inspect the per-stage details before reporting completion or scheduling
follow-up work.

### Format dispatch boundary

The public format-processor headers do not imply that `ContentManager` dispatches all
formats through `IContentProcessor`. In the current `ingestRawBlob()` implementation,
HTML/XHTML, Markdown, and opt-in OCR have direct processing paths; the archive path
consults a registered `ArchiveProcessor`. The manager source does not directly invoke the
PDF, Office, image, audio, video, CAD, or geospatial extraction classes. Other callers
may process content separately and pass a structured specification to `importContent()`,
but that caller path is not established by this API page.

The current server setup registers a `TextProcessor`; this registration alone does not
establish that `ingestRawBlob()` dispatches every text MIME type through it.

**TODO:** Verify server/plugin integration for the remaining format processors before
documenting end-to-end manager extraction support. Do not treat header inventory as
evidence that a format is extracted by `ingestRawBlob()`.

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

### Validation contract and unsupported controls

`ContentValidator::validate()` rejects empty input, an invalid supplied filename,
undetectable/generic MIME when MIME enforcement is enabled, oversized input, detected
magic-byte mismatches, and policy violations when a `ContentPolicy` is supplied. Text
limits use the smaller of `max_content_size` and `max_text_length`. Inputs shorter than
four bytes pass the magic-byte helper because there is not enough data to check.

`ContentValidationConfig` also serializes `check_for_malware`, `scan_for_pii`,
`check_for_abuse`, `enable_schema_validation`, schema path, and timeout fields, but
`ContentValidator::validate()` does not run those scans/schema checks or enforce those
timeouts. `ContentValidationConfig::fromJson()` deserializes these values but does not
establish that the requested capability is available or enforce a complete configuration
range policy. Its `malware_checked` result remains false. Setting these fields is not
evidence that a detector ran or that a payload was scanned; deployments requiring these
controls must verify and invoke their separately wired security components. A successful
validator result means only the checks actually described above passed.

### VersionManager contract

`VersionManager` is a standalone in-process helper; current source does not establish
integration with `ContentManager` or durable storage. It retains metadata and full
content snapshots in memory, so its history is lost when the object/process is destroyed.
`createVersion()` stores the caller-supplied hash and size without checking them against a
payload. `createVersionWithContent()` stores the complete snapshot and computes a
heuristic line delta; the delta is not a documented reversible representation.

`getContent()` returns `nullopt` for missing versions, metadata-only entries, and
zero-length snapshots, so callers cannot distinguish the latter two through that method.
There is no rollback API or thread-safety guarantee. Deleting the highest numbered
version permits that number to be reused by the next create call. Do not use this helper
as a durable, concurrent, monotonic audit/version store.

## Compatibility and validation

The content header overview describes compatibility expectations and compile-time
dependencies. Before changing a public declaration, check its users and tests. Existing
focused contract coverage includes
[`test_content_contract_hardening_focused.cpp`](../../tests/content/test_content_contract_hardening_focused.cpp),
[`test_content_embedding_pipeline.cpp`](../../tests/content/test_content_embedding_pipeline.cpp),
[`test_content_security.cpp`](../../tests/content/test_content_security.cpp), and
[`test_content_version_manager.cpp`](../../tests/content/test_content_version_manager.cpp).

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
- [VersionManager declaration](../../include/content/version_manager.h) and
  [implementation](version_manager.cpp).
- [Contract tests](../../tests/content/test_content_contract_hardening_focused.cpp),
  [embedding tests](../../tests/content/test_content_embedding_pipeline.cpp), and
  [security tests](../../tests/content/test_content_security.cpp), plus the
  [versioning tests](../../tests/content/test_content_version_manager.cpp).
- [Module architecture](ARCHITECTURE.md) and [roadmap](ROADMAP.md).
