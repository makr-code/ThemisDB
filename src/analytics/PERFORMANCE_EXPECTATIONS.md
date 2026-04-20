# PERFORMANCE_EXPECTATIONS — src/analytics

## Scope
- Modul: `src/analytics`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_olap_performance.cpp`
  - `benchmarks/bench_update_pipeline.cpp`
  - `benchmarks/bench_exporters.cpp`
  - `benchmarks/bench_arm_simd.cpp`

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| AN-1 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_OLAP_Count` |
| AN-2 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_UpdateStateMachine_Transition` |
| AN-3 | Siehe Zielbeschreibung: Parquet Export 1M Rows | `BM_Export_Parquet_1M` |
| AN-4 | Siehe Zielbeschreibung: CSV Export 1M Rows | `BM_Export_CSV_1M` |
| AN-5 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_UpdateStateMachine_RollbackPath` |
| AN-7 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_OLAP_GroupBy_SingleDim` |
| AN-8 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_OLAP_GroupBy_TwoDim` |
| AN-9 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_OLAP_ComplexQuery` |
| AN-10 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_ARM_DotProduct_NEON` |

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.
