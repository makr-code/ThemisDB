> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

# Test Data (`tests/data/`)

Entwickler-Einstieg für statische Testdaten und Zertifikatsartefakte.

## Struktur

- `certificates/`: Zertifikate, Schlüssel und Hilfsskripte für Security-/TLS-nahe Tests

## Installation

Keine separate Installation nötig; Testdaten werden direkt aus dem Repository verwendet.

## Usage

- Testdaten nur erweitern, wenn betroffene Tests reproduzierbar aktualisiert sind
- Zertifikatsmaterial aus `certificates/` nur im Testkontext verwenden

## Bezug

- Architekturkontext: [`ARCHITECTURE.md`](ARCHITECTURE.md)
- Rolloutplanung: [`ROADMAP.md`](ROADMAP.md)
- Erweiterungen: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)
- Test-Root: [`../README.md`](../README.md)
