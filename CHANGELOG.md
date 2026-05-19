# Changelog

All notable changes to ThemisDB will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Released
- **v1.9.0-alpha (2026-04-26) veröffentlicht**
  - GitHub Pre-Release: https://github.com/makr-code/ThemisDB/releases/tag/v1.9.0-alpha
  - Assets: `ThemisDB-COMMUNITY-1.9.0-alpha-windows-x64.zip` und `ThemisDB-COMMUNITY-1.9.0-alpha-windows-x64.msi`

### Fixed

- **CONTENT module — Phase 3.1–5 quality remediation** (`src/content/`, `tests/`, related docs)
  - **CON-007 — `VideoProcessor::healthCheck()` correctness**: `#else` (no-FFmpeg) branch was returning `initialized_` (always `true` after `initialize()`). Now returns `false` to surface the missing FFmpeg dependency to health-check aggregators, consistent with `TTSProcessor` and `STTProcessor`.
  - **CON-008 — `extractMetadata()` simulation stub documentation**: no-FFmpeg fallback in `video_processor.cpp` had no STUB/SIMULATION NOTE and contained an unreachable MKV-detection branch (identical EBML magic bytes as the WebM branch). STUB/SIMULATION NOTE added; dead branch removed.
  - **CON-009 — RAII fix: raw `new`/`delete` for tags JSON in metadata encryption** (`content_manager.cpp:901-950`): The "tags" field encryption loop allocated a temporary `nlohmann::json` on the heap via `new` and performed manual `delete` calls in every exit path (early continue, success, exception). Replaced with a local RAII variable (`tags_tmp`) that is automatically destroyed at end of scope. Eliminates three raw `delete` call sites and removes exception-unsafe `delete` in the catch block.
  - **CON-010 — RAII fix: `EVP_MD_CTX` manual cleanup in `content_fs.cpp::sha256Hex()`**: 4 raw `EVP_MD_CTX_free()` early-return call sites replaced with `std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>` RAII wrapper. Context is freed automatically on all exit paths (normal return, early return on error, and future exceptions).
  - **CON-011 — RAII fix: `EVP_MD_CTX` manual cleanup in `content_manager.cpp::ingestStream()`**: 3 raw `EVP_MD_CTX_free(sha256_ctx); sha256_ctx = nullptr;` call sites (init-fail, update-fail in loop, finalize) replaced by `unique_ptr::reset()`. Eliminates the exception-unsafe leak path in the streaming ingest path.
  - **Redundant `file.close()` removal** (`content_manager.cpp` archive ingestion loop): explicit `file.close()` removed; `std::ifstream` RAII already guarantees closure at end of loop iteration.
  - **CON-012 — exception-in-destructor: `AsyncIngestionWorker::~AsyncIngestionWorker()`**: destructor called `stop()` without exception guard; if `stop()` throws (mutex op or `promise::set_exception` during stack unwinding), `std::terminate()` would be invoked. Destructor is now `noexcept` with a `try/catch(...)` guard around `stop()`.
  - **CON-013 — scanner false positive: `executeWithRetry` loop `<= max_retries`**: static scanner flagged `for (i <= max_retries)` as `OFF_BY_ONE`; loop is correct (`max_retries=0` → one initial attempt). Clarifying comment added to suppress future false-positive reports.
  - **Phase 4 unit tests** (`tests/test_content_con007_con012_remediations.cpp`): 5 regression tests added:
    - `VideoProcessorHealthCheckCON007/NotInitialized_AlwaysUnhealthy`
    - `VideoProcessorHealthCheckCON007/SimulationMode_InitializedButUnhealthy` (CON-007 core regression)
    - `VideoProcessorHealthCheckCON007/FFmpegMode_InitializedAndHealthy` (CON-007 guard)
    - `VideoProcessorHealthCheckCON007/AfterShutdown_AlwaysUnhealthy`
    - `AsyncIngestionWorkerCON012/DestructorIsNoexcept` (`std::is_nothrow_destructible` compile-time check)
  - **Phase 4 test fix**: `VideoProcessorExtendedTest::HealthCheck` in `test_video_processor_extended.cpp` updated to expect `false` in no-FFmpeg builds and `true` in FFmpeg builds.
  - **CON-014 — Uninitialized `IngestionJob` POD fields** (`async_ingestion_worker.h`): `created_at`, `started_at`, `completed_at`, `total_items`, `processed_items`, `progress` had no in-class defaults. Added `= 0` / `= 0.0f` defaults; removed 15 redundant explicit zero-assignment lines across 4 call sites in `async_ingestion_worker.cpp`.
  - **CON-015 — Uninitialized `QItem::hop`** (`content_manager.cpp`): local struct `QItem` had `int hop` without initializer; added `= 0`.
  - **CON-016 — Atomic stat members without in-class defaults** (`async_ingestion_worker.h`): added `{0}` in-class initializers to 5 `std::atomic` member variables; guards against future delegating constructors.
  - **CON-017 — Uninitialized `ExtractionResult` fields** (`content_processor.h`): `ok = false`; `MediaData::{duration_seconds, width, height, bitrate} = 0`.
  - **CON-018 — Uninitialized `IngestResult::success` and `Stats` fields** (`content_manager.h`): `success = false`; `Stats` int/int64_t fields `= 0`.
  - **CON-019 — Uninitialized archive structs** (`archive_processor.h`): `ArchiveMember`, `ArchiveMetadata`, `ArchiveExtractionResult`, `ArchiveProcessorResult` POD fields all have in-class defaults.
  - **CON-020 — Uninitialized PDF structs** (`pdf_processor.h`): `PDFPageInfo` and `PDFMetadata` POD fields default to `0`/`false`.
  - **CON-021 — Uninitialized office structs** (`office_processor.h`): `WordDocumentInfo`, `Sheet`, `Slide`, `OfficeMetadata` int fields default to `0`.
  - **CON-022 — OOP: `[[nodiscard]]` on result-returning methods** (`content_manager.h`): `importContent`, `ingestRawBlob`, `ingestStream`, `deleteContent`, `createDirectory`, `registerPath`.
  - **CON-023 — Uninitialized `GeoExtractionData::bounds`** (`content_plugin_interface.h`): `std::array<double,4>` zero-initialised with `= {}`.
  - **CON-024 — Uninitialized `CADExtractionData::bounding_box_min/max`** (`content_plugin_interface.h`): Both `std::array<double,3>` fields zero-initialised with `= {}`.
  - **CON-025 — OOP: `[[nodiscard]]` on `IContentProcessorPlugin::getStatistics()`** (`content_plugin_interface.h`): diagnostic added.
  - **CON-026 — OOP: `[[nodiscard]]` on `IContentProcessorPlugin::generateEmbedding()`** (`content_plugin_interface.h`): completes `[[nodiscard]]` coverage for all value-returning virtual methods in the plugin interface.
  - **MODULE_GAPS.md populated**: was a boilerplate placeholder; now contains gap-scan v3 results (4,077 items), categorized by severity and file, with a Phase 3.1–7 implementation roadmap.
- **CONTENT module — Phase 8 quality remediation** (`src/content/video_processor.cpp`, `src/content/MODULE_GAPS.md`)
  - **CON-027 — Multiplication overflow in `VideoProcessor` thumbnail resize** (`video_processor.cpp:743`):
    `thumbnail.resize(thumb_width * thumb_height * 3)` performed three-factor `int` multiplication
    (signed, CWE-190). If thumbnail dimensions are configured above ~26 755 px, the product
    overflows before the implicit widening to `size_t`, yielding garbage allocation size.
    **Fix:** `thumbnail.resize(static_cast<size_t>(thumb_width) * static_cast<size_t>(thumb_height) * 3u)`.
  - **False-positive audit — `type_conversion` (516 scanner hits):** Full scanner re-run shows
    314 SHIFT_OVERFLOW (stream `<<` operator misidentified as bit-shift), 93 ARITHMETIC_OVERFLOW
    (normal arithmetic), 87 CAST_TO_SMALLER_TYPE (`static_cast<int>` is the correct fix, not
    a new problem), and 7 MULTIPLICATION_OVERFLOW (only 1 real — CON-027). No bare
    `int x = vec.size()` narrowing exists in the module. Remaining 515 scanner hits are closed
    as false positives.
  - **False-positive audit — `oop_design` (1,479 scanner hits):** 1,442 VIRTUAL_IN_CTOR_DTOR
    (scanner misreads virtual declarations as calls inside constructors), 17 MISSING_OVERRIDE
    (scanner flags pure-virtual base class declarations, not derived-class implementations),
    6 PURE_VIRTUAL_UNIMPLEMENTED (intentional interface contracts). All concrete processor
    subclasses already use `override` throughout. All 1,479 hits are closed as false positives.

### Added
- **HammingCoder — RAID-2 / Hamming Shard-Level Error Correction** (`include/sharding/redundancy_strategy.h`, `src/sharding/redundancy_strategy.cpp`)
  - `HAMMING` added to `ErasureCodingAlgorithm` enum; `ErasureCoder::create(HAMMING)` factory method returns a `HammingCoder` instance
  - `HammingCoder::encode()`: systematic XOR-based parity; parity shard _p_ covers data shard _j_ when bit _p_ of (_j_+1) is set — classical Hamming assignment at block granularity
  - `HammingCoder::decode()`: iterative XOR repair; recovers all shards whose parity coverage allows; `std::runtime_error` on irrecoverable failure sets
  - No Galois-Field arithmetic — purely XOR-based; O(k × r × shard_size) encode/decode
  - 16 focused tests in `tests/test_hamming_coder.cpp` (HC_01..HC_16): chunk invariants, single/multi-shard failure, canonical Hamming(7,4) coverage verification, 1 MB round-trip, edge cases
  - `HammingCoderFocusedTests` CTest target registered

### Security

- **Task Scheduler AuthZ Hardening (GAP-001)** 🔐
  - Activated runtime permission checks in `TaskScheduler` for:
    - `registerTask()` → requires `task:register`
    - `executeTaskNow()` / `executeDAG()` → requires `task:execute`
    - `registerFunction()` → requires `task:register_function` and `system_admin` role
  - Added denied-access security audit events (`UNAUTHORIZED_ACCESS`) with structured
    justification metadata (`required_permission`, `reason`, `justification`).
  - `HttpServer` task create/execute routes now propagate authenticated request context
    (user, IP, permissions, roles, justification) into `TaskScheduler` thread-local context.
  - `TaskSchedulerApiHandler::executeTask()` now returns `status=error` when scheduler
    execution is rejected (e.g., missing permission), instead of reporting `executed`.
- **JWT/JWKS cache synchronization hardening** 🔒
  - `JWTValidator::fetchJWKS()` now guarantees `jwks_refreshing_` reset via RAII even on exceptional exits,
    preventing stuck refresh state under parallel validation/key-fetch error paths.
  - Added explicit header includes for `std::mutex` / `std::condition_variable` in
    `include/auth/jwt_validator.h` to keep synchronization primitives self-contained.

- **Docker Image Security Hardening** 🔒
  - `THEMIS_ENABLE_ENCRYPTED_STORAGE` Build-ARG hinzugefügt (default: `OFF`):
    `gocryptfs` und `fuse` werden nur noch installiert, wenn der ARG explizit auf `ON` gesetzt wird.
    Dadurch entfällt Go-stdlib 1.22.2 aus dem Standard-Runtime-Image.
  - `tar` wird nach Paketinstallation aus dem Runtime-Image entfernt (`apt-get purge -y --auto-remove tar`).
  - CVE-Scan (Docker Scout) vor und nach den Änderungen: 39 CVEs (inkl. 3 CRITICAL, 11 HIGH)
    → 3 CVEs verbleibend (alle LOW/MEDIUM, kein upstream-Fix verfügbar).
  - Community-Image auf DockerHub veröffentlicht: `themisdb/themisdb:latest` und `themisdb/themisdb:1.8.1-rc1`.
  - Verbleibende CVEs: CVE-2024-2236 (libgcrypt20, LOW), CVE-2024-56433 (shadow, LOW),
    CVE-2025-45582 (tar, MEDIUM) — alle ohne upstream-Fix; Waiver dokumentiert in
    `docs/audit-reports/cve-waivers.md`.

### Testing

- **Integration Pipeline Coverage Expansion** 🧪
  - **Application Resilience Scenarios (APP-11..APP-13)** — `tests/integration/pipeline/application_profile_pipeline_test.cpp`
    - APP-11: Timeout → retry/fallback behavior; validates assistant request retry logic under LLM timeout conditions
    - APP-12: Retry-budget exhaustion behavior; verifies fallback activation when retry budget exhausted
    - APP-13: Circuit-breaker open-state enforcement; ensures circuit breaker blocks requests after repeated assistant failures
  - **Security Hardening Scenarios (SEC-04..SEC-06)** — `tests/integration/pipeline/security_pipeline_test.cpp`
    - SEC-04: RBAC-denied request path (403 + audit/no info leakage); validates denied requests return 403 with audit trail and no sensitive data
    - SEC-05: Multi-rotation consistency/readability checks; verifies secret rotation preserves read consistency across replicas
    - SEC-06: Revoked-token rejection with audit verification; ensures revoked tokens are rejected and audit events recorded
  - **Documentation Sync**
    - Updated `tests/integration/INTEGRATION_TEST_GUIDELINES.md` with APP/SEC scenario ranges and design patterns
    - Updated `tests/integration/ROADMAP.md` with new coverage milestones (APP-11..APP-13, SEC-04..SEC-06)
  - CTest targets: `ApplicationProfilePipelineTest`, `SecurityPipelineTest`

### Documentation

- **Root Governance Consolidation 📚 — 2026-05-13**
  - Einheitliche Terminologie auf Root-Ebene abgestimmt:
    - `ROADMAP.md` = Features/Milestones
    - `FUTURE_ENHANCEMENTS.md` = offene Enhancements/Stub-Replacements
    - `FEATURE_ENHANCEMENT.md` = generierter Reifegrad-Report (nicht backlog-führend)
    - Breaking Changes in `ROADMAP.md` + `CHANGELOG.md`
    - `VERSIONING.md` + `RELEASE_STRATEGY.md` = eindeutiges Release-Type-/Suffix-Modell (`alpha`, `beta`, `rc`, `stable`)
    - `COPILOT_INSTRUCTIONS.md` = verbindliche AI-/Agent-Regeln zur Root-Dokument-Synchronisierung
  - Release-Strategie mit Milestone-Modell synchronisiert: `RELEASE_STRATEGY.md` enthält Mapping `RELEASE_TYPE` ↔ Tag ↔ Milestone ↔ Changelog-Schnitt.
  - Traceability-Referenzen ergänzt:
    - [ROADMAP.md](ROADMAP.md) (Milestone-Planung)
    - [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md) (Enhancement-Backlog)
    - [FEATURE_ENHANCEMENT.md](FEATURE_ENHANCEMENT.md) (Maturity Snapshot)
    - [RELEASE_STRATEGY.md](RELEASE_STRATEGY.md) (Release-/Milestone-Regeln)
  - Dokumentations-/Auditnachweis gemäß Referenzen:
    - [docs/DOCUMENTATION_REVIEW_GUIDELINES.md](docs/DOCUMENTATION_REVIEW_GUIDELINES.md)
    - [docs/SYSTEMATISCHER_REVIEWPLAN.md](docs/SYSTEMATISCHER_REVIEWPLAN.md)
    - [docs/PR_DOCUMENTATION_CHECKLIST.md](docs/PR_DOCUMENTATION_CHECKLIST.md)
    - [docs/de/development/SOURCE_CODE_AUDIT.md](docs/de/development/SOURCE_CODE_AUDIT.md)
    - [docs/audit-framework/AUDIT_RUNBOOK.md](docs/audit-framework/AUDIT_RUNBOOK.md)
  - Audit-Checkliste abgeschlossen:
    - Fachreview gegen Doku-Checklisten durchgeführt
    - Dokumentationsaudit durchgeführt
    - Ergebnis in Root-Dokumentation und Changelog dokumentiert
    - Betroffene Bereiche festgehalten: `COPILOT_INSTRUCTIONS.md`, `VERSIONING.md`, `RELEASE_STRATEGY.md`, `CHANGELOG.md`, `ROADMAP.md`, `FUTURE_ENHANCEMENTS.md`
    - Nachweis-Orte: [ROADMAP.md](ROADMAP.md) (Abschnitt "Root Governance: Terminology and Traceability"), [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md) (Abschnitt "Root Governance Role"), [VERSIONING.md](VERSIONING.md), [RELEASE_STRATEGY.md](RELEASE_STRATEGY.md)

- **Module-Docs Sync 📚 — 2026-04-17**
  - 58 Module indexiert; 761 Primary-Markdown-Dateien in `src/` und `include/`
  - 0 Module ohne Sekundärdokumentation erkannt; Issues erzeugt
  - Sekundärdokumentation aktualisiert in `docs/de/` und `docs/en/`
  - Tool: `tools/module_docs_builder.py` v1.0.0
  <!-- changelog-updater: module-docs-sync-2026-04-17 -->

- **Module-Docs Sync 📚 — 2026-04-16**
  - 56 Module indexiert; 752 Primary-Markdown-Dateien in `src/` und `include/`
  - 6 Module ohne Sekundärdokumentation erkannt; Issues erzeugt
  - Sekundärdokumentation aktualisiert in `docs/de/` und `docs/en/`
  - Tool: `tools/module_docs_builder.py` v1.0.0
  <!-- changelog-updater: module-docs-sync-2026-04-16 -->

- **Module-Docs Sync 📚 — 2026-04-15**
  - 56 Module indexiert; 752 Primary-Markdown-Dateien in `src/` und `include/`
  - 6 Module ohne Sekundärdokumentation erkannt; Issues erzeugt
  - Sekundärdokumentation aktualisiert in `docs/de/` und `docs/en/`
  - Tool: `tools/module_docs_builder.py` v1.0.0
  <!-- changelog-updater: module-docs-sync-2026-04-15 -->



## [1.9.0] - 2026-04-11

> **Release Aggregation Document:** [`docs/de/releases/RELEASE_NOTES_v1.9.0.md`](docs/de/releases/RELEASE_NOTES_v1.9.0.md)
> **Aggregation Issue:** [#3071](https://github.com/makr-code/ThemisDB/issues/3071)

### Added
- **Chimera Multi-Model Adapter** — Streaming Result Sets, Prepared Statements, Connection-Pool-Adapter-Schnittstellen; Simulationsmodus für alle 4 Datenbankmodelle (PR #4478, Issue #3509)
- **Governance Compliance Evaluatoren** — ISO 27001 Annex A `Iso27001ControlSet` + HIPAA Security Rule `HipaaRuleSet` (PR #4484, Issue #3515)
- **Vollständige IPv6 Dual-Stack-Unterstützung** im Wire Protocol Server (PR #3769, Issue #3754)
- **QUIC/HTTP3 Transportschicht** (PR #3291, Issue #1994)
- **Nativer gRPC-Transport** für Binary Wire Protocol (PR #3299, Issue #2024)
- **Kernel Bypass DPDK/io_uring** — `DPDKServer`, `IoUringServer`, `CpuPinner`, `NumaAllocator`, `ZeroCopyDmaBuffer` (Issue #4057)
- **FAISS GPU Backend** — IVF_SQ8 + HNSW_FLAT; 50 Tests; `getCapabilities()` meldet INT8/L2/IP (Issue #4052)
- **BackendRegistry O(1) Selektion** — `typeIndex_` map eliminiert O(n²) + alle `dynamic_cast` im Hot-Path (Issue #4066)
- **Workload-Adaptive Optimizer** — OLTP/OLAP/MIXED/GRAPH/VECTOR/TIMESERIES Klassifikation, Predictive Scaling (Issue #4060)
- **Advanced Cache Optimization** — Multi-Partition, Bloom-Filter, adaptive Eviction (LRU/LIRS/ARC/2Q), LZ4-Kompression (Issue #4059)
- **NUMA-Aware Memory Manager** — `NUMAMemoryManager` mit Topology-Detection + Affinity-Allokation (Issue #4058, PR #4505)
- **mTLS Zertifikatsauthentifizierung** (PR #2777, Issue #1549)
- **GDPR Art. 17 PII Purge Propagation** im Cache (PR #2815, Issue #1591)
- **Authenticode/GPG Signaturverifizierung** für ModuleLoader (PR #2654, Issue #2473)
- **InputValidator Security API** (PR #4513)
- **Istio/Envoy Sidecar-Kompatibilität** (PR #3337, Issue #2208)
- **LZ4/Zstd Verbindungskompression** für Wire Protocol V2 (PR #2925, Issue #2206)
- **Per-Tenant Bandbreiten-Quotas** (PR #2924, Issue #2205)
- **KafkaCDCProducer + ICDCTransport** (Issue #3992; `cdc_kafka.yaml`)
- **Stable Importer Plugin ABI** `THEMIS_IMPORTER_PLUGIN_V1` — `PluginSandboxConfig`, `V1ImporterAdapter` (Importers Phase 10)
- **Lock-Free L1 Cache Read Path** — `l1_mutex_` → `std::shared_mutex`; Lazy-Expiry via CAS
- **gRPC Factory Wiring** für `ExecuteAQL`/`StreamAQL`; `GrpcApiServer` mutex + 30-Sekunden Shutdown-Deadline
- **MqttClientService + MqttCDCTransport** — MQTT CDC Bridge (Server ROADMAP)
- **DecisionRecordYamlProcessor** in `LoraRouter`, `AdapterLoadBalancer`, `LoraOrchestrator` (LLM ROADMAP)
- **Multi-field Boosting** `MultiFieldBoostedSearch`; Phase 5: `ConversationalSearch`, `FederatedSearch` (Search)
- **MultiHopReasoner + AdaptiveRetrieval** für Knowledge Graph RAG (PR #4509)
- **Knowledge Graph-augmentiertes Retrieval** mit Entity Linking (PR #2748, Issue #2242)
- **Forecasting Batch/Streaming** — `predictBatch()`, O(1)-`update()`, parallele Auto-Tune, FNV-1a Cache; 17 Tests (Issue #4054)
- **NCCL/RCCL Distributed `mergeTopK`** Integration (Issue #3867, PR #4568)
- **QueryFederation Shard-Key-Routing** (Point-Lookup + Range); `QueryEngine::createDefault()` (Query ROADMAP)
- **Adaptive Deadlock Prevention** (Issue #4091)
- **Automatische Indexierungs-Empfehlungen** (Issue #4084)
- **AdaptiveFlushController** in TSStore integriert (PR #4500)
- **PMU non-Linux Stub-Abdeckung** — macOS kpc, Windows QueryThreadCycleTime, RDTSC/CNTVCT_EL0 (Issue #4086)
- **Multi-Environment Config Overlay** dev/staging/prod (Issue #3997)
- **AQL Query-Migrations-Assistent** ArangoDB → ThemisDB AQL (PR #2694, Issue #1360)
- **Speaker-Verifizierung** für Voice-Biometrics (PR #2605, Issue #2494)
- **Per-Tenant Metric-Namespacing** + strukturiertes Log-Search (PR #4503)
- **Runtime Capability Escalation Blocking** (PR #4504)
- **WiscKey GC/Log Compaction** (Issue #940)
- **PERF-D1..D7 Benchmark-Suite** — Adaptive Flush, Parallel Batch Insert, SIMD Distance, Lock-Free 2PC, Streaming Blob Write, Query Lazy-Eval (PRs #4493–#4498)

### ⚠️ Breaking Changes
- **`QueryEngine::createDefault()`** wirft `std::runtime_error` wenn keine Storage/Index-Adapter injiziert sind — Konstruktor-Injektion verwenden
- **Cache L1** — `L1Entry`-Felder atomicisiert; eigene Subklassen müssen auf `std::shared_mutex`-Muster umgestellt werden
- **Importer Plugin ABI** — ältere DSO-Plugins ohne `THEMIS_IMPORTER_PLUGIN_V1`-Export werden nicht mehr geladen

---

## [1.8.1-rc1] - 2026-04-04

> **Release Notes:** [`docs/de/releases/RELEASE_NOTES_v1.8.1-rc1.md`](docs/de/releases/RELEASE_NOTES_v1.8.1-rc1.md)

### Added
- **README: Comprehensive Technology & Feature Badges** 🏷️
  - Added 11 badge categories to the README header showcasing ThemisDB capabilities:
  - *Technology Stack*: C++17/20, CUDA, Vulkan, RocksDB, llama.cpp
  - *Multi-Model Capabilities*: Relational (AQL), Vector (HNSW+FAISS), Graph (Property Graphs),
    Document, Geospatial (GeoJSON/R-tree), TimeSeries
  - *Enterprise & Security*: ACID (MVCC), TLS 1.3, PKI (X.509/GPG), RBAC, AES-256-GCM Encryption
  - *AI/ML Integration*: LLM-Ready, RAG, Vector Search, Embeddings, LoRA Fine-Tuning
  - *Performance*: GPU-Accelerated, SIMD, 45K WPS, 120K RPS
  - *Distributed Systems*: Sharding, Raft Replication, CDC
  - *Query & Analytics*: AQL, GraphQL, OLAP, Full-Text Search (BM25)
  - *Data Integration*: PostgreSQL Wire Import, Multi-Format Export, Content Pipeline
  - *Observability*: Prometheus, OpenTelemetry, Audit Logging
  - *Quality Metrics*: 41 Modules, 500K+ LOC, 3 Production-Ready Core Modules
  - *Community*: Chat (Slack), Forum (GitHub Discussions), Contributing Guide
  - All badges link to relevant `src/` module directories using shields.io
- **Geo Module: Full GeoJSON RFC 7946 parsing** 🌍
  - `EWKBParser::parseGeoJSON()` now handles all seven RFC 7946 geometry types:
    `Point`, `MultiPoint`, `LineString`, `MultiLineString`, `Polygon`, `MultiPolygon`,
    and `GeometryCollection` (including 3D variants with Z coordinates).
  - `EWKBParser::toGeoJSON()` serializes all seven geometry types.
  - EWKB `parse()` and `serialize()` now support all geometry types (types 4–7).
  - `GeometryCollection` is parsed recursively up to a depth of 8 to prevent stack
    overflow on adversarial input.
  - `computeMBR()` and `computeCentroid()` now recurse into nested sub-geometries.
  - WGS84 coordinate range validation: longitude must be in [-180, 180] and latitude
    in [-90, 90]; invalid coordinates throw `std::runtime_error`. Compile with
    `-DTHEMIS_GEO_COMPAT_LAX` to skip coordinate range validation during a migration
    window.
- **Geo Module: In-memory R-tree spatial index** 🌳
  - New `GeoRTree` class (`include/geo/geo_rtree.h`, `src/geo/geo_rtree.cpp`):
    an in-memory R-tree index for `GeometryInfo` objects enabling sub-linear
    `intersects` and `contains` queries.
  - When compiled with `THEMIS_GEO_BOOST_BACKEND` and Boost.Geometry headers present,
    uses `boost::geometry::index::rtree` with `rstar<16>` splitting strategy.
  - Without Boost, automatically falls back to an O(n) linear MBR scan —
    semantically identical, no dependency required.
  - `bulkLoad(entries)` uses STR (Sort-Tile-Recursive) packing via the Boost bulk-insert
    constructor for 3–5× faster cold-start load compared to incremental `insert()`.
  - `memoryBytes()` returns a conservative estimate of heap usage and logs the value
    via the existing structured audit log field `geo_index_bytes_allocated`.
  - 20 unit tests covering: empty index, insert, bulkLoad (including replace-on-reload),
    remove, clear, intersects (single/multiple/overlapping/world), contains
    (single/multiple/boundary), memory reporting, and move semantics.
- **Geo Module: ST_UNION and ST_DIFFERENCE geometry operations** 🔷
  - New `ISpatialComputeBackend::stUnion(geom1, geom2)` and `stDifference(geom1, geom2)`
    virtual methods added to `include/geo/spatial_backend.h`.
  - `CpuExactBackend` (cpu-only, no Boost dependency): full implementation using
    the Greiner-Hormann polygon clipping algorithm (ACM TOG 1998) with fast-paths
    for containment, disjoint, and B-inside-A (returns polygon with hole ring).
    Point and Point-Polygon cases handled with simple coordinate logic.
  - `BoostCpuExactBackend`: implementation via `boost::geometry::union_` and
    `boost::geometry::difference`; falls back to CpuExactBackend for non-polygon types.
  - `GpuBatchBackend`: delegates to `getCpuExactBackend()` with audit log and
    metrics records — same pattern as the existing `stBuffer` GPU fallback.
  - AQL functions `ST_UNION(geom1, geom2)` and `ST_DIFFERENCE(geom1, geom2)` registered
    in `include/query/functions/geo_functions.h`; return GeoJSON geometry.
  - 15 new unit tests in `tests/geo/test_geo_st_union_difference.cpp` (parameterised
    over `cpu_exact` and `gpu_spatial` backends) and 7 AQL-level tests added to
    `tests/geo/test_aql_st_functions.cpp`.

### ⚠️ Breaking Changes
- **GeoJSON strict parsing** (`EWKBParser::parseGeoJSON`): coordinate values outside
  the WGS84 range (longitude [-180, 180], latitude [-90, 90]) now throw
  `std::runtime_error`. Previously, out-of-range coordinates were silently accepted.
  To restore the old lenient behavior for one release cycle, compile with
  `-DTHEMIS_GEO_COMPAT_LAX=1`.
- **Unknown geometry types** now throw `std::runtime_error` with the message
  `"GeoJSON: unsupported geometry type: <type>"` instead of silently returning an
  empty geometry.

### Changed
- **Config Architecture Reorganization** 🗂️
  - **Hierarchical Directory Structure**: Reorganized all config files into logical categories
    - `config/core/` - Core system configurations (config.yaml, security.yaml, updates.yaml)
    - `config/platform/` - Platform-specific configs (rpi3, rpi4, rpi5, qnap)
    - `config/ai_ml/` - AI/ML configurations (LLM, vision, LoRA, RAG)
    - `config/security/` - Security & authentication configs (RBAC, PII, Kerberos)
    - `config/compliance/` - Compliance & ethics (ethical guidelines, audit, governance)
    - `config/performance/` - Performance optimizations (scaling, query cache, acceleration)
    - `config/data_management/` - Data lifecycle (retention, redundancy, MIME types)
    - `config/distributed/` - Distributed system configs (replication, sharding)
    - `config/licensing/` - License configurations (community, enterprise)
    - `config/networking/` - Network configurations (connection pooling)
    - `config/content/` - Content processing (processors, edge types)
    - `config/monitoring/` - Monitoring & observability (Prometheus metrics)
    - `config/features/` - Feature flags and capability generation
    - `config/assistants/` - Assistant configurations (docs, feedback)
    - `config/processing/` - Stream/event processing (CEP rules)
    - `config/deprecated/` - Deprecated/backup files
  - **ConfigPathResolver Utility**: Automatic backward compatibility layer
    - Resolves legacy paths to new hierarchical locations
    - Provides fallback mechanism with deprecation warnings
    - Zero breaking changes to existing code
    - Includes `resolve()`, `tryResolve()`, and `mapLegacyToNew()` methods
  - **Updated C++ Code Paths**: Updated all config loading code to use new structure
    - `src/server/http_server.cpp` - LoRA training config
    - `src/server/mcp_server.cpp` - LLM system prompts
    - `src/utils/pii_detector.cpp` - PII patterns
    - `src/main_server.cpp` - Core config, security, retention policies
    - `src/index/vector_index.cpp` - Scaling optimizations
    - `src/content/mime_detector.cpp` - MIME types
  - **Comprehensive Documentation**:
    - `config/README.md` - Complete directory structure overview
    - `config/MIGRATION_GUIDE.md` - Detailed migration instructions
    - Full path mapping table (60+ config files)
  - **Benefits**: Improved organization, better discoverability, scalability, backward compatibility

### Added
- **Search Module v1.5.0 — 7 new search components** 🔍
  - **`QueryExpander`** (`include/search/query_expander.h`): Synonym expansion with
    configurable max_expansions; Levenshtein-based spelling correction against a
    user-supplied vocabulary; alternative query generation; zero-result relaxation
    (drops last token).  Tests: `tests/test_query_expander.cpp` (28 tests).
  - **`FuzzyMatcher`** (`include/search/fuzzy_matcher.h`): Levenshtein, Soundex,
    Metaphone, and N-gram (Dice-coefficient) similarity; public static utilities for
    direct use; wraps `SecondaryIndexManager::scanFulltextFuzzy`.  Tests:
    `tests/test_fuzzy_matcher.cpp` (24 tests).
  - **`FacetedSearch`** (`include/search/faceted_search.h`): Per-field value-count
    facets (`computeFacet`), multi-column batch facets (`computeFacets`), numeric
    range-bucket facets (`computeRangeFacet`), and drill-down filter intersection
    (`applyFacetFilters`).  Tests: `tests/test_faceted_search.cpp` (20 tests).
  - **`SearchAnalytics`** (`include/search/search_analytics.h`): Thread-safe query
    event log (circular eviction at `Config::max_events`); `computeMetrics()` returns
    average/p95/p99 latency, zero-result rate, and top-20 queries.  Tests:
    `tests/test_search_analytics.cpp` (26 tests).
  - **`AutocompleteEngine`** (`include/search/autocomplete.h`): Prefix-index
    suggestions via `SecondaryIndexManager::scanKeysRange`; popular-query
    suggestions via `SearchAnalytics`; combined, deduplicated, score-ranked output.
    Tests: `tests/test_autocomplete.cpp` (18 tests).
  - **`LearningToRank`** (`include/search/learning_to_rank.h`): Dot-product linear
    re-ranker over a 6-dimensional `RankingFeatures` vector; online pairwise
    gradient-descent training from `ClickEvent` data; deterministic A/B variant
    routing via `selectVariant()` / `rerankWithVariant()`.  Tests:
    `tests/test_learning_to_rank.cpp` (28 tests).
  - **`MultiModalSearch`** (`include/search/multi_modal_search.h`): Accepts
    `ModalQuery` components (TEXT / IMAGE / AUDIO / CUSTOM), dispatches to
    `SecondaryIndexManager` or `VectorIndexManager`, fuses via weighted RRF.
    `searchTextAndImage()` convenience method.  Tests:
    `tests/test_multi_modal_search.cpp` (18 tests).

- **Search Module v1.4.0 — HybridSearch production hardening** 🔍
  - **Configurable vector metric**: `Config::vector_metric` (COSINE / DOT / L2) —
    was hardcoded to COSINE; DOT and L2 now correctly convert distance to similarity.
  - **Strict config validation**: constructor throws `std::invalid_argument` on
    k == 0, rrf_k ≤ 0, negative weights, k > max_k, k_bm25/k_vector > max_candidates,
    empty default_table / default_column.
  - **Resource limits**: `Config::max_k` and `Config::max_candidates` bound
    unbounded index scans (default 10,000 each).
  - **Score normalization edge cases**: range == 0 now yields 1.0 for positive
    scores, 0.0 for zero scores.
  - **Linear-combination pre-normalization**: BM25 and vector scores are always
    normalized to [0,1] before weighting, eliminating scale incompatibility.
  - **`SearchStats`**: appended to every `search()` return; exposes `bm25_ok`,
    `vector_ok`, `partial_result`, `bm25_count`, `vector_count`.
  - **Exception safety**: `search()` catches all backend and fusion exceptions,
    logs via `THEMIS_ERROR`, and returns empty/partial results rather than throwing.
  - **Thread-safety and exception-safety documentation** added to header.
  - **`normalizeScores` promoted to `public static`** for direct testability.
  - **Tests**: `test_hybrid_search.cpp` (35+ tests), `test_rrf_fusion.cpp` (20 tests),
    `test_score_normalization.cpp` (15 tests), `test_hybrid_search_integration.cpp`
    (18 integration tests).
  - **Benchmark**: `benchmarks/benchmark_hybrid_search.cpp`.

- **Shard Repair / Anti-Entropy Engine** 🔧 (`include/sharding/shard_repair_engine.h`)
  - **Background anti-entropy scan**: periodic `checkDocumentHealth()` across all shards; degraded documents are automatically queued for recovery
  - **Repair worker thread**: drains job queue via `RedundancyStrategy::recoverDocument()` (RAID-5/6 + Mirror modes)
  - **On-demand triggers** returning trackable job IDs: `triggerRepair(shard_id)`, `triggerFullScan()`, `triggerDocumentRepair(doc_id)`
  - **Per-shard `ShardHealthReport`**: status `HEALTHY` / `DEGRADED` / `FAILED` / `REBUILDING`, scan + repair counters
  - **Prometheus metrics forwarding**: repair events forwarded to `PrometheusMetrics` and exposed via `exportPrometheusMetrics()` and `ShardingMetricsHandler::getMetrics()`
  - **Admin API repair endpoints**: `POST /admin/repair`, `POST /admin/repair/scan`, `GET /admin/repair/{job_id}`
  - **`AutoRecoveryManager::setRepairEngine()`**: wires legacy `AutoRecoveryManager` to delegate `repairDocument()` to the new engine

- **Improved Reed-Solomon erasure decoder** ⚡
  - Replaced XOR-only parity (single-chunk recovery) with **Vandermonde matrix** systematic codec over GF(2⁸)
  - Recovers up to `parity_shards` simultaneously lost chunks — enables true RAID-6 dual-parity recovery
  - Both `ReedSolomonCoder` and `CauchyReedSolomonCoder` now validate `missing_indices.size() <= parity_shards`

- **v1.5.x Query Optimizer Production Integration** 🎯
  - **Shard Metadata Integration (preparatory)**: Integration point for metadata-backed row estimates
    - `DistributedQueryCostModel::getShardRowCount()` replaces hardcoded 10K constant with dynamic estimates
    - Currently uses hash-based heuristic; full MetadataShard integration planned for v1.5.1
    - Provides foundation for accurate cardinality estimation in distributed queries
    - Integrates with existing sharding infrastructure
  - **Predicate-based Selectivity Estimation**: Calculate query selectivity from predicates
    - `DistributedQueryCostModel::calculatePredicateSelectivity()` analyzes query patterns
    - Histogram-based estimation framework (extensible)
    - Column-specific heuristics: ID columns (0.1%), status (20%), names (5%)
    - Combined predicates use product of individual selectivities
    - Bounded selectivity: [0.01%, 100%]
  - **Network Latency Monitoring (preparatory)**: Integration point for latency-aware query planning
    - `DistributedQueryCostModel::measureShardLatency()` provides latency integration hook
    - Currently uses naming-convention heuristics; Prometheus integration planned for v1.5.1
    - Enables locality detection (< 1ms latency threshold)
    - Network-aware parallelism optimization
    - Foundation for latency-aware join strategies
  - **Comprehensive Integration Tests**: `tests/test_optimizer_v1_5_x_integration.cpp`
    - Tests for shard metadata integration
    - Tests for selectivity calculation
    - Tests for network latency awareness
    - Tests for partition pruning
    - Full pipeline integration tests

- **v1.5.x FAISS Vector Search Improvements** 🚀
  - **ADC (Asymmetric Distance Computation) Tables**: ~40% faster vector search
    - Enabled by default in `AdvancedVectorIndex::Config`
    - Precomputed distance tables for IndexIVFPQ
    - Optional polysemous hash tables for early termination
    - No accuracy trade-off (bit-exact results)
    - Minimal memory overhead (~1-2% of index size)
  - **Configuration Options**:
    - `use_adc_tables`: Enable ADC distance tables (default: true)
    - `polysemous_ht`: Polysemous codes for early termination (default: 0)
  - **Performance Impact**:
    - Search speed: ~40% faster (varies by dataset)
    - Particularly effective for high-dimensional vectors (>128d)
    - Higher throughput with lower query latency

### Changed
- **Write-Amplification Optimization (v1.5.0)** ⚡
  - **Larger Memtables**: Increased default `memtable_size_mb` from 256MB to 512MB
    - ~50% fewer L0 file flushes → ~30-40% reduction in write-amplification
    - Improves write throughput for data ingestion and high-write workloads
  - **More Write Buffers**: Increased default `max_write_buffer_number` from 3 to 6
    - Allows writes to continue during memtable flush operations
    - Reduces write stalls and improves sustained write throughput
  - **Total Write Buffer Limit**: Set `db_write_buffer_size_mb` default to 2048MB (2GB)
    - Previously unlimited (0), now has sensible default to prevent OOM with many column families
    - Auto-manages write buffer allocation across all column families
  - **Async I/O Enabled by Default**: Enhanced asynchronous I/O for better scan performance
    - `enable_async_io` now defaults to `true` (was `false`)
    - `async_io_readahead_size_mb` increased from 64MB to 128MB
    - Expected improvement: 2-5x faster sequential scans and range queries
  - **Documentation**: Added comprehensive "Write-Amplification Optimization" section to PERFORMANCE_TIPS.md
    - Explains write-amp problem and solutions
    - Tuning guidelines for different workloads (high-throughput, balanced, low-latency, memory-constrained)
    - Monitoring metrics and Prometheus queries
    - Best practices and configuration examples
  - **Server Logging**: Updated main_server.cpp to display new optimization settings
    - Shows memtable size, write buffer count, and async I/O status at startup
    - Displays optimization profile (write-optimized, high-throughput, balanced, or low-latency)
  - **Trade-offs**: Higher memtable memory (up to ~2GB capped by `db_write_buffer_size_mb`; theoretical 3-4GB if cap is raised), longer recovery time
  - **Backward Compatibility**: All settings can be overridden via configuration
  - **Testing**: Added comprehensive configuration test suite (`test_write_amplification_config.cpp`)

- **Documentation Consolidation for Beta/RC** 📚
  - Archived 70+ historical documents (GAP analyses, old roadmaps, TODO lists, implementation summaries)
  - Organized archives into structured directories: gaps/, roadmaps/, todos/, implementation-summaries/
  - Updated documentation index to reflect current Beta/RC-ready status (v1.5.0-dev)
  - Streamlined navigation and removed outdated references
  - See [docs/ARCHIVED/README.md](docs/ARCHIVED/README.md) for archive index

### Added
- **HSM Security Warning System (FIND-002)** 🔒
  - **Startup Warning Banner**: Prominent warning displayed when stub HSM provider is active
    - 80-character ASCII box with clear security messaging
    - Directs users to HSM production setup documentation
    - Can be suppressed in development with `--allow-stub-hsm` flag
  - **Periodic Security Logging**: ERROR-level warnings logged every 5 minutes when stub HSM is active
    - Persistent reminder of insecure configuration
    - Helps prevent accidental production deployment with stub provider
  - **Prometheus Metrics**: HSM security status exposed via `/metrics` endpoint
    - `themis_hsm_insecure_config`: Gauge indicating insecure configuration (0=secure, 1=insecure)
    - `themis_hsm_provider_type{provider="stub|real"}`: Provider type information
    - `hsm_security_stub_active`: Legacy metric name for backward compatibility
    - `hsm_compliance_status{standard="..."}`: Compliance status for NIST, ISO, PCI DSS, GDPR
  - **Command-Line Flag**: `--allow-stub-hsm` flag for development environments
    - Suppresses warning banner and periodic logging
    - Documented in help output (`--help`)
  - **Documentation Updates**:
    - QUICKSTART.md now includes prominent HSM security warning at top
    - Configuration examples show HSM settings with warnings
    - References to `docs/security/HSM_PRODUCTION_SETUP.md` throughout
  - **Compliance**: Addresses critical security finding FIND-002 from v1.4.1 audit
    - Prevents master encryption keys from being unprotected in production
    - Supports NIST SP 800-53 SC-12, ISO 27001 A.8.24, PCI DSS 3.6, GDPR Art. 32

### Changed
- main_server.cpp now initializes HSM provider at startup and validates security configuration
- Prometheus metrics endpoint (`/metrics`) now includes HSM security metrics
- Help output (`--help`) now lists `--allow-stub-hsm` flag

- **Multi-GPU Vector Indexing API (v2.4)** 🎉
  - **MultiGPUVectorIndex**: Multi-device API and partition/merge scaffolding for distributed vector search
    - Logical support for 2-8 devices via index partitioning (round-robin, hash-based, range-based, balanced)
    - Query fan-out and centralized top-k merge logic for aggregating per-partition results
    - Designed for future distributed search across multiple GPUs once GPU backends are available
    - **Current execution**: Uses CPU-based GPUVectorIndex backend (no actual multi-GPU execution yet)
    - Fault-tolerant design with graceful degradation when partitions are unavailable
    - **GPU execution and collectives**: Planned for v2.5+ (NCCL/RCCL, P2P transfers, actual GPU offload)
  - **API Features (scaffolding)**:
    - `enableMultiGPU` configuration flag for multi-device indexing
    - `deviceIds` parameter for future GPU selection (configuration only, no GPU enumeration in v2.4)
    - `partitionStrategy` option for data distribution across logical partitions
    - Per-partition statistics with hooks for future per-GPU metrics (VRAM, utilization)
    - Load imbalance and scaling efficiency metrics computed over logical partitions
  - **Testing**:
    - Unit tests covering partitioning/merge logic and API behavior (394 lines)
    - Tests validate API correctness on CPU, ready for GPU backend integration
    - Example application demonstrating configuration and partition behavior (237 lines)
  - **Documentation**:
    - Complete API guide (`docs/MULTI_GPU_VECTOR_INDEXING.md`) with current CPU-only status clearly noted
    - API reference with code examples and notes on planned GPU backends (v2.5+)
    - Discussion of anticipated performance characteristics once GPU support lands
    - Troubleshooting guide noting current limitations (no GPU execution, no NCCL/RCCL yet)

- **Git-Like Features Integration** 🎉
  - **SnapshotManager Re-enabled**: Named snapshots for MVCC are now fully operational
    - 5 REST endpoints for snapshot/tag management
    - Integration with DiffEngine for tag-based diffs
    - Persistent snapshot storage in RocksDB
  - **PITR API Handler**: Point-in-Time Recovery REST API integration
    - POST `/api/v1/pitr/restore/sequence` - Restore to specific sequence number
    - POST `/api/v1/pitr/restore/tag` - Restore to named snapshot tag
    - POST `/api/v1/pitr/restore/timestamp` - Restore to timestamp
    - POST `/api/v1/pitr/preview` - Preview restore operation (dry-run)
    - GET `/api/v1/pitr/progress` - Get current restore progress
  - **DiffEngine Enhanced**: Now accepts optional SnapshotManager for tag-based diffs
  - **MergeEngine API Integration** 🆕
    - **3-Way Merge Support**: Full Git-like merge functionality now integrated
    - REST API endpoints for merge operations:
      - POST `/api/v1/merge` - Perform three-way merge between sequences
      - POST `/api/v1/merge/preview` - Preview merge without applying (dry-run)
      - POST `/api/v1/merge/by-tag` - Merge using snapshot tags instead of sequences
      - GET `/api/v1/merge/can-fast-forward` - Check if fast-forward merge is possible
    - **BranchManager Enhanced**: Non-fast-forward branch merges now supported
      - Automatic integration with MergeEngine for complex merges
      - Conflict detection and resolution strategies
      - Fast-forward detection and optimization
    - **Conflict Resolution**: Multiple strategies available (OURS, THEIRS, MANUAL, FAST_FORWARD)
    - **Full Integration**: MergeEngine properly initialized in HTTP server and connected to BranchManager

### Changed
- Updated DiffEngine initialization to support SnapshotManager reference
- HTTP server now properly converts between Beast and httplib types for git-feature endpoints
- CMake configuration updated to include multi-GPU vector indexing sources and tests

### Fixed
- Re-enabled previously disabled SnapshotManager due to incomplete type issues
- Added proper error handling with default case in PITR progress phase conversion

### Documentation

- **Module-Docs Sync 📚 — 2026-04-04**
  - 52 Module indexiert; 691 Primary-Markdown-Dateien in `src/` und `include/`
  - 17 Module ohne Sekundärdokumentation erkannt; Issues erzeugt
  - Sekundärdokumentation aktualisiert in `docs/de/` und `docs/en/`
  - Tool: `tools/module_docs_builder.py` v1.0.0
  <!-- changelog-updater: module-docs-sync-2026-04-04 -->

- **Module-Docs Sync 📚 — 2026-04-03**
  - 52 Module indexiert; 691 Primary-Markdown-Dateien in `src/` und `include/`
  - 17 Module ohne Sekundärdokumentation erkannt; Issues erzeugt
  - Sekundärdokumentation aktualisiert in `docs/de/` und `docs/en/`
  - Tool: `tools/module_docs_builder.py` v1.0.0
  <!-- changelog-updater: module-docs-sync-2026-04-03 -->

- **Module-Docs Sync 📚 — 2026-04-02**
  - 52 Module indexiert; 691 Primary-Markdown-Dateien in `src/` und `include/`
  - 17 Module ohne Sekundärdokumentation erkannt; Issues erzeugt
  - Sekundärdokumentation aktualisiert in `docs/de/` und `docs/en/`
  - Tool: `tools/module_docs_builder.py` v1.0.0
  <!-- changelog-updater: module-docs-sync-2026-04-02 -->

- **Module-Docs Sync 📚 — 2026-04-01**
  - 52 Module indexiert; 691 Primary-Markdown-Dateien in `src/` und `include/`
  - 17 Module ohne Sekundärdokumentation erkannt; Issues erzeugt
  - Sekundärdokumentation aktualisiert in `docs/de/` und `docs/en/`
  - Tool: `tools/module_docs_builder.py` v1.0.0
  <!-- changelog-updater: module-docs-sync-2026-04-01 -->

- **Module-Docs Sync 📚 — 2026-03-31**
  - 52 Module indexiert; 691 Primary-Markdown-Dateien in `src/` und `include/`
  - 17 Module ohne Sekundärdokumentation erkannt; Issues erzeugt
  - Sekundärdokumentation aktualisiert in `docs/de/` und `docs/en/`
  - Tool: `tools/module_docs_builder.py` v1.0.0
  <!-- changelog-updater: module-docs-sync-2026-03-31 -->

- **Module-Docs Sync 📚 — 2026-03-30**
  - 52 Module indexiert; 691 Primary-Markdown-Dateien in `src/` und `include/`
  - 17 Module ohne Sekundärdokumentation erkannt; Issues erzeugt
  - Sekundärdokumentation aktualisiert in `docs/de/` und `docs/en/`
  - Tool: `tools/module_docs_builder.py` v1.0.0
  <!-- changelog-updater: module-docs-sync-2026-03-30 -->

- **Module-Docs Sync 📚 — 2026-03-29**
  - 52 Module indexiert; 691 Primary-Markdown-Dateien in `src/` und `include/`
  - 17 Module ohne Sekundärdokumentation erkannt; Issues erzeugt
  - Sekundärdokumentation aktualisiert in `docs/de/` und `docs/en/`
  - Tool: `tools/module_docs_builder.py` v1.0.0
  <!-- changelog-updater: module-docs-sync-2026-03-29 -->

- **Module-Docs Sync 📚 — 2026-03-28**
  - 52 Module indexiert; 691 Primary-Markdown-Dateien in `src/` und `include/`
  - 17 Module ohne Sekundärdokumentation erkannt; Issues erzeugt
  - Sekundärdokumentation aktualisiert in `docs/de/` und `docs/en/`
  - Tool: `tools/module_docs_builder.py` v1.0.0
  <!-- changelog-updater: module-docs-sync-2026-03-28 -->

- **Module-Docs Sync 📚 — 2026-03-27**
  - 52 Module indexiert; 691 Primary-Markdown-Dateien in `src/` und `include/`
  - 17 Module ohne Sekundärdokumentation erkannt; Issues erzeugt
  - Sekundärdokumentation aktualisiert in `docs/de/` und `docs/en/`
  - Tool: `tools/module_docs_builder.py` v1.0.0
  <!-- changelog-updater: module-docs-sync-2026-03-27 -->

- **Module-Docs Sync 📚 — 2026-03-16**
  - 48 Module indexiert; 421 Primary-Markdown-Dateien in `src/` und `include/`
  - 15 Module ohne Sekundärdokumentation erkannt; Issues erzeugt
  - Sekundärdokumentation aktualisiert in `docs/de/` und `docs/en/`
  - Tool: `tools/module_docs_builder.py` v1.0.0
  <!-- changelog-updater: module-docs-sync-2026-03-16 -->

- **Module-Docs Sync 📚 — 2026-03-15**
  - 48 Module indexiert; 421 Primary-Markdown-Dateien in `src/` und `include/`
  - 15 Module ohne Sekundärdokumentation erkannt; Issues erzeugt
  - Sekundärdokumentation aktualisiert in `docs/de/` und `docs/en/`
  - Tool: `tools/module_docs_builder.py` v1.0.0
  <!-- changelog-updater: module-docs-sync-2026-03-15 -->

- **Module-Docs Sync 📚 — 2026-03-14**
  - 48 Module indexiert; 421 Primary-Markdown-Dateien in `src/` und `include/`
  - 15 Module ohne Sekundärdokumentation erkannt; Issues erzeugt
  - Sekundärdokumentation aktualisiert in `docs/de/` und `docs/en/`
  - Tool: `tools/module_docs_builder.py` v1.0.0
  <!-- changelog-updater: module-docs-sync-2026-03-14 -->

- **Module-Docs Sync 📚 — 2026-03-13**
  - 48 Module indexiert; 421 Primary-Markdown-Dateien in `src/` und `include/`
  - 15 Module ohne Sekundärdokumentation erkannt; Issues erzeugt
  - Sekundärdokumentation aktualisiert in `docs/de/` und `docs/en/`
  - Tool: `tools/module_docs_builder.py` v1.0.0
  <!-- changelog-updater: module-docs-sync-2026-03-13 -->

- **Module-Docs Sync 📚 — 2026-03-12**
  - 47 Module indexiert; 277 Primary-Markdown-Dateien in `src/` und `include/`
  - 15 Module ohne Sekundärdokumentation erkannt; Issues erzeugt
  - Sekundärdokumentation aktualisiert in `docs/de/` und `docs/en/`
  - Tool: `tools/module_docs_builder.py` v1.0.0
  <!-- changelog-updater: module-docs-sync-2026-03-12 -->
- **GPU Master Tracking Document** 📋
  - Added `docs/GPU_MASTER_TRACKING.md` - Comprehensive master tracking document for GPU implementation roadmap (v2.x series)
  - Complete timeline and deliverables for all GPU backends (CUDA, Vulkan, HIP, Multi-GPU)
  - Performance targets, quality metrics, and success criteria
  - Risk mitigation strategies and resource planning
  - Cross-references to all GPU documentation: `FUTURE_GPU_SUPPORT.md`, `GPU_SUPPORT_ROADMAP.md`, `GPU_VECTOR_INDEXING_ARCHITECTURE.md`
  - Updated `docs/00_DOCUMENTATION_INDEX.md` with new GPU Vector Indexing section
- Added `MULTI_GPU_VECTOR_INDEXING.md` documenting multi-GPU implementation
- Added `GIT_FEATURES_INTEGRATION_STATUS.md` documenting integration status
- Documented that BranchManager and MergeEngine are pending (separate draft PRs)

---

## [1.5.0] - 2026-02-03

### Added
- **RFC 3161 Timestamp Authority (TSA) - PRODUCTION READY** 🎉
  - Full RFC 3161 client implementation with OpenSSL cryptographic operations
  - Integration with external TSA providers (FreeTSA, DigiCert, Sectigo)
  - eIDAS compliance support for qualified electronic timestamps
  - Long-term validation (LTV) for 30-year timestamp retention
  - Comprehensive TSA setup guide (`docs/en/security/TSA_SETUP.md`)
  - Configuration management via `config/timestamp_authority.yaml`
  - CMake option `THEMIS_USE_OPENSSL_TSA` to control TSA mode (default: ON)
  - Build-time and runtime warnings when stub mode is active
  - Support for SHA-256, SHA-384, SHA-512 hash algorithms
  - Certificate chain validation and verification
  - 10+ comprehensive tests for RFC 3161 compliance

- **FAISS Quantizer Integration - Production Ready** (#1079) 🚀
  - **FAISS K-means Integration**: ProductQuantizer now uses FAISS K-means clustering
    - `ProductQuantizer`: FAISS K-means for 20-30% faster training with SIMD optimizations
    - Automatic fallback to custom K-means if FAISS unavailable or errors occur
    - Uses faiss::Clustering and faiss::IndexFlatL2 for optimal performance
  - **FAISS-optimized Binary Operations**: BinaryQuantizer uses compiler intrinsics
    - `BinaryQuantizer`: SIMD-optimized popcount for faster Hamming distance
    - Uses __builtin_popcount (GCC) or __popcnt (MSVC) same as FAISS
    - `ResidualQuantizer`: Inherits FAISS acceleration from ProductQuantizer stages (30% faster training)
  - **Backend Selection**: New `prefer_faiss` configuration option
    - Defaults to `true` when FAISS is available
    - Graceful fallback to custom implementation on errors
  - **Runtime Inspection**: `getBackend()` method reports actual backend in use
  - **Build System**: Uses existing `THEMIS_HAS_FAISS` conditional compilation
  - **Production Ready**: Fully tested with actual FAISS API integration

### Changed
- TSA implementation now uses OpenSSL by default (was stub in v1.4.1)
- Improved CMake configuration for security features
- Enhanced security feature reporting in build system
- **ProductQuantizer**: Updated from v1.3.0 to v1.5.0 with actual FAISS K-means integration
- **BinaryQuantizer**: Updated from v1.4.1 to v1.5.0 with FAISS-optimized Hamming distance
- **ResidualQuantizer**: Updated from v1.4.1 to v1.5.0 with FAISS-accelerated composition
- **FAISS Integration Complete** ✅
  - Documented that AdvancedVectorIndex uses FAISS natively (IVF+PQ, HNSW, GPU)
  - Clarified that FAISS is the PRIMARY vector indexing solution for production
  - Custom quantizers now have actual FAISS integration with graceful fallback
  - Marked LearnedQuantizer as deprecated (research-only)
  - Updated `LIBRARY_USAGE_ANALYSIS.md` and `LIBRARY_OPTIMIZATION_QUICKREF.md`

### Performance Improvements
- **20-30% faster ProductQuantizer training** with FAISS K-means (verified with actual integration)
- **10-15% faster BinaryQuantizer Hamming distance** with SIMD intrinsics
- **30% faster ResidualQuantizer training** (via FAISS ProductQuantizer composition)
- Zero overhead when FAISS not available (graceful fallback maintained)

### Backward Compatibility
- ✅ All existing quantization code continues to work without changes
- ✅ API remains unchanged (new options are optional with sensible defaults)
- ✅ Default behavior gains performance boost with FAISS when available
- ✅ Graceful degradation when FAISS unavailable
### Removed
- **GPU Vector Index Stubs (CLEANUP)** 🧹
  - Removed incomplete GPU backend implementations (~1500 LOC)
    - `src/index/gpu_vector_index_cuda.cpp` (384 lines, 3 TODOs)
    - `src/index/gpu_vector_index_vulkan.cpp` (385 lines, 6 TODOs)
    - `src/index/gpu_vector_index_hip.cpp` (419 lines, 4 TODOs)
    - `src/index/gpu_vector_index_kernels.cu` (CUDA kernels)
    - `src/index/gpu_vector_index_hip_kernels.cpp` (HIP kernels)
  - Removed GPU backend classes from public API
  - Removed GPU-specific CMake configuration
  - **Rationale**: These were research stubs with 65+ TODO comments and no functional GPU acceleration
  - **Current Status**: `GPUVectorIndex` now uses CPU-only implementation (SIMD-optimized)
  - **Future Plans**: Proper GPU support planned for v2.x series (see `docs/FUTURE_GPU_SUPPORT.md`)

### Fixed
- **FIND-003 (CRITICAL):** RFC 3161 Timestamp Authority implementation complete
  - Resolves eIDAS compliance gap for qualified electronic timestamps
  - Enables legally binding digital signatures in EU
  - Supports long-term signature validation for regulated industries

### Security
- Enabled cryptographic timestamps for audit trails and document signing
- Added eIDAS-compliant timestamp validation
- Improved certificate chain verification for TSA responses

### Documentation
- Added comprehensive TSA setup guide (400+ lines)
- Documented integration with multiple TSA providers
- Added troubleshooting guide for common TSA issues
- **Added GPU Support Roadmap Documentation**
  - `docs/FUTURE_GPU_SUPPORT.md` - Detailed GPU roadmap for v2.x
  - `docs/GPU_SUPPORT_ROADMAP.md` - User migration guide
  - Updated `docs/GPU_VECTOR_INDEXING.md` - CPU-only status notice
  - Updated `docs/GPU_VECTOR_INDEXING_ARCHITECTURE.md` - Future architecture
  - Updated `README.md` - Clarified CPU-only vector indexing status
- Updated compliance documentation for eIDAS and ETSI EN 319 422

---

## [1.4.2] - 2026-02-06

### Changed
- **Vector Quantization Migration to FAISS**
  - ProductQuantizer now uses FAISS native implementation when available
  - Maintains API compatibility with existing code
  - Provides fallback implementation for non-FAISS builds
  - ResidualQuantizer automatically benefits through composition
  - Expected performance improvements through FAISS SIMD optimizations

### Added
- **FAISS ADC Optimization**: Implemented Asymmetric Distance Computation tables
  - ~40% faster asymmetric distance computation with FAISS
  - Uses precomputed asymmetric distance tables instead of decode + L2 distance
  - Automatic fallback to decode method on error or when FAISS unavailable
- **Performance Documentation**: Added `docs/PRODUCT_QUANTIZER_OPTIMIZATION.md`
  - Detailed benchmarking guidelines
  - GPU acceleration architecture documentation
  - Performance tuning recommendations

### Improved
- Reduced quantization code complexity by leveraging FAISS library
- Better maintainability through external library usage
- Conditional compilation support for FAISS availability
- Optimized distance computation path for production workloads

---

## [1.4.0] - 2026-01-19

### Added - Modular Architecture

- **Modular Build System**: Split monolithic `themis_core` into focused module libraries
  - `themis_base`: Core utilities, cross-cutting concerns, plugin infrastructure
  - `themis_storage`: Storage engine, indexes, backup management
  - `themis_query`: Query engine, AQL parser, analytics
  - `themis_security`: Encryption, PKI, RBAC, authentication
  - `themis_transaction`: Transaction management, CDC, saga support
  - `themis_network`: HTTP/gRPC servers, API handlers
  - `themis_sharding`: Distributed system (optional)
  - `themis_llm`: LLM integration (optional)
  - `themis_content`: Content processors (optional)
  - `themis_timeseries`: Time-series support (optional)
  - `themis_graph`: Graph analytics (optional)
  - `themis_geo`: Geospatial features (optional)
- **Export Macro System**: Platform-specific DLL export/import macros for all modules
- **Configurable Modules**: Optional modules can be excluded via CMake options
- **Backward Compatibility**: Monolithic build remains default; modular enabled with `-DTHEMIS_BUILD_MODULAR=ON`

### Changed

- **BinaryQuantizer Simplified**: Reduced implementation by 79 lines (-34%)
  - Marked as `@deprecated` - NOT used in production code
  - Recommends using FAISS `IndexBinaryFlat` for production workloads
  - Maintains API compatibility for existing tests
  - Part of FAISS migration initiative (see `LIBRARY_USAGE_ANALYSIS.md`)

- **LearnedQuantizer Marked as Research/Deprecated**: 393 lines
  - Marked as `@deprecated` - NOT used in production code
  - Research implementation for vector compression studies
  - Maintained for experimental workloads only
  - Part of code cleanup initiative (see `LIBRARY_USAGE_ANALYSIS.md`)

### Fixed

- **Windows Build Issues**: Resolves COFF symbol limit (>65,000 symbols) by splitting into smaller modules
- **Build Performance**: Parallel module compilation reduces full rebuild time by 30-50%

### Documentation

- Added `docs/architecture/MODULARIZATION_GUIDE.md` with comprehensive usage examples
- Updated build documentation with modular build instructions

---

## [1.8.0] - 2026-03-22

> **Release Aggregation Document:** [`docs/de/releases/RELEASE_NOTES_v1.8.0.md`](docs/de/releases/RELEASE_NOTES_v1.8.0.md)
> **Aggregation Issue:** [#4300](https://github.com/makr-code/ThemisDB/issues/4300)

### Added
- **JWT Scope Enforcement** — `JWTClaims.scopes` from OAuth2 `scope`/`scp` claims; `authorizeViaJWT()` / `authorizeViaKerberos()` enforce `required_scope` against `role_scope_map_`; `setRoleScopeMapping()` + `setJWKSForTesting()` API (PR #4279, #4270)
- **ArrowUserRegistrationPlugin** — Apache Arrow-backed in-memory user store; `bulkSyncFromArrow()` upsert; `authenticateFromArrow()` SHA-256 verification; 13 tests (PR #4280, Issue #99)
- **CRL / OCSP Certificate Revocation** — `PluginSecurityVerifier::checkCRL()` + `checkOCSP()` with libcurl HTTP, OpenSSL DER parse, per-serial cache; 24 tests (PR #4283, #4292, Issue #38)
- **Serializable Snapshot Isolation (SSI)** — `IsolationLevel::SerializableSnapshot=4`; `SSIConfig`; `detectConflicts()` range-intersection; predicate lock API; 38 tests (PR #4281, Issue #122)
- **SAGA Orchestration Engine** — `SAGAOrchestrator` with execute/validate/getStatus/getMetrics/template management; 23 tests
- **Versioned API Routing** — `RouteVersionRouter` (301 to `/v1/`); `/v2/` bulk NDJSON, SSE streaming, async jobs via `AdaptiveQueryCache`; 37 tests (PR #4285)
- **PredictivePrefetcher Markov ML** — order-1 Markov chain + 24-bucket ToD weighting; RocksDB persistence; A/B toggle; 14 tests
- **Cache Warmup Parallel Bulk Load** — concurrent startup pre-population (PR #4250, Issue #244)
- **Geo Clustering** — `GeoClusteringEngine::dbscanCluster()` + `kmeansCluster()`; 20 tests; perf opt-in via `THEMIS_RUN_PERF_TESTS=1` (Issue #4003)
- **PolicyManager Hot-Reload** — `reloadPolicies()` with `PolicyValidator`, double-buffer swap, `governance_policy_reload_total` counter; 7 tests
- **HuggingFace Hub 429 Back-off** — `Retry-After` parse (integer + HTTP-date); `ExporterMetrics::recordRateLimitHit()`; 5 tests
- **HardwareAccelerator operator completeness** — `FilterLessThanOp` + `FilterGreaterThanOrEqualOp`; 45 tests (PR #4289, Issue #85)
- **ExporterFactory** — concrete `ArrowIPCExporter`, `ParquetExporter`, `FeatherExporter`, `JSONCSVExporter`; 43+ tests (PR #4284, Issue #3868)
- **JoinExporter** — cross-collection hash-join export with PII redaction + memory budget (PR #4297)
- **Wire Protocol V2** — RFC 7540 §6.3 PRIORITY + §5.3.1 cycle detection; all 4 ACs complete (PR #4266, #4267)
- **SIGHUP Hot-Reload** — inotify / kqueue / ReadDirectoryChangesW cross-platform file watcher; 250 ms debounce (PR #4253)
- **GpuErasureCoderOpenCL** — OpenCL-accelerated encode/decode/batchEncode (PR #4265, Issue #105)
- **Intelligent Prefetching System** — access-pattern prefetch scheduler with Markov lookahead (PR #4257, Issue #192)
- **Materialized Views & Incremental Maintenance** — `MaterializedViewManager` with delta refresh (PR #4258, Issue #195)
- **UDP Ingestion Server** — fire-and-forget UDP server for metrics/telemetry sinks (PR #4271, Issue #190)
- **Bandwidth Management / QoS** — token-bucket rate limiting; CRITICAL/HIGH/NORMAL/BULK priority queues; Prometheus metrics (PR #4273, Issue #190)
- **MySQL / MariaDB Importer** — streaming cursor, type mapping, TLS, connection pooling (PR #4288)
- **`DistributedGraphManager` read-path shared_mutex** — TSAN-verified concurrent read/write locking (PR #4299)
- **`ProcessGraphVisitLog`** — per-node visit timestamps for process graph traversal (PR #4254)
- **`ProvenanceTracker` live engine** — replaces AQL template stubs with real `AQLEngine` connection (PR #4268)
- **TSStore buffering + SIMD decode** — Gorilla insert buffering; AVX-512/AVX2/NEON/scalar dispatch; ~35% CPU reduction for single-point ingestion (PR #4269)
- **RAG real LLM engine** — replaces `LLMIntegration` / `LLMJudgeIntegration` stubs (PR #4277)
- **CapabilityAutoGenerator persistence** — schedule state + YAML output persistence (PR #4275, Issue #217)
- **`/v1/admin/shards` endpoints** — list, detail, decommission; `OrphanDetector` wired to `DistributedCoordinator` (PR #4259, #4262)
- **`/v1/admin/storage/stats` endpoint** — RocksDB SST-property-based accurate disk usage (PR #4274, Issue #205)
- **Multi-GPU NVML device monitoring** — runtime device health via NVML (PR #4270)
- **`AsyncIngestionWorker` YAML config** — YAML-driven configuration + `user_context` propagation (PR #4296)
- **Abuse detection** — `abuse_detector.cpp` wired into CMake build (PR #4287)
- **`SecuritySignatureManager` RocksDB iteration** — full-iteration batch signature verification (PR #4260, Issue #206)
- **`ManifestDatabase::deleteManifest()`** — removes all associated sidecar files on entry removal (PR #4261)
- **Transaction Savepoints CI** — full CI coverage for savepoints (PR #4276)
- **OCC CI + correctness audit** — test accuracy fixes + CI workflow (PR #4264)
- **`TaskScheduler` user-context propagation** — `user_id` / `auth_method` in all audit events (PR #4278)
- **`ConfigEncryptedStore` concurrent reads** — `mutex_` upgraded to `std::shared_mutex` (PR #4295)
- **Config Audit Trail** — atomic hot-path; concurrency regression test (PR #4286)
- **`MetricsCollector` concurrent reads** — mutex upgraded to `std::shared_mutex` (PR #4272)
- **`PluginRegistry` concurrent reads** — mutex upgraded to `std::shared_mutex`; WASM scaffold (PR #4256)
- **CDC sequence counter** — audit complete; `AUDIT.md` updated (PR #4294)
- **`PKIClient` v1.8.0** — replaces fallback stub verification (PR #4263)

### ⚠️ Breaking Changes
- **ZSTD compression** — `StreamWriter` replaces zlib (DEFLATE) with ZSTD; update link dependency from `libz` to `libzstd` (PR #4252)
- **HTTP path routing** — unversioned paths now redirect 301 to `/v1/`; update client paths accordingly (PR #4285)
- **CI workflow paths** — 138 workflows reorganised into 9 categories; see `.github/WORKFLOW_REGISTRY.md` for mapping (PR #4290)

### Fixed
- `CEPEngine` deadlock — window lock now released before invoking user callbacks (PR #4291)
- PE certificate parsing off-by-one in `DataDirectory[4]` size; ELF `.security` sidecar added (PR #4292)
- OCC conflict detection test correctness (PR #4264)
- `ProvenanceTracker` AQL template stub (PR #4268)
- Config Audit Trail concurrent entry drop under load (PR #4286)
- `SecuritySignatureManager` prefix end-condition in RocksDB iterator (PR #4260)
- `ManifestDatabase` orphaned sidecar artefacts on delete (PR #4261)

### Changed
- `BackendRegistry` logging upgraded from `std::cout` to structured logger (PR #4251)
- `RocksDBWrapper::approximateSize()` uses SST property instead of estimate (PR #4274)
- `TaskScheduler` audit events include authenticated user identity (PR #4278)

---

## [1.7.0] - 2026-03-09

> **Release Aggregation Document:** [`docs/de/releases/RELEASE_NOTES_v1.7.0.md`](docs/de/releases/RELEASE_NOTES_v1.7.0.md)
> **Aggregation Issue:** [#3486](https://github.com/makr-code/ThemisDB/issues/3486) · **Parent:** [#3073](https://github.com/makr-code/ThemisDB/issues/3073)

### Added
- **Config Architecture Reorganization** — hierarchical `config/` directory structure with 16 category subdirectories; `ConfigPathResolver` for backward-compatible legacy path resolution; migration guide at `config/MIGRATION_GUIDE.md`
- **Multi-GPU Vector Indexing API (v2.4 scaffolding)** — `MultiGPUVectorIndex` with round-robin/hash/range/balanced partition strategies; query fan-out and top-k merge (CPU-backed; GPU execution planned v2.5+)
- **Git-Like Features Integration** — SnapshotManager (named MVCC snapshots), PITR REST API (restore by sequence/tag/timestamp + preview), MergeEngine REST API (3-way merge, fast-forward check), enhanced BranchManager
- **HybridSearch production hardening** — configurable vector metric (COSINE/DOT/L2), strict config validation, `SearchStats`, exception-safe search, pre-normalization; 88+ tests
- **Distributed Query Optimizer (v1.5.x)** — dynamic shard row estimates, predicate selectivity, network latency hooks
- **FAISS ADC distance table acceleration** — ~40% faster `IndexIVFPQ` search via precomputed distance tables; enabled by default
- **Documentation validation CI** — `.github/workflows/documentation-validation.yml` with 5 jobs (link-check, markdown-lint, spell-check, structure-check, summary)
- **44-module documentation audit** — all module READMEs, ROADMAPs, and ARCHITECTUREs aligned with actual source implementations
- **Test + benchmark coverage audit** — 6 new benchmark suites + 21 new unit test files closing coverage gaps across all 44 modules
- **RAG scientific foundations** — `docs/en/rag/RAG_SCIENTIFIC_FOUNDATIONS.md`: 460-line IEEE reference with 40 peer-reviewed citations

### ⚠️ Breaking Changes
- **themis module migration** — module initialisation code migrated from `src/utils/` and `src/base/` to `src/themis/`; update `#include` paths accordingly

### Fixed
- 119 broken documentation links corrected in hub/index files
- `DiffEngine` initialization updated to accept optional `SnapshotManager` reference
- Re-enabled `SnapshotManager` (was disabled due to incomplete type issues)

---

### Added
- **API Versioning and Compatibility Strategy**: Comprehensive API versioning infrastructure
  - **Accept-Version header** support for REST APIs to specify desired API version
  - **API-Version response header** indicating the API version used to process the request
  - **Deprecation tracking system** with automated warning headers (Deprecation, Sunset, Link)
  - **24-month deprecation policy** ensuring backward compatibility and smooth migrations
  - **gRPC version negotiation** via metadata (`api-version` key)
  - **Version resolution** supporting formats: `v1.4.1`, `v1.4`, `v1`, `latest`
  - **APIVersionManager** class for centralized version management
  - **Compatibility matrix** documenting supported versions (v1.0.0 to v1.4.1)
  - **Migration guide framework** with templates and best practices
  - Comprehensive documentation:
    - [API Versioning Strategy](docs/api/API_VERSIONING.md)
    - [Deprecation Registry](docs/api/DEPRECATION_REGISTRY.md)
    - [Migration Guides](docs/migration/README.md)
    - [v1.3 to v1.4 Migration Guide](docs/migration/v1.3-to-v1.4.md)
  - Updated proto files with API version metadata
  - Related to #751 (API-Versionierung und Kompatibilitäts-Strategie)
- **Query Result Pagination**: Comprehensive pagination support for query results with multiple strategies
  - **Cursor-based pagination** with expiration and versioning (1-hour TTL default)
  - **Keyset pagination** using ORDER BY values for O(log n) performance
  - **Configurable page sizes** with validation (min: 1, max: 10,000, default: 100)
  - Enhanced `PaginatedResponse` with detailed metadata (`PageInfo`, `has_next_page`, `has_prev_page`)
  - ORDER BY value encoding in cursors eliminates database lookups for sort values
  - Cursor expiration prevents stale cursor accumulation
  - Multiple pagination methods supported: CURSOR, OFFSET, KEYSET
  - 17 comprehensive tests with 100% pass rate
  - Backward compatible with existing pagination API
  - Related to #751
- **Plugin Metrics and Monitoring**: Comprehensive metrics tracking for all plugins with Prometheus integration
  - `PluginMetrics` class for thread-safe metrics collection
  - Automatic tracking of load time, reload time, function call latency (P95/P99)
  - Resource usage monitoring (memory per plugin)
  - Error tracking and count metrics
  - JSON API endpoint: `/api/plugins/metrics`
  - Prometheus metrics integrated into `/metrics` endpoint
  - <1% performance overhead from instrumentation
  - See [Plugin Metrics Documentation](docs/plugins/PLUGIN_METRICS.md)
- **CHIMERA Suite Branding**: Rebranded benchmark framework to "CHIMERA Suite" (_Comprehensive Hybrid Inferencing & Multi-model Evaluation Resource Assessment_)
  - Tagline: "Benchmark the Unbenchmarkable"
  - Vendor-neutral, scientifically rigorous benchmark framework
  - Updated all documentation, scripts, and CI workflows
  - Result files now use `CHIMERA_RESULTS_*` naming pattern
  - See [CHIMERA Suite Documentation](benchmarks/chimera/README.md)
- Documentation Archival System - Formal process for archiving outdated documentation
- Retroactive Release Building System - Build binaries from historical version tags
- Schema Manager for database self-awareness and introspection
- Independent Health/Error service on alternate port (9090)

### Performance
- **Query Pagination Improvements**:
  - Reduced database lookups by storing ORDER BY values in cursors
  - O(log n) keyset pagination vs O(n) offset-based pagination
  - Memory efficiency through configurable page size limits (max 10,000 items)
  - Cursor expiration prevents stale cursor accumulation

### Changed
- **Documentation Reorganization**: Major cleanup and restructuring of documentation
  - Fixed version inconsistencies across README, VERSION file, and badges
  - Moved 70+ historical implementation documents to `docs/implementation-history/` archive
  - Created comprehensive archive README explaining historical documents
  - Updated all broken links in main documentation files
  - Added archive reference in main documentation index
  - Cleaner root directory with only essential documentation files
- Improved documentation structure and organization
- Benchmark suite renamed to CHIMERA Suite with comprehensive rebranding

---

## [1.4.0-stable] - 2026-01-19

### 🎯 Extended Context Window (32K+) - Production Ready

**Status Change:** Experimental (v1.4.0-alpha) → Production-Ready (v1.4.0-stable)

#### Added

**Configuration & Feature Flags:**
- Comprehensive extended context configuration (`config/llm_extended_context.yaml`)
- Feature maturity status flags ("experimental", "beta", "stable")
- Backward compatibility mode with automatic fallback
- Production validation checks (memory, model support, RoPE config, thread-safety)
- Model-specific configuration overrides
- [Configuration Reference](config/llm_extended_context.yaml)

**RoPE/YARN Scaling - Production Ready:**
- Finalized integration on both Model and API levels
- All scaling methods production-ready: Linear, NTK, YaRN, Dynamic
- YaRN parameters fully configurable (ext_factor, attn_factor, beta_fast, beta_slow)
- Error handling and validation for scaling configuration
- [Production Guide](docs/de/llm/EXTENDED_CONTEXT_PRODUCTION_GUIDE.md)

**Memory Profiling & Monitoring:**
- 30+ new Prometheus metrics for extended context monitoring
  - Context window metrics: length, cache size, scaling factor
  - RoPE/YARN metrics: method, errors, YARN parameters
  - Memory metrics: RAM/VRAM usage, pressure, OOM events
  - Thread-safety metrics: LoRA switches, lock contention
- Memory estimation utilities with accuracy tracking
- Real-time RAM/VRAM profiling per model
- Memory pressure alerts and OOM prevention
- Grafana dashboard templates

**Thread-Safety:**
- Sequential LoRA operations mode for context scaling
- Configurable mutex-based synchronization
- Lock timeout configuration (default: 1000ms)
- Lock contention monitoring and alerts
- Safe concurrent request handling

**Documentation:**
- [Extended Context Production Guide](docs/de/llm/EXTENDED_CONTEXT_PRODUCTION_GUIDE.md)
- [Status Update v1.4.0](docs/de/llm/EXTENDED_CONTEXT_STATUS_UPDATE.md)
- Memory requirements calculator
- Deployment checklist and best practices
- Troubleshooting guide
- Migration guide from v1.4.0-alpha

#### Changed

**Extended Context:**
- Updated llm_config.example.yaml with extended_context section
- Improved RoPE scaling quality for high factors (>8x)
- Enhanced memory estimation accuracy (±10% for most models)
- Better error messages for configuration issues

#### Fixed

**Issues Resolved (GAP Analysis):**
- ✅ RoPE/YARN integration finalized on Model and API level
- ✅ Thread-safety for Context Scaling with LoRA/Adapters
- ✅ Comprehensive RAM/VRAM profiling and monitoring
- ✅ Feature flags and backward compatibility
- Reference: [INVESTIGATION_GAPS_SIMULATIONS_THEMISDB.md](docs/implementation-history/INVESTIGATION_GAPS_SIMULATIONS_THEMISDB.md)

**Production Readiness Score:**
- v1.4.0-alpha: 38% → v1.4.0-stable: 93%
- All critical gaps addressed
- Safe for production deployment with gradual rollout strategy

---

## [1.4.0-alpha] - 2026-01-05

### Added

#### 🧠 Advanced LLM Capabilities
- **Grammar-Constrained Generation** - EBNF/GBNF support for guaranteed valid JSON/XML/CSV outputs (95-99% reliability)
  - Built-in grammars: JSON, XML, CSV, ReAct Agent
  - Thread-safe grammar cache with LRU eviction
  - [Documentation](docs/en/llm/GRAMMAR_CONSTRAINED_GENERATION.md)
  
- **RoPE Scaling** - Extended context window from 4K → 32K tokens (8x increase)
  - Linear, NTK-aware, YaRN scaling methods
  - [Documentation](docs/en/llm/ROPE_SCALING_IMPLEMENTATION.md)
  
- **Vision Support** - Multi-modal LLMs with CLIP-based image encoding
  - LLaVA integration for image analysis
  - Single and multiple image support
  - [Documentation](docs/en/llm/VISION_SUPPORT_QUICK_START.md)
  
- **Flash Attention** - CUDA kernels for 15-25% speedup, 30% memory reduction
  - [Documentation](docs/en/llm/FLASH_ATTENTION_IMPLEMENTATION.md)
  
- **Speculative Decoding** - 2-3x faster inference with draft+target models
- **Continuous Batching** - 2x+ throughput with dynamic request batching

#### 🏢 Enterprise Features
- Hot Spare Management - Automatic failover with health monitoring
- Enhanced Prometheus Metrics - LLM inference and cache performance tracking
- WAL Replication via gRPC - Distributed inter-shard replication
- Multi-GPU LoRA Support - Distributed LoRA adapters across GPUs
- PostgreSQL Protocol Enhancements - COPY, prepared statements, transaction support

### Changed
- 31 new test suites with comprehensive coverage
- 11 new performance benchmarks
- 17 new documentation guides
- 938 files changed (+113,762 lines, -45,154 lines)

**[→ Complete Release Notes](release-changelogs/v1.4.0-alpha.md)**

---

## [1.3.4-hotfix] - 2026-01-04

### Fixed
- **CRITICAL:** Fixed server hang at "Adaptive Index Manager initialized" in RAID cluster mode
  - Root cause: AdaptiveIndexManager MVCC coordination before Sharding Manager initialization
  - Solution: Conditional Column Family opening when `THEMIS_ENABLE_SHARDING=true` detected
  - Files: `src/storage/rocksdb_wrapper.cpp`, `src/server/http_server.cpp`
  
- **CRITICAL:** Fixed incorrect Docker Compose port mappings (`808X:8080` → `808X:8765`)
  - All 9 RAID shards now properly expose HTTP/REST API endpoints
  - File: `docker/compose/docker-compose-sharding.yml`

### Added
- RAID Endurance Test Suite - 2-hour automated testing for all RAID modes
  - Script: `scripts/raid_endurance_test.py`
  - Monitoring: `scripts/monitor_raid_test.ps1`
  - Verification: All 9 RAID shards (RAID 0/1/5) operational with 0% error rate

### Changed
- Docker build context reduced from 3GB to 85MB (97% reduction)
- Updated `.dockerignore` to exclude build artifacts while preserving vcpkg baseline
- Improved Dockerfile.themis-server for more reliable builds

**[→ Complete Release Notes](release-changelogs/v1.3.4-hotfix.md)**

---

## [1.3.4] - 2026-01-02

### Security
> **Comprehensive Security Summary:** See [Security Work Summary v1.3.4](docs/de/releases/SECURITY_WORK_SUMMARY_V1.3.4.md)

#### Fixed
- **7 Critical Security Vulnerabilities** in RocksDB wrapper (100% segfault risk elimination)
  - Use-after-free in BlockBasedTableOptions
  - Null-pointer checks for environment initialization
  - Transaction-based deletion to prevent deadlocks
  - GetBaseDB() null-pointer checks across 7 locations
  - Transaction resource leak fixes
  - Column Family handle cleanup improvements
  - BackupEngine exception safety
  - [Audit Report](docs/ROCKSDB_WRAPPER_AUDIT_REPORT.md)

- **8 Medium-Severity Issues**
  - Improved transaction error handling
  - Enhanced iterator lifecycle management
  - Better snapshot handling
  - Backup engine null-checks

#### Changed
- Upgraded Docker base image: Ubuntu 22.04 → Ubuntu 24.04 LTS (80%+ CVE reduction)
- Secure token handling in Update Checker (no hardcoded credentials)
- Binary authenticity verification with cryptographic manifest signing (RSA-4096, SHA-256)

**[→ Complete Release Notes](release-changelogs/v1.3.4.md)**

---

## [1.3.3] - 2025-12-21

### Added
- **HTTP/2 with Server Push** - CDC/Changefeed with ~0ms latency
- **WebSocket Support** - Bidirectional streaming for real-time communication
- **MQTT Broker** - IoT messaging with WebSocket transport and monitoring
- **HTTP/3 Base Implementation** - QUIC protocol (experimental)
- **PostgreSQL Wire Protocol** - BI tool compatibility
- **MCP Server** - Model Context Protocol support for LLM integration

**[→ Complete Release Notes](release-changelogs/v1.3.3.md)**

---

## [1.3.2] - 2025-12-21

### Added
- **Image Analysis AI Plugin Architecture** running parallel with LLM
  - Multi-backend support: llama.cpp Vision, ONNX Runtime, OpenCV DNN, OpenVINO, ncnn
  - Plugin interfaces: `IImageAnalysisBackend`, `ImageAnalysisManager`
  - 15+ comprehensive unit tests and benchmarks

**[→ Complete Release Notes](release-changelogs/v1.3.2.md)**

---

## [1.3.1] - 2025-12-20

### Added
- `ATTRIBUTIONS.md` documenting 15+ core dependencies
- Documentation of ThemisDB's **12 unique innovations**
- Clear attribution for all major dependencies

**[→ Complete Release Notes](release-changelogs/v1.3.1.md)**

---

## [1.3.0] - 2025-12-17

### Added
- **Native LLM Integration with llama.cpp** (optional feature)
  - Embedded LLM engine for LLaMA/Mistral/Phi-3 (1B-70B parameters)
  - GPU acceleration with NVIDIA CUDA support
  - PagedAttention for advanced memory management
  - Quantization support (Q4_K_M, Q5_K_M, Q8_0)
  - Grafana dashboards with metrics and alerts
  - [Setup Guide](docs/de/guides/LLM_COMPLETE_SETUP_GUIDE.md)

- **Voice Assistant Integration** (Enterprise feature)
  - Natural language voice interaction (Whisper.cpp + Piper TTS + llama.cpp)
  - Phone call recording with automatic transcription
  - Meeting protocol generation with AI-powered minutes
  - Speaker diarization
  - Multi-language support (100+ languages)
  - [Documentation](docs/en/features/voice_assistant_guide.md)

**[→ Complete Release Notes](release-changelogs/v1.3.0.md)**

---

## Earlier Versions

For releases prior to v1.3.0, please see:
- [Release Changelogs Directory](release-changelogs/)
- [GitHub Releases Page](https://github.com/makr-code/ThemisDB/releases)

---

## Release Notes

Detailed release notes for each version are available in the [release-changelogs/](release-changelogs/) directory:

- [v1.4.0-alpha](release-changelogs/v1.4.0-alpha.md) - Advanced LLM features
- [v1.3.4-hotfix](release-changelogs/v1.3.4-hotfix.md) - RAID sharding deadlock hotfix
- [v1.3.4](release-changelogs/v1.3.4.md) - Security improvements
- [v1.3.3](release-changelogs/v1.3.3.md) - Network protocol enhancements
- [v1.3.2](release-changelogs/v1.3.2.md) - Image analysis AI plugin
- [v1.3.1](release-changelogs/v1.3.1.md) - Third-party attribution
- [v1.3.0](release-changelogs/v1.3.0.md) - LLM integration

---

## Upgrade Notes

### From 1.3.x to 1.4.0-alpha

- LLM features now include advanced capabilities (grammar constraints, RoPE scaling, vision support)
- New configuration options available for Flash Attention and Speculative Decoding
- See [Migration Guide](docs/MIGRATION_GUIDE.md) for detailed upgrade instructions

### From 1.2.x to 1.3.x

- LLM integration is now optional and requires explicit build flag: `-DTHEMIS_ENABLE_LLM=ON`
- New protocols (HTTP/2, WebSocket, MQTT) require explicit opt-in for security
- See [Configuration Guide](docs/en/guides/guides_configuration.md) for new settings

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on:
- How to contribute to ThemisDB
- Code style and standards
- Pull request process
- Documentation requirements

---

## Version Format

ThemisDB follows [Semantic Versioning](https://semver.org/):

- **MAJOR** version for incompatible API changes
- **MINOR** version for new functionality in a backward compatible manner
- **PATCH** version for backward compatible bug fixes
- **-alpha**, **-beta**, **-rc** suffixes for pre-release versions
