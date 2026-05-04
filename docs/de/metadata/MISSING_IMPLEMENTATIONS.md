# Metadata-Modul – Fehlende / Unvollständige Implementierungen
<!-- Status: current | validated: 2026-04-06 -->
<!-- Primärdokumentation: ../../../src/metadata/README.md -->

**Stand:** 2026-03-11  
**Modul:** `metadata`  
**Quelle:** Reality-Check gegen Sourcecode (Commit: `ab36d18bd`)

> Dieses Dokument listet Punkte auf, die in der primären Dokumentation als erledigt markiert
> sind oder als Teil der Production-Readiness-Checkliste gelten, aber beim Abgleich mit dem
> Sourcecode als fehlend, unvollständig oder eingeschränkt erkannt wurden.
>
> **Kein Auto-Issue:** Diese Einträge dienen als transparenter Befund-Bericht. Issue-Erstellung
> und Priorisierung obliegen dem Engineering-Team.

---

## Befund 1 – Leistungsbenchmarks nicht vorhanden ✅ Behoben

| Feld | Inhalt |
|------|--------|
| **Claim-Quelle** | `src/metadata/ROADMAP.md`, Production Readiness Checklist |
| **Claim** | `[?] Performance benchmarks (cache hit rate, scan latency) – planned for v1.6.0` |
| **Erwartet** | Dedizierte Benchmark-Suite für Metadaten-Cache-Hit-Rate und RocksDB-Scan-Latenz |
| **Beobachtet** | Kein `bench_metadata_*.cpp` oder ähnliches im Repository; keine Benchmark-Ergebnisse dokumentiert |
| **Geprüfte Pfade** | `tests/bench_*`, `benchmarks/`, `src/metadata/` (kein Benchmark-Verzeichnis gefunden) |
| **Evidence** | ROADMAP Production Readiness Checklist: `[?] Performance benchmarks` |
| **Status** | ✅ **Behoben** – `benchmarks/bench_metadata_cache.cpp` implementiert (META-MISSING-001). Benchmarks: cold scan, warm hit, Hit/Miss-Vergleich, TTL-Varianten, adaptives TTL, Concurrent-Reads, RocksDB-Direktvergleich. Ergebnisse: `docs/benchmarks/metadata_cache_benchmark_results.md` |
| **Issue-Titelvorschlag** | _(behoben, kein Issue nötig)_ |

---

## Befund 2 – Security-Audit ✅ Behoben (v1.6.0)

| Feld | Inhalt |
|------|--------|
| **Claim-Quelle** | `src/metadata/ROADMAP.md`, Production Readiness Checklist |
| **Claim** | `[?] Security audit (metadata access control, information disclosure) – planned for v1.6.0` |
| **Erwartet** | Formales Security-Audit: Zugriffskontrolle auf Metadaten-APIs, Information Disclosure (Schema-Leak), Audit-Log-Manipulation |
| **Beobachtet** | Keine Audit-Ergebnisse im Repository; SchemaApiHandler prüft HTTP-Auth, aber kein formales Security-Review belegt |
| **Geprüfte Pfade** | `src/server/schema_api_handler.cpp`, `include/metadata/schema_audit_log.h`, `docs/de/metadata/` |
| **Evidence** | ROADMAP Production Readiness Checklist: `[?] Security audit` |
| **Status** | ✅ **Behoben in v1.6.0** – `IMetadataSecurityProvider`-Interface eingeführt (`include/metadata/imetadata_security_provider.h`) mit `NoOpMetadataSecurityProvider` (Default, Zero-Overhead) und `InMemoryRbacMetadataSecurityProvider` (Thread-sicheres RBAC: grant/revoke/revokeAll, Wildcard-Resource `"*"`, ADMIN-impliziert-alle, `MetadataAccessDeniedException`). 11 Acceptance-Criteria-Tests. ROADMAP-Checkliste von `[?]` auf `[x]` aktualisiert. CI: `metadata-interfaces-ci.yml`. |
| **Issue-Titelvorschlag** | _(behoben, kein Issue nötig)_ |

---

## Befund 3 – `src/metadata/README.md` referenzierte nicht-existierende Dateien (behoben)

| Feld | Inhalt |
|------|--------|
| **Claim-Quelle** | `src/metadata/README.md`, Abschnitt "Relevant Interfaces" |
| **Claim** | Auflistung von `metadata_manager.cpp`, `schema_registry.cpp`, `type_system.cpp`, `index_metadata.cpp` |
| **Erwartet** | Korrekte Liste der tatsächlich vorhandenen Source-Dateien |
| **Beobachtet** | Keines der vier genannten Dateien existiert in `src/metadata/`. Die tatsächlichen Dateien sind: `schema_manager.cpp`, `statistics_collector.cpp`, `information_schema.cpp`, u. a. |
| **Geprüfte Pfade** | `src/metadata/*.cpp` |
| **Evidence** | `ls src/metadata/*.cpp` zeigt keine der genannten Dateien |
| **Status** | ✅ **Behoben in Commit `ab36d18bd`** – README auf tatsächliche Dateien aktualisiert |
| **Issue-Titelvorschlag** | _(behoben, kein Issue nötig)_ |

---

## Befund 4 – `src/metadata/README.md` Reifegrad als „Beta" markiert (behoben)

| Feld | Inhalt |
|------|--------|
| **Claim-Quelle** | `src/metadata/README.md`, Abschnitt "Current Delivery Status" |
| **Claim** | `Maturity: 🟡 Beta` |
| **Erwartet** | Korrekte Reifegrad-Angabe entsprechend tatsächlichem Implementierungsstand |
| **Beobachtet** | Alle Phase 1–3-Features sind implementiert und getestet (v1.5.x); ROADMAP korrekt als Production-Ready markiert |
| **Geprüfte Pfade** | `src/metadata/ROADMAP.md`, alle `tests/test_schema_*.cpp`, `tests/test_statistics_*.cpp` |
| **Evidence** | ROADMAP-Abschnitt "Current Status": „Full-featured, production-ready metadata layer" |
| **Status** | ✅ **Behoben in Commit `ab36d18bd`** – README auf 🟢 Production-Ready aktualisiert |
| **Issue-Titelvorschlag** | _(behoben, kein Issue nötig)_ |

---

## Befund 5 – Keine explizite Forward/Backward-Compat-Policy (bekannte Einschränkung)

| Feld | Inhalt |
|------|--------|
| **Claim-Quelle** | `src/metadata/ROADMAP.md`, Planned Features |
| **Claim** | `[ ] Explicit compat-mode policy enforcement (forward/backward) (Target: v1.9 / Q1 2027)` |
| **Erwartet** | Vollständige Forward/Backward-Kompatibilitätsprüfung als Policy-Engine |
| **Beobachtet** | `validateMigration()` in `schema_version_manager.cpp` prüft strukturelle Konsistenz (keine doppelten Spalten, nicht identisch mit vorheriger Version); explizite `forward`/`backward`-Compat-Modi fehlen |
| **Geprüfte Pfade** | `include/metadata/schema_version_manager.h` (`validateMigration`), `src/metadata/schema_version_manager.cpp` |
| **Evidence** | ROADMAP: `validateMigration` API vorhanden; explizite Compat-Policy für v1.9 geplant |
| **Severity** | Low – bewusst zurückgestellt; Zielversion v1.9 / Q1 2027 |
| **Issue-Titelvorschlag** | `feat(metadata): implement explicit forward/backward compat-mode policy enforcement` |
| **Label-Vorschläge** | `enhancement`, `metadata`, `schema-evolution` |

---

## Befund 6 – ROADMAP referenziert `system_catalog.cpp` (existiert nicht)

| Feld | Inhalt |
|------|--------|
| **Claim-Quelle** | `src/metadata/ROADMAP.md`, Phase 1, Zeile: „`metadata/system_catalog.cpp`" |
| **Claim** | `[x] SystemCatalog: table, column, index, and statistics metadata persistence (metadata/system_catalog.cpp)` |
| **Erwartet** | Datei `src/metadata/system_catalog.cpp` mit einer eigenständigen `SystemCatalog`-Klasse |
| **Beobachtet** | `src/metadata/system_catalog.cpp` existiert nicht. Die SystemCatalog-Funktionalität (Catalog-Persistenz via RocksDB) ist in `schema_manager.cpp` implementiert. Kein `class SystemCatalog` in `include/metadata/` gefunden. |
| **Geprüfte Pfade** | `src/metadata/*.cpp`, `include/metadata/*.h` |
| **Evidence** | `ls src/metadata/*.cpp` zeigt keine `system_catalog.cpp`; `grep -r "class SystemCatalog" include/metadata/` liefert keine Treffer |
| **Severity** | Low – Funktionalität ist vorhanden (in `schema_manager.cpp`); nur die ROADMAP-Referenz war irreführend. Wurde in ROADMAP und FUTURE_ENHANCEMENTS.md korrigiert. |
| **Status** | ✅ **Behoben** – ROADMAP und FUTURE_ENHANCEMENTS.md aktualisiert (Commit: `3c9b336dc`) |
| **Issue-Titelvorschlag** | _(behoben, kein Issue nötig)_ |

---

## Weiterführende Dokumentation

- [src/metadata/ROADMAP.md](../../../src/metadata/ROADMAP.md) — vollständiger Feature-Status
- [src/metadata/README.md](../../../src/metadata/README.md) — Modulübersicht
- [docs/de/metadata/README.md](README.md) — German Secondary Docs
- [missing-implementations.json](missing-implementations.json) — maschinenlesbare Version
