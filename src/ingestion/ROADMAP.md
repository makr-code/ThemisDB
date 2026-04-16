# Ingestion Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Roadmap-Status: current | validated: 2026-04-06 | Primary: src/ingestion/ | Secondary: docs/de/ingestion/ -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../docs/de/ingestion/README.md -->

## Current Status
v1.x – Production-grade data intake layer. All planned Phase 1–3 connectors and features are fully implemented:

**Connectors:** filesystem (HTML/XML), HuggingFace datasets, generic HTTP REST API, Kafka consumer, S3/GCS/Azure Blob, CDC (PostgreSQL logical replication), JDBC/ODBC, web crawler / sitemap, distributed multi-node coordinator (work-stealing thread pool), plugin API for third-party connectors.

**Features:** token-bucket rate limiting per source, incremental checkpointing, quarantine queue with per-document exponential back-off retry, per-source schema validation, dry-run mode, Prometheus-compatible metrics, admin API (pause/resume/quarantine), fluent IngestionBuilder, cursor-based and offset/limit pagination, OAuth 2.0 token refresh.
v1.5.x – Production-grade data intake layer. All connectors (FileSystem, HuggingFace, GenericAPI, Kafka, ObjectStorage, Database, WebCrawler, CDC) implemented and production-ready. Distributed ingestion coordinator with work-stealing pool available. Dynamic source reconfiguration, schema validation, and Plugin API complete.

## Completed ✅
- [x] GenericApiConnector – paginated JSON REST API ingestion with exponential back-off; real `curl_easy_perform` (PR #1915)
- [x] FileSystemIngester – recursive directory walk with HTML/XML extraction (pugixml)
- [x] HuggingFaceConnector – dataset split ingestion with API token auth; real libcurl implementation (Issue: #1915)
- [x] IngestionManager – parallel multi-source orchestration with thread pool
- [x] Token-bucket rate limiting per source
- [x] Incremental checkpoint-based ingestion (skip re-processing)
- [x] Quarantine queue for persistently failing documents
- [x] Pause/resume per-source control
- [x] Dry-run mode (no database writes)
- [x] Prometheus-compatible metrics export (docs_processed, errors, throughput, etc.)
- [x] Admin API (list, pause, resume, quarantine)
- [x] Fluent IngestionBuilder API
- [x] Cursor-based pagination support alongside offset/limit (Issue: #2409)
- [x] PDF and DOCX binary format ingestion via external converters (Issue: #1889)
- [x] S3 / GCS / Azure Blob object-storage source connector (`ingestion/object_storage_connector.cpp`) (Issue: #1905)
  - Providers: AWS S3, GCS, Azure Blob (compile-time SDK flags); mock-injection path for unit tests
  - 28 unit tests; path-traversal protection; JSON `text_field` extraction; credentials never logged
- [x] Kafka consumer source connector (`ingestion/kafka_connector.cpp`) (Issue: #1904)
- [x] JDBC-compatible database source connector (`ingestion/database_connector.cpp`) (Issue: #1894)
- [x] Web crawler / sitemap ingestion source (`ingestion/web_crawler_connector.cpp`) (Issue: #1895)
  - BFS crawl, sitemap, robots.txt, SSRF prevention (http/https only)
- [x] Per-source schema validation before write (`IngestionBuilder::withSchemaValidation`) (Issue: #1896)
- [x] Distributed ingestion coordinator across nodes (`ingestion/ingestion_coordinator.cpp`, work-stealing thread pool) (Issue: #1897, #1906)
- [x] Dynamic source reconfiguration without restart (`IngestionManager::reconfigureSource`) (Issue: #1900)
- [x] Change-data-capture (CDC) source for live database streams (`ingestion/cdc_connector.cpp`) (Issue: #2199; stream backend gated behind `THEMIS_ENABLE_CDC_STREAM` — see Known Issues)
- [x] Plugin API for third-party source connectors (`ingestion_manager.h`: `ConnectorPluginRegistry`, `IngestionManager::registerConnectorPlugin`, `IngestionBuilder::withPluginSource`) (Issue: #1908)
- [x] Per-document quarantine retry with exponential back-off (Issue: #1916)
- [x] Binary MIME type detection (PDF, DOCX) before dispatch to converters (Issue: #1917)
- [x] OAuth 2.0 token refresh handling within connectors (Issue: #2408)

## In Progress 🚧
*(none currently in progress)*

## Planned Features 📋

### Short-term (Next 3-6 months)
- [x] JDBC-compatible database source (Issue: #1894) → `ingestion/database_connector.cpp`
- [x] Web crawler / sitemap ingestion source (Issue: #1895) → `ingestion/web_crawler_connector.cpp`
- [x] Per-source schema validation before write (Issue: #1896) → `IngestionManager::withSchemaValidation()`

### Long-term (6-12 months)
- [x] Distributed ingestion coordinator across nodes (Issue: #1897) → `ingestion/ingestion_coordinator.cpp`
- [x] Dynamic source reconfiguration without restart (Issue: #1900) → `IngestionManager::reconfigureSource()`
### Remaining
- [x] End-to-end ingestion lineage tracking (Issue: #1901) → `IngestionLineageRecord`, `IngestionLineageStore`, `IngestionManager::enableLineageTracking()`
- [x] Unit tests coverage > 80% (Issue: #1909) — 18 focused test targets registered in tests/CMakeLists.txt
- [x] Integration tests (filesystem, HuggingFace, generic API) (Issue: #1910) — IngestionIntegrationFocusedTests added
- [x] Performance benchmarks (docs/sec, MB/sec) (Issue: #1911) — bench_ingestion_kv registered in cmake/CMakeLists.txt

## Implementation Phases

### Phase 1: Core Ingestion Infrastructure (Status: Completed ✅)
- [x] FileSystemIngester – recursive directory walk with HTML/XML extraction (pugixml)
- [x] GenericApiConnector – paginated JSON REST with exponential back-off
- [x] HuggingFaceConnector – dataset split ingestion with API token auth
- [x] IngestionManager – parallel multi-source orchestration with thread pool
- [x] IngestionBuilder – fluent configuration API
- [x] IngestionAdminApi – list, pause, resume, quarantine endpoints
- [x] Token-bucket rate limiting per source
- [x] Incremental checkpoint-based ingestion (skip re-processing)
- [x] Quarantine queue for persistently failing documents
- [x] Prometheus-compatible metrics (docs_processed, errors, throughput)

### Phase 2: HTTP Hardening & Binary Formats (Status: Completed ✅)
- [x] Replace libcurl stubs with real `curl_easy_perform` calls (`ingestion/api_connector.cpp`, `ingestion/huggingface_connector.cpp`) (Target: Q2 2026) (Issue: #1915)
- [x] Configurable TLS CA bundle path (`RetryConfig::ca_bundle_path` → `CURLOPT_CAINFO`); `CURLOPT_SSL_VERIFYPEER` always enabled; file existence validated before use (Issue: INGESTION-MISSING-001)
- [x] Per-document quarantine retry with exponential back-off (Target: Q2 2026) (Issue: #1916)
- [x] `DocumentWriteFn` injection for quarantine retry (`IngestionManager::setDocumentWriteForTesting()`) — eliminates last write stub
- [x] Binary MIME type detection (PDF, DOCX) before dispatch to converters (Target: Q2 2026) (Issue: #1917)
- [x] OAuth 2.0 token refresh handling within connectors (Target: Q3 2026) (Issue: #2408)
- [x] Cursor-based pagination support alongside offset/limit (Target: Q3 2026)

### Phase 3: Distributed Sources & Connectors (Status: Completed ✅)
- [x] Kafka consumer source connector (`ingestion/kafka_connector.cpp`) (Issue: #1904)
- [x] S3 / GCS / Azure Blob object-storage source connector (`ingestion/object_storage_connector.cpp`) (Issue: #1905)
  - Providers: AWS S3 (`THEMIS_ENABLE_S3`), GCS (`THEMIS_ENABLE_GCS`), Azure Blob (`THEMIS_ENABLE_AZURE`); graceful `CONNECTOR_NOT_SUPPORTED` when no SDK is compiled
  - Features: prefix filtering, `max_keys` cap, JSON `text_field` extraction, path-traversal rejection, `RetryConfig` passthrough, throughput metrics
  - Test coverage: 28 unit tests via mock-injection in `tests/test_ingestion_object_storage.cpp` (no cloud credentials required)
  - Security: `..` path components rejected; credentials never logged
- [x] Dedicated S3-compatible source connector (`ingestion/s3_connector.cpp`) (Issue: #178, v1.7.0)
  - Incremental mode: `IngestionCheckpoint::cursor` stores last processed key; `ListObjectsV2` `StartAfter` used on restart
  - Configurable `max_keys_per_list` (default 1 000) and `max_concurrent_downloads` (default 4)
  - Flat-file format delegation: `.jsonl`, `.csv`, `.parquet`, `.json`, `.txt`, `.html`, `.xml` written to temp file and parsed by `FileSystemIngester`
  - `IngestionManager` routes `provider == "s3"` to `S3Connector`; GCS/Azure continue using `ObjectStorageConnector`
  - Test coverage: 32 unit tests via mock-injection in `tests/test_s3_connector.cpp` (no cloud credentials required)
  - Security: path-traversal guard (`..` rejection); credentials never logged
- [x] Distributed ingestion coordinator across nodes (work-stealing thread pool) (Issue: #1906) → `ingestion/ingestion_coordinator.cpp`
- [x] Change-data-capture (CDC) source for live database streams (Issue: #2199)
  - `ingestFromStream()` implemented using PostgreSQL logical replication protocol (libpq, `test_decoding` output plugin) under `THEMIS_ENABLE_CDC_STREAM`
  - `isAvailable()` performs a live `IDENTIFY_SYSTEM` check when `THEMIS_ENABLE_CDC_STREAM` is set
  - Replication slot created automatically on first use (`CREATE_REPLICATION_SLOT ... IF NOT EXISTS LOGICAL test_decoding`)
  - WAL polling loop with exponential-timeout drain (3 consecutive empty polls), LSN tracking, and Standby Status Update feedback
  - Supports `table_filter`, `operations`, `text_columns`, `max_events`, `poll_timeout_ms`, `from_lsn` options
  - Mock-injection path (`setCdcEventFetchForTesting`) for unit tests without a live database
- [x] JDBC-compatible database source connector (`ingestion/database_connector.cpp`) (Issue: #1894)
  - Full ODBC-backed implementation under `THEMIS_ENABLE_ODBC`; mock-injection path for unit tests
  - JDBC URL parsing (PostgreSQL, MySQL, SQL Server, SQLite); `text_columns` extraction; credential masking in error messages
- [x] Web crawler / sitemap ingestion source (`ingestion/web_crawler_connector.cpp`) (Issue: #1895)
  - Full libcurl-backed crawler under `THEMIS_ENABLE_CURL`; SSRF protection (private IP allowlist); sitemap XML parsing; depth-limited BFS; mock-injection path
- [x] Per-source schema validation before write – `IngestionManager::withSchemaValidation()` / `buildValidatorFromSchema()` (Issue: #1896)
  - JSON field existence / type / pattern rules; `reject_invalid` flag; `schema_violations` metric counter
- [x] Plugin API for third-party source connectors (`ingestion_manager.h`: `ConnectorPluginRegistry`, `IngestionManager::registerConnectorPlugin`, `IngestionBuilder::withPluginSource`) (Issue: #1908)

### Phase 4: Observability & Lineage (Status: Completed ✅)
- [x] End-to-end ingestion lineage tracking (Issue: #1901)
  - `IngestionLineageRecord` struct: `run_correlation_id`, `source_id`, `connector_type`, `connector_version`, `doc_id`, `ingested_at`, `bytes`, `doc_count`, `transformation_steps`, `status`
  - `IngestionLineageStore` class: thread-safe in-memory store; `record()`, `getBySource()`, `getByCorrelationId()`, `getAll()`, `clear()`, `size()`
  - `LineageStatus` enum: `SUCCESS`, `FAILED`, `QUARANTINED`, `DRY_RUN`
  - `IngestionManager` API: `enableLineageTracking()`, `isLineageTrackingEnabled()`, `getLineageRecords(source_id)`, `getLineageRecordsByRun(correlation_id)`, `getAllLineageRecords()`, `clearLineageRecords()`
  - Tracking is opt-in (disabled by default); one batch record per successful source run + per-quarantine-entry records
  - Transformation steps auto-detected: `schema_validation`, `mime_detection`, `rate_limiting`, `incremental_checkpoint`, `dry_run`, `deontic_extraction`, `semantic_validation`, `reference_validation`
  - Test coverage: `tests/test_ingestion_lineage.cpp` (19 tests) → `IngestionLineageFocusedTests`

### Phase 5: LLM-Driven Semantic Extraction for Legal Texts (Status: Phase 1 ✅ / Phase 2 Planned 📋)

**Phase 1 (Completed ✅):**
- [x] `config/ingestion/legal-ingestion-schema.yaml` — YAML schema with deontic rules, entity patterns, temporal patterns, quality gates, and agentic verification agents
- [x] `DeonticExtractor` (`include/ingestion/deontic_extractor.h`, `src/ingestion/deontic_extractor.cpp`) — regex-based deontic logic extraction for 7 categories (obligation, permission, prohibition, definition, condition, exception, reference); entity extraction (law_reference, person_role, organization, temporal, threshold_value); injectable extractor function for LLM Phase 2
- [x] `SemanticValidator` (`include/ingestion/semantic_validator.h`, `src/ingestion/semantic_validator.cpp`) — quality gates (min_confidence, deontic_confidence, section_hierarchy, temporal_present, no_dangling_refs); semantic scoring; document-level extraction with per-section splitting; injectable validator function
- [x] `AgenticReferenceValidator` (`include/ingestion/agentic_reference_validator.h`, `src/ingestion/agentic_reference_validator.cpp`) — regex-based reference extraction (§ N Abs. M, named-law, Article, EU directive); knowledge-base lookup; dangling reference detection and reporting; injectable extractor function
- [x] `LegalIngestionConfig` struct in `ingestion_manager.h` — enabled, confidence_threshold, validate_references, require_section_struct, flag_low_confidence
- [x] `IngestionManager::setLegalIngestionConfig()` / `getLegalIngestionConfig()` — per-source legal pipeline registration
- [x] `IngestionManager::runLegalExtraction()` — ad-hoc extraction on a single document
- [x] `IngestionBuilder::withLegalIngestionConfig()` — fluent builder support
- [x] Lineage tracking extended: `deontic_extraction`, `semantic_validation`, `reference_validation` steps recorded when legal pipeline is active
- [x] Unit tests: `tests/test_legal_extraction.cpp` (80+ tests) → `LegalExtractionFocusedTests`
- [x] Documentation: `docs/guides/legal-text-ingestion.md`, `examples/legal-ingestion-example.md`

**Phase 2 (Planned 📋 — Target: Q3 2026):**
- [x] `LegalLlmAdapter` unit tests registered: `tests/test_ingestion_llm_adapter.cpp` → `IngestionLlmAdapterFocusedTests`; CI: `ingestion-llm-adapter-ci.yml`
- [ ] LoRA adapter training pipeline for German legal texts (BImSchG, StGB, DSGVO)
- [ ] Mistral 7B integration via llama.cpp (injectable via `DeonticExtractor::setExtractorFn()`)
- [ ] SpaCy `de_legal_ner` custom model for `DeonticExtractor::extractEntities()`
- [ ] Full agentic verification loop (deontic consistency checker, temporal validator, hierarchy fixer)
- [ ] Multi-law knowledge base populated from gesetze-im-internet.de
- [ ] Performance benchmarks: extraction throughput, LLM latency

## Production Readiness Checklist
- [x] Unit tests coverage > 80% (Issue: #1909) — 19 focused test targets in tests/CMakeLists.txt
- [x] Integration tests (filesystem, HuggingFace, generic API) (Issue: #1910) — IngestionIntegrationFocusedTests
- [x] Performance benchmarks (docs/sec, MB/sec) (Issue: #1911) — bench_ingestion_kv in cmake/CMakeLists.txt
- [x] Security audit (path traversal, API key storage) (Issue: #1912)
- [x] Documentation complete (Issue: #1913)
- [x] API stability guaranteed (Issue: #1914) — `IngestionBuilder` fluent API stable from v1.x; connector interface locked

## Known Issues & Limitations
- PDF/DOCX ingestion requires external converters (pdftotext, pandoc); not handled natively.
- `CdcConnector`: full replication driver requires `THEMIS_ENABLE_CDC_STREAM` at compile time; without it, `ingestFromStream()` returns `CONNECTOR_NOT_SUPPORTED`. The source file always compiles (uses `#ifdef` internally).

---

## Ingestion v2.0 — Universal File Ingestion with Workflow Orchestration (Target: v2.0.0)

> First use case: **Legal Documents** (Gesetze, Verordnungen, Bescheide, Vorschriften)

### Phase 1: Foundation — Core Data Structures + WorkflowEngine (Target: v2.0.0)
- [x] `IIngestionStep` interface + `PluginType::INGESTION_STEP` — `include/ingestion/ingestion_step.h` (2026-04-15)
- [x] `ExtractionContext` struct — `include/ingestion/extraction_context.h` (2026-04-15)
- [x] `FileManifest` struct — `include/ingestion/file_manifest.h` (2026-04-15)
- [x] `BaseEntity`, `EntityRelation`, `VectorRecord`, `BaseEntitySet` — `include/ingestion/base_entity.h` (2026-04-15)
- [x] `StepRegistry` (thread-safe, supports dlopen) — `include/ingestion/workflow_engine.h` (2026-04-15)
- [x] `WorkflowEngine` (YAML/JSON profile loading, profile selection, step execution) — `include/ingestion/workflow_engine.h` + `src/ingestion/workflow_engine.cpp` (2026-04-15)
- [x] `IngestionManager::setWorkflowEngine()` / `getWorkflowEngine()` — `include/ingestion/ingestion_manager.h` (2026-04-15)
- [x] `ERR_WORKFLOW_*` error codes 9600–9619 — `include/utils/error_registry.h` (2026-04-15)
- [x] `FileFormat` extended: EPUB, XLSX, CSV, ZIP, SHP, GEOJSON, DXF, PNG, JPG, MD, DB — `include/ingestion/filesystem_ingester.h` (2026-04-15)
- [x] `PluginType::INGESTION_STEP` added — `include/plugins/plugin_interface.h` (2026-04-15)
- [x] Tests: SR-01..SR-05 (StepRegistry), WE-01..WE-15 (WorkflowEngine) — `tests/test_workflow_engine.cpp` (2026-04-15)
- [x] Tests: FM-01..FM-03, EC-01..EC-07, BA-01..BA-10 — `tests/test_ingestion_base_entity.cpp` (2026-04-15)

### Phase 2: Builtin Steps (Target: v2.0.0)
- [x] `builtin.parse_text` — `src/ingestion/steps/parse_text_step.cpp` (2026-04-15)
- [x] `builtin.chunk_text` — fixed / sentence / §-aware section strategy — `src/ingestion/steps/chunk_text_step.cpp` (2026-04-15)
- [x] `builtin.legal_metadata` — regex norm/date/Aktenzeichen extractor — `src/ingestion/steps/legal_metadata_step.cpp` (2026-04-15)
- [x] `builtin.deontic_extractor` — wraps `DeonticExtractor` + `SemanticValidator` — `src/ingestion/steps/deontic_step.cpp` (2026-04-15)
- [x] `builtin.base_entity_assembler` — dedup + canonical-ID finalisation — `src/ingestion/steps/base_entity_assembler_step.cpp` (2026-04-15)
- [ ] `builtin.decompress` — ZIP/tar/gzip unpack, recursive re-ingestion (Target: Q3 2026)
- [ ] `builtin.legal_reference_extractor` — wraps `AgenticReferenceValidator` as a Step (Target: Q3 2026)
- [ ] `builtin.chunk_embed` — text chunk → vector (ONNX-CLIP / multilingual-E5) (Target: Q3 2026)

### Phase 3: YAML Workflow Profiles (Target: v2.0.0)
- [x] `config/ingestion/workflows/legal-document-de.json` — Pilot: German legal documents (2026-04-15)
- [x] `config/ingestion/workflows/default.json` — Fallback for unknown file types (2026-04-15)
- [x] `config/ingestion/workflows/geo-data.json` — SHP/GeoJSON/KML (2026-04-15)
- [x] `config/ingestion/workflows/image-document.json` — PNG/JPG + OCR + CLIP (2026-04-15)
- [x] `config/ingestion/workflows/spreadsheet.json` — XLSX/CSV (2026-04-15)
- [ ] Migrate YAML format with native yaml-cpp parser (currently JSON subset only) (Target: Q3 2026)
- [ ] DLL step plugin sandbox (manifest `allowedPaths`, `allowedMime`) (Target: Q3 2026)

### Phase 4: NER + LLM Integration (Status: Completed ✅)
- [x] `builtin.ner_de` — NER via `ITextGenerationBackend` (spaCy-wrapper or LLM-based) — `src/ingestion/steps/ner_step.cpp` (2026-04-15)
- [x] `builtin.llm_extract` — generic LLM step with prompt template from YAML — `src/ingestion/steps/llm_extract_step.cpp` (2026-04-15)
- [x] LLM-based deontic analysis (`use_llm: true` in deontic_extractor step config) (2026-04-15)
- [x] Multilingual support: `language: de | en | fr | ...` profile parameter (2026-04-15)
- [x] Tests: NE-01..NE-08, LE-01..LE-06 — `tests/test_ingestion_ner_llm.cpp` (2026-04-15)

### Phase 5: BaseEntity Sink — Graph + Vector (Status: Completed ✅)
- [x] `EntityNormalizer` — canonical-ID generation for legal provisions (`law:<norm>:§<n>:Abs<m>`) — `include/ingestion/entity_assembler.h` (2026-04-15)
- [x] `RelationBuilder` — cross-refs → `CITES`, `AMENDS`, `SUPERSEDES` edges — `include/ingestion/entity_assembler.h` (2026-04-15)
- [x] `IGraphWriter` / `InMemoryGraphWriter` — `BaseEntitySet.nodes/edges` → graph store — `include/ingestion/ingestion_sinks.h` (2026-04-15)
- [x] `IVectorWriter` / `InMemoryVectorWriter` — `BaseEntitySet.chunks` → vector index — `include/ingestion/ingestion_sinks.h` (2026-04-15)
- [x] `DocumentStoreSinkAdapter` — production `IDocWriter` backed by `IDocumentStore` (Phase 5 remainder) — `include/ingestion/ingestion_sinks.h` (2026-04-15)
- [x] Integration with `IDocumentStore` (document module) via `DocumentStoreSinkAdapter` (2026-04-15)
- [x] Tests: BA-01..BA-08, GW-01..GW-05, VW-01..VW-05 + DS-01..DS-05 — `tests/test_ingestion_assembler_sinks.cpp`, `tests/test_ingestion_legal_domain.cpp` (2026-04-15)

### Phase 6: Legal Domain Specialisation (Status: Completed ✅)
- [x] `GesetzParser` — Teil → Abschnitt → § recursive hierarchy — `include/ingestion/legal_domain.h` (2026-04-15)
- [x] `TemporalExtractor` — `effective_from` / `effective_to` extraction from metadata + text — `include/ingestion/legal_domain.h` (2026-04-15)
- [x] `BehoerdenMapper` — norm reference → responsible authority (30 built-in + injectable fallback) — `include/ingestion/legal_domain.h` (2026-04-15)
- [x] `BescheidExtractor` — `Aktenzeichen`, `Antragsteller`, `Bescheiddatum`, `Auflagen`, `Nebenbestimmungen` — `include/ingestion/legal_domain.h` (2026-04-15)
- [x] `CrossDocumentLinker` — § X Gesetz Y → § Z Gesetz W graph edges across files — `include/ingestion/legal_domain.h` (2026-04-15)
- [x] `LegalEntityExport` — JSON-LD + Turtle/N-Triples RDF for juris / EUR-Lex compatibility — `include/ingestion/legal_domain.h` (2026-04-15)
- [x] Tests: LD-01..LD-15 + DS-01..DS-05 — `tests/test_ingestion_legal_domain.cpp` (2026-04-15)

### Phase 7: LLM-as-judge Runtime Quality Control (Status: Completed ✅)
- [x] `IngestionJudgeConfig` — per-dimension thresholds, LLM params, re-ingestion limits — `include/ingestion/ingestion_quality_judge.h` (2026-04-15)
- [x] `IngestionQualityReport` — completeness/groundedness/entity_coverage/relation_coherence scores, missing_entities, ungrounded_claims, recommended_steps, rationales — `include/ingestion/ingestion_quality_judge.h` (2026-04-15)
- [x] `IIngestionQualityObserver` — noexcept callbacks: onQualityEvaluated / onReIngestionTriggered / onReIngestionComplete — `include/ingestion/ingestion_quality_judge.h` (2026-04-15)
- [x] `IngestionQualityJudge` — stateless LLM-prompt builder + score parser + observer dispatch; thread-safe; fail-open when backend unavailable — `include/ingestion/ingestion_quality_judge.h` + `src/ingestion/ingestion_quality_judge.cpp` (2026-04-15)
- [x] `ReIngestionController` — feedback loop: run → evaluate → re-ingest until quality passes or max_reingestion_attempts exhausted; persists best-quality context — `include/ingestion/ingestion_quality_judge.h` + `src/ingestion/ingestion_quality_judge.cpp` (2026-04-15)
- [x] `IngestionManager::setReIngestionController()` / `getReIngestionController()` — `include/ingestion/ingestion_manager.h` + `src/ingestion/ingestion_manager.cpp` (2026-04-15)
- [x] Tests: IJ-01..IJ-08, QR-01..QR-04, RC-01..RC-06, IM-01..IM-02 (20 tests) — `tests/test_ingestion_quality_judge.cpp` (2026-04-15)
- [x] Stub/mock audit: `NullTextGenerationBackend`, `InMemoryGraphWriter/VectorWriter/DocWriter`, `InMemorySharedCheckpointStore`, all connector `ingestFromMock` paths (cdc/database/kafka/s3/object_storage), `TextProcessor::generateEmbedding` — all upgraded to canonical `STUB/SIMULATION NOTE:` format (2026-04-15)
- [x] Google Benchmarks: QJ01..QJ11 — `benchmarks/bench_ingestion_quality_judge.cpp` — fail-open path, single/all-dim eval, entity scaling, bullet-list parsing, observer dispatch (0/N), config mutation, ctor/dtor, CRAG-style feedback loop simulation — registered in `benchmarks/CMakeLists.txt` (2026-04-15)
- [x] Header documentation: scientific paper references (LLM-as-judge Zheng et al. NeurIPS 2023, RAGAS Es et al. EACL 2024, CRAG Yan et al. ICLR 2024) and full SOLID / SoC annotation added to `include/ingestion/ingestion_quality_judge.h` (2026-04-15)

### Phase 8: Global Toolbox (Status: Completed ✅)
- [x] `IngestionToolbox` — injectable system-wide service wrapping `WorkflowEngine` + `StepRegistry` + `ITextGenerationBackend` — `include/toolbox/ingestion_toolbox.h` + `src/toolbox/ingestion_toolbox.cpp` (namespace `themis::toolbox`) (2026-04-15)
  - `createDefault()` factory pre-registers `builtin.ner_de` + `builtin.llm_extract`
  - `setWorkflowEngine()` / `setTextBackend()` for DI (no singleton)
  - `extractEntities(text, mime, filename)` convenience method
- [x] `AQLIngestionBridge` — bridge in `aql/` consuming `toolbox/`; `ingestion/` never imported from `aql/` — `include/aql/aql_ingestion_bridge.h` + `src/aql/aql_ingestion_bridge.cpp` (2026-04-15)
  - `enrichInsertPayload(json&)` — WorkflowEngine on INSERT/UPSERT payload → `_entities` + optional graph write
  - `extractEntitiesForContext(text)` → `vector<BaseEntity>` for NL→AQL schema context injection
  - `buildEntityContext(entities)` → compact entity string for LLM prompt enrichment
- [x] `LLMAQLHandler::setIngestionBridge()` / `ingestionBridge()` — opt-in bridge injection (2026-04-15)
- [x] `AQLQueryBuilder::withIngestionEnrichment()` / `hasIngestionEnrichment()` — advisory DML enrichment flag (2026-04-15)
- [x] Tests: IT-01..IT-10, AB-01..AB-10, QB-01..QB-04, LH-01..LH-03 (27 tests) — `tests/test_toolbox_ingestion.cpp` (2026-04-15)
- [x] ARCHITECTURE.md updated — "Global Ingestion Toolbox" section + `toolbox::` in namespace hierarchy

## Breaking Changes
- `IngestionBuilder` fluent API is stable from v1.x.
- Source connector interface may gain new lifecycle hooks in v1.6.0.
- `FileFormat` enum extended in v2.0.0 — switch statements that previously covered all enum values must add cases for: `MD`, `EPUB`, `XLSX`, `CSV`, `ZIP`, `SHP`, `GEOJSON`, `DXF`, `PNG`, `JPG`, `DB`.
- `PluginType` enum extended with `INGESTION_STEP` — any serialisation/deserialisation of `PluginType` strings must handle `"ingestion_step"`.