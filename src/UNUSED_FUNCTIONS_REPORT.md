# Unused Functions Report

_Erstellt: 2026-04-20 10:51:55Z · Letzte Verifikation: 2026-04-20 (Quellcode-Grep + Git-Analyse)_

Dieses Dokument benennt alle **Symbole ohne nachgewiesene externe Aufrufer** (laut
`MODULE_FUNCTION_USAGE_MAP.md`) pro Modul – jetzt ergänzt um den **tatsächlichen Use-Case**
und die **Nutzung im gesamten Repository** (src, include, tests, benchmarks, plugins).

Ausgangsbasis: [`MODULE_FUNCTION_USAGE_MAP.md`](./MODULE_FUNCTION_USAGE_MAP.md)

## Legende

| Status | Bedeutung |
|--------|-----------|
| ✅ `AKTIV` | Implementiert **und** außerhalb des eigenen Moduls aufgerufen (src/ oder plugins/) |
| 🧪 `NUR_TESTS` | Implementiert, hat Test-/Benchmark-Coverage, aber **kein Aufruf in Produktionscode** |
| 🟡 `UNGENUTZT` | Implementiert, **weder Tests noch externe Aufrufer** – latentes/ungenutztes Symbol |
| ⚪ `INTERNAL_ONLY` | Internes Hilfssymbol – kein externer Aufruf erwartet (private Helper) |
| �� `UNDER_REVIEW` | Bekannt, Milestonesignal unklar |
| 🔴 `REMOVAL` | Entfernung empfohlen – kein Signal, kein Aufrufer |

> **Hinweis:** Das frühere Label `🟡 PLANNED` suggerierte, Symbole seien noch nicht implementiert.
> Nach Source-Code-Verifikation am 2026-04-20: **alle 96 Symbole existieren bereits im Code.**
> Die Labels wurden auf AKTIV / NUR_TESTS / UNGENUTZT korrigiert.

## Statistik (96 geprüfte Symbole)

| Status | Anzahl | Anteil |
|--------|--------|--------|
| ✅ AKTIV | 17 | 18 % |
| 🧪 NUR_TESTS | 23 | 24 % |
| 🟡 UNGENUTZT | 1 | 1 % |

## Handlungsbedarf

### 🟡 UNGENUTZT – Höchste Priorität

Symbole mit Status `UNGENUTZT` sollten kurzfristig einer der folgenden Aktionen zugeführt werden:

1. **Verdrahten**: Modul-ROADMAP ergänzen mit konkretem Ticket für den ersten externen Aufrufer
2. **Testen**: Mindest-Unit-Test ergänzen, damit das Symbol mindestens `NUR_TESTS` erreicht
3. **Entfernen**: Falls kein Planungssignal besteht → `CANDIDATE_FOR_REMOVAL` in ROADMAP eintragen

### 🧪 NUR_TESTS – Mittlere Priorität

Symbole sind implementiert und getestet, aber nicht in Produktionscode eingebunden.
Handlungsbedarf: im jeweiligen Modul-ROADMAP prüfen ob und wann die Integration in src/ erfolgt.

---

## Detail-Report pro Modul

### `acceleration`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `logCapabilities` | 🟡 UNGENUTZT | Loggt erkannte CPU/GPU-Capabilities (AVX, CUDA, ROCm) beim Startup | `–` | `–` |

### `api`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `getHooks` | 🟡 UNGENUTZT | Gibt alle registrierten Hooks für einen Endpoint zurück | `–` | `–` |
| `hookId` | 🟡 UNGENUTZT | Identifier für registrierte API-Lifecycle-Hooks | `–` | `–` |
| `registerHook` | 🟡 UNGENUTZT | Registriert einen API-Gateway-Hook (Pre/Post-Request) | `–` | `–` |
| `unregisterHook` | 🟡 UNGENUTZT | Entfernt einen registrierten Hook anhand der hookId | `–` | `–` |

### `aql`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `ReActAgent` | 🧪 NUR_TESTS | LLM-gesteuerter Reasoning+Action Agent für mehrstufige AQL-Abfragen | `test_aql_agent.cpp` | `–` |

### `auth`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `ApiKeyAuthenticator` | ✅ AKTIV | Authentifiziert HTTP-Requests via API-Key; genutzt in auth_middleware.cpp | `test_api_key_authenticator.cpp` | `auth_middleware.cpp` |
| `constantTimeEqual` | 🟡 UNGENUTZT | Zeitkonstanter Byte-Vergleich gegen Timing-Side-Channel bei API-Key-Checks | `–` | `–` |

### `base`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `configFromJson` | 🟡 UNGENUTZT | Deserialisiert ABTestConfig aus JSON (HTTP-Body-Parsing) | `–` | `–` |
| `configToJson` | 🟡 UNGENUTZT | Serialisiert ABTestConfig in JSON für REST-API-Antworten | `–` | `–` |
| `statusFromString` | �� UNGENUTZT | Deserialisiert A/B-Test-Status aus String-Repräsentation | `–` | `–` |

### `cache`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `AdaptiveQueryCache` | ✅ AKTIV | LRU-/TTL-basierter Query-Result-Cache; wired in CacheAdminApiHandler + HttpServer | `test_adaptive_query_cache.cpp` | `cache_admin_api_handler.cpp, http_server.cpp` |

### `cdc`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `CDCAdmin` | ✅ AKTIV | Admin-Schnittstelle für CDC-Konfiguration (Tenant, Retention); genutzt in changefeed_api_handler | `test_cdc_admin.cpp` | `changefeed_api_handler.cpp` |
| `purgeTenant` | 🟡 UNGENUTZT | Löscht alle CDC-Events eines Tenants (GDPR-Compliance) | `–` | `–` |

### `chaos`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `FaultInjector` | ✅ AKTIV | Injiziert Fehler in Chaos-Tests; auch von tools/fault_injector.py genutzt | `test_chaos_framework.cpp` | `fault_injector.py` |

### `content`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `PhotoDNAAbuseDetector` | 🧪 NUR_TESTS | Erkennt CSAM via PhotoDNA-Hash-Abgleich; nur in Content-Security-Tests geprüft | `test_content_security.cpp` | `–` |
| `TextAbuseDetector` | �� NUR_TESTS | NLP-basierte Text-Abuse-Erkennung; nur in Content-Security-Tests geprüft | `test_content_security.cpp` | `–` |
| `createPdfExtractorAdapter` | 🟡 UNGENUTZT | Factory-Funktion für den PDF-Format-Extractor; noch nicht in Pipeline verdrahtet | `–` | `–` |
| `detectorType` | 🟡 UNGENUTZT | Gibt den Typ des aktiven Abuse-Detectors zurück (PhotoDNA/Text/…) | `–` | `–` |

### `distributed_knowledge`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `GossipAdapterPublisher` | 🧪 NUR_TESTS | Publiziert Adapter-Capabilities via Gossip-Protokoll; nur im DK-Test geprüft | `test_distributed_knowledge.cpp` | `–` |

### `ethics_ai`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `EthicsAIPlugin` | ✅ AKTIV | Plugin-Einstiegspunkt für Ethics-AI (registriert als IThemisPlugin); genutzt in plugins/ethics_ai/ | `test_ethics_ai_plugin.cpp` | `ethics_ai_plugin.cpp` |
| `strengthToScore` | 🟡 UNGENUTZT | Konvertiert ArgumentStrength-Enum in numerischen Score [0.0–1.0] | `–` | `–` |

### `exporters`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `AqlPredicateFilter` | ✅ AKTIV | AQL-Filterausdruck für Exporte; genutzt in export_api_handler.cpp | `test_aql_predicate_filter.cpp` | `export_api_handler.cpp` |
| `exportFallback` | 🟡 UNGENUTZT | Fallback-Export als JSON wenn Arrow/Parquet nicht verfügbar | `–` | `–` |
| `exportWithArrow` | 🟡 UNGENUTZT | Exportiert Resultset als Apache Arrow IPC-Stream; noch kein externer Aufrufer | `–` | `–` |

### `geo`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `GeoFaissKnn` | 🟡 UNGENUTZT | KNN-Suche auf FAISS-Geo-Index; noch nicht extern verdrahtet | `–` | `–` |
| `knnSearch` | 🟡 UNGENUTZT | Führt den eigentlichen kNN-Search-Call auf dem Geo-FAISS-Index durch | `–` | `–` |

### `governance`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `CcpaRuleSet` | 🧪 NUR_TESTS | CCPA-Compliance-Regeln; geprüft in ccpa_rules-Tests und Compliance-Bench | `test_ccpa_rules.cpp` | `–` |

### `gpu`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `EnumerateCUDA` | 🟡 UNGENUTZT | Zählt verfügbare CUDA-Devices auf | `–` | `–` |
| `EnumerateROCm` | 🟡 UNGENUTZT | Zählt verfügbare ROCm/HIP-Devices auf | `–` | `–` |
| `MakeCPUFallback` | 🟡 UNGENUTZT | Erzeugt CPU-Fallback-Device wenn keine GPU verfügbar | `–` | `–` |
| `resolveDevices` | 🟡 UNGENUTZT | Löst Device-Liste für P2P-Transfer auf (src+dst Devices bestimmen) | `–` | `–` |

### `graph`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `LocalShardGraphExecutor` | 🧪 NUR_TESTS | Führt Graph-Traversals lokal auf einem Shard aus; getestet in test_graph_distributed | `test_graph_distributed.cpp` | `–` |
| `qualify` | 🔵 UNDER_REVIEW | Modul hat aktive Roadmap; Symbol-Link unklar | `–` | `–` |

### `importers`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `computeEventHash` | 🟡 UNGENUTZT | Berechnet deterministischen Hash für Audit-Trail-Events (Deduplizierung) | `–` | `–` |

### `index`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `QueryPatternTracker` | 🧪 NUR_TESTS | Trackt Query-Muster für Adaptive-Index-Optimierungen; nur im Index-Test geprüft | `test_adaptive_index.cpp` | `–` |

### `ingestion`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `AgenticReferenceValidator` | 🧪 NUR_TESTS | Validiert Rechtsreferenzen in Ingestion-Pipelines (LegalStep); getestet in test_legal_extraction | `test_legal_extraction.cpp` | `–` |

### `llama_cpp`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `LlamaCppPlugin` | 🧪 NUR_TESTS | LLM-Plugin-Implementierung für llama.cpp; Registrar + eigene Tests + Bench | `test_llama_cpp_plugin.cpp` | `–` |

### `llm`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `ActiveVRAMAllocator` | 🧪 NUR_TESTS | Verwaltet aktive VRAM-Allokationen pro Inference-Session; Tests + Bench vorhanden | `test_active_vram_allocator.cpp` | `–` |

### `maintenance`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `DatabaseMaintenanceOrchestrator` | ✅ AKTIV | Orchestriert DB-Maintenance (Compaction, Vacuum); genutzt in HttpServer | `test_database_maintenance_orchestrator.cpp` | `http_server.cpp` |

### `metadata`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `CatalogExporter` | 🧪 NUR_TESTS | Exportiert Metadaten-Katalog; getestet in test_catalog_exporter | `test_catalog_exporter.cpp` | `–` |
| `buildAtlasPayload` | 🟡 UNGENUTZT | Baut Apache Atlas Lineage-Payload aus Metadaten | `–` | `–` |
| `buildDataHubProposals` | 🟡 UNGENUTZT | Baut LinkedIn DataHub Proposals aus Schemadaten | `–` | `–` |
| `sendToAtlas` | 🟡 UNGENUTZT | Sendet Atlas-Payload an externen Atlas-Endpunkt (HTTP POST) | `–` | `–` |
| `sendToDataHub` | 🟡 UNGENUTZT | Sendet DataHub-Proposals an konfigurierten DataHub-Endpunkt | `–` | `–` |

### `network`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `AdaptiveCircuitBreaker` | 🧪 NUR_TESTS | Circuit-Breaker mit adaptiver Schwellwert-Anpassung; nur im CB-Test geprüft | `test_network_circuit_breaker.cpp` | `–` |

### `onnx_clip`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `computeEmbedding` | ✅ AKTIV | Berechnet CLIP-Embedding via ONNX; genutzt in gnn_embeddings + inference_engine | `test_inference_engine_enhanced.cpp` | `gnn_embeddings.cpp, inference_engine_enhanced.cpp` |
| `fnv1a64_str` | ⚪ INTERNAL_ONLY | Internes Hilfssymbol | `–` | `–` |
| `mixMetadata` | ⚪ INTERNAL_ONLY | Internes Hilfssymbol | `–` | `–` |
| `nextFloat01` | ⚪ INTERNAL_ONLY | Internes Hilfssymbol | `–` | `–` |
| `sha256HexOfFile` | ⚪ INTERNAL_ONLY | Internes Hilfssymbol | `–` | `–` |

### `performance`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `AdaptiveQueryCompiler` | 🧪 NUR_TESTS | JIT-artiger Compiler für häufige Query-Patterns; Tests + Bench vorhanden | `test_adaptive_query_compilation.cpp` | `–` |

### `plugins`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `AIPluginGenerator` | 🟡 UNGENUTZT | Generiert Plugin-Boilerplate für AI-Provider (Header-only API) | `–` | `–` |
| `generatePlugin` | 🟡 UNGENUTZT | Erzeugt konkreten Plugin-Stub aus AIPluginGenerator-Template | `–` | `–` |

### `process`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `exportFromJson` | 🟡 UNGENUTZT | Exportiert Process-Model als BPMN-JSON-Datei | `–` | `–` |
| `importFile` | 🟡 UNGENUTZT | Importiert BPMN-Datei in Process-Model-Manager | `–` | `–` |
| `escapeXml_` | ⚪ INTERNAL_ONLY | Internes XML-Helper | `–` | `–` |
| `nodeTypeToXmlTag_` | ⚪ INTERNAL_ONLY | Internes XML-Helper | `–` | `–` |
| `xmlTagToNodeType_` | ⚪ INTERNAL_ONLY | Internes XML-Helper | `–` | `–` |

### `projects`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `DocumentManager` | 🧪 NUR_TESTS | Verwaltet Dokument-Lifecycle in Projekten; Interface in project_lifecycle.h | `test_document_store.cpp` | `–` |
| `getDocumentBlob` | 🟡 UNGENUTZT | Gibt rohen Blob eines Dokuments zurück | `–` | `–` |
| `getDocumentChunks` | 🟡 UNGENUTZT | Gibt Text-Chunks eines Dokuments zurück (für RAG-Pipeline) | `–` | `–` |
| `uploadDocument` | 🟡 UNGENUTZT | Lädt Dokument in den DocumentManager hoch | `–` | `–` |

### `prompt_engineering`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `attackCategoryName` | 🟡 UNGENUTZT | Gibt lesbaren Namen einer AdversarialAttackCategory zurück | `–` | `–` |
| `SimpleAdversarialTester` | 🔵 UNDER_REVIEW | Tester-Klasse; kein explizites Roadmap-Signal | `–` | `–` |

### `query`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `executeBroadcastJoin` | 🟡 UNGENUTZT | Führt Broadcast-Join für kleine Lookup-Tabellen aus | `–` | `–` |
| `executeGraceHashJoin` | 🟡 UNGENUTZT | Führt Grace-Hash-Join für große Datenmengen aus (partitioniert) | `–` | `–` |
| `executeHashJoin` | 🟡 UNGENUTZT | Führt Hash-Join-Algorithmus aus (AdaptiveJoin-Strategie) | `–` | `–` |
| `executeIndexNestedLoopJoin` | 🟡 UNGENUTZT | Führt Index-NL-Join aus (nutzt verfügbare Indizes) | `–` | `–` |
| `executeMergeJoin` | 🟡 UNGENUTZT | Führt Sort-Merge-Join-Algorithmus aus | `–` | `–` |
| `executeNestedLoopJoin` | 🟡 UNGENUTZT | Führt Nested-Loop-Join aus (Fallback für kleine Relationen) | `–` | `–` |

### `rag`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `ABTestingFramework` | 🧪 NUR_TESTS | A/B-Testing für RAG-Pipelines (Retrieval-/Ranking-Strategien) | `test_ab_testing_framework.cpp` | `–` |

### `replication`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `mergeFields` | 🟡 UNGENUTZT | Merged einzelne Felder nach konfigurierbarer Merge-Policy | `–` | `–` |
| `mergeJson` | 🟡 UNGENUTZT | Merged zwei JSON-Dokumente bei Replikationskonflikt | `–` | `–` |
| `selectBase` | 🟡 UNGENUTZT | Wählt Basis-Version bei Konflikt-Auflösung (CRDT-Merge) | `–` | `–` |

### `scheduler`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `activateScheduler` | 🟡 UNGENUTZT | Aktiviert verteilten Task-Coordinator (startet Heartbeat + Worker-Loop) | `–` | `–` |
| `deactivateScheduler` | 🟡 UNGENUTZT | Deaktiviert den Coordinator graceful (drainiert pending Tasks) | `–` | `–` |

### `security`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `AccessControl` | ✅ AKTIV | ABAC-Zugriffssteuerung; genutzt in compliance_reporter.cpp + Tests + Bench | `test_access_control.cpp` | `compliance_reporter.cpp` |
| `disableMFA` | 🟡 UNGENUTZT | Deaktiviert MFA für einen User (Admin-Only-Pfad) | `–` | `–` |
| `verifyMFA` | 🟡 UNGENUTZT | Prüft TOTP/FIDO2-MFA-Token für privilegierte Operationen | `–` | `–` |

### `server`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `AdaptiveRateLimiter` | 🧪 NUR_TESTS | Token-Bucket-Ratenlimiter mit dynamischer Anpassung; nur im Test geprüft | `test_rate_limiting_improvements.cpp` | `–` |
| `AdminApiHandler` | 🔵 UNDER_REVIEW | Admin-API; Nutzung ops-abhängig | `–` | `–` |
| `computeErrorRate` | ⚪ INTERNAL_ONLY | Berechnet Error-Rate intern | `–` | `–` |
| `computeP99` | ⚪ INTERNAL_ONLY | Berechnet p99-Latenzen intern | `–` | `–` |
| `pruneAndAdapt` | ⚪ INTERNAL_ONLY | Internes Hilfssymbol | `–` | `–` |

### `stable_diffusion`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `SDPlugin` | 🧪 NUR_TESTS | Stable-Diffusion-Plugin-Implementierung; Tests + Plugin-Registrar vorhanden | `test_sd_plugin.cpp` | `–` |
| `samplerFromString` | 🟡 UNGENUTZT | Parst Sampler-Namen (euler, ddim, …) zu Enum; Header-only Helper | `–` | `–` |
| `free_sd_ctx` | 🔵 UNDER_REVIEW | sd_ctx Cleanup-Funktion; Verknüpfung unklar | `–` | `–` |

### `storage`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `AdaptiveCompactionScheduler` | 🧪 NUR_TESTS | Plant und steuert RocksDB-Compactions adaptiv; Tests vorhanden | `test_adaptive_compaction.cpp` | `–` |

### `temporal`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `BiTemporalTable` | 🧪 NUR_TESTS | Bi-temporale Tabelle (valid-time + transaction-time); Tests + Bench vorhanden | `test_bi_temporal.cpp` | `–` |

### `timeseries`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `watermarkReached` | 🟡 UNGENUTZT | Prüft ob Watermark überschritten (triggert Flush/Window-Close) | `–` | `–` |
| `watermarkThreshold` | 🟡 UNGENUTZT | Konfiguriert Watermark-Threshold für Late-Arrival-Handling | `–` | `–` |

### `toolbox`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `IngestionToolbox` | ✅ AKTIV | Haupt-Toolbox für Ingestion-Pipelines; genutzt in RAG- und AQL-Bridge | `test_toolbox_ingestion.cpp` | `rag_ingestion_bridge.cpp, aql_ingestion_bridge.cpp` |
| `contentManager` | 🟡 UNGENUTZT | Gibt den ContentManager aus der ContentToolboxBridge zurück | `–` | `–` |
| `enrichExisting` | 🟡 UNGENUTZT | Reichert existierende Entitäten mit zusätzlichen Extraktionen an | `–` | `–` |

### `training`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `AdaLoRAAdapter` | 🧪 NUR_TESTS | AdaLoRA-Adapter für Parameter-effizientes Fine-Tuning; Tests vorhanden | `test_ada_lora_adapter.cpp` | `–` |

### `transaction`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `BranchManager` | ✅ AKTIV | Verwaltet Branch-Transaktionen; genutzt in BranchApiHandler + HttpServer | `test_branch_manager.cpp` | `branch_api_handler.cpp, http_server.cpp` |

### `user_storage_encrypted`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `GocryptfsBackend` | ✅ AKTIV | Backend-Implementierung für Gocryptfs-verschlüsselten Storage | `test_user_storage_v03.cpp` | `gocryptfs_backend.cpp` |
| `createContainer` | ✅ AKTIV | Erstellt verschlüsselten Gocryptfs-Container; Plugin-Tests + impl vorhanden | `test_user_storage_features.cpp` | `gocryptfs_backend.cpp` |
| `isMounted` | ✅ AKTIV | Prüft ob Container gemountet; genutzt in usb_volume_hardening.cpp | `test_usb_volume_hardening.cpp` | `usb_volume_hardening.cpp` |
| `mountContainer` | ✅ AKTIV | Mounted einen Gocryptfs-Container; Tests + Bench vorhanden | `test_user_storage_features.cpp` | `gocryptfs_backend.cpp` |
| `unmountContainer` | ✅ AKTIV | Unmountet einen Container; Bench vorhanden | `bench_user_storage_mount_latency.cpp` | `gocryptfs_backend.cpp` |

### `utils`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `AuditLogger` | ✅ AKTIV | Schreibt Audit-Events für Compliance; genutzt in policy_engine + http_server + export_api | `test_chunk_level_encryption.cpp` | `export_api_handler.cpp, policy_engine.cpp` |

### `voice`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `NoiseSuppressor` | 🧪 NUR_TESTS | RNNoise-basierte Rauschunterdrückung; nur im Voice-Produktionstest geprüft | `test_voice_production.cpp` | `–` |
| `applyRNNoiseSuppression` | 🟡 UNGENUTZT | Wendet RNNoise auf gesamten Audio-Buffer an | `–` | `–` |
| `processRNNoiseFrames` | 🟡 UNGENUTZT | Verarbeitet Audio-Frames durch RNNoise-Modell | `–` | `–` |
| `resampleLinear` | ⚪ INTERNAL_ONLY | Internes Audio-Resample-Helper | `–` | `–` |

### `whisper`

| Symbol | Status | Use-Case | Tests | Externe Aufrufer |
|--------|--------|----------|-------|-----------------|
| `WhisperPlugin` | 🧪 NUR_TESTS | Whisper-ASR-Plugin-Implementierung; Tests + Bench vorhanden | `test_whisper_plugin.cpp` | `–` |
| `addReader` | 🧪 NUR_TESTS | Registriert einen Audio-Reader für den Whisper-Plugin-Stack | `test_whisper_plugin.cpp` | `–` |
| `canRead` | 🧪 NUR_TESTS | Prüft ob Whisper-Plugin einen Audio-Chunk lesen kann | `test_whisper_plugin.cpp` | `–` |
| `parseWav` | 🟡 UNGENUTZT | Parsed WAV-Header und extrahiert Audio-Rohdaten | `–` | `–` |
| `shellEscape` | ⚪ INTERNAL_ONLY | Internes Shell-Escape-Helper | `–` | `–` |
