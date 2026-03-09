# Metadata-Modul – Fehlende / Unvollständige Implementierungen
<!-- Status: current | validated: 2026-03-09 -->
<!-- Primärdokumentation: ../../../src/metadata/README.md -->

**Stand:** 2026-03-09  
**Modul:** `metadata`  
**Quelle:** Reality-Check gegen Sourcecode (Commit: `ab36d18bd`)

> Dieses Dokument listet Punkte auf, die in der primären Dokumentation als erledigt markiert
> sind oder als Teil der Production-Readiness-Checkliste gelten, aber beim Abgleich mit dem
> Sourcecode als fehlend, unvollständig oder eingeschränkt erkannt wurden.
>
> **Kein Auto-Issue:** Diese Einträge dienen als transparenter Befund-Bericht. Issue-Erstellung
> und Priorisierung obliegen dem Engineering-Team.

---

## Befund 1 – Leistungsbenchmarks nicht vorhanden

| Feld | Inhalt |
|------|--------|
| **Claim-Quelle** | `src/metadata/ROADMAP.md`, Production Readiness Checklist |
| **Claim** | `[?] Performance benchmarks (cache hit rate, scan latency) – planned for v1.6.0` |
| **Erwartet** | Dedizierte Benchmark-Suite für Metadaten-Cache-Hit-Rate und RocksDB-Scan-Latenz |
| **Beobachtet** | Kein `bench_metadata_*.cpp` oder ähnliches im Repository; keine Benchmark-Ergebnisse dokumentiert |
| **Geprüfte Pfade** | `tests/bench_*`, `benchmarks/`, `src/metadata/` (kein Benchmark-Verzeichnis gefunden) |
| **Evidence** | ROADMAP Production Readiness Checklist: `[?] Performance benchmarks` |
| **Issue-Titelvorschlag** | `bench(metadata): add cache hit rate and scan latency benchmarks (closes v1.6.0 target)` |
| **Label-Vorschläge** | `performance`, `metadata`, `benchmarking` |

---

## Befund 2 – Security-Audit ausstehend

| Feld | Inhalt |
|------|--------|
| **Claim-Quelle** | `src/metadata/ROADMAP.md`, Production Readiness Checklist |
| **Claim** | `[?] Security audit (metadata access control, information disclosure) – planned for v1.6.0` |
| **Erwartet** | Formales Security-Audit: Zugriffskontrolle auf Metadaten-APIs, Information Disclosure (Schema-Leak), Audit-Log-Manipulation |
| **Beobachtet** | Keine Audit-Ergebnisse im Repository; SchemaApiHandler prüft HTTP-Auth, aber kein formales Security-Review belegt |
| **Geprüfte Pfade** | `src/server/schema_api_handler.cpp`, `include/metadata/schema_audit_log.h`, `docs/de/metadata/` |
| **Evidence** | ROADMAP Production Readiness Checklist: `[?] Security audit` |
| **Issue-Titelvorschlag** | `security(metadata): conduct access control and information disclosure audit (closes v1.6.0 target)` |
| **Label-Vorschläge** | `security`, `metadata`, `audit` |

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

## Weiterführende Dokumentation

- [src/metadata/ROADMAP.md](../../../src/metadata/ROADMAP.md) — vollständiger Feature-Status
- [src/metadata/README.md](../../../src/metadata/README.md) — Modulübersicht
- [docs/de/metadata/README.md](README.md) — German Secondary Docs
- [missing-implementations.json](missing-implementations.json) — maschinenlesbare Version
