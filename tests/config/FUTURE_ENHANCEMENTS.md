# FUTURE_ENHANCEMENTS (`tests/config/`)

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

## Scope

- bessere Nachvollziehbarkeit testrelevanter Konfigurationsparameter
- klarere Zuordnung von Policy-Änderungen zu betroffenen Test-Suites
- reproduzierbare Review-Regeln für Konfigurationsänderungen

## Design Constraints

- keine Annahmen ohne nachvollziehbare Referenz auf vorhandene Konfigurationsdateien
- keine Vermischung mit Produktionskonfiguration
- Änderungen müssen mit `tests/`-Rootdokumentation konsistent sein

## Required Interfaces

- Einstieg: `tests/config/README.md`
- Architektur: `tests/config/ARCHITECTURE.md`
- Rolloutplanung: `tests/config/ROADMAP.md`

## Implementation Notes

- zuerst Policy-Keys und Konsumenten dokumentieren, danach Detailregeln ergänzen
- Abweichungen explizit als Gap statt als erledigt markieren

## Test Strategy

- Dokument-Linting auf geänderte Dateien (`python3 scripts/docs-lint.py tests/config/*.md`)
- Konsistenzcheck gegen `tests/README.md` und `tests/CMakeLists.txt`

## Security / Reliability

- testnahe Sicherheits-/Policy-Defaults müssen nachvollziehbar bleiben
- Konfigurationsänderungen ohne zugehörige Verifikation vermeiden
