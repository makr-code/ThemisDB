# PERFORMANCE_EXPECTATIONS — src/timeseries

## Scope
- Modul: `src/timeseries`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_timeseries_adaptive_flush.cpp`
  - `benchmarks/bench_gorilla_codec.cpp`
  - `benchmarks/bench_timeseries_ingestion.cpp`
  - `benchmarks/bench_security.cpp`

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| TS-1 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `AdaptiveFlushFixture_SingleThreaded` |
| TS-2 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_GorillaDecode_Sine` |
| TS-3 | Siehe Zielbeschreibung: Range Scan P99 (1M pts) | `TimeseriesBenchmarkFixture_TimeRangeQuery` |
| TS-4 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `AdaptiveFlushFixture_BatchWatermark` |
| TS-5 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `AdaptiveFlushFixture_MultiThreaded` |
| TS-6 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `TimeseriesBenchmarkFixture_RawDataIngestion` |
| TS-7 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_GorillaCompressionRatio` |
| TS-9 | Siehe Zielbeschreibung: Buffer-to-Storage Flush P99 | `AdaptiveFlushFixture_P99Latency` |
| TS-10 | Siehe Zielbeschreibung: Gorilla Insert P99 | `TimeseriesBenchmarkFixture_BatchIngestion` |
| TS-11 | Siehe Zielbeschreibung: AES-256-GCM Throughput | `BM_AES256GCM_Encrypt_1MB` |

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.
