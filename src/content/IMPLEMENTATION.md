**Author:** ThemisDB Team
**Created:** 2026-10-30
**Last Updated:** 2026-11-19
**Status:** active
**Source level:** Level 1 — `src/content/`
**SOT domain:** `module-behavior`
**Canonical source references:** [manager implementation](content_manager.cpp), [server manager setup](../server/http_server.cpp), [manager API](../../include/content/content_manager.h), [processor chain](../../include/content/processor_chain_config.h), [streaming tests](../../tests/content/test_content_streaming_ingestion.cpp), [architecture](ARCHITECTURE.md)
**Related issue:** #7109, #7508
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot
**Human_Review:** false
**Human_Author:** ThemisDB Team
**Provenance note:** Exact model backend not verifiable from final document text alone.

# Content module implementation invariants

## Purpose and audience

Use this as a source-oriented checklist when changing ingestion orchestration,
processors, streaming, or stage configuration. The module is responsible for routing and
processing integration; storage/index internals remain owned by their respective modules.
See [architecture](ARCHITECTURE.md).

## Data path and invariants

### Raw blob

- `ingestRawBlob()` rejects an invalid non-empty filename before routing.
- MIME selection uses a detected content type when the caller omits one, then tries a
  filename extension. If no MIME can be selected, ingestion returns an unsuccessful
  result.
- The current raw-blob manager path does not dispatch every category through the
  registered `IContentProcessor` map. It directly handles HTML/XHTML, Markdown, and
  opt-in OCR; the archive path consults a registered `ArchiveProcessor`. If no archive
  processor is registered, it stores the archive with metadata rather than extracting it.
- The manager source does not directly invoke PDF, Office, image, audio, video, CAD, or
  geospatial extraction classes. The current server setup registers a `TextProcessor`,
  but that does not establish an end-to-end route for every text MIME type. TODO: verify
  caller/server/plugin integration before claiming routes for the remaining processors.
- The pipeline builds metadata/chunk specifications and calls `importContent()` for
  persistence. The `IngestResult` carries success, IDs, an error string, and
  per-stage outcomes where populated.

### Stream

- `ingestStream()` rejects an unreadable or empty stream, reads an initial header for MIME
  detection, and validates MIME and format before proceeding.
- The implementation identifies `text/plain`, `text/csv`, Markdown, NDJSON, and
  JSON-lines forms as streaming-capable. Other MIME types are buffered up to the
  configured `max_buffered_bytes` (default 256 MiB) and then passed through the raw-blob
  path.
- The text streaming path stores produced chunks as it reads. Do not generalize this
  lower-memory behavior to PDF, images, archives, or all other formats.
- Empty/generic `application/octet-stream` and malformed MIME values are rejected by
  `ContentValidator::validateMimeType()`.

### Stages and failures

`ProcessorChainConfig` resolves an exact MIME entry before a category entry, then the
global default; selected entries replace rather than merge. Stage defaults are enabled,
zero retries, 100 ms delay, and `continue_on_error == false`. Its JSON parser silently
ignores wrong-typed stage fields, non-object entries, and unknown category names; an
invalid retry value can therefore leave a stage at its default instead of reporting a
configuration error. Negative `max_retries` values are accepted but execute zero
attempts. The parser and public mutable fields also impose no retry/delay ceiling.

The manager's shared retry helper makes `1 + max_retries` attempts for non-negative
values and sleeps `retry_delay_ms` before retries only when that value is positive. There
is no hard total-attempt or elapsed-time cap. In current raw-blob handling this helper is
used for HTML/Markdown chunk generation (not the initial extraction call) and storage.
`continue_on_error` can mark those extraction outcomes skipped and continue; storage
failure still returns an unsuccessful ingestion result. An initial HTML/Markdown
extraction error is logged but does not receive a stage outcome. These paths are bounded
only by the provided integer configuration, not by a safety ceiling.

`AsyncIngestionConfig::retry_attempts` is parsed/configurable but is not consumed by the
worker loop: a thrown processing exception marks the job failed after one call. There is
no worker retry/recovery loop or `DEGRADED` job status. Do not promise retries, recovery,
or a persistent degraded state from this setting.

These permissive parser semantics and unbounded-by-policy stage settings are current
release blockers. Invalid configuration must not be represented as successfully applied.
Until runtime validation is implemented and tested, validate configuration externally
and do not treat ignored fields or a successful job wrapper as evidence that an optional
stage ran. Any future fallback must expose success, skipped/degraded state, attempt
count, and failure context in the caller-visible result.

## Extension contract

`IContentProcessor` implementations provide `extract`, `chunk`, `generateEmbedding`,
`getName`, and supported categories. Keep processor ownership and registration explicit;
`ContentManager::registerProcessor()` accepts a `unique_ptr`. Format-specific build
dependencies and feature flags are listed in the [public-header overview](../../include/content/README.md).

## Edge cases and regression expectations

- Test empty and unreadable streams, exact chunk boundaries, oversize non-streaming
  input, malformed MIME, and format-validation failures.
- Test MIME/category overrides and legacy boolean stage configuration when changing
  `ProcessorChainConfig` serialization.
- Test missing optional processors separately from required validation/storage paths.
- Preserve `ContentManager` result semantics; callers must inspect `success` and error
  information rather than assume every returned ID implies all enrichment completed.
- Keep any resource limit aligned across stream buffering, format processors, archive
  extraction, and deployment settings.

## Validation

Focused source coverage includes
[`test_content_streaming_ingestion.cpp`](../../tests/content/test_content_streaming_ingestion.cpp),
[`test_content_pipeline_hardening.cpp`](../../tests/content/test_content_pipeline_hardening.cpp),
and [`test_content_contract_hardening_focused.cpp`](../../tests/content/test_content_contract_hardening_focused.cpp).
Their test registration is in
[`tests/content/CMakeLists.txt`](../../tests/content/CMakeLists.txt). Some streaming tests
explicitly mirror helper logic; cite them as regression checks, not end-to-end proof.

## Sources

- [Manager implementation](content_manager.cpp),
  [validator implementation](content_validator.cpp), and
  [embedding integration](content_manager_embedding.cpp).
- [Manager API](../../include/content/content_manager.h),
  [processor API](../../include/content/content_processor.h),
  [stage configuration](../../include/content/processor_chain_config.h), and
  [validator API](../../include/content/content_validator.h).
- [Streaming tests](../../tests/content/test_content_streaming_ingestion.cpp),
  [pipeline hardening tests](../../tests/content/test_content_pipeline_hardening.cpp),
  and [API contract tests](../../tests/content/test_content_contract_hardening_focused.cpp).
- [README](README.md), [architecture](ARCHITECTURE.md), and
  [production requirements](PRODUCTION_REQUIREMENTS.md).
