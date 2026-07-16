> **Build:** `cmake --preset nightly-bench-sweep && cmake --build --preset nightly-bench-sweep`

# ANN Benchmarks (`benchmarks/ann/`)

Bereichsdokumentation für ANN-spezifische Benchmark-Szenarien (Approximate Nearest Neighbor).

## Scope

- Vektorindex- und Similarity-Search-nahe Messszenarien
- Recall/Latency/Throughput-Betrachtung im Kontext der Retrieval-Pipeline
- Referenz für ANN-nahe Performanceziele gegenüber allgemeinen Benchmarks in `benchmarks/`

## Layer Mapping (Zielarchitektur)

| Layer | Relevanz | Begründung |
|---|---|---|
| ANN Frontdoor | Hoch | Primärer Fokus auf Vektorsuche und ANN-Verhalten |
| Tensor Mid-Layer | Mittel | Embedding-Qualität beeinflusst Suchergebnisse indirekt |
| Graph Truth Layer | Niedrig | Graph-/Metadatenkontext kann Filter/Scoring flankieren |
| LLM/LoRA Final Layer | Mittel | Retrieval-Qualität wirkt direkt auf RAG-/LLM-Eingaben |

## Installation

Benchmark-Binaries werden über den Preset-Flow erzeugt:

```bash
cmake --preset nightly-bench-sweep
cmake --build --preset nightly-bench-sweep
```

## Usage

- Bereichsspezifische ANN-Auswertungen mit den zentralen `bench_*`-Targets und Orchestrierungs-Skripten unter `benchmarks/` durchführen
- Ergebnisinterpretation mit Fokus auf Recall/Latenz-Trade-offs dokumentieren

## Bezug

- Benchmark-Root: [`../README.md`](../README.md)
- Bereichsplanung: [`../ROADMAP.md`](../ROADMAP.md)
- Erweiterungen: [`../FUTURE_ENHANCEMENTS.md`](../FUTURE_ENHANCEMENTS.md)
- Strategischer Layer-Kontext: [`../../FUTURE_PLAN.md`](../../FUTURE_PLAN.md)
