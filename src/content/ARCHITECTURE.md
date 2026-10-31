**Author:** ThemisDB Contributors
**Created:** 2026-10-31
**Last Updated:** 2026-10-31
**Status:** active
**Source level:** Level 1 — `src/content/`
**SOT domain:** `module-behavior`
**Canonical source references:** [manager API](../../include/content/content_manager.h), [manager implementation](content_manager.cpp), [server manager setup](../server/http_server.cpp), [processor-chain config](../../include/content/processor_chain_config.h), [streaming tests](../../tests/content/test_content_streaming_ingestion.cpp), [content test registration](../../tests/content/CMakeLists.txt)
**Related milestone:** DOC-WEEKLY-2026-44
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** themisdb-doc-orchestrator
**Human_Review:** false
**Human_Author:** ThemisDB Contributors
**Provenance note:** Exact model backend not verifiable from final document text alone.

# Architecture - Content Module

## Overview

The content module exposes raw-blob and stream ingestion, format processors, optional
enrichment, and storage/index integration through `ContentManager`. Exact behavior depends
on the registered processor, configured stages, and optional build/runtime dependencies.
The flow below describes the current manager implementation; it is not a guarantee that
every optional stage runs for every input.

## Main Execution Planes

1. Intake and routing
- `ingestRawBlob()` validates a supplied filename; when MIME is omitted, it attempts
  content-type detection and then filename-extension lookup.
- `ingestStream()` validates the detected/header MIME. Line-oriented text formats use the
  streaming path; other formats are buffered up to `max_buffered_bytes` (default 256 MiB)
  before using the raw-blob path.
- The raw-blob manager path does not dispatch every category through the registered
  `IContentProcessor` map. In the current implementation it has direct HTML/XHTML and
  Markdown paths, an opt-in OCR path, and a special archive path that consults a
  registered `ArchiveProcessor`. If no archive processor is registered, the archive is
  stored with metadata instead of extracted.
- The server setup currently registers a `TextProcessor`, but this is not evidence that
  the raw-blob path invokes that processor for all text MIME types.
- Other public processor classes are not, by their presence alone, evidence of a direct
  `ContentManager::ingestRawBlob()` route. TODO: map any caller/server/plugin dispatch for
  the remaining format processors before claiming end-to-end manager extraction support.

2. Processing and persistence
- Processing builds content metadata and chunk specifications, then uses
  `importContent()` for persistence.
- The manager writes content/blob and chunk records through the supplied storage manager;
  it also writes full-text/vector data when those interfaces and corresponding data are
  available. Graph relations are added for chunk/parent-child relationships where the
  relevant path applies.
- Validation, security, deduplication, and enrichment are capability/configuration
  dependent. Do not infer that every control or optional enrichment is enabled by default.

3. Retrieval paths
- `searchContent()` uses the vector-index manager.
- `searchContentHybrid()` combines the available vector and full-text results using
  reciprocal-rank fusion; `searchWithExpansion()` traverses graph-index neighbors.
- These are separate API paths and require the corresponding index dependencies/data.

4. Asynchronous intake
- `AsyncIngestionWorker` owns worker threads and a bounded queue. It is an optional
  wrapper around manager calls, not an ingestion thread pool owned by `ContentManager`.
- A submitted stream is non-owning and must remain alive while the job processes it.

## Core Contracts

| Contract | Behavior |
|---|---|
| ingestion interfaces | manager methods and registered processor routing |
| stage configuration | exact MIME configuration, then category configuration, then global defaults |
| result reporting | `IngestResult` reports required-stage success and any populated stage outcomes |
| retrieval interfaces | vector, hybrid full-text/vector, and graph-expanded search paths |

## Failure Semantics

- `IngestResult` reports success/failure and an error string; callers should inspect
  per-stage outcomes where populated. A returned content ID alone does not prove that
  optional enrichment completed.
- Validation and format behavior depends on the path and enabled dependencies; this
  document does not claim that every malformed input is rejected by one shared gate.
- `ProcessorChainConfig` controls stage enablement and retry behavior; the manager's
  implementation determines how each configured stage is applied.

## Sourcecode Verification (Module: content/architecture)

- Verified files:
  - src/content/content_manager.cpp
  - src/content/content_validator.cpp
  - src/content/content_policy.cpp
  - src/content/content_security.cpp
  - src/content/mime_detector.cpp
  - src/content/pdf_processor.cpp
  - src/content/office_processor.cpp
  - src/content/ocr_processor.cpp
  - src/content/embedding_pipeline.cpp
  - src/content/deduplication_checker.cpp
- Verified architecture claims:
  - explicit ingestion/routing, validation/security, and enrichment planes
  - bounded structured failures for invalid/degraded processing paths
  - module-local orchestration for multi-format content processing

## Module Dependencies

### Direct manager dependencies
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| storage | `RocksDBWrapper` | Content metadata, blobs, and chunks |
| index | `VectorIndexManager`, `GraphIndexManager`, `SecondaryIndexManager` | Vector, graph, and full-text operations |
| security | optional `FieldEncryption` and malware-filter interfaces | Optional integrations passed to or attached to the manager |

`ContentManager`'s constructor takes the storage and three index managers; field encryption
is optional. Async processing is provided separately by `AsyncIngestionWorker`.

## Integration Points

### Storage and indexes
**Sources:** `content_manager.cpp`, `ContentManager` declaration
**Observed behavior:** `importContent()` writes content/chunk data through the supplied
storage manager and interacts with the supplied vector, secondary, and graph indexes for
the corresponding paths. This page makes no claim about independent storage sessions or
write serialization.

### Processor and stage configuration
`registerProcessor()` stores a non-null processor under its first supported category;
the current raw-blob manager implementation consults this map in its archive branch.
`ProcessorChainConfig::getEffectiveConfig()` selects an exact MIME entry, otherwise a
category entry, otherwise `default_config`; it does not merge entries. Stage defaults and
JSON forms are defined by the public configuration header.

### Threading boundary
`AsyncIngestionWorker` has worker threads and queue synchronization. The public
`ContentManager` declaration does not promise that all manager methods are thread-safe;
callers must not infer a module-wide locking guarantee from the asynchronous wrapper.

## Validation references

- [Manager contract and caveats](API.md)
- [Processor-chain contract](../../include/content/processor_chain_config.h)
- [Streaming ingestion tests](../../tests/content/test_content_streaming_ingestion.cpp)
- [Focused test registration](../../tests/content/CMakeLists.txt)