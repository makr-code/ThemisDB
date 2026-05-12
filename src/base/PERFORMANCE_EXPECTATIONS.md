# PERFORMANCE_EXPECTATIONS — src/base

## Scope
- Modul: `src/base`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Dieses Modul nutzt die Ziel-ID-Matrix des Parent-Moduls `system_level` als Referenzpfad.
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_tpcc.cpp`
  - `benchmarks/bench_vector_search.cpp`

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| BM-1 | Siehe Zielbeschreibung: OLTP (TPC-C) 4-Core | `TPCCLiteFixture_NewOrderTransaction` |
| BM-2 | Siehe Zielbeschreibung: OLTP (TPC-C) 8-Core | `TPCCLiteFixture_NewOrderTransaction` |
| BM-3 | Siehe Zielbeschreibung: OLTP (TPC-C) 16-Core | `TPCCLiteFixture_NewOrderTransaction` |
| BM-4 | Siehe Zielbeschreibung: OLTP (TPC-C) 32-Core | `TPCCLiteFixture_NewOrderTransaction` |
| BM-5 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `TPCCLiteFixture_StockLevelTransaction` |
| BM-6 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_VectorSearch_efSearch` |
| BM-7 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `TPCCLiteFixture_PaymentTransaction` |

## Modulspezifische harte Grenzwerte (v1.9.0)

| Gate-ID | Erwartungswert | Messregel |
|---|---|---|
| BAG-1 | >= 18000 txn/s (TPC-C NewOrder Throughput) | mean aus `TPCCLiteFixture_NewOrderTransaction` |
| BAG-2 | <= 35 ms (TPC-C StockLevel P95) | p95 aus `TPCCLiteFixture_StockLevelTransaction` |
| BAG-3 | <= 30 ms (VectorSearch P99) | p99 aus `BM_VectorSearch_efSearch` |
| BAG-4 | Regression <= 8 % gegen letzte Release-Baseline | `(current - baseline) / baseline` |

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