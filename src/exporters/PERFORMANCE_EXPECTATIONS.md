# PERFORMANCE_EXPECTATIONS — src/exporters

## Scope
- Modul: `src/exporters`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Dieses Modul nutzt die Ziel-ID-Matrix des Parent-Moduls `analytics` als Referenzpfad.
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_olap_performance.cpp`
  - `benchmarks/bench_update_pipeline.cpp`
  - `benchmarks/bench_parquet_export.cpp`
  - `benchmarks/bench_csv_export.cpp`
  - `benchmarks/bench_arm_simd.cpp`

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| AN-1 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_OLAP_Count` |
| AN-2 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_UpdateStateMachine_Transition` |
| AN-3 | Siehe Zielbeschreibung: Parquet Export 1M Rows | `BM_ParquetExport_1M` |
| AN-4 | Siehe Zielbeschreibung: CSV Export 1M Rows | `BM_CsvExport_1M` |
| AN-5 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_UpdateStateMachine_RollbackPath` |
| AN-7 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_OLAP_GroupBy_SingleDim` |
| AN-8 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_OLAP_GroupBy_TwoDim` |
| AN-9 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_OLAP_ComplexQuery` |
| AN-10 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_ARM_DotProduct_NEON` |

## Modulspezifische harte Grenzwerte (v1.9.0)

| Gate-ID | Erwartungswert | Messregel |
|---|---|---|
| EXG-1 | >= 120 MB/s (Parquet Export Throughput) | mean aus `BM_ParquetExport_1M` |
| EXG-2 | >= 90 MB/s (CSV Export Throughput) | mean aus `BM_CsvExport_1M` |
| EXG-3 | <= 60 ms (OLAP Complex Query P99) | p99 aus `BM_OLAP_ComplexQuery` |
| EXG-4 | Regression <= 8 % gegen letzte Release-Baseline | `(current - baseline) / baseline` |

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