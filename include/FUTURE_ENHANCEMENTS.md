# FUTURE_ENHANCEMENTS (`include/`)

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

## Scope

- Ausbau der entwicklerorientierten Header-Dokumentation in `include/`
- präzisere Zuordnung öffentlicher Verträge zu den strategischen Schichten aus `FUTURE_PLAN.md`
- bessere Nachvollziehbarkeit, welche Header für stabile Integration vorgesehen sind

## Design Constraints

- keine spekulativen API-Behauptungen ohne Header-/Source-Bezug
- keine Vermischung von öffentlichem Vertrag und privater Implementierungslogik
- Änderungen müssen mit existierender Modulstruktur unter `include/` konsistent bleiben

## Required Interfaces

- Navigationspfad über `include/README.md`
- Architekturkontext über `include/ARCHITECTURE.md`
- priorisierte Rollout-Schritte über `include/ROADMAP.md`

## Implementation Notes

- zunächst Root-Ebene konsolidieren, danach modulweise nach Risiko/Nutzungsgrad vertiefen
- bei strategischen Layer-Bezügen (ANN/Tensor/Graph/LLM) explizit als Plan/Gap markieren, wenn noch keine vollständige Umsetzung vorliegt
- Link-Konsistenz zu `src/`-Dokumenten als harte Mindestanforderung behandeln

## Test Strategy

- Dokument-Linting auf betroffene Dateien (`python3 scripts/docs-lint.py include/*.md`)
- Preset-Referenzen gegen `CMakePresets.json` querprüfen

## Security / Reliability

- öffentliche Header als stabile Vertragsgrenze dokumentieren
- keine inoffiziellen oder internen Pfade als öffentliche API ausweisen
