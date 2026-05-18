# PERFORMANCE_EXPECTATIONS — src/ethics_ai

## Scope
- Modul: `src/ethics_ai`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_ethics_ai_plugin.cpp`
  - `benchmarks/bench_rag_ethics.cpp` (nur RAG-nahe Ethics-Checks, nicht Primärpfad für `src/ethics_ai`)

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| ETH-1 | Siehe Zielbeschreibung: Single Argument Generation P95 | `BM_DiscourseEngine_MakeDecisionSingleSchool` |
| ETH-2 | Siehe Zielbeschreibung: Batch 5 Arguments (parallel, 5 Schulen) | `BM_DiscourseEngine_MakeDecisionFiveSchools` |
| ETH-3 | Siehe Zielbeschreibung: Embedding Latenz (512-Token, CPU) | `BM_RAGContextEngine_VectorSemanticSearch512` |
| ETH-4 | Siehe Zielbeschreibung: Batch 10 Queries | `BM_RAGContextEngine_BuildContextBatch10` |
| ETH-5 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_DiscourseEngine_ContinueDebateRound` |
| ETH-6 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_EthicsEvaluator_RecordDecision` |

## Modulspezifische harte Grenzwerte (v1.9.0)

| Gate-ID | Erwartungswert | Messregel |
|---|---|---|
| ETHG-1 | <= 60 ms (Single Argument Generation P95) | p95 aus `BM_DiscourseEngine_MakeDecisionSingleSchool` |
| ETHG-2 | <= 140 ms (Batch 5 Schulen P99) | p99 aus `BM_DiscourseEngine_MakeDecisionFiveSchools` |
| ETHG-3 | >= 4500 ops/s (Batch-10-Kontextaufbau Throughput) | mean aus `BM_RAGContextEngine_BuildContextBatch10` |
| ETHG-4 | Regression <= 8 % gegen letzte Release-Baseline | Vergleich der Primärbenchmarks `ETH-1..ETH-6` |

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
