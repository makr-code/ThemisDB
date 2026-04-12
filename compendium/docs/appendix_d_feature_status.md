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

### AI / Acceleration

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| `AiHardwareDispatcher` | `acceleration/` | ✅ GA | AHD-01..AHD-30 | NPU-Prioritätskette, INT4/W4A8, ONNX-EP-Fallback |

### Utils

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| UUID v7 (`generate_uuid_v7`) | `utils/` | ✅ GA | UV7-01..UV7-20 | RFC 9562, 48-bit Timestamp, 18-bit Seq, MT19937-64 |
| Streaming ZSTD (`zstd_compress_stream`) | `utils/` | ✅ GA | ZS-01..ZS-10 | ZSTD_CStream/DStream, max_output_bytes DoS-Guard |

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

### Ethics AI Plugin (v0.0.x)

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

### Sharding / Paxos

| Feature | Modul | Status | Tests | Beschreibung |
|---------|-------|--------|-------|-------------|
| Paxos WAL `logAccept` | `sharding/` | ✅ GA | — | ACCEPT vor Quorum-Broadcast persistiert |
| Paxos WAL `logCommit` | `sharding/` | ✅ GA | — | COMMIT in `broadcastCommit()` persistiert |
| `recoverFromWAL()` | `sharding/` | ✅ GA | — | Snapshot + WAL-Replay; `commit_index_` wird restauriert |
| Snapshot-Compaction | `sharding/` | ✅ GA | — | `shouldCreateSnapshot(ops)` nach konfigurierbaren Operationen |

---

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
