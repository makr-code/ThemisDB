# Unused Functions Report

_Erstellt: 2026-04-20 10:51:55Z · Letzte Quellcode-Verifikation: 2026-04-20 11:04:49Z_

Dieses Dokument benennt alle **Symbole ohne nachgewiesene externe Aufrufer** pro Modul in `src/`.  
Jedes Symbol wurde durch direkten Quellcode-Grep (`src/` + `include/`) verifiziert.

Ausgangsbasis: [`MODULE_FUNCTION_USAGE_MAP.md`](./MODULE_FUNCTION_USAGE_MAP.md)

## Prognose-Legende

| Symbol | Bedeutung |
|--------|-----------|
| 🟢 `IMPLEMENTIERT` | Symbol **existiert im Quellcode**, kein externer Aufrufer in `src/`+`include/` gefunden |
| ⚠️ `VERALTET` | Symbol **existiert und wird bereits extern genutzt** – Eintrag in MODULE_FUNCTION_USAGE_MAP war veraltet |
| 🔵 `UNDER_REVIEW` | Ungeklärt – Symbol ist bekannt, aber kein klares Milestonesignal und kein bestätigter Aufrufer |
| ⚪ `INTERNAL_ONLY` | Internes Hilfssymbol – kein externer Aufruf erwartet (private Helper) |
| 🔴 `CANDIDATE_FOR_REMOVAL` | Entfernung empfohlen – kein Planungssignal, keine Nutzung nachgewiesen |

> **Wichtig:** Das Label `🟡 PLANNED` aus der vorherigen Version dieses Dokuments war irreführend,
> da es suggerierte, die Symbole seien noch nicht implementiert. Alle 96 Symbole existieren
> im Quellcode (verifiziert am 2026-04-20). Die `PLANNED`-Einträge wurden entsprechend korrigiert.

## Statistik

- **Gesamt geprüfte Symbole:** 112 in 47 Modulen
- **🟢 IMPLEMENTIERT** (vorhanden, kein externer Aufrufer): 88
- **⚠️ VERALTET** (vorhanden, externe Nutzung nachgewiesen – Dokumentationsfehler): 8
- **⚪ INTERNAL_ONLY** (interne Hilfsfunktionen): 12
- **🔵 UNDER_REVIEW** (ungeklärt): 4
- **🔴 CANDIDATE_FOR_REMOVAL**: 0

## Handlungsbedarf

### ⚠️ Sofortbedarf: Veraltete Einträge in `MODULE_FUNCTION_USAGE_MAP.md` korrigieren

Die folgenden Symbole wurden im Nutzungsmap als "kein externer Aufrufer" markiert,
sind aber nachweislich extern eingesetzt. Der Map-Eintrag wird korrigiert:

| Modul | Symbol | Bestätigte externe Call-Site(s) |
|-------|--------|----------------------------------|
| `auth` | `ApiKeyAuthenticator` | `src/server/auth_middleware.cpp:103,108` |
| `cache` | `AdaptiveQueryCache` | `src/server/cache_admin_api_handler.cpp:85, src/server/http_server.cpp:355` |
| `cdc` | `CDCAdmin` | `src/server/changefeed_api_handler.cpp:709,743` |
| `exporters` | `AqlPredicateFilter` | `src/server/export_api_handler.cpp:122` |
| `maintenance` | `DatabaseMaintenanceOrchestrator` | `src/server/http_server.cpp:1182` |
| `toolbox` | `IngestionToolbox` | `src/rag/rag_ingestion_bridge.cpp:32, src/aql/aql_ingestion_bridge.cpp:27` |
| `transaction` | `BranchManager` | `src/server/branch_api_handler.cpp:31, src/server/http_server.cpp:470` |
| `utils` | `AuditLogger` | `src/server/policy_engine.cpp:38,203, src/server/http_server.cpp:741` |

### 🟢 Langfristig: Implementierte aber ungenutzte Symbole (kein Handlungsdruck)

Diese Symbole sind implementiert, werden aber (noch) nicht extern konsumiert.
Handlungsbedarf besteht nur wenn sie dauerhaft ungenutzt bleiben (dann `CANDIDATE_FOR_REMOVAL`).

---

## Detail-Report pro Modul

### `acceleration`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `logCapabilities` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `api`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `getHooks` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `hookId` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `registerHook` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `unregisterHook` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `aql`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `ReActAgent` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `auth`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `ApiKeyAuthenticator` | ⚠️ VERALTET | Symbol ist implementiert und wird extern genutzt – Quellenachweise: src/server/auth_middleware.cpp:103,108 |
| `constantTimeEqual` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `base`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `configFromJson` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `configToJson` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `statusFromString` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `cache`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `AdaptiveQueryCache` | ⚠️ VERALTET | Symbol ist implementiert und wird extern genutzt – Quellenachweise: src/server/cache_admin_api_handler.cpp:85, src/server/http_server.cpp:355 |

### `cdc`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `CDCAdmin` | ⚠️ VERALTET | Symbol ist implementiert und wird extern genutzt – Quellenachweise: src/server/changefeed_api_handler.cpp:709,743 |
| `purgeTenant` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `chaos`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `FaultInjector` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `content`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `PhotoDNAAbuseDetector` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `TextAbuseDetector` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `createPdfExtractorAdapter` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `detectorType` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `distributed_knowledge`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `GossipAdapterPublisher` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `ethics_ai`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `EthicsAIPlugin` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `strengthToScore` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `exporters`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `AqlPredicateFilter` | ⚠️ VERALTET | Symbol ist implementiert und wird extern genutzt – Quellenachweise: src/server/export_api_handler.cpp:122 |
| `exportFallback` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `exportWithArrow` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `geo`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `GeoFaissKnn` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `knnSearch` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `governance`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `CcpaRuleSet` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `gpu`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `EnumerateCUDA` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `EnumerateROCm` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `MakeCPUFallback` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `resolveDevices` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `graph`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `LocalShardGraphExecutor` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `qualify` | 🔵 UNDER_REVIEW | Modul hat aktive Roadmap; Verknüpfung zum Symbol nicht explizit |

### `importers`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `computeEventHash` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `index`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `QueryPatternTracker` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `ingestion`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `AgenticReferenceValidator` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `llama_cpp`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `LlamaCppPlugin` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `llm`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `ActiveVRAMAllocator` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `maintenance`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `DatabaseMaintenanceOrchestrator` | ⚠️ VERALTET | Symbol ist implementiert und wird extern genutzt – Quellenachweise: src/server/http_server.cpp:1182 |

### `metadata`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `CatalogExporter` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `buildAtlasPayload` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `buildDataHubProposals` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `sendToAtlas` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `sendToDataHub` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `network`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `AdaptiveCircuitBreaker` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `onnx_clip`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `computeEmbedding` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `fnv1a64_str` | ⚪ INTERNAL_ONLY | Internes Hilfssymbol (Namensschema); kein öffentliches API-Symbol erwartet |
| `mixMetadata` | ⚪ INTERNAL_ONLY | Internes Hilfssymbol (Namensschema); kein öffentliches API-Symbol erwartet |
| `nextFloat01` | ⚪ INTERNAL_ONLY | Internes Hilfssymbol (Namensschema); kein öffentliches API-Symbol erwartet |
| `sha256HexOfFile` | ⚪ INTERNAL_ONLY | Internes Hilfssymbol (Namensschema); kein öffentliches API-Symbol erwartet |

### `performance`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `AdaptiveQueryCompiler` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `plugins`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `AIPluginGenerator` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `generatePlugin` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `process`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `escapeXml_` | ⚪ INTERNAL_ONLY | Internes Hilfssymbol (Namensschema); kein öffentliches API-Symbol erwartet |
| `exportFromJson` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `importFile` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `nodeTypeToXmlTag_` | ⚪ INTERNAL_ONLY | Internes Hilfssymbol (Namensschema); kein öffentliches API-Symbol erwartet |
| `xmlTagToNodeType_` | ⚪ INTERNAL_ONLY | Internes Hilfssymbol (Namensschema); kein öffentliches API-Symbol erwartet |

### `projects`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `DocumentManager` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `getDocumentBlob` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `getDocumentChunks` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `uploadDocument` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `prompt_engineering`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `SimpleAdversarialTester` | 🔵 UNDER_REVIEW | Modul hat aktive Roadmap; Verknüpfung zum Symbol nicht explizit |
| `attackCategoryName` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `query`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `executeBroadcastJoin` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `executeGraceHashJoin` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `executeHashJoin` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `executeIndexNestedLoopJoin` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `executeMergeJoin` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `executeNestedLoopJoin` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `rag`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `ABTestingFramework` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `replication`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `mergeFields` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `mergeJson` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `selectBase` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `scheduler`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `activateScheduler` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `deactivateScheduler` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `security`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `AccessControl` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `disableMFA` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `verifyMFA` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `server`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `AdaptiveRateLimiter` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `AdminApiHandler` | 🔵 UNDER_REVIEW | Admin-API; Nutzung ops-workflow-abhängig (nicht immer im Build aktiv) |
| `computeErrorRate` | ⚪ INTERNAL_ONLY | Internes Hilfssymbol (Namensschema); kein öffentliches API-Symbol erwartet |
| `computeP99` | ⚪ INTERNAL_ONLY | Internes Hilfssymbol (Namensschema); kein öffentliches API-Symbol erwartet |
| `pruneAndAdapt` | ⚪ INTERNAL_ONLY | Internes Hilfssymbol (Namensschema); kein öffentliches API-Symbol erwartet |

### `stable_diffusion`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `SDPlugin` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `free_sd_ctx` | 🔵 UNDER_REVIEW | Modul hat aktive Roadmap; Verknüpfung zum Symbol nicht explizit |
| `samplerFromString` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `storage`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `AdaptiveCompactionScheduler` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `temporal`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `BiTemporalTable` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `timeseries`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `watermarkReached` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `watermarkThreshold` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `toolbox`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `IngestionToolbox` | ⚠️ VERALTET | Symbol ist implementiert und wird extern genutzt – Quellenachweise: src/rag/rag_ingestion_bridge.cpp:32, src/aql/aql_ingestion_bridge.cpp:27 |
| `contentManager` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `enrichExisting` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `training`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `AdaLoRAAdapter` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `transaction`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `BranchManager` | ⚠️ VERALTET | Symbol ist implementiert und wird extern genutzt – Quellenachweise: src/server/branch_api_handler.cpp:31, src/server/http_server.cpp:470 |

### `user_storage_encrypted`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `GocryptfsBackend` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `createContainer` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `isMounted` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `mountContainer` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `unmountContainer` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |

### `utils`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `AuditLogger` | ⚠️ VERALTET | Symbol ist implementiert und wird extern genutzt – Quellenachweise: src/server/policy_engine.cpp:38,203, src/server/http_server.cpp:741 |

### `voice`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `NoiseSuppressor` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `applyRNNoiseSuppression` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `processRNNoiseFrames` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `resampleLinear` | ⚪ INTERNAL_ONLY | Internes Hilfssymbol (Namensschema); kein öffentliches API-Symbol erwartet |

### `whisper`

| Symbol | Status | Begründung |
|--------|--------|------------|
| `WhisperPlugin` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `addReader` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `canRead` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `parseWav` | 🟢 IMPLEMENTIERT | Symbol in Modul-Quellcode vorhanden; kein externer Aufrufer im Scope src/+include/ gefunden |
| `shellEscape` | ⚪ INTERNAL_ONLY | Internes Hilfssymbol (Namensschema); kein öffentliches API-Symbol erwartet |
