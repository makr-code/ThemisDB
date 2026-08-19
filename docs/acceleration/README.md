# Acceleration Docs Overview

Pfad: `docs/acceleration/`

## Zweck

Diese Seite ist der Einstieg für die technische Modul-Dokumentation rund um das
Acceleration-Modul. Sie ergänzt die Quellcode-Readmes in `src/acceleration/`
und `include/acceleration/`.

## Primäre Modul-Dokumentation

- [`../../src/acceleration/README.md`](../../src/acceleration/README.md) — Modulüberblick, Laufzeitverhalten, Grenzen, Build-Flags
- [`../../include/acceleration/README.md`](../../include/acceleration/README.md) — Public API Header, Entry-Points, Konfigurationsoptionen
- [`../../src/acceleration/ROADMAP.md`](../../src/acceleration/ROADMAP.md) — Umsetzungstand, Phasen, Known Issues, Production Readiness
- [`../../src/acceleration/FUTURE_ENHANCEMENTS.md`](../../src/acceleration/FUTURE_ENHANCEMENTS.md) — Scope, Constraints, Interfaces, Performanceziele

## Vertiefende Themen

- [`capability_negotiation.md`](capability_negotiation.md) — Capability-Scoring, Backend-Auswahl und Fallback-Kette
- [`troubleshooting.md`](troubleshooting.md) — Betriebliches Troubleshooting
- [`error_codes.md`](error_codes.md) — Fehlercode-Taxonomie und Diagnosehilfe
- [`metrics.md`](metrics.md) — Performance- und Beobachtbarkeitsmetriken
- [`production_readiness.md`](production_readiness.md) — Abnahme-/Produktionsreifeaspekte

## Installation

Das Acceleration-Modul wird als Teil von ThemisDB gebaut. Für Linux ist der
aktuelle Configure-/Build-Preset:

```bash
cmake --preset linux-release
cmake --build --preset linux-release
```

## Usage

- Nutze `src/acceleration/README.md` für Implementierungsdetails, Laufzeitverhalten
  und Grenzen.
- Nutze `include/acceleration/README.md` für Public-API-Header, Entry-Points und
  Konfigurationsflächen.
- Nutze `capability_negotiation.md` für den DeviceManager-Probe-Bridge-Vertrag
  (`setEnumerateFn()`, CPU-Fallback-Sentinel, fail-closed Auswahl).
- Nutze `troubleshooting.md` und `error_codes.md` für operative Diagnose und
  Fehleranalyse.

## Hinweise

- Änderungen an API-Verträgen oder Laufzeitverhalten sollten immer zusammen mit
  den Modul-Readmes und den verlinkten Roadmap/Future-Dokumenten aktualisiert werden.
