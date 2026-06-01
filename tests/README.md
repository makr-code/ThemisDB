> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

# ThemisDB Tests (`tests/`)

Test-Suite für Unit-, Integrations- und Modulgrenzentests.

## Struktur

Top-Level-Suiten u. a.:

- `tests/geo/`, `tests/llm/`, `tests/security/`, `tests/storage/`, `tests/temporal/`, `tests/timeseries/`
- zusätzliche root-nahe `test_*.cpp` für cross-modulare Regressionen

## Ausführung (aktueller CMake-Flow)

```bash
cmake --preset linux-release
cmake --build --preset linux-release
ctest --preset linux-release
```

Gezielt (Beispiel):

```bash
ctest --preset linux-release -R temporal
```

## Installation

Tests werden als Teil des Standard-Builds erzeugt (`THEMIS_BUILD_TESTS=ON` im Release-Preset).

## Usage

Führen Sie `ctest` vollständig oder gefiltert (`-R`) aus, um betroffene Module gezielt zu prüfen.

## Richtlinien

- Neue Tests als deterministische `test_*.cpp`
- Keine stillen Flaky-Retries in Testlogik
- Fixtures unter `tests/fixtures/`, wenn wiederverwendbar

## Bezug

- Build-Quelle der Wahrheit: [`../CMakePresets.json`](../CMakePresets.json)
- Root-Quickstart: [`../README.md`](../README.md)
