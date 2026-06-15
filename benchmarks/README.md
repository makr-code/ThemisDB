> **Build:** `cmake --preset nightly-bench-sweep && cmake --build --preset nightly-bench-sweep`

# ThemisDB Benchmarks (`benchmarks/`)

Benchmark-Sammlung für Performance-, Skalierungs- und Integrationsmessungen.

## Struktur

- C++ Benchmarks: `bench_*.cpp`
- Python/Script-Orchestrierung: `*.py`, `*.sh`, `*.ps1`
- Teilbereiche: `ai/`, `analytics/`, `aql/`, `core/`, `query/`, `rag/`, `transaction/`, `tpc/`, `ycsb/`, `mmdb/`, `ann/`, `ldbc/`, `chimera/`
- Roadmap-Scaffolds für die neuen Architektur-Epics: `epic1_retrieval/`, `epic2_evaluation/`, `epic3_distributed_tensor/`

## Reproduzierbare Basiskommandos

```bash
cmake --preset nightly-bench-sweep
cmake --build --preset nightly-bench-sweep
ctest --preset linux-release
```

Registrierungs-Audit:

```bash
python3 benchmarks/scripts/audit_benchmark_registration.py
```

Direkte Script-Läufe (Beispiele):

```bash
python3 benchmarks/hardware_scaling_benchmark.py --help
python3 benchmarks/run_benchmark_orchestrator.py --help
bash benchmarks/run_all_benchmarks.sh
```

## Installation

Benchmark-Buildartefakte werden über den CMake-Preset `nightly-bench-sweep` erzeugt.

## Usage

Ausführung erfolgt entweder über erzeugte Benchmark-Binaries (`bench_*`) oder über Orchestrierungs-Skripte in diesem Verzeichnis.

## CMake-Registrierungspolicy (Build/CTest-konform)

- **Top-Level Benchmarks** (`benchmarks/bench*.cpp`, `benchmark_*.cpp`, `llm_bench.cpp`) bleiben nur für noch nicht migrierte oder bewusst globale Targets im Root erhalten.
- **Modul-Benchmarks** folgen dem Muster `benchmarks/<module>/CMakeLists.txt` (analog `tests/<module>/...`), z. B. `benchmarks/ai/`, `benchmarks/analytics/`, `benchmarks/aql/`, `benchmarks/core/`, `benchmarks/query/`, `benchmarks/rag/`, `benchmarks/transaction/`, `benchmarks/performance_optimizations/`.
- **Manuelle/Script-basierte Benchmarks** (`*.py`, `*.sh`, `*.ps1`) sind absichtlich **nicht** als CTest-Gates registriert und werden über ihre Runner ausgeführt.
- **Intentionale Exclusions** bleiben explizit dokumentiert (aktuell: `performance_optimizations/phase2/benchmark_phase2.cpp`, Placeholder/disabled).

Hinweis zur CTest-Integration: Benchmarks sind standardmäßig keine obligatorischen CTest-Checks; sie werden über dedizierte Benchmark-Presets/Runner und Build-Targets ausgeführt.

## Methodik-Hinweis

Historische Ergebniszahlen in älteren Reports bleiben als Historie erhalten. Für aktuelle Aussagen sind reproduzierbare Kommandos, Parameter und Artefakte (`results/`, JSON/CSV-Ausgaben) maßgeblich.

## Navigation

- Bereichsindex: [`INDEX.md`](INDEX.md)
- Roadmap: [`ROADMAP.md`](ROADMAP.md)
- Erweiterungen: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)
