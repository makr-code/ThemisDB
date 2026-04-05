# ThemisDB v1.8.1-rc1 — Release Notes

**Release Date:** 2026-04-04  
**Type:** Release Candidate  
**Previous Version:** v1.8.0  
**Milestone:** v1.8.1  

---

## 🎯 Übersicht

ThemisDB v1.8.1-rc1 ist ein Geo-, Search- und Storage-Hardening-Release mit 16 Feature-Bereichen.
Schwerpunkte sind die vollständige RFC 7946 GeoJSON-Unterstützung (alle 7 Geometrietypen) mit
R-tree-Spatialindex und ST_UNION/ST_DIFFERENCE-Operationen, sieben neue Search-Module (QueryExpander,
FuzzyMatcher, FacetedSearch, SearchAnalytics, AutocompleteEngine, LearningToRank, MultiModalSearch),
Produktionshärtung des FAISS-Vektorindex mit ADC-Tabellen (~40 % schnellere Suche), eine
Reed-Solomon-Verbesserung auf Vandermonde-Basis (echtes RAID-6 Dual-Parity), eine vollständige
Anti-Entropy-Engine für Shard-Reparatur sowie Git-ähnliche Datenbankfeatures (3-Way Merge,
Point-in-Time Recovery, Named Snapshots). Zusätzlich wird die HSM-Sicherheitswarnung (FIND-002)
eingeführt und die Konfig-Hierarchie in 16 logische Kategorien reorganisiert.

**Produktionsstatus (Stand 2026-04-04):** 44 von 50 Modulen ✅ Production-ready; 3 Beta 🟡; 3 Alpha/in Härtung 🔴/🚧.

> **Post-Publication Update (2026-04-05):** Docker-Image-Security-Patch auf `themisdb/themisdb:latest` und `themisdb/themisdb:1.8.1-rc1` veröffentlicht — CVEs: 39 → 3 (keine CRITICAL/HIGH mehr). Modulstatus nach Beta-Graduation: gpu, process, sharding jetzt ✅ Production-ready (47/50). Siehe [Sicherheitshinweis](#sicherheits-patch-2026-04-05) unten.

---

## ⚠️ Breaking Changes

| # | Modul | Änderung | Migration |
|---|-------|----------|-----------|
| 1 | **geo** | `EWKBParser::parseGeoJSON()` wirft `std::runtime_error` für Koordinaten außerhalb WGS84-Bereich (Lon ∉ [−180,180], Lat ∉ [−90,90]) — bisher stille Akzeptanz | Compile mit `-DTHEMIS_GEO_COMPAT_LAX=1` für einen Release-Zyklus, um altes Verhalten beizubehalten; anschließend Daten normalisieren |
| 2 | **geo** | Unbekannte GeoJSON-Geometrietypen werfen jetzt `std::runtime_error("GeoJSON: unsupported geometry type: <type>")` statt einer leeren Geometrie zurückzugeben | Catch-Blöcke um `parseGeoJSON()`-Aufrufe hinzufügen; ungültige Typen vorab validieren |

---

## 🆕 Neue Features

### Geo-Modul: RFC 7946 Vollständigkeit

**Vollständige GeoJSON-Unterstützung aller 7 Geometrietypen** (`include/geo/ewkb_parser.h`, `src/geo/ewkb_parser.cpp`)

- `EWKBParser::parseGeoJSON()` und `toGeoJSON()` unterstützen alle RFC 7946-Typen: `Point`, `MultiPoint`, `LineString`, `MultiLineString`, `Polygon`, `MultiPolygon`, `GeometryCollection` inkl. 3D-Varianten (Z-Koordinaten)
- EWKB `parse()` und `serialize()` jetzt für Typen 4–7 implementiert
- `GeometryCollection` wird rekursiv bis Tiefe 8 geparst (Schutz vor Stack Overflow bei feindlichem Input)
- `computeMBR()` und `computeCentroid()` rekursieren in verschachtelte Sub-Geometrien
- WGS84-Koordinatenbereichsprüfung: ungültige Koordinaten werfen `std::runtime_error`; compile mit `-DTHEMIS_GEO_COMPAT_LAX` für Legacy-Daten

**In-Memory R-tree Spatialindex** (`include/geo/geo_rtree.h`, `src/geo/geo_rtree.cpp`)

- Neue Klasse `GeoRTree`: In-Memory R-tree für sub-lineare `intersects`/`contains`-Abfragen
- Mit `THEMIS_GEO_BOOST_BACKEND` + Boost.Geometry: `boost::geometry::index::rtree` mit `rstar<16>`-Strategie
- Ohne Boost: automatischer Fallback auf O(n) lineares MBR-Scan (semantisch identisch)
- `bulkLoad()`: STR (Sort-Tile-Recursive) Packing — 3–5× schneller als inkrementelles Einfügen
- `memoryBytes()`: Heap-Schätzung, ins strukturierte Audit-Log geschrieben (`geo_index_bytes_allocated`)
- 20 Unit-Tests (Empty-Index, Insert, BulkLoad, Remove, Clear, Intersects, Contains, Move-Semantics)

**ST_UNION und ST_DIFFERENCE** (`include/geo/spatial_backend.h`)

- Neue virtuelle Methoden `stUnion(geom1, geom2)` und `stDifference(geom1, geom2)` in `ISpatialComputeBackend`
- `CpuExactBackend`: Greiner-Hormann Polygon-Clipping (ACM TOG 1998) mit Fast-Paths für Containment/Disjoint/B-inside-A
- `BoostCpuExactBackend`: via `boost::geometry::union_` / `boost::geometry::difference`; Fallback auf CpuExactBackend für Nicht-Polygon-Typen
- `GpuBatchBackend`: delegiert an `getCpuExactBackend()` mit Audit-Log und Metriken
- AQL-Funktionen `ST_UNION(geom1, geom2)` und `ST_DIFFERENCE(geom1, geom2)` in `include/query/functions/geo_functions.h` registriert
- 15 parametrisierte Unit-Tests + 7 AQL-Level-Tests

---

### Search-Modul v1.5.0 — 7 neue Suchkomponenten

| Klasse | Header | Kernfunktion | Tests |
|--------|--------|--------------|-------|
| `QueryExpander` | `include/search/query_expander.h` | Synonym-Expansion, Levenshtein-Rechtschreibkorrektur, Zero-Result-Relaxation | 28 |
| `FuzzyMatcher` | `include/search/fuzzy_matcher.h` | Levenshtein, Soundex, Metaphone, N-gram (Dice) | 24 |
| `FacetedSearch` | `include/search/faceted_search.h` | Per-Feld Value-Count-Facetten, Range-Bucket-Facetten, Drill-Down-Filter | 20 |
| `SearchAnalytics` | `include/search/search_analytics.h` | Thread-sicheres Query-Event-Log; avg/p95/p99 Latenz, Zero-Result-Rate, Top-20 | 26 |
| `AutocompleteEngine` | `include/search/autocomplete.h` | Prefix-Index + Popular-Queries via `SearchAnalytics`; dedupliziertes Score-Ranking | 18 |
| `LearningToRank` | `include/search/learning_to_rank.h` | 6D Dot-Product Linear-Reranker; Online Pairwise Gradient Descent; A/B Routing | 28 |
| `MultiModalSearch` | `include/search/multi_modal_search.h` | TEXT/IMAGE/AUDIO/CUSTOM Modal-Queries; gewichtetes RRF-Fusion | 18 |

---

### Search-Modul v1.4.0 — HybridSearch Produktionshärtung

- **Konfigurierbares Vektorsimilarity-Maß**: `Config::vector_metric` (COSINE / DOT / L2) — war hardcodiert auf COSINE
- **Strenge Konfigurationsvalidierung**: Constructor wirft `std::invalid_argument` bei k=0, rrf_k≤0, negativen Weights, k>max_k
- **Ressourcenlimits**: `Config::max_k` und `Config::max_candidates` (je default 10.000) begrenzen unbegrenzte Indexscans
- **Score-Normalisierung**: Range=0 → 1.0 (positive Scores) / 0.0 (Zero Scores); BM25 + Vektorscores vor Gewichtung auf [0,1] normalisiert
- **`SearchStats`**: wird bei jedem `search()`-Rückgabewert angehängt (`bm25_ok`, `vector_ok`, `partial_result`, Zähler)
- **Exception Safety**: `search()` fängt Backend-Exceptions, loggt via `THEMIS_ERROR`, gibt leere/Partial-Results zurück
- Tests: `test_hybrid_search.cpp` (35+), `test_rrf_fusion.cpp` (20), `test_score_normalization.cpp` (15), Integration (18)

---

### Sharding: Shard Repair / Anti-Entropy Engine

**Neue Klasse `ShardRepairEngine`** (`include/sharding/shard_repair_engine.h`)

- **Hintergrund-Anti-Entropy-Scan**: Periodisches `checkDocumentHealth()` über alle Shards; degradierte Dokumente werden zur Recovery gequeuet
- **Repair Worker Thread**: Draint Job-Queue via `RedundancyStrategy::recoverDocument()` (RAID-5/6 + Mirror)
- **On-Demand Trigger**: `triggerRepair(shard_id)`, `triggerFullScan()`, `triggerDocumentRepair(doc_id)` — geben trackbare Job-IDs zurück
- **`ShardHealthReport`** pro Shard: Status `HEALTHY` / `DEGRADED` / `FAILED` / `REBUILDING`, Scan- und Repair-Zähler
- **Prometheus-Metriken**: Repair-Events via `PrometheusMetrics` + `ShardingMetricsHandler::getMetrics()`
- **Admin-API**: `POST /admin/repair`, `POST /admin/repair/scan`, `GET /admin/repair/{job_id}`
- `AutoRecoveryManager::setRepairEngine()` delegiert `repairDocument()` an die neue Engine

---

### Storage: Reed-Solomon Verbesserter Erasure-Decoder

- Ersetzt XOR-Only Parity (Single-Chunk Recovery) durch **Vandermonde-Matrix-Codec** über GF(2⁸)
- Ermöglicht echte RAID-6 Dual-Parity-Recovery: bis zu `parity_shards` gleichzeitig verlorene Chunks
- Betrifft `ReedSolomonCoder` und `CauchyReedSolomonCoder`: Beide validieren `missing_indices.size() <= parity_shards`

---

### Index: FAISS ADC-Tabellen (~40 % schnellere Vektorsuche)

- **Asymmetric Distance Computation (ADC) Tables** standardmäßig aktiviert in `AdvancedVectorIndex::Config`
- Vorberechnete Distanztabellen für `IndexIVFPQ` — kein Genauigkeitsverlust (bit-exakte Ergebnisse)
- `polysemous_ht`: Optionale Polysemous Hash Tables für Early Termination (default: 0)
- Konfigurationsoptionen: `use_adc_tables` (default: true), `polysemous_ht`
- Speicher-Overhead: ~1–2 % der Indexgröße; besonders effektiv bei hochdimensionalen Vektoren (>128 dim)

---

### Storage: Write-Amplification-Optimierung

- **Größere Memtables**: Default `memtable_size_mb` 256 MB → **512 MB** (~30–40 % weniger Write-Amplification)
- **Mehr Write Buffers**: Default `max_write_buffer_number` 3 → **6** (reduziert Write-Stalls)
- **`db_write_buffer_size_mb`** default 0 (unbegrenzt) → **2048 MB** (OOM-Schutz beim Betrieb mit vielen Column Families)
- **Async I/O**: `enable_async_io` default false → **true**; `async_io_readahead_size_mb` 64 → **128 MB** (erwartet 2–5× schnellere Sequential Scans)
- `main_server.cpp` zeigt neue Optimization-Einstellungen beim Start an

---

### Sicherheit: HSM-Sicherheitswarnung (FIND-002)

- **Startup-Warnbanner**: Sichtbare ASCII-Box-Warnung wenn Stub-HSM aktiv; `--allow-stub-hsm` Flag für Entwicklung
- **Periodisches ERROR-Logging**: alle 5 Minuten wenn Stub-HSM aktiv
- **Prometheus-Metriken**: `themis_hsm_insecure_config`, `themis_hsm_provider_type`, `hsm_compliance_status{standard="NIST|ISO|PCI|GDPR"}`
- **QUICKSTART.md**: Prominente HSM-Sicherheitswarnung; Verweis auf `docs/security/HSM_PRODUCTION_SETUP.md`
- Löst kritischen Audit-Befund FIND-002 (v1.4.1) — verhindert ungeschützte Master-Encryption-Keys in Produktion

---

### GPU & Vektorindex: Multi-GPU API (v2.4) — Scaffolding

> **Hinweis:** In v2.4 keine tatsächliche GPU-Ausführung — die `MultiGPUVectorIndex`-Klasse verwendet
> CPU-basiertes `GPUVectorIndex` Backend. Echte GPU-Offloading (NCCL/RCCL, P2P) ist für v2.5+ geplant.

- Logische Multi-Device-Unterstützung: 2–8 Devices via Partitionierung (Round-Robin, Hash, Range, Balanced)
- Query Fan-out und zentralisiertes Top-k Merge für aggregierte Partition-Ergebnisse
- `enableMultiGPU`, `deviceIds`, `partitionStrategy` Konfigurationsparameter
- Per-Partition-Statistiken mit Hooks für zukünftige GPU-Metriken (VRAM, Auslastung)
- Unit-Tests (394 Zeilen), Beispielanwendung (237 Zeilen), vollständige API-Dokumentation (`docs/MULTI_GPU_VECTOR_INDEXING.md`)

---

### Git-ähnliche Datenbankfeatures: 3-Way Merge, PITR, Named Snapshots

**SnapshotManager (re-aktiviert):**
- Named Snapshots für MVCC vollständig operational
- 5 REST-Endpunkte für Snapshot/Tag-Verwaltung
- Integration mit `DiffEngine` für tag-basierte Diffs
- Persistente Snapshot-Speicherung in RocksDB

**PITR API Handler:**
- `POST /api/v1/pitr/restore/sequence` — Restore auf Sequence-Nummer
- `POST /api/v1/pitr/restore/tag` — Restore auf Named Snapshot
- `POST /api/v1/pitr/restore/timestamp` — Restore auf Zeitstempel
- `POST /api/v1/pitr/preview` — Dry-Run-Vorschau
- `GET /api/v1/pitr/progress` — Fortschrittsabfrage

**MergeEngine — 3-Way Merge:**
- `POST /api/v1/merge` — Dreiweg-Merge zwischen Sequences
- `POST /api/v1/merge/preview` — Dry-Run-Vorschau
- `POST /api/v1/merge/by-tag` — Merge via Snapshot-Tags
- `GET /api/v1/merge/can-fast-forward` — Fast-Forward-Prüfung
- `BranchManager`: unterstützt jetzt Non-Fast-Forward Merges mit `MergeEngine`-Integration
- Conflict Resolution Strategies: `OURS`, `THEIRS`, `MANUAL`, `FAST_FORWARD`

---

### Config-Architektur: Hierarchische Reorganisation

Alle Config-Dateien wurden in 16 logische Kategorien unterteilt:

| Kategorie | Beschreibung |
|-----------|-------------|
| `config/core/` | Kernsystem (config.yaml, security.yaml, updates.yaml) |
| `config/platform/` | Plattform-Specs (rpi3, rpi4, rpi5, qnap) |
| `config/ai_ml/` | LLM, Vision, LoRA, RAG |
| `config/security/` | RBAC, PII, Kerberos |
| `config/compliance/` | Ethische Richtlinien, Audit, Governance |
| `config/performance/` | Skalierung, Query-Cache, Acceleration |
| `config/data_management/` | Retention, Redundanz, MIME-Typen |
| `config/distributed/` | Replikation, Sharding |
| `config/licensing/` | Community, Enterprise |
| `config/networking/` | Connection Pooling |
| `config/content/` | Content-Prozessoren, Edge-Typen |
| `config/monitoring/` | Prometheus-Metriken |
| `config/features/` | Feature-Flags |
| `config/assistants/` | Docs, Feedback |
| `config/processing/` | CEP-Regeln |
| `config/deprecated/` | Backup/Deprecated |

`ConfigPathResolver` löst Legacy-Pfade automatisch auf veraltete Dateien auf — **keine Breaking Changes** an bestehendem Code.

---

### Query Optimizer v1.5.x: Shard-Schätzungen und Selectivity

- **`DistributedQueryCostModel::getShardRowCount()`**: Ersetzt Hardcode-10K durch dynamische Schätzungen (Hash-Heuristik; MetadataShard-Integration für v1.5.1)
- **`calculatePredicateSelectivity()`**: Analysiert Abfragemuster; Histogramm-Framework; spaltenspezifische Heuristiken (ID 0,1 %, Status 20 %, Namen 5 %)
- **`measureShardLatency()`**: Latency-Integrationshook; Naming-Convention-Heuristik; Prometheus-Integration für v1.5.1
- Tests: `tests/test_optimizer_v1_5_x_integration.cpp`

---

## 🔧 Fixes

| # | Problem | Fix |
|---|---------|-----|
| 1 | `SnapshotManager` wegen unvollständigem Typ deaktiviert | Re-aktiviert nach Typ-Fix |
| 2 | PITR-Fortschritt-Phase-Konvertierung fehlerhaft | Fehlender Default-Case in Phase-Enum-Konvertierung ergänzt |

---

## 📊 Modulstatus-Zusammenfassung

| Status | Anzahl | Beispiele |
|--------|--------|-----------|
| ✅ Production-ready | 47 | storage, graph, auth, llm, temporal, search, geo, rag, gpu, process, sharding, ... |
| 🟡 Beta | 1 | chimera |
| 🔴 Alpha | 2 | ethics_ai, onnx_clip |

> **Hinweis:** gpu, process und sharding haben am 2026-04-03/04 alle Beta-Graduation-Gates bestanden
> und wurden auf ✅ Production-ready hochgestuft (Nachweis: `scripts/operations/BETA_MODULE_GRADUATION_TODO.md`).

---

## 🔒 Sicherheits-Patch (2026-04-05) {#sicherheits-patch-2026-04-05}

Nach Veröffentlichung des RC1-Images wurde ein Docker-Image-Security-Patch eingespielt:

| Maßnahme | Details |
|----------|---------|
| `THEMIS_ENABLE_ENCRYPTED_STORAGE=OFF` als Default | `gocryptfs` + Go-stdlib-Dep entfernt aus Standard-Runtime-Image |
| `apt-get purge tar` | `tar`-Binary aus Image entfernt |
| CVEs vor Patch | 39 (3 CRITICAL, 11 HIGH, 21 MEDIUM, 4 LOW) |
| CVEs nach Patch | 3 (alle LOW/MEDIUM, kein upstream-Fix) |
| Verbleibende CVEs | CVE-2024-2236 (libgcrypt20/LOW), CVE-2024-56433 (shadow/LOW), CVE-2025-45582 (tar-Metadaten/MEDIUM) |
| Waiver-Dokument | [`docs/audit-reports/cve-waivers.md`](../../audit-reports/cve-waivers.md) |

Betroffene Tags: `themisdb/themisdb:latest`, `themisdb/themisdb:1.8.1-rc1` (beide neu gepusht).

---

## 🔗 Links

- **Changelog:** [CHANGELOG.md](../../../CHANGELOG.md#181-rc1---2026-04-04)
- **Roadmap:** [roadmap.md](../../../roadmap.md)
- **Migration Guide Config:** [config/MIGRATION_GUIDE.md](../../../config/MIGRATION_GUIDE.md)
- **HSM Setup:** [docs/security/HSM_PRODUCTION_SETUP.md](../../security/HSM_PRODUCTION_SETUP.md)
- **Multi-GPU API:** [docs/MULTI_GPU_VECTOR_INDEXING.md](../../MULTI_GPU_VECTOR_INDEXING.md)
- **Docker Guide:** [docker/README.md](../../../docker/README.md)
- **Docker Hub (Release Tag):** `docker pull themisdb/themisdb:1.8.1-rc1`
- **Docker Hub (Latest):** `docker pull themisdb/themisdb:latest`
