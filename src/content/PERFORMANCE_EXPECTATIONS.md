# PERFORMANCE_EXPECTATIONS — src/content

## Scope
- Modul: `src/content`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Dieses Modul nutzt die Ziel-ID-Matrix des Parent-Moduls `search` als Referenzpfad.
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_rag_hybrid_retriever.cpp`

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| SE-1 | Siehe Zielbeschreibung: Hybrid Search P99 (10M-Doc-Index) | `BM_RRF_Balanced` |
| SE-2 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_RRF_BM25Only` |
| SE-3 | Siehe Zielbeschreibung: Facet Counting (1k distinct, 100k Docs) | `BM_RRF_VectorOnly` |
| SE-4 | Siehe Zielbeschreibung: LTR Re-Ranking (Top-100) | `BM_Linear_Balanced` |
| SE-5 | Siehe Zielbeschreibung: Autocomplete P99 (1M-Term-Dict) | `BM_ConfigConstruction` |
| SE-6 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_RRF_Disjoint` |

## Modulspezifische harte Grenzwerte (v1.9.0)

| Gate-ID | Erwartungswert | Messregel |
|---|---|---|
| CTG-1 | <= 85 ms (Hybrid Search P99) | p99 aus `BM_RRF_Balanced` |
| CTG-2 | >= 14000 qps (BM25-Only Throughput) | mean aus `BM_RRF_BM25Only` |
| CTG-3 | <= 28 ms (LTR Re-Ranking P95) | p95 aus `BM_Linear_Balanced` |
| CTG-4 | Regression <= 8 % gegen letzte Release-Baseline | `(current - baseline) / baseline` |

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