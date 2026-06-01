# ROADMAP (`include/`)

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

- [x] Root-Header-Doku (`README.md`, `ARCHITECTURE.md`, `ROADMAP.md`, `FUTURE_ENHANCEMENTS.md`) für `include/` konsolidiert (Target: 2026-Q2)
- [x] Build-Kommandos auf aktuelle Presets (`linux-release`) normalisiert (Target: 2026-Q2)

## In Progress

- [ ] Modulweise Header-Doku-Priorisierung nach Konsumentenwirkung (Target: 2026-Q2)
- [ ] Verknüpfung kritischer Header-Surfaces mit den korrespondierenden `src/<module>/ARCHITECTURE.md`-Sektionen (Target: 2026-Q3)

## Planned Features

- [ ] API-Vertragsindex für häufig genutzte Header-Einstiege je Schicht (ANN/Tensor/Graph/LLM) ergänzen (Target: 2026-Q3)
- [ ] Pro priorisiertem Modul explizite Kompatibilitäts-/Deprecation-Hinweise dokumentieren (Target: 2026-Q3)
- [ ] Header-Dokumentation mit reproduzierbaren Compile-Checks in CI koppeln (Target: 2026-Q4)

## Production Readiness Checklist

- [x] Root-Dokumente für `include/` vorhanden und verlinkt
- [x] Preset-Kommandos entsprechen `CMakePresets.json`
- [ ] Priorisierte Header-Surfaces mit Akzeptanzkriterien dokumentiert
- [ ] Änderungsverfolgung (Breaking/Deprecation) je Modul konsistent
