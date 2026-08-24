> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

# ThemisDB Tests (`tests/`)

Test-Suite für Unit-, Integrations- und Modulgrenzentests.

## Struktur

Top-Level-Suiten u. a.:

- `tests/geo/`, `tests/llm/`, `tests/security/`, `tests/storage/`, `tests/temporal/`, `tests/timeseries/`
- Roadmap-Scaffolds für die neuen Architektur-Epics: `tests/epic1_retrieval/`, `tests/epic2_evaluation/`, `tests/epic3_distributed_tensor/`
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

- Verbindlicher Standard: `TESTING_STANDARDS.md`
- Historische Summary/Report-Dateien in `tests/` sind Kontext, aber nicht
  kanonisch fuer aktuelle Testregeln.
- Neue Tests als deterministische `test_*.cpp`
- Keine stillen Flaky-Retries in Testlogik
- Fixtures unter `tests/fixtures/`, wenn wiederverwendbar
- Modulnahe Tests über `tests/<module>/CMakeLists.txt` registrieren und dafür
  `themis_register_module_test()` oder `themis_register_module_focused_test()`
  verwenden
- Root-nahe Einzeltests nur registrieren, wenn das zugehörige
  `add_executable()`-Target im selben CMake-Pfad tatsächlich existiert
- `add_test()` nicht auf veraltete oder auskommentierte Targets zeigen lassen;
  fehlende Targets zuerst im Modulpfad reparieren oder die Root-Registrierung
  entfernen

## Bezug

- Build-Quelle der Wahrheit: [`../CMakePresets.json`](../CMakePresets.json)
- Root-Quickstart: [`../README.md`](../README.md)
- Teststandard (kanonisch): [`TESTING_STANDARDS.md`](TESTING_STANDARDS.md)
- Mapping Alt -> Kanonisch (Tests/Benchmarks): [`TEST_BENCHMARK_DOC_CANONICAL_MAPPING.md`](TEST_BENCHMARK_DOC_CANONICAL_MAPPING.md)
