# PERFORMANCE_EXPECTATIONS — src/llama_cpp

## Scope
- Modul: `src/llama_cpp`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Dieses Modul nutzt die Ziel-ID-Matrix des Parent-Moduls `llm` als Referenzpfad.
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

## Modulspezifische harte Grenzwerte (v1.9.0)

| Gate-ID | Erwartungswert | Messregel |
|---|---|---|
| LLCPG-1 | <= 1100 ms (TTFT P95) | p95 aus `RealLLMBench_RealModel_TextGeneration_50Tokens` |
| LLCPG-2 | >= 8500 tok/s (Batch Embedding Throughput) | mean aus `RealLLMBench_RealModel_BatchEmbedding_100Docs` |
| LLCPG-3 | <= 75 ms (LoRA Metadata Load P99) | p99 aus `BM_Storage_LoadMetadata` |
| LLCPG-4 | Regression <= 8 % gegen letzte Release-Baseline | `(current - baseline) / baseline` |

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.

## Numerische Mindestziele (Release Gate)

| Gate-ID | Erwartungswert | Messregel |
|---|---|---|
| NG-1 Latenz P95 | <= 50 ms | p95 aus Benchmark-Run (`--benchmark_repetitions=5`) |
| NG-2 Latenz P99 | <= 100 ms | p99 aus Benchmark-Run (`--benchmark_repetitions=5`) |
| NG-3 Throughput-Stabilitaet | Regression <= 10 % gegen letzte Baseline | `(current - baseline) / baseline` |

Hinweis:
- Diese Mindestziele gelten als moduluebergreifende Release-Grenzen solange kein strengeres, modulspezifisches Ziel hinterlegt ist.
- Bei `proxy` oder `not_measurable` bleibt das Ziel numerisch gueltig, wird aber ueber den dokumentierten Proxy-Pfad verifiziert.