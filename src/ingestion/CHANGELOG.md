> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Ingestion Module

All notable changes to the Ingestion module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]
### Fixed
- **Ingestion reliability hardening (rest block)** (`src/ingestion/**/*.cpp`)
  - Replaced all remaining `catch (...)` handlers in Ingestion implementation files with typed `catch (const std::exception&)`.
  - Targeted delta in this block: **44 → 0** across 14 files (`ingestion_manager`: 6→0, `cdc_connector`: 5→0, plus 12 additional Ingestion files).

- Ingestion catch-all hardening (Phase 23, 2026-05-19): replaced all remaining 44 `catch(...)`
  handlers with typed `catch (const std::exception&)` across 14 files; zero catch-all handlers
  remain in `src/ingestion/*.cpp`.
- Phase 2 LLM pipeline: LoRA fine-tuning integration, SpaCy NLP pipeline, agentic verification loop
- Extended binary MIME detection (XLSX, ODT, RTF)
- Distributed checkpoint store (etcd-backed)

## [1.5.1] — 2026-03-21
### Added
- `tests/test_ingestion_llm_adapter.cpp` registered as `IngestionLlmAdapterFocusedTests` in `tests/CMakeLists.txt` (Phase 1 + Phase 2 unit tests for `LegalLlmAdapter`)
- CI workflow `.github/workflows/ingestion-llm-adapter-ci.yml` — runs `IngestionLlmAdapterFocusedTests` on ubuntu-22.04 (gcc-12, clang-15) and ubuntu-24.04 (gcc-14)

## [1.5.0] — 2026-03-12
### Added
- LLM-driven deontic extraction for legal texts (`DeonticExtractor`, `SemanticValidator`, `AgenticReferenceValidator`)
- Lineage tracking: per-record provenance metadata stored alongside ingested documents
- `IngestionBuilder` fluent API for programmatic pipeline construction
- Dry-run mode: validates connectors and schemas without persisting data
- Prometheus metrics endpoint: ingestion throughput, queue depth, error rates, retry counts
- Admin REST API: pause/resume individual connectors, drain quarantine queue
- OAuth 2.0 token refresh with automatic retry on 401 responses
- Binary MIME detection for PDF and DOCX payloads prior to schema validation

### Changed
- Token-bucket rate limiter is now connector-scoped (previously global)
- Incremental checkpoint interval made configurable per connector (default 5 000 records)
- Quarantine retry back-off ceiling raised from 60 s to 300 s

### Fixed
- CDC connector: logical replication slot leak on reconnect
- WebCrawler: infinite redirect loop when Location header echoes request URL
- ObjectStorage connector: multipart upload left incomplete on cancellation

## [1.4.0] — 2025-09-01
### Added
- `DistributedCoordinator` with work-stealing thread pool for parallel shard ingestion
- S3, GCS, and Azure Blob connectors (unified `ObjectStorageConnector`)
- CDC connector: PostgreSQL logical replication via pgoutput protocol
- JDBC/ODBC database connector with cursor-based pagination
- WebCrawler connector with robots.txt compliance and politeness delay
- Per-source schema validation with JSON Schema (draft-07)

### Changed
- Kafka connector upgraded to librdkafka 2.x API
- HuggingFace connector migrated from datasets REST v1 to v2

### Fixed
- FileSystem ingester: symlink cycle detection
- GenericAPI connector: connection pool exhaustion under burst traffic

## [1.3.0] — 2025-03-01
### Added
- Kafka connector with consumer group management and offset commit
- Exponential back-off retry with jitter; failed records routed to quarantine queue
- HTML and XML parsing via pugixml in `FilesystemIngester`

### Fixed
- HuggingFace connector: token expiry not detected before first request

## [1.2.0] — 2024-09-01
### Added
- HuggingFace datasets connector (streaming and batch modes)
- `GenericAPI` connector backed by libcurl with configurable headers and auth
- Credential masking in structured log output

## [1.1.0] — 2024-04-01
### Added
- `FilesystemIngester`: recursive directory scan, file-type filtering
- Token-bucket rate limiter (global)
- Incremental checkpointing to local RocksDB store

## [1.0.0] — 2024-01-01
### Added
- Initial implementation of the Ingestion module
- Core `IngestionManager` and `IngestionCoordinator` scaffolding
- Basic file-system and HTTP connectors
