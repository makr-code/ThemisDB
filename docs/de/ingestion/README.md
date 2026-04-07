# Ingestion-Modul

<!-- Status: current | validated: 2026-04-06 | Primary: ../../../src/ingestion/ | ../../../include/ingestion/ -->
<!-- Links: ../../../src/ingestion/README.md · ../../../src/ingestion/ROADMAP.md · inventory.md · missing-implementations.md -->

**Stand:** 6. April 2026  
**Version:** 1.7.x (Production-ready)  
**Kategorie:** Daten-Intake / Konnektoren

---

## Übersicht

Das Ingestion-Modul ist ThemisDB's **Daten-Intake-Schicht**. Es stellt eine einheitliche Pipeline für das Laden von Dokumenten aus heterogenen externen Quellen bereit — lokale Dateisysteme, HuggingFace-Datasets, REST-APIs, Apache Kafka, S3/GCS/Azure Blob, JDBC-Datenbanken, Web-Crawler und CDC (Change-Data-Capture) — und schreibt diese normalisiert in die Datenbank.

**Operative Features:**
- Parallele Multi-Quell-Ingestion mit konfigurierbarem Thread-Pool
- Exponentielles Backoff-Retry pro Quelle
- Token-Bucket-Rate-Limiting pro Quelle
- Inkrementelles Checkpointing (kein Re-Processing)
- Quarantäne-Queue für persistente Fehler mit Per-Dokument-Retry
- Prometheus-kompatible Metriken-Export
- Admin-API (list, pause, resume, quarantine)
- Fluent Builder-API (`IngestionBuilder`)
- Plugin-API für externe Konnektoren

**Status:** 🟢 Production-ready

---

## Konnektoren-Übersicht

| Konnektor | Datei | Status |
|---|---|---|
| FileSystem-Ingester | `filesystem_ingester.cpp` | ✅ Production-ready |
| Generic REST API | `api_connector.cpp` | ✅ Production-ready (real libcurl) |
| HuggingFace-Datasets | `huggingface_connector.cpp` | ✅ Production-ready (real libcurl, PR #1915) |
| Apache Kafka Consumer | `kafka_connector.cpp` | ✅ Production-ready |
| S3 / GCS / Azure Blob | `object_storage_connector.cpp` | ✅ Production-ready |
| JDBC-Datenbank | `database_connector.cpp` | ✅ Production-ready |
| Web-Crawler / Sitemap | `web_crawler_connector.cpp` | ✅ Production-ready |
| CDC (live DB-Streams) | `cdc_connector.cpp` | ✅ Production-ready (Replikations-Treiber via `THEMIS_ENABLE_CDC_STREAM`; graceful fallback ohne Flag) |
| Verteilter Koordinator | `ingestion_coordinator.cpp` | ✅ Production-ready |

---

## Source-Code-Referenz

Vollständiges Inventar aller Quelldateien: **[inventory.md](./inventory.md)**

**Kern-Dateien:**

| Komponente | Header | Source | Beschreibung |
|---|---|---|---|
| IngestionManager | `ingestion_manager.h` | `ingestion_manager.cpp` | Orchestrierung aller Quellen |
| IngestionBuilder | *(in ingestion_manager.h)* | `ingestion_manager.cpp` | Fluent Builder API |
| IngestionAdminApi | *(in ingestion_manager.h)* | `ingestion_manager.cpp` | Admin-Endpoints |
| GenericApiConnector | `api_connector.h` | `api_connector.cpp` | REST-API Konnektor |
| FileSystemIngester | `filesystem_ingester.h` | `filesystem_ingester.cpp` | Dateisystem-Konnektor |
| HuggingFaceConnector | `huggingface_connector.h` | `huggingface_connector.cpp` | HuggingFace-Datasets |
| KafkaConnector | `kafka_connector.h` | `kafka_connector.cpp` | Kafka Consumer |
| ObjectStorageConnector | `object_storage_connector.h` | `object_storage_connector.cpp` | S3/GCS/Azure Blob |
| DatabaseConnector | `database_connector.h` | `database_connector.cpp` | JDBC-Datenbank |
| WebCrawlerConnector | `web_crawler_connector.h` | `web_crawler_connector.cpp` | Web-Crawler |
| CdcConnector | `cdc_connector.h` | `cdc_connector.cpp` | Change-Data-Capture |
| IngestionCoordinator | `ingestion_coordinator.h` | `ingestion_coordinator.cpp` | Verteilter Koordinator |

---

## Build-Optionen

| Flag | Beschreibung |
|---|---|
| `THEMIS_ENABLE_S3` | AWS S3 Konnektor aktivieren |
| `THEMIS_ENABLE_GCS` | Google Cloud Storage aktivieren |
| `THEMIS_ENABLE_AZURE` | Azure Blob Storage aktivieren |
| `THEMIS_ENABLE_CDC_STREAM` | CDC Stream-Backend aktivieren (erfordert Replikations-Treiber) |
| `THEMIS_HAS_PUGIXML` | HTML/XML-Text-Extraktion in FileSystemIngester |

---

## Quicklinks

- [Primäre Architekturdokumentation](../../../src/ingestion/README.md)
- [ROADMAP.md](../../../src/ingestion/ROADMAP.md)
- [ARCHITECTURE.md](../../../src/ingestion/ARCHITECTURE.md)
- [FUTURE_ENHANCEMENTS.md](../../../src/ingestion/FUTURE_ENHANCEMENTS.md)
- [inventory.md](./inventory.md) — Vollständiges Modul-Inventar
- [missing-implementations.md](./missing-implementations.md) — Bekannte Lücken

---

## Bekannte Einschränkungen

Detaillierter Report: **[missing-implementations.md](./missing-implementations.md)**

Kurzfassung (Stand 2026-03-10 — alle Items gelöst):
- Alle ROADMAP-Aufgaben implementiert. Keine offenen Items.
