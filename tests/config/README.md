> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

# Test Configuration (`tests/config/`)

Entwickler-Einstieg für testbezogene Konfigurationen und Policy-Profile.

## Struktur

- `policies.test.yaml`: zentrale Test-Policy (z. B. Schwellwerte/Regelwerk für Tests)

## Installation

Es ist keine separate Installation notwendig; die Dateien werden direkt aus dem Repository von Tests geladen.

## Usage

- Konfigurationsänderungen zusammen mit den betroffenen Test-Suites validieren
- Schema-/Policy-Änderungen gegen bestehende `test_*.cpp`-Erwartungen prüfen

## Bezug

- Architekturkontext: [`ARCHITECTURE.md`](ARCHITECTURE.md)
- Rolloutplanung: [`ROADMAP.md`](ROADMAP.md)
- Erweiterungen: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)
- Test-Root: [`../README.md`](../README.md)
