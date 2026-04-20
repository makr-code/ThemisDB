# PERFORMANCE_EXPECTATIONS — src/llm

## Scope
- Modul: `src/llm`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_llm_real_models.cpp`
  - `benchmarks/bench_lora_framework.cpp`

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| L-1 | Siehe Zielbeschreibung: Time-to-First-Token (512-Token, A10G) | `RealLLMBench_RealModel_TextGeneration_50Tokens` |
| L-2 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `RealLLMBench_RealModel_TextEmbedding_Generation` |
| L-3 | Siehe Zielbeschreibung: LoRA Adapter Hot-Load (7B, Rank 64) | `BM_Storage_LoadMetadata` |
| L-4 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_Storage_SaveAdapter_64KB` |
| L-5 | Siehe Zielbeschreibung: Work-Stealing Dispatch P99 | `BM_Orchestrator_HealthCheck` |
| L-6 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `RealLLMBench_RealModel_ContextScaling` |
| L-7 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `RealLLMBench_RealModel_BatchEmbedding_100Docs` |
| L-8 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `RealLLMBench_RealModel_ContextScaling` |

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.
