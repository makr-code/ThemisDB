# Ingestion-Modul — Fehlende / Unvollständige Implementierungen

<!-- Status: current | validated: 2026-03-09 -->
<!-- Primärdokumentation: ../../../src/ingestion/ -->

Dieser Report dokumentiert Funktionen, die in `src/ingestion/ROADMAP.md`, `src/ingestion/README.md` oder anderen Primary-Docs als implementiert beschrieben werden oder als geplant gelten, jedoch bei der Reality-Check-Prüfung als **nicht vollständig umgesetzt** oder **als Stub** befunden wurden.

Prüfstand: 2026-03-09 | Branch: `develop`

---

## 1. HuggingFaceConnector — HTTP-GET-Client simuliert

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/ingestion/README.md` §"HuggingFaceConnector" ("Downloads and ingests datasets from the HuggingFace Hub API"); `src/ingestion/FUTURE_ENHANCEMENTS.md` §"Implemented Connectors" (Status: `✅ Implemented`) |
| **Erwartet** | `HuggingFaceConnector::HttpClient::get()` sendet echte HTTP-GET-Anfragen an die HuggingFace API via `libcurl` |
| **Beobachtet** | `HttpClient::get()` gibt immer `{status_code=200, body="{\"status\": \"available\", \"rows\": 12000}"}` zurück — kein echter Netzwerk-Call. Der Kommentar lautet: "Simulated response for demonstration" |
| **Evidence** | `src/ingestion/huggingface_connector.cpp` Zeilen 37 ("preserve the existing stub structure"), 56–68 (hardcoded response); Datei-Header: `Stubs: 1` |
| **ROADMAP-Status** | Kein separates ROADMAP-Item; `src/ingestion/ROADMAP.md` §"Known Issues" listet dies explizit: "libcurl HTTP calls in HuggingFaceConnector are stubbed" |
| **Issue-Titelvorschlag** | `[ingestion] Replace HuggingFaceConnector simulated HTTP client with real libcurl implementation` |
| **Label-Vorschläge** | `type:bug`, `priority:high`, `ingestion`, `status:open` |

---

## 2. CdcConnector — Stream-Backend Compile-Time-Stub

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/ingestion/ROADMAP.md` §"Completed ✅ — CDC source for live database streams" (Issue: #2199); `src/ingestion/FUTURE_ENHANCEMENTS.md` §"Implemented Connectors" ("CDC Source: ✅ Implemented") |
| **Erwartet** | `CdcConnector::ingestFromStream()` konsumiert einen Replikations-Stream (PostgreSQL logical replication / MySQL binlog) und erzeugt `CdcEvent`-Objekte |
| **Beobachtet** | `ingestFromStream()` gibt `IngestionErrorCode::CONNECTOR_NOT_SUPPORTED` zurück mit der Meldung: "CdcConnector stream backend not yet implemented; set up a replication driver and complete ingestFromStream()". Der Kommentar erklärt: "production skeleton is intentionally left as a compile-time gated stub" |
| **Evidence** | `src/ingestion/cdc_connector.cpp` Zeilen 354–363 ("gated stub"), 369 (`CONNECTOR_NOT_SUPPORTED`); Datei-Header: `Stubs: 1` |
| **ROADMAP-Status** | Als `[x]` markiert — aber das ist die Konnektor-Registrierung; der Stream-Ingestion-Pfad selbst ist ein Stub |
| **Issue-Titelvorschlag** | `[ingestion] Implement CdcConnector ingestFromStream() with PostgreSQL/MySQL replication driver` |
| **Label-Vorschläge** | `type:feature`, `priority:medium`, `ingestion`, `cdc`, `status:open` |

---

## 3. IngestionAdminApi::retryQuarantineItem() — Schreib-Erfolg immer true

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/ingestion/ROADMAP.md` §"Per-document quarantine retry with exponential back-off [x]" (Issue: #1916); `src/ingestion/FUTURE_ENHANCEMENTS.md` §"Per-Document Quarantine Retry — ✅ Implemented" |
| **Erwartet** | `retryQuarantineItem(doc_id)` ruft den Storage-Write-Pfad auf und gibt `true` oder `false` je nach tatsächlichem Schreib-Ergebnis zurück |
| **Beobachtet** | Der Write-Erfolg ist immer `true` (Kommentar: "NOTE: the failure branch below is unreachable in this stub but preserves the intended production behaviour"); das Storage-Layer (`entry.raw_payload`) wird nicht tatsächlich in die Datenbank geschrieben |
| **Evidence** | `src/ingestion/ingestion_manager.cpp` Zeilen 1893–1895 ("unreachable in this stub"); Datei-Header: `Stubs: 1` |
| **ROADMAP-Status** | Als `[x]` markiert — Quarantine-Queue-Datenstruktur und Retry-Loop implementiert; Storage-Write nicht verdrahtet |
| **Issue-Titelvorschlag** | `[ingestion] Wire IngestionAdminApi::retryQuarantineItem() to real storage write path` |
| **Label-Vorschläge** | `type:bug`, `priority:medium`, `ingestion`, `status:open` |

---

## 4. End-to-End Ingestion Lineage Tracking — Nicht implementiert

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/ingestion/ROADMAP.md` §"Planned Features / Remaining — End-to-end ingestion lineage tracking (Issue: #1901)" |
| **Erwartet** | Jedes ingested Dokument wird mit einem Lineage-Record verknüpft: Quelle, Zeitstempel, Konnektor-Version, Transformationsschritte |
| **Beobachtet** | Kein Treffer für `IngestionLineage`, `LineageRecord`, `lineage_tracking` in `src/ingestion/`. Die `IngestionReport`-Struktur enthält Statistiken aber keine Lineage-Daten |
| **Evidence** | Kein Code-Treffer; nur ROADMAP-Referenz `[I] Issue: #1901` |
| **ROADMAP-Status** | `[I]` Issue offen (Issue: #1901) |
| **Issue-Titelvorschlag** | `[ingestion] Implement end-to-end ingestion lineage tracking` |
| **Label-Vorschläge** | `type:feature`, `priority:low`, `ingestion`, `observability`, `status:open` |

---

## Zusammenfassung

| # | Feature | Quelle | Kritikalität | Status |
|---|---|---|---|---|
| 1 | HuggingFaceConnector HTTP-Client | `huggingface_connector.cpp` L37,56-68 | **Hoch** | Simulated Stub |
| 2 | CdcConnector Stream-Backend | `cdc_connector.cpp` L354-363 | Mittel | Compile-Stub |
| 3 | retryQuarantineItem() Storage-Write | `ingestion_manager.cpp` L1893-1895 | Mittel | Unreachable Stub |
| 4 | Lineage Tracking | ROADMAP Issue #1901 | Niedrig | Nicht implementiert |

*Alle anderen ROADMAP-Einträge sind durch vorhandene Implementierungsdateien auf `develop` belegt. Kein kritischer Ingestion-Pfad (GenericApiConnector, FileSystemIngester, KafkaConnector, ObjectStorageConnector, DatabaseConnector, WebCrawlerConnector, IngestionCoordinator) enthält Stubs.*
