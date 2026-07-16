### Context

This issue implements the roadmap item '`AsyncIngestionWorker`: YAML Config Loading and User Context' for the content domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: `AsyncIngestionWorker`: YAML Config Loading and User Context

### Goal

Deliver the scoped changes for `AsyncIngestionWorker`: YAML Config Loading and User Context in src/content/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### `AsyncIngestionWorker`: YAML Config Loading and User Context
**Priority:** Medium
**Target Version:** v1.8.0

`async_ingestion_worker.cpp` has 2 TODOs: line 969 (`job.user_context = ""; // TODO: Add user context support`) and line 1010 (`// TODO: Implement YAML config loading`). Worker pool configuration is hardcoded; user context is not propagated to downstream audit logs.

**Implementation Notes:**
- `[ ]` Implement YAML config loading at line 1010: parse `config/content/async_worker.yaml` (keys: `worker_threads`, `queue_depth`, `batch_size`, `retry_attempts`) via `ConfigPathResolver::resolve()` + `ConfigSchemaValidator`.
- `[ ]` Propagate `user_context` from the caller's request metadata at line 969 into the `IngestionJob`; use it in downstream `AuditLogger::logEvent()` calls so ingestion events are attributable to the originating user.

---


**Priority:** High
**Target Version:** v1.7.0

Currently `ContentManager::ingest()` buffers the entire content in memory before processing. Files larger than `config_.max_content_size_bytes` are rejected. Implement chunked streaming ingestion in `async_ingestion_worker.cpp` that processes content in configurable chunks, enabling ingestion of files up to several GB.

**Implementation Notes:**
- `[x]` Add `ContentManager::ingestStream(std::istream& stream, const ContentMetadata& meta)` overload.
- `[x]` `async_ingestion_worker.cpp` reads chunks of `chunk_size_bytes` (default: 4 MB, configurable) from the stream; each chunk is processed by the appropriate `IIngestionPlugin::processChunk()` method.
- `[x]` Processors that support streaming (text, CSV, NDJSON) implement `processChunk()`; processors that require full data (PDF, image) buffer up to a configurable `max_buffered_bytes` limit (default: 256 MB) before falling back to error.
- `[x]` Back-pressure: `ingestStream()` blocks the caller when the worker queue depth exceeds `config_.max_queue_depth`; returns a `std::future<ContentId>` for async callers (CON-005 ✅).
- `[x]` Partial failure: if a chunk fails validation in `content_validator.cpp`, the entire ingestion transaction is rolled back and the partial content is purged from storage.

**Performance Targets:**
- 1 GB NDJSON file ingested at ≥ 100 MB/s sustained throughput on NVMe storage.
- Peak RSS increase during streaming ingestion < 2× `chunk_size_bytes` (i.e., two chunks in-flight at most).

---

### Acceptance Criteria

- [ ] Implement YAML config loading at line 1010: parse `config/content/async_worker.yaml` (keys: `worker_threads`, `queue_depth`, `batch_size`, `retry_attempts`) via `ConfigPathResolver::resolve()` + `ConfigSchemaValidator`.
- [ ] Propagate `user_context` from the caller's request metadata at line 969 into the `IngestionJob`; use it in downstream `AuditLogger::logEvent()` calls so ingestion events are attributable to the originating user.
- [ ] Add `ContentManager::ingestStream(std::istream& stream, const ContentMetadata& meta)` overload.
- [ ] `async_ingestion_worker.cpp` reads chunks of `chunk_size_bytes` (default: 4 MB, configurable) from the stream; each chunk is processed by the appropriate `IIngestionPlugin::processChunk()` method.
- [ ] Processors that support streaming (text, CSV, NDJSON) implement `processChunk()`; processors that require full data (PDF, image) buffer up to a configurable `max_buffered_bytes` limit (default: 256 MB) before falling back to error.
- [ ] Back-pressure: `ingestStream()` blocks the caller when the worker queue depth exceeds `config_.max_queue_depth`; returns a `std::future<ContentId>` for async callers (CON-005 ✅).
- [ ] Partial failure: if a chunk fails validation in `content_validator.cpp`, the entire ingestion transaction is rolled back and the partial content is purged from storage.
- [ ] 1 GB NDJSON file ingested at ≥ 100 MB/s sustained throughput on NVMe storage.
- [ ] Peak RSS increase during streaming ingestion < 2× `chunk_size_bytes` (i.e., two chunks in-flight at most).

### Relationships

- Roadmap row: #167 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/content/FUTURE_ENHANCEMENTS.md#asyncingestionworker-yaml-config-loading-and-user-context
- Source key: roadmap:167:content:v1.8.0:asyncingestionworker-yaml-config-loading-and-user-context

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:167:content:v1.8.0:asyncingestionworker-yaml-config-loading-and-user-context -->
<!-- roadmap-ref: row=167;module=content;target=v1.8.0 -->
<!-- roadmap-detail: src/content/FUTURE_ENHANCEMENTS.md#asyncingestionworker-yaml-config-loading-and-user-context -->
