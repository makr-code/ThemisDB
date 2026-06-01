> **Build:** `cmake --preset nightly-bench-sweep && cmake --build --preset nightly-bench-sweep`

# TPC Benchmarks (`benchmarks/tpc/`)

Bereichsdokumentation für transaktions- und analyseorientierte TPC-nahe Lastprofile.

## Scope

- OLTP/OLAP-nahe Referenzszenarien (TPC-C/TPC-H-Kontext)
- baselineartige Messung von Query-, Storage- und Transaktionspfaden
- Vergleichbarkeit gegenüber anderen Datenbanksystem-orientierten Benchmarks

## Layer Mapping (Zielarchitektur)

| Layer | Relevanz | Begründung |
|---|---|---|
| ANN Frontdoor | Niedrig | Fokus liegt nicht auf Approximate-NN-Suche |
| Tensor Mid-Layer | Niedrig | Tensorpfade sind nicht Kern von TPC-Baselines |
| Graph Truth Layer | Hoch | Konsistente Query-/Transaktionsausführung ist zentral |
| LLM/LoRA Final Layer | Niedrig | TPC misst primär Datenbank-Kernverhalten |

## Installation

```bash
cmake --preset nightly-bench-sweep
cmake --build --preset nightly-bench-sweep
```

## Usage

- TPC-nahe Benchmarks als Baseline für Kern-Datenbankpfade verwenden
- bei Vergleichen Konfigurationsbezug über `tpc_c_config.yaml` und `tpc_h_README.md` einbeziehen

## Bezug

- Benchmark-Root: [`../README.md`](../README.md)
- TPC-H Zusatzhinweise: [`tpc_h_README.md`](tpc_h_README.md)
- Bereichsplanung: [`../ROADMAP.md`](../ROADMAP.md)
- Strategischer Layer-Kontext: [`../../FUTURE_PLAN.md`](../../FUTURE_PLAN.md)
