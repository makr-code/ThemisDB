> **Build:** `cmake --preset nightly-bench-sweep && cmake --build --preset nightly-bench-sweep`

# ThemisDB Benchmarks (`benchmarks/`)

Benchmark-Sammlung für Performance-, Skalierungs- und Integrationsmessungen.

## Struktur

- C++ Benchmarks: `bench_*.cpp`
- Python/Script-Orchestrierung: `*.py`, `*.sh`, `*.ps1`
- Teilbereiche: `tpc/`, `ycsb/`, `mmdb/`, `ann/`, `ldbc/`, `chimera/`
- Roadmap-Scaffolds für die neuen Architektur-Epics: `epic1_retrieval/`, `epic2_evaluation/`, `epic3_distributed_tensor/`

## Reproduzierbare Basiskommandos

```bash
cmake --preset nightly-bench-sweep
cmake --build --preset nightly-bench-sweep
ctest --preset linux-release
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

## Methodik-Hinweis

Historische Ergebniszahlen in älteren Reports bleiben als Historie erhalten. Für aktuelle Aussagen sind reproduzierbare Kommandos, Parameter und Artefakte (`results/`, JSON/CSV-Ausgaben) maßgeblich.

## Navigation

- Bereichsindex: [`INDEX.md`](INDEX.md)
- Roadmap: [`ROADMAP.md`](ROADMAP.md)
- Erweiterungen: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)
