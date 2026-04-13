# Appendix D: Feature Status Matrix

**Version:** 1.5.0-dev  
**Stand:** 15. Februar 2026  
**Kategorie:** Feature-Übersicht und Implementierungsstatus

---

## Zweck dieses Appendix

Dieser Appendix bietet eine vollständige, strukturierte Übersicht aller ThemisDB-Features mit aktuellem Implementierungsstatus, Dokumentationsverweisen und Roadmap-Planung. Dies dient als zentraler Referenzpunkt für:

- **Entwickler**: Welche Features sind verfügbar? Wo ist der Code?
- **Architekten**: Welche Capabilities hat ThemisDB? Was fehlt noch?
- **Projektmanager**: Was ist Production-Ready? Was ist geplant?
- **Nutzer**: Welche Funktionen kann ich nutzen?

---

## Status-Definitionen

| Symbol | Status | Bedeutung |
|--------|--------|-----------|
| ✅ | **Production-Ready** | Vollständig implementiert, getestet (>85% Coverage), dokumentiert, performance-validated |
| 🟢 | **MVP Complete** | Kernfunktionalität vorhanden, stabil, aber ggf. noch Optimierungen ausstehend |
| 🟡 | **In Development** | Aktiv in Entwicklung, teilweise funktional, noch nicht production-grade |
| 🟠 | **Design Phase** | Spezifikation vorhanden, Implementierung geplant |
| 🔵 | **Planned** | Auf Roadmap, noch keine Spezifikation |
| ⚪ | **Backlog** | Idee/Anforderung dokumentiert, keine aktive Planung |
| 🆕 | **Alpha** | Neu in v1.4.0-alpha, noch nicht production-ready |
| ❌ | **Not Planned** | Bewusst nicht im Scope |

---

## Zusammenfassung

**Aktuelle Reife (Stand Feb 2026):**
- **Core Database**: ✅ Production-Ready (85%+ Test Coverage)
- **Multi-Model Support**: ✅ Production-Ready (Relational, Graph, Vector, Document, Time-Series)
- **Security & Compliance**: ✅ Production-Ready (DSGVO, BSI C5, Audit Logging)
- **Horizontal Scaling**: ✅ Production-Ready (2/4/8-Node Clusters, 91% Efficiency)
- **High Availability**: 🟢 MVP Complete (Hot Spare, WAL Replication, promoted from Alpha)
- **Voice Assistant**: 🆕 Beta (Whisper STT, Piper TTS, Meeting Protocols, Call Center Automation)
- **LLM/RAG Integration**: 🟢 MVP Complete (8 Features: Prefix/Response Caching, Multi-GPU, Paged Attention, LoRA, Grammar-Constrained Generation, RoPE Scaling, Vision - promoted from Alpha)
- **Performance Optimizations**: 🟢 MVP Complete (Flash Attention, Speculative Decoding, Continuous Batching)
- **PostgreSQL Wire Protocol**: 🟡 In Development (COPY, LISTEN/NOTIFY, Binary Format, Pipeline Mode - see src/server/FUTURE_ENHANCEMENTS.md)
- **Enhanced Monitoring**: ✅ Production-Ready (30+ Prometheus-Metriken für LLM, HA, Performance)

---

## v1.5.0-dev Feature-Highlights

### LLM-Integration (Kapitel 17)

| Feature | Status | Maturity | Documentation | Performance |
|---------|--------|----------|---------------|-------------|
| **Prefix Caching** | 🟢 MVP Complete | Stable | Chapter 17.12.1 | 75% cost savings |
| **Response Caching** | 🟢 MVP Complete | Stable | Chapter 17.12.2 | 60-80% savings |
| **Multi-GPU Support** | 🟢 MVP Complete | Stable | Chapter 17.12.3 | 4-8x throughput |
| **Paged Attention** | 🟢 MVP Complete | Stable | Chapter 17.12.4 | 80% memory reduction |
| **LoRA Support** | 🟢 MVP Complete | Stable | Chapter 17.12.5 | 99% less memory |
| **Grammar-Constrained Generation** | 🟢 MVP Complete | Stable | Chapter 17.12.6 | 95-99% valid outputs |
| **RoPE Scaling** | 🟢 MVP Complete | Stable | Chapter 17.12.7 | 4K→32K context |
| **Vision Support** | 🟢 MVP Complete | Stable | Chapter 17.12.8 | Multimodal AI |

**Roadmap:**
- Production-Ready (v1.6): Extended testing, performance tuning, production certification

### Enterprise Features (Kapitel 10)

| Feature | Status | Maturity | Documentation | Use Case |
|---------|--------|----------|---------------|----------|
| **Voice Assistant** | 🟡 Beta | Improving | Chapter 10.7 | Call Center, Meeting Protocols |
| **Multi-Tenancy** | ✅ Production-Ready | Stable | Chapter 10.1 | SaaS Applications |
| **RBAC** | ✅ Production-Ready | Stable | Chapter 10.1 | Enterprise Security |
| **Audit Logging** | ✅ Production-Ready | Stable | Chapter 10.1 | Compliance |

**Roadmap (Voice Assistant):**
- Production-Ready (v1.6): Enhanced language support, improved accuracy, production certification

### Performance-Optimierungen (Kapitel 21)

| Feature | Status | Maturity | Documentation | Performance |
|---------|--------|----------|---------------|-------------|
| **Flash Attention** | 🟢 MVP Complete | Stable | Chapter 20.9A.1 | 69% throughput, 37% memory |
| **Speculative Decoding** | 🟡 In Development | Experimental | Chapter 20.9A.2 | 2-3x speedup (target) |
| **Continuous Batching** | 🟢 MVP Complete | Stable | Chapter 20.9A.3 | 176% throughput |

**Roadmap:**
- Production-Ready (v1.6): Hardware compatibility testing, production deployment guides

### High Availability (Kapitel 16)

| Feature | Status | Maturity | Documentation | Performance |
|---------|--------|----------|---------------|-------------|
| **Hot Spare** | 🟢 MVP Complete | Stable | Chapter 16.10.1 | <5s failover |
| **WAL Replication** | 🟢 MVP Complete | Stable | Chapter 16.10.2 | Zero data loss |
| **Multi-SSD WAL** | 🆕 Alpha | Early | Chapter 16.10.2 | <10% write overhead |
| **Replication Slots** | 🆕 Alpha | Early | Chapter 16.10.2 | Lag monitoring |

**Roadmap:**
- Beta (Feb 2026): Extended failover testing, disaster recovery
- GA (Mar 2026): Production HA certification

### Monitoring & Observability (Kapitel 19)

| Feature | Status | Maturity | Documentation | Metrics Count |
|---------|--------|----------|---------------|---------------|
| **LLM Metrics** | 🆕 Alpha | Early | Chapter 19.6A.1 | 15+ metrics |
| **Performance Metrics** | 🆕 Alpha | Early | Chapter 19.6A.2 | 10+ metrics |
| **HA Metrics** | 🆕 Alpha | Early | Chapter 19.6A.3 | 8+ metrics |
| **Grafana Dashboards** | 🆕 Alpha | Early | Chapter 19.6A | 3 dashboards |
| **Alerting Rules** | 🆕 Alpha | Early | Chapter 19.6A.4 | 12+ rules |

**Roadmap:**
- Beta (Feb 2026): Dashboard refinement, extended alerting
- GA (Mar 2026): Complete observability stack

### PostgreSQL Wire Protocol (Kapitel 31)

| Feature | Status | Maturity | Documentation | Performance |
|---------|--------|----------|---------------|-------------|
| **COPY Protocol** | 🆕 Alpha | Early | Chapter 31.9A.1 | 250K rows/s |
| **LISTEN/NOTIFY** | 🆕 Alpha | Early | Chapter 31.9A.1 | Real-time CDC |
| **Binary Format** | 🆕 Alpha | Early | Chapter 31.9A.2 | 80% size reduction |
| **Pipeline Mode** | 🆕 Alpha | Early | Chapter 31.9A.2 | 17x throughput |
| **Prepared Stmt Cache** | 🆕 Alpha | Early | Chapter 31.9A.2 | 50x speedup |
| **Vector Type Mapping** | 🆕 Alpha | Early | Chapter 31.9A.3 | Native pgvector |
| **JSONB Support** | 🆕 Alpha | Early | Chapter 31.9A.3 | Full compatibility |

**Roadmap:**
- Beta (Feb 2026): Extended client library testing
- GA (Mar 2026): Full PostgreSQL compatibility certification

---

## Vollständige Feature-Matrix

**Vollständige Feature-Matrix siehe:**
- Admin Tools: [`feature_matrix.md`](../admin_tools/feature_matrix.md)
- Features Overview: [`features_overview.md`](../features/features_overview.md)
- Sharding Status: Chapter 16 (Horizontal Scaling)
- Security Status: Chapter 10 (Section 10.2)
- Query Optimization: Chapter 15 (Section 15.4)

---

## Feature-Maturity-Levels

### Alpha (v1.4.0-alpha)
**Definition:** Neue Features in früher Entwicklung, für Feedback und Testing.

**Charakteristiken:**
- ✅ Kernfunktionalität implementiert
- ✅ Dokumentation vorhanden
- ⚠️ Noch nicht production-ready
- ⚠️ API kann sich ändern
- ⚠️ Performance noch nicht final optimiert

**Empfohlene Nutzung:**
- Development/Staging Environments
- Feature Evaluation
- Feedback und Bug Reports

### Beta (geplant: Feb 2026)
**Definition:** Features sind stabil, werden für Production vorbereitet.

**Charakteristiken:**
- ✅ Vollständig implementiert
- ✅ >80% Test Coverage
- ✅ Performance-validated
- ⚠️ Noch unter intensivem Testing
- ⚠️ Minor API changes möglich

**Empfohlene Nutzung:**
- Pre-Production Testing
- Pilot-Deployments
- Performance Benchmarking

### GA (General Availability - geplant: Mar 2026)
**Definition:** Production-ready, vollständig getestet und dokumentiert.

**Charakteristiken:**
- ✅ Production-Ready
- ✅ >85% Test Coverage
- ✅ Performance-validated
- ✅ Complete Documentation
- ✅ Stable API
- ✅ Long-term Support

**Empfohlene Nutzung:**
- Production Deployments
- Mission-Critical Workloads
- Enterprise Applications

---

## Implementierungsstatus v1.4.0-alpha

### Gesamt-Übersicht

| Kategorie | Features | Alpha | Beta | GA | Geplant |
|-----------|----------|-------|------|----|---------| 
| **LLM Integration** | 6 | 6 | 0 | 0 | 0 |
| **Performance** | 3 | 3 | 0 | 0 | 0 |
| **High Availability** | 4 | 4 | 0 | 0 | 0 |
| **Monitoring** | 5 | 5 | 0 | 0 | 0 |
| **PostgreSQL Protocol** | 7 | 7 | 0 | 0 | 0 |
| **Gesamt** | 25 | 25 | 0 | 0 | 0 |

**Prozentsatz:**
- Alpha Features: 100% (25/25)
- Production-Ready: 0% (Target: 100% by Mar 2026)

---

## Implementierungsstatus v1.9.x (2026-04-12)

Die folgenden Komponenten wurden mit dem v1.9.x-Release (Commit 2026-04-12) in den Produktionscode aufgenommen:

### Storage & Ingest

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `StreamingIngestManager` | `storage/` | ✅ GA | SM-01..SM-10 | Ring-Buffer-Ingest ≥ 1 M Events/s, OverflowPolicy BLOCK/DROP |
| `TSStore::putBatch` | `timeseries/` | ✅ GA | TB-01..TB-14 | Zero-Copy Batch Write via `std::span<const TSRow>` |
| `TsStreamCursor` | `timeseries/` | ✅ GA | SC-01..SC-08 | Lazy paginierter Iterator, page_size=4 096 |
| `TemporalCompressor LZ4` | `temporal/` | ✅ GA | TC-LZ4-01..TC-LZ4-05 | LZ4-Block-Kompression im TemporalCompressor |
| `STORAGE_COMPACTION` Wiring | `maintenance/` | ✅ GA | — | `StorageCompactionHandler` in `maintenance_orchestrator_` registriert |
| `MVCC_CLEANUP` Wiring | `maintenance/` | ✅ GA | — | `MvccCleanupHandler` mit 24 h Watermark registriert |

### Maintenance

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `DatabaseMaintenanceOrchestrator` | `maintenance/` | ✅ GA | 40+ (OrchestratorTests) | Zentraler Wartungskoordinator: CRUD, Cron, DAG, Window-Enforcement, Audit |
| `MaintenanceScheduleStore` | `maintenance/` | ✅ GA | — | RocksDB-Persistenz für Schedules (`maint_sched::<id>`) |
| `StorageCompactionHandler` | `maintenance/` | ✅ GA | — | Kapselt `CompactionManager::compactAll()` |
| `MvccCleanupHandler` | `maintenance/` | ✅ GA | — | Bereinigt MVCC-Versionen mit konfigurierbarem Watermark |
| DAG Task Dependencies | `maintenance/` | ✅ GA | — | `depends_on`-Felder; topologische Sortierung via Kahn; Zykelerkennung |
| Maintenance Window Enforcement | `maintenance/` | ✅ GA | — | Tasks außerhalb `window_utc_hours` → SKIPPED |
| 11 REST Endpoints | `maintenance/` | ✅ GA | — | `/api/v1/maintenance/schedules` + `/jobs` + `/status` |

### Analytics

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `ColumnarCache` | `storage/` | ✅ GA | CC-01..CC-12 | LRU-Eviction + PinGuard RAII, ≥ 10× Speedup |
| `IStreamingJoin` / `HashJoin` | `analytics/` | ✅ GA | SJ-01..SJ-15 | Equi-Join, Inner/LeftOuter, composite keys |
| `IntervalJoin` | `analytics/` | ✅ GA | SJ-01..SJ-15 | Zeitbasierter Event-Join, LRU-Pruning |

### Performance & Netzwerk

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `LockFreeHistogram<T>` | `performance/` | ✅ GA | LFH-01..LFH-12 | Lock-free, ≤ 20 ns record(), Exponential+Linear |
| `RequestCoalescer` (Singleflight) | `cache/` | ✅ GA | RC-01..RC-14 | Thundering-Herd-Schutz, fn() exakt einmal |
| `IoUringBatchedSender` | `network/` | ✅ GA | IUB-01..IUB-12 | O(1) Syscalls/Runde, writev(2)-Fallback |
| LIRS `shared_mutex` Fix | `performance/` | ✅ GA | — | TOCTOU-Race beseitigt; `get()` → `unique_lock` |
| RCU `g_rcu_reader_count` Fix | `performance/` | ✅ GA | — | `readers_active()` war immer false, jetzt korrekt |

### Geo-Modul (v1.x)

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| ST_BUFFER | `geo/` | ✅ GA | — | CPU-exact + Boost; GPU-Fallback mit Audit-Log |
| ST_UNION / ST_DIFFERENCE | `geo/` | ✅ GA | — | CPU-exact, Boost, GPU-fallback; AQL-Funktionen |
| CUDA Geo-Kernel | `geo/acceleration/` | ✅ GA | — | Haversine-Distanz, Containment-Batch (CUDA + ROCm/HIP) |
| S2-Zell-Indizierung | `geo/` | ✅ GA | — | Hierarchische S2-Zellen; `cellForPoint()`, `coveringCells()` |
| H3-Hexagonales Grid | `geo/` | ✅ GA | — | Uber H3; `geoToH3()`, `kRing()`, `compact()` |
| `TemporalSpatialQuery` | `geo/` | ✅ GA | — | Location-at-Time T; `locationAtTime()`, `entitiesInBboxAtTime()` |
| `RasterGrid` / `sampleAt()` | `geo/` | ✅ GA | — | Elevation-Sampling mit bilinearer Interpolation |
| `generateHeatmap()` | `geo/` | ✅ GA | — | Gaussian-KDE aus Punktwolke; konfigurierbare Bandbreite + Auflösung |
| Spatial JOIN | `geo/` | ✅ GA | — | Alle Paare innerhalb Distanz (CUDA-optimiert) |
| ROCm/HIP-Backend | `geo/` | ✅ GA | — | AMD GPU-Unterstützung als CUDA-Alternative |

### AI / Acceleration

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `AiHardwareDispatcher` | `acceleration/` | ✅ GA | AHD-01..AHD-30 | NPU-Prioritätskette, INT4/W4A8, ONNX-EP-Fallback |
| `GPUVectorIndex` Oversubscription | `index/` | ✅ GA | 26 (GPUMemOversubTests) | Partitions-basiertes VRAM-Paging; LRU/MRU/SEQUENTIAL Prefetch |
| `GPUMemoryOversubscriptionManager` | `index/` | ✅ GA | 26 (GPUMemOversubTests) | Unified Memory (cudaMallocManaged/hipMallocManaged); CPU-Fallback |
| `GPUUnifiedMemoryAllocator` | `index/` | ✅ GA | — | Transparente CUDA/HIP Unified Memory + CPU-Heap-Fallback |
| `CUDAVectorBackend` | `acceleration/` | ✅ GA | — | HNSW on GPU; Batch-KNN; CUDA Graph Capture; buildHnswAnnIndex |
| `MultiGPUVectorBackend` | `acceleration/` | ✅ GA | — | NCCL/RCCL Fan-out KNN; NVLink P2P; CPU-Fallback; bis zu N GPUs |
| `GeoAccelerationBridge` | `acceleration/` | ✅ GA | — | CUDA/Vulkan Haversine + Point-in-Polygon; Backend-Dispatch |
| Vulkan Compute (non-NVIDIA) | `acceleration/` | ✅ GA | — | SPIR-V Shaders: L2/Cosine/InnProd/TopK/Batch+Geo-Kernels |
| ROCm/HIP Backend (AMD) | `acceleration/` | ✅ GA | — | `ann_kernels.hip` + `geo_kernels.hip`; non-HIP CPU-Fallback |

### Utils

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| UUID v7 (`generate_uuid_v7`) | `utils/` | ✅ GA | UV7-01..UV7-20 | RFC 9562, 48-bit Timestamp, 18-bit Seq, MT19937-64 |
| Streaming ZSTD (`zstd_compress_stream`) | `utils/` | ✅ GA | ZS-01..ZS-10 | ZSTD_CStream/DStream, max_output_bytes DoS-Guard |

### Content-Modul (v1.x)

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `ContentManager::ingestRawBlob()` | `content/` | ✅ GA | — | Auto-MIME, 10-stufige Pipeline, Archive-Rekursion |
| `PdfProcessor` | `content/` | ✅ GA | — | Text-Extraktion via poppler-cpp |
| `OfficeProcessor` (OOXML/ODF) | `content/` | ✅ GA | — | DOCX/XLSX/PPTX/ODF via libzip+pugixml |
| `OfficeProcessor` (Legacy) | `content/` | ✅ GA | — | DOC/XLS/PPT via LibreOffice-Headless-Fallback |
| `HtmlProcessor` | `content/` | ✅ GA | — | Boilerplate-Entfernung |
| `MarkdownProcessor` | `content/` | ✅ GA | — | Frontmatter-Parsing |
| `SttProcessor` / `TtsProcessor` | `content/` | ✅ GA | — | Whisper-Transkription + TTS |
| `OcrProcessor` | `content/` | ✅ GA | — | Tesseract OCR für Bild-PDFs |
| `DeduplicationChecker` | `content/` | ✅ GA | — | pHash (Bilder) + MinHash+LSH (Text); Jaccard-Schwellenwert |
| `LanguageDetector` | `content/` | ✅ GA | — | Multi-Sprach-Erkennung mit Konfidenz |
| `ContentManagerLlm` | `content/` | ✅ GA | — | Summary, Topics, Sentiment, Category via LLM |

### Scheduler-Modul (v1.5)

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `TaskScheduler` | `scheduler/` | ✅ GA | — | Cron/Interval/CDC/Manual/Webhook Trigger; 5/6-Feld, Timezone |
| DAG-Workflow-Engine | `scheduler/` | ✅ GA | — | Topologische Ausführung, paralleler Fan-Out, `branch_condition` |
| Retry-Policies | `scheduler/` | ✅ GA | — | FIXED/EXPONENTIAL/LINEAR/JITTER/FIBONACCI |
| `TaskScheduler` Dynamic Scaling | `scheduler/` | ✅ GA | — | Auto-Concurrency via Queue-Tiefe + `enable_dynamic_scaling` |
| `TaskAuditManager` | `scheduler/` | ✅ GA | — | Durchsuchbares Audit-Log; GDPR-Modus |
| `TaskAnomalyDetector` | `scheduler/` | ✅ GA | — | Anomalie-Erkennung über Task-Ausführungs-Metriken |
| Prometheus-Export (`exportMetrics()`) | `scheduler/` | ✅ GA | — | Metriken-Text für Scraping |
| `HybridRetentionManager` | `scheduler/` | ✅ GA | — | 3-Stufen: Gorilla (0–7d), Varianz-Downsampling (7–365d), Tages-Aggregate (>365d) |
| `DistributedTaskCoordinator` | `scheduler/` | ✅ GA | — | Leader-Election; One-Runner-Per-Cluster |
| `ExternalSchedulerAdapter` | `scheduler/` | ✅ GA | — | Kubernetes CronJob + Apache Airflow Adapter |

### Analytics-Modul (v1.9)

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `OLAPEngine` GPU-Pfad | `analytics/` | ✅ GA | — | GPU-beschleunigt (threshold=10K rows); LRU Result Cache (TTL) |
| `OLAPEngine` CUBE/ROLLUP | `analytics/` | ✅ GA | — | CUBE (alle Dimensionskombinationen), ROLLUP (Hierarchisch), GROUPING SETS |
| Window Functions | `analytics/` | ✅ GA | — | ROW_NUMBER, SUM/AVG OVER, P50/P90/P99, ROWS/RANGE BETWEEN |
| `CEPEngine` (EPL Parser) | `analytics/` | ✅ GA | — | NFA Pattern Matching; EPL CREATE RULE; WINDOW/GROUP BY/HAVING/ACTION |
| `CEPEngine` Event Types | `analytics/` | ✅ GA | — | CDC (INSERT/UPDATE/DELETE), Graph, Query, System events; MPMC Ring Buffer |
| `AnomalyDetector` | `analytics/` | ✅ GA | — | Isolation Forest; konfigurierbare Kontaminationsrate; Rolling Window |
| `ModelServingEngine` | `analytics/` | ✅ GA | — | Online + Batch ONNX Inference; versioniertes Model Loading |
| Inkrementelle Materialized Views | `analytics/` | ✅ GA | — | CDC-getrieben; Delta-Update ohne Full-Recompute |

### Search-Modul (v2.4)

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `HybridSearch` | `search/` | ✅ GA | — | BM25+Vector RRF (k=60); konfig. Gewichte; Partial-Result Stats |
| `DistributedHybridSearch` | `search/` | ✅ GA | — | Cross-Shard RRF; mTLS; skip_failed_shards Degraded Mode |
| `SearchHighlighter` | `search/` | ✅ GA | — | Term Highlighting `<mark>`; Best-Passage Snippet Extraktion |
| `SearchResultStream` | `search/` | ✅ GA | — | Cursor-basierte Streaming-Pagination (total_k/page_size) |
| `ConversationalSearch` | `search/` | ✅ GA | — | Multi-Turn Context Window (last N turns); automatische Query-Reformulation |
| `FederatedSearch` | `search/` | ✅ GA | — | Tenant-isolierte Parallel-Suche mit per-Tenant Gewichtung |
| `FacetedSearch` | `search/` | ✅ GA | — | Facettierte Suche über kategorische + Bereichs-Felder |
| `LearningToRank` | `search/` | ✅ GA | — | Listwise/Pairwise LTR; Feature-basiertes Re-Ranking |
| `AutocompleteEngine` | `search/` | ✅ GA | — | Prefix-Trie Autocomplete; Fuzzy + Phonetik |
| `MultiModalSearch` | `search/` | ✅ GA | — | Text + Bild + Audio Query Fusion |

### Ingestion-Modul (v1.5)

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `IngestionManager` | `ingestion/` | ✅ GA | — | Parallele Multi-Source-Orchestrierung; Thread-Pool; Prometheus-Metriken |
| `IngestionBuilder` | `ingestion/` | ✅ GA | — | Fluent API (withKafkaSource/withApiSource/withHuggingFaceSource/...) |
| `GenericApiConnector` | `ingestion/` | ✅ GA | — | Cursor/Offset-Pagination; OAuth 2.0; libcurl; Exponential Back-off |
| `HuggingFaceConnector` | `ingestion/` | ✅ GA | — | Dataset-Split-Ingest; API-Token-Auth |
| `ObjectStorageConnector` | `ingestion/` | ✅ GA | — | S3/GCS/Azure Blob; Prefix-Filter |
| `KafkaConnector` | `ingestion/` | ✅ GA | — | Consumer-Group; Auto-Commit; Back-pressure |
| `WebCrawlerConnector` | `ingestion/` | ✅ GA | — | Sitemap + Recursive Crawl; politeness_delay |
| `CdcConnector` (PostgreSQL) | `ingestion/` | ✅ GA | — | Logical Replication Slot; WAL-basiertes CDC |
| `DatabaseConnector` (JDBC/ODBC) | `ingestion/` | ✅ GA | — | Batch-Select + Cursor-basierte Pagination |
| Quarantine + Retry | `ingestion/` | ✅ GA | — | Exponential Back-off; Dead-Letter nach max_retries |
| Incremental Checkpoint | `ingestion/` | ✅ GA | — | Fortschritt persistiert; Re-Start ab letztem Checkpoint |
| Plugin API | `ingestion/` | ✅ GA | — | `ISourceConnector`-Interface für Third-Party Konnektoren |

### Importers-Modul (v2.1)

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `PostgresImporter` v2.1 | `importers/` | ✅ GA | — | FK-Preservation (ON DELETE/UPDATE, DEFERRABLE); CHECK/EXCLUDE/GENERATED Constraints |
| `MongoImporter` | `importers/` | ✅ GA | — | BSON→JSON; _id-Preservation |
| `MySQLImporter` | `importers/` | ✅ GA | — | Batch-Import; Charset-Mapping |
| `OracleImporter` | `importers/` | ✅ GA | — | DDL + Data Export |
| `SqliteImporter` | `importers/` | ✅ GA | — | Lightweight; kein Server nötig |
| `FlatFileImporter` | `importers/` | ✅ GA | — | CSV/TSV/Parquet; Auto-Type-Detection; Parquet-Kompression |
| `KafkaImporter` | `importers/` | ✅ GA | — | Consumer-Group; Offset-Tracking |
| `S3Importer` | `importers/` | ✅ GA | — | Prefix-Filter; Multipart-Download |
| `IImporter::importDataStreaming()` | `importers/` | ✅ GA | — | Low-Memory Streaming; per-Row Callback; frühzeitiger Abbruch |
| `SchemaInferenceEngine` | `importers/` | ✅ GA | — | Implicit FK Discovery; Semantic Type Detection; Cardinality Estimation |
| `AuditedImporter` | `importers/` | ✅ GA | — | Audit-Trail für jede importierte Zeile + Schema-Änderung |
| `IImporterPlugin` API | `importers/` | ✅ GA | — | Plugin-Interface für Third-Party Importer |

---

## Implementierungsstatus — Plugins & Sharding

### Plugins (v2.x)

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `WhisperPlugin` v2.0 | `whisper/` | ✅ GA | 30 (WhisperPluginFocusedTests) | `IAudioBackend`, WavAudioChunkReader, Strategy Pattern, Provenienz-Stempel |
| `WavAudioChunkReader` | `whisper/` | ✅ GA | im WhisperPlugin-Suite | RIFF/WAV ohne libsndfile; 16-bit PCM + float32 |
| `SDPlugin` v2.1 | `stable_diffusion/` | ✅ GA | 45 (SDPluginFocusedTests, A–O) | Text2Img, Batch, Img2Img, SDPromptSanitizer, Provenienz |
| `SDStubGenerator` | `stable_diffusion/` | ✅ GA | im SDPlugin-Suite | CI-Stub ohne Modell-Datei |
| `SDPromptSanitizer` | `stable_diffusion/` | ✅ GA | im SDPlugin-Suite | Keyword-Blocklist, negative_prompt-Policy (SD-NP-01) |

### Network-Modul (v1.8)

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `WireProtocolServer` | `network/` | ✅ GA | — | Port 8766; dedizierted I/O+Worker-Pool; mTLS; SCRAM-SHA-256 |
| `WireProtocolServer` WebSocket-Upgrade | `network/` | ✅ GA | — | HTTP-Upgrade auf Port 8766; JSON Text-Frames (THEMIS_ENABLE_WEBSOCKET) |
| `WireProtocolServer` IPv6+Dual-Stack | `network/` | ✅ GA | — | IPv6-Socket + ipv6_dual_stack für IPv4-mapped Verbindungen |
| `QuicTransport` | `network/` | ✅ GA | — | Port 8770; UDP+TLS 1.3; 0-RTT; kein HOL-Blocking |
| `RaftLoadBalancer` | `network/` | ✅ GA | — | Raft In-Process-Konsensus; konsistentes Routing; Leader-Election |
| UDP Fast-Path | `network/` | ✅ GA | — | Port 8769; latenzoptimierter Ingest-Pfad |
| gRPC Transport | `network/` | ✅ GA | — | Port 8771; HTTP/2 Multiplexing |

### Security-Modul (v1.x)

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `RLSManager` | `security/` | ✅ GA | — | PostgreSQL-kompatible Row-Level Security; PERMISSIVE+RESTRICTIVE; JSON-Persistenz |
| `ZeroTrustPolicyEnforcer` | `security/` | ✅ GA | — | Per-Request Identity + CIDR Whitelist/Blacklist; Composite Trust Score |
| `FieldEncryption` (AES-256-GCM) | `security/` | ✅ GA | — | Field-Level Encryption; Key Rotation (90d); DEK/KEK/MasterKey Hierarchie |
| HSM Integration | `security/` | ✅ GA | — | HashiCorp Vault; HSM-backed Key Storage |
| AQL Injection Detection | `security/` | ✅ GA | — | Pattern-basierte Angriffserkennung in AQL-Queries |
| Malware Scanner (Plugin Manifests) | `security/` | ✅ GA | — | Plugin-Manifest-Scan vor Activation |
| CMS/PKCS#7 + eIDAS Timestamping | `security/` | ✅ GA | — | Digitale Signaturen; eIDAS-konformes Timestamping |

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `EthicsEvaluator` | `ethics_ai/` | ✅ GA | 28 (EthicsAiPluginFocusedTests) | 5-Dimensions-Scoring (Quality/Consistency/Fairness/Alignment/Transparency) |
| `PhilosophyLoader` | `ethics_ai/` | ✅ GA | 7 (PhilosophyLoaderFocusedTests) | YAML-Profilladung mit Cache und Validierung |
| `EthicalDiscourseEngine` | `ethics_ai/` | ✅ GA | 11 (DiscourseEngineFocusedTests) | `initializeDebate()` + `makeDecision()` |
| `ArgumentStore` | `ethics_ai/` | ✅ GA | 18 (ArgumentStoreStandaloneFocusedTests) | BaseEntity-Persistenz + Standalone-Modus |
| `RAGContextEngine` | `ethics_ai/` | ✅ GA | 13 (RAGContextEngineFocusedTests) | 7 AQL-Abfragemuster |
| Ethics AI Integration | `ethics_ai/` | ✅ GA | 21 (EthicsAiIntegration) | FullPipeline, ArgumentStoreRAG, RAGContextBuild |

### LLM Module — Produktionskomponenten (v1.0)

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `LlamaWrapper::generateVision()` | `llm/` | ✅ GA | — | Multi-Modal Inference via CLIP-VisionEncoder; LLaVA-Architektur |
| `VisionEncoder` | `llm/` | ✅ GA | — | CLIP-GGUF-Modell; `encodeImage()` → float-Embedding-Vektor |
| `LlamaWrapper` RoPE Scaling | `llm/` | ✅ GA | — | LINEAR/NTK/YARN/DYNAMIC; 4K→32K Kontext-Extension |
| `MultiLoRAManager::loadLoRA()` | `llm/` | ✅ GA | — | vLLM-Stil; dynamic load, INT8/INT4-Quantisierung via `quantizeLoRA()` |
| `MultiLoRAManager` Multi-GPU | `llm/` | ✅ GA | — | ROUND_ROBIN/DATA_PARALLEL/MODEL_PARALLEL; GPUDirect P2P |
| `ProductionValidator` | `llm/testing/` | ✅ GA | — | 72h-Stresstest, Lasttest, Performance-Regression |
| `IntegrationTestSuite` | `llm/testing/` | ✅ GA | — | 14 Szenarien (Komponenten-, Multi-Modell-, Fehler-, Performance-Tests) |
| `LlamaWrapper` Output-Validierung | `llm/` | ✅ GA | — | UTF-8-Prüfung, min/max-Länge, Kohärenz-Score |

### Prompt-Engineering-Modul (v2.0)

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `PromptManager` | `prompt_engineering/` | ✅ GA | — | RocksDB-Persistenz; CRUD; TBB lock-free Hash-Map; YAML Bulk-Load |
| `PromptManager::validateTemplate()` | `prompt_engineering/` | ✅ GA | — | Syntaxprüfung + Warnungen; in `createTemplate()` integriert |
| `PromptManager::buildMultiModalPrompt()` | `prompt_engineering/` | ✅ GA | — | Multi-Modal Prompts mit ImageDescription-Blöcken |
| `FeedbackCollector` | `prompt_engineering/` | ✅ GA | — | 10 Feedback-Typen; FNV-1a Audit-Checksum; Z-Score Outlier |
| Git-ähnliche Versionskontrolle | `prompt_engineering/` | ✅ GA | — | Branches, Commits, Diffs, Parent-Tracking |
| A/B Testing | `prompt_engineering/` | ✅ GA | — | Welch-t-Test Signifikanztests; `std::erfc`-basierter Z-Score |
| `SelfImprovementOrchestrator` | `prompt_engineering/` | ✅ GA | — | Auto-Optimierung bei Feedback-Verschlechterung; Hintergrund-Worker |
| `TreeOfThoughtsBuilder` | `prompt_engineering/` | ✅ GA | — | Multi-Pfad-Reasoning (Beam Search, max_depth, beam_width) |
| `ProTeGiOptimizer` | `prompt_engineering/` | ✅ GA | — | Textual-Gradient-Optimierung; population_size + iterations |
| DSPy-Modul (`dspy_module.h`) | `prompt_engineering/` | ✅ GA | — | DSPy-kompatibler Prompt-Deklarations-Layer |
| `ContextWindowManager` | `prompt_engineering/` | ✅ GA | — | Context-Window-Budget-Enforcement |

### Voice-Modul (v1.1)

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `VoiceAssistant` | `voice/` | ✅ GA | — | STT (Whisper) + LLM (LlamaWrapper) + TTS; Session-Management |
| Wake-Word-Detektion | `voice/` | ✅ GA | — | "Hey Themis"; konfig. Wake-Words; Low-Latency Detection |
| `VoiceBiometricAuthenticator` | `voice/` | ✅ GA | — | Sprecher-Enrollment + Verification; Confidence Score |
| `TelephonyBridge` (SIP/WebRTC) | `voice/` | ✅ GA | — | Anruf-Aufzeichnung + Transkription; Sprecher-Diarisierung |
| Browser WebSocket Streaming | `voice/` | ✅ GA | — | Real-Time Audio-Chunking; ws://server/voice/stream |
| Meeting-Protokoll-Generierung | `voice/` | ✅ GA | — | Zusammenfassung + Action-Items + Key-Points via LLM |
| `EmotionAnalyzer` | `voice/` | ✅ GA | — | Emotionserkennung aus Audio-Features |
| `VoiceMacro` | `voice/` | ✅ GA | — | Konfigurierbare Sprach-Makros + Trigger-Patterns |

### RAG-Modul v2 (v2.x)

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `RAGJudge` | `rag/` | ✅ GA | — | 5-dimensionale Evaluierung (Faith/Rel/Compl/Coh/Ethics); FAST/BALANCED/THOROUGH |
| `HybridRetriever` | `rag/` | ✅ GA | — | BM25+Vector RRF-Fusion (k=60); linear Fallback; Factory-Helfer |
| `DocumentSplitter` | `rag/` | ✅ GA | — | FIXED/SENTENCE/SEMANTIC/RECURSIVE; chunk_size + overlap |
| `BatchEvaluator` | `rag/` | ✅ GA | — | Parallele Massenevaluierung mit Worker-Threads + Futures |
| `EvaluationCache` | `rag/` | ✅ GA | — | Thread-sicherer LRU-Cache mit TTL; Invalidierungs-Triggers |
| `CalibrationManager` | `rag/` | ✅ GA | — | Temperature Scaling, Platt Scaling, Isotonic Regression; ECE/Brier |
| `HallucinationDashboard` | `rag/` | ✅ GA | — | Rolling-Window Halluzinationsrate + Trend (IMPROVING/STABLE/DEGRADING) |
| `StreamingRetriever` | `rag/` | ✅ GA | — | Inkrementelles Kontext-Window-Filling |
| `KnowledgeGraphRetriever` | `rag/` | ✅ GA | — | Entity-Linking + Graph-augmentierte Retrieval |
| `CrossEncoderReranker` | `rag/` | ✅ GA | — | Heuristischer Scorer + ONNX-Stub |
| `ContinuousLearningOrchestrator` | `rag/` | ✅ GA | — | Bayesian Optimierung über top_k/threshold via User-Feedback |
| `RlaifTrainer` | `rag/` | ✅ GA | — | Constitutional AI / RLAIF Training Pipeline |

### Graph-Modul (v1.x)

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `GraphQueryOptimizer` | `graph/` | ✅ GA | — | Cost-Based Algo-Selektion (BFS/DFS/BiDir/A*/Dijkstra); Schema-aware |
| Constrained Path Finding | `graph/` | ✅ GA | — | max_depth, required/forbidden vertices, timeout_ms |
| Adaptive Kostenmodell | `graph/` | ✅ GA | — | EMA-basiertes Lernen aus Ausführungsfeedback |
| Parallele Traversierung | `graph/` | ✅ GA | — | Parallele BFS-Frontier-Expansion; fan_out_threshold |
| `DistributedGraphManager` | `graph/` | ✅ GA | — | Shard-übergreifende Graph-Queries; EXPLAIN-Endpunkt |
| Inkrementelle Graph-Queries | `graph/` | ✅ GA | — | Live-Updates ohne Full-Recompute |
| Graph Query Result Streaming | `graph/` | ✅ GA | — | Cursor-basierte Streaming-Pagination für große Path-Sets |

### Chaos Engineering & Failover (v1.x)

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `FaultInjector` | `chaos/` | ✅ GA | — | NODE_FAILURE/NETWORK_PARTITION/LEADER_CRASH/DISK_FAILURE/RANDOM; zeitlich begrenzt |
| `ChaosScheduler` | `chaos/` | ✅ GA | — | Zeitgesteuerte Fault-Injections; Expiry-Handling |
| `DisasterRecoveryManager` | `failover/` | `✅ GA` | — | DR-Plan-Ausführung (7 Schritte); Step-Hooks; dry_run; Statistiken |
| Automatic Failover Queue | `failover/` | ✅ GA | — | Worker-Loop; Epoch-Fencing; Catchup-Wait |

### Sharding / Paxos

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| Paxos WAL `logAccept` | `sharding/` | ✅ GA | — | ACCEPT vor Quorum-Broadcast persistiert |
| Paxos WAL `logCommit` | `sharding/` | ✅ GA | — | COMMIT in `broadcastCommit()` persistiert |
| `recoverFromWAL()` | `sharding/` | ✅ GA | — | Snapshot + WAL-Replay; `commit_index_` wird restauriert |
| Snapshot-Compaction | `sharding/` | ✅ GA | — | `shouldCreateSnapshot(ops)` nach konfigurierbaren Operationen |

### Replikation (v1.6)

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `WALArchivalManager` | `replication/` | ✅ GA | — | Zstd + AES-256-GCM; S3/GCS/Azure via `IArchivalBackend` |
| Storage-Tier Lifecycle | `replication/` | ✅ GA | — | standard → cold → glacier; konfigurierbare Alters-Schwellen |
| `LogicalReplicationManager` | `replication/` | ✅ GA | — | Schema-aware Slots; Row-Filter, DDL-Streaming, Parallel Decoding |
| `LogicalReplicationManager` Cross-Version | `replication/` | ✅ GA | — | Per-Change-Transformer + Cross-Version-Transforms |
| `IArchivalBackend` | `replication/` | ✅ GA | — | Pluggable Backend-Interface für WAL-Segment-Storage |

### Governance-Modul (v1.x)

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `PolicyEngine::evaluate()` | `governance/` | ✅ GA | — | YAML-Policy, RBAC/ABAC, CCPA-Opt-Out, Hot-Reload |
| `PolicyEngine::simulateDecision()` | `governance/` | ✅ GA | — | Dry-Run ohne Audit-Eintrag; `matched_rule` + `matched_profile` |
| `PolicyEngine::checkQueryPermission()` | `governance/` | ✅ GA | — | Decision + `FieldMaskingPolicy` für DataMasker |
| `PolicyEngine::checkInferencePermission()` | `governance/` | ✅ GA | — | LLM-Endpunkt Zugangskontrolle; OpenAI-style Fehlerkörper |
| `OpaAdapter` | `governance/` | ✅ GA | — | OPA (Open Policy Agent) Integration; nativer Fallback |
| `DataMasker::maskFields()` | `governance/` | ✅ GA | — | REDACT/HASH/TRUNCATE/TOKENIZE/ENCRYPT; letzter In-Process-Schutz |
| `DataLineageTracker` | `governance/` | ✅ GA | — | Append-only Provenienz; Prometheus-Counter; Audit-Log |
| `ModelGovernancePolicy` | `governance/` | ✅ GA | — | KI/ML Training Data Lineage, Bias-Auditing, Export-Check |
| `CrossTenantPolicyInheritance` | `governance/` | ✅ GA | — | Hierarchische Policy-Vererbung über Tenants |
| `ComplianceReporter` | `governance/` | ✅ GA | — | GDPR/HIPAA/CCPA/PCI-DSS/SOC 2/ISO 27001 Reports (JSON + PDF) |

### Transaction-Modul (v1.9)

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `BranchManager` | `transaction/` | ✅ GA | — | Git-ähnliche Branches + 3-Way-Merge; from_tag/from_sequence |
| `SnapshotManager` | `transaction/` | ✅ GA | — | Named Snapshots/Tags; PITR-Basis |
| SAGA Distributed | `transaction/` | ✅ GA | — | Kompensationsaktionen für Relational/Index/Graph/Vector; Distributed Orchestration |
| `DeadlockPredictor` | `transaction/` | ✅ GA | — | Probability Scoring (0–1); Lock-Order-Empfehlung; Adaptive Timeout |
| `DistributedTransactionManager` (2PC) | `transaction/` | ✅ GA | — | WAL-backed Coordinator; Parallel Prepare/Commit; Crash Recovery |
| `GlobalTransactionManager` (TrueTime) | `transaction/` | ✅ GA | — | Multi-Region ACID; TrueTime-basiertes Commit-Timestamp |
| Transaction Savepoints | `transaction/` | ✅ GA | — | `createSavepoint`/`rollbackToSavepoint`/`releaseSavepoint` |
| OCC Mode | `transaction/` | ✅ GA | — | `getEntityVersion`/`optimisticPut`/`optimisticErase`; per-Entity Versions |
| Bulk Transaction API | `transaction/` | ✅ GA | — | `bulkPutEntities`/`bulkEraseEntities`; kein Per-Row-Overhead |
| Transaction Explain | `transaction/` | ✅ GA | — | `explain()`: Locks, Write-Set, MVCC-Version-Chain |

### Cache-Modul (v1.x)

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `AdaptiveQueryCache` (L1/L2/L3) | `cache/` | ✅ GA | — | BoundedLRU (L1) + zstd/lz4 (L2) + RocksDB (L3); Adaptive TTL |
| `SemanticCache` | `cache/` | ✅ GA | — | SHA-256 Fingerprint + Cosine Similarity; semantische Äquivalenz |
| GDPR-aware Invalidation | `cache/` | ✅ GA | — | `invalidatePII()` (Art. 17); UUID-basierte PII-Verknüpfung |
| Tenant-Isolation + Quotas | `cache/` | ✅ GA | — | `per_tenant_max_bytes`; cross-tenant Zugriff = nullopt |
| Circuit Breaker (L3) | `cache/` | ✅ GA | — | CLOSED/OPEN/HALF_OPEN; `cb_failure_threshold` konfigurierbar |
| Cache Warmup | `cache/` | ✅ GA | — | `warmupFromLog()`/`exportSnapshot()`; Bulk-Vorausladen |
| Rate Limiting | `cache/` | ✅ GA | — | Token Bucket; `max_requests_per_second` konfigurierbar |

### Observability-Modul (v1.x)

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `MetricsCollector` (Singleton) | `observability/` | ✅ GA | — | Prometheus Text-Format; 40+ Metriken (Query/Cache/Shard/Security/System) |
| `QueryProfiler` | `observability/` | ✅ GA | — | Per-Phase Timing (parse/optimize/execute); Index-Usage; Empfehlungen |
| `StorageProfiler` | `observability/` | ✅ GA | — | RocksDB-Stats; Write/Read-Amplification; Cache Hit Rates |
| `PerformanceAnalyzer` | `observability/` | ✅ GA | — | Automatische Issue-Erkennung; INFO/WARNING/CRITICAL; Empfehlungen |
| `AlertingEngine` (Alertmanager) | `observability/` | ✅ GA | — | Rule-based Alerting; Alertmanager Integration; PagerDuty/Slack |
| Distributed Tracing | `observability/` | ✅ GA | — | OpenTelemetry-kompatibel; Span-Kontext-Propagierung |
| Kubernetes Health Probes | `observability/` | ✅ GA | — | `/ready` + `/live` Endpunkte |

### Exporters-Modul (v1.x)

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `JsonlExporter` | `exporters/` | ✅ GA | — | JSONL; Instruction-Tuning: Alpaca/ShareGPT/ChatML/OpenAI |
| `ParquetExporter` | `exporters/` | ✅ GA | — | Parquet + konfigurierbare Arrow-Schema; Kompression |
| `ArrowIpcExporter` | `exporters/` | ✅ GA | — | Zero-Copy Apache Arrow IPC Pipeline |
| `HuggingFaceExporter` | `exporters/` | ✅ GA | — | HF Hub Direct Upload; dataset_info.json + README-Karte |
| `StreamingExporter` | `exporters/` | ✅ GA | — | Cursor-basiertes Streaming; Fortschritts-Callback + ETA |
| `IncrementalExporter` | `exporters/` | ✅ GA | — | Delta-Export mit Watermark + Checkpoint |
| AQL Predicate Filter | `exporters/` | ✅ GA | — | AQL-basiertes Filtern bei Export |
| AES-256-GCM Export Encryption | `exporters/` | ✅ GA | — | Verschlüsselter Export; PolicyEngine-Authorization |
| PII Detection + Redaction | `exporters/` | ✅ GA | — | Automatische PII-Erkennung + Redaktion vor Export |

## Implementierungsstatus — Process Module (v1.0)

### Serialisierer & Import

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `BpmnSerializer::importXml()` | `process/` | ✅ GA | BpmnSerializerTest | Zustandsbasierter XML-Tokenizer; 10 MiB-Guard; `bpmn:`-Namespace-Strip; tolerant |
| `BpmnSerializer::exportXml()` | `process/` | ✅ GA | BpmnSerializerTest | Volle BPMN 2.0-Fidelity; OMG-konforme Ausgabe |
| `BpmnSerializer::importFile()` | `process/` | ✅ GA | — | Datei-I/O-Wrapper |
| `BpmnSerializer::exportFromJson()` | `process/` | ✅ GA | — | Export aus normalisiertem JSON-Graph |
| `EpkSerializer::importText()` | `process/` | ✅ GA | EpkSerializerTest | EPK-Text-Notation + JSON-Array-Format |
| `EpkSerializer::exportText()` | `process/` | ✅ GA | EpkSerializerTest | Round-trip-fähig |
| `EpkSerializer::exportJson()` | `process/` | ✅ GA | EpkSerializerTest | Strukturiertes JSON für LLM-Kontext |
| `VccVpbImporter` | `process/` | ✅ GA | VccVpbImporterTest | VCC-VPB YAML-Import |

### ProcessModelManager

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `importBpmn()` | `process/` | ✅ GA | ProcessModuleTest | BPMN 2.0 → ProcessModelRecord; RocksDB `proc:def:<id>` |
| `importEpk()` | `process/` | ✅ GA | ProcessModuleTest | EPK → ProcessModelRecord |
| `importVccVpb()` | `process/` | ✅ GA | ProcessModuleTest | VCC-VPB → ProcessModelRecord |
| `save()`/`get()`/`remove()`/`list()` | `process/` | ✅ GA | ProcessModuleTest | Versioniertes CRUD |
| `deployToEngine()` | `process/` | ✅ GA | ProcessModuleTest | Registriert Modell mit ProcessGraphManager |
| `exportBpmn()`/`exportEpk()` | `process/` | ✅ GA | ProcessModuleTest | Rückexport in Originalformat |
| `generateLlmDescriptor()` | `process/` | ✅ GA | LlmDescriptorTest | JSON-Schema für LLM-System-Prompts |

### ProcessLinker

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `attachObject()` | `process/` | ✅ GA | ProcessModuleTest | 8 `ProcessLinkType`-Werte; node-scoped optional |
| `detachObject()` | `process/` | ✅ GA | ProcessModuleTest | Hard-Delete (kein Tombstone) |
| `getAttachments()` / `getNodeAttachments()` | `process/` | ✅ GA | ProcessModuleTest | Mit optionalem Typ-Filter |
| `findInstancesWithObject()` | `process/` | ✅ GA | ProcessModuleTest | Sekundärindex `proc:obj_idx:` |
| `linkProcesses()` / `getLinks()` | `process/` | ✅ GA | ProcessModuleTest | Typisierte Prozess-zu-Prozess-Links |
| `registerRequiredDocument()` | `process/` | ✅ GA | ProcessModuleTest | Pflichtdokumentregistrierung pro Modell-Knoten |
| `getMissingDocuments()` | `process/` | ✅ GA | ProcessModuleTest | Prüft welche Pflichtdokumente fehlen |

### ProcessGraphRag

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `retrieve()` | `process/` | ✅ GA | ProcessGraphRagTest | Vollständiger Graph-RAG-Kontext; DE/EN |
| `retrieveForNode()` | `process/` | ✅ GA | ProcessGraphRagTest | Knoten-scoped RAG |
| `summarizeVerwaltungsvorgang()` | `process/` | ✅ GA | ProcessGraphRagTest | JSON-Zusammenfassung für UI/API |
| `checkCompliance()` | `process/` | ✅ GA | ProcessGraphRagTest | Compliance-Check (Dokumente, SLA, Status) |
| `findSimilarCases()` | `process/` | ✅ GA | ProcessGraphRagTest | Cosine/Jaccard-Ähnlichkeit; konfigurierbarer Threshold |
| `buildAdminProcessingPrompt()` | `process/` | ✅ GA | ProcessGraphRagTest | Verwaltungsspezifischer LLM-Prompt (DE) |

### LlmProcessDescriptor

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `generateDescriptor()` | `process/` | ✅ GA | LlmDescriptorTest | JSON-Schema mit nodes/edges/compliance/sla |
| `generateSystemPrompt()` | `process/` | ✅ GA | LlmDescriptorTest | System-Prompt DE/EN |
| `buildConformancePrompt()` | `process/` | ✅ GA | LlmDescriptorTest | Conformance-Checking-Prompt |

---

**Version History:**
- 1.4.0-alpha (2026-01-06): 25 neue Alpha-Features (LLM, Performance, HA, Monitoring, Protocol)
- 1.3.0 (2025-12-30): Umfassendes Update mit allen Features bis Dez 2025

## Implementierungsstatus — Training, CDC & Updates (2026-04-12)

### Training-Modul (v1.5)

| Feature | Header | Status | Beschreibung |
|---------|--------|--------|-------------|
| `TrainingPipeline::run()` | `training/training_pipeline.h` | ✅ GA | 6-stufige End-to-End LoRA-Pipeline aus ThemisDB-Collections |
| `IncrementalLoRATrainer::train()` | `training/incremental_lora_trainer.h` | ✅ GA | INITIAL/RESUME/CONTINUE-Modes, INT8/INT4 Quantisierung |
| `IncrementalLoRATrainer::resumeFromCheckpoint()` | `training/incremental_lora_trainer.h` | ✅ GA | SHA-256-verifiziertes Laden, Auto-Rollback |
| `AutoLabeler::label()` | `training/auto_labeler.h` | ✅ GA | LLM-Annotation + Keyword-Extraktion, Konfidenz-Filter |
| `KnowledgeGraphEnricher` | `training/knowledge_graph_enricher.h` | ✅ GA | KG-Kontext je Sample aus ThemisDB-Graph |
| `LoraDataSelection` | `training/lora_data_selection.h` | ✅ GA | Aktives Lernen / Uncertainty Sampling |
| `LoRACheckpointManager::save()/resume()` | `training/lora_checkpoint_manager.h` | ✅ GA | Atomares Speichern, SHA-256, Auto-Rollback, JSON-Manifest |
| `ProvenanceTracker::record()` | `training/provenance_tracker.h` | ✅ GA | Lineage-Graph für Gewichts-Änderungen |
| `ModalityParser` | `training/modality_parser.h` | ✅ GA | Multimodale Input-Normalisierung (Text/Bild/Audio) |
| `LoRAAdapterMerger` | `training/lora_adapter_merger.h` | ✅ GA | Mergen mehrerer LoRA-Adapter (gewichtete Linearkombination) |

### CDC-Modul (v1.x)

| Feature | Header | Status | Beschreibung |
|---------|--------|--------|-------------|
| `Changefeed::listEvents()` | `cdc/changefeed.h` | ✅ GA | Sequenz-basiert, Key-Präfix-Filter, Long-Polling |
| `Changefeed::subscribe()` | `cdc/changefeed.h` | ✅ GA | Push-Subscription mit SubscriptionFilter + cancel() |
| `Changefeed::compactByKey()` | `cdc/changefeed.h` | ✅ GA | Manuelle Kompaktierung, removed_count + bytes_freed |
| `Changefeed::redactByKeyPrefix()` | `cdc/changefeed.h` | ✅ GA | GDPR-Redaktion im Event-Log |
| `Changefeed::watermarks()` | `cdc/changefeed.h` | ✅ GA | min_seq, max_seq, consumer_lag |
| `CdcMaterializedView` | `cdc/cdc_materialized_view.h` | ✅ GA | Live-Sync MV via Changefeed |
| `CdcAdmin` | `cdc/cdc_admin.h` | ✅ GA | Feed-Verwaltung, Reset, Export |
| `ConsumerGroup` | `cdc/consumer_group.h` | ✅ GA | Multi-Consumer-Offset-Management |
| `CrossCollectionStream` | `cdc/cross_collection_stream.h` | ✅ GA | Joins über mehrere Changefeeds |

### Updates-Modul (v1.x)

| Feature | Header | Status | Beschreibung |
|---------|--------|--------|-------------|
| `ClusterUpdateManager::update()` | `updates/cluster_update_manager.h` | ✅ GA | Rolling-Update mit DRAIN/APPLY/HEALTHCHECK/ROLLBACK |
| `CanaryRollout::deploy()/promote()/rollback()` | `updates/canary_rollout.h` | ✅ GA | Prozentbasierter Canary, automatischer Rollback bei Error-Rate |
| `HotReloadEngine::reloadConfig()/reloadPlugin()` | `updates/hot_reload_engine.h` | ✅ GA | Zero-Downtime Konfigurations- und Plugin-Reload |
| `InPlaceSchemaMigrator::execute()` | `updates/in_place_schema_migrator.h` | ✅ GA | Online-Schema-Migration ohne Downtime |
| `BlueGreenDeployment::cutover()/rollback()` | `updates/blue_green_deployment.h` | ✅ GA | Instant Traffic-Switch Blue↔Green |
| `DeltaUpdateEngine` | `updates/delta_update_engine.h` | ✅ GA | Inkrementelle Patch-Anwendung auf Datenbankebene |
| `DependencyResolver` | `updates/dependency_resolver.h` | ✅ GA | Update-Reihenfolge basierend auf Abhängigkeitsgraph |
| `ManifestDatabase` | `updates/manifest_database.h` | ✅ GA | Versionierter Software-Manifest-Store |
| `CoordinatedUpdateManager` | `updates/coordinated_update_manager.h` | ✅ GA | Multi-Cluster koordinierte Updates mit Quorum-Fencing |

## Implementierungsstatus — Auth, BiTemporal, Storage, LLM-Infrastruktur (2026-04-13)

### Auth-Modul C++ API (v1.x)

| Feature | Header | Status | Beschreibung |
|---------|--------|--------|-------------|
| `JWTValidator::validate()` | `auth/jwt_validator.h` | ✅ GA | RS256/ES256/EdDSA, Blacklist, KID-Revokation |
| `OAuthPKCEFlow::exchangeCode()` | `auth/oauth_pkce_flow.h` | ✅ GA | RFC 7636 PKCE, Authorization-Code-Flow |
| `SAMLAuthenticator::validateResponse()` | `auth/saml_authenticator.h` | ✅ GA | SAML 2.0 SP, Attribut-Mapping, IdP-Zertifikat |
| `WebAuthnAuthenticator::finishRegistration/Authentication()` | `auth/webauthn_authenticator.h` | ✅ GA | FIDO2/WebAuthn, PassKey-Support |
| `LDAPAuthenticator::authenticate()` | `auth/ldap_authenticator.h` | ✅ GA | LDAPS, Connection-Pool, Group-Lookup |
| `MFAAuthenticator::validateTOTP()` | `auth/mfa_authenticator.h` | ✅ GA | TOTP RFC 6238, Recovery Codes, Enrollment |
| `SessionManager::validateAndRefresh()` | `auth/session_manager.h` | ✅ GA | Idle/Absolute-Timeout, Max-Sessions-Per-User |
| `PasswordPolicy::validate()` | `auth/password_policy.h` | ✅ GA | Entropie-Scoring, HaveIBeenPwned-Check |
| `FederatedIdentityManager::validateToken()` | `auth/federated_identity_manager.h` | ✅ GA | OIDC Multi-Realm, Token-Exchange |
| `JwksValidator` | `auth/jwks_validator.h` | ✅ GA | JWKS-Endpoint, Auto-Refresh |
| `JwtKeyRotationManager` | `auth/jwt_key_rotation_manager.h` | ✅ GA | Key-Rotation ohne Downtime |
| `OidcProvider` | `auth/oidc_provider.h` | ✅ GA | OIDC Discovery, UserInfo-Endpoint |
| `ApiKeyAuthenticator` | `auth/api_key_authenticator.h` | ✅ GA | Scoped API-Keys, Constant-Time-Vergleich |
| `AuthRateLimiter` | `auth/auth_rate_limiter.h` | ✅ GA | Sliding-Window, Per-User/Per-IP |
| `MtlsAuthenticator` | `auth/mtls_authenticator.h` | ✅ GA | Client-Zertifikat-Authentifizierung |

### Bi-Temporal-Modul (v1.x)

| Feature | Header | Status | Beschreibung |
|---------|--------|--------|-------------|
| `BiTemporalTable::insertWithValidTime()` | `temporal/bi_temporal.h` | ✅ GA | SQL:2011 Bi-Temporale Einfügung |
| `BiTemporalTable::queryAsOf()` | `temporal/bi_temporal.h` | ✅ GA | AS OF Abfrage (System-Zeit + Gültigkeits-Zeit) |
| `BiTemporalTable::detectGaps()` | `temporal/bi_temporal.h` | ✅ GA | Lücken-Erkennung im Gültigkeits-Zeitraum |
| `TemporalForeignKey::validate()` | `temporal/bi_temporal.h` | ✅ GA | Referentielle Integrität über Zeiträume |
| `BiTemporalJoin` | `temporal/bitemporal_join.h` | ✅ GA | SEQUENCED/NON_SEQUENCED/CURRENT Join |
| `TemporalQueryEngine::execute()` | `temporal/temporal_query_engine.h` | ✅ GA | AS OF / FROM-TO / CONTAINED IN Abfragen |
| `TemporalSnapshotManager` | `temporal/snapshot_manager.h` | ✅ GA | Snapshot-Handle für PITR |
| `SystemVersionedTable` | `temporal/system_versioned_table.h` | ✅ GA | Automatische System-Zeit-Verwaltung |
| `IntervalTreeIndex` | `temporal/interval_tree_index.h` | ✅ GA | Effiziente Overlap-Suche |
| `TemporalAggregator` | `temporal/temporal_aggregator.h` | ✅ GA | Zeitraum-aggregierte Aggregate (SUM/AVG/COUNT) |

### Storage/Backup-Modul C++ API (v1.x)

| Feature | Header | Status | Beschreibung |
|---------|--------|--------|-------------|
| `BackupManager::createFullBackup()` | `storage/backup_manager.h` | ✅ GA | Full Backup, AES-256-GCM, RAID-aware |
| `BackupManager::createIncrementalBackup()` | `storage/backup_manager.h` | ✅ GA | Inkrementell seit letztem Backup |
| `BackupManager::createDifferentialBackup()` | `storage/backup_manager.h` | ✅ GA | Differenziell seit letztem Full-Backup |
| `BackupManager::restoreFromBackup()` | `storage/backup_manager.h` | ✅ GA | RAID-Shard-Rekonstruktion |
| `BackupManager::archiveWAL()` | `storage/backup_manager.h` | ✅ GA | WAL-Archivierung für PITR |
| `PITRManager::restore()` | `storage/pitr_manager.h` | ✅ GA | Point-in-Time Recovery, Fortschritts-Callback |
| `PITRManager::previewRestore()` | `storage/pitr_manager.h` | ✅ GA | Dry-Run ohne Änderungen |
| `TieredStorageManager::put()/get()` | `storage/tiered_storage.h` | ✅ GA | HOT/WARM/COLD transparent, Auto-Promotion |
| `AdaptiveCompactionManager` | `storage/adaptive_compaction.h` | ✅ GA | Write/Read-Amplification-basierte Strategie |
| `BlobStorageManager` | `storage/blob_storage_manager.h` | ✅ GA | Multi-Backend (FS/GCS/S3), Zero-Copy |
| `ErasureCodingBackend` | `storage/erasure_coding_backend.h` | ✅ GA | Reed-Solomon, konfigurierbare Parität |

### LLM-Infrastruktur (v1.x)

| Feature | Header | Status | Beschreibung |
|---------|--------|--------|-------------|
| `PagedKVCache::store()/sharePrefix()` | `llm/paged_kv_cache.h` | ✅ GA | vLLM Paged Attention, Prefix-Sharing |
| `ContinuousBatchScheduler::step()` | `llm/continuous_batch_scheduler.h` | ✅ GA | Continuous Batching, Preemption, Priority |
| `SpeculativeDecoder::decode()` | `llm/speculative_decoder.h` | ✅ GA | Draft-Model, 5-Token-Lookahead, Acceptance-Threshold |
| `OpenAICompatAdapter` | `llm/openai_compat_adapter.h` | ✅ GA | /v1/chat/completions + Streaming |
| `LoRARouter::route()` | `llm/lora_router.h` | ✅ GA | A/B-Testing, Canary-Rollout, Fallback |
| `AdapterRegistry::registerAdapter()` | `llm/adapter_registry.h` | ✅ GA | Versionierter Adapter-Store, Provenance |
| `ModelRouter::route()` | `llm/model_router.h` | ✅ GA | Rule-based Routing (Kontext/Tenant/Custom) |
| `AdapterLoadBalancer` | `llm/adapter_load_balancer.h` | ✅ GA | GPU-Placement, JIT-Eviction, Migration |
| `AdapterDeploymentManager` | `llm/adapter_deployment_manager.h` | ✅ GA | Blue/Green, Canary für Adapter |
| `LlamaResourceManager` | `llm/llama_resource_manager.h` | ✅ GA | llama.cpp Ressourcen-Lifecycle |
| `GpuMemoryManager` | `llm/gpu_memory_manager.h` | ✅ GA | VRAM-Budget, OOM-Safe-Fail |
| `EmbeddedLLM` | `llm/embedded_llm.h` | ✅ GA | In-Process LLM ohne Server |

## Implementierungsstatus — RAG Advanced, Sharding C++ API, Query/Index Engine (2026-04-13)

### RAG-Modul v2 — Erweiterte Pipeline-Komponenten

| Feature | Header | Status | Beschreibung |
|---------|--------|--------|-------------|
| `AgenticRAG::run()` | `rag/agentic_rag.h` | ✅ GA | Iterativer Retrieve-then-Reason Agent (max_iterations, quality_threshold, cancel) |
| `AgenticRAGFactory` | `rag/agentic_rag.h` | ✅ GA | Factory für Standard- und Custom-Konfigurationen |
| `MultiStepRAGOrchestrator::run()` | `rag/multi_step_rag.h` | ✅ GA | Decompose-then-Retrieve (max_steps, LLM-Decomposition, MergeStrategy) |
| `MultiModalRAG::query()` | `rag/multimodal_rag.h` | ✅ GA | TEXT/IMAGE/TABLE/CODE/AUDIO; OCR-Support |
| `RAGContextAssembler::assemble()` | `rag/rag_context_assembler.h` | ✅ GA | Token-Budget-Management, Dedup, Ordering |
| `DistributedRAGEvaluator::evaluate()` | `rag/distributed_rag_evaluator.h` | ✅ GA | Parallele Multi-Judge-Evaluierung, Quorum, AggregationStrategy |
| `DistributedEvaluatorFactory` | `rag/distributed_rag_evaluator.h` | ✅ GA | Builder für Distributed-Judge-Cluster |

### Sharding-Modul C++ API (v1.x)

| Feature | Header | Status | Beschreibung |
|---------|--------|--------|-------------|
| `AdaptiveShardRouter::route()` | `sharding/adaptive_shard_router.h` | ✅ GA | Lernender Router mit Exploration/Exploitation, Feedback-Loop |
| `ConsistentHashRing::getShardForKey()` | `sharding/consistent_hash.h` | ✅ GA | Virtueller Knoten-Ring, 150 vNodes/Shard, getNShardsForKey |
| `CrossShardTransaction::commit()` | `sharding/cross_shard_transaction.h` | ✅ GA | 2PC + WAL, IsolationLevel SERIALIZABLE/SNAPSHOT/READ_COMMITTED |
| `EpochFencingManager::checkFence()` | `sharding/epoch_fencing.h` | ✅ GA | Stale-Leader-Schutz, Lease-Management, advanceEpoch |
| `HotShardSplitPolicy` | `sharding/auto_rebalancer.h` | ✅ GA | ML-prädiktive Shard-Aufteilung, QPS-Threshold |
| `DataMigrator::migrate()` | `sharding/data_migrator.h` | ✅ GA | Live-Migration mit Dual-Write, Verifikation, Rate-Limiting |
| `AutoRecoveryManager` | `sharding/auto_recovery_manager.h` | ✅ GA | Automatische Shard-Recovery nach Ausfall |
| `GossipConfigManager` | `sharding/gossip_config_manager.h` | ✅ GA | Gossip-basierte Konfigurationsverteilung |

### Query Engine C++ API (v1.x)

| Feature | Header | Status | Beschreibung |
|---------|--------|--------|-------------|
| `AdaptiveQueryStats::recordExecution()` | `query/adaptive_optimizer.h` | ✅ GA | Query-Statistiken akkumulieren, Kardinalitätsfehler erkennen |
| `AdaptivePlanSelector::selectPlan()` | `query/adaptive_optimizer.h` | ✅ GA | KEEP/SWITCH/PARALLEL_TEST basierend auf Feedback |
| `DistributedQueryCostModel` | `query/adaptive_optimizer.h` | ✅ GA | Cross-Shard-Join-Kosten, Partition Pruning |
| `MultiIndexOptimizer` | `query/adaptive_optimizer.h` | ✅ GA | Optimale Index-Kombination für komplexe Prädikate |
| `AdaptiveJoin::executeJoin()` | `query/adaptive_join.h` | ✅ GA | HASH/SORT_MERGE/INDEX/NESTED_LOOP/GRACE_HASH, Runtime-Wahl |
| `ApproximateCountDistinct` (HyperLogLog) | `query/approximate_aggregator.h` | ✅ GA | ±2 % Fehler, merge für verteilte Aggregation |
| `ApproximatePercentile` (T-Digest) | `query/approximate_aggregator.h` | ✅ GA | p50/p95/p99, merge-fähig |

### Index-Modul C++ API (v1.x)

| Feature | Header | Status | Beschreibung |
|---------|--------|--------|-------------|
| `IndexManager` | `index/index_manager.h` | ✅ GA | Koordiniert Vector/Secondary/Graph-IndexManager |
| `InvertedIndex::search()` | `index/inverted_index.h` | ✅ GA | BM25, Stemming, Stopwords, RocksDB-backed |
| `AdvancedVectorIndex` | `index/advanced_vector_index.h` | ✅ GA | AUTO-Type (HNSW/IVF/SQ/PQ), WorkloadType-aware |
| `DistributedVectorIndex` | `index/distributed_vector_index.h` | ✅ GA | Scatter-Gather K-NN, ConsistentHash, Replikation |
| `BinaryQuantizer` | `index/binary_quantizer.h` | ✅ GA | 1-bit Quantisierung, FAISS-Integration |
| `IndexSuggestionEngine::recommend()` | `index/adaptive_index.h` | ✅ GA | Auto-Empfehlung basierend auf Query-Patterns + Selektivität |
| `QueryPatternTracker` | `index/adaptive_index.h` | ✅ GA | Scan-Latenz, Rows-scanned, Cache-Miss aufzeichnen |
| `SelectivityAnalyzer` | `index/adaptive_index.h` | ✅ GA | Distinct-Values, Histogramm, Selektivitäts-Score |

## Implementierungsstatus — Server/API, Metadata, Performance, Plugins, UserStorage (2026-04-13 session 4)

### Server/API-Modul (v1.x)

| Feature | Header | Status | Beschreibung |
|---------|--------|--------|-------------|
| `APIGateway::registerHandler()` | `server/api_gateway.h` | ✅ GA | Einheitlicher HTTP-Einstiegspunkt; Route-Registrierung, Load-Shedding, Deprecation-Header |
| `AdaptiveRateLimiter::allowRequest()` | `server/adaptive_rate_limiter.h` | ✅ GA | Feedback-basiertes Rate-Limiting, Burst-Toleranz, Overload-Threshold |
| `AuthMiddleware::authenticate()` | `server/auth_middleware.h` | ✅ GA | JWT/GSSAPI/API-Key/USB-Token; Scope-Prüfung |
| `DistributedGateway` / `ConsistentHashRing` | `server/distributed_gateway.h` | ✅ GA | Cluster-Routing, Circuit-Breaker, Failover, 200 vNodes |
| `AsyncJobApiHandler` / `AsyncJobRegistry` | `server/async_job_api_handler.h` | ✅ GA | Async-Job-Queue (PENDING/RUNNING/COMPLETED/FAILED), prune() |
| `GrpcApiServer::start()` | `api/grpc_server.h` | ✅ GA | gRPC Port 8771, TLS, max-message-size, registerService |
| `GraphQLSchemaBuilder::build()` | `api/graphql_schema_builder.h` | ✅ GA | Type/Query/Mutation-Registrierung, Schema-Validierung |
| `IWebSocketFrameCallback` | `api/websocket_handler.h` | ✅ GA | WebSocket-Frame-Callbacks (Text/Binary/Ping/Close), CloseCode |

### Metadata/Schema-Modul (v1.x)

| Feature | Header | Status | Beschreibung |
|---------|--------|--------|-------------|
| `SchemaManager::createTable()` | `metadata/schema_manager.h` | ✅ GA | Tabellen/Relationships/Indexes; AdaptiveTTL-Konfiguration |
| `InformationSchema::getTables()` | `metadata/information_schema.h` | ✅ GA | SQL-kompatible IS: tables/columns/statistics/referential_constraints |
| `SchemaVersionManager::persistChange()` | `metadata/schema_version_manager.h` | ✅ GA | WAL-backed Migrations; rollback; AuditLog-Integration |
| `DistributedMetadataCatalog::publishSchema()` | `metadata/distributed_catalog.h` | ✅ GA | Cluster-weite Schema-Verteilung via Consensus |
| `SchemaConsistencyChecker::check()` | `metadata/schema_consistency_checker.h` | ✅ GA | Schema↔Daten-Konsistenz; Auto-Repair; dry_run |
| `IndexRecommender` | `metadata/index_recommender.h` | ✅ GA | Empfiehlt Indexes basierend auf Query-Statistiken |
| `SchemaAuditLog` | `metadata/schema_audit_log.h` | ✅ GA | Append-only Schema-Änderungsprotokoll |
| `ERDiagramExporter` | `metadata/er_diagram_exporter.h` | ✅ GA | ER-Diagramm-Export (Mermaid/PlantUML) |

### Performance-Internals (v1.x)

| Feature | Header | Status | Beschreibung |
|---------|--------|--------|-------------|
| `AdaptiveQueryCompiler::compile()` | `performance/adaptive_query_compiler.h` | ✅ GA | JIT-ähnliche Query-Kompilierung; Hot-Query-Cache |
| `IntelligentPrefetcher::predict_next()` | `performance/intelligent_prefetcher.h` | ✅ GA | ML-basiertes Prefetching (SEQUENTIAL/STRIDED/RANDOM/POINTER_CHASE); Hardware-Prefetch |
| `WorkloadPredictor::forecast()` | `performance/workload_predictor.h` | ✅ GA | LSTM/EMA Lastvorhersage; ScaleDirection (UP/DOWN/STABLE); Replikationsempfehlung |
| `HardwareAccelerator::execute()` | `performance/hardware_accelerator.h` | ✅ GA | CPU/CUDA/OpenCL/NPU/FPGA Dispatch; OperatorType-Auswahl |
| `LockFreeRingBuffer<T>` | `performance/lockfree_metrics_buffer.h` | ✅ GA | SPSC Lock-Free FIFO; tryPush/tryPop; dropped_count |
| `WorkloadPredictor::getScaleRecommendation()` | `performance/workload_predictor.h` | ✅ GA | Konkrete Replikations-Empfehlung mit Konfidenz |

### Plugin-System (v1.x)

| Feature | Header | Status | Beschreibung |
|---------|--------|--------|-------------|
| `PluginManager::loadPlugin()` | `plugins/plugin_manager.h` | ✅ GA | Signatur-Verifikation, Hot-Reload, PluginReloadPhase-Callbacks |
| `WasmHostAPI` | `plugins/wasm_host_api.h` | ✅ GA | WASM-Plugins (Wasmtime/Wasmer/Wasm3); Host-Function-ABI |
| `PluginHealthMonitor::start()` | `plugins/plugin_health_monitor.h` | ✅ GA | Auto-Disable + Recovery; MonitoringEvent-Callbacks; Health-Score |
| `SignedPluginRepository` | `plugins/signed_plugin_repository.h` | ✅ GA | OCI-Registry-Integration für signierte Plugin-Pakete |
| `SelfHealingPlugin` | `plugins/self_healing_plugin.h` | ✅ GA | Automatische Neustart-Logik bei Crash |
| `PluginDependencyResolver` | `plugins/plugin_dependency_resolver.h` | ✅ GA | Topologische Sortierung + Zyklenerkennung für Plugin-Deps |

### User Storage Encrypted (v1.x)

| Feature | Header | Status | Beschreibung |
|---------|--------|--------|-------------|
| `MultiLevelEncryptedStorage::mount()` | `user_storage_encrypted/multi_level_storage.hpp` | ✅ GA | FUSE-basierter Multi-Level-Verschlüsselungsspeicher; SecurityLevel LOW/MEDIUM/HIGH |
| `KeyRotationScheduler::scheduleRotation()` | `user_storage_encrypted/key_rotation_scheduler.hpp` | ✅ GA | Geplante + manuelle Schlüsselrotation; IRotationStore-Persistenz |
| `GocryptfsBackend::mount()` | `user_storage_encrypted/gocryptfs_backend.hpp` | ✅ GA | Gocryptfs FUSE-Backend; mount/unmount/isMounted |
| `Argon2idKeyDerivationService::deriveKey()` | `user_storage_encrypted/key_derivation_service.hpp` | ✅ GA | Argon2id KDF (OWASP-Empfehlung); memory/iterations/parallelism |
| `reconcileStaleMounts()` | `user_storage_encrypted/multi_level_storage.hpp` | ✅ GA | Aufräumen verwaister FUSE-Mounts beim Systemstart |

## Implementierungsstatus — Geo Extended, Timeseries Internals, AQL Advanced, Scheduler (2026-04-13 session 5)

### Geo-Modul Erweiterungen (v2.x)

| Feature | Header | Status | Beschreibung |
|---------|--------|--------|-------------|
| `GeoRTree::bulkLoad()` / `queryBBox()` / `queryKNN()` | `geo/geo_rtree.h` | ✅ GA | R*-Tree Spatial Index; Bulk-Load, BBox-Query, K-NN |
| `SpatialJoinIterator::advance()` | `geo/spatial_join.h` | ✅ GA | Lazy Iterator; INTERSECTS/WITHIN_DISTANCE/CONTAINS/OVERLAPS |
| `TileServer::getTile()` | `geo/tile_server.h` | ✅ GA | Mapbox Vector Tiles (MVT/Protobuf); Layer-Config; Geometrie-Vereinfachung |
| `geoDbscan()` / `geoKMeans()` | `geo/geo_clustering.h` | ✅ GA | DBSCAN (eps_m, min_points) + K-Means (k, max_iter); Noise-Points |
| `RasterGrid::sample()` / `generateHeatmap()` | `geo/raster.h` | ✅ GA | Bilinear-Sampling, Gaussian-Heatmap, normalize |

### Timeseries-Internals (v1.9.x)

| Feature | Header | Status | Beschreibung |
|---------|--------|--------|-------------|
| `TimeSeriesStore::put()` / `query()` / `aggregate()` | `timeseries/timeseries.h` | ✅ GA | Core-KV-Store; Range-Query + Window-Aggregation |
| `Hypertable::insertBatch()` / `compressChunk()` | `timeseries/hypertable.h` | ✅ GA | Auto-Partitionierung nach Zeit; Gorilla+Zstd Chunk-Kompression |
| `ContinuousAggWatermarkStore` / `DistributedAggregateCoordinator` | `timeseries/continuous_agg.h` | ✅ GA | Inkrementelle Materialisierung; Rollup-Hierarchien; Watermark |
| `BitWriter` / `BitReader` (Gorilla) | `timeseries/gorilla.h` | ✅ GA | Delta-of-Delta Timestamp + XOR Value Encoding |
| `DownsamplingPipeline::addPolicy()` | `timeseries/downsampling.h` | ✅ GA | Mehrstufiges Downsampling; AggFunc-Auswahl; Retention |
| `RetentionManager::startAsync()` | `timeseries/retention.h` | ✅ GA | Soft+Hard Delete; StagedDeletion; Audit-Trail |
| `TsStreamCursor::nextBatch()` | `timeseries/ts_stream_cursor.h` | ✅ GA | Batch-Streaming-Iterator über Zeitreihen |
| `PrometheusRemoteWriteHandler::handle()` | `timeseries/prometheus_remote_write.h` | ✅ GA | Prometheus Remote-Write-Endpoint; Grafana-kompatibles Remote-Read |

### AQL Advanced API (v1.x)

| Feature | Header | Status | Beschreibung |
|---------|--------|--------|-------------|
| `AQLQueryBuilder::from()` / `build()` | `aql/aql_query_builder.h` | ✅ GA | Fluent Builder; FROM/FILTER/SORT/LIMIT/JOIN/SELECT |
| `AQLQueryValidator::validate()` | `aql/aql_query_validator.h` | ✅ GA | Syntaktisch + semantisch; ERROR/WARNING/INFO; Line/Column |
| `AQLOptimizerAdvisor::analyze()` | `aql/aql_optimizer_advisor.h` | ✅ GA | Index-Empfehlungen; Rewrite-Vorschläge; estimated_speedup |
| `AQLConversationContext::ask()` | `aql/aql_conversation_context.h` | ✅ GA | Multi-Turn NL→AQL; Schema-Kontext; Auto-Correct |
| `AQLMigrationAssistant::migrate()` | `aql/aql_migration_assistant.h` | ✅ GA | SQL→AQL Migrationshilfe; BLOCKING/WARNING Issues |
| `AQLAgent::run()` | `aql/aql_agent.h` | ✅ GA | LLM-gesteuerter DB-Agent; Tool-Registrierung; Reasoning-Steps |

### Scheduler-Internals (v1.x)

| Feature | Header | Status | Beschreibung |
|---------|--------|--------|-------------|
| `TaskScheduler::addTask()` | `scheduler/task_scheduler.h` | ✅ GA | CRON/INTERVAL/CDC/MANUAL/WEBHOOK; 5/6-Feld, Timezone, CDC-Trigger |
| `DistributedTaskCoordinator::start()` | `scheduler/distributed_task_coordinator.h` | ✅ GA | Leader-Election; One-Runner-Per-Cluster; LeaderChange-Callback |
| `TaskAnomalyDetector::analyzeTask()` | `scheduler/task_anomaly_detector.h` | ✅ GA | Frequenz/Pattern/Resource/Failure-Rate-Erkennung; Callback |
| `TaskAuditManager::query()` / `exportTo()` | `scheduler/task_audit_manager.h` | ✅ GA | Vollständiger Audit-Trail; JSON/CSV/Parquet-Export; 90-Tage-Retention |
| `HybridRetentionManager::startAsync()` | `scheduler/hybrid_retention_manager.h` | ✅ GA | 3-Stufen: Gorilla (0–7d) + Downsampling (7–365d) + Tages-Agg (>365d) |

## Implementierungsstatus — LLM Advanced AI, Replication, Content Module, Acceleration (2026-04-13 session 6)

### LLM Advanced AI (v2.x)

| Feature | Header | Status | Beschreibung |
|---------|--------|--------|-------------|
| `AiOrchestrator::run()` | `llm/ai_orchestrator.h` | ✅ GA | Multi-Mode Pipeline: RAG/AGENTIC/CRITIQUE; ModeSpec, BudgetSpec, SafetySpec, JudgeSpec |
| `AsyncInferenceEngine::submitAsync()` | `llm/async_inference_engine.h` | ✅ GA | Streaming Token-Callback; DROP_OLDEST/BLOCK Backpressure; Dedup-Cache |
| `InferenceEngineEnhanced` | `llm/inference_engine_enhanced.h` | ✅ GA | Multi-Model LB (ROUND_ROBIN/LEAST_LOADED/FASTEST); Circuit Breaker; ModelResourceQuota |
| `InlineTrainingEngine::stepAsync()` | `llm/inline_training_engine.h` | ✅ GA | On-the-Fly LoRA Fine-Tuning; AdamW/LION Optimizer; Cosine LR Scheduler |
| `ConstitutionalReasoningEngine::reason()` | `llm/constitutional_reasoning_engine.h` | ✅ GA | Prinzipienbasiert; max_revision_rounds; hard_block; HHH-Prinzipien |
| `EthicsAwareConfidenceDetector::evaluate()` | `llm/ethics_aware_confidence_detector.h` | ✅ GA | Bias-Kategorien; Token-Konfidenz; Ethics + Uncertainty Combined Score |

### Replication Multi-Master & CRDT (v1.x)

| Feature | Header | Status | Beschreibung |
|---------|--------|--------|-------------|
| `ReplicationManager::addReplica()` | `replication/replication_manager.h` | ✅ GA | PRIMARY/SECONDARY/ARBITER; SYNC/ASYNC/SEMI_SYNC; Lag-Monitoring |
| `VectorClock::happensBefore()` / `isConcurrent()` | `replication/multi_master_replication.h` | ✅ GA | Kausalordnung; Nebenläufigkeitserkennung; merge() |
| `HybridLogicalClock::now()` | `replication/multi_master_replication.h` | ✅ GA | Wall + Logical Timestamp; TrueTime-ähnlich |
| `ConflictResolver::resolve()` | `replication/multi_master_replication.h` | ✅ GA | LWW/FWW/MERGE/CUSTOM; CONCURRENT_WRITE/DELETE_UPDATE/SCHEMA_CONFLICT |
| `GrowOnlyCounter` / `PNCounter` / `MVRegister` (CRDT) | `replication/crdt_types.h` | ✅ GA | Kommutative CRDT-Merge; G-Counter, PN-Counter, Multi-Value Register |
| `ReplicationSlot::advance()` / `drop()` | `replication/replication_slot.h` | ✅ GA | Persistent WAL-Slots; LSN-Fortschritt; PAUSE/RESUME |

### Content-Modul (v1.x)

| Feature | Header | Status | Beschreibung |
|---------|--------|--------|-------------|
| `ContentManager::ingestRawBlob()` Pipeline 10 Stages | `content/content_manager.h` | ✅ GA | MIME-Auto, Malware-Scan, pHash-Dedup, OCR, Chunking, Embed, LLM-Tagging |
| `EmbeddingPipeline::embed()` | `content/embedding_pipeline.h` | ✅ GA | Batch-Embedding; normalize; Failure-Callback; Device-Auswahl |
| `AsyncIngestionWorker::enqueue()` | `content/async_ingestion_worker.h` | ✅ GA | 4 Worker-Threads; QUEUED/RUNNING/FAILED/RETRYING; Priority-Queue |
| `ContentSecurity::scan()` | `content/content_security.h` | ✅ GA | Malware/ZIP-Bomb/Abuse; sanitize_error_messages; block_on_malware |
| `IContentProcessor` Plugin-Interface | `content/content_processor.h` | ✅ GA | Eigene MIME-Typen; ExtractionResult (text/language/geo_data/media_data) |
| `ContentPolicy` MIME-Whitelist/Blacklist | `content/content_policy.h` | ✅ GA | allowed_mime_types; enable_deduplication; ocr_enabled; isAllowed() |

### Acceleration-Internals (v1.x)

| Feature | Header | Status | Beschreibung |
|---------|--------|--------|-------------|
| `ComputeBackend::executeSimilarityKernel()` | `acceleration/compute_backend.h` | ✅ GA | FP32/FP16/BF16/INT8/INT4; CUDA/HIP/Vulkan/OpenCL/DirectX; Health-Status |
| `ANNKernelFallbackDispatcher::search()` | `acceleration/kernel_fallback_dispatcher.h` | ✅ GA | Fallback-Kette CUDA→HIP→Vulkan→CPU; RetryPolicy; backend_used |
| `GeoKernelFallbackDispatcher::computeDistancesBatch()` | `acceleration/kernel_fallback_dispatcher.h` | ✅ GA | Haversine/Vincenty Batch; GPU-Fallback auf CPU |
| `VecKnnPipeline::search()` / `searchBatch()` | `acceleration/vec_knn.h` | ✅ GA | IVF nprobe + HNSW ef_search; Distance-Cache; AQL-Filter |
| `DeviceManager::getDevices()` | `acceleration/device_manager.h` | ✅ GA | Multi-GPU-Inventar; VRAM/Compute-Units; hasGPU(); logDeviceInfo() |
| `CudaBuffer<T>` / `VulkanBuffer<T>` RAII | `acceleration/raii/cuda_raii.h` | ✅ GA | Automatische GPU-Speicherfreigabe; upload/download |
