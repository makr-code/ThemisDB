> **Build:** `cmake --preset nightly-bench-sweep && cmake --build --preset nightly-bench-sweep`

# LDBC Benchmarks (`benchmarks/ldbc/`)

Bereichsdokumentation für graphzentrierte Benchmark-Szenarien im LDBC/Social-Graph-Umfeld.

## Scope

- Traversal-, Pfad- und Graph-Query-nahe Messszenarien
- Interaktive Graph-Workloads mit Fokus auf Antwortzeit und Skalierung
- Vergleichsrahmen für graphorientierte Lastprofile im Benchmark-Portfolio

## Layer Mapping (Zielarchitektur)

| Layer | Relevanz | Begründung |
|---|---|---|
| ANN Frontdoor | Niedrig | Vektorsuche steht nicht im Vordergrund |
| Tensor Mid-Layer | Niedrig | Tensorpfade sind nicht primärer LDBC-Messgegenstand |
| Graph Truth Layer | Hoch | Kerndomäne ist graphbasierte Anfrageausführung |
| LLM/LoRA Final Layer | Niedrig | LLM-Anteile sind hier nicht primär benchmarkrelevant |

## Installation

Benchmark-Build erfolgt über den standardisierten Sweep-Preset:

```bash
cmake --preset nightly-bench-sweep
cmake --build --preset nightly-bench-sweep
```

## Usage

- LDBC-nahe Szenarien im Kontext der zentralen Benchmark-Targets auswerten
- Ergebnisse gegen allgemeine Graph-/Query-Benchmarks unter `benchmarks/` einordnen

## Bezug

- Benchmark-Root: [`../README.md`](../README.md)
- Bereichsplanung: [`../ROADMAP.md`](../ROADMAP.md)
- Erweiterungen: [`../FUTURE_ENHANCEMENTS.md`](../FUTURE_ENHANCEMENTS.md)
- Strategischer Layer-Kontext: [`../../FUTURE_PLAN.md`](../../FUTURE_PLAN.md)
