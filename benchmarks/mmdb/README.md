> **Build:** `cmake --preset nightly-bench-sweep && cmake --build --preset nightly-bench-sweep`

# MMDB Benchmarks (`benchmarks/mmdb/`)

Bereichsdokumentation für multimodale/mehrdimensionale Datenbank-Benchmarks.

## Scope

- Workloads mit kombiniertem Query-, Vektor- und Kontextbezug
- Messung von Latenz, Durchsatz und Ressourcennutzung für multimodale Pfade
- Referenzrahmen für zusammengesetzte Retrieval-/Analyse-Szenarien

## Layer Mapping (Zielarchitektur)

| Layer | Relevanz | Begründung |
|---|---|---|
| ANN Frontdoor | Mittel | Retrieval-/Similarity-Aspekte können Bestandteil sein |
| Tensor Mid-Layer | Mittel | Tensornahe Verarbeitung kann multimodale Pfade prägen |
| Graph Truth Layer | Mittel | Struktur-/Beziehungsinformationen ergänzen Auswertung |
| LLM/LoRA Final Layer | Mittel | Downstream-Generierung profitiert von multimodalem Kontext |

## Installation

```bash
cmake --preset nightly-bench-sweep
cmake --build --preset nightly-bench-sweep
```

## Usage

- MMDB-spezifische Benchmarks als Ergänzung zu ANN-, Graph- und LLM-nahen Messungen betrachten
- Ergebnisse zusammen mit den Root-Benchmarkreports dokumentieren

## Bezug

- Benchmark-Root: [`../README.md`](../README.md)
- Bereichsplanung: [`../ROADMAP.md`](../ROADMAP.md)
- Erweiterungen: [`../FUTURE_ENHANCEMENTS.md`](../FUTURE_ENHANCEMENTS.md)
- Strategischer Layer-Kontext: [`../../FUTURE_PLAN.md`](../../FUTURE_PLAN.md)
