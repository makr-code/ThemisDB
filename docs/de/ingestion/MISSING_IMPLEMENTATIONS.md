# Ingestion-Modul — Fehlende / Unvollständige Implementierungen

<!-- Status: current | validated: 2026-04-06 | updated: 2026-03-11 -->
<!-- Primärdokumentation: ../../../src/ingestion/ -->

Dieser Report dokumentiert Funktionen, die in `src/ingestion/ROADMAP.md`, `src/ingestion/README.md` oder anderen Primary-Docs als implementiert beschrieben werden oder als geplant gelten, jedoch bei der Reality-Check-Prüfung als **nicht vollständig umgesetzt** oder **als Stub** befunden wurden.

Prüfstand: 2026-03-09 | Branch: `develop` | Aktualisiert: 2026-03-11

---

## 1. HuggingFaceConnector — HTTP-GET-Client simuliert — ✅ Gelöst

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/ingestion/README.md` §"HuggingFaceConnector" ("Downloads and ingests datasets from the HuggingFace Hub API"); `src/ingestion/FUTURE_ENHANCEMENTS.md` §"Implemented Connectors" (Status: `✅ Implemented`) |
| **Erwartet** | `HuggingFaceConnector::HttpClient::get()` sendet echte HTTP-GET-Anfragen an die HuggingFace API via `libcurl` |
| **Beobachtet (alt)** | `HttpClient::get()` gab immer `{status_code=200, body="{\"status\": \"available\", \"rows\": 12000}"}` zurück — kein echter Netzwerk-Call. |
| **Lösung (2026-03-10)** | `hfHttpGet()` verwendet `curl_easy_perform` mit TLS-Verifizierung, Bearer-Token-Header und Timeout-Konfiguration. `Stubs: 0` im Datei-Header bestätigt. (Issue: #1915 — `[x]` in ROADMAP) |
| **Erweiterung (2026-03-11)** | `RetryConfig::ca_bundle_path` (Issue: INGESTION-MISSING-001) hinzugefügt: konfigurierbarer CA-Bundle-Pfad via `CURLOPT_CAINFO`. Gilt für `hfHttpGet()`, `hfHttpPost()`, `apiHttpGet()`, `apiHttpPost()`. Parsierbar via `SourceConfig::options["ca_bundle_path"]` oder direkt per `setRetryConfig()`. TLS-Verifizierung (`CURLOPT_SSL_VERIFYPEER = 1L`) bleibt immer aktiv; `ca_bundle_path` überschreibt nur den verwendeten CA-Store. |
| **Issue-Titelvorschlag** | `[ingestion] Replace HuggingFaceConnector simulated HTTP client with real libcurl implementation` — ✅ gelöst |
| **Label-Vorschläge** | `type:bug`, `priority:high`, `ingestion`, `status:resolved` |

---

## 2. CdcConnector — Stream-Backend Compile-Time-Stub — ✅ Gelöst

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/ingestion/ROADMAP.md` §"Completed ✅ — CDC source for live database streams" (Issue: #2199); `src/ingestion/FUTURE_ENHANCEMENTS.md` §"Implemented Connectors" ("CDC Source: ✅ Implemented") |
| **Erwartet** | `CdcConnector::ingestFromStream()` konsumiert einen Replikations-Stream (PostgreSQL logical replication / MySQL binlog) und erzeugt `CdcEvent`-Objekte |
| **Beobachtet (alt)** | `ingestFromStream()` gab `IngestionErrorCode::CONNECTOR_NOT_SUPPORTED` zurück; Datei-Header: `Stubs: 1` |
| **Lösung (2026-03-10)** | `ingestFromStream()` ist vollständig implementiert via PostgreSQL logical replication protocol (libpq, `test_decoding` output plugin) unter `#ifdef THEMIS_ENABLE_CDC_STREAM`. Ohne das Flag fällt der Connector auf `CONNECTOR_NOT_SUPPORTED` zurück (legitimer Build-Gate). `Stubs: 0` im Datei-Header bestätigt. |

---

## 3. IngestionAdminApi::retryQuarantineItem() — Schreib-Erfolg immer true — ✅ Gelöst

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/ingestion/ROADMAP.md` §"Per-document quarantine retry with exponential back-off [x]" (Issue: #1916); `src/ingestion/FUTURE_ENHANCEMENTS.md` §"Per-Document Quarantine Retry — ✅ Implemented" |
| **Erwartet** | `retryQuarantineItem(doc_id)` ruft den Storage-Write-Pfad auf und gibt `true` oder `false` je nach tatsächlichem Schreib-Ergebnis zurück |
| **Beobachtet (alt)** | Der Write-Erfolg war immer `true`; Storage-Layer nicht verdrahtet; Datei-Header: `Stubs: 1` |
| **Lösung (2026-03-10)** | `DocumentWriteFn`-Injection via `IngestionManager::setDocumentWriteForTesting()` implementiert. Wenn eine Write-Funktion injiziert ist, wird das Ergebnis zurückgegeben; andernfalls `true` (Production deployments wire die Write-Funktion über die Orchestrierungsebene). `Stubs: 0` im Datei-Header bestätigt. |

---

## 4. End-to-End Ingestion Lineage Tracking — ✅ Gelöst

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/ingestion/ROADMAP.md` §"Planned Features / Remaining — End-to-end ingestion lineage tracking (Issue: #1901)" |
| **Erwartet** | Jedes ingested Dokument wird mit einem Lineage-Record verknüpft: Quelle, Zeitstempel, Konnektor-Version, Transformationsschritte |
| **Beobachtet (alt)** | Kein Treffer für `IngestionLineage`, `LineageRecord`, `lineage_tracking` in `src/ingestion/`. Die `IngestionReport`-Struktur enthielt Statistiken aber keine Lineage-Daten |
| **Lösung (2026-03-10)** | `IngestionLineageRecord` + `IngestionLineageStore` in `include/ingestion/ingestion_manager.h` implementiert. `IngestionManager::enableLineageTracking()`, `getLineageRecords()`, `getLineageRecordsByRun()`, `getAllLineageRecords()`, `clearLineageRecords()` als öffentliche API. Lineage-Records enthalten: `run_correlation_id`, `source_id`, `connector_type`, `connector_version`, `doc_id`, `ingested_at`, `bytes`, `doc_count`, `transformation_steps`, `status`. 19 Unit-Tests in `tests/test_ingestion_lineage.cpp` → `IngestionLineageFocusedTests`. |

---

## Zusammenfassung

| # | Feature | Quelle | Kritikalität | Status |
|---|---|---|---|---|
| 1 | HuggingFaceConnector HTTP-Client | `huggingface_connector.cpp` | **Hoch** | ✅ Gelöst (2026-03-10) |
| 2 | CdcConnector Stream-Backend | `cdc_connector.cpp` | Mittel | ✅ Gelöst (2026-03-10) |
| 3 | retryQuarantineItem() Storage-Write | `ingestion_manager.cpp` | Mittel | ✅ Gelöst (2026-03-10) |
| 4 | Lineage Tracking | ROADMAP Issue #1901 | Niedrig | ✅ Gelöst (2026-03-10) |

*Alle ROADMAP-Einträge sind durch vorhandene Implementierungsdateien auf `develop` belegt. Kein kritischer Ingestion-Pfad enthält Stubs. Lineage-Tracking (Issue #1901) vollständig implementiert.*
