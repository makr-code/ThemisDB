# ROADMAP (`tests/`)

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

- [x] Root-Testdokumente auf konkrete `tests/`-Struktur und CMake-Realität ausgerichtet (Target: 2026-Q2)
- [x] Veraltete Preset-Referenzen (`linux-ninja-release`) auf `linux-release` aktualisiert (Target: 2026-Q2)

## In Progress

- [ ] Root-Dokumente für `tests/config/` und `tests/data/` ergänzen (Target: 2026-Q2)
- [ ] Schichtbezogene Abdeckungszuordnung (ANN/Tensor/Graph/LLM) aus bestehenden Test-Suites konsolidieren (Target: 2026-Q3)

## Planned Features

- [ ] Dokumentierte Matrix: Produktionsmodul `src/<module>` → verantwortliche Test-Suites (Target: 2026-Q3)
- [ ] Fokus-Targets aus `tests/CMakeLists.txt` als kuratierte Regression-Sets dokumentieren (Target: 2026-Q3)
- [ ] CI-fähige, reproduzierbare Test-Kommandos pro Prioritätsblock dokumentieren (Target: 2026-Q4)

## Implementation Phases

### Phase 1: Dokumentationskonsolidierung
- [x] Root-Dokumente (`README/ARCHITECTURE/ROADMAP/FUTURE_ENHANCEMENTS`) inhaltlich konkretisieren (Target: 2026-Q2)
- [ ] Fehlende Unterbereichs-Readmes (`tests/config`, `tests/data`) ergänzen (Target: 2026-Q2)

### Phase 2: Abdeckungs-Transparenz
- [ ] Modul-zu-Test-Mapping für kritische Runtime-Bereiche veröffentlichen (Target: 2026-Q3)
- [ ] Fokus-Suites mit klaren Akzeptanzkriterien und Ziel-Cadence versehen (Target: 2026-Q3)

### Phase 3: Rollout-Härtung
- [ ] Doku-Änderungen regelmäßig gegen CMake-Presets/Test-Presets verifizieren (Target: 2026-Q4)

## Production Readiness Checklist

- [x] Root-Dokumente sind source-verifizierbar und nicht nur Template-Platzhalter
- [x] Test-Kommandos referenzieren vorhandene Presets
- [ ] Kritische modulübergreifende Regression-Sets explizit dokumentiert
- [ ] Schichtbezogene Abdeckungs-Gaps als priorisierter Backlog gepflegt
