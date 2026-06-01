> **Build:** `cmake --preset nightly-bench-sweep && cmake --build --preset nightly-bench-sweep`

# YCSB Benchmarks (`benchmarks/ycsb/`)

Bereichsdokumentation für Throughput-/Latenz-Benchmarks im YCSB-ähnlichen KV-Workloadprofil.

## Scope

- CRUD-lastige Lastprofile mit Fokus auf Storage- und Zugriffspfadverhalten
- Vergleich von Latenzverteilungen und Throughput unter variierenden Workload-Mixen
- Baseline für skalierungsnahe Änderungen im Kernsystem

## Layer Mapping (Zielarchitektur)

| Layer | Relevanz | Begründung |
|---|---|---|
| ANN Frontdoor | Niedrig | Vektorsuche ist nicht primärer Messgegenstand |
| Tensor Mid-Layer | Niedrig | Tensorverarbeitung steht nicht im Zentrum |
| Graph Truth Layer | Mittel | konsistente Datenzugriffspfade bleiben Grundlage |
| LLM/LoRA Final Layer | Niedrig | YCSB adressiert primär Kern-Datenpfade |

## Installation

```bash
cmake --preset nightly-bench-sweep
cmake --build --preset nightly-bench-sweep
```

## Usage

- YCSB-nahe Lastprofile als Storage-/KV-Baselines verwenden
- Ergebnisse zusammen mit den zentralen Benchmark-Reports unter `benchmarks/` auswerten

## Bezug

- Benchmark-Root: [`../README.md`](../README.md)
- Bereichsplanung: [`../ROADMAP.md`](../ROADMAP.md)
- Erweiterungen: [`../FUTURE_ENHANCEMENTS.md`](../FUTURE_ENHANCEMENTS.md)
- Strategischer Layer-Kontext: [`../../FUTURE_PLAN.md`](../../FUTURE_PLAN.md)
