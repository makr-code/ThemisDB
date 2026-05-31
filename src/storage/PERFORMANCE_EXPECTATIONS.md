# PERFORMANCE_EXPECTATIONS — src/storage

## Scope
- Modul: `src/storage`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_storage_performance.cpp`
  - `benchmarks/bench_tpcc.cpp`
  - `benchmarks/bench_vector_search.cpp`

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| BM-1 | OLTP New-Order darf keine signifikante Regression ggü. Baseline zeigen | `BENCHMARK_REGISTER_F(TPCCLiteFixture, NewOrderTransaction)` |
| BM-2 | OLTP Payment darf keine signifikante Regression ggü. Baseline zeigen | `BENCHMARK_REGISTER_F(TPCCLiteFixture, PaymentTransaction)` |
| BM-3 | OLTP Stock-Level darf keine signifikante Regression ggü. Baseline zeigen | `BENCHMARK_REGISTER_F(TPCCLiteFixture, StockLevelTransaction)` |
| BM-4 | Vector-Search efSearch bleibt im Baseline-Korridor | `BM_VectorSearch_efSearch` |
| BM-5 | Sustained write no-sync bleibt im Baseline-Korridor | `BM_Storage_SustainedWrite_NoSync` |
| BM-6 | Sustained batched write bleibt im Baseline-Korridor | `BM_Storage_SustainedWrite_Batched` |
| BM-7 | WAL group-commit batch bleibt im Baseline-Korridor | `BM_WAL_GroupCommit_Batch` |

## Modulspezifische harte Grenzwerte (v1.9.0)

| Gate-ID | Erwartungswert | Messregel |
|---|---|---|
| SG-1 | >= 18000 txn/s (TPC-C NewOrder Throughput) | mean aus `BENCHMARK_REGISTER_F(TPCCLiteFixture, NewOrderTransaction)` |
| SG-2 | <= 35 ms (TPC-C StockLevel Latenz P95) | p95 aus `BENCHMARK_REGISTER_F(TPCCLiteFixture, StockLevelTransaction)` |
| SG-3 | <= 30 ms (VectorSearch Latenz P99) | p99 aus `BM_VectorSearch_efSearch` |
| SG-4 | Regression <= 8 % gegen letzte Release-Baseline | `(current - baseline) / baseline` |

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

## Sourcecode Verification (Module: storage/performance)

- Gepruefte Benchmark-Quellen:
  - `benchmarks/bench_storage_performance.cpp`
  - `benchmarks/bench_tpcc.cpp`
  - `benchmarks/bench_vector_search.cpp`
- Gepruefte Ziel-Fall-Zuordnung:
  - TPC-C fixture benchmarks (`BENCHMARK_REGISTER_F(TPCCLiteFixture, ...)`)
  - storage sustained-write and WAL group-commit benchmarks (`BM_Storage_*`, `BM_WAL_GroupCommit_Batch`)
  - vector search regression signal (`BM_VectorSearch_efSearch`)
- Ergebnis:
  - Referenzierte Benchmark-Faelle existieren im aktuellen Benchmark-Source.
  - Erwartungswerte bleiben an reproduzierbare Release-Profil-Messungen und Baseline-Vergleich gebunden.