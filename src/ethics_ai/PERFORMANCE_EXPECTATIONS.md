# PERFORMANCE_EXPECTATIONS — src/ethics_ai

## Scope
- Modul: `src/ethics_ai`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_rag_ethics.cpp`

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| ETH-1 | Siehe Zielbeschreibung: Single Argument Generation P95 | `BM_EthicalCompliance_Full_Good` |
| ETH-2 | Siehe Zielbeschreibung: Batch 5 Arguments (parallel, 5 Schulen) | `BM_MoralDiversity_MultiFramework` |
| ETH-3 | Siehe Zielbeschreibung: Embedding Latenz (512-Token, CPU) | `BM_EthicalCompliance_Disabled` |
| ETH-4 | Siehe Zielbeschreibung: Batch 10 Queries | `BM_AutonomyRespect_Good` |
| ETH-5 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_MoralDiversity_MultiFramework` |
| ETH-6 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_EthicalCompliance_Disabled` |

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.
