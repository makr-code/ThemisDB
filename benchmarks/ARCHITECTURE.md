# Benchmark Architecture (`benchmarks/`)

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Kontext

`benchmarks/` bündelt C++-Benchmark-Binaries, Ausführungs-Skripte und Auswertungsartefakte für Performance- und Skalierungsanalysen.

## Architektur-Surfaces

| Surface | Ort |
|---|---|
| CMake-Gating / Build-Einstieg | `benchmarks/CMakeLists.txt` (`THEMIS_BUILD_BENCHMARKS`, `themis_add_standard_benchmark`) |
| C++ Benchmark-Ziele | `benchmarks/bench_*.cpp` |
| Orchestrierung / Auswertung | `benchmarks/*.py`, `benchmarks/*.sh`, `benchmarks/scripts/` |
| Suite-Bereiche | `benchmarks/tpc/`, `benchmarks/ycsb/`, `benchmarks/mmdb/`, `benchmarks/ann/`, `benchmarks/ldbc/`, `benchmarks/chimera/` |
| Ergebnis-/Analyseartefakte | `benchmarks/results_analysis_reports/`, `benchmarks/benchmark_results/` |

## Laufzeitmodell

1. CMake aktiviert Benchmarks nur bei `THEMIS_BUILD_BENCHMARKS=ON`.
2. Falls Google Benchmark fehlt, wird der Build mit Warnung beendet (`find_package(benchmark)`).
3. Standard-Benchmarks werden über `themis_add_standard_benchmark(...)` registriert.
4. Skripte orchestrieren mehrstufige Läufe, Baseline-Vergleiche und Report-Generierung.

## Beziehung zur strategischen Zielarchitektur

In Bezug auf `../FUTURE_PLAN.md` dient `benchmarks/` als Messschicht über alle Ziel-Layer:

- ANN Frontdoor: ANN-/Search-Suiten (`benchmarks/ann/`, Retrieval-nahe `bench_*.cpp`)
- Tensor Mid-Layer: Tensor-/Inference-nahe Benchmarks in LLM/Training-nahen Bench-Zielen
- Graph Truth Layer: Graph-/Query-/LDBC-nahe Benchmarks (`benchmarks/ldbc/`, graph/query-orientierte Benchmarks)
- LLM/LoRA Final Layer: LLM-/RAG-orientierte Benchmarks und Orchestrierungs-Skripte

Die Benchmarks dokumentieren Messpfade; sie behaupten nicht, dass jede strategische Layer-Integration bereits vollständig abgeschlossen ist.

## Nicht-Ziele

- keine funktionale Produktdokumentation
- keine Ersetzung von Modul-`PERFORMANCE_EXPECTATIONS.md` in `src/`
- keine automatische Aussage über CI-Abdeckung ohne Workflow-Nachweis

## Sourcecode Verification (Scope: benchmarks/<root>)

- Verifiziert:
  - `benchmarks/CMakeLists.txt`
  - `benchmarks/README.md`
  - `benchmarks/ROADMAP.md`
  - `benchmarks/FUTURE_ENHANCEMENTS.md`
  - Verzeichnisstruktur und vorhandene `bench_*.cpp` / Skript-Einstiege
