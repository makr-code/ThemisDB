# ROADMAP (`include/`)

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

- [x] Root-Header-Doku (`README.md`, `ARCHITECTURE.md`, `ROADMAP.md`, `FUTURE_ENHANCEMENTS.md`) für `include/` konsolidiert (Target: 2026-Q2)
- [x] Build-Kommandos auf aktuelle Presets (`linux-release`) normalisiert (Target: 2026-Q2)
- [x] Modulweise Priorisierung nach API-Risiko/Nutzungsgrad inklusive Akzeptanzkriterien erfasst (Target: 2026-Q2)

## In Progress

- [x] Verknüpfung kritischer Header-Surfaces mit den korrespondierenden `src/<module>/ARCHITECTURE.md`-Sektionen (Target: 2026-Q3) — abgeschlossen 2026-06-01
- [x] Prioritätsblock A (Core-Runtime-APIs) vollständig gegen Header- und Implementierungsstand verifiziert — `core`, `storage`, `query`, `transaction`, `server`, `security`, `network`, `api` je mit `ARCHITECTURE.md` + `ROADMAP.md` + `FUTURE_ENHANCEMENTS.md` abgedeckt (abgeschlossen 2026-06-01)

## Priorisierte Modulblöcke (API-Risiko/Nutzungsgrad)

| Priorität | Module | Risiko-/Nutzungsrationale | Akzeptanzkriterien |
|---|---|---|---|
| A (kritisch) | `core`, `storage`, `query`, `transaction`, `server`, `security`, `network`, `api` | zentrale Runtime- und Vertragsflächen mit hoher Konsumentenwirkung | je Modul: `README+ARCHITECTURE+ROADMAP+FUTURE_ENHANCEMENTS` source-verifiziert, öffentliche Entry-Header benannt, Breaking-/Deprecation-Hinweise dokumentiert |
| B (hoch) | `auth`, `sharding`, `replication`, `metadata`, `observability`, `performance`, `cache`, `search` | häufig integrierte Plattform- und Betriebsflächen | Modul-Doku auf aktuelle Headerstruktur abgeglichen, Schnittstellen zu A-Modulen und `src/`-Dokumenten explizit verlinkt |
| C (mittel) | `analytics`, `temporal`, `timeseries`, `geo`, `rag`, `llm`, `training`, `voice`, `plugins` | domänenspezifische, teils optionale oder ausbaubare API-Flächen | Kern-Headerpfade dokumentiert, Layer-Bezug (ANN/Tensor/Graph/LLM) markiert, offene Gaps explizit als geplant gekennzeichnet |
| D (tail) | verbleibende Spezial-/Supportmodule unter `include/` | geringere direkte Konsumentenbreite oder Hilfscharakter | minimaler Developer-Einstieg vorhanden, Verweise auf zuständige Hauptmodule gesetzt |

## Planned Features

- [ ] API-Vertragsindex für häufig genutzte Header-Einstiege je Schicht (ANN/Tensor/Graph/LLM) ergänzen (Target: 2026-Q3)
- [ ] Pro priorisiertem Modul explizite Kompatibilitäts-/Deprecation-Hinweise dokumentieren (Target: 2026-Q3)
- [ ] Header-Dokumentation mit reproduzierbaren Compile-Checks in CI koppeln (Target: 2026-Q4)

## Production Readiness Checklist

- [x] Root-Dokumente für `include/` vorhanden und verlinkt
- [x] Preset-Kommandos entsprechen `CMakePresets.json`
- [x] Priorisierte Header-Surfaces mit Akzeptanzkriterien dokumentiert
- [ ] Änderungsverfolgung (Breaking/Deprecation) je Modul konsistent
