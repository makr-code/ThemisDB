# Unused Functions Report

_Generated: 2026-04-20 10:51:55Z (heuristische statische Analyse + ROADMAP/FUTURE_ENHANCEMENTS Signale)_

Dieses Dokument benennt alle **nicht extern genutzten Funktionen/Symbole** pro Modul in `src/`  
und liefert eine **Prognose**, ob eine zukünftige Nutzung vorgesehen ist.

Ausgangsbasis: [`MODULE_FUNCTION_USAGE_MAP.md`](./MODULE_FUNCTION_USAGE_MAP.md)

## Prognose-Legende

| Symbol | Bedeutung |
|--------|-----------|
| 🟡 `PLANNED` | Zukünftige Nutzung vorgesehen – ROADMAP/FUTURE_ENHANCEMENTS enthält konkretes Signal |
| 🔵 `UNDER_REVIEW` | Ungeklärt – Symbol ist bekannt, aber kein klares Milestonesignal |
| ⚪ `INTERNAL_ONLY` | Internes Hilfssymbol – kein externer Aufruf erwartet (private Helper) |
| 🔴 `CANDIDATE_FOR_REMOVAL` | Entfernung empfohlen – kein Planungssignal, keine Nutzung nachgewiesen |

## Statistik

- **Gesamt:** 112 ungenutzte Symbole in 47 Modulen
- **⚪ INTERNAL_ONLY:** 12
- **🔴 CANDIDATE_FOR_REMOVAL:** 0
- **🔵 UNDER_REVIEW:** 4
- **🟡 PLANNED:** 96

## Soforthandlungsbedarf

Symbole mit `🔴 CANDIDATE_FOR_REMOVAL` sollten kurzfristig in den jeweiligen  
Modul-`ROADMAP.md`/`FUTURE_ENHANCEMENTS.md` als explizite Entscheidung (keep/remove) festgehalten werden.

---

## Detail-Report pro Modul

### `acceleration`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `logCapabilities` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |

### `api`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `hookId` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `registerHook` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `unregisterHook` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `getHooks` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |

### `aql`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `ReActAgent` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |

### `auth`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `ApiKeyAuthenticator` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `constantTimeEqual` | 🟡 PLANNED | Security-API; Nutzung durch Security-Consumer bei Feature-Aktivierung |

### `base`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `statusFromString` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `configToJson` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `configFromJson` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |

### `cache`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `AdaptiveQueryCache` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |

### `cdc`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `CDCAdmin` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |
| `purgeTenant` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |

### `chaos`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `FaultInjector` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |

### `content`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `detectorType` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `PhotoDNAAbuseDetector` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |
| `TextAbuseDetector` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |
| `createPdfExtractorAdapter` | 🟡 PLANNED | Plugin-Registrar-Muster; Modul hat aktive Roadmap mit Erweiterungsplan |

### `distributed_knowledge`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `GossipAdapterPublisher` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |

### `ethics_ai`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `EthicsAIPlugin` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |
| `strengthToScore` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |

### `exporters`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `AqlPredicateFilter` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `exportWithArrow` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `exportFallback` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |

### `geo`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `GeoFaissKnn` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |
| `knnSearch` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |

### `governance`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `CcpaRuleSet` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |

### `gpu`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `MakeCPUFallback` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `EnumerateCUDA` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `EnumerateROCm` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `resolveDevices` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |

### `graph`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `LocalShardGraphExecutor` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |
| `qualify` | 🔵 UNDER_REVIEW | Modul hat aktive Roadmap; Verknüpfung zum Symbol nicht explizit |

### `importers`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `computeEventHash` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |

### `index`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `QueryPatternTracker` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |

### `ingestion`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `AgenticReferenceValidator` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |

### `llama_cpp`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `LlamaCppPlugin` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |

### `llm`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `ActiveVRAMAllocator` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |

### `maintenance`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `DatabaseMaintenanceOrchestrator` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |

### `metadata`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `CatalogExporter` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `buildAtlasPayload` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `sendToAtlas` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `buildDataHubProposals` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `sendToDataHub` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |

### `network`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `AdaptiveCircuitBreaker` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |

### `onnx_clip`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `sha256HexOfFile` | ⚪ INTERNAL_ONLY | Internes Hilfssymbol (Namensschema); kein öffentliches API-Symbol erwartet |
| `fnv1a64_str` | ⚪ INTERNAL_ONLY | Internes Hilfssymbol (Namensschema); kein öffentliches API-Symbol erwartet |
| `mixMetadata` | ⚪ INTERNAL_ONLY | Internes Hilfssymbol (Namensschema); kein öffentliches API-Symbol erwartet |
| `nextFloat01` | ⚪ INTERNAL_ONLY | Internes Hilfssymbol (Namensschema); kein öffentliches API-Symbol erwartet |
| `computeEmbedding` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |

### `performance`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `AdaptiveQueryCompiler` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |

### `plugins`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `AIPluginGenerator` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `generatePlugin` | 🟡 PLANNED | Plugin-Registrar-Muster; Modul hat aktive Roadmap mit Erweiterungsplan |

### `process`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `importFile` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `exportFromJson` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `escapeXml_` | ⚪ INTERNAL_ONLY | Internes Hilfssymbol (Namensschema); kein öffentliches API-Symbol erwartet |
| `nodeTypeToXmlTag_` | ⚪ INTERNAL_ONLY | Internes Hilfssymbol (Namensschema); kein öffentliches API-Symbol erwartet |
| `xmlTagToNodeType_` | ⚪ INTERNAL_ONLY | Internes Hilfssymbol (Namensschema); kein öffentliches API-Symbol erwartet |

### `projects`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `DocumentManager` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `uploadDocument` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `getDocumentBlob` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `getDocumentChunks` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |

### `prompt_engineering`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `attackCategoryName` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `SimpleAdversarialTester` | 🔵 UNDER_REVIEW | Modul hat aktive Roadmap; Verknüpfung zum Symbol nicht explizit |

### `query`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `executeHashJoin` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `executeMergeJoin` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `executeNestedLoopJoin` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `executeIndexNestedLoopJoin` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `executeGraceHashJoin` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `executeBroadcastJoin` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |

### `rag`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `ABTestingFramework` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |

### `replication`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `selectBase` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `mergeJson` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `mergeFields` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |

### `scheduler`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `activateScheduler` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `deactivateScheduler` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |

### `security`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `AccessControl` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |
| `verifyMFA` | 🟡 PLANNED | Security-API; Nutzung durch Security-Consumer bei Feature-Aktivierung |
| `disableMFA` | 🟡 PLANNED | Security-API; Nutzung durch Security-Consumer bei Feature-Aktivierung |

### `server`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `AdaptiveRateLimiter` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `pruneAndAdapt` | ⚪ INTERNAL_ONLY | Internes Hilfssymbol (Namensschema); kein öffentliches API-Symbol erwartet |
| `computeP99` | ⚪ INTERNAL_ONLY | Internes Hilfssymbol (Namensschema); kein öffentliches API-Symbol erwartet |
| `computeErrorRate` | ⚪ INTERNAL_ONLY | Internes Hilfssymbol (Namensschema); kein öffentliches API-Symbol erwartet |
| `AdminApiHandler` | 🔵 UNDER_REVIEW | Admin-API; Nutzung ops-workflow-abhängig (nicht immer im Build aktiv) |

### `stable_diffusion`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `free_sd_ctx` | 🔵 UNDER_REVIEW | Modul hat aktive Roadmap; Verknüpfung zum Symbol nicht explizit |
| `samplerFromString` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `SDPlugin` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |

### `storage`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `AdaptiveCompactionScheduler` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |

### `temporal`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `BiTemporalTable` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |

### `timeseries`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `watermarkThreshold` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `watermarkReached` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |

### `toolbox`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `enrichExisting` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |
| `contentManager` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |
| `IngestionToolbox` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |

### `training`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `AdaLoRAAdapter` | 🟡 PLANNED | Plugin-Registrar-Muster; Modul hat aktive Roadmap mit Erweiterungsplan |

### `transaction`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `BranchManager` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |

### `user_storage_encrypted`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `createContainer` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `mountContainer` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |
| `unmountContainer` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |
| `isMounted` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |
| `GocryptfsBackend` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |

### `utils`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `AuditLogger` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |

### `voice`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `NoiseSuppressor` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `resampleLinear` | ⚪ INTERNAL_ONLY | Internes Hilfssymbol (Namensschema); kein öffentliches API-Symbol erwartet |
| `processRNNoiseFrames` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `applyRNNoiseSuppression` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |

### `whisper`

| Symbol | Prognose | Begründung |
|--------|----------|------------|
| `canRead` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |
| `parseWav` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `shellEscape` | ⚪ INTERNAL_ONLY | Internes Hilfssymbol (Namensschema); kein öffentliches API-Symbol erwartet |
| `addReader` | 🟡 PLANNED | Konzept in ROADMAP/FUTURE_ENHANCEMENTS erwähnt; Planungssignale vorhanden |
| `WhisperPlugin` | 🟡 PLANNED | Direkterwähnung im Modul-ROADMAP/FUTURE_ENHANCEMENTS mit Planungssignal |
