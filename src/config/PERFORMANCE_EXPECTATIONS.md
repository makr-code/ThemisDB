# PERFORMANCE_EXPECTATIONS — src/config

## Scope
- Modul: `src/config`
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

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.
