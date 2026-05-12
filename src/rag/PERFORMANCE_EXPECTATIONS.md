# PERFORMANCE_EXPECTATIONS — src/rag

## Scope
- Modul: `src/rag`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_rag_hybrid_retriever.cpp`
  - `benchmarks/bench_rag_ethics.cpp`

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| RA-1 | Siehe Zielbeschreibung: Fast Evaluation P99 | `BM_RRF_Balanced` |
| RA-2 | Siehe Zielbeschreibung: Balanced Evaluation P99 | `BM_RRF_Balanced` |
| RA-3 | Siehe Zielbeschreibung: Thorough Evaluation P99 | `BM_RRF_Disjoint` |
| RA-4 | Siehe Zielbeschreibung: HybridRetriever Recall@10 | `BM_RRF_Balanced` |
| RA-5 | Siehe Zielbeschreibung: CrossEncoderReranker MRR@10 | `BM_RRF_BM25Only` |
| RA-6 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_RRF_VectorOnly` |
| RA-7 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_Linear_Balanced` |
| RA-8 | Siehe Zielbeschreibung: ClaimExtractor (1k Zeichen) | `BM_EthicalCompliance_Full_Good` |

## Modulspezifische harte Grenzwerte (v1.9.0)

| Gate-ID | Erwartungswert | Messregel |
|---|---|---|
| RAGG-1 | <= 90 ms (Fast/Balanced Evaluation P99) | p99 aus `BM_RRF_Balanced` |
| RAGG-2 | <= 120 ms (Thorough Evaluation P99) | p99 aus `BM_RRF_Disjoint` |
| RAGG-3 | >= 12000 qps (Vector-Only Throughput) | mean aus `BM_RRF_VectorOnly` |
| RAGG-4 | <= 35 ms (ClaimExtractor P95) | p95 aus `BM_EthicalCompliance_Full_Good` |
| RAGG-5 | Regression <= 8 % gegen letzte Release-Baseline | `(current - baseline) / baseline` |

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