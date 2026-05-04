[docs](../../README.md) > [de](../README.md) > [chimera](./index.md) > [README](./README.md)
**Datum:** 2026-04-16
**Status:** review
**Primary (Quelle der Wahrheit):**
- `src/chimera/README.md`
- `src/chimera/ARCHITECTURE.md`
- `src/chimera/ROADMAP.md`
- `src/chimera/FUTURE_ENHANCEMENTS.md`
- `src/chimera/CHANGELOG.md`
- `src/chimera/themisdb_adapter.cpp`
- `include/chimera/README.md`
- `include/chimera/themisdb_adapter.hpp`
- `include/chimera/database_adapter.hpp`
- `tests/chimera/test_chimera_streaming.cpp`
- `tests/chimera/test_chimera_prepared_statements.cpp`

**Bezug / Reference:**
- Issue: `[MODULE] chimera`
- Kontext: Secondary-Doku für Reality-Check, Roadmap-Verifikation und Doku-Migration des Moduls `chimera`.

---

# chimera-Modul — Überblick und Verifikationsstand

## TL;DR

Das Modul `chimera` enthält aktuell primär den **ThemisDB-Referenzadapter** mit in-process Simulationspfaden und optionalem engine-backed Dispatch.
Streaming und Prepared Statements sind implementiert und durch dedizierte Tests abgedeckt.

## Scope (Task 1: Reality-Check)

- **Implementierung:** `src/chimera/themisdb_adapter.cpp`
- **Public API:** `include/chimera/themisdb_adapter.hpp`, `include/chimera/database_adapter.hpp`
- **Tests:** `tests/chimera/test_chimera_streaming.cpp`, `tests/chimera/test_chimera_prepared_statements.cpp`
- **Primary-Doku:** `src/chimera/{README,ARCHITECTURE,ROADMAP,FUTURE_ENHANCEMENTS,CHANGELOG}.md`

## Verifizierte Aussagen (Tasks 1–3)

1. **Primary-Doku gegen Code abgeglichen**
   - Der Implementierungsfokus liegt in `ThemisDBAdapter`; weitere Vendor-Adapter oder eine Adapter-Factory sind im Modulpfad `src/chimera/` derzeit nicht vorhanden.
   - Simulationsmodus + optionale Engine-Injection sind im Code sichtbar (`themisdb_adapter.cpp`) und in der aktualisierten Primary-Doku explizit erfasst.
2. **ROADMAP/FUTURE_ENHANCEMENTS verifiziert**
   - Neue `src/chimera/ROADMAP.md` bildet den aktuellen Stand phasenweise mit offenen Punkten und Targets ab.
   - Neue `src/chimera/FUTURE_ENHANCEMENTS.md` folgt dem geforderten implementierbaren Format (`Scope`, `Design Constraints`, `Required Interfaces`, `Implementation Notes`, `Test Strategy`, `Performance Targets`, `Security / Reliability`).
3. **Research-/Entscheidungshinweise verankert**
   - Build-Flag-abhängige Grenzen (`THEMISDB_ENGINE_AVAILABLE`) und Capability-Mismatch wurden als zentrale Constraints in Roadmap/Future und im Missing-Report verankert.

## Offene Punkte / Risiken

- Include-Dokumentation (`include/chimera/README.md`) wurde ergänzt — API-Referenz für beide Header-Dateien ist nun vorhanden.
- Es gibt Capability-Claims (z. B. Connection Pooling), die nicht vollständig durch konkrete Implementierung im Adapter gespiegelt sind.
- Engine-backed Pfade enthalten weiterhin `NOT_IMPLEMENTED`-Fehlerpfade bei fehlender Build-Flag-Aktivierung.

Details und Priorisierung:
- [MISSING_IMPLEMENTATIONS.md](./MISSING_IMPLEMENTATIONS.md)
