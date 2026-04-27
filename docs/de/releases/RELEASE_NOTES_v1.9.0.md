# ThemisDB v1.9.0 — Release Aggregation

**Release Date:** 2026-04-11  
**Type:** Feature Release  
**Previous Version:** v1.8.0 (2026-03-22) / v1.8.1-rc1 (2026-04-04)  
**Milestone:** v1.9.0  
**Aggregation Issue:** [makr-code/ThemisDB#3071](https://github.com/makr-code/ThemisDB/issues/3071)

---

## 🎯 Übersicht

ThemisDB v1.9.0 ist ein breites Feature- und Härtungs-Release mit über 40 gemergten Pull Requests und
mehr als 20 abgeschlossenen Milestone-Issues. Schwerpunkte sind der neue Multi-Model-Adapter **Chimera**
(Streaming, Prepared Statements, Connection-Pool), die vollständigen **Compliance-Evaluatoren** für ISO 27001
und HIPAA, tiefgreifende **Netzwerkinfrastruktur**-Erweiterungen (IPv6, QUIC/HTTP3, gRPC, Istio,
Kernel Bypass DPDK/io_uring), umfangreiche **Performance**-Optimierungen (FAISS GPU Backend, NUMA,
Workload-Adaptive Optimizer, Advanced Cache Optimization), sowie Ausbau der **KI/ML-Pipeline**
(Knowledge Graph RAG, Forecasting Batch/Streaming, NCCL/RCCL distributed mergeTopK) und
der **Sicherheitsschicht** (mTLS, GDPR PII Purge, Authenticode/GPG, InputValidator).

---

## ⚠️ Breaking Changes

| # | Modul | Änderung | Migration |
|---|-------|----------|-----------|
| 1 | **query** | `QueryEngine::createDefault()` wirft ab v1.9.0 `std::runtime_error` wenn keine konkreten Storage/Index-Adapter injiziert wurden — bisher stub-mäßig ignoriert | Konstruktor-Injektion von `IStorageEnginePtr` + `IIndexManagerPtr` verwenden; `createDefault()`-Aufrufe nur wo beide Abhängigkeiten verfügbar sind |
| 2 | **cache** | L1-Cache-Felder atomicisiert; `l1_eviction_mutex_` trennt Eviction von Read-Path — direktes Lock-Acquire auf `l1_mutex_` in abgeleiteten Klassen bricht Invariante | Eigene Cache-Subklassen auf `std::shared_mutex` (read) / unique_lock (write) umstellen; Eviction nur über `l1_eviction_mutex_` |
| 3 | **importers** | Stable Plugin ABI `THEMIS_IMPORTER_PLUGIN_V1` eingeführt — ältere DSO-Plugins ohne `themis_importer_create`-Export werden nicht mehr geladen | Importer-Plugins gegen neue ABI-Headerdatei `include/importers/importer_plugin.h` neu kompilieren; Versioning-Struct `THEMIS_IMPORTER_PLUGIN_V1` im Plugin-Entry-Point befüllen |

---

## 📦 Enthaltene Pull Requests

### Kern-Features (v1.9.0-spezifisch)

| PR # | Modul | Inhalt |
|------|-------|--------|
| [#4478](https://github.com/makr-code/ThemisDB/pull/4478) | **chimera** | Streaming Result Sets, Prepared Statements, Connection-Pool-Adapter-Schnittstellen (Issue #3509) |
| [#4484](https://github.com/makr-code/ThemisDB/pull/4484) | **governance** | ISO 27001 Annex A Control Evaluators + HIPAA Security Rule Evaluators (Issue #3515) |

### Netzwerkinfrastruktur

| PR # | Modul | Inhalt |
|------|-------|--------|
| [#3769](https://github.com/makr-code/ThemisDB/pull/3769) | **network** | Vollständige IPv6 Dual-Stack-Unterstützung im Wire Protocol Server (Issue #3754) |
| [#3337](https://github.com/makr-code/ThemisDB/pull/3337) | **network** | Istio/Envoy Sidecar-Kompatibilität für Service-Mesh (Issue #2208) |
| [#3335](https://github.com/makr-code/ThemisDB/pull/3335) | **network** | `GeoTopologyRouter` — `fallback_cross_region`-Durchsetzung korrigiert + Dead-Code-Removal |
| [#3299](https://github.com/makr-code/ThemisDB/pull/3299) | **network** | Nativer gRPC-Transport für Binary Wire Protocol (Issue #2024) |
| [#3291](https://github.com/makr-code/ThemisDB/pull/3291) | **network** | QUIC/HTTP3-Transportschicht-Integration (Issue #1994) |
| [#2926](https://github.com/makr-code/ThemisDB/pull/2926) | **network** | TCP-Multiplexing Code-Audit — Aggregat-Stats, COMPRESSED Guard, Session-Cleanup (Issues #2095, #2204) |
| [#2925](https://github.com/makr-code/ThemisDB/pull/2925) | **network** | LZ4 und Zstd Verbindungsebenen-Kompression für Wire Protocol V2 (Issue #2206) |
| [#2924](https://github.com/makr-code/ThemisDB/pull/2924) | **network** | Per-Tenant-Netzwerkbandbreiten-Quotas + Audit-Fixes (Issue #2205) |
| [#4501](https://github.com/makr-code/ThemisDB/pull/4501) | **network** | `NetworkAuditLog` und `AdaptiveIOScaler` implementiert |

### Performance & Hardware

| PR # | Modul | Inhalt |
|------|-------|--------|
| [#4495](https://github.com/makr-code/ThemisDB/pull/4495) | **acceleration** | PERF-D3 — Parallel Batch Insert + SIMD Distance Pipeline (~60 % schnellerer Vektor-Insert) |
| [#4568](https://github.com/makr-code/ThemisDB/pull/4568) | **acceleration** | NCCL/RCCL Distributed `mergeTopK` Integrationstest (Issue #3867) |
| [#4505](https://github.com/makr-code/ThemisDB/pull/4505) | **performance** | `NUMAMemoryManager` — Topology-Detection, Affinity-Allokation, Migration + Stats (Issue #4058) |
| [#4493](https://github.com/makr-code/ThemisDB/pull/4493) | **timeseries** | PERF-D1 — `bench_timeseries_adaptive_flush` Benchmark (≥ 500 k Pts/s Akzeptanztest) |
| [#4494](https://github.com/makr-code/ThemisDB/pull/4494) | **storage** | PERF-D5 — Streaming-Blob-Schreibpfad: Parallel-Chunking + atomic WriteBatch |
| [#4497](https://github.com/makr-code/ThemisDB/pull/4497) | **query** | PERF-D7 — `bench_query_lazy_eval` Benchmark für columnar SIMD Lazy Evaluation |
| [#4498](https://github.com/makr-code/ThemisDB/pull/4498) | **transaction** | PERF-D4 — Batched Prepare + Lock-Free 2PC-Koordination (~60 % weniger Latenz) |
| [#4499](https://github.com/makr-code/ThemisDB/pull/4499) | **timeseries** | PERF-D1-C — Regressions- und Performance-Tests für `AdaptiveFlushController` |
| [#4500](https://github.com/makr-code/ThemisDB/pull/4500) | **timeseries** | `AdaptiveFlushController` in TSStore-Schreibpfad integriert |
| [#4571](https://github.com/makr-code/ThemisDB/pull/4571) | **index** | Secondary-Index-Schreibpfad-Overhead reduziert |

### Auth & Security

| PR # | Modul | Inhalt |
|------|-------|--------|
| [#2777](https://github.com/makr-code/ThemisDB/pull/2777) | **auth** | mTLS Zertifikats-basierte Authentifizierung (Issue #1549) |
| [#2815](https://github.com/makr-code/ThemisDB/pull/2815) | **cache** | GDPR Art. 17 PII-Purge-Propagation + Audit-Fixes (Issue #1591) |
| [#2654](https://github.com/makr-code/ThemisDB/pull/2654) | **themis** | Authenticode (Windows) & GPG (Linux) Signaturverifizierung für ModuleLoader (Issue #2473) |
| [#4513](https://github.com/makr-code/ThemisDB/pull/4513) | **security** | `InputValidator` Security-API implementiert + Input-Validierungs-Tests reaktiviert |
| [#4474](https://github.com/makr-code/ThemisDB/pull/4474) | **auth** | Fehlende fokussierte Testtargets in Build- und CMake-Definitionspfaden registriert |

### KI/ML & RAG

| PR # | Modul | Inhalt |
|------|-------|--------|
| [#2748](https://github.com/makr-code/ThemisDB/pull/2748) | **rag** | Knowledge Graph-augmentiertes Retrieval mit Entity-Linking + Audit-Fix (Issue #2242) |
| [#4509](https://github.com/makr-code/ThemisDB/pull/4509) | **rag** | `MultiHopReasoner` und `AdaptiveRetrieval` implementiert (Phase 7) |
| [#2694](https://github.com/makr-code/ThemisDB/pull/2694) | **aql** | AQL-Query-Migrations-Assistent (ArangoDB AQL → ThemisDB AQL) (Issue #1360) |
| [#2605](https://github.com/makr-code/ThemisDB/pull/2605) | **voice** | Speaker-Verifizierung für Voice-Biometrics + Audit-Fixes (Issue #2494) |

### Query & Storage

| PR # | Modul | Inhalt |
|------|-------|--------|
| [#4496](https://github.com/makr-code/ThemisDB/pull/4496) | **cdc** | CDC `SequenceCounter` — WriteBatch-Optimierung (2 → 1 WAL-Schreib/Event) + False-Sequence-Fix |

### Observability & Server

| PR # | Modul | Inhalt |
|------|-------|--------|
| [#4503](https://github.com/makr-code/ThemisDB/pull/4503) | **observability** | Per-Tenant Metric-Namespacing + strukturiertes Log-Search |
| [#4504](https://github.com/makr-code/ThemisDB/pull/4504) | **plugins** | Runtime Capability Escalation Blocking implementiert |
| [#4510](https://github.com/makr-code/ThemisDB/pull/4510) | **scheduler** | 10 fehlende fokussierte Testtargets + SLO-basiertes adaptives Rate-Limiting registriert |
| [#4511](https://github.com/makr-code/ThemisDB/pull/4511) | **search** | Phase 5 Schnittstellen — `ConversationalSearch`, `FederatedSearch`, `SearchQualityMonitor` |
| [#4514](https://github.com/makr-code/ThemisDB/pull/4514) | **sharding** | Phase 4.2 — `AutoFailoverManager` fokussierte Testsuite (39 Tests) |

### Prompt Engineering & Dokumentation

| PR # | Modul | Inhalt |
|------|-------|--------|
| [#4506](https://github.com/makr-code/ThemisDB/pull/4506) | **prompt_engineering** | Typed Template DSL — `PromptTemplateCompiler`, `IPromptTemplate`-Schnittstellen |
| [#2754](https://github.com/makr-code/ThemisDB/pull/2754) | **acceleration** | Betriebshandbuch — Operational Troubleshooting Guide (Issue #1396) |
| [#2688](https://github.com/makr-code/ThemisDB/pull/2688) | **acceleration** | Capability-Negotiation und Fallback-Verhalten Leitfaden (Issue #1402) |
| [#1324](https://github.com/makr-code/ThemisDB/pull/1324) | **docs** | Produktions-Runbook, Security-Posture-Guide und Deployment-Configs (Issue #1255) |

---

## 🗂️ Roadmap-Segmente — Abgeschlossene Ziele

### Segment: Chimera (Multi-Model-Adapter)

> Roadmap-Datei: `src/chimera/ROADMAP.md`

- [x] Adapter-API für Streaming/Prepared Statements im Header-Shim definiert (PR #4478)
- [x] Simulationsmodus für relationale, dokumenten-, graph- und vektorbezogene Grundpfade (PR #4478)
- [x] Streaming- und Prepared-Statement-Basisimplementierung (PR #4478)
- [x] Verbindungsprüfung und strukturierte Fehlercodes in Kernpfaden (PR #4478)
- [x] Streaming-Tests: `tests/chimera/test_chimera_streaming.cpp` (PR #4478)
- [x] Prepared-Statement-Tests: `tests/chimera/test_chimera_prepared_statements.cpp` (PR #4478)

### Segment: Governance & Compliance

> Roadmap-Datei: `src/governance/ROADMAP.md`

- [x] ISO 27001 Annex A Control Evaluators (`iso27001_rules.cpp`, `Iso27001ControlSet`) (PR #4484)
- [x] HIPAA Security Rule Evaluators (`hipaa_rules.cpp`, `HipaaRuleSet`) (PR #4484)
- [x] PolicyManager Hot-Reload bereits in v1.8.0 abgeschlossen ✅

### Segment: Netzwerk

> Roadmap-Datei: `src/network/ROADMAP.md`

- [x] IPv6 Dual-Stack-Unterstützung (PR #3769, Issue #3754)
- [x] QUIC/HTTP3-Transportschicht (PR #3291, Issue #1994)
- [x] Nativer gRPC-Transport (PR #3299, Issue #2024)
- [x] Istio/Envoy Sidecar-Kompatibilität (PR #3337, Issue #2208)
- [x] LZ4/Zstd Verbindungskompression (PR #2925, Issue #2206)
- [x] Per-Tenant-Bandbreiten-Quotas (PR #2924, Issue #2205)
- [x] TCP-Multiplexing Audit + Fixes (PR #2926, Issues #2095, #2204)
- [x] Kernel Bypass DPDK/io_uring — `DPDKServer`, `IoUringServer`, `CpuPinner`, `NumaAllocator`, `ZeroCopyDmaBuffer` (Issue #4057)

### Segment: Performance & Acceleration

> Roadmap-Dateien: `src/acceleration/ROADMAP.md`, `src/performance/ROADMAP.md`

- [x] FAISS GPU Backend — IVF_SQ8 (`GpuIndexIVFScalarQuantizer`) + HNSW_FLAT (`faiss::IndexHNSWFlat`); 50 Tests in `tests/test_faiss_gpu_backend.cpp` (Issue #4052)
- [x] BackendRegistry O(n²) → O(1) — `typeIndex_` (`unordered_map<BackendType, RegisteredBackend>`); hot query path ohne `dynamic_cast` (Issue #4066)
- [x] PMU non-Linux Stub-Abdeckung — macOS `kpc`, Windows `QueryThreadCycleTime`, RDTSC/CNTVCT_EL0-Fallback (Issue #4086)
- [x] Workload-Adaptive Optimizer — OLTP/OLAP/MIXED/GRAPH/VECTOR/TIMESERIES Klassifikation, dynamische Strategie, Predictive Scaling (Issue #4060)
- [x] Advanced Cache Optimization — Multi-Partition-Cache, Bloom-Filter, adaptive Eviction (LRU/LIRS/ARC/2Q), transparente Kompression (Issue #4059)
- [x] NUMA-Aware Memory Management — `NUMAMemoryManager`, Topology-Detection, Affinity-Allokation (Issue #4058, PR #4505)
- [x] Kernel Block-Dimension Occupancy Tuning (Issue #4064)
- [x] TensorCore INT8 Quantized Precision Path (Issue #4051)
- [x] CUDA HNSW Kernel — Visited-Array-Memory-Skalierung (Issue #4050)
- [x] Speculative Decoding für Latenzreduzierung (Issue #4055)
- [x] NCCL/RCCL Distributed `mergeTopK` (Issue #3867, PR #4568)
- [x] Multi-GPU Sharding für große Embedding-Datasets (Issue #4053)

### Segment: Query & Storage

> Roadmap-Dateien: `src/query/ROADMAP.md`, `src/storage/ROADMAP.md`

- [x] `QueryFederation` Shard-Key-Routing (Point-Lookup + Range) via `ShardingManager::GetShardForKey` / `GetShardsForKeyRange`
- [x] `QueryEngine::createDefault()` — Injektion von `RocksDBWrapper` + `SecondaryIndexManager`
- [x] Query Rewriting für Graph-Optimierung (Issue #4080)
- [x] Automatische Indexierungs-Empfehlungen (Issue #4084)
- [x] Adaptive Deadlock Prevention (Issue #4091)
- [x] WiscKey Garbage Collection / Log Compaction (Issue #940)

### Segment: Cache & CDC

> Roadmap-Dateien: `src/cache/ROADMAP.md`, `src/cdc/ROADMAP.md`

- [x] Lock-Free L1 Read Path — `l1_mutex_` → `std::shared_mutex`; `L1Entry`-Felder atomicisiert; Lazy-Expiry via CAS
- [x] `KafkaCDCProducer` + `ICDCTransport` + `cdc_kafka.yaml`; `CDCKafkaProducerFocusedTests` in `tests/CMakeLists.txt` (Issue #3992)

### Segment: Analytics & Forecasting

> Roadmap-Datei: `src/analytics/ROADMAP.md`

- [x] Forecasting-Erweiterungen: `predictBatch()` (N-Series parallel), `update(double)` (O(1) inkrementelle ETS/ARIMA/LR-Zustandsabsorption), parallele Auto-Tune Grid Search via `std::async`, FNV-1a 64-bit Fit-Result-Cache; 17 neue Tests (Issue #4054)

### Segment: API & gRPC

> Roadmap-Datei: `src/api/ROADMAP.md`

- [x] gRPC Factory Wiring für `ExecuteAQL`, `StreamAQL`; Search-RPCs feature-gated über `ThemisDBGrpcServiceFactory`
- [x] `GrpcApiServer::start()` — `mutex_` vor `BuildAndStart()` freigegeben
- [x] `GrpcApiServer::stop()` — 30-Sekunden `Shutdown()`-Deadline

### Segment: Server & MQTT

> Roadmap-Datei: `src/server/ROADMAP.md`

- [x] `MqttClientService` — bidirektionaler MQTT-Client; Boost.Asio async I/O; Auto-Reconnect mit exponential Back-off aus `MqttRetryConfig`
- [x] `MqttCDCTransport : ICDCTransport` — CDC → MQTT Bridge; `{cdc_topic_prefix}{collection}/{EVENT_TYPE}` Topic-Schema

### Segment: Importers — Stable Plugin ABI

> Roadmap-Datei: `src/importers/ROADMAP.md`, Phase 10

- [x] Stabile C-Linkage Plugin ABI in `include/importers/importer_plugin.h`; `THEMIS_IMPORTER_PLUGIN_V1` versioniertes Struct
- [x] `ImporterRegistry::loadPlugin(path)` und `unloadPlugin(name)` via `themis_importer_create`
- [x] `PluginSandboxConfig` — per-Job Memory-Limit + Timeout-Durchsetzung via dediziertem Thread
- [x] `V1ImporterAdapter` — `IImporter`-Wrapper für V1-Plugins mit Sandbox-Allocator-Callbacks
- [x] Oracle Importer Skeleton + V1 ABI Docs in `docs/importers/plugin_guide.md`
- [x] Neue Tests in `tests/test_importer_plugin_api.cpp` (V1 ABI, loadPlugin-Fehlerpfade, Sandbox-Config, `V1ImporterAdapter`-Lifecycle)

### Segment: LLM — DecisionRecord Integration

> Roadmap-Datei: `src/llm/ROADMAP.md`

- [x] `DecisionRecordYamlProcessor` in `LoraRouter`, `AdapterLoadBalancer`, `LoraOrchestrator` integriert via `setDecisionRecordProcessor()`
- [x] Fokussierte Tests: `DecisionRecordIntegrationFocusedTests` (DRI-01..11), `DecisionRecordYamlProcessorFocusedTests`, `DecisionRecordE2EFocusedTests`

### Segment: Konfiguration

> Roadmap-Datei: `src/config/ROADMAP.md`

- [x] Multi-Environment Config Overlay (dev/staging/prod) (Issue #3997)

### Segment: Search

> Roadmap-Datei: `src/search/ROADMAP.md`

- [x] Multi-Field-Boosting (title > body > tags) via `MultiFieldBoostedSearch`
- [x] Phase 5 Schnittstellen: `ConversationalSearch`, `FederatedSearch`, `SearchQualityMonitor` (PR #4511)

---

## ✨ Neue Features im Detail

### Chimera — Multi-Model-Adapter (PR #4478, Issue #3509)

> **Dateien:** `include/chimera/chimera_adapter.h`, `src/chimera/chimera_adapter.cpp`,
> `tests/chimera/test_chimera_streaming.cpp`, `tests/chimera/test_chimera_prepared_statements.cpp`

- **Streaming Result Sets**: Inkrementelle Ausgabe großer Ergebnismengen ohne vollständige Materialisierung
- **Prepared Statements**: Parametrisierte Abfragen mit typisierten Bind-Variablen und Wiederverwendung des Execution-Plans
- **Connection Pool Adapter**: Verwalteter Pool für parallele Multi-Model-Verbindungen (relational, dokumenten, graph, vektor)
- **Simulationsmodus**: Testbarer Grundpfad für alle vier Datenbankmodelle ohne externe Abhängigkeiten
- **Strukturierte Fehlercodes**: Einheitliche Fehlerklassifikation über alle Adapterpfade
- Verbindungsprüfung beim Pool-Checkout mit konfigurierbarem Timeout

### Governance — ISO 27001 & HIPAA Compliance (PR #4484, Issue #3515)

> **Dateien:** `include/governance/iso27001_rules.h`, `src/governance/iso27001_rules.cpp`,
> `include/governance/hipaa_rules.h`, `src/governance/hipaa_rules.cpp`

- **`Iso27001ControlSet`**: Evaluatoren für ISO 27001 Annex A Controls (Zugriffskontrolle, Kryptographie, Betriebssicherheit, Kommunikationssicherheit, Compliance)
- **`HipaaRuleSet`**: Evaluatoren für HIPAA Security Rule (Administrative Safeguards, Physical Safeguards, Technical Safeguards)
- Bewertung von Controls gegen aktuelle Systemkonfiguration mit strukturiertem `ComplianceResult`
- Automatisierte Nachweis-Generierung für Audit-Reports

### Netzwerk — IPv6 Dual-Stack (PR #3769, Issue #3754)

- Vollständige IPv6-Unterstützung im Wire Protocol Server (RFC 4038)
- Dual-Stack-Konfiguration: paralleler Betrieb auf IPv4 + IPv6
- Socket-Options für IPv6-Only-Mode (`IPV6_V6ONLY`)
- `[::1]`-Format für IPv6-Literal in Konfigurationsdateien

### Netzwerk — QUIC/HTTP3 (PR #3291, Issue #1994)

- QUIC-Transportschicht als Alternative zu TCP für latenzarme, mehrfach-gestreamte Verbindungen
- HTTP/3 Session-Multiplex ohne Head-of-Line-Blocking
- Integration mit bestehendem `WireProtocolServer`; Fallback auf HTTP/2 wenn QUIC nicht verfügbar

### Performance — FAISS GPU Backend (Issue #4052)

- **IVF_SQ8** via `GpuIndexIVFScalarQuantizer` (QT_8bit) — 8-Bit Scalar Quantisierung für VRAM-Effizienz
- **HNSW_FLAT** via `faiss::IndexHNSWFlat` mit `hnswM`-Konfigurationsfeld
- Vollständige `default:`-Zweige in allen Switch-Statements
- `setError()`-Helper ersetzt bare `std::cerr`
- `getCapabilities()` meldet jetzt `INT8` Precision und L2/IP Metric-Bits
- 50 Tests in `tests/test_faiss_gpu_backend.cpp`

### Performance — Workload-Adaptive Optimizer (Issue #4060)

- Automatische Workload-Klassifikation: OLTP / OLAP / MIXED / GRAPH / VECTOR / TIMESERIES
- Dynamische Strategie-Auswahl basierend auf Echtzeitmetriken
- Ressourcen-Reallokation zwischen Workload-Wechseln
- Performance-Feedback-Loop für kontinuierliche Selbstoptimierung
- Predictive Scaling basierend auf historischen Lastprofilen

### Performance — Advanced Cache Optimization (Issue #4059)

- Multi-Partition-Cache zur Parallelisierung von Read/Write-Zugriff
- Bloom-Filter Pre-Screening — reduziert unnötige Cache-Lookups
- Adaptive Eviction: LRU / LIRS / ARC / 2Q dynamisch auswählbar
- Transparente Value-Kompression (LZ4) für hot-data-intensive Workloads
- Cache-oblivious Scan-Helper für sequentielle Zugriffsmuster
- Per-Partition Hit/Miss-Statistiken für granulares Monitoring

### Auth — mTLS Zertifikatsauthentifizierung (PR #2777, Issue #1549)

- Gegenseitige TLS-Authentifizierung mit Client-Zertifikat-Verifizierung (RFC 5246)
- Integration mit bestehender PKI (`PKIClient`)
- Konfigurierbare CA-Chain-Validierung; Revocation-Check via CRL/OCSP (bereits in v1.8.0)
- RBAC-Mapping aus Zertifikats-Subject-DN und SAN-Extension

### RAG — Knowledge Graph-augmentiertes Retrieval (PR #2748, Issue #2242)

- Entity Linking: Erkennung von Named Entities in Suchanfragen; Verknüpfung mit Wissens-Graph-Knoten
- `MultiHopReasoner`: Mehrstufiges Reasoning über Graph-Kanten für komplexe Anfragen (PR #4509)
- `AdaptiveRetrieval`: Dynamische Auswahl zwischen Dense/Sparse/Graph-Retrieval basierend auf Anfrage-Typ (PR #4509)
- Audit-Log-Integration für Retrieval-Entscheidungen

---

## 🔗 Zugeordnete Issues

### Milestone-Issues (v1.9.0)

| Issue | Titel | Status |
|-------|-------|--------|
| [#3509](https://github.com/makr-code/ThemisDB/issues/3509) | [MODULE] chimera | ✅ Geschlossen |
| [#3515](https://github.com/makr-code/ThemisDB/issues/3515) | [MODULE] governance | ✅ Geschlossen |
| [#3754](https://github.com/makr-code/ThemisDB/issues/3754) | Full IPv6 Support in Wire Protocol | ✅ Geschlossen |
| [#3867](https://github.com/makr-code/ThemisDB/issues/3867) | NCCL/RCCL Distributed `mergeTopK` | ✅ Geschlossen |
| [#3954](https://github.com/makr-code/ThemisDB/issues/3954) | Distributed Transaction Coordinator (2PC) | ✅ Geschlossen |
| [#3955](https://github.com/makr-code/ThemisDB/issues/3955) | Distributed SAGA Coordinator | ✅ Geschlossen |
| [#3992](https://github.com/makr-code/ThemisDB/issues/3992) | Kafka-Compatible Producer Interface | ✅ Geschlossen |
| [#3997](https://github.com/makr-code/ThemisDB/issues/3997) | Multi-Environment Config Overlay (dev/staging/prod) | ✅ Geschlossen |
| [#4050](https://github.com/makr-code/ThemisDB/issues/4050) | CUDA HNSW Kernel: Visited Array Memory Scaling | ✅ Geschlossen |
| [#4051](https://github.com/makr-code/ThemisDB/issues/4051) | TensorCore Matmul: INT8 Quantized Precision Path | ✅ Geschlossen |
| [#4052](https://github.com/makr-code/ThemisDB/issues/4052) | FAISS GPU Backend: HNSW and ScalarQuantizer Index Types | ✅ Geschlossen |
| [#4053](https://github.com/makr-code/ThemisDB/issues/4053) | Multi-GPU Sharding for Large Embedding Datasets | ✅ Geschlossen |
| [#4054](https://github.com/makr-code/ThemisDB/issues/4054) | Forecasting: Batch Prediction, Streaming Update, SIMD Fit | ✅ Geschlossen |
| [#4055](https://github.com/makr-code/ThemisDB/issues/4055) | Speculative Decoding for Latency Reduction | ✅ Geschlossen |
| [#4057](https://github.com/makr-code/ThemisDB/issues/4057) | Kernel Bypass (DPDK/io_uring) | ✅ Geschlossen |
| [#4058](https://github.com/makr-code/ThemisDB/issues/4058) | NUMA-Aware Memory Management | ✅ Geschlossen |
| [#4059](https://github.com/makr-code/ThemisDB/issues/4059) | Advanced Cache Optimization | ✅ Geschlossen |
| [#4060](https://github.com/makr-code/ThemisDB/issues/4060) | Workload-Adaptive Optimization | ✅ Geschlossen |
| [#4064](https://github.com/makr-code/ThemisDB/issues/4064) | Kernel Block-Dimension Occupancy Tuning | ✅ Geschlossen |
| [#4066](https://github.com/makr-code/ThemisDB/issues/4066) | BackendRegistry: O(n²) Backend Selection Index | ✅ Geschlossen |
| [#4080](https://github.com/makr-code/ThemisDB/issues/4080) | Query Rewriting for Graph Optimization | ✅ Geschlossen |
| [#4084](https://github.com/makr-code/ThemisDB/issues/4084) | Automatic Indexing Recommendations | ✅ Geschlossen |
| [#4086](https://github.com/makr-code/ThemisDB/issues/4086) | Phase 4: PMU Counters — Non-Linux Stub Coverage | ✅ Geschlossen |
| [#4091](https://github.com/makr-code/ThemisDB/issues/4091) | Adaptive Deadlock Prevention | ✅ Geschlossen |

### Weitere geschlossene Milestone-Issues (Netzwerk, Auth, Integration)

| Issue | Titel | Status |
|-------|-------|--------|
| [#1255](https://github.com/makr-code/ThemisDB/issues/1255) | Production Runbook, Documentation and Security Posture | ✅ |
| [#1360](https://github.com/makr-code/ThemisDB/issues/1360) | AQL query migration assistant | ✅ |
| [#1396](https://github.com/makr-code/ThemisDB/issues/1396) | Publish operational troubleshooting guide | ✅ |
| [#1402](https://github.com/makr-code/ThemisDB/issues/1402) | Capability negotiation and fallback behavior documentation | ✅ |
| [#1549](https://github.com/makr-code/ThemisDB/issues/1549) | Implement mTLS authentication using certificates | ✅ |
| [#1591](https://github.com/makr-code/ThemisDB/issues/1591) | GDPR-aware cache invalidation for PII data | ✅ |
| [#1994](https://github.com/makr-code/ThemisDB/issues/1994) | QUIC/HTTP3 transport layer integration | ✅ |
| [#2024](https://github.com/makr-code/ThemisDB/issues/2024) | gRPC native transport separate from server module | ✅ |
| [#2095](https://github.com/makr-code/ThemisDB/issues/2095) | Implement TCP multiplexing for multiple logical streams | ✅ |
| [#2204](https://github.com/makr-code/ThemisDB/issues/2204) | Implement TCP multiplexing for multiple logical streams | ✅ |
| [#2205](https://github.com/makr-code/ThemisDB/issues/2205) | Per-tenant network bandwidth quotas | ✅ |
| [#2206](https://github.com/makr-code/ThemisDB/issues/2206) | Implement LZ4 and Zstd connection compression | ✅ |
| [#2207](https://github.com/makr-code/ThemisDB/issues/2207) | Network topology-aware routing for geo-distributed clusters | ✅ |
| [#2208](https://github.com/makr-code/ThemisDB/issues/2208) | Implement Istio/Envoy sidecar compatibility for service mesh | ✅ |
| [#2242](https://github.com/makr-code/ThemisDB/issues/2242) | Knowledge graph-augmented retrieval for entity linking | ✅ |
| [#2473](https://github.com/makr-code/ThemisDB/issues/2473) | Authenticode & GPG Signature Verification for Modules | ✅ |
| [#2494](https://github.com/makr-code/ThemisDB/issues/2494) | Implement speaker verification for voice biometrics | ✅ |
| [#3071](https://github.com/makr-code/ThemisDB/issues/3071) | Release-Aggregation für v1.9.0 | 🔵 Dieses Dokument |
| [#3776](https://github.com/makr-code/ThemisDB/issues/3776) | Scheduled Graph Edge Refresh: Dokumentation & Architektur | ✅ |
| [#940](https://github.com/makr-code/ThemisDB/issues/940) | WiscKey: Garbage Collection/Log Compaction | ✅ |

---

## 🔒 QA-Abnahme

### QA-Kriterien (gemäß Issue #3071)

| Kriterium | Status | Nachweis |
|-----------|--------|----------|
| Endabnahme erfolgt | ✅ | Milestone-Stand 2026-04-11; alle PRs gemergt |
| Doku validiert | ✅ | Release Notes, Roadmap-Checkboxen, CHANGELOG |
| QA & Blocker-Lösung geprüft | ✅ | Keine offenen CRITICAL/HIGH-Blocker im Milestone |
| Alle Milestone-Issues geschlossen | ✅ (bis auf #3071) | GitHub Milestone v1.9.0 |

### Test-Coverage-Highlights

| Feature | Testsuite | Anzahl Tests |
|---------|-----------|-------------|
| Chimera Streaming | `test_chimera_streaming.cpp` | ≥ 10 |
| Chimera Prepared Statements | `test_chimera_prepared_statements.cpp` | ≥ 10 |
| FAISS GPU Backend | `test_faiss_gpu_backend.cpp` | 50 |
| Importer Plugin ABI | `test_importer_plugin_api.cpp` | ≥ 8 |
| DecisionRecord Integration | `DecisionRecordIntegrationFocusedTests` (DRI-01..11) | 11 |
| Forecasting Batch/Streaming | `ForecastingBatchStreamingTests` | 17 |
| CDC Kafka Producer | `CDCKafkaProducerFocusedTests` | ≥ 5 |
| AutoFailoverManager | Phase 4.2 Testsuite | 39 |

---

## 📈 Entwicklungstrends (v1.8.0 → v1.9.0)

| Trend | Beobachtung |
|-------|-------------|
| **Netzwerk-Tiefe** | Vollständiger Stack von QUIC/HTTP3 über gRPC bis Kernel Bypass; IPv6 durchgängig |
| **GPU-Abdeckung** | FAISS IVF_SQ8 + HNSW_FLAT; TensorCore INT8; NCCL/RCCL Multi-GPU; CUDA HNSW Memory |
| **Compliance-First** | ISO 27001 + HIPAA Evaluatoren als eigenständige, testbare Compliance-Engines |
| **Plugin-Ökosystem** | Stable ABI v1 für Importer-Plugins; Capability Escalation Blocking für Security |
| **Multi-Model** | Chimera bündelt relationale, dokument-, graph- und vektorbasierte Pfade in einheitlichem Adapter |
| **Performance-Durchgängigkeit** | PERF-D1..D7 Benchmark-Suite parallel zur Feature-Implementierung; adaptive Controller |
| **KI/ML-Pipeline** | MultiHop-RAG, AdaptiveRetrieval, DecisionRecordYamlProcessor, Speculative Decoding |
| **Concurrency-Härtung** | Lock-Free L1 Cache; `std::shared_mutex` Pattern konsequent durchgezogen |

---

## 📋 Produktions-Readiness-Checkliste

- [x] Alle v1.9.0 Milestone-PRs gemergt (#4478, #4484, + alle Netzwerk/Perf/Auth PRs)
- [x] Roadmap-Checkboxen in betroffenen `src/*/ROADMAP.md` auf `[x]` gesetzt
- [x] Keine offenen CRITICAL/HIGH Security Blocker (CVE-Waiver dokumentiert für LOW/MEDIUM in v1.8.1-rc1)
- [x] CHANGELOG.md Eintrag verlinkt auf dieses Dokument
- [x] QA-Kriterien aus Issue #3071 erfüllt (Endabnahme, Doku-Validierung)
- [x] Test-Coverage für alle Kern-Features vorhanden
- [x] Breaking-Change-Migration dokumentiert (3 Einträge)

---

## 🔍 Bekannte Einschränkungen

- **Kernel Bypass (DPDK/io_uring)**: `DPDKServer` und `IoUringServer` sind in dieser Version als vollständige Implementierungen verfügbar; Produktionseinsatz erfordert DPDK-kompatible Netzwerkkarte und Root-Berechtigungen
- **Chimera Simulationsmodus**: Vorproduktionstests verwenden den Simulationsmodus; für Produktion müssen echte Backend-Adapter injiziert werden
- **FAISS GPU Backend**: GPU-Ausführung setzt CUDA-Toolkit und kompatible GPU voraus; fallback auf CPU-Backend wenn unavailable
- **MQTT TLS** (`THEMIS_ENABLE_MQTT_TLS`): TLS-Support für `MqttClientService` ist für v1.10.0 vorgesehen (PR #4512 mit v1.10.0-Label); in v1.9.0 nur Plain-Text-MQTT verfügbar

---

> **Aggregations-Issue:** [makr-code/ThemisDB#3071](https://github.com/makr-code/ThemisDB/issues/3071)  
> **Erstellungsdatum:** 2026-04-27  
> **Erstellt von:** @copilot (retrospektive Release-Organisation)
