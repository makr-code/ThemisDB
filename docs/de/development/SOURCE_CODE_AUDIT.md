# ThemisDB Source-Code-zu-Dokumentation Audit

**Stand:** 6. April 2026
**Generiert aus:** Source-Code Analyse

---

## Komponenten-Übersicht

| Komponente | Header | Source | LOC | Klassen | Doku | Status |
|------------|--------|--------|-----|---------|------|--------|
| analytics | 3 | 2 | 3742 | 4 | ✅ | ✅ OK |
| cache | 6 | 1 | 492 | 3 | ✅ | ✅ OK |
| cdc | 1 | 1 | 510 | 2 | ✅ | ✅ OK |
| content | 16 | 15 | 9091 | 6 | ✅ | ✅ OK |
| geo | 2 | 3 | 304 | 3 | ✅ | ✅ OK |
| governance | 1 | 1 | 259 | 2 | ✅ | ✅ OK |
| index | 12 | 11 | 14629 | 7 | ✅ | ✅ OK |
| llm | 2 | 2 | 679 | 2 | ✅ | ✅ OK |
| query | 12 | 12 | 12560 | 7 | ✅ | ✅ OK |
| replication | 2 | 1 | 1612 | 6 | ✅ | ✅ OK |
| security | 16 | 16 | 8138 | 9 | ✅ | ✅ OK |
| server | 20 | 20 | 18282 | 10 | ✅ | ✅ OK |
| sharding | 21 | 19 | 12278 | 12 | ✅ | ✅ OK |
| storage | 9 | 10 | 4591 | 4 | ✅ | ✅ OK |
| timeseries | 7 | 8 | 2767 | 5 | ✅ | ✅ OK |
| transaction | 2 | 2 | 895 | 3 | ✅ | ✅ OK |

**Gesamt:** 132 Header, 124 Sources, 90,829 Zeilen

---

## Detaillierte Komponenten-Analyse

### SERVER

**Beschreibung:** HTTP Server with REST API handlers

**Implementierung:**
- Header: `include/server/` (20 Dateien, 2623 Zeilen)
- Source: `src/server/` (20 Dateien, 15659 Zeilen)

**Hauptklassen:**
- `HttpServer`
- `KeysApiHandler`
- `SAGAApiHandler`
- `ReportsApiHandler`
- `ClassificationApiHandler`
- `AuditApiHandler`
- `PIIApiHandler`
- `RetentionApiHandler`
- `RangerAdapter`
- `SSEConnectionManager`

**Dokumentation:** `docs/server/README.md`

### INDEX

**Beschreibung:** Secondary, vector, graph, and fulltext indexes

**Implementierung:**
- Header: `include/index/` (12 Dateien, 3329 Zeilen)
- Source: `src/index/` (11 Dateien, 11300 Zeilen)

**Hauptklassen:**
- `SecondaryIndexManager`
- `VectorIndexManager`
- `GraphIndexManager`
- `FulltextIndex`
- `PropertyGraph`
- `AdaptiveIndex`
- `GNNEmbeddings`

**Dokumentation:** `docs/index/README.md`

### QUERY

**Beschreibung:** AQL parser, query engine, optimizer, and semantic cache

**Implementierung:**
- Header: `include/query/` (12 Dateien, 2927 Zeilen)
- Source: `src/query/` (12 Dateien, 9633 Zeilen)

**Hauptklassen:**
- `AQLParser`
- `AQLTranslator`
- `QueryEngine`
- `QueryOptimizer`
- `SemanticCache`
- `WindowEvaluator`
- `SubqueryEngine`

**Dokumentation:** `docs/aql/README.md`

### SHARDING

**Beschreibung:** Horizontal scaling with consistent hashing and auto-rebalancing

**Implementierung:**
- Header: `include/sharding/` (21 Dateien, 5171 Zeilen)
- Source: `src/sharding/` (19 Dateien, 7107 Zeilen)

**Hauptklassen:**
- `ShardTopology`
- `ConsistentHashRing`
- `DataMigrator`
- `AutoRebalancer`
- `GossipProtocol`
- `MTLSClient`
- `ShardRouter`
- `URN`
- `HealthCheck`
- `ShardLoadDetector`
- `StreamProtocol`
- `BackpressureProtocol`

**Dokumentation:** `docs/sharding/sharding_overview.md`

### CONTENT

**Beschreibung:** Content ingestion pipeline with processors

**Implementierung:**
- Header: `include/content/` (16 Dateien, 2389 Zeilen)
- Source: `src/content/` (15 Dateien, 6702 Zeilen)

**Hauptklassen:**
- `ContentManager`
- `ContentTypeRegistry`
- `TextProcessor`
- `PDFProcessor`
- `OfficeProcessor`
- `ImageProcessor`

**Dokumentation:** `docs/content/README.md`

### SECURITY

**Beschreibung:** Field-level encryption, RBAC, PKI, and HSM support

**Implementierung:**
- Header: `include/security/` (16 Dateien, 2585 Zeilen)
- Source: `src/security/` (16 Dateien, 5553 Zeilen)

**Hauptklassen:**
- `FieldEncryption`
- `KeyProvider`
- `MockKeyProvider`
- `HSMProvider`
- `VaultKeyProvider`
- `PKIKeyProvider`
- `RBAC`
- `CMSSigning`
- `MalwareScanner`

**Dokumentation:** `docs/security/security_overview.md`

### STORAGE

**Beschreibung:** RocksDB wrapper and entity storage

**Implementierung:**
- Header: `include/storage/` (9 Dateien, 1606 Zeilen)
- Source: `src/storage/` (10 Dateien, 2985 Zeilen)

**Hauptklassen:**
- `RocksDBWrapper`
- `BaseEntity`
- `BlobRedundancyManager`
- `KeySchema`

**Dokumentation:** `docs/storage/README.md`

### ANALYTICS

**Beschreibung:** OLAP engine with CUBE, ROLLUP, and CEP streaming

**Implementierung:**
- Header: `include/analytics/` (3 Dateien, 2044 Zeilen)
- Source: `src/analytics/` (2 Dateien, 1698 Zeilen)

**Hauptklassen:**
- `OLAPEngine`
- `ColumnarStore`
- `CEPEngine`
- `WindowProcessor`

**Dokumentation:** `docs/analytics/README.md`

### TIMESERIES

**Beschreibung:** Time series with Gorilla compression and continuous aggregates

**Implementierung:**
- Header: `include/timeseries/` (7 Dateien, 803 Zeilen)
- Source: `src/timeseries/` (8 Dateien, 1964 Zeilen)

**Hauptklassen:**
- `TimeSeriesStore`
- `GorillaEncoder`
- `GorillaDecoder`
- `ContinuousAggregateManager`
- `RetentionManager`

**Dokumentation:** `docs/timeseries/README.md`

### REPLICATION

**Beschreibung:** Leader-Follower and Multi-Master replication with CRDTs

**Implementierung:**
- Header: `include/replication/` (2 Dateien, 891 Zeilen)
- Source: `src/replication/` (1 Dateien, 721 Zeilen)

**Hauptklassen:**
- `ReplicationManager`
- `MultiMasterReplication`
- `VectorClock`
- `HybridLogicalClock`
- `ConflictResolver`
- `CRDTMerger`

**Dokumentation:** `docs/replication/README.md`

### TRANSACTION

**Beschreibung:** ACID transactions with MVCC and SAGA patterns

**Implementierung:**
- Header: `include/transaction/` (2 Dateien, 271 Zeilen)
- Source: `src/transaction/` (2 Dateien, 624 Zeilen)

**Hauptklassen:**
- `TransactionManager`
- `Transaction`
- `SAGACoordinator`

**Dokumentation:** `docs/transaction/README.md`

### LLM

**Beschreibung:** LLM interaction store and prompt management

**Implementierung:**
- Header: `include/llm/` (2 Dateien, 198 Zeilen)
- Source: `src/llm/` (2 Dateien, 481 Zeilen)

**Hauptklassen:**
- `LLMInteractionStore`
- `PromptManager`

**Dokumentation:** `docs/llm/README.md`

### CDC

**Beschreibung:** Change Data Capture with event streaming

**Implementierung:**
- Header: `include/cdc/` (1 Dateien, 137 Zeilen)
- Source: `src/cdc/` (1 Dateien, 373 Zeilen)

**Hauptklassen:**
- `Changefeed`
- `ChangeEvent`

**Dokumentation:** `docs/features/features_cdc.md`

### CACHE

**Beschreibung:** Semantic query cache with LRU eviction

**Implementierung:**
- Header: `include/cache/` (6 Dateien, 244 Zeilen)
- Source: `src/cache/` (1 Dateien, 248 Zeilen)

**Hauptklassen:**
- `SemanticCache`
- `ResultCache`
- `CacheProvider`

**Dokumentation:** `docs/cache/README.md`

### GEO

**Beschreibung:** Geo-spatial operations with GPU acceleration

**Implementierung:**
- Header: `include/geo/` (2 Dateien, 72 Zeilen)
- Source: `src/geo/` (3 Dateien, 232 Zeilen)

**Hauptklassen:**
- `SpatialComputeBackend`
- `GeoRegistry`
- `GeoOpsExtension`

**Dokumentation:** `docs/geo/README.md`

### GOVERNANCE

**Beschreibung:** Data governance and classification policies

**Implementierung:**
- Header: `include/governance/` (1 Dateien, 74 Zeilen)
- Source: `src/governance/` (1 Dateien, 185 Zeilen)

**Hauptklassen:**
- `PolicyEngine`
- `ClassificationProfile`

**Dokumentation:** `docs/governance/README.md`

