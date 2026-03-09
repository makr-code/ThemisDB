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

## Befund 2 – At-least-once nicht für SSE-Verbindungen garantiert

| Feld | Inhalt |
|------|--------|
| **Claim-Quelle** | `src/cdc/ROADMAP.md`, Completed-Liste |
| **Claim** | `[x] At-least-once delivery guarantees with consumer acknowledgement (Issue: #1606)` |
| **Erwartet** | At-least-once-Guarantee für alle CDC-Transporte inkl. SSE |
| **Beobachtet** | Implementiert nur für Consumer Groups (`ConsumerGroupManager::fetchEventsAtLeastOnce()` in `src/cdc/consumer_group.cpp`); für reine SSE-Verbindungen (`GET /changefeed/stream`) existiert **kein Acknowledgement-Loop** – Verbindungsabbrüche führen zu Eventverlust |
| **Geprüfte Pfade** | `src/cdc/consumer_group.cpp`, `src/cdc/delivery_tracker.cpp`, `src/cdc/changefeed.cpp`; ROADMAP Known Issues: *"At-least-once delivery is not yet guaranteed for SSE connections"* |
| **Evidence** | `consumer_group.h` + `delivery_tracker.h` existieren; keine SSE-ACK-Logik in `changefeed.cpp` gefunden |
| **Issue-Titelvorschlag** | `feat(cdc): extend at-least-once delivery guarantee to SSE connections` |
| **Label-Vorschläge** | `enhancement`, `cdc`, `reliability` |

---

## Befund 3 – Change-Log-Retention nicht zur Laufzeit konfigurierbar

| Feld | Inhalt |
|------|--------|
| **Claim-Quelle** | `src/cdc/ROADMAP.md`, Completed-Liste |
| **Claim** | `[x] Change log TTL and size-based retention policies (Issue: #1608)` |
| **Erwartet** | Automatische TTL- und größenbasierte Retention, zur Laufzeit konfigurierbar |
| **Beobachtet** | Manuelles Admin-Trimming via `CDCAdmin::purgeOlderThan()` (REST: `POST /changefeed/retention`) ist implementiert; automatische zeitgesteuerte oder größenbasierte Retention **ohne Server-Neustart** ist **nicht verfügbar** |
| **Geprüfte Pfade** | `src/cdc/cdc_admin.cpp`, `include/cdc/cdc_admin.h`; ROADMAP Known Issues: *"Change log retention policies are not configurable at runtime"* |
| **Evidence** | `cdc_admin.cpp` stellt `purgeOlderThan()` bereit; kein Background-Retention-Thread oder Runtime-Config-Hook gefunden |
| **Issue-Titelvorschlag** | `feat(cdc): runtime-configurable change log retention policies (TTL + size-based)` |
| **Label-Vorschläge** | `enhancement`, `cdc`, `operations` |

---

## Weiterführende Dokumentation

- [src/cdc/ROADMAP.md](../../../src/cdc/ROADMAP.md) — vollständiger Feature-Status
- [src/cdc/README.md](../../../src/cdc/README.md) — Modulübersicht
- [features_change_data_capture.md](../features/features_change_data_capture.md) — End-User-Dokumentation
- [missing-implementations.json](missing-implementations.json) — maschinenlesbare Version
