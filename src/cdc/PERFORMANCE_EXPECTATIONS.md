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

## Modulspezifische harte Grenzwerte (v1.9.0)

| Gate-ID | Erwartungswert | Messregel |
|---|---|---|
| CDCG-1 | >= 50000 events/s (Event Recording Throughput) | mean aus `ChangefeedBenchmarkFixture_EventRecordingThroughput` |
| CDCG-2 | <= 40 ms (Event Delivery P99) | p99 aus `ChangefeedBenchmarkFixture_EventPolling` |
| CDCG-3 | <= 80 ms (Ack Path P95) | p95 aus `CdcPipelineBenchFixture_AcknowledgeEvents` |
| CDCG-4 | Regression <= 8 % gegen letzte Release-Baseline | `(current - baseline) / baseline` |

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