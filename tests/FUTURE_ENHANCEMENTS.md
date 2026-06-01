# FUTURE_ENHANCEMENTS (`tests/`)

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

## Scope

- Ausbau der Entwicklerdokumentation für reproduzierbare Testausführung
- transparentere Zuordnung von Test-Suites zu Produktionsmodulen und Zielschichten
- schrittweises Schließen von Doku-Gaps in bislang unterdokumentierten Test-Unterbereichen

## Design Constraints

- keine Behauptung von Testabdeckung ohne nachweisbare Suite/Datei im Repository
- keine Spekulation über nicht vorhandene CI-Pipelines oder externe Infrastruktur
- Preset-/Command-Referenzen müssen auf vorhandene `CMakePresets.json`-Namen zeigen

## Required Interfaces

- Root-Einstieg: `tests/README.md`
- Architekturkontext: `tests/ARCHITECTURE.md`
- Rolloutplanung: `tests/ROADMAP.md`
- Build/Test-Quelle der Wahrheit: `../CMakePresets.json`, `tests/CMakeLists.txt`

## Implementation Notes

- zuerst Root-Dokumentation konsolidieren, dann Bereichsdokumente (`tests/config`, `tests/data`) ergänzen
- Layer-Bezug (ANN/Tensor/Graph/LLM) als Abdeckungs- und Planungsmodell dokumentieren, nicht als Vollständigkeitsclaim
- Focused-Targets als eigene Regression-Blöcke sichtbar machen

## Test Strategy

- Dokument-Linting auf geänderte Dateien (`python3 scripts/docs-lint.py tests/*.md`)
- Preset-Referenzen gegen `CMakePresets.json` validieren
- bei inhaltlichen CMake-Aussagen Querverifikation gegen `tests/CMakeLists.txt`

## Security / Reliability

- sicherheitskritische Testblöcke (`tests/security/`, `tests/integration/security/`) klar auffindbar halten
- deterministische, nachvollziehbare Testpfade statt impliziter Ausführungsannahmen
