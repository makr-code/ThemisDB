# FUTURE_ENHANCEMENTS (`tests/data/`)

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

## Scope

- bessere Transparenz über Zweck und Nutzung jedes Testdatenbereichs
- reproduzierbare Richtlinien für neue Datenartefakte
- konsistente Verbindung zwischen Datenpfaden und Test-Suites

## Design Constraints

- keine impliziten Annahmen über nicht versionierte externe Datenquellen
- keine Nutzung von Testzertifikaten außerhalb des Testkontexts
- Änderungen müssen mit `tests/`-Rootdokumentation konsistent bleiben

## Required Interfaces

- Einstieg: `tests/data/README.md`
- Architektur: `tests/data/ARCHITECTURE.md`
- Rolloutplanung: `tests/data/ROADMAP.md`

## Implementation Notes

- zunächst bestehende Datenpfade und Konsumenten dokumentieren
- zusätzliche Fälle (z. B. weitere Zertifikatstypen) nur mit klarer Testbegründung aufnehmen

## Test Strategy

- Dokument-Linting auf geänderte Dateien (`python3 scripts/docs-lint.py tests/data/*.md`)
- Querverifikation mit `tests/data/certificates/README.md`

## Security / Reliability

- private Testschlüssel strikt als Testmaterial kennzeichnen
- negative Testfälle (abgelaufen/schwach/manipuliert) dauerhaft reproduzierbar halten
