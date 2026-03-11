# CDC Modul – Fehlende / Unvollständige Implementierungen
<!-- Status: current | validated: 2026-03-09 -->
<!-- Primärdokumentation: ../../../src/cdc/README.md -->

**Stand:** 2026-03-09  
**Modul:** `cdc`  
**Quelle:** Reality-Check gegen Sourcecode (Commit: `bdd737e6713b382f667d9c1a3843aa97c900e160`)

> Dieses Dokument listet Punkte auf, die in der primären Dokumentation als erledigt markiert
> sind oder als Teil der Production-Readiness-Checkliste gelten, aber beim Abgleich mit dem
> Sourcecode als fehlend, unvollständig oder eingeschränkt erkannt wurden.
>
> **Kein Auto-Issue:** Diese Einträge dienen als transparenter Befund-Bericht. Issue-Erstellung
> und Priorisierung obliegen dem Engineering-Team.

---

## Befund 1 – Unit-Test-Coverage < 80 %

| Feld | Inhalt |
|------|--------|
| **Claim-Quelle** | `src/cdc/ROADMAP.md`, Production Readiness Checklist |
| **Claim** | `[I] Unit tests coverage > 80% (Issue: #1623)` |
| **Erwartet** | Unit-Test-Coverage > 80 % für das CDC-Modul als DoD-Kriterium |
| **Beobachtet** | Issue #1623 ist **offen**; kein Coverage-Report im Repository; Teilabdeckung durch vorhandene Tests (z. B. `tests/test_cdc_delivery_tracker.cpp`, `tests/test_cdc_debezium_format.cpp`, `tests/test_cdc_outbox.cpp`, `tests/test_cdc_cross_collection_stream.cpp`, `tests/test_cdc_materialized_view.cpp`, `tests/test_cdc_change_stream_compressor.cpp`, `tests/test_cdc_schema_registry.cpp`) |
| **Geprüfte Pfade** | `tests/test_cdc_*.cpp` (existierend), `src/cdc/changefeed.cpp` (kein dedizierter Unit-Test-Satz gefunden), `src/cdc/tenant_buffer_manager.cpp` (kein dedizierter Unit-Test-Satz gefunden), `src/cdc/cdc_admin.cpp` (kein dedizierter Unit-Test-Satz gefunden) |
| **Evidence** | Issue #1623 open (kein PR verlinkt); kein `coverage_report.xml` oder ähnliches im Repo |
| **Issue-Titelvorschlag** | `test(cdc): increase unit test coverage to >80% (closes #1623)` |
| **Label-Vorschläge** | `testing`, `cdc`, `quality` |

---

## Befund 2 – At-least-once nicht für SSE-Verbindungen garantiert ✅ BEHOBEN

| Feld | Inhalt |
|------|--------|
| **Claim-Quelle** | `src/cdc/ROADMAP.md`, Completed-Liste |
| **Claim** | `[x] At-least-once delivery guarantees with consumer acknowledgement (Issue: #1606)` |
| **Erwartet** | At-least-once-Guarantee für alle CDC-Transporte inkl. SSE |
| **Beobachtet (behoben)** | `GET /changefeed/stream` unterstützt jetzt `consumer_id` + `ack_timeout_ms` Query-Parameter; `ChangefeedApiHandler::delivery_tracker_` trackt in-flight Events; `POST /changefeed/stream/ack` quittiert Events via `DeliveryTracker::acknowledgeUpTo()`; unquittierte Events werden bei Folge-Requests redelivered. 5 Integrationstests in `tests/test_http_changefeed_sse.cpp` verifizieren at-least-once-Semantik und Reconnect-Szenario. |
| **Geprüfte Pfade** | `src/server/changefeed_api_handler.cpp`, `include/server/changefeed_api_handler.h`, `src/cdc/delivery_tracker.cpp`, `include/cdc/delivery_tracker.h`, `src/server/http_server.cpp`, `tests/test_http_changefeed_sse.cpp` |
| **Status** | ✅ Implementiert und getestet |

---

## Befund 3 – Change-Log-Retention nicht zur Laufzeit konfigurierbar ✅ Behoben

| Feld | Inhalt |
|------|--------|
| **Claim-Quelle** | `src/cdc/ROADMAP.md`, Completed-Liste |
| **Claim** | `[x] Change log TTL and size-based retention policies (Issue: #1608)` |
| **Erwartet** | Automatische TTL- und größenbasierte Retention, zur Laufzeit konfigurierbar |
| **Status** | **Behoben** – `Changefeed::updateRetentionPolicy()` startet/stoppt den Background-Cleanup-Thread jetzt automatisch wenn `enabled` wechselt. `PUT /changefeed/retention` und `POST /config` (`cdc_retention_hours`) wenden Änderungen ohne Neustart an. |
| **Geprüfte Pfade** | `src/cdc/changefeed.cpp` (`updateRetentionPolicy()`), `src/server/changefeed_api_handler.cpp` (`handleRetentionPut()`), `src/server/http_server.cpp` (Hot-Reload), `tests/test_cdc_retention.cpp` |
| **Issue-Titelvorschlag** | `feat(cdc): runtime-configurable change log retention policies (TTL + size-based)` |
| **Label-Vorschläge** | `enhancement`, `cdc`, `operations` |

---

## Weiterführende Dokumentation

- [src/cdc/ROADMAP.md](../../../src/cdc/ROADMAP.md) — vollständiger Feature-Status
- [src/cdc/README.md](../../../src/cdc/README.md) — Modulübersicht
- [features_change_data_capture.md](../features/features_change_data_capture.md) — End-User-Dokumentation
- [missing-implementations.json](missing-implementations.json) — maschinenlesbare Version
