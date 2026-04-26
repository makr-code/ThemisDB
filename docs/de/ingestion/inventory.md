# Ingestion-Modul – Primäres Inventar

<!-- Status: current | validated: 2026-04-06 -->
<!-- Primärdokumentation: ../../../src/ingestion/ | ../../../include/ingestion/ -->

**Datum:** März 2026  
**Modul:** `ingestion`  
**Modulpfad:** `src/ingestion/` + `include/ingestion/`

---

## 1. Dokumentationsdateien im Modul

| Datei | Beschreibung |
|---|---|
| `src/ingestion/README.md` | Modulübersicht, Konnektor-Beschreibungen, Architektur-Diagramm, Dependencies, Usage-Beispiele, Production-Status |
| `src/ingestion/ARCHITECTURE.md` | Architektur-Guide: Design-Prinzipien, Komponenten-Diagramm, Datenfluss, Threading, Security, Konfiguration, Fehlerbehandlung |
| `src/ingestion/ROADMAP.md` | Implementierungsstatus (v1.5.x), abgeschlossene Features, Phasenmodell, Production-Readiness-Checkliste, Known Issues |
| `src/ingestion/FUTURE_ENHANCEMENTS.md` | Geplante Erweiterungen, implementierte Konnektoren-Tabelle, IEEE-Referenzen |
| `include/ingestion/FUTURE_ENHANCEMENTS.md` | Header-Interface-Enhancements (alle implementiert außer Plugin-Sandbox) |

---

## 2. Quellcode-Dateien (`src/ingestion/`)

### Kern-Orchestrierung

| Quelldatei | Header | Beschreibung |
|---|---|---|
| `ingestion_manager.cpp` | `include/ingestion/ingestion_manager.h` | Orchestrierung aller Quellen; `IngestionManager`, `IngestionBuilder`, `IngestionAdminApi`, `ConnectorPluginRegistry`, `IngestionMetricsExporter` |
| `ingestion_coordinator.cpp` | `include/ingestion/ingestion_coordinator.h` | Verteilter Ingestion-Koordinator mit Work-Stealing-Thread-Pool; Leader-Election via TTL-Lease; Production-ready |

### Konnektoren

| Quelldatei | Header | Beschreibung | Status |
|---|---|---|---|
| `api_connector.cpp` | `include/ingestion/api_connector.h` | Generic REST API Konnektor; echte `curl_easy_perform`-Calls; PR #1915 | ✅ Production |
| `filesystem_ingester.cpp` | `include/ingestion/filesystem_ingester.h` | Rekursiver Verzeichnis-Walker; HTML/XML-Extraktion (pugixml); PDF/DOCX via externe Konverter; MIME-Detection | ✅ Production |
| `huggingface_connector.cpp` | `include/ingestion/huggingface_connector.h` | HuggingFace Hub Datasets; echte `curl_easy_perform`-Calls; OAuth 2.0 Token-Refresh; PR #1915 | ✅ Production |
| `kafka_connector.cpp` | `include/ingestion/kafka_connector.h` | Apache Kafka Consumer (librdkafka); Consumer-Group-Management; Offset-Commit an Checkpoint | ✅ Production |
| `object_storage_connector.cpp` | `include/ingestion/object_storage_connector.h` | S3/GCS/Azure Blob; 28 Unit-Tests; Pfad-Traversal-Schutz; SSE-Enforcement; keine Credentials im Log | ✅ Production |
| `database_connector.cpp` | `include/ingestion/database_connector.h` | JDBC-kompatibler Datenbank-Konnektor (ODBC) | ✅ Production |
| `web_crawler_connector.cpp` | `include/ingestion/web_crawler_connector.h` | HTTP Web-Crawler; BFS; Sitemap-Discovery; robots.txt; SSRF-Schutz (nur http/https) | ✅ Production |
| `cdc_connector.cpp` | `include/ingestion/cdc_connector.h` | Change-Data-Capture (CDC); PostgreSQL logical replication (libpq) unter `THEMIS_ENABLE_CDC_STREAM`; graceful fallback ohne Flag | ✅ Production |

---

## 3. Öffentliche Header (`include/ingestion/`)

| Header | Beschreibung |
|---|---|
| `ingestion_manager.h` | `IngestionManager`, `IngestionBuilder`, `IngestionAdminApi`, `ConnectorPluginRegistry`, `SourceConfig`, `RetryConfig`, `RateLimitConfig`, `QuarantineEntry` |
| `api_connector.h` | `GenericApiConnector` Interface + Config |
| `filesystem_ingester.h` | `FileSystemIngester` Interface + `BinaryConverter` Config |
| `huggingface_connector.h` | `HuggingFaceConnector` Interface + Config |
| `kafka_connector.h` | `KafkaConnector` Interface + `KafkaConnectorConfig` |
| `object_storage_connector.h` | `ObjectStorageConnector` Interface + `StorageProviderConfig` |
| `database_connector.h` | `DatabaseConnector` Interface + `DatabaseConnectorConfig` |
| `web_crawler_connector.h` | `WebCrawlerConnector` Interface + Options |
| `cdc_connector.h` | `CdcConnector` Interface + `CdcConnectorConfig` |
| `ingestion_coordinator.h` | `IngestionCoordinator` Interface + `CoordinatorConfig` |
| `FUTURE_ENHANCEMENTS.md` | Header-Interface-Enhancements Dokument |

---

## 4. Sekundäre Dokumentation (`docs/de/ingestion/`)

| Datei | Beschreibung |
|---|---|
| `docs/de/ingestion/README.md` | Deutsche Modul-Übersicht; Konnektor-Status-Tabelle; Quicklinks |
| `docs/de/ingestion/inventory.md` | Dieses Inventardokument |
| `docs/de/ingestion/missing-implementations.md` | Report fehlender/unvollständiger Implementierungen |

---

## 5. Reality-Check-Ergebnis (Stand: März 2026)

### ✅ Korrekt dokumentiert
- Alle 10 Quelldateien und ihre zugehörigen Header sind vorhanden
- `IngestionManager`, `IngestionBuilder`, `IngestionAdminApi`, Plugin-Registry korrekt dokumentiert
- `GenericApiConnector` mit echten `curl_easy_perform`-Calls (PR #1915) korrekt als implementiert geführt
- KafkaConnector, ObjectStorageConnector, DatabaseConnector, WebCrawlerConnector alle vorhanden

### 🔧 Korrigiert (März 2026, PR #ingestion-build-system)
- `src/ingestion/ROADMAP.md`: JDBC-Konnektor `[P]`→`[x]`, Web-Crawler `[P]`→`[x]`, Schema-Validation `[P]`→`[x]`, Verteilter Koordinator `[I]`→`[x]`, Dynamic-Reconfiguration `[I]`→`[x]`; Phase 2 & Phase 3 Status: "In Progress" → "Completed"
- `src/ingestion/README.md`: Status "Beta" → "Production-ready v1.5.x"
- `src/ingestion/ARCHITECTURE.md`: Known Limitations aktualisiert (Kafka-Planung entfernt); neue Konnektoren (CDC, Coordinator) in Komponenten-Tabelle ergänzt
- `include/ingestion/FUTURE_ENHANCEMENTS.md`: alle `[ ]` Planned-Items auf `[x]` (implementiert); Design-Constraints aktualisiert
- `src/ingestion/FUTURE_ENHANCEMENTS.md`: 13 IEEE-Referenzen hinzugefügt; validated-Header ergänzt
- `HuggingFaceConnector` HTTP-Client: Stub → Production (echte `curl_easy_perform`-Calls, PR #1915) ✅
- `CdcConnector::ingestFromStream()`: Compile-Stub → Production (PostgreSQL logical replication via `#ifdef THEMIS_ENABLE_CDC_STREAM`) ✅
- `IngestionAdminApi::retryQuarantineItem()`: Schreib-Erfolg immer `true` → `DocumentWriteFn`-Injection implementiert ✅
- Build-System: alle 10 `src/ingestion/*.cpp`-Dateien bedingungslos in `THEMIS_CORE_SOURCES` (`cmake/CMakeLists.txt`) und `THEMIS_INGESTION_SOURCES` (`cmake/ModularBuild.cmake`) registriert
- Tests: 18 Standalone-Focused-Test-Targets in `tests/CMakeLists.txt` via `add_ingestion_focused_test`-Makro ergänzt
- Benchmark: `bench_ingestion_kv` in `cmake/CMakeLists.txt` registriert

### ⚠️ Bekannte Einschränkungen (dokumentiert in `missing-implementations.md`)
- ✅ Alle ROADMAP-Aufgaben implementiert. Keine offenen Items.
