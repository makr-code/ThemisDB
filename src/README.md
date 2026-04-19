# ThemisDB Source (`src/`)

Dieses Verzeichnis enthält die produktive Implementierung von ThemisDB.

## Struktur

- 58 Top-Level-Module (z. B. `api`, `query`, `storage`, `llm`, `temporal`, `sharding`)
- Modul-Doku pro Modul in der Regel über:
  - `README.md`
  - `ROADMAP.md`
  - `FUTURE_ENHANCEMENTS.md`

## Einstieg

- Root-Überblick: [`../README.md`](../README.md)
- Modulstatus: [`../roadmap.md`](../roadmap.md)
- Öffentliche Header-Gegenstücke: [`../include/README.md`](../include/README.md)

## Installation

Build erfolgt über den Root-Buildprozess von ThemisDB.

## Build/Test (aktueller Flow)

```bash
cmake --list-presets
cmake --preset linux-ninja-release
cmake --build --preset linux-ninja-release
ctest --preset linux-ninja-release
```

## Usage

Die Module in `src/` werden über ThemisDB-Binaries und APIs genutzt; direkte Nutzung erfolgt modulabhängig über die öffentlichen Header in `include/`.

## Offene Doku-Tasks (rekursiv)

- Modul-Readmes mit konkreten Symbol-/Dateireferenzen weiter vereinheitlichen.
- Historische Aussagen ohne Codebeleg in Modulunterordnern schrittweise als historisch markieren.
