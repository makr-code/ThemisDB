[docs](../../README.md) > [de](../README.md) > [chimera](./index.md) > [missing-implementations](./missing-implementations.md)
**Datum:** 2026-04-16
**Status:** review
**Primary (Quelle der Wahrheit):**
- `src/chimera/themisdb_adapter.cpp`
- `include/chimera/themisdb_adapter.hpp`
- `include/chimera/database_adapter.hpp`
- `src/chimera/ROADMAP.md`
- `src/chimera/FUTURE_ENHANCEMENTS.md`

**Bezug / Reference:**
- Issue: `[MODULE] chimera`
- Kontext: Missing-Implementations-Report für den modulweisen Reality-Check und die Doku-Migration.

---

# chimera — Missing Implementations Report

## Zusammenfassung

Die Kernfunktionalität des ThemisDB-Referenzadapters ist vorhanden und für den Simulationsmodus testbar.
Offene Lücken betreffen vor allem **Produktionshärtung, Capability-Konsistenz und Dokumentationsabdeckung**.

## Befund 1 — Capability-Claim `CONNECTION_POOLING` ohne dedizierte API-Umsetzung

| Feld | Inhalt |
|---|---|
| **Priorität** | Hoch |
| **Impact** | Risiko falscher Feature-Erkennung in Integrationen; mögliche Fehlkonfiguration bei Laufzeitentscheidungen |
| **Evidence** | `src/chimera/themisdb_adapter.cpp` (`has_capability`/`get_capabilities` melden `Capability::CONNECTION_POOLING`), aber `include/chimera/themisdb_adapter.hpp` enthält keinen dedizierten Pooling-Adaptervertrag |
| **Folge-Issue (Vorschlag)** | `feat(chimera): align connection pooling capability with concrete API contract and tests` |

---

## Befund 2 — Engine-backed Pfade enthalten `NOT_IMPLEMENTED`-Abbrüche

| Feld | Inhalt |
|---|---|
| **Priorität** | Hoch |
| **Impact** | Build-/Runtime-abhängige Feature-Lücken in produktionsnahen Konfigurationen |
| **Evidence** | `src/chimera/themisdb_adapter.cpp`: Fehlerpfade mit `ErrorCode::NOT_IMPLEMENTED` in engine-dispatch Routen (u. a. QueryEngine/VectorIndexManager/GraphIndexManager) |
| **Folge-Issue (Vorschlag)** | `feat(chimera): close NOT_IMPLEMENTED gaps in engine-backed adapter paths` |

---

## Befund 3 — Include-Primärdokumentation für `include/chimera/` fehlt

| Feld | Inhalt |
|---|---|
| **Priorität** | Mittel |
| **Impact** | Erhöhte Onboarding- und Integrationskosten für Consumer der Public Header |
| **Evidence** | `include/chimera/` enthält Header (`database_adapter.hpp`, `themisdb_adapter.hpp`), aber keine Modul-README/ROADMAP/FUTURE-Dokumente |
| **Folge-Issue (Vorschlag)** | `docs(chimera): add include/chimera primary docs for public API contracts` |

---

## Befund 4 — Modulpfad enthält derzeit nur Referenzadapter statt dokumentierter Multi-Vendor-Struktur

| Feld | Inhalt |
|---|---|
| **Priorität** | Mittel |
| **Impact** | Erwartungsabweichung zwischen historischer Modulbeschreibung und tatsächlicher Codebasis |
| **Evidence** | `src/chimera/` enthält aktuell `README.md` und `themisdb_adapter.cpp`; keine adapter-spezifischen Dateien wie `adapter_factory.cpp`, `mongodb_adapter.cpp`, `postgresql_adapter.cpp` im Modulpfad |
| **Folge-Issue (Vorschlag)** | `docs(chimera): reconcile README claims with current module file set and ownership boundaries` |

---

## Bereits verifizierte Umsetzungen (kein Gap)

- Streaming-Pfade sind implementiert und testabgedeckt (`tests/chimera/test_chimera_streaming.cpp`).
- Prepared-Statement-Pfade sind implementiert und testabgedeckt (`tests/chimera/test_chimera_prepared_statements.cpp`).
- Primary-Doku-Set in `src/chimera/` wurde für den aktuellen Realstand konsolidiert.
