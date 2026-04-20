# PERFORMANCE_EXPECTATIONS — src/cdc

## Scope
- Modul: `src/cdc`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_changefeed_throughput.cpp`
  - `benchmarks/bench_cdc_pipeline.cpp`

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| CDC-1 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `ChangefeedBenchmarkFixture_ConcurrentSubscribers` |
| CDC-2 | Siehe Zielbeschreibung: Event Delivery P99 | `ChangefeedBenchmarkFixture_EventPolling` |
| CDC-3 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `CdcPipelineBenchFixture_AcknowledgeEvents` |
| CDC-4 | Siehe Zielbeschreibung: Resume nach 24h Offline (10M Events) | `CdcPipelineBenchFixture_FetchEventsAtLeastOnce` |
| CDC-5 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `ChangefeedBenchmarkFixture_EventRecordingThroughput` |
| CDC-6 | Siehe Zielbeschreibung: Log Compaction (1M Events) | `CdcPipelineBenchFixture_CreateDeleteGroup` |

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.
